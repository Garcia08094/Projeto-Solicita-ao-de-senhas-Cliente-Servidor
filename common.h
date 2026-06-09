#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#define SOCKET_INVALID INVALID_SOCKET
#define SOCKET_ERROR_VALUE SOCKET_ERROR
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define CLOSE_SOCKET close
#define SOCKET_INVALID -1
#define SOCKET_ERROR_VALUE -1
#endif

#define DEFAULT_PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 256
#define PASSWORD_PREFIX 'A'

/* Inicializa a rede. No Windows, a biblioteca Winsock precisa ser iniciada antes do uso de sockets. */
static int initialize_network(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "Erro ao inicializar Winsock.\n");
        return 0;
    }
#endif
    return 1;
}

/* Libera recursos da rede. No Linux/macOS nao ha finalizacao especial para sockets. */
static void cleanup_network(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/* Remove '\n' e '\r' do fim das mensagens digitadas ou recebidas pela rede. */
static void trim_newline(char *text)
{
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        length--;
    }
}

/* Garante que a mensagem inteira seja enviada, mesmo que o send envie apenas uma parte. */
static int send_all(socket_t socket_fd, const char *message)
{
    size_t total_sent = 0;
    size_t message_length = strlen(message);

    while (total_sent < message_length) {
        int sent = send(socket_fd, message + total_sent, (int)(message_length - total_sent), 0);
        if (sent == SOCKET_ERROR_VALUE || sent == 0) {
            return 0;
        }
        total_sent += (size_t)sent;
    }

    return 1;
}

#endif
