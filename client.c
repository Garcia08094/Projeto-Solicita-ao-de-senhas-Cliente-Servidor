#include "common.h"

/* Mostra o menu de operacoes disponiveis para o usuario. */
static void show_menu(void)
{
    printf("\n=== Sistema de Senhas ===\n");
    printf("1 - Solicitar senha\n");
    printf("2 - Ver ultima senha gerada\n");
    printf("3 - Sair\n");
    printf("Digite uma opcao: ");
    fflush(stdout);
}

/* Abre o socket do cliente e faz a conexao com o servidor TCP. */
static socket_t connect_to_server(const char *host, int port)
{
    socket_t client_socket;
    struct sockaddr_in server_address;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == SOCKET_INVALID) {
        fprintf(stderr, "Erro ao criar socket do cliente.\n");
        return SOCKET_INVALID;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, host, &server_address.sin_addr) <= 0) {
        fprintf(stderr, "Endereco IP invalido: %s\n", host);
        CLOSE_SOCKET(client_socket);
        return SOCKET_INVALID;
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR_VALUE) {
        fprintf(stderr, "Nao foi possivel conectar ao servidor %s:%d.\n", host, port);
        CLOSE_SOCKET(client_socket);
        return SOCKET_INVALID;
    }

    return client_socket;
}

/* Recebe a resposta do servidor e mostra na tela do cliente. */
static int receive_response(socket_t socket_fd)
{
    char buffer[BUFFER_SIZE];
    int received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);

    if (received <= 0) {
        printf("Conexao com o servidor foi encerrada.\n");
        return 0;
    }

    buffer[received] = '\0';
    printf("%s", buffer);
    return 1;
}

int main(int argc, char *argv[])
{
    const char *host = "127.0.0.1";
    int port = DEFAULT_PORT;
    socket_t client_socket;
    char option[BUFFER_SIZE];

    if (argc >= 2) {
        host = argv[1];
    }

    if (argc >= 3) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Porta invalida. Use um valor entre 1 e 65535.\n");
            return 1;
        }
    }

    if (!initialize_network()) {
        return 1;
    }

    client_socket = connect_to_server(host, port);
    if (client_socket == SOCKET_INVALID) {
        cleanup_network();
        return 1;
    }

    printf("Conectado ao servidor %s:%d.\n", host, port);
    receive_response(client_socket);

    /* Le a opcao escolhida, envia o comando correspondente e aguarda a resposta. */
    while (1) {
        show_menu();

        if (fgets(option, sizeof(option), stdin) == NULL) {
            printf("\nEntrada encerrada.\n");
            break;
        }

        trim_newline(option);

        if (strcmp(option, "1") == 0) {
            send_all(client_socket, "SOLICITAR\n");
        } else if (strcmp(option, "2") == 0) {
            send_all(client_socket, "STATUS\n");
        } else if (strcmp(option, "3") == 0) {
            send_all(client_socket, "SAIR\n");
        } else {
            printf("Opcao invalida.\n");
            continue;
        }

        if (!receive_response(client_socket)) {
            break;
        }

        if (strcmp(option, "3") == 0) {
            break;
        }
    }

    CLOSE_SOCKET(client_socket);
    cleanup_network();
    printf("Cliente finalizado.\n");
    return 0;
}
