#include "Wallet.hpp"

int main()
{
    Wallet wallet;
    uint32_t monedas;
    wallet.inputID();
    wallet.connecttoCentralServer();
    Wallethandler cs(wallet.sockfd);
    cs.controlsend(Ask_4_coins);
    cs.controlrecv();
    if (cs.ctrl.controlmessage == Ask_4_ID)
    {
        setID(&cs.msg, wallet.ID);
        sendMsg(cs.CentralServersocket, &cs.msg);
        recvMsg(cs.CentralServersocket, &cs.msg);
        getCoins(&cs.msg, &monedas);
        std::cout << "El ID: " << (int)wallet.ID << " tiene " << (int)monedas << " monedas." << std::endl;
    }
    close(wallet.sockfd);
    return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}


int Wallethandler::controlsend(uint32_t controlm)
{
    ctrl.controlmessage = controlm;
    setControl(&msg, ctrl);
    int ret = sendMsg(CentralServersocket, &msg);
    return ret;
}

int Wallethandler::controlrecv()
{
    int ret = recvMsg(CentralServersocket, &msg);
    getControl(&msg, &ctrl);
    return ret;
}


//Se conecta con el servidor central

void Wallet::connecttoCentralServer()
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
    ctrl.controlmessage = Connection_From_Wallet;
    setControl(&msg, ctrl);
    sendMsg(sockfd, &msg);
}


//Con esta funcion se pide el ID de la wallet que se quiere consultar.
void Wallet::inputID()
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