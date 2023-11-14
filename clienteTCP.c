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
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>

#define MAXLINE 4096
#define BUFF_SIZE 100



int main(int argc, char **argv)
{
    int sockfd, n;
    unsigned int sa_len;
    char recvline[MAXLINE + 1];
    char error[MAXLINE + 1];
    struct sockaddr_in servidor;
    struct sockaddr_in infoCliente;
    struct sockaddr_in infoServidor;

    if (argc != 3)
    {
        strcpy(error, "Uso: ");
        strcat(error, argv[0]);
        strcat(error, " <EndereçoIP> <Porta>");
        perror(error);
        exit(1);
    }

    // Cria um socket para a conexao
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro de socket");
        exit(1);
    }

    // Configura o com base no IP e na porta passados como argumento
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servidor.sin_addr) <= 0)
    {
        perror("Erro ao converter endereço IP");
        exit(1);
    }

    // Estabelece conexão com o servidor
    if (connect(sockfd, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
    {
        perror("Erro de conexão");
        exit(1);
    }

    // Pega os dados cliente
    sa_len = sizeof(infoCliente);
    getsockname(sockfd, (struct sockaddr *)&infoCliente, &sa_len);

    char bufferCliente[20];
    const char *pCliente = inet_ntop(AF_INET, &infoCliente.sin_addr, bufferCliente, BUFF_SIZE);
    if (pCliente != NULL)
    {
        printf("Cliente Local: %s:%d\n", bufferCliente, (int)ntohs(infoCliente.sin_port));
    }

    // Pega as informações do servidor
    sa_len = sizeof(infoServidor);
    getpeername(sockfd, (struct sockaddr *)&infoServidor, &sa_len);

    char bufferServidor[BUFF_SIZE];
    const char *pServidor = inet_ntop(AF_INET, &infoServidor.sin_addr, bufferServidor, BUFF_SIZE);
    if (pServidor != NULL)
    {
        printf("Servidor: %s:%d\n", bufferServidor, (int)ntohs(infoServidor.sin_port));
    }

    // Abre o arquivo de entrada "in.txt" para leitura
    FILE *arquivoEntrada = fopen("in.txt", "r");
    if (arquivoEntrada == NULL)
    {
        perror("Erro ao abrir o arquivo de entrada");
        exit(1);
    }

    // Abre o arquivo de saída "out.txt" para escrita
    FILE *arquivoSaida = fopen("out.txt", "w");
    if (arquivoSaida == NULL)
    {
        perror("Erro ao abrir o arquivo de saída");
        fclose(arquivoEntrada);
        exit(1);
    }

    fd_set readfds, writefds;
    int maxfd;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        // Monitorar o socket para leitura (resposta do servidor)
        FD_SET(sockfd, &readfds);
        maxfd = sockfd;

        // Monitorar o socket para escrita (enviar dados ao servidor)
        FD_SET(sockfd, &writefds);

        // Adiciona o descritor do arquivo de entrada ao conjunto de leitura
        FD_SET(fileno(arquivoEntrada), &readfds);
        maxfd = (maxfd > fileno(arquivoEntrada)) ? maxfd : fileno(arquivoEntrada);

        // Verifica se há dados prontos para leitura ou escrita
        if (select(maxfd + 1, &readfds, &writefds, NULL, NULL) == -1)
        {
            perror("select");
            break;
        }

        // Se há dados prontos para leitura, lê do arquivo e envia para o servidor
        if (FD_ISSET(fileno(arquivoEntrada), &readfds))
        {
            if (fgets(recvline, MAXLINE, arquivoEntrada) != NULL)
            {
                if (write(sockfd, recvline, strlen(recvline)) < 0)
                {
                    perror("Erro ao escrever para o servidor");
                    fclose(arquivoEntrada);
                    fclose(arquivoSaida);
                    exit(1);
                }
            }
        }

        // Se há dados prontos para leitura do socket (resposta do servidor)
        if (FD_ISSET(sockfd, &readfds))
        {
            if ((n = read(sockfd, recvline, MAXLINE)) > 0)
            {
                recvline[n] = 0;
                fprintf(arquivoSaida, "%s", recvline);
            }
            else if (n == 0)
            {
                // Servidor fechou a conexão
                printf("Conexão fechada pelo servidor\n");
                break;
            }
            else
            {
                perror("Erro ao ler do servidor");
                fclose(arquivoEntrada);
                fclose(arquivoSaida);
                exit(1);
            }
        }
    }

    // Fecha os arquivos
    fclose(arquivoEntrada);
    fclose(arquivoSaida);

    // Encerra a conexão após receber a resposta completa do servidor
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    exit(0);
}
