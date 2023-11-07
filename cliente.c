#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096
#define BUFF_SIZE 100

// Função para determinar a mensagem a ser enviada com base na mensagem recebida
char* pegarMensagem(char  *msg){
    if ((strcmp(msg, "SIMULE: CPU_INTENSIVA\0") == 0)) {
        return "SIMULAÇÃO: CPU_INTENSIVA CONCLUÍDA";
    }
    
    if ((strcmp(msg, "SIMULE: MEMORIA_INTENSIVA\0") == 0)) {
        return "SIMULAÇÃO: MEMORIA_INTENSIVA CONCLUÍDA";
    }
    
    if ((strcmp(msg, "SIMULE: GPU_INTENSIVA\0") == 0)) {
        return "SIMULAÇÃO: GPU_INTENSIVA CONCLUÍDA";
    }
    return "SIMULAÇÃO: DISCO_INTENSIVO CONCLUÍDA";
}

int main(int argc, char **argv) {
    int    sockfd, n;
    unsigned int sa_len;
    char   recvline[MAXLINE + 1];
    char   error[MAXLINE + 1];
    struct sockaddr_in servidor;
    struct sockaddr_in infoCliente;
    struct sockaddr_in infoServidor;
    
    if (argc != 2) {
        strcpy(error, "Uso: ");
        strcat(error, argv[0]);
        strcat(error, " <EndereçoIP>");
        perror(error);
        exit(1);
    }

    // Cria um socket para a conexao
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro de socket");
        exit(1);
    }

    // Configura o com base no IP passado como argumento
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port   = htons(8089);
    if (inet_pton(AF_INET, argv[1], &servidor.sin_addr) <= 0) {
        perror("Erro ao converter endereço IP");
        exit(1);
    }

    // Estabelece conexão com o servidor
    if (connect(sockfd, (struct sockaddr *) &servidor, sizeof(servidor)) < 0) {
        perror("Erro de conexão");
        exit(1);
    }

    // Pega os dados cliente
    sa_len = sizeof(infoCliente);
    getsockname(sockfd, (struct sockaddr *)&infoCliente, &sa_len);

    char bufferCliente[20];
    const char* pCliente = inet_ntop(AF_INET, &infoCliente.sin_addr, bufferCliente, BUFF_SIZE);        
    if(pCliente != NULL) {
        printf("Cliente Local: %s:%d\n" , bufferCliente,(int) ntohs(infoCliente.sin_port));        
    }

    // Pega as informações do servidor
    sa_len = sizeof(infoServidor);
    getpeername(sockfd, (struct sockaddr *)&infoServidor, &sa_len);
    
    char bufferServidor[BUFF_SIZE];
    const char* pServidor = inet_ntop(AF_INET, &infoServidor.sin_addr, bufferServidor, BUFF_SIZE);        
    if(pServidor != NULL) {
        printf("Servidor: %s:%d\n" , bufferServidor,(int) ntohs(infoServidor.sin_port));
    }

    // Envia uma mensagem para o servidor
    char mensagem[BUFF_SIZE] ;
    sprintf(mensagem, "Cliente %s:%d", bufferCliente,(int) ntohs(infoCliente.sin_port));
    if (write(sockfd, mensagem, strlen(mensagem)) < 0) {
        perror("Erro ao escrever para o servidor");
        exit(1);
    }

    while (1) {  
        // Fica esperando e le as mensagens enviadas pelo servidor
        if ((n = read(sockfd, recvline, MAXLINE)) > 0) {       
            recvline[n] = 0;
            sleep(2);
            // Com base na mensagem recebida, se envia uma mensagem devolta para o servidor
            const char *mensagem = ((rand() % 10) != 1? pegarMensagem(recvline) : "DESCONECTAR");
            if (write(sockfd, mensagem, strlen(mensagem)) < 0) {
                perror("Erro ao escrever para o servidor");
                exit(1);
            }            
        } else if (n == 0) { // Servidor fechou a conexao
            printf("Conexão fechada pelo servidor\n");
            break; 
        }
    }

    close(sockfd); // Fecha o socket
    exit(0);
}
