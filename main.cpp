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

#include "Socket.h"
#include "IniConfig.h"
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
#include <string>

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

int main(int argc, char** argv)
{
    int client_fd = -1;
    char buffer[BUFFER_SIZE];
    ssize_t received = 0;

    // Load socket path from configuration (if available)
    std::string ini_path;
    if (argc > 1) ini_path = argv[1];
    IniConfig cfg(ini_path);
    std::string socket_path = cfg.get("SOCKET_PATH");
    if (socket_path.empty()) socket_path = "/tmp/daemon_socket_example.sock";

    if (daemonize() != 0) {
        return 1;
    }

    // Create Unix domain socket
    Socket unix_socket(SocketType::UNIX_DOMAIN);
    if (unix_socket.initUnixSocket(socket_path) < 0) {
        return 1;
    }

    // Remove old socket file if it exists
    unlink(socket_path.c_str());

    // Bind and listen on Unix socket
    if (unix_socket.bind() < 0) {
        return 1;
    }

    if (unix_socket.listen(5) < 0) {
        return 1;
    }

    // Create TCP socket (CLI pass socket) using config values
    Socket cliPassSocket(SocketType::TCP);
    std::string tcp_ip = cfg.get("TCP_IP");
    std::string port_str = cfg.get("CLI_PASSTHROUGH_PORT");
    int cli_port = 12345;
    if (tcp_ip.empty()) tcp_ip = "127.0.0.1";
    if (!port_str.empty()) {
        try {
            cli_port = std::stoi(port_str);
        } catch (...) {
            fprintf(stderr, "Invalid CLI_PASSTHROUGH_PORT '%s', using 12345\n", port_str.c_str());
        }
    }

    if (cliPassSocket.initTcpSocket(tcp_ip, cli_port) < 0) {
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    // Bind and listen on TCP socket
    if (cliPassSocket.bind() < 0) {
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    if (cliPassSocket.listen(5) < 0) {
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    // Create second TCP socket for configuration/control messages
    Socket configSocket(SocketType::TCP);
    std::string config_port_str = cfg.get("CONFIG_PORT");
    int config_port = 22345;
    if (!config_port_str.empty()) {
        try { config_port = std::stoi(config_port_str); } catch (...) {
            fprintf(stderr, "Invalid CONFIG_PORT '%s', using 22345\n", config_port_str.c_str());
        }
    }

    if (configSocket.initTcpSocket(tcp_ip, config_port) < 0) {
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        cliPassSocket.close();
        return 1;
    }

    if (configSocket.bind() < 0) {
        configSocket.close();
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    if (configSocket.listen(5) < 0) {
        configSocket.close();
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    int unix_fd = unix_socket.getFileDescriptor();
    int tcp_cliFd = cliPassSocket.getFileDescriptor();
    int tcp_cfgFd = configSocket.getFileDescriptor();

    while (1) {
        fd_set readfds;
            int max_fd = unix_fd;
            if (tcp_cliFd > max_fd) max_fd = tcp_cliFd;
            if (tcp_cfgFd > max_fd) max_fd = tcp_cfgFd;

        FD_ZERO(&readfds);
        FD_SET(unix_fd, &readfds);
            FD_SET(tcp_cliFd, &readfds);
            FD_SET(tcp_cfgFd, &readfds);

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(unix_fd, &readfds)) {
            client_fd = unix_socket.accept();
            if (client_fd < 0) {
                if (errno == EINTR) {
                    continue;
                }
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

        if (FD_ISSET(tcp_cliFd, &readfds)) {
            client_fd = cliPassSocket.accept();
            if (client_fd < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }

            while ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[received] = '\0';
                printf("[cli] Received: %s\n", buffer);
                fflush(stdout);
            }

            if (received < 0 && errno != EINTR) {
                perror("recv (tcp)");
            }

            close(client_fd);
            client_fd = -1;
        }
            if (FD_ISSET(tcp_cfgFd, &readfds)) {
                client_fd = configSocket.accept();
                if (client_fd < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                while ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
                    buffer[received] = '\0';
                    printf("[cfg] Received: %s\n", buffer);
                    fflush(stdout);
                }

                if (received < 0 && errno != EINTR) {
                    perror("recv (config)");
                }

                close(client_fd);
                client_fd = -1;
            }
    }
    unix_socket.close();
    unix_socket.cleanupUnixSocket();
    cliPassSocket.close();
    configSocket.close();

    return 0;
}
