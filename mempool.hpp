#ifndef MEMPOOL_HPP
#define MEMPOOL_HPP
#include "ProtocoloMPS.hpp"

#define MAX_THREADS 10
bool flag = 1;
Transaccion_packed tran[MAX_TRANSACTIONS];
int cantidad = 0;
pthread_mutex_t lock_x;
int j = 0;

class MempoolCShandle
{
    int CentralServerSocket;
    Msg msg;
    Control ctrl;

public:
    MempoolCShandle(int serversocket) : CentralServerSocket(serversocket) {}
    void delete_transactions(Mempool aux);
    void recvControl();
    void sendControl(uint32_t controlm);
    void handleCentralServer();
};

class MempoolMPhandle
{
    int Miningpoolsocket;
    Msg msg;
    Control ctrl;

public:
    MempoolMPhandle(int MPsocket) : Miningpoolsocket(MPsocket) {}
    void recvControl();
    void sendControl(uint32_t controlm);
    void addTransactions(Mempool aux);
    void handleMiningPool();
};

class MempoolServer
{
    int portnoMempool;
    int portnoCentralServer;
    int val = 1;
    Control ctrl;
    Msg msg;
public:
    int listeningsocket;
    int centralserversocket;
    socklen_t clilen;
    struct hostent *server;
    struct sockaddr_in MiningPool_addr, CentralServer_addr, Mempool_addr;
    int SetMempool();
    int ConnecttoCentralServer();
};

#endif