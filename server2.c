#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#define MAX_CLIENTS 10

#define MAX_HISTORY 50
char chat_history[MAX_HISTORY][1024];
int history_count = 0;


void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    srand(time(0));
    if (argc < 2) {
        fprintf(stderr, "Usage: <%s> <portno>\n", argv[0]);
        exit(1);
    }

    int portno = atoi(argv[1]);
    int master_socket, addrlen, new_socket, client_socket[MAX_CLIENTS], max_clients = MAX_CLIENTS, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    char buffer[1025];
    fd_set readfds;
    
    int id_counter = 1;

    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        error("Socket failed");
    }

    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        error("setsockopt failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portno);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        error("Bind failed");
    }

    if (listen(master_socket, 3) < 0) {
        error("Listen failed");
    }

    addrlen = sizeof(address);
    printf("Chat Server is listening on port %d ...\n", portno);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error\n");
        }

        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                error("Accept failed");
            }

      int assigned_id = id_counter++;
            char id_msg[20];
            snprintf(id_msg, sizeof(id_msg), "%d", assigned_id);
            write(new_socket, id_msg, strlen(id_msg));

            for (int k = 0; k < history_count; k++) {
                write(new_socket, chat_history[k], strlen(chat_history[k]));
                write(new_socket, "\n", 1);
                usleep(50000); 
            }

            printf("New connection (Assigned ID: %d), socket fd is %d\n", assigned_id, new_socket);

            for (i = 0; i < max_clients; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                bzero(buffer, 1025);
                valread = read(sd, buffer, 1024);

                if (valread == 0) {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Client disconnected.\n");
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("[Broadcast] %s\n", buffer);

                    if (history_count < MAX_HISTORY) {
                        strcpy(chat_history[history_count], buffer);
                        history_count++;
                    } else {
                        for (int k = 0; k < MAX_HISTORY - 1; k++) {
                            strcpy(chat_history[k], chat_history[k + 1]);
                        }
                        strcpy(chat_history[MAX_HISTORY - 1], buffer);
                    }

                    for (int j = 0; j < max_clients; j++) {
                        int dest_socket = client_socket[j];
                        if (dest_socket > 0) {
                            write(dest_socket, buffer, strlen(buffer));
                            write(dest_socket, "\n", 1);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
