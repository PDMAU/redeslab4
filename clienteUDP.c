#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 4096
#define BUFF_SIZE 100

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <EndereçoIP> <Porta>\n", argv[0]);
        exit(1);
    }

    int sockfd, n;
    char recvline[MAXLINE + 1];
    struct sockaddr_in servidor;

    // Cria um socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Erro de socket");
        exit(1);
    }

    // Configura o endereço do servidor
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &servidor.sin_addr) <= 0)
    {
        perror("Erro ao converter endereço IP");
        exit(1);
    }

    // Envia uma mensagem "Hello, I am a UDP client" para o servidor
    const char *mensagem = "Hello, I am a UDP client";
    if (sendto(sockfd, mensagem, strlen(mensagem), 0, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
    {
        perror("Erro ao enviar mensagem para o servidor");
        exit(1);
    }

    // Espera por resposta do servidor
    socklen_t len = sizeof(servidor);
    if ((n = recvfrom(sockfd, recvline, MAXLINE, 0, (struct sockaddr *)&servidor, &len)) < 0)
    {
        perror("Erro ao receber mensagem do servidor");
        exit(1);
    }

    recvline[n] = 0;
    printf("Message from server: %s\n", recvline);

    // Fecha o socket e sai
    close(sockfd);

    exit(0);
}
