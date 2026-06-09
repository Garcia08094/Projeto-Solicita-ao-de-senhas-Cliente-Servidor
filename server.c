#include "common.h"

#ifdef _WIN32
#include <process.h>
typedef CRITICAL_SECTION mutex_t;
#define THREAD_RETURN unsigned __stdcall
#define MUTEX_INIT(m) InitializeCriticalSection(m)
#define MUTEX_LOCK(m) EnterCriticalSection(m)
#define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#define MUTEX_DESTROY(m) DeleteCriticalSection(m)
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#define THREAD_RETURN void *
#define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
#define MUTEX_LOCK(m) pthread_mutex_lock(m)
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
#endif

typedef struct {
    socket_t socket_fd;
    struct sockaddr_in address;
} client_info_t;

/* Proxima senha que sera entregue. Fica no servidor para manter a sequencia unica. */
static int next_password_number = 1;

/* Protege o contador quando varios clientes solicitam senha ao mesmo tempo. */
static mutex_t password_mutex;

/* Monta uma identificacao legivel do cliente para exibir no terminal do servidor. */
static void format_client_address(const struct sockaddr_in *address, char *output, size_t output_size)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address->sin_addr), ip, sizeof(ip));
    snprintf(output, output_size, "%s:%d", ip, ntohs(address->sin_port));
}

/* Gera uma nova senha sequencial no formato A001, A002, A003... */
static void generate_password(char *password, size_t password_size)
{
    int number;

    MUTEX_LOCK(&password_mutex);
    number = next_password_number++;
    MUTEX_UNLOCK(&password_mutex);

    snprintf(password, password_size, "%c%03d", PASSWORD_PREFIX, number);
}

/* Retorna a ultima senha gerada sem alterar a sequencia. */
static int current_last_password(void)
{
    int last_password;

    MUTEX_LOCK(&password_mutex);
    last_password = next_password_number - 1;
    MUTEX_UNLOCK(&password_mutex);

    return last_password;
}

/* Processa os comandos enviados pelo cliente e devolve a resposta correta. */
static void process_command(socket_t client_socket, const char *command, const char *client_label)
{
    char response[BUFFER_SIZE];

    if (strcmp(command, "SOLICITAR") == 0 || strcmp(command, "1") == 0) {
        char password[16];
        generate_password(password, sizeof(password));
        snprintf(response, sizeof(response), "Senha gerada: %s\n", password);
        send_all(client_socket, response);
        printf("Senha %s gerada para %s\n", password, client_label);
    } else if (strcmp(command, "STATUS") == 0 || strcmp(command, "2") == 0) {
        int last_password = current_last_password();
        if (last_password == 0) {
            send_all(client_socket, "Nenhuma senha foi gerada ainda.\n");
        } else {
            snprintf(response, sizeof(response), "Ultima senha gerada: %c%03d\n", PASSWORD_PREFIX, last_password);
            send_all(client_socket, response);
        }
    } else if (strcmp(command, "SAIR") == 0 || strcmp(command, "3") == 0) {
        send_all(client_socket, "Conexao encerrada pelo servidor.\n");
    } else {
        send_all(client_socket, "Comando invalido. Use SOLICITAR, STATUS ou SAIR.\n");
    }
}

/* Cada cliente conectado e atendido por esta funcao em uma thread separada. */
static THREAD_RETURN handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    socket_t client_socket = client->socket_fd;
    char client_label[64];
    char buffer[BUFFER_SIZE];

    format_client_address(&client->address, client_label, sizeof(client_label));
    printf("Cliente conectado: %s\n", client_label);
    send_all(client_socket, "Bem-vindo ao Sistema de Senhas.\nComandos: SOLICITAR, STATUS ou SAIR.\n");

    free(client);

    /* Continua recebendo comandos ate o cliente sair ou desconectar. */
    while (1) {
        int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            printf("Cliente desconectado: %s\n", client_label);
            break;
        }

        buffer[received] = '\0';
        trim_newline(buffer);

        if (strlen(buffer) == 0) {
            continue;
        }

        process_command(client_socket, buffer, client_label);

        if (strcmp(buffer, "SAIR") == 0 || strcmp(buffer, "3") == 0) {
            printf("Cliente encerrou a conexao: %s\n", client_label);
            break;
        }
    }

    CLOSE_SOCKET(client_socket);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* Cria o socket TCP do servidor, associa a porta e inicia a escuta por clientes. */
static socket_t create_server_socket(int port)
{
    socket_t server_socket;
    struct sockaddr_in server_address;
    int option = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == SOCKET_INVALID) {
        fprintf(stderr, "Erro ao criar socket do servidor.\n");
        return SOCKET_INVALID;
    }

    /* Permite reiniciar o servidor usando a mesma porta sem esperar tanto tempo. */
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&option, sizeof(option));

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons((unsigned short)port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR_VALUE) {
        fprintf(stderr, "Erro ao associar servidor a porta %d.\n", port);
        CLOSE_SOCKET(server_socket);
        return SOCKET_INVALID;
    }

    if (listen(server_socket, BACKLOG) == SOCKET_ERROR_VALUE) {
        fprintf(stderr, "Erro ao colocar servidor em modo de escuta.\n");
        CLOSE_SOCKET(server_socket);
        return SOCKET_INVALID;
    }

    return server_socket;
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    socket_t server_socket;

    if (argc >= 2) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Porta invalida. Use um valor entre 1 e 65535.\n");
            return 1;
        }
    }

    if (!initialize_network()) {
        return 1;
    }

    MUTEX_INIT(&password_mutex);
    server_socket = create_server_socket(port);
    if (server_socket == SOCKET_INVALID) {
        MUTEX_DESTROY(&password_mutex);
        cleanup_network();
        return 1;
    }

    printf("Servidor iniciado na porta %d.\n", port);
    printf("Aguardando clientes...\n");

    /* Loop principal: aceita conexoes e cria uma thread para cada cliente. */
    while (1) {
        client_info_t *client = (client_info_t *)malloc(sizeof(client_info_t));
#ifdef _WIN32
        int address_size = sizeof(client->address);
#else
        socklen_t address_size = sizeof(client->address);
#endif

        if (client == NULL) {
            fprintf(stderr, "Erro de memoria ao aceitar cliente.\n");
            continue;
        }

        client->socket_fd = accept(server_socket, (struct sockaddr *)&client->address, &address_size);
        if (client->socket_fd == SOCKET_INVALID) {
            fprintf(stderr, "Erro ao aceitar conexao de cliente.\n");
            free(client);
            continue;
        }

#ifdef _WIN32
        uintptr_t thread_handle = _beginthreadex(NULL, 0, handle_client, client, 0, NULL);
        if (thread_handle == 0) {
            fprintf(stderr, "Erro ao criar thread do cliente.\n");
            CLOSE_SOCKET(client->socket_fd);
            free(client);
        } else {
            CloseHandle((HANDLE)thread_handle);
        }
#else
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client) != 0) {
            fprintf(stderr, "Erro ao criar thread do cliente.\n");
            CLOSE_SOCKET(client->socket_fd);
            free(client);
        } else {
            pthread_detach(thread_id);
        }
#endif
    }

    CLOSE_SOCKET(server_socket);
    MUTEX_DESTROY(&password_mutex);
    cleanup_network();
    return 0;
}
