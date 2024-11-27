#ifndef PROTO_H_
#define PROTO_H_ 1

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <openssl/evp.h>
#include <sstream>
#include <string>
#include <chrono>
#include <math.h>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <mutex>
#include <algorithm>
#include <vector>

#define MAX_TRANSACTIONS 30
#define MAX_THREADS 10

const uint8_t lut[8] = {0x0, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};

const uint32_t Ask_For_Header = 1;
const uint32_t Ask_For_State = 2;
const uint32_t Got_Valid_Nonce = 3;
const uint32_t Not_Yet_Nonce = 4;
const uint32_t Nonce_Was_Found = 5;
const uint32_t Send_Your_Nonce = 6;
const uint32_t Sending_Header = 7;
const uint32_t Quit = 8;
const uint32_t Ask_For_Current_Header = 9;
const uint32_t Got_Valid_Block = 10;
const uint32_t Sending_Current_Header = 11;
const uint32_t Send_Valid_Block = 12;
const uint32_t Send_Mempool = 13;
const uint32_t Sending_Mempool = 14;
const uint32_t Got_Valid_share = 15;
const uint32_t Send_your_share = 16;
const uint32_t NEW_BLOCK = 17;
const uint32_t SEND_TRANSACTIONS = 18;
const uint32_t NEW_TRANSACTION = 19;
const uint32_t SEND_NEW_TRANSACTION = 20;

const uint32_t Connection_From_MiningPool = 30;
const uint32_t Connection_From_MemPool = 31;
const uint32_t Connection_From_Wallet = 32;

const uint32_t Send_Miner_ID = 40;
const uint32_t Sending_Miner_ID = 41;

const uint32_t Correctly_deleted = 50;

const uint32_t Ask_4_coins = 52;
const uint32_t Ask_4_ID = 53;

const uint32_t Agregadas_Exitosamente = 60;

const uint32_t Validated = 70;
const uint32_t Not_validated = 71;

const uint32_t NuevoHeader = 100;

const int payperblock = 230;

const int hashlenght =32;

typedef enum
{
    TYPE_BLOCK,
    TYPE_HEADER,
    TYPE_CONTROL,
    TYPE_MEMPOOL,
    TYPE_ID,
    TYPE_COINS,
    TYPE_DIFF,
} Type;

struct __attribute__((__packed__)) Coins_packed
{
    uint32_t monedas;
};

struct __attribute__((__packed__)) Diff_packed
{
    uint8_t dificultad;
};

struct __attribute__((__packed__)) ID_packed
{
    uint8_t ID;
};
struct __attribute__((__packed__)) Header_packed
{
    uint8_t type;
    uint16_t size8;
};

struct __attribute__((__packed__)) Hash_packed
{
    static const uint8_t HASH_SZ8 = hashlenght;
    uint8_t hash[HASH_SZ8];
    void printHexString() const;
    bool operator==(const Hash_packed &other) const;
};

bool Hash_packed::operator==(const Hash_packed &other) const
{
    for (int i = 0; i < HASH_SZ8; i++)
    {
        if (!(hash[i] == other.hash[i]))
        {
            return 0;
        }
    }
    return 1;
};

struct __attribute__((__packed__)) MKRT_packed
{
    static const uint8_t MKRT_SZ8 = hashlenght;
    uint8_t mkrt[MKRT_SZ8];
    void printHexString() const;
};

struct __attribute__((__packed__)) Transaccion_packed
{
    uint8_t sender;
    uint8_t receiver;
    uint8_t fee;
    uint8_t quantity;
    bool operator==(const Transaccion_packed &other) const;
};

bool Transaccion_packed::operator==(const Transaccion_packed &other) const
{
    return ((sender == other.sender) && (receiver == other.receiver) && (fee == other.fee) && (quantity == other.quantity));
}

struct __attribute__((__packed__)) BlkHeader_packed
{
    uint32_t version;
    Hash_packed prevBlockHash;
    MKRT_packed MerkleRoot;
    uint32_t Timestamp;
    uint32_t DiffTarget;
    uint32_t Nonce;
};

struct __attribute__((__packed__)) Transacciones_packed
{
    uint8_t cantidad;
    Transaccion_packed stack[0];
};

struct __attribute__((__packed__)) Block_packed
{
    BlkHeader_packed header;
    uint8_t cantidad;
    Transaccion_packed stack[0];
};

struct __attribute__((__packed__)) Control
{
    uint32_t controlmessage;
};

struct __attribute__((__packed__)) Msg
{
    Header_packed hdr;

    union __attribute__((__packed__))
    {
        BlkHeader_packed bh;
        Control ctrl;
        Block_packed bloque;
        Transacciones_packed transacc;
        ID_packed id;
        Coins_packed coins;
        Diff_packed diff;
    } Payload;
};

void string2hash(Hash_packed &s, const std::string &ss)
{
    std::string aux;
    for (size_t i = 0; i < ss.length(); i += 2)
    {
        aux = ss.substr(i, 2);
        s.hash[i / 2] = stoi(aux, nullptr, 16);
    }
}
void string2mkrt(MKRT_packed &s, const std::string &ss)
{
    std::string aux;
    for (size_t i = 0; i < ss.length(); i += 2)
    {
        aux = ss.substr(i, 2);
        s.mkrt[i / 2] = stoi(aux, nullptr, 16);
    }
}

void Hash_packed::printHexString() const
{
    std::cout << std::hex << std::setfill('0');
    for (int i = 0; i < HASH_SZ8; ++i)
    {
        std::cout << std::setw(2) << static_cast<int>(hash[i]);
    }
    std::cout << std::endl;
}

void MKRT_packed::printHexString() const
{
    std::cout << std::hex << std::setfill('0');
    for (int i = 0; i < MKRT_SZ8; ++i)
    {
        std::cout << std::setw(2) << static_cast<int>(mkrt[i]);
    }
    std::cout << std::endl;
}

void printheader(const BlkHeader_packed &head)
{
    std::cout << std::hex << "Version: "; //<<head.version << std::endl;
    std::cout << "0x" << std::setfill('0') << std::setw(8) << std::hex << head.version << std::endl;
    std::cout << "Previous Block Hash: ";
    head.prevBlockHash.printHexString();
    std::cout << "Merkle Root: ";
    head.MerkleRoot.printHexString();
    std::cout << "Instante: ";
    std::cout << "0x" << std::setfill('0') << std::setw(8) << std::hex << head.Timestamp << std::endl;
    std::cout << "Dificultad de la red: ";
    std::cout << "0x" << std::setfill('0') << std::setw(8) << std::hex << head.DiffTarget << std::endl;
    std::cout << "Nonce: ";
    std::cout << "0x" << std::setfill('0') << std::setw(8) << std::hex << head.Nonce << std::dec << std::endl;
}

void printMempool(const Transaccion_packed *tran, uint8_t cantidad)
{
    std::cout << "---------------transacciones----------------" << std::endl;
    std::cout << "Cantidad: " << (int)cantidad << std::endl;
    for (int i = 0; i < cantidad; ++i)
    {
        std::cout << "---------------Transaccion " << i + 1 << "----------------" << std::endl;
        std::cout << "Precio: " << (int)tran[i].fee << std::endl;
        std::cout << "Cantidad: " << (int)tran[i].quantity << std::endl;
        std::cout << "Recibe: " << (int)tran[i].receiver << std::endl;
        std::cout << "Envia: " << (int)tran[i].sender << std::endl;
    }
}

class Block
{
public:
    BlkHeader_packed header;
    uint8_t cantidad;
    Transaccion_packed stack[MAX_TRANSACTIONS];

    Block(const Msg *msg)
    {
        assert(msg->hdr.type == TYPE_BLOCK);
        header.DiffTarget = ntohl(msg->Payload.bloque.header.DiffTarget);
        header.Nonce = ntohl(msg->Payload.bloque.header.Nonce);
        header.Timestamp = ntohl(msg->Payload.bloque.header.Timestamp);
        header.version = ntohl(msg->Payload.bloque.header.version);
        header.prevBlockHash = msg->Payload.bloque.header.prevBlockHash;
        header.MerkleRoot = msg->Payload.bloque.header.MerkleRoot;
        cantidad = msg->Payload.bloque.cantidad;
        for (uint8_t i = 0; i < msg->Payload.bloque.cantidad; i++)
        {
            stack[i] = msg->Payload.bloque.stack[i];
        }
    }

    Block() : cantidad(MAX_TRANSACTIONS)
    {
        std::string s = "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a";
        std::string s2 = "0000000000000000000000000000000000000000000000000000000000000000";
        string2hash(header.prevBlockHash, s2);
        string2mkrt(header.MerkleRoot, s);
        header.version = 0x01000000;
        header.Timestamp = 0x29ab5f49;
        header.DiffTarget = 0xffff001d;
        header.Nonce = 0x00000000;

        for (uint8_t i = 0; i < MAX_TRANSACTIONS; i++)
        {
            stack[i].fee = i + 1;
            stack[i].quantity = i + 1;
            stack[i].receiver = i + 1;
            stack[i].sender = i + 1;
        }
    }

    void printb()
    {
        printheader(header);
        printMempool(stack, cantidad);
    }
};

class Mempool
{
public:
    int cantidad;
    Transaccion_packed trans[MAX_TRANSACTIONS];
    Mempool(const Msg *msg)
    {
        assert(msg->hdr.type == TYPE_MEMPOOL);
        cantidad = msg->Payload.transacc.cantidad;
        std::memcpy(trans, msg->Payload.transacc.stack, msg->Payload.transacc.cantidad * sizeof(Transaccion_packed));
    }
    Mempool()
    {
        cantidad = MAX_TRANSACTIONS;
    }
};

class Blockchain
{
public:
    int numberofblocks;
    BlkHeader_packed netwheader;
    Block blockchain[200];
    Blockchain() : numberofblocks(0)
    {
        std::string s = "0000000000000000000000000000000000000000000000000000000000000000";
        string2hash(netwheader.prevBlockHash, s);
    }
    void addBlock(Block &bloque)
    {
        if (numberofblocks < 200)
        {
            blockchain[numberofblocks] = bloque;
            numberofblocks++;
        }
    }
    void validateBlock() {}
    Hash_packed prevHash()
    {
        return blockchain[numberofblocks - 1].header.prevBlockHash;
    }
    void printlastblock()
    {
        printheader(blockchain[numberofblocks - 1].header);
        printMempool(blockchain[numberofblocks - 1].stack, blockchain[numberofblocks - 1].cantidad);
    }
    BlkHeader_packed network_header()
    {
        std::string s = "0000000000000000000000000000000000000000000000000000000000000000";
        string2mkrt(netwheader.MerkleRoot, s);
        netwheader.version = 0x01000000;
        netwheader.Timestamp = 0x00000000;
        netwheader.DiffTarget = 0x00000016;
        netwheader.Nonce = 0x00000000;
        return netwheader;
    }
    int monedas(uint8_t ID)
    {
        int cuenta = 0;
        for(int i = 0; i < numberofblocks; i++)
        {
            for(int j = 0; j < blockchain[i].cantidad ; j++)
            {
                if(blockchain[i].stack[j].sender == ID)
                {
                    cuenta -= blockchain[i].stack[j].quantity;
                }
                if(blockchain[i].stack[j].receiver == ID)
                {
                    cuenta += blockchain[i].stack[j].quantity;
                }
            }
            
        }
        return cuenta;
    }
};

inline static void setHeader(Msg *msg, const BlkHeader_packed &head)
{
    msg->hdr.type = TYPE_HEADER;
    msg->hdr.size8 = htons(sizeof(BlkHeader_packed) + sizeof(Header_packed));
    msg->Payload.bh.DiffTarget = htonl(head.DiffTarget);
    msg->Payload.bh.Nonce = htonl(head.Nonce);
    msg->Payload.bh.Timestamp = htonl(head.Timestamp);
    msg->Payload.bh.version = htonl(head.version);
    msg->Payload.bh.prevBlockHash = head.prevBlockHash;
    msg->Payload.bh.MerkleRoot = head.MerkleRoot;
}

inline static void setControl(Msg *msg, const Control &ctr)
{
    msg->hdr.type = TYPE_CONTROL;
    msg->hdr.size8 = htons(sizeof(Control) + sizeof(Header_packed));
    msg->Payload.ctrl.controlmessage = htonl(ctr.controlmessage);
}

inline static void setBlock(Msg *msg, const Block_packed &block)
{
    msg->hdr.type = TYPE_BLOCK;
    msg->hdr.size8 = htons(sizeof(Block_packed) + sizeof(Header_packed) + sizeof(Transaccion_packed) * block.cantidad);
    msg->Payload.bloque.header.DiffTarget = htonl(block.header.DiffTarget);
    msg->Payload.bloque.header.Nonce = htonl(block.header.Nonce);
    msg->Payload.bloque.header.Timestamp = htonl(block.header.Timestamp);
    msg->Payload.bloque.header.version = htonl(block.header.version);
    msg->Payload.bloque.header.prevBlockHash = block.header.prevBlockHash;
    msg->Payload.bloque.header.MerkleRoot = block.header.MerkleRoot;
    msg->Payload.bloque.cantidad = block.cantidad;
    std::memcpy(msg->Payload.bloque.stack, block.stack, sizeof(Transaccion_packed) * block.cantidad);
}

inline static void setMempool(Msg *msg, const Transaccion_packed &tran, uint8_t cantidad)
{
    msg->hdr.type = TYPE_MEMPOOL;
    msg->hdr.size8 = htons(sizeof(Header_packed) + sizeof(Transacciones_packed) + sizeof(Transaccion_packed) * cantidad);
    msg->Payload.transacc.cantidad = cantidad;
    std::memcpy(msg->Payload.transacc.stack, &tran, sizeof(Transaccion_packed) * cantidad);
}

inline static void setID(Msg *msg, const uint8_t id)
{
    msg->hdr.type = TYPE_ID;
    msg->hdr.size8 = htons(sizeof(Header_packed)+sizeof(ID_packed));
    msg->Payload.id.ID = id;
}

inline static void setDiff(Msg *msg, const uint8_t diff)
{
    msg->hdr.type = TYPE_DIFF;
    msg->hdr.size8 = htons(sizeof(Header_packed)+sizeof(Diff_packed));
    msg->Payload.id.ID = diff;
}

inline static void setCoins(Msg *msg, const uint32_t coins)
{
    msg->hdr.type = TYPE_COINS;
    msg->hdr.size8 = htons(sizeof(Header_packed)+sizeof(Coins_packed));
    msg->Payload.coins.monedas = htonl(coins);
}

inline static void getCoins(const Msg *msg, uint32_t *coin)
{
    assert(msg->hdr.type == TYPE_COINS);
    *coin = ntohl(msg->Payload.coins.monedas);
}


inline static void getDiff(const Msg *msg, uint8_t *diff)
{
    assert(msg->hdr.type == TYPE_DIFF);
    *diff = msg->Payload.coins.monedas;
}


inline static void getID(const Msg *msg, uint8_t *id)
{
    assert(msg->hdr.type == TYPE_ID);
    *id = msg->Payload.id.ID;
}

inline static void getBlock(const Msg *msg, Block_packed *block)
{
    assert(msg->hdr.type == TYPE_BLOCK);
    block->header.DiffTarget = ntohl(msg->Payload.bh.DiffTarget);
    block->header.Nonce = ntohl(msg->Payload.bh.Nonce);
    block->header.Timestamp = ntohl(msg->Payload.bh.Timestamp);
    block->header.version = ntohl(msg->Payload.bh.version);
    block->header.prevBlockHash = msg->Payload.bh.prevBlockHash;
    block->header.MerkleRoot = msg->Payload.bh.MerkleRoot;
    block->cantidad = msg->Payload.bloque.cantidad;
    std::memcpy(block->stack, msg->Payload.bloque.stack, msg->Payload.bloque.cantidad * sizeof(Transaccion_packed));
}

inline static void getHeader(const Msg *msg, BlkHeader_packed *head)
{
    assert(msg->hdr.type == TYPE_HEADER);
    head->DiffTarget = ntohl(msg->Payload.bh.DiffTarget);
    head->Nonce = ntohl(msg->Payload.bh.Nonce);
    head->Timestamp = ntohl(msg->Payload.bh.Timestamp);
    head->version = ntohl(msg->Payload.bh.version);
    head->prevBlockHash = msg->Payload.bh.prevBlockHash;
    head->MerkleRoot = msg->Payload.bh.MerkleRoot;
}

inline static void getControl(const Msg *msg, Control *ctr)
{
    assert(msg->hdr.type == TYPE_CONTROL);
    ctr->controlmessage = ntohl(msg->Payload.ctrl.controlmessage);
}

int sendMsg(int sockfd, const Msg *msg)
{
    size_t toSend = ntohs(msg->hdr.size8);
    ssize_t sent;
    uint8_t *ptr = (uint8_t *)msg;

    while (toSend)
    {
        sent = send(sockfd, ptr, toSend, 0);
        if ((sent == -1 && errno != EINTR) || sent == 0)
            return sent;
        if (sent > 0)
        {
            toSend -= sent;
            ptr += sent;
        }
    }
    return 1;
}
int recvMsg(int sockfd, Msg *msg)
{
    size_t toRecv = sizeof(Header_packed);
    ssize_t recvd;
    uint8_t *ptr = (uint8_t *)&msg->hdr;
    int headerRecvd = 0;

    while (toRecv)
    {
        recvd = recv(sockfd, ptr, toRecv, 0);
        if ((recvd == -1 && errno != EINTR) || recvd == 0)
            return recvd;
        if (recvd > 0)
        {
            toRecv -= recvd;
            ptr += recvd;
        }
        if (toRecv == 0 && headerRecvd == 0)
        {
            headerRecvd = 1;
            ptr = (uint8_t *)&msg->Payload;
            toRecv = ntohs(msg->hdr.size8) - sizeof(Header_packed);
        }
    }
    return 1;
}

void hash(BlkHeader_packed *Headerhash, unsigned char *Blockhash)
{
    unsigned char buffer[80];
    std::memcpy(&buffer[0], &Headerhash->version, sizeof(uint32_t));
    std::memcpy(&buffer[4], &Headerhash->prevBlockHash, hashlenght);
    std::memcpy(&buffer[36], &Headerhash->MerkleRoot, hashlenght);
    std::memcpy(&buffer[68], &Headerhash->Timestamp, sizeof(uint32_t));
    std::memcpy(&buffer[72], &Headerhash->DiffTarget, sizeof(uint32_t));
    std::memcpy(&buffer[76], &Headerhash->Nonce, sizeof(uint32_t));
    unsigned char hashaux[hashlenght];
    SHA256(buffer, 80, hashaux);
    SHA256(hashaux, hashlenght, Blockhash);
    return;
}

void char2hash(Hash_packed *hash, unsigned char *Blockhash)
{
    std::memcpy(hash->hash, Blockhash, hashlenght);
    return;
}

int validhash(unsigned char *valid, uint32_t diff)
{
    uint32_t cociente = diff / 8;
    uint32_t modulo = diff % 8;
    uint8_t shifted = ((valid[31 - cociente] & 0x0F) << 4) | ((valid[31 - cociente] & 0xF0) >> 4);
    for (uint32_t i = 0; i < cociente; i++)
    {
        if (valid[31 - i] & 0xff)
        {
            return 0;
        }
    }
    if (shifted & lut[modulo])
    {
        return 0;
    }
    return 1;
}


void merklerootcalc(MKRT_packed& mkrt, Mempool& mem)
{
    unsigned char buffer[MAX_TRANSACTIONS][hashlenght];
    unsigned char hashable [2*hashlenght];
    unsigned char storage[MAX_TRANSACTIONS *sizeof(Transaccion_packed)];
    std::memcpy(storage,mem.trans,mem.cantidad);
    bool impar;
    for(int i = 0; i < mem.cantidad; i++)
    {
        SHA256(&storage[i*sizeof(Transaccion_packed)], sizeof(Transaccion_packed), buffer[i]);
    }

    int numberofleaves = mem.cantidad%2;
    if(numberofleaves%2 == 1){
         impar = 1;
         numberofleaves--;
    }
    while(numberofleaves > 1)
    {
        for(int i = 0; i <numberofleaves; i+=2)
        {
            std::memcpy(hashable,buffer[i],hashlenght);
            std::memcpy(hashable,buffer[i+1],hashlenght);
            SHA256(hashable,2*hashlenght,buffer[i/2]);
        }
        numberofleaves/=2;
    }
    if(impar)
    {
        std::memcpy(hashable,buffer[0],hashlenght);
        std::memcpy(hashable,buffer[mem.cantidad-1],hashlenght);
        SHA256(hashable,2*hashlenght,buffer[0]);
    }
    std::memcpy(mkrt.mkrt,buffer,hashlenght);
    return;
}


#endif // PROTO_H_
