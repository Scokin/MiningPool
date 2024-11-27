#ifndef MINER_HPP
#define MINER_HPP
#include "ProtocoloMPS.hpp"

typedef std::chrono::_V2::system_clock::time_point Time_t;

class Minerohandler
{
public:
    int ss;
    int hashes;
    int ret;
    uint32_t diff;
    uint32_t MP_diff;
    BlkHeader_packed Header;
    int hashnum;
    unsigned char blhash[hashlenght];
    Msg msg;
    Control ctrl;
    Minerohandler(int ss, int hashes, uint32_t MP_dif);
    int getWU();
    int dojob();
    int sendNonce();
    int sendShare();
    int Pregunta();
    void controlsend(uint32_t controlm);
    void controlrecv();
    void headersend();
    void headerrecv();
    
};

class Minero
{
public:
    int sockfd;
    uint8_t pooldiff;
    uint8_t ID;
    Msg msg;
    Control ctrl; 
    int tempInput;
    bool validInput = false;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int portno;
    Minero(int MPport): portno(MPport){}
    void connecttoMiningPool();
    void inputID();
};


#endif