/*
 Soner
 
 
 Send a file over a socket.
 
 Interface:
 
 ./executable [<sever_hostname> [<port>]]

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>                      /* getprotobyname */
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

enum { PORTSIZE = 6 };

volatile int signo_taken;

#define MAXFILES        1000

void
sig_handler(int signo)
{
    signo_taken = signo;
}

int
main(int argc, char **argv)
{
    struct addrinfo hints,
    *res;
    char *server_hostname = "127.0.0.1";
    char file_path[BUFSIZ];
    char *server_reply = NULL;
    char *user_input = NULL;
    char buffer[BUFSIZ];
    int filefd;
    int sockfd;
    struct stat st;
    ssize_t read_return;
    struct hostent *hostent;
    unsigned short server_port = 12345;
    char portNum[PORTSIZE];
    char remote_file[BUFSIZ];
    int select;
    char *client_server_files[MAXFILES];
    int i = 0;
    int j;
    
    if (argc != 3) {
        fprintf(stderr, "Usage   ./client  <ip>  <port>\n");
        exit(EXIT_FAILURE);
    }
    
    server_hostname = argv[1];
    server_port = strtol(argv[2], NULL, 10);
    
    /* Prepare hint (socket address input). */
    hostent = gethostbyname(server_hostname);
    if (hostent == NULL) {
        fprintf(stderr, "error: gethostbyname(\"%s\")\n", server_hostname);
        exit(EXIT_FAILURE);
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;          // ipv4
    hints.ai_socktype = SOCK_STREAM;    // tcp
    hints.ai_flags = AI_PASSIVE;        // fill in my IP for me
    
    sprintf(portNum, "%d", server_port);
    getaddrinfo(NULL, portNum, &hints, &res);
    
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    /* Do the actual connection. */
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        return EXIT_FAILURE;
    }
    
    signal(SIGINT, sig_handler);
    while (! signo_taken) {
        puts("connected to the server");
        
        
        while (! signo_taken) {

            puts("-----------------");
            puts("|1 - listLocal| \n|2 - listServer| \n|3 - sendFile| \n|4 - help| \n|5 - exit| ");
            puts("-----------------");
            
            scanf("%d", &select);
            
            if (signo_taken)
                break;
            
            switch (select) {
                case 1:                 // list files of client's directory
                    system("find . -maxdepth 1 -type f | sort");
                    sprintf(remote_file, "%s", "listLocal");
                    send(sockfd, remote_file, sizeof(remote_file), 0);
                    break;
                    
                case 2:                 // listServer
                    sprintf(remote_file, "%s", "listServer");
                    send(sockfd, remote_file, sizeof(remote_file), 0);
                    puts("---- Files btw Server and the Client ----");
                    for (j = 0; j < i; ++j) {
                        puts(client_server_files[j]);
                    }
                    break;
                    
                case 3:                 // send file
                    fputs("Enter filename: ",stdout);
                    fflush(stdout);
                    
                    memset(file_path, 0, sizeof file_path);
                    scanf("%s", file_path);
                    
                    sprintf(remote_file, "%s", "sendFile");
                    send(sockfd, remote_file, sizeof(remote_file), 0);
                    
                    if (signo_taken)
                        break;

                    filefd = open(file_path, O_RDONLY);
                    if (filefd == -1) {
                        perror("open send file");
                        break;
                    }
                    
                    if (fstat(filefd,&st) < 0) {
                        perror("stat send file");
                        close(filefd);
                        break;
                    }
                    
                    // send file name to server
                    memset(remote_file, 0, sizeof(remote_file));
                    
                    sprintf(remote_file, "%lld,%s",
                            (long long) st.st_size,file_path);
                    
                    send(sockfd, remote_file, sizeof(remote_file), 0);
                    
                    
                    while (1) {
                        read_return = read(filefd, buffer, BUFSIZ);
                        if (read_return == 0)
                            break;
                        
                        if (read_return == -1) {
                            perror("read");
                            break;
                        }
                        
                        if (write(sockfd, buffer, read_return) == -1) {
                            perror("write");
                            break;
                        }
                    }
                    
                    close(filefd);
                    
                    if (i < MAXFILES)
                        client_server_files[i++] = strdup(file_path);
                    
                    puts("file complete");
                    break;
                    
                case 4:
                    sprintf(remote_file, "%s", "help");
                    send(sockfd, remote_file, sizeof(remote_file), 0);
                    puts("listLocal - to list the local files in the directory client program started");
                    puts("listServer - transferred files btw server and the client");
                    puts("sendFile - send wanted file to location of the server");
                    break;
                    
                case 5:
                    sprintf(remote_file, "%s", "exit");
                    send(sockfd, remote_file, sizeof(remote_file), 0);
                    free(user_input);
                    free(server_reply);
                    exit(EXIT_SUCCESS);
                    break;
                    
                default:
                    puts("Wrong selection!");
                    break;
            }
            
        }
    }
    
    if (signo_taken)
        printf("!!  OUCH,  CTRL - C received on client  !!\n");
    
    free(user_input);
    free(server_reply);
    exit(EXIT_SUCCESS);
}
