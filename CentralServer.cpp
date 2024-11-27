#include "central_server.hpp"

//Thread que administra la construccion de la Blockchain y la comunicacion con la Mempool.

void *Blockchainbuilder(void *ptr)
{
    //Extraccion de socket de la mempool.
    int newsockfd = *((int *)ptr);
    CentralServerMempoolHandler Mempoolhandler(newsockfd);
    pthread_mutex_lock(&lock_x);
    while (1)
    {   
        //Esperar a que haya bloque para agregar.
        while (!agregar_bloque)
        {
            pthread_cond_wait(&cond_agregar_bloque, &lock_x);
        }
        agregar_bloque = 0;

        // Agregar bloque a la blockchain.
        BC.addBlock(aux);
        BC.printlastblock();
        // Borra de la mempool las transacciones de este bloque.
        Mempoolhandler.SendNewBlockTransaction();
        // Se actualiza el hash del "bloque anterior" de la blockchain.
        BC.netwheader.prevBlockHash = previoushash;
        // Bloque agregado.
        validado = 1;
        // Se avisa al otro thread que se agrego el bloque con exito.
        pthread_cond_signal(&cond_validado);
        std::cout << "La Blockchain tiene " << BC.numberofblocks << " bloques." << std::endl;
    }
    pthread_mutex_unlock(&lock_x);
    pthread_exit(NULL); 
}


//Thread que administra la comunicacion del servidor central con las MiningPools.
void *MPhandlethread(void *ptr)
{
    //Extraccion de socket de la miningpool.
    int newsockfd = *((int *)ptr);
    delete (int*)ptr;
    CentralServerMiningPoolHandler Miningpooolhandler(newsockfd);

    Control thread_ctrl;
    Block blk;
    unsigned char blhash[hashlenght];
    BlkHeader_packed header;

    while (1)
    {
        //Se manejan los mensajes de la Miningpool.
        int ret = Miningpooolhandler.recvControl(thread_ctrl);
        if (ret == -1)
        {
            close(newsockfd);
        }
        switch (thread_ctrl.controlmessage)
        {
        case Ask_For_Current_Header:
            //Se envia el header actual
            Miningpooolhandler.sendControl(Sending_Current_Header);
            header = BC.network_header(); 
            Miningpooolhandler.sendHeader(header);
            break;
        case Got_Valid_Block:
            //Se verifica la validez del bloque que envia la Miningpool.
            Miningpooolhandler.sendControl(Send_Valid_Block);
            Miningpooolhandler.recvBlock(blk);
            hash(&(blk.header), blhash);
            if (validhash(blhash, header.DiffTarget) && (blk.header.prevBlockHash == previoushash))
            {
                //Si es valido se agrega a la blockchain y se manda un mensaje de aceptacion.
                char2hash(&previoushash, blhash);
                pthread_mutex_lock(&lock_x);
                aux = blk;
                agregar_bloque = 1;
                pthread_cond_signal(&cond_agregar_bloque);
                while (!validado)
                {
                    pthread_cond_wait(&cond_validado, &lock_x);
                }
                validado = 0;
                Miningpooolhandler.sendControl(Validated);
                
                pthread_mutex_unlock(&lock_x);
            }
            else
            {   
                //Si no es valido se devuelve un mensaje de rechazo.
                Miningpooolhandler.sendControl(Not_validated);
            }
            break;
        default:
            break;
        }
    }
    close(newsockfd);
    pthread_exit(NULL);
}
//Thread que administra la comunicacion del servidor central con las Wallets.
void *WalletHandler(void *ptr)
{
    int newsockfd = *((int *)ptr);
    CentralServerWalletHandler WH(newsockfd);
    Control ctrl;
    uint8_t id;
    Blockchain aux;
    WH.recvControl(ctrl);
    int monedas;
    /**
     *  Si la wallet pide monedas, se hace una copia de la Blockchain hasta ese momento y se calcula el
     * monto de monedas que tiene la ID hasta ese momento.
    */
    if(ctrl.controlmessage == Ask_4_coins)
    {
        WH.sendControl(Ask_4_ID);
        recvMsg(WH.WalletSocket,&WH.msg);
        getID(&WH.msg,&id);
        pthread_mutex_lock(&lock_x);
        aux = BC;
        pthread_mutex_unlock(&lock_x);
        monedas = aux.monedas(id);
        setCoins(&WH.msg,(uint32_t)monedas);
        sendMsg(WH.WalletSocket,&WH.msg);
    }
    close(newsockfd);
    pthread_exit(NULL);
}

//Funciones para simplificar mandar y recibir mensajes de control.
int CentralServerMiningPoolHandler::recvControl(Control& thread_ctrl)
{
    int ret = recvMsg(Miningpoolsocket, &msg);
    getControl(&msg, &thread_ctrl);
    return ret;
}
int CentralServerMiningPoolHandler::sendControl(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    int ret = sendMsg(Miningpoolsocket, &msg);
    return ret;
}


//Envia el Header a las Mining pools que lo pidan
int CentralServerMiningPoolHandler::sendHeader(BlkHeader_packed& header)
{
    setHeader(&msg,header);
    int ret = sendMsg(Miningpoolsocket,&msg);
    return ret;
}

//Recibe el bloque "Valido" de las MiningPools

int CentralServerMiningPoolHandler::recvBlock(Block& blk)
{
    int ret = recvMsg(Miningpoolsocket,&msg);
    Block aux(&msg);
    blk = aux;
    return ret;
}

//Funciones para simplificar mandar y recibir mensajes de control.
int CentralServerMempoolHandler::recvControl()
{
    int ret = recvMsg(Mempoolsocket, &msg);
    getControl(&msg, &ctrl);
    return ret;
}
int CentralServerMempoolHandler::sendControl(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    int ret = sendMsg(Mempoolsocket, &msg);
    return ret;
}
/**
 *  Le envia a la mempool las transacciones que se validaron en el
 * ultimo bloque para que esta las elimine de la pool de transacciones
 * disponibles.
 */


void CentralServerMempoolHandler::SendNewBlockTransaction()
{
    sendControl(NEW_BLOCK);
    recvControl();
    if (ctrl.controlmessage == SEND_TRANSACTIONS)
    {
        setMempool(&msg, BC.blockchain[BC.numberofblocks - 1].stack[0], BC.blockchain[BC.numberofblocks - 1].cantidad);
        sendMsg(Mempoolsocket, &msg);
        recvControl();
        if (ctrl.controlmessage == Correctly_deleted)
        {
            return;
        }
        else
        {
            perror("No se borro correctamente");
            exit(1);
        }
    }
}

//Funciones para simplificar mandar y recibir mensajes de control.

int CentralServerWalletHandler::recvControl(Control &thread_ctrl)
{
    int ret = recvMsg(WalletSocket, &msg);
    getControl(&msg, &thread_ctrl);
    return ret;
}
int CentralServerWalletHandler::sendControl(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    int ret = sendMsg(WalletSocket, &msg);
    return ret;
}

int CentralServer::SetTheServer()
{
    listeningsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningsocket < 0)
        perror("ERROR opening socket");
    bzero((char *)&CentralServer_addr, sizeof(CentralServer_addr));
    portno = 7000;
    CentralServer_addr.sin_family = AF_INET;
    CentralServer_addr.sin_addr.s_addr = INADDR_ANY;
    CentralServer_addr.sin_port = htons(portno);
    setsockopt(listeningsocket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (bind(listeningsocket, (struct sockaddr *)&CentralServer_addr, sizeof(CentralServer_addr)) < 0)
        perror("ERROR on binding");
    listen(listeningsocket, 5);
    clilen = sizeof(MiningPool_addr);
    return 1;
}

int main()
{
    CentralServer centralserver;
    Block genesis;
    BC.addBlock(genesis);
    centralserver.SetTheServer();

    std::cout << "El Servidor central de la blockchain fue desplegado con exito." << std::endl;

    pthread_mutex_init(&lock_x, NULL);
    pthread_cond_init(&cond_agregar_bloque, NULL);
    pthread_cond_init(&cond_validado, NULL);
    int newsockfd;
    pthread_t threadsMiningPool[MAX_THREADS];
    pthread_t blockchainbuilderthread;
    pthread_t Wallet;
    Msg msg;
    Control ctrl;

    int j = 0;
    while (j < MAX_THREADS)
    {
        newsockfd = accept(centralserver.listeningsocket, (struct sockaddr *)&centralserver.MiningPool_addr, &centralserver.clilen);
        if (newsockfd < 0)
            perror("ERROR on accept");

        recvMsg(newsockfd, &msg);
        getControl(&msg, &ctrl);
        //Se determina que tipo de cliente intenta conectarse al Servidor Central.
        if (ctrl.controlmessage == Connection_From_MiningPool)
        {
            std::cout << "Una Mining pool se conecto con exito al servidor central." << std::endl;
            int *sockfd_ptr = new int(newsockfd);
            if (pthread_create(&threadsMiningPool[j], NULL, MPhandlethread, (void *)sockfd_ptr) != 0)
            {
                perror("pthread_create");
                close(newsockfd);
                delete sockfd_ptr;
            }
            else
            {
                j++;
            }
        }
        else if (ctrl.controlmessage == Connection_From_MemPool)
        {
            std::cout << "El Servidor central se conecto exitosamente con la Mempool." << std::endl;
            int *sockfd_ptr = new int(newsockfd);
            if (pthread_create(&blockchainbuilderthread, NULL, Blockchainbuilder, (void *)sockfd_ptr) != 0)
            {
                perror("pthread_create");
                close(newsockfd);
                delete sockfd_ptr;
            }
        }
        else if (ctrl.controlmessage == Connection_From_Wallet)
        {
             std::cout << "Una Wallet se conecto con exito al servidor central." << std::endl;
            int *sockfd_ptr = new int(newsockfd);
            if (pthread_create(&Wallet, NULL, WalletHandler, (void *)sockfd_ptr) != 0)
            {
                perror("pthread_create");
                close(newsockfd);
                delete sockfd_ptr;
            }
        }
        else
        {
            close(newsockfd);
        }
    }

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_join(threadsMiningPool[i], NULL);
    }
    pthread_join(blockchainbuilderthread, NULL);
    pthread_join(Wallet, NULL);

    pthread_mutex_destroy(&lock_x);
    pthread_cond_destroy(&cond_agregar_bloque);
    pthread_cond_destroy(&cond_validado);
    close(centralserver.listeningsocket);
    return 0;
}