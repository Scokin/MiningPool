#include "mempool.hpp"

void catch_int(int sig_num)
{
    for (int i = 4; i <= 14; i++)
    {
        close(i);
    }
    flag = 0;
}

/**
 * Threads handlers de la mempool, se utilizan para comunicarse con el
 * servidor central y con las Mining pools
 * 
 *  Servidor Central: Esta conexion se utiliza para borrar las transacciones
 * que fueron validadas con exito en la blockchain.
 * 
 *  Mining pools: Estas piden transacciones para armar bloques y validarlos
 * ademas de sumar transacciones a la mempool. 
 */

void *centralserverthread(void *ptr)
{
    int newsockfd = *((int *)ptr);
    delete (int *)ptr;
    MempoolCShandle CShandler(newsockfd);

    while (flag)
    {
        CShandler.handleCentralServer();
    }
    close(newsockfd);
    pthread_exit(NULL);
}

void *MiningPoolsthread(void *ptr)
{
    int newsockfd = *((int *)ptr);
    MempoolMPhandle MPhandle(newsockfd);

    while (flag)
    {
        MPhandle.handleMiningPool();
    }
    close(newsockfd);
    pthread_exit(NULL);
}

/**
 * Definicion de las funciones que utiliza la Mempool para comunicarse con el servidor Central
*/

void MempoolCShandle::recvControl()
{
    int ret = recvMsg(CentralServerSocket, &msg);
    assert(ret == 1);
    getControl(&msg, &ctrl);
    return;
}

void MempoolCShandle::sendControl(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    int ret = sendMsg(CentralServerSocket, &msg);
    assert(ret == 1);
    return;
}

void MempoolCShandle::delete_transactions(Mempool aux)
{
    for (int i = 0; i < aux.cantidad; i++)
    {
        for (int k = 0; k < cantidad; k++)
        {
            if (aux.trans[i] == tran[k])
            {
                for (int t = k; t < cantidad - 1; t++)
                {
                    tran[t] = tran[t + 1];
                }
                cantidad--;
                k--;
                break;
            }
        }
    }
}

void MempoolCShandle::handleCentralServer()
{
    recvControl();
    if (ctrl.controlmessage == NEW_BLOCK)
    {
        pthread_mutex_lock(&lock_x);
        sendControl(SEND_TRANSACTIONS);
        recvMsg(CentralServerSocket, &msg);
        Mempool aux(&msg);
        delete_transactions(aux);
        sendControl(Correctly_deleted);
        pthread_mutex_unlock(&lock_x);
    }
}

/**
 * Definicion de las funciones que utiliza la Mempool para comunicarse con las Mining Pools.
*/

void MempoolMPhandle::recvControl()
{
    int ret = recvMsg(Miningpoolsocket, &msg);
    assert(ret == 1);
    getControl(&msg, &ctrl);
    return;
}
void MempoolMPhandle::sendControl(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    int ret = sendMsg(Miningpoolsocket, &msg);
    assert(ret == 1);
    return;
}
void MempoolMPhandle::addTransactions(Mempool aux)
{
    if (aux.cantidad > (MAX_TRANSACTIONS - cantidad))
    {
        perror("overflow de mempool");
        return;
    }
    for (int i = 0; i < aux.cantidad; i++)
    {
        tran[cantidad] = aux.trans[i];
        cantidad++;
    }
    return;
}
void MempoolMPhandle::handleMiningPool()
{
    recvControl();
    if (ctrl.controlmessage == Send_Mempool)
    {
        sendControl(Sending_Mempool);
        pthread_mutex_lock(&lock_x);
        setMempool(&msg, tran[0], cantidad);
        pthread_mutex_unlock(&lock_x);
        sendMsg(Miningpoolsocket, &msg);
        return;
    }
    if (ctrl.controlmessage == NEW_TRANSACTION)
    {
        sendControl(SEND_NEW_TRANSACTION);
        recvMsg(Miningpoolsocket, &msg);
        pthread_mutex_lock(&lock_x);
        Mempool aux(&msg);
        addTransactions(aux);
        pthread_mutex_unlock(&lock_x);
        sendControl(Agregadas_Exitosamente);
        return;
    }
}
/**
 *  Definicion de las funciones que permiten conectarse al servidor central y armar el servidor para que
 * se conecten las mining pools.
*/
int MempoolServer::SetMempool()
{
    listeningsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningsocket < 0)
        perror("ERROR opening socket");
    bzero((char *)&Mempool_addr, sizeof(Mempool_addr));
    portnoMempool = 8000;
    Mempool_addr.sin_family = AF_INET;
    Mempool_addr.sin_addr.s_addr = INADDR_ANY;
    Mempool_addr.sin_port = htons(portnoMempool);
    setsockopt(listeningsocket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (bind(listeningsocket, (struct sockaddr *)&Mempool_addr, sizeof(Mempool_addr)) < 0)
        perror("ERROR on binding");
    listen(listeningsocket, 5);
    clilen = sizeof(Mempool_addr);
    return 1;
}
int MempoolServer::ConnecttoCentralServer()
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
    ctrl.controlmessage = Connection_From_MemPool;
    setControl(&msg, ctrl);
    sendMsg(centralserversocket, &msg);
    return 1;
}

int main()
{
    int newsockfd;
    pthread_t thread[MAX_THREADS];
    pthread_t centralserverconnection;
    signal(SIGINT, catch_int);

    MempoolServer Mempool_server;
    Mempool_server.SetMempool();
    Mempool_server.ConnecttoCentralServer();

    std::cout << "La mempool se conecto con exito al servidor central." << std::endl;
    int *data = new int;
    *data = Mempool_server.centralserversocket;

    pthread_mutex_init(&lock_x, NULL);

    if (pthread_create(&centralserverconnection, NULL, centralserverthread, (void *)data) != 0)
    {
        perror("pthread_create");
        delete data;
        close(newsockfd);
    }

    while (flag && (j <= MAX_THREADS))
    {
        newsockfd = accept(Mempool_server.listeningsocket, (struct sockaddr *)&Mempool_server.MiningPool_addr, &Mempool_server.clilen);
        if (newsockfd < 0)
            perror("ERROR on accept");
        std::cout << "Una Mining pool se conecto con exito a la mempool" << std::endl;

        if (pthread_create(&thread[j], NULL, MiningPoolsthread, (void *)&newsockfd) != 0)
        {
            perror("pthread_create");
            close(newsockfd);
        }
        else
        {
            j++;
        }
    }

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_join(thread[i], NULL);
    }
    close(Mempool_server.listeningsocket);
    pthread_mutex_destroy(&lock_x);
    return 0;
}