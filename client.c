# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>

struct sockaddr_in set_addr(char *host, int port){
   struct sockaddr_in sa;
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = inet_addr(host); //host = "127.0.0.1"
   sa.sin_port = htons(port);
   memset(&sa.sin_zero, '\0', sizeof(sa.sin_zero));
   return sa;
}

int set_socket(char *client_host, int port){
   struct sockaddr_in client_addr = set_addr(client_host, port);
   struct sockaddr *addr = (struct sockaddr *)&client_addr;
   int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(sockfd < 0){
      perror("socket");
      exit(-1);
   }
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
    perror("setsockopt() error...\n");
    exit(1);
    }
   int bind_sock = bind(sockfd, addr, sizeof(struct sockaddr));
   if(bind_sock == -1){
      perror("bind");
      exit(-1);
   }
   return sockfd;
}

int connect_to_server(char *host, int port, int sockfd){
    struct sockaddr *server_addr;
    struct sockaddr_in server_addr_in = set_addr(host, port);
    server_addr = (struct sockaddr*) &server_addr_in;
    int connect_server = connect(sockfd, server_addr, sizeof(struct sockaddr));
    if(connect_server == -1){
    perror("connect");
    exit(-1);
    }
}

int main(int argc, char **argv){
    char host[100];
    int port;
    if(argc < 3){
        printf("Usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }
    strcpy(host, argv[1]);
    port = atoi(argv[2]);
    printf("%s:%d\n", host, port);
    char buffer[1000] = { 0 };
    int signal = 0;
    while(1){
        
        pid_t pid = fork();
        
        if(signal == 1) exit(0);
        int sockfd = set_socket(host, 0);
        connect_to_server(host, port, sockfd);
        if(pid < 0){
            perror("fork");
            exit(0);}
        else if(pid == 0){
            read(sockfd, buffer, sizeof(buffer));
            printf("%s", buffer);
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer), stdin);
            if(strcmp(buffer, "\n")==0){
                kill(pid, 1);
            }
            while (buffer[strlen(buffer) - 1] == '\n'){
                buffer[strlen(buffer) - 1] = 0;
            }
            if(write(sockfd, buffer, strlen(buffer)+1)<0){
                perror("write");
                exit(-1);
            }
            read(sockfd, buffer, sizeof(buffer));
            printf("%s", buffer);
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer), stdin);
            if(strcmp(buffer, "\n")==0){
                kill(pid, 1);
            }
            while (buffer[strlen(buffer) - 1] == '\n'){
                buffer[strlen(buffer) - 1] = 0;
            }
            if(write(sockfd, buffer, strlen(buffer)+1)<0){
                perror("write");
                exit(-1);
            }
            read(sockfd, buffer, sizeof(buffer));
            printf("%s", buffer);
            exit(0);
        }
        else if(pid > 0 ){
            wait(0);
        }
        // if(signal == 1) exit(0);
    }   
    return 0;
}