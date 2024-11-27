#ifndef CENTRAL_SERVER_HPP
#define CENTRAL_SERVER_HPP

#include "ProtocoloMPS.hpp"

/**
 * Mutex para variables globales
 */
pthread_mutex_t lock_x;

/**
 *  Declaracion de variable condicional y bool para agregar bloques.
 */
pthread_cond_t cond_agregar_bloque;
bool agregar_bloque = 0;

/**
 *  Declaracion de variable condicional y bool para validar bloques.
 */
pthread_cond_t cond_validado;
bool validado = 0;

Blockchain BC;
Block aux;
Hash_packed previoushash;

class CentralServer
{
public:
    int listeningsocket;
    int portno;
    socklen_t clilen;
    int val = 1;
    struct sockaddr_in MiningPool_addr, CentralServer_addr;
    int SetTheServer();
};

class CentralServerMempoolHandler
{
public:
    int Mempoolsocket;
    Msg msg;
    Control ctrl;
    CentralServerMempoolHandler(int Mempoolsock) : Mempoolsocket(Mempoolsock) {}
    int recvControl();
    int sendControl(uint32_t controlm);
    void SendNewBlockTransaction();
};

class CentralServerMiningPoolHandler
{
public:
    int Miningpoolsocket;
    Msg msg;
    Control ctrl;
    unsigned char blhash[hashlenght];
    CentralServerMiningPoolHandler(int MPS) : Miningpoolsocket(MPS) {}
    int recvControl(Control &thread_ctrl);
    int sendControl(uint32_t controlm);
    int sendHeader(BlkHeader_packed &header);
    int recvBlock(Block &blk);
};

class CentralServerWalletHandler
{
public:
    int WalletSocket;
    Msg msg;
    Control ctrl;
    CentralServerWalletHandler(int WS) : WalletSocket(WS) {}
    int recvControl(Control &thread_ctrl);
    int sendControl(uint32_t controlm);
};

#endif