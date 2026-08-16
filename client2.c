#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: <%s> <Server_IP> <portno>\n", argv[0]);
        exit(1);
    }

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host!");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Connection failed\n");

    char name[50];
    char id_str[20];
    char buffer[512];

    bzero(id_str, 20);
    n = read(sockfd, id_str, 19);
    if (n < 0) error("Error reading ID");
    printf("Connected! Your Assigned ID is: %s\n", id_str);

    printf("Enter your Name: ");
    fgets(name, 50, stdin);
    name[strcspn(name, "\r\n")] = 0;
    printf("\n--- Welcome to the Chat Group (History will load below) ---\n");

    pid_t pid = fork();
    if (pid < 0) {
        error("Fork failed");
    } else if (pid == 0) {
        while (1) {
            bzero(buffer, 512);
            int nr = read(sockfd, buffer, 511);
            if (nr <= 0) {
                printf("Server disconnected.\n");
                exit(0);
            }
            printf("\n%s\n> ", buffer);
            fflush(stdout);
        }
    } else {
        char message[256];
        char send_buffer[512];
        
        while (1) {
            printf("> ");
            fflush(stdout);

            bzero(message, 256);
            if (fgets(message, 256, stdin) == NULL) {
                printf("Error reading input.\n");
                continue;
            }

            message[strcspn(message, "\r\n")] = 0; 

            if (strlen(message) == 0) {
                continue;
            }

            if (strncmp(message, "bye", 3) == 0) {
                snprintf(send_buffer, sizeof(send_buffer), "Client [ID: %s, Name: %s] disconnected.", id_str, name);
                write(sockfd, send_buffer, strlen(send_buffer));
                printf("Disconnection requested exiting \n");
                kill(pid, SIGKILL);
                break;
            }

            snprintf(send_buffer, sizeof(send_buffer), "%s: %s", name, message);
            if (write(sockfd, send_buffer, strlen(send_buffer)) < 0) {
                perror("Error on writing");
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}
