// gcc server_vuln.c -o server_vuln
// 0 - account blocked; 1 - account OK; 2 - account not ready;

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

#define BUFFER_SIZE 64


struct sockaddr_in set_address(char *host, int port){
    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = inet_addr(host);
    memset(&addr_in.sin_zero, 0, sizeof(addr_in.sin_zero));
    return addr_in;
}

int set_socket(char *host, int port){
    struct sockaddr_in addr_in = set_address(host, port);
    struct sockaddr *addr = (struct sockaddr*)&addr_in;
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0){
        perror("socket");
        exit(1);}
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
    perror("setsockopt() error...\n");
    exit(1);
    }
    int bindfd = bind(sockfd, addr, sizeof(struct sockaddr));
    if(bindfd < 0){
        perror("bind");
        exit(1);}
    return sockfd;
}

int readFile(char *account[], int status[]){
    FILE *fp = fopen("nguoidung.txt", "r+");
    if(!fp){
        perror("fopen");
        exit(1);}
    int i = 0;
    while(!feof(fp)){
        account[i] = (char*)malloc(sizeof(char)*64);
        account[i+1] = (char*)malloc(sizeof(char)*64);
        fscanf(fp, "%s %s %d\n", account[i], account[i+1], &status[i]);
        i=i+2;
    }
    return i-2;
}

void writeFile(char *account[], int status[], int size){
    FILE *fp = fopen("nguoidung.txt", "w+");
    if(!fp){
        perror("fopen");
        exit(1);}
    int i = 0;
    while(i<=size){
        fprintf(fp, "%s %s %d\n", account[i], account[i+1], status[i]);
        i = i+2;
    }
}

int passHandler(int connfd, char *password){
    int i = 0, j = 0, k = 0;
    char alphabets[64];
    char numbers[64];
    for(i ; i < strlen(password) ; i++){
        if('a'<=password[i]&&password[i]<='z'){
            alphabets[j] = password[i];
            j++;
        }
        else if('A'<=password[i]&&password[i]<='Z'){
            alphabets[j] = password[i];
            j++;
        }
        else if('0'<=password[i]&&password[i]<='9'){
            numbers[k] = password[i];
            k++;
        }
    }
    if(( j + k - strlen(password)) > 0){
        write(connfd, "Error\n", strlen("Error\n"));
        return 0;
    }
    if(j>0){
        alphabets[j] = '\n';
        write(connfd, alphabets, j+1);}
    if(i>0){
        numbers[k] = '\n';
        write(connfd, numbers, k+1);
    } 
    return 1;
}

void read_blocklist(int block_status[], int size){
    FILE *fp = fopen("block.txt", "r");
    if(!fp){
        perror("fopen");
        exit(1);}
    int i = 0;
    while(i<=size){
        fscanf(fp, "%d\n", &block_status[i]);
        i=i+2;
    }
}
void write_blocklist(int block_status[], int size){
    FILE *fp = fopen("block.txt", "w");
    if(!fp){
        perror("fopen");
        exit(1);}
    int i = 0;
    while(i<=size){
        fprintf(fp, "%d\n", block_status[i]);
        i=i+2;
    }
}


int auth(int connectfd, char *account[], int status[], int size, int *i){
    char message[] = "User Access Verification\n\nUsername: ";
    write(connectfd, message, strlen(message));
    char buffer[BUFFER_SIZE];
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    read(connectfd, buffer, 64);
    strcpy(username, buffer);
    write(connectfd, "\nPassword: ", strlen("\nPassword: "));
    read(connectfd, buffer, 64);
    strcpy(password, buffer);
    for(*i = 0; *i < size; *i=*i+2){
        if(strncmp(username, account[*i], strlen(account[*i])) == 0){
            if(strncmp(password, account[*i+1], (strlen(password)-1)) == 0 && status[*i] == 1){
                return -1;
            }
            else if(strncmp(password, account[*i+1], strlen(account[*i+1])) == 0 && status[*i] == 2){
                return -2;
            }
            else if(strncmp(password, account[*i+1], strlen(account[*i+1])) == 0 && status[*i] == 0){
                return -3;
            }
            else{
                return *i;
            } 
        }
    }
    return *i;
}


int main(int argc, char *argv[]){
    if(argc < 3){
        fprintf(stderr, "Usage: %s host port\n", argv[0]);
        exit(1);
    }
    char *host = argv[1];
    int port = atoi(argv[2]);
    char *account[100];
    int i = 0, status[100];
    int block_status[100] = {0};
    int sockfd = set_socket(host, port);
    if(listen(sockfd, 1) == -1) {
        perror("listen");
        close(sockfd);
        exit(-1);
    }
    printf("Listening on %s port %d\n", host, port);
    struct sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(peer_addr);
    int connfd;
    while(1) {
    if ((connfd = accept(sockfd, (struct sockaddr*) &peer_addr, &addr_len)) < 0){
        exit(-1);
    }
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      exit(0); 
    }
    int size = readFile(account, status);
    if (pid == 0) { //Child
        close(sockfd);
        int i = 0;
        int authid = auth(connfd, account, status, size, &i);
        if (authid == -1) {
            char success[] = "OK\n";
            write(connfd, success, strlen(success));
            write(connfd, "New password: \n", strlen("New password: \n"));
            char buffer[BUFFER_SIZE];
            int buff_size = read(connfd, buffer, BUFFER_SIZE);
            if(strncmp(buffer, "bye", strlen("bye")) == 0){
                char buf[9] = "Goodbye ";
                strcat(buf, account[i]);
                write(connfd, buf, strlen(buf));
                close(connfd);
            }
            else{
                memset(account[i+1], '\0', sizeof(account[i]));
                strncpy(account[i+1], buffer, buff_size-1);              
                if(passHandler(connfd, account[i+1])){
                    writeFile(account, status, size);
                }
            }
        }
        else if(authid == -2){
            char notReady[] = "Account not ready!";
            write(connfd, notReady, strlen(notReady));
        }
        else if(authid == -3){
            char blocked[] = "Account is blocked!";
            write(connfd, blocked, strlen(blocked));
        }
        else {
            char invalid[] = "not OK";
            write(connfd, invalid, strlen(invalid));
            if(authid < size){
                read_blocklist(block_status, size);
                block_status[authid] ++;
                if(block_status[authid] == 3){
                    status[authid] = 0;
                    writeFile(account, status, size);
                }
                write_blocklist(block_status, size);
            }
        }
    } else if (pid > 0) { //parent
      close(connfd);
    }
  }
  return 0; 
}


