#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>

#define LISTENQ 10
#define MAXDATASIZE 100
#define BUFF_SIZE 100
#define MAXLINE 4096

struct sockaddr_in servidor, cliente_info, endereco_cliente;
socklen_t tamanho_endereco_cliente;
FILE *arquivo_log;

// Função para obter uma mensagem com base em um número
char* obterMensagem(int numeroDaMensagem){
    if (numeroDaMensagem == 1) {
        return "SIMULE: CPU_INTENSIVA";
    }
    
    if (numeroDaMensagem == 2) {
        return "SIMULE: MEMORIA_INTENSIVA";
    }
    
    if (numeroDaMensagem == 3) {
        return "SIMULE: GPU_INTENSIVA";
    }
    return "SIMULE: DISCO_INTENSIVO";
}

// Função para lidar com um cliente conectado
void processarCliente(int connfd)
{
    char buf[MAXDATASIZE];
    char infoCliente[BUFF_SIZE];

    // Obtendo informações sobre o cliente
    getpeername(connfd, (struct sockaddr *)&cliente_info, &tamanho_endereco_cliente);
    char bufferLocal[20];
    inet_ntop(AF_INET, &cliente_info.sin_addr, bufferLocal, BUFF_SIZE);
    
    sprintf(infoCliente, "%s:%d", bufferLocal,(int) ntohs(cliente_info.sin_port));
    
    while (1)
    {       
        // Recebendo uma mensagem do cliente, esperando a mensagem inicial enviada pelo cliente
        ssize_t tamanhoDaMensagemRecebida = recv(connfd, buf, sizeof(buf) - 1, 0);
        if (tamanhoDaMensagemRecebida < 0)
        {
            perror("recv error");
            break;
        }
        else if (tamanhoDaMensagemRecebida == 0)
        {
            printf("Conexão fechada pelo cliente\n");
            fprintf(arquivo_log, "%s: Conexão foi FINALIZADA\n", infoCliente);
            break;
        }

        if (strcmp(buf, "DESCONECTAR") == 0)
        {
            fprintf(arquivo_log, "%s: Conexão foi FINALIZADA\n", infoCliente);
            break;
        }else {
            buf[tamanhoDaMensagemRecebida] = '\0';        
            fprintf(arquivo_log, "%s: %s\n", infoCliente, buf);
        }
        fflush(arquivo_log);
        // Limpar o buffer
        memset(buf, 0, sizeof(buf));
        
        // Enviar uma mensagem para o cliente, informando qual operacao deve ser executada
        // utilizamos um random para escolher alguma das opçoes
        const char *mensagem = obterMensagem(rand() % 3);
        if (write(connfd, mensagem, strlen(mensagem)) < 0) {
            perror("write error");
            exit(1);
        }
        fprintf(arquivo_log, "%s: %s\n", infoCliente, mensagem);
    }
    
    close(connfd);
}

int main(int argc, char **argv)
{
    // Abertura de arquivo para registro de log
    arquivo_log = fopen("log.txt", "w"); 
    int listenTCPfd, connfd;
    int listenUDPfd, connfd;
    pid_t childpid;
    char   error[MAXLINE + 1];
    char* p;    

    if (argc != 2) {
        strcpy(error, "É necessário informar a porta que o servidor irá escutar");
        perror(error);
        exit(0);
    }

    long arg = strtol(argv[1], &p, 10);
    int porta = arg;
    int portaUDP = 8888;
 
    // Criando socket TCP
    if ((listenTCPfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // Criando socket UDP
    if ((listenUDPfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // Configurando endereço do servidor
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    servidor.sin_port = htons(porta);

    // Associação do socket com o endereço do servidor TCP
    if (bind(listenTCPfd, (struct sockaddr *)&servidor, sizeof(servidor)) == -1)
    {
        perror("bind");
        exit(1);
    }

    // Colocar o socket TCP em modo de escuta
    if (listen(listenTCPfd, LISTENQ) == -1)
    {
        perror("listen");
        exit(1);
    }

    for (;;)
    {
        tamanho_endereco_cliente = sizeof(endereco_cliente);
        // Aceitando uma conexão do cliente
        if ((connfd = accept(listenTCPfd, (struct sockaddr *)NULL, NULL)) == -1) 
            { //check se for TCP: perror("accept");
                for(int i = 0; i < FD_SETSIZE; i++) { //3 clientes consecutivos
                    
                }
            }

            if ((childpid = fork()) == 0)
            {                          
                close(listenTCPfd);       
                processarCliente(connfd); 
                exit(0);              
            }

            close(connfd); 
    }

    close(listenTCPfd);
    fflush(arquivo_log);
    fclose(arquivo_log);
    return (0);
}
