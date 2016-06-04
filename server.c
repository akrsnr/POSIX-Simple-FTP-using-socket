/*
 Soner
 
 Receive a file over a socket.
 
 Interface:
 
 ./executable [<port>]
 
 */

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

#include <pthread.h>

double cpu_time_used;
clock_t start, end;
char *all_ids[BUFSIZ];
int all_id_counter = 0;


struct client {
    socklen_t client_len;
    struct sockaddr_in client_address;
    int client_sockfd;
    pthread_t thread;
};


enum { PORTSIZE = 6 };

void *forClient(void *ptr);

void
sig_handler(int signo)
{
    if (signo == SIGINT)
        printf("!!  OUCH,  CTRL - C received  by server !!\n");
}

void portCleaner(const char* port_num) {
    char temp[100] = "sudo lsof -t -i tcp:";
    sprintf(temp, "%s%s%s", temp, port_num, " | xargs kill -9;");
    system(temp);
    //puts(temp);
}

void deleteElement(char *arr[], char* str, int n) {
    int i = 0;
    int found;
    int pos = 0;
    for (i = 0; i < n; i++)
    {
        if (strcmp(arr[i], str) == 0)
        {
            found = 1;
            pos = i;
            break;
        }
    }
    if (found == 1)
    {
        for (i = pos; i <  n - 1; i++)
        {
            arr[i] = arr[i + 1];
        }
    }
#if 0
    printf("Resultant array is\n");
    
    for( c = 0 ; c < n - 1 ; c++ )
        printf("%s\n", arr[c]);
#endif
    
    /*
     int x = sizeof(arr) / sizeof(*arr);
     printf("%d\n\n", x);
     */
}

int
main(int argc, char **argv)
{
    int enable = 1;
    int server_sockfd;
    unsigned short server_port = 12345u;
    struct sockaddr_in server_address;
    struct protoent *protoent;
    char protoname[] = "tcp";
    
    
    struct client *ctl;
    
    if (argc != 2) {
        fprintf(stderr, "Usage   ./server  <port>\n");
        exit(EXIT_FAILURE);
    }
    server_port = strtol(argv[1], NULL, 10);
    
    /* Create a socket and listen to it.. */
    protoent = getprotobyname(protoname);
    if (protoent == NULL) {
        perror("getprotobyname");
        exit(EXIT_FAILURE);
    }
    server_sockfd = socket(
                           AF_INET,
                           SOCK_STREAM,
                           protoent->p_proto
                           );
    if (server_sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(server_port);
    if (bind(
             server_sockfd,
             (struct sockaddr*)&server_address,
             sizeof(server_address)
             ) == -1
        ) {
        perror("bind");
        portCleaner(argv[1]);
        exit(EXIT_FAILURE);
    }
    if (listen(server_sockfd, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "listening on port %d\n", server_port);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,1);
    
    start = clock();
    
    while (1) {
        
        ctl = malloc(sizeof(struct client));
        if (ctl == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        
        ctl->client_len = sizeof(ctl->client_address);
        puts("waiting for client");
        
        ctl->client_sockfd = accept(server_sockfd,
                                    (struct sockaddr *) &ctl->client_address, &ctl->client_len);
        
        if (ctl->client_sockfd < 0) {
            perror("Cannot accept connection\n");
            close(server_sockfd);
            exit(EXIT_FAILURE);
        }
        
        pthread_create(&ctl->thread, &attr, forClient, ctl);
        
        
        
    }
    
    return EXIT_SUCCESS;
}

void *
forClient(void *ptr)
{
    end = clock();
    cpu_time_used = 1000 * (((double) (end - start)) / CLOCKS_PER_SEC);
    
    struct client *ctl = ptr;
    int connect_socket = ctl->client_sockfd;
    int filefd;
    ssize_t read_return;
    char buffer[BUFSIZ];
    char *file_path;
    long long file_length;
    char receiveFileName[BUFSIZ];
    char id[BUFSIZ];
    
    // Thread number means client's id
    printf("Connected time  [%lf] ---  Client number [%ld]\n", cpu_time_used, pthread_self());
    
    sprintf(id, "%ld", pthread_self());
    all_ids[all_id_counter++] = id;
    printf("before all id counter %d\n", all_id_counter);
    
#if 0
    //send(connect_socket, &all_id_counter, sizeof(int), 0);
    int i = 0;
    for (; i < all_id_counter; ++i) {
        puts(all_ids[i]);
        //send(connect_socket, all_ids[i], sizeof(all_ids[i]), 0);
    }
#endif
    
    while (recv(connect_socket, receiveFileName, sizeof(receiveFileName), 0)) {
        
        if((strcmp(receiveFileName, "listServer") == 0
            || strcmp(receiveFileName, "listLocal") == 0 || strcmp(receiveFileName, "help") == 0
            || strcmp(receiveFileName, "exit") == 0 || strcmp(receiveFileName, "sendFile") == 0)) {
            printf("--- Command <%s> ---\n", receiveFileName);
            continue;
        }
        
        file_length = strtoll(receiveFileName,&file_path,10);
        
        if (*file_path != ',') {
            fprintf(stderr,"syntax error in request -- '%s'\n",
                    receiveFileName);
            exit(EXIT_FAILURE);
        }
        file_path += 1;
        
        fprintf(stderr, "is the file name received? ?   =>  %s [%lld bytes]\n",
                file_path,file_length);
        
        
        filefd = open(file_path,
                      O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (filefd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        
        for (;  file_length > 0;  file_length -= read_return) {
            read_return = BUFSIZ;
            if (read_return > file_length)
                read_return = file_length;
            
            read_return = read(connect_socket, buffer, read_return);
            if (read_return == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            if (read_return == 0)
                break;
            
            if (write(filefd, buffer, read_return) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        
        fprintf(stderr,"file complete\n");
        
        close(filefd);
    }
    
    fprintf(stderr, "%s  -- cid = %s\n", "Client dropped connection", id);
    deleteElement(all_ids, id, all_id_counter);
    --all_id_counter;
#if 0
    printf("after all id counter %d\n", all_id_counter);
    for (i = 0; i < all_id_counter; ++i) {
        puts(all_ids[i]);
    }
#endif
    
    close(connect_socket);
    free(ctl);
    
    
    return (void *) 0;
}
