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

int Socket(int domain, int type, int protocol){
    int listen;
    if ((listen = socket(domain, type, protocol)) == -1)
    {
        perror("socket");
        exit(1);
    }
    return listen;
}

// Função para lidar com um cliente conectado
void processarCliente(int connfd, int ehTCP)
{
    char buf[MAXDATASIZE];
    char infoCliente[BUFF_SIZE];

    // Obtendo informações sobre o cliente
    getpeername(connfd, (struct sockaddr *)&cliente_info, &tamanho_endereco_cliente);
    char bufferLocal[20];
    inet_ntop(AF_INET, &cliente_info.sin_addr, bufferLocal, BUFF_SIZE);

    sprintf(infoCliente, "%s:%d", bufferLocal, (int)ntohs(cliente_info.sin_port));

    if (ehTCP)
    {
        // Enviar uma mensagem de boas-vindas para o cliente TCP
        time_t current_time;
        time(&current_time);
        char *time_str = ctime(&current_time);

        dprintf(connfd, "Hello from server to client in:\nIP address: %s\nPort: %d\nTime: %s", bufferLocal, (int)ntohs(cliente_info.sin_port), time_str);
        printf("TCP client connected in: %s\n", infoCliente);
    }

    while (1)
    {
        // Recebendo uma mensagem do cliente
        ssize_t tamanhoDaMensagemRecebida = recv(connfd, buf, sizeof(buf) - 1, 0);
        if (tamanhoDaMensagemRecebida < 0)
        {
            perror("recv error");
            break;
        }
        else if (tamanhoDaMensagemRecebida == 0)
        {
            
            printf("%s: Conexão foi FINALIZADA\n", infoCliente);
            break;
        }

        buf[tamanhoDaMensagemRecebida] = '\0';
        printf("%s: %s\n", infoCliente, buf);

        // Enviar uma mensagem de boas-vindas para o cliente UDP
        if (!ehTCP)
        {
            dprintf(connfd, "Hello Client UDP");
        }

        // Ecoar a mensagem de volta para o cliente
        if (write(connfd, buf, strlen(buf)) < 0)
        {
            perror("write error");
            exit(1);
        }
    }

    close(connfd);
}

void processarClienteUDP(int socket)
{
    char buf[MAXDATASIZE];
    char infoCliente[BUFF_SIZE];

    // Obtendo informações sobre o cliente
    socklen_t len = sizeof(cliente_info);
    if (recvfrom(socket, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&cliente_info, &len) < 0)
    {
        perror("recvfrom error");
        exit(1);
    }

    char bufferLocal[20];
    inet_ntop(AF_INET, &cliente_info.sin_addr, bufferLocal, BUFF_SIZE);

    sprintf(infoCliente, "%s:%d", bufferLocal, (int)ntohs(cliente_info.sin_port));
    
    // // Enviar uma mensagem de boas-vindas para o cliente UDP
    // const char *mensagem = "Message from server: Hello Client UDP";
    // if (sendto(socket, mensagem, strlen(mensagem), 0, (struct sockaddr *)&cliente_info, sizeof(cliente_info)) < 0)
    // {
    //     perror("Erro ao enviar mensagem para o cliente UDP");
    //     exit(1);
    // }

    while (1)
    {
        // Recebendo uma mensagem do cliente
        ssize_t tamanhoDaMensagemRecebida = recv(socket, buf, sizeof(buf) - 1, 0);
        if (tamanhoDaMensagemRecebida < 0)
        {
            perror("recv error");
            break;
        }
        else if (tamanhoDaMensagemRecebida == 0)
        {
            
            printf("%s: Conexão foi FINALIZADA\n", infoCliente);
            break;
        }

        buf[tamanhoDaMensagemRecebida] = '\0';
        printf("%s: %s\n", infoCliente, buf);

        // Enviar uma mensagem de boas-vindas para o cliente UDP
        const char *resposta = "Message from server: Hello Client UDP";
        if (sendto(socket, resposta, strlen(resposta), 0, (struct sockaddr *)&cliente_info, sizeof(cliente_info)) < 0)
        {
            perror("Erro ao enviar mensagem para o cliente UDP");
            exit(1);
        }
    }
}


int main(int argc, char **argv)
{    
    int listenTCPfd, listenUDPfd;
    int connfd;
    pid_t childpid;
    char error[MAXLINE + 1];
    char *p;

    if (argc != 2)
    {
        strcpy(error, "É necessário informar a porta que o servidor irá escutar");
        perror(error);
        exit(0);
    }

    long arg = strtol(argv[1], &p, 10);
    int porta = arg;
    int portaUDP = 8888;

    // Criando socket TCP
    listenTCPfd = Socket(AF_INET, SOCK_STREAM, 0);

    // Criando socket UDP
    listenUDPfd = Socket(AF_INET, SOCK_DGRAM, 0);

    // Configurando endereço do servidor
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    servidor.sin_port = htons(porta);

    // Associação do socket TCP com o endereço do servidor
    if (bind(listenTCPfd, (struct sockaddr *)&servidor, sizeof(servidor)) == -1)
    {
        perror("bind");
        exit(1);
    }

    // Configurando endereço do servidor UDP
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    servidor.sin_port = htons(portaUDP);

    // Associação do socket UDP com o endereço do servidor
    if (bind(listenUDPfd, (struct sockaddr *)&servidor, sizeof(servidor)) == -1)
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

    fd_set readfds;
    int maxfd;

    for (;;)
    {
        FD_ZERO(&readfds);

        FD_SET(listenTCPfd, &readfds);
        FD_SET(listenUDPfd, &readfds);

        maxfd = (listenTCPfd > listenUDPfd) ? listenTCPfd : listenUDPfd;

        // Monitorar os descritores de leitura usando select
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(1);
        }

        // Verificar se há uma conexão TCP pendente
        if (FD_ISSET(listenTCPfd, &readfds))
        {
            tamanho_endereco_cliente = sizeof(endereco_cliente);

            // Aceitar uma conexão do cliente TCP
            if ((connfd = accept(listenTCPfd, (struct sockaddr *)NULL, NULL)) == -1)
            {
                perror("accept");
                continue;
            }

            if ((childpid = fork()) == 0)
            {
                close(listenTCPfd);
                processarCliente(connfd, 1);
                exit(0);
            }

            close(connfd);
        }

        // Verificar se há dados disponíveis no socket UDP
        if (FD_ISSET(listenUDPfd, &readfds))
        {
            tamanho_endereco_cliente = sizeof(endereco_cliente);

            // Receber uma mensagem do cliente UDP
           
            processarClienteUDP(listenUDPfd);
            close(connfd);
        }
    }

    close(listenTCPfd);
    close(listenUDPfd);
    return (0);
}
