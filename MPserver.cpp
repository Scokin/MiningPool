#include "Mining_pool.hpp"

//Thread que administra la comunicacion con la mempool y el central server.

void *serverandmempool(void *ptr)
{
    //Extraccion de sockets de la mempool y el server central de los argumentos del thread.
    thread_args data = *((thread_args *)ptr);
    S.mempoolsocket = data.mempoolsocket;
    S.centralserversocket = data.centralserversocket;
    //Transaccionderecompensa.
    S.blockprice();
    
    //Inicializacion de variables para estadistica de velocidad de hasheo y seguimiento de progreso.
    double result = pow(2, pooldiff);
    int timeinmil;
    headernumber = 0;

    pthread_mutex_lock(&lock_x);
    while (flag)
    {
        /**
         *  Construccion de bloque a hashear, con base en el header del servidor central y
         * de transacciones en la mempool. Se llama a funciones dentro de ServerandMempooolHandler
         * que de forma secuencial arman un bloque.
         */ 
        
        if(!S.getcurrentheader())
        {
            perror("Error getting currentheader");
            exit(1);
        }
        if(!S.askmempool())
        {
            perror("Error getting mempool");
            exit(1);
        }
        S.buildblock();
        //Bools para controlar las esperas en las variables condicionales.
        Header_valido = 0;
        Header_disponible = 1;
        //Se "borran" las transacciones guardadas en la lista de transacciones local de la MiningPool.  
        miningpooltransactions.cantidad = 0;
        
        //Se asigna el header del bloque construido.
        Header_para_minar = S.bloque.header;
        test = S.bloque.header.prevBlockHash;

        /**
         *  Reseteo de las variables que controlan el flujo de trabajo de los 
         * otros threads.
        */
        totalshares = 0;
        S.Noncecorrecto = 0;
        nuevoBloque = 0;
        terminaron = 0;

        
        //Broadcast para despertar a todos los threads que esperan header.
        pthread_cond_broadcast(&cond_Header_disponible);

        //Broadcast para aceptar a los nuevos Mineros que se unen a la Miningpool.
        if (waiting_thread)
        {
            pthread_cond_broadcast(&cond_waiting_thread);
            waiting_thread = 0;
        }

        //Se empieza a medir el tiempo para minar un bloque.
        S.Tstart = std::chrono::steady_clock::now();

        //Se espera a que los threads mineros avisen que se mino un bloque con exito.
        while (!Header_valido && flag)
        {
            pthread_cond_wait(&cond_Header_valido, &lock_x);
        }

        /**
         *  Condicion para verificar si terminaron los threads mineros con un hash
         * exitoso o porque se agrego un nuevo bloque a la blockchain desde otra
         * Miningpool.
         */
        if (!nuevoBloque)
        {
            //Se termina de medir el tiempo para minar un bloque.
            S.Tend = std::chrono::steady_clock::now();
            timeinmil = std::chrono::duration_cast<std::chrono::milliseconds>(S.Tend - S.Tstart).count();
            //Se asigna el nuevo Nonce al bloque armado
            S.bloque.header.Nonce = Header_minado.Nonce;
            
            /**
             *  Se manda el bloque minado al central server, si es valido se paga a los
             * mineros, se suma en uno la cantidad de bloques minados y se imprimen 
             * estadisticas de velocidad de la MiningPool.
             */
            if (S.sendblock())
            {
                S.sendpay2miners();
                std::cout << "La mining pool tiene una velocidad de hasheo de: ";
                std::cout << static_cast<long>((result * totalshares) / static_cast<double>(timeinmil));
                std::cout << " kiloHashes/s" << std::endl;
                headernumber++;
                std::cout << "Esta MiningPool puso: " << headernumber << " bloques en la Blockchain." << std::endl;
            }
            else
            {
                std::cout << "El bloque fue rechazado." << std::endl;
            }
        }
    }
    //Se cierran los sockets de comunicacion.
    close(S.centralserversocket);
    close(S.mempoolsocket);
    //Cuando se cierra el programa se libera el mutex y se sale del pthread
    pthread_mutex_unlock(&lock_x);
    pthread_exit(NULL);

}

//Thread que administra la comunicacion con un minero.

void *Minerhandle(void *ptr)
{   
    //Extraccion del socket del minero y su ID.
    Miner_thread_args mdata = *((Miner_thread_args *)ptr);
    int newsockfd = mdata.minersocket;
    uint8_t ID = mdata.ID;
    
    /**
     *  Variable para controlar cada cuanto se pregunta si alguien mas agrego un
     * bloque a la blockchain.
    */
    
    int check = 0;

    //Header local del thread para checkeo de nuevo bloque
    BlkHeader_packed checkheader;

    /**
     *  Cualquier minero menos el primero que quiera unirse debera a esperar a
     * que el bloque actual termine.
    */
    pthread_mutex_lock(&lock_x);
    if (j != 1)
    {
        waiting_thread = 1;
        while (waiting_thread)
        {
            pthread_cond_wait(&cond_waiting_thread, &lock_x);
        }
    }
    //Se agrega el minero al distribuidor de rangos.
    RD.addthread(newsockfd);
    pthread_mutex_unlock(&lock_x);
    //Se inicializa la clase handler de el thread minero.
    MPhandler MPh(newsockfd, ID);
    while (flag)
    {
        //Se llama la funcion que maneja los mensajes de los mineros.
        MPh.handlerequest();
        /**
         *  Logica para verificar si hay nuevo bloque en la blockchain. Tiene la
         * ventaja de que mientras mas Mineros esten minando, antes se descubrira si se
         * agrego un nuevo bloque.
         */   
        pthread_mutex_unlock(&lock_x);
        check++;
        if ((check == checknewblock) && (nuevoBloque !=0))
        {
            pthread_mutex_lock(&lock_x);
            check = 0;
            S.getcurrentheader();
            checkheader = S.head;
            if (!(checkheader.prevBlockHash == test))
            {
                S.Noncecorrecto = 1;
                nuevoBloque = 1;
                terminaron = 0;
            }
            pthread_mutex_unlock(&lock_x);
        }
    }
    close(newsockfd);
    pthread_exit(NULL);
}

//Funcion que pide la pool de transacciones a la Mempool.

int ServerandMempooolHandler::askmempool()
{
    controlsend(Send_Mempool, mempoolsocket);
    controlrecv(mempoolsocket);
    if (ctrl.controlmessage == Sending_Mempool)
    {
        ret = recvMsg(mempoolsocket, &msg);
        assert(ret == 1);
        Mempool memaux(&msg);
        mem = memaux;
        return 1;
    }
    else
    {
        return 0;
    }
}

//Funcion que pide el header actual al servidor central.

int ServerandMempooolHandler::getcurrentheader()
{
    controlsend(Ask_For_Current_Header, centralserversocket);
    controlrecv(centralserversocket);
    if (ctrl.controlmessage == Sending_Current_Header)
    {
        ret = recvMsg(centralserversocket, &msg);
        assert(ret == 1);
        getHeader(&msg, &head);
        return 1;
    }
    else
    {
        return 0;
    }
}

//Funcion que ensambla el bloque.

void ServerandMempooolHandler::blockprice()
{
    Blockearning.fee = 0;
    Blockearning.quantity = 255;
    Blockearning.receiver = MP_address;
    Blockearning.sender = 255; 
}

void ServerandMempooolHandler::buildblock()
{
    bloque.cantidad = mem.cantidad + 1;
    bloque.header = head;
    auto now = std::chrono::high_resolution_clock::now();
    bloque.header.Timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    merklerootcalc(bloque.header.MerkleRoot, mem);
    bloque.stack[0] = Blockearning;
    std::memcpy(&bloque.stack[1], mem.trans, sizeof(Transaccion_packed) * mem.cantidad);
    return;
}

//Funcion que envia el bloque al servidor central.

int ServerandMempooolHandler::sendblock()
{
    controlsend(Got_Valid_Block, centralserversocket);
    controlrecv(centralserversocket);
    if (ctrl.controlmessage == Send_Valid_Block)
    {
        size_t sz8 = sizeof(Block_packed) + sizeof(Transaccion_packed) * (mem.cantidad + 1) + sizeof(uint8_t);
        Block_packed *blk = (Block_packed *)malloc(sz8);
        blk->cantidad = mem.cantidad + 1;
        blk->header = bloque.header;
        std::memcpy(blk->stack, bloque.stack, sizeof(Transaccion_packed) * (mem.cantidad + 1));
        
        setBlock(&msg, *blk);    
        free(blk);
        sendMsg(centralserversocket, &msg);
        controlrecv(centralserversocket);

        if (ctrl.controlmessage == Validated)
        {
            printheader(blk->header);
            printMempool(blk->stack, blk->cantidad);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

//Funciones para simplificar mandar y recibir mensajes de control.

void ServerandMempooolHandler::controlsend(uint32_t controlm, int socket)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    ret = sendMsg(socket, &msg);
    assert(ret == 1);
}
void ServerandMempooolHandler::controlrecv(int socket)
{
    ret = recvMsg(socket, &msg);
    assert(ret == 1);
    getControl(&msg, &ctrl);
    return;
}


//Se envia la paga a los mineros de las colaboraciones realizadas para el minado del bloque.

void ServerandMempooolHandler::sendpay2miners()
{
    controlsend(NEW_TRANSACTION, mempoolsocket);
    controlrecv(mempoolsocket);
    if (ctrl.controlmessage == SEND_NEW_TRANSACTION)
    {
        setMempool(&msg, miningpooltransactions.trans[0], miningpooltransactions.cantidad);
        sendMsg(mempoolsocket, &msg);
    }
    controlrecv(mempoolsocket);
    if (ctrl.controlmessage == Agregadas_Exitosamente)
    {
        return;
    }
}

/**
 * Funciones para agregar y eliminar mineros del rango de distribucion de Nonce. 
 */

void RangeDistribution::addthread(int socket)
{
    std::lock_guard<std::mutex> lock(mtx);
    sockets.push_back(socket);
};

void RangeDistribution::deletethread(int socket)
{
    std::lock_guard<std::mutex> lock(mtx);
    sockets.erase(std::remove(sockets.begin(), sockets.end(), socket), sockets.end());
}
uint32_t RangeDistribution::distributeranges(int socket)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = std::find(sockets.begin(), sockets.end(), socket);
    if (it == sockets.end())
    {
        exit(0);
    }
    size_t index = std::distance(sockets.begin(), it);
    size_t cantidad = sockets.size();
    return maxR / cantidad * index;
}
int RangeDistribution::cantidaddemineros()
{
    return sockets.size();
}

//Maneja la salida del programa 

void catch_int(int sig_num)
{
    for (int i = 4; i <= 14; i++)
    {
        close(i);
    }
    flag = 0;
    pthread_cond_signal(&cond_Header_valido);
    Header_valido = 1;
}

//Funciones para simplificar mandar y recibir mensajes de control.

void MPhandler::control_send(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    int ret = sendMsg(socket, &msg);
    assert(ret == 1);
    return;
}

void MPhandler::controlrecv()
{
    int ret = recvMsg(socket, &msg);
    assert(ret == 1);
    getControl(&msg, &ctrl);
    return;
}

// Contesta los mensajes de control de los Mineros.
void MPhandler::handlerequest()
{
    if (recvMsg(socket, &msg) != 1)
    {
        //Si algun minero se desconecta lo saca de la lista de rangos.
        j--;
        RD.deletethread(socket);
        pthread_exit(NULL);
    }
    getControl(&msg, &ctrl);

    switch (ctrl.controlmessage)
    {
    case Ask_For_Header:
        requestedheader();
        break;

    case Ask_For_State:
        requestedstate();
        break;

    case Got_Valid_share:
        gotshare();
        break;

    case Got_Valid_Nonce:
        gotnonce();
        break;

    default:
        break;
    }
}

//Entrega el header a minar al minero cuando este lo pide.
void MPhandler::requestedheader()
{
    pthread_mutex_lock(&lock_x);
    while (!Header_disponible)
    {
        pthread_cond_wait(&cond_Header_disponible, &lock_x);
    }
    headnum = headernumber;
    Header_para_minar_thread = Header_para_minar;

    pthread_mutex_unlock(&lock_x);
    control_send(Sending_Header);
    Header_para_minar_thread.Nonce = RD.distributeranges(socket);
    setHeader(&msg, Header_para_minar_thread);
    share = 0;
    sendMsg(socket, &msg);
}
//Contesta al minero si se encontro un header o que siga buscando.
void MPhandler::requestedstate()
{
    pthread_mutex_lock(&lock_x);
    if (S.Noncecorrecto == 1 && nuevoBloque == 0)
    {

        control_send(Nonce_Was_Found);
        terminaron++;
        payminer();

        if (terminaron == RD.cantidaddemineros())
        {
            Header_valido = 1;
            pthread_cond_signal(&cond_Header_valido);
        }
        pthread_mutex_unlock(&lock_x);
        std::cout << " las shares de este minero son: " << share << " las shares totales son: " << totalshares << std::endl;
        return;
    }
    if (S.Noncecorrecto == 0)
    {
        control_send(Not_Yet_Nonce);
        pthread_mutex_unlock(&lock_x);
        return;
    }
    if (S.Noncecorrecto == 1 && nuevoBloque == 1)
    {
        control_send(Nonce_Was_Found);
        terminaron++;
        if (terminaron == RD.cantidaddemineros())
        {
            Header_valido = 1;
            pthread_cond_signal(&cond_Header_valido);
        }
        pthread_mutex_unlock(&lock_x);
        return;
    }
}
//Valida la Share del minero.
void MPhandler::gotshare()
{
    control_send(Send_your_share);
    recvMsg(socket, &msg);
    getHeader(&msg, &Header_minado_thread);
    hash(&Header_minado_thread, blhash);
    if (validhash(blhash, pooldiff))
    {
        share++;
        pthread_mutex_lock(&lock_x);
        totalshares++;
        pthread_mutex_unlock(&lock_x);
    }
}
//Recibe el header validado por el minero.
void MPhandler::gotnonce()
{
    pthread_mutex_lock(&lock_x);
    if ((S.Noncecorrecto == 0) && (headernumber == headnum))
    {
        control_send(Send_Your_Nonce);
        recvMsg(socket, &msg);
        getHeader(&msg, &Header_minado_thread);
        Header_minado = Header_minado_thread;
        S.Noncecorrecto = 1;
        Header_disponible = 0;
        share++;
        totalshares++;
        terminaron++;
        std::cout << " las shares de este minero son: " << share << " las shares totales son: " << totalshares << std::endl;
        payminer();
        if (terminaron == RD.cantidaddemineros())
        {
            Header_valido = 1;
            pthread_cond_signal(&cond_Header_valido);
        }
        pthread_mutex_unlock(&lock_x);
        return;
    }
    if (S.Noncecorrecto == 1 && nuevoBloque == 1)
    {
        control_send(Nonce_Was_Found);
        terminaron++;
        if (terminaron == RD.cantidaddemineros())
        {
            Header_valido = 1;
            pthread_cond_signal(&cond_Header_valido);
        }
        pthread_mutex_unlock(&lock_x);
        return;
    }
    else
    {
        terminaron++;
        control_send(Nonce_Was_Found);
        payminer();
        if (terminaron == RD.cantidaddemineros())
        {
            Header_valido = 1;
            pthread_cond_signal(&cond_Header_valido);
        }
        pthread_mutex_unlock(&lock_x);
        return;
    }
}
//Agrega transaccion para pagar al minero por sus shares.
void MPhandler::payminer()
{
    aux.fee = 0;
    aux.quantity = (int)(payperblock * ((float)share / (float)totalshares));
    aux.sender = MP_address;
    aux.receiver = ID;
    miningpooltransactions.trans[miningpooltransactions.cantidad] = aux;
    miningpooltransactions.cantidad++;
}


// Se debe poner puerto e ID de la Minepool.

int main(int argc, char *argv[])
{
    if (argc < 3) {
         fprintf(stderr,"Error de argumentos\n");
         exit(1);
     }
    int MPport = atoi(argv[1]);
    MP_address = atoi(argv[2]);
    int newsockfd;
    Msg msg;
    uint8_t ID;
    Control ctrl;
    pthread_t thread[MAX_THREADS];
    pthread_t connection;
    signal(SIGINT, catch_int);
    MPS mining_pool_server(MPport);
    mining_pool_server.SetTheServer();
    mining_pool_server.ConnecttoMempool();
    mining_pool_server.ConnecttoCentralServer();
    struct thread_args *data = new thread_args;
    data->centralserversocket = mining_pool_server.centralserversocket;
    data->mempoolsocket = mining_pool_server.mempoolsocket;

    pthread_mutex_init(&lock_x, NULL);
    pthread_cond_init(&cond_Header_valido, NULL);
    pthread_cond_init(&cond_Header_disponible, NULL);
    pthread_cond_init(&cond_waiting_thread, NULL);

    if (pthread_create(&connection, NULL, serverandmempool, (void *)data) != 0)
    {
        perror("pthread_create");
        close(mining_pool_server.centralserversocket);
        close(mining_pool_server.mempoolsocket);
    }
    struct Miner_thread_args *mdata = new Miner_thread_args;
    while (flag && (j <= MAX_THREADS))
    {
        newsockfd = accept(mining_pool_server.listeningsocket, (struct sockaddr *)&mining_pool_server.Miner_addr, &mining_pool_server.clilen);
        if (newsockfd < 0)
            perror("ERROR on accept");

        ctrl.controlmessage = Send_Miner_ID;
        setControl(&msg, ctrl);
        sendMsg(newsockfd, &msg);
        recvMsg(newsockfd, &msg);
        getControl(&msg, &ctrl);
        if (ctrl.controlmessage == Sending_Miner_ID)
        {
            recvMsg(newsockfd, &msg);
            getID(&msg, &ID);
            setDiff(&msg, pooldiff);
            sendMsg(newsockfd, &msg);
            mdata->ID = ID;
            mdata->minersocket = newsockfd;

            if (pthread_create(&thread[j], NULL, Minerhandle, (void *)mdata) != 0)
            {
                perror("pthread_create");
                close(newsockfd);
            }
            else
            {
                j++;
            }
        }
    }

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_join(thread[i], NULL);
    }
    delete data;
    delete mdata;
    close(mining_pool_server.listeningsocket);
    pthread_mutex_destroy(&lock_x);
    pthread_cond_destroy(&cond_Header_valido);
    pthread_cond_destroy(&cond_Header_disponible);
    pthread_cond_destroy(&cond_waiting_thread);
    return 0;
}
int MPS::SetTheServer()
{
    listeningsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningsocket < 0)
        perror("ERROR opening socket");
    bzero((char *)&MiningPool_addr, sizeof(MiningPool_addr));
    MiningPool_addr.sin_family = AF_INET;
    MiningPool_addr.sin_addr.s_addr = INADDR_ANY;
    MiningPool_addr.sin_port = htons(portnoMP);
    setsockopt(listeningsocket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (bind(listeningsocket, (struct sockaddr *)&MiningPool_addr, sizeof(MiningPool_addr)) < 0)
        perror("ERROR on binding");
    listen(listeningsocket, 5);
    clilen = sizeof(Miner_addr);
    return 1;
}
int MPS::ConnecttoMempool()
{
    portnoMempool = 8000;
    mempoolsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mempoolsocket < 0)
        perror("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&Mempool_addr, sizeof(Mempool_addr));
    Mempool_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&Mempool_addr.sin_addr.s_addr,
          server->h_length);
    Mempool_addr.sin_port = htons(portnoMempool);
    if (connect(mempoolsocket, (struct sockaddr *)&Mempool_addr, sizeof(Mempool_addr)) < 0)
        perror("ERROR connecting");
    std::cout << "Conexion exitosa con la Mempool." << std::endl;
    return 1;
}
int MPS::ConnecttoCentralServer()
{
    portnoCentralServer = 7000;
    centralserversocket = socket(AF_INET, SOCK_STREAM, 0);
    if (centralserversocket < 0)
        perror("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&CentralServer_addr, sizeof(CentralServer_addr));
    CentralServer_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&CentralServer_addr.sin_addr.s_addr,
          server->h_length);
    CentralServer_addr.sin_port = htons(portnoCentralServer);
    if (connect(centralserversocket, (struct sockaddr *)&CentralServer_addr, sizeof(CentralServer_addr)) < 0)
        perror("ERROR connecting");
    ctrl.controlmessage = Connection_From_MiningPool;
    setControl(&msg, ctrl);
    sendMsg(centralserversocket, &msg);
    std::cout << "Conexion exitosa con el Central Server." << std::endl;
    return 1;
}