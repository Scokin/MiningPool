#include "Minero.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no se puso puerto\n");
        exit(1);
    }
    int MPport = atoi(argv[1]);
    int hashes = 200;
    int ret;
    Minero miner(MPport);
    miner.inputID();
    miner.connecttoMiningPool();
    Minerohandler cs(miner.sockfd, hashes, miner.pooldiff);

    while (1)
    {
        ret = cs.getWU();
        if (ret == -1)
        {
            close(miner.sockfd);
            exit(0);
        }
        ret = cs.dojob();
        if (ret == -1)
        {
            close(miner.sockfd);
            exit(0);
        }
        if (ret == 2)
        {
            break;
        }
    }

    close(miner.sockfd);
    return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

Minerohandler::Minerohandler(int ss, int hashes, uint32_t MP_dif)
    : ss(ss),
      hashes(hashes),
      MP_diff(MP_dif),
      hashnum(0) {}

void Minerohandler::controlsend(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    ret = sendMsg(ss, &msg);
    assert(ret == 1);
    return;
}
void Minerohandler::controlrecv()
{
    ret = recvMsg(ss, &msg);
    assert(ret == 1);
    getControl(&msg, &ctrl);
    return;
}
void Minerohandler::headersend()
{
    setHeader(&msg, Header);
    ret = sendMsg(ss, &msg);
    // printheader(Header);
    assert(ret == 1);
    return;
}

void Minerohandler::headerrecv()
{
    ret = recvMsg(ss, &msg);
    assert(ret == 1);
    getHeader(&msg, &Header);
    diff = Header.DiffTarget;
    // printheader(Header);

    return;
}

int Minerohandler::getWU()
{
    std::cout << "asking4header" << std::endl;
    controlsend(Ask_For_Header);
    controlrecv();

    if (ctrl.controlmessage == Sending_Header)
    {
        headerrecv();
        return 1;
    }
    else
    {
        return -1;
    }
}

int Minerohandler::dojob()
{

    while (1)
    {
        for (int i = 0; i < hashes; i++)
        {
            hash(&Header, blhash);
            if (validhash(blhash, MP_diff))
            {
                if (validhash(blhash, diff))
                {
                    ret = sendNonce();
                    hashnum = 0;
                    if (ret == 1)
                    {
                        return 1;
                    }
                    if (ret == 0)
                    {
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    ret = sendShare();
                    Header.Nonce++;
                }
            }
            else
            {
                Header.Nonce++;
            }
        }
        ret = Pregunta();
        hashnum = hashnum + hashes;
        if (ret == 1)
        {
            continue;
        }
        if (ret == 0)
        {
            return 0;
        }
        if (ret == 2)
        {
            return 2;
        }
        else
        {
            return -1;
        }
    }
}

int Minerohandler::sendNonce()
{
    controlsend(Got_Valid_Nonce);
    controlrecv();

    if (ctrl.controlmessage == Send_Your_Nonce)
    {
        headersend();
        return 1;
    }
    if (ctrl.controlmessage == Nonce_Was_Found)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int Minerohandler::sendShare()
{
    controlsend(Got_Valid_share);
    controlrecv();
    if (ctrl.controlmessage == Send_your_share)
    {
        headersend();
        return 1;
    }
    else
    {
        return 0;
    }
}

int Minerohandler::Pregunta()
{
    controlsend(Ask_For_State);
    controlrecv();
    if (ctrl.controlmessage == Not_Yet_Nonce)
    {
        return 1;
    }
    if (ctrl.controlmessage == Nonce_Was_Found)
    {
        return 0;
    }
    if (ctrl.controlmessage == Quit)
    {
        return 2;
    }
    else
    {
        return -1;
    }
}

void Minero::connecttoMiningPool()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    recvMsg(sockfd, &msg);
    getControl(&msg, &ctrl);
    if (ctrl.controlmessage == Send_Miner_ID)
    {
        ctrl.controlmessage = Sending_Miner_ID;
        setControl(&msg, ctrl);
        sendMsg(sockfd, &msg);
        setID(&msg, ID);
        sendMsg(sockfd, &msg);
        recvMsg(sockfd, &msg);
        getDiff(&msg, &pooldiff);
    }
}
void Minero::inputID()
{
    while (!validInput)
    {
        std::cout << "Escribe el ID de tu cuenta (entre 0 y 255): ";
        std::cin >> tempInput;

        if (std::cin.fail() || tempInput < 0 || tempInput > 255)
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "La ID no cumple los requisitos.";
        }
        else
        {
            ID = static_cast<uint8_t>(tempInput);
            validInput = true;
        }
    }
}

