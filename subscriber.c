#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#define MAX_PFDS    32
#define BUFFLEN     256     

int main(int argc, char* argv[]) {
    struct pollfd pfds[MAX_PFDS];
    int nfds = 0;
    int sockfd;
    char buffer[BUFFLEN];
    char buffer_copie[BUFFLEN];
    int no_biti;
    int flag = 1;
    int ret;
    char *comanda;
    struct sockaddr_in serv_addr;

    // dezactivare buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc < 4) {
        printf ("Insuficiente argumente\n");
        exit(0);
    }
    if (strlen(argv[1]) > 10) {
        printf ("ID_CLIENT INCORECT\n");
        exit(0);
    }
    // creare socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // verificare creare socket
    if (sockfd < 0) {
        printf("socket creat incorect\n");
        exit(0);
    }
    // adaugare in vector STDIN_FILENO
    // pentru citire date de la user de la tastatura 
    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    nfds++;

    // adaugare socket listener  
    pfds[nfds].fd = sockfd;
    pfds[nfds].events = POLLIN;
    nfds++;

    // completare campuri structura
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    if (ret == 0) {
        printf("eroare inet_aton\n");
        exit(0);
    }
    // conectare subscriber la socket
    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        printf("eroare la conectare\n");
        exit(0);
    }
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    
    // se dezactiveaza algoritmul lui Nagle pentru conexiunea la server
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // ca sa verific ca primesc comenzi cu argumente corecte
    int check = 1;
    while (1) {
        if (poll(pfds, nfds, 0) < 0) {
            printf("eroare la poll");
            exit(0);
        }
        // daca citesc datele (comanda) de la tastatura
        if (pfds[0].revents & POLLIN) { 
            // zeroizare buffer
            memset(buffer, 0, BUFFLEN);
            // citire comanda de la tastatura in buffer
            fgets(buffer, BUFFLEN - 1, stdin);
            // creez o copie a bufferului pentru a-i da send apoi daca e cazul
            strcpy(buffer_copie, buffer);
            // parsez primul cuvant pentru a vedea ce comanda primesc
            comanda = strtok(buffer, " ");
            printf("Comanda este: %s\n", comanda);

            // 1. daca primesc exit
            if (strncmp (comanda, "exit", 4) == 0) {
                printf("dam exit\n");
                // deconectare
                exit(0);
                
            }
            // 2. daca primesc substribe
            if (strncmp(comanda, "subscribe", 9) == 0) {
               // parsez topicul
                comanda = strtok(NULL, " ");
                //daca nu exista topic
                if (comanda == NULL) {
                    printf("Eroare: Nu am primit topic la subscribe.\n");
                    check = 0;
                }
                // verific SF-ul
                comanda = strtok(NULL, " ");
                // daca nu exista sau daca nu este 0 sau 1
                // arunc eroare
                if (comanda == NULL || !(atoi(comanda) == 0 || atoi(comanda) == 1)) {
                    printf("Eroare: SF inexistent sau incorect.\n");
                    check  = 0;
                }
            }

            // // 3. daca primesc unsubscribe
            if (strncmp (comanda, "unsubscribe", 11) == 0) {
                // parsez topicul
                comanda = strtok(NULL, " ");
                // daca nu exista topic
                if (comanda == NULL) {
                    printf("Eroare. Nu am primit topic la unsubscribe.\n");
                    check = 0;
                }
            }
            //trimit mesajul daca este corect 
            if (check == 1) {
                // SEND SPECIAL 
                send(sockfd, buffer_copie, strlen(buffer_copie) + 1, 0);
                printf("Comanda livrata cu succes !\n");
                if (strcmp(strtok (buffer_copie, " \n"), "subscribe") == 0) {
                    printf("Subscribe to topic\n");
                }
                else if (strcmp(strtok (buffer_copie, " \n"), "unsubscribe") == 0) {
                    printf("Unsubscribe from topic\n");
                }
            }
        }
        // daca primesc date de pe socketul de conexiune la server
        if ((pfds[1].revents & POLLIN) != 0) {
            printf("am primit mesajul de la server\n");
            // zeroizez bufferul ca sa stochez in el
            // mesajul primit
            memset(buffer, 0, BUFFLEN);
            no_biti = recv(sockfd, buffer, sizeof(buffer), 0);
            // nu am primit nimic
            if (no_biti == 0) {
                break;
            }
            // verific ca receive ul este corect
            if (no_biti < 0) {       
                printf("Receive da eroare.\n");
            }
        }

    }
    printf("Urmeaza inchiderea socketului!\n");
    // inchidere socket
    close(sockfd);
    return 0;
        
}
