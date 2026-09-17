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
#include "ConfigParser.h"
#include "ConfigSerializer.h"
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

#define BUFFER_SIZE 256
#define TCP_PORT 12345

namespace
{

    void log_errno(const std::string &context)
    {
        std::cerr << context << ": " << std::strerror(errno) << '\n';
    }

    void handle_signal(int signal_number)
    {
        (void)signal_number;
    }

    bool is_client_disconnect(int error_code)
    {
        switch (error_code)
        {
        case ECONNRESET:
        case ENOTCONN:
        case EPIPE:
        case ETIMEDOUT:
        case ECONNABORTED:
        case EPROTO:
            return true;
        default:
            return false;
        }
    }

    int daemonize()
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            log_errno("fork");
            return -1;
        }

        if (pid > 0)
        {
            std::exit(0);
        }

        if (setsid() < 0)
        {
            log_errno("setsid");
            return -1;
        }

        std::signal(SIGCHLD, SIG_IGN);
        std::signal(SIGHUP, SIG_IGN);
        std::signal(SIGTERM, handle_signal);

        pid = fork();
        if (pid < 0)
        {
            log_errno("second fork");
            return -1;
        }

        if (pid > 0)
        {
            std::exit(0);
        }

        umask(0);

        if (chdir("/") < 0)
        {
            log_errno("chdir");
            return -1;
        }

        if (close(STDIN_FILENO) < 0)
        {
            log_errno("close STDIN_FILENO");
        }
        if (close(STDOUT_FILENO) < 0)
        {
            log_errno("close STDOUT_FILENO");
        }
        if (close(STDERR_FILENO) < 0)
        {
            log_errno("close STDERR_FILENO");
        }

        return 0;
    }

} // namespace

int main(int argc, char **argv)
{
    int client_fd = -1;
    char buffer[BUFFER_SIZE];
    ssize_t received = 0;

    // Load socket path from configuration (if available)
    std::string ini_path;
    if (argc > 1)
        ini_path = argv[1];
    IniConfig cfg(ini_path);
    std::string socket_path = cfg.get("SOCKET_PATH");
    if (socket_path.empty())
        socket_path = "/tmp/daemon_socket_example.sock";

    // if (daemonize() != 0) {
    //     return 1;
    // }

    std::signal(SIGPIPE, SIG_IGN);

    // Create Unix domain socket
    Socket unix_socket(SocketType::UNIX_DOMAIN);
    if (unix_socket.initUnixSocket(socket_path) < 0)
    {
        return 1;
    }

    // Remove old socket file if it exists
    unlink(socket_path.c_str());

    // Bind and listen on Unix socket
    if (unix_socket.bind() < 0)
    {
        return 1;
    }

    if (unix_socket.listen(5) < 0)
    {
        return 1;
    }

    // Create TCP socket (CLI pass socket) using config values
    Socket cliPassSocket(SocketType::TCP);
    std::string tcp_ip = cfg.get("TCP_IP");
    std::string port_str = cfg.get("CLI_PASSTHROUGH_PORT");
    int cli_port = 12345;
    if (tcp_ip.empty())
        tcp_ip = "127.0.0.1";
    if (!port_str.empty())
    {
        try
        {
            cli_port = std::stoi(port_str);
        }
        catch (...)
        {
            std::cerr << "Invalid CLI_PASSTHROUGH_PORT '" << port_str
                      << "', using 12345\n";
        }
    }

    if (cliPassSocket.initTcpSocket(tcp_ip, cli_port) < 0)
    {
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    // Bind and listen on TCP socket
    if (cliPassSocket.bind() < 0)
    {
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    if (cliPassSocket.listen(5) < 0)
    {
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    // Create second TCP socket for configuration/control messages
    Socket configSocket(SocketType::TCP);
    std::string config_port_str = cfg.get("CONFIG_PORT");
    int config_port = 22345;
    if (!config_port_str.empty())
    {
        try
        {
            config_port = std::stoi(config_port_str);
        }
        catch (...)
        {
            std::cerr << "Invalid CONFIG_PORT '" << config_port_str
                      << "', using 22345\n";
        }
    }

    if (configSocket.initTcpSocket(tcp_ip, config_port) < 0)
    {
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        cliPassSocket.close();
        return 1;
    }

    if (configSocket.bind() < 0)
    {
        configSocket.close();
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    if (configSocket.listen(5) < 0)
    {
        configSocket.close();
        cliPassSocket.close();
        unix_socket.close();
        unix_socket.cleanupUnixSocket();
        return 1;
    }

    int unix_fd = unix_socket.getFileDescriptor();
    int tcp_cliFd = cliPassSocket.getFileDescriptor();
    int tcp_cfgFd = configSocket.getFileDescriptor();

    while (1)
    {
        fd_set readfds;
        int max_fd = unix_fd;
        if (tcp_cliFd > max_fd)
            max_fd = tcp_cliFd;
        if (tcp_cfgFd > max_fd)
            max_fd = tcp_cfgFd;

        FD_ZERO(&readfds);
        FD_SET(unix_fd, &readfds);
        FD_SET(tcp_cliFd, &readfds);
        FD_SET(tcp_cfgFd, &readfds);

        if (select(max_fd + 1, &readfds, nullptr, nullptr, nullptr) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            log_errno("select");
            break;
        }

        if (FD_ISSET(unix_fd, &readfds))
        {
            client_fd = unix_socket.accept();
            if (client_fd < 0)
            {
                if (errno == EINTR || errno == ECONNABORTED || errno == EPROTO)
                {
                    continue;
                }
                log_errno("accept (unix)");
                break;
            }

            std::cout << "Unix socket accepted connection, fd: " << client_fd << '\n';
            if ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
            {
                buffer[received] = '\0';
                char sendBuffer[BUFFER_SIZE];
                std::string message(buffer);
                std::cout << "Unix Received: " << message << '\n';
                std::istringstream streamMsg(message);
                std::string token;
                std::vector<std::string> vecTokens;
                while (streamMsg >> token)
                {
                    vecTokens.push_back(token);
                }

                std::cout << "Unix Received tokens: " << vecTokens.front() << std::endl;
                if (!vecTokens.empty() && vecTokens.front() == "switch_status")
                {
                    std::cout << "Received 'status' command on unix socket\n";
                    std::string statusConfig = R"(
/-----------------------
/- Status overview
/-----------------------
Port1: Active
Port2: Inactive
Port3: Active
/-----------------------
)";
                    std::memset(sendBuffer, 0, sizeof(sendBuffer));
                    std::memcpy(sendBuffer, statusConfig.c_str(), statusConfig.size());
                    std::cout << "Unix Received: " << message << '\n';
                    send(client_fd, sendBuffer, statusConfig.size(), 0);
                }
                else if (!vecTokens.empty() && vecTokens.front() == "set")
                {
                    std::cout << "Received 'set' command on unix socket\n";
                    ConfigParser configParser(vecTokens[1]);
                    ConfigSerializer configSerializer(configParser);
                    std::string setConfig = "set " + vecTokens[1];
                    std::memset(sendBuffer, 0, sizeof(sendBuffer));
                    std::memcpy(sendBuffer, setConfig.c_str(), setConfig.size());
                    send(client_fd, sendBuffer, setConfig.size(), 0);
                }
                else if (!vecTokens.empty() && vecTokens.front() == "copy")
                {
                    std::cout << "Received 'copy' command on unix socket\n";
                    std::string copyConfig = "copy " + vecTokens[1];
                    std::memset(sendBuffer, 0, sizeof(sendBuffer));
                    std::memcpy(sendBuffer, copyConfig.c_str(), copyConfig.size());
                    send(client_fd, sendBuffer, copyConfig.size(), 0);
                }
            }

            if (received == 0)
            {
                std::cerr << "peer disconnected on unix socket\n";
            }
            else if (received < 0)
            {
                if (errno == EINTR)
                {
                    std::cerr << "recv interrupted by signal, continuing\n";
                    continue;
                }
                if (is_client_disconnect(errno))
                {
                    std::cerr << "client disconnected on unix socket\n";
                }
                else
                {
                    log_errno("recv (unix)");
                    break;
                }
            }

            close(client_fd);
            client_fd = -1;
        }

        if (FD_ISSET(tcp_cliFd, &readfds))
        {
            client_fd = cliPassSocket.accept();
            if (client_fd < 0)
            {
                if (errno == EINTR || errno == ECONNABORTED || errno == EPROTO)
                {
                    continue;
                }
                log_errno("accept (tcp cli)");
                break;
            }

            if ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
            {
                buffer[received] = '\0';
                std::string message(buffer);
                // std::cout << "[cli] Received: " << message << '\n';
            }

            if (received == 0)
            {
                std::cerr << "peer disconnected on cli socket\n";
            }
            else if (received < 0)
            {
                if (errno == EINTR)
                {
                    std::cerr << "recv interrupted by signal, continuing\n";
                    continue;
                }
                if (is_client_disconnect(errno))
                {
                    std::cerr << "client disconnected on cli socket\n";
                }
                else
                {
                    log_errno("recv (tcp)");
                    break;
                }
            }
            std::cout << "Closing client_fd: " << client_fd << '\n';
            close(client_fd);
            client_fd = -1;
        }

        if (FD_ISSET(tcp_cfgFd, &readfds))
        {
            client_fd = configSocket.accept();
            if (client_fd < 0)
            {
                if (errno == EINTR || errno == ECONNABORTED || errno == EPROTO)
                {
                    continue;
                }
                log_errno("accept (tcp cfg)");
                break;
            }

            if ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
            {
                buffer[received] = '\0';
                std::string message(buffer);
                std::cout << "[cfg] Received: " << message << '\n';
            }

            if (received == 0)
            {
                std::cerr << "peer disconnected on config socket\n";
            }
            else if (received < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                if (is_client_disconnect(errno))
                {
                    std::cerr << "client disconnected on config socket\n";
                }
                else
                {
                    log_errno("recv (config)");
                    break;
                }
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
