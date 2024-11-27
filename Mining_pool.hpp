#ifndef MINING_POOL_HPP
#define MINING_POOL_HPP
#include "ProtocoloMPS.hpp"

Mempool miningpooltransactions;
#define MAX_THREADS 10
sig_atomic_t flag = 1;
typedef std::chrono::_V2::steady_clock::time_point Time_t;
int j = 0;
int totalshares = 0;
int headernumber;
uint8_t pooldiff = 14;
int terminaron = 0;
bool nuevoBloque = 0;
Hash_packed test;
const int checknewblock = 100;


/**
 * Mutex para variables globales
 */
pthread_mutex_t lock_x;

/**
 *  Declaracion de variable condicional y bool para desbloquear los mineros.
 * Cuando el thread conectado al servidor fabrica un bloque y libera
 * el Header se hace broadcast para despertar a todos los threads
 * para que envien el bloque a los mineros.
 */
bool Header_disponible = 0;
pthread_cond_t cond_Header_disponible;

/**
 *  Declaracion de variable condicional y bool para advertir al thread que
 * comunica con el servidor central y la Mempool de que se hayo un
 * bloque valido.
 */
pthread_cond_t cond_Header_valido;
bool Header_valido = 0;

/**
 *  Headers y pool de transacciones globales para comunicar a los
 * threads.
 */

BlkHeader_packed Header_para_minar;
BlkHeader_packed Header_minado;

/**
 *  Declaracion de variable condicional y bool para que los mineros un
 * nuevo header para unirse. (Normalmente no seria estrictamente
 * necesario, sin embargo la implementacion usada de determinacion
 * de rangos de nonce no es dinamica y queda fija al iniciar el
 * minado sobre un header).
 */

pthread_cond_t cond_waiting_thread;
bool waiting_thread = 0;

uint8_t MP_address;
struct thread_args
{
    int mempoolsocket;
    int centralserversocket;
};

struct Miner_thread_args
{
    uint8_t ID;
    int minersocket;
};

class ServerandMempooolHandler
{
public:
    int mempoolsocket;
    int centralserversocket;
    bool Noncecorrecto;
    int ret;
    Block bloque;
    BlkHeader_packed head;
    Transaccion_packed Blockearning;
    Msg msg;
    Time_t Tstart;
    Time_t Tend;
    Control ctrl;
    Mempool mem;
    void blockprice();
    int askmempool();
    int getcurrentheader();
    void buildblock();
    int sendblock();
    void controlsend(uint32_t controlm, int socket);
    void controlrecv(int socket);
    void sendpay2miners();
};
/**
 *  Esta clase se utiliza para distribuir rangos de Nonce entre mineros
 * en funcion de la cantidad de mineros conectados a la Mempool.
*/

class RangeDistribution
{
    uint32_t maxR = 0xFFFFFFFF;
    uint32_t minR = 0;
    std::vector<int> sockets;
    std::mutex mtx;

public:
    void addthread(int socket);
    void deletethread(int socket);
    uint32_t distributeranges(int socket);
    int cantidaddemineros(); 
};

class MPhandler
{
public:
    int socket;
    uint8_t ID;
    int share;
    Msg msg;
    Control ctrl;
    BlkHeader_packed Header_para_minar_thread;
    BlkHeader_packed Header_minado_thread;
    Transaccion_packed aux;
    unsigned char blhash[hashlenght];
    int headnum;
    MPhandler(int socket, uint8_t id) : socket(socket), ID(id), share(0) {}
    void control_send(uint32_t controlm);
    void controlrecv();
    void handlerequest();
    void requestedheader();
    void requestedstate();
    void gotshare();
    void gotnonce();
    void payminer();
};
/**
 *  Se usa para establecer la conexion entre la MiningPool con la
 * mempool y el servidor central. Ademas establece el servidor para
 * que se conecten los mineros.
*/
class MPS
{
public:
    int listeningsocket;
    int mempoolsocket;
    int centralserversocket;
    int portnoMP;
    int portnoMempool;
    int portnoCentralServer;
    int val = 1;
    socklen_t clilen;
    Msg msg;
    Control ctrl;
    struct hostent *server;
    struct sockaddr_in MiningPool_addr, Miner_addr, Mempool_addr, CentralServer_addr;
    MPS(int MPport) : portnoMP(MPport){}
    int SetTheServer(); 
    int ConnecttoMempool();
    int ConnecttoCentralServer();
};


RangeDistribution RD;
ServerandMempooolHandler S;

#endif