/**
 * @file main.c
 * @brief Daemon socket example entry point.
 *
 * This file contains a minimal example of a daemon-style process that
 * communicates over a socket interface. It is intended to demonstrate
 * the basic structure for creating a background service and handling
 * client connections.
 *
 * @details
 * The example is intentionally small and educational. It focuses on the
 * overall daemon pattern and socket setup without introducing extra
 * production complexity.
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
#include <unistd.h>

#define SOCKET_PATH "/tmp/daemon_socket_example.sock"
#define BUFFER_SIZE 256

static void handle_signal(int signal_number)
{
    (void)signal_number;
    /* A real daemon would handle cleanup here. */
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

int main(void)
{
    int server_fd = -1;
    int client_fd = -1;
    struct sockaddr_un address;
    char buffer[BUFFER_SIZE];
    ssize_t received = 0;

    if (daemonize() != 0) {
        return 1;
    }

    unlink(SOCKET_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path), "%s", SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            break;
        }

        while ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[received] = '\0';
            printf("Received: %s\n", buffer);
            fflush(stdout);
        }

        if (received < 0 && errno != EINTR) {
            perror("recv");
        }

        close(client_fd);
    }

    unlink(SOCKET_PATH);
    close(server_fd);
    return 0;
}

