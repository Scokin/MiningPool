#ifndef WALLET_HPP
#define WALLET_HPP
#include "ProtocoloMPS.hpp"

class Wallethandler
{
public:
    int CentralServersocket;
    Msg msg;
    Control ctrl;
    Wallethandler(int css) : CentralServersocket(css) {};
    int controlsend(uint32_t controlm);
    int controlrecv();
    
};

class Wallet
{
public:
    int sockfd;
    uint8_t ID;
    Msg msg;
    Control ctrl;
    int tempInput;
    bool validInput = false;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int portno = 7000;
    void connecttoCentralServer();
    void inputID();
};


#endif