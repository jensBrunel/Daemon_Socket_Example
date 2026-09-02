/**
 * @file Socket.cpp
 * @brief Implementation of the generic Socket class.
 *
 * This file contains the implementation of socket creation, binding,
 * and listening operations for both Unix domain and TCP sockets.
 *
 * @version 1.0
 * @date 2026-08-31
 */

#include "Socket.h"
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>

Socket::Socket(SocketType type)
    : m_eSocketType(type), m_iFileDescriptor(-1), m_iTcpPort(0)
{
}

Socket::~Socket()
{
    if (m_iFileDescriptor >= 0) {
        close();
    }
}

int Socket::initUnixSocket(const std::string& socket_path)
{
    if (m_eSocketType != SocketType::UNIX_DOMAIN) {
        perror("Socket type mismatch: expected UNIX_DOMAIN");
        return -1;
    }
    m_sUnixSocketPath = socket_path;

    m_iFileDescriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_iFileDescriptor < 0) {
        perror("unix socket creation failed");
        return -1;
    }

    return 0;
}

int Socket::initTcpSocket(const std::string& ip, int port)
{
    if (m_eSocketType != SocketType::TCP) {
        perror("Socket type mismatch: expected TCP");
        return -1;
    }
    m_iTcpPort = port;
    m_sTcpIp = ip;

    m_iFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (m_iFileDescriptor < 0) {
        perror("tcp socket creation failed");
        return -1;
    }

    if (setTcpSocketOptions() < 0) {
        ::close(m_iFileDescriptor);
        m_iFileDescriptor = -1;
        return -1;
    }

    return 0;
}

int Socket::bind()
{
    if (m_iFileDescriptor < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    if (m_eSocketType == SocketType::UNIX_DOMAIN) {
        struct sockaddr_un address;
        memset(&address, 0, sizeof(address));
        address.sun_family = AF_UNIX;
        snprintf(address.sun_path, sizeof(address.sun_path), "%s", m_sUnixSocketPath.c_str());

        if (::bind(m_iFileDescriptor, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("unix bind failed");
            return -1;
        }
    } else if (m_eSocketType == SocketType::TCP) {
        struct sockaddr_in tcp_addr;
        memset(&tcp_addr, 0, sizeof(tcp_addr));
        tcp_addr.sin_family = AF_INET;
        tcp_addr.sin_port = htons(m_iTcpPort);
        // Convert textual IP to binary
        if (m_sTcpIp.empty()) {
            tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            if (inet_pton(AF_INET, m_sTcpIp.c_str(), &tcp_addr.sin_addr) != 1) {
                fprintf(stderr, "invalid IP address: %s\n", m_sTcpIp.c_str());
                return -1;
            }
        }
        if (::bind(m_iFileDescriptor, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
            perror("tcp bind failed");
            return -1;
        }
    }

    return 0;
}

int Socket::listen(int backlog)
{
    if (m_iFileDescriptor < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }
    if (::listen(m_iFileDescriptor, backlog) < 0) {
        perror("listen failed");
        return -1;
    }

    return 0;
}

int Socket::accept()
{
    if (m_iFileDescriptor < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    int client_fd = ::accept(m_iFileDescriptor, NULL, NULL);
    if (client_fd < 0) {
        if (errno != EINTR) {
            perror("accept failed");
        }
        return -1;
    }

    return client_fd;
}

int Socket::getFileDescriptor() const
{
    return m_iFileDescriptor;
}

SocketType Socket::getSocketType() const
{
    return m_eSocketType;
}

int Socket::close()
{
    if (m_iFileDescriptor >= 0) {
        if (::close(m_iFileDescriptor) < 0) {
            perror("close failed");
            return -1;
        }
        m_iFileDescriptor = -1;
    }

    return 0;
}

int Socket::cleanupUnixSocket()
{
    if (m_eSocketType != SocketType::UNIX_DOMAIN) {
        fprintf(stderr, "cleanupUnixSocket: not a Unix domain socket\n");
        return -1;
    }
    if (unlink(m_sUnixSocketPath.c_str()) < 0) {
        perror("unlink failed");
        return -1;
    }

    return 0;
}

int Socket::setTcpSocketOptions()
{
    int opt = 1;
    if (setsockopt(m_iFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return -1;
    }

    return 0;
}
