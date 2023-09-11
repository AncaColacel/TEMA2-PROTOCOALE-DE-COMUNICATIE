#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <math.h>

#define MAX_PFDS    32
#define BUFFLEN     2000
#define MAX_CLIENTS  5  
#define ADDRESS      "127.0.0.1"

// structura cu argumente primite de server de la clientul tcp

struct client_tcp {
    char* comanda;
    char* topic;
    char* SF;
};

// structura cu argumente primite de server de la clientul udp
struct client_udp {
    char topic_name[50];
    uint8_t type;
    char data[1501];
}  __attribute__((packed));

// structura pt a trimite mesajul catre TCP
struct mesaj_to_tcp {
    uint32_t adresa_ip_UDP;
    uint16_t port_udp;
    char topic[51];
    char tip_date[2];
    char payload[1501];
}  __attribute__((packed));

int main(int argc, char* argv[]) {
    struct pollfd pfds[MAX_PFDS];
    int nfds = 0;
    int new_fd, tcp_fd, udp_fd;
    char buffer[BUFFLEN];
    int no_biti;
    int ret;
    int flag = 1;
    int port_number;
    struct sockaddr_in server_addr, client_addr;
    struct client_tcp vector_mesaje_tcp[1000];
    struct client_udp vector_mesaje_udp[1000];
    int contor_tcp = 0;
    int contor_udp = 0;
    char* ID_CLI_TCP;
    char* PORT_CLI_TCP;

    if (argc < 2) {
        printf ("Insuficiente argumente\n");
        exit(0);
    }

    // dezactivare buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    // creare socket tcp
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    // verificare creare socket
    if (tcp_fd < 0) {
        printf("socket tcp creat incorect\n");
        exit(0);
    }

    // creare socket udp
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    // verificare creare socket
    if (udp_fd < 0) {
        printf("socket udp creat incorect\n");
        exit(0);
    }

    // port number
    port_number = atoi(argv[1]);

    uint32_t serv_addr;
    inet_pton(AF_INET, ADDRESS, &serv_addr);

    // completare structura tcp
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = serv_addr;

    // completare structura udp
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = serv_addr;
    client_addr.sin_port = htons(port_number);

    // adaugare in vector STDIN_FILENO
    // pentru citire date de la user de la tastatura 
    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    nfds++;

    // adaugare socket tcp (fd == 3)
    pfds[nfds].fd = tcp_fd;
    pfds[nfds].events = POLLIN;
    nfds++;

    // adaugare socket udp (fd == 4)
    pfds[nfds].fd = udp_fd;
    pfds[nfds].events = POLLIN;
    nfds++;

    // legare socket tcp
    ret = bind(tcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        printf("eroare la legare socket tcp\n");
        exit(0);
    }

    // legare socket udp
    ret = bind(udp_fd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
     if (ret < 0) {
        printf("eroare la legare socket udp\n");
        exit(0);
    }

    int enable = 1;
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    
    ret = listen(tcp_fd, MAX_CLIENTS);
    if (ret < 0) {
        printf("eroare la listen\n");
        exit(0);
    }
    while (1) {
        poll (pfds, nfds, 0);
        for (int i = 0; i < nfds; i++) {
            // cazul in care fd ul este pt STDIN
            // primesc o unica comanda de tastatura, exit
            if(pfds[i].revents & POLLIN) {
                
                if (pfds[i].fd == 0) {
                    printf("Ramura STDIN ->\n");
                    // zeroizare buffer
                    memset(buffer, 0, BUFFLEN);
                    // citire comanda de la tastatura in buffer
                    fgets(buffer, BUFFLEN - 1, stdin);
                    if (strncmp (buffer, "exit", 4) == 0) {
                        close(tcp_fd);
                        //close(udp_fd);
                        exit(0);
                    }
                    printf("EROARE: comanda gresita!\n");
                }
                // daca am gasit fd-ul pt socket tcp
                else if (pfds[i].fd == tcp_fd) {
                    printf("Ramura tcp ->\n");
                    socklen_t cli_len = sizeof(client_addr);
                    new_fd = accept(tcp_fd, (struct sockaddr *)&client_addr, &cli_len);
                    pfds[nfds].fd = new_fd;
                    nfds++;
                    // se dezactiveaza algoritmul lui Nagle pentru conexiunea la clientul TCP
                    setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                    // zeroizare buffer
                    memset(buffer, 0, BUFFLEN);
                    // primire comanda de la client tcp
                    no_biti = recv(new_fd, buffer, sizeof(buffer), 0);
                    printf("Comanda primita de la server -> %s\n", buffer);
                    // parsare mesaj de la client tcp
                    // adaugare in vectorul de structuri componentele
                    char *comanda;
                    comanda = strtok(buffer, " ");
                    if (strncmp(comanda, "subscribe", 9) == 0) {
                        printf("New client connected");
                    }
                    else if (strncmp(comanda, "unsubscribe", 11) == 0) {
                        printf("New client connected");
                    }
                }
                // daca am gasit fd-ul pe socket udp
                else if (pfds[i].fd == udp_fd) {
                    printf("Ramura udp: \n");
                    socklen_t cli_len = sizeof(client_addr);
                    // se dezactiveaza algoritmul lui Nagle pentru conexiunea la clientul TCP
                    setsockopt(udp_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                    memset(buffer, 0, BUFFLEN);
                    // primire comanda de la client udp
                    no_biti = recvfrom(udp_fd, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *) &client_addr, (socklen_t *)&cli_len); 
                    buffer[no_biti] = '\0'; 
                    // retin topicul pt fiecare mesaj
                    strncpy(vector_mesaje_udp->topic_name, buffer, 49);
                    // retin tipul de date
                    vector_mesaje_udp->type = (uint8_t)buffer[50];
                    // retin payloadul
                    strncpy(vector_mesaje_udp->data, buffer + 51, 1499);
                    // daca am un int
                    if (vector_mesaje_udp->type == 0) {
                        long int_num;
                        int_num = ntohl(*(uint32_t*)(buffer + 52));
                        if (vector_mesaje_udp->data[0]) {
                            int_num *= -1;
                        }
                        printf("Tipul de date este: %s\n", "INT");
                        printf("Payload-ul este: %ld\n", int_num);
                        printf("***************************************\n");
                        char ip_str[INET_ADDRSTRLEN];
                        struct mesaj_to_tcp buffer_to_send;
                        inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), ip_str, INET_ADDRSTRLEN);
                        strcpy(&buffer_to_send.adresa_ip_UDP, ip_str);
                        strcpy(&buffer_to_send.port_udp, argv[1]);
                        strcpy(buffer_to_send.topic, vector_mesaje_udp->topic_name);
                        buffer_to_send.topic[51] = '\0';
                        sprintf(buffer_to_send.tip_date, "%hhu", vector_mesaje_udp->type);
                        buffer_to_send.tip_date[1] = '\0';
                        //strcpy(buffer_to_send.tip_date, vector_mesaje_udp->type);
                        strcpy(buffer_to_send.payload, vector_mesaje_udp->data);
                        buffer_to_send.payload[1501] = '\0';
                        printf("Aici ar trebui sa fie adresa: %u\n", buffer_to_send.adresa_ip_UDP);
                        printf("Aici ar trebui sa fie portul: %u\n", buffer_to_send.port_udp);
                        printf("Aici ar trebui sa fie topicul: %s\n", buffer_to_send.topic);
                        printf("Aici ar trebui sa fie tipul de date: %hhu\n", buffer_to_send.tip_date);
                        printf("Aici ar trebui sa fie datele: %s\n", buffer_to_send.payload);
                        for (int i = 0; i < contor_tcp; i++) {
                            for (int j = 0; j < contor_udp; j++) {
                                if (strcmp(vector_mesaje_tcp->topic, vector_mesaje_udp->topic_name) == 0) {
                                    int biti_trimisi = send(tcp_fd, &buffer_to_send, sizeof(buffer_to_send), 0);
                                    if (no_biti < 0) {
                                        printf("eroare la send ID_CLIENT\n");
                                    }
                                }
                            }
                        }
                        

                    }
                    if (vector_mesaje_udp->type == 1) {
                        double real_num = ntohs(*(uint16_t*)(buffer + 51));
                        real_num = real_num / 100;
                        printf("Tipul de date este: %s\n", "SHORT");
                        printf("Payload-ul este: %.2f\n", real_num);
                        printf("***************************************\n");
                        char ip_str[INET_ADDRSTRLEN];
                        struct mesaj_to_tcp buffer_to_send;
                        inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), ip_str, INET_ADDRSTRLEN);
                        strcpy(&buffer_to_send.adresa_ip_UDP, ip_str);
                        strcpy(&buffer_to_send.port_udp, argv[1]);
                        strcpy(buffer_to_send.topic, vector_mesaje_udp->topic_name);
                        buffer_to_send.topic[51] = '\0';
                        sprintf(buffer_to_send.tip_date, "%hhu", vector_mesaje_udp->type);
                        buffer_to_send.tip_date[1] = '\0';
                        //strcpy(buffer_to_send.tip_date, vector_mesaje_udp->type);
                        strcpy(buffer_to_send.payload, vector_mesaje_udp->data);
                        buffer_to_send.payload[1501] = '\0';
                        printf("Aici ar trebui sa fie adresa: %u\n", buffer_to_send.adresa_ip_UDP);
                        printf("Aici ar trebui sa fie portul: %u\n", buffer_to_send.port_udp);
                        printf("Aici ar trebui sa fie topicul: %s\n", buffer_to_send.topic);
                        printf("Aici ar trebui sa fie tipul de date: %hhu\n", buffer_to_send.tip_date);
                        printf("Aici ar trebui sa fie datele: %s\n", buffer_to_send.payload);
                        int biti_trimisi = send(tcp_fd, &buffer_to_send, sizeof(buffer_to_send), 0);
                        for (int i = 0; i < contor_tcp; i++) {
                            for (int j = 0; j < contor_udp; j++) {
                                if (strcmp(vector_mesaje_tcp->topic, vector_mesaje_udp->topic_name) == 0) {
                                    int biti_trimisi = send(tcp_fd, &buffer_to_send, sizeof(buffer_to_send), 0);
                                    if (no_biti < 0) {
                                        printf("eroare la send ID_CLIENT\n");
                                    }
                                }
                            }
                        }

                    }
                    if (vector_mesaje_udp->type == 2) {
                        float real_num = ntohl(*(uint32_t*)(buffer + 52));
                        float result = pow(10, (uint8_t)buffer[5]);
                        real_num = real_num / result;
                        printf("ala: %.4f\n", (uint8_t)buffer[56]);
                        if (vector_mesaje_udp->data[0]) {
                            real_num = real_num;
                        }

                        printf("Tipul de date este: %s\n", "FLOAT");
                        printf("Payload-ul este: %.4f\n", real_num);
                        printf("***************************************\n");
                        char ip_str[INET_ADDRSTRLEN];
                        struct mesaj_to_tcp buffer_to_send;
                        inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), ip_str, INET_ADDRSTRLEN);
                        strcpy(&buffer_to_send.adresa_ip_UDP, ip_str);
                        strcpy(&buffer_to_send.port_udp, argv[1]);
                        strcpy(buffer_to_send.topic, vector_mesaje_udp->topic_name);
                        buffer_to_send.topic[51] = '\0';
                        sprintf(buffer_to_send.tip_date, "%hhu", vector_mesaje_udp->type);
                        buffer_to_send.tip_date[1] = '\0';
                        //strcpy(buffer_to_send.tip_date, vector_mesaje_udp->type);
                        strcpy(buffer_to_send.payload, vector_mesaje_udp->data);
                        buffer_to_send.payload[1501] = '\0';
                        printf("Aici ar trebui sa fie adresa: %u\n", buffer_to_send.adresa_ip_UDP);
                        printf("Aici ar trebui sa fie portul: %u\n", buffer_to_send.port_udp);
                        printf("Aici ar trebui sa fie topicul: %s\n", buffer_to_send.topic);
                        printf("Aici ar trebui sa fie tipul de date: %hhu\n", buffer_to_send.tip_date);
                        printf("Aici ar trebui sa fie datele: %s\n", buffer_to_send.payload);
                        for (int i = 0; i < contor_tcp; i++) {
                            for (int j = 0; j < contor_udp; j++) {
                                if (strcmp(vector_mesaje_tcp->topic, vector_mesaje_udp->topic_name) == 0) {
                                    int biti_trimisi = send(tcp_fd, &buffer_to_send, sizeof(buffer_to_send), 0);
                                    if (no_biti < 0) {
                                        printf("eroare la send ID_CLIENT\n");
                                    }
                                }
                            }
                        }


                    }
                    if (vector_mesaje_udp->type == 3) {
                        printf("Tipul de date este: %s\n", "STRING");
                        printf("Payload-ul este: %s\n", vector_mesaje_udp->data);
                        printf("***************************************\n");
                        char ip_str[INET_ADDRSTRLEN];
                        struct mesaj_to_tcp buffer_to_send;
                        inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), ip_str, INET_ADDRSTRLEN);
                        strcpy(&buffer_to_send.adresa_ip_UDP, ip_str);
                        strcpy(&buffer_to_send.port_udp, argv[1]);
                        strcpy(buffer_to_send.topic, vector_mesaje_udp->topic_name);
                        buffer_to_send.topic[51] = '\0';
                        sprintf(buffer_to_send.tip_date, "%hhu", vector_mesaje_udp->type);
                        buffer_to_send.tip_date[1] = '\0';
                        //strcpy(buffer_to_send.tip_date, vector_mesaje_udp->type);
                        strcpy(buffer_to_send.payload, vector_mesaje_udp->data);
                        buffer_to_send.payload[1501] = '\0';
                        printf("Aici ar trebui sa fie adresa: %u\n", buffer_to_send.adresa_ip_UDP);
                        printf("Aici ar trebui sa fie portul: %u\n", buffer_to_send.port_udp);
                        printf("Aici ar trebui sa fie topicul: %s\n", buffer_to_send.topic);
                        printf("Aici ar trebui sa fie tipul de date: %hhu\n", buffer_to_send.tip_date);
                        printf("Aici ar trebui sa fie datele: %s\n", buffer_to_send.payload);
                        int biti_trimisi = send(tcp_fd, &buffer_to_send, sizeof(buffer_to_send), 0);
                        for (int i = 0; i < contor_tcp; i++) {
                            for (int j = 0; j < contor_udp; j++) {
                                if (strcmp(vector_mesaje_tcp->topic, vector_mesaje_udp->topic_name) == 0) {
                                    int biti_trimisi = send(tcp_fd, &buffer_to_send, sizeof(buffer_to_send), 0);
                                    if (no_biti < 0) {
                                        printf("eroare la send ID_CLIENT\n");
                                    }
                                }
                            }
                        }
                    }

                }

            }

        }

    }
}
    

