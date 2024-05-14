#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h> 
#include "protocolMP2M.hpp"
#define h_addr h_addr_list[0]

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    pid_t pid;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    typedef struct {
        sem_t sem;
    } mut;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");


    Header HeadBlock;
    Msg msg;

    uint8_t *flag;
    mut *semaforo;
    flag = (uint8_t *)mmap(NULL, sizeof *flag, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    semaforo = (mut *)mmap(NULL, sizeof(mut), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    sem_init(&semaforo->sem , 1, 0);
    pid = fork();
    switch( pid ) {
        case 0:
            
            unsigned char blhash[32];
            while(1){
                recvMsg(sockfd,&msg);
                getHeader(&msg,&HeadBlock);
                *flag = 1;
                sem_post(&semaforo->sem);
                while(*flag){
                    hash(&HeadBlock,blhash);
                    if(validhash(blhash)){
                        setHeader(&msg,HeadBlock);
                        sendMsg(sockfd,&msg);
                        break;
                    }
                    else{
                        HeadBlock.Nonce++;
                    }
                }
            }
            break;
            
        case -1:
            perror("error en el fork:");
            break;
            
        default:
        /*
            while(1){
                //PONER MUTEX, tener corriendo un while no sirve.
                sem_wait(&semaforo->sem);
                recvMsg(sockfd,&msg);
                getHeader(&msg,&HeadBlock);    
                }
            
                            
        */
        break;
    }
    close(sockfd);
    return 0;
}


