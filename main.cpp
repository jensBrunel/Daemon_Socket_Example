/**
 * @file main.cpp
 * @brief Daemon socket example entry point (C++ version).
 *
 * This file contains a minimal example of a daemon-style process that
 * communicates over a socket interface. It is intended to demonstrate
 * the basic structure for creating a background service and handling
 * client connections.
 *
 * @version 1.0
 * @date 2026-08-27
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/daemon_socket_example.sock"
#define BUFFER_SIZE 256
#define TCP_PORT 12345

static void handle_signal(int signal_number)
{
    (void)signal_number;
}

static int daemonize(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid > 0) {
        exit(0);
    }

    if (setsid() < 0) {
        perror("setsid");
        return -1;
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGTERM, handle_signal);

    pid = fork();
    if (pid < 0) {
        perror("second fork");
        return -1;
    }

    if (pid > 0) {
        exit(0);
    }

    umask(0);

    if (chdir("/") < 0) {
        perror("chdir");
        return -1;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

int main()
{
    int unix_fd = -1;
    int tcp_fd = -1;
    int client_fd = -1;
    struct sockaddr_un address;
    struct sockaddr_in tcp_addr;
    char buffer[BUFFER_SIZE];
    ssize_t received = 0;

    if (daemonize() != 0) {
        return 1;
    }

    unlink(SOCKET_PATH);

    unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_fd < 0) {
        perror("unix socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path), "%s", SOCKET_PATH);

    if (bind(unix_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("unix bind");
        close(unix_fd);
        return 1;
    }

    if (listen(unix_fd, 5) < 0) {
        perror("unix listen");
        close(unix_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        perror("tcp socket");
        close(unix_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    int opt = 1;
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(tcp_fd);
        close(unix_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_addr.sin_port = htons(TCP_PORT);

    if (bind(tcp_fd, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("tcp bind");
        close(tcp_fd);
        close(unix_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    if (listen(tcp_fd, 5) < 0) {
        perror("tcp listen");
        close(tcp_fd);
        close(unix_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    while (1) {
        fd_set readfds;
        int max_fd = (unix_fd > tcp_fd) ? unix_fd : tcp_fd;

        FD_ZERO(&readfds);
        FD_SET(unix_fd, &readfds);
        FD_SET(tcp_fd, &readfds);

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(unix_fd, &readfds)) {
            client_fd = accept(unix_fd, NULL, NULL);
            if (client_fd < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("accept (unix)");
                break;
            }

            while ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[received] = '\0';
                printf("Unix Received: %s\n", buffer);
                fflush(stdout);
            }

            if (received < 0 && errno != EINTR) {
                perror("recv (unix)");
            }

            close(client_fd);
            client_fd = -1;
        }

        if (FD_ISSET(tcp_fd, &readfds)) {
            client_fd = accept(tcp_fd, NULL, NULL);
            if (client_fd < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("accept (tcp)");
                break;
            }

            while ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[received] = '\0';
                printf("TCP Received: %s\n", buffer);
                fflush(stdout);
            }

            if (received < 0 && errno != EINTR) {
                perror("recv (tcp)");
            }

            close(client_fd);
            client_fd = -1;
        }
    }

    if (tcp_fd >= 0) close(tcp_fd);
    if (unix_fd >= 0) {
        close(unix_fd);
        unlink(SOCKET_PATH);
    }

    return 0;
}
