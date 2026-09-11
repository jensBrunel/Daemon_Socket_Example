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
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

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
        std::cerr << "Socket type mismatch: expected UNIX_DOMAIN\n";
        return -1;
    }
    m_sUnixSocketPath = socket_path;

    m_iFileDescriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_iFileDescriptor < 0) {
        std::cerr << "unix socket creation failed: " << std::strerror(errno) << '\n';
        return -1;
    }

    return 0;
}

int Socket::initTcpSocket(const std::string& ip, int port)
{
    if (m_eSocketType != SocketType::TCP) {
        std::cerr << "Socket type mismatch: expected TCP\n";
        return -1;
    }
    m_iTcpPort = port;
    m_sTcpIp = ip;

    m_iFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (m_iFileDescriptor < 0) {
        std::cerr << "tcp socket creation failed: " << std::strerror(errno) << '\n';
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
        std::cerr << "Socket not initialized\n";
        return -1;
    }

    if (m_eSocketType == SocketType::UNIX_DOMAIN) {
        sockaddr_un address{};
        address.sun_family = AF_UNIX;

        if (m_sUnixSocketPath.size() >= sizeof(address.sun_path)) {
            std::cerr << "Unix socket path too long: " << m_sUnixSocketPath << '\n';
            return -1;
        }
        std::copy(m_sUnixSocketPath.begin(), m_sUnixSocketPath.end(), address.sun_path);
        address.sun_path[m_sUnixSocketPath.size()] = '\0';

        if (::bind(m_iFileDescriptor, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
            std::cerr << "unix bind failed: " << std::strerror(errno) << '\n';
            return -1;
        }
    } else if (m_eSocketType == SocketType::TCP) {
        sockaddr_in tcp_addr{};
        tcp_addr.sin_family = AF_INET;
        tcp_addr.sin_port = htons(m_iTcpPort);
        // Convert textual IP to binary
        if (m_sTcpIp.empty()) {
            tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            if (inet_pton(AF_INET, m_sTcpIp.c_str(), &tcp_addr.sin_addr) != 1) {
                std::cerr << "invalid IP address: " << m_sTcpIp << '\n';
                return -1;
            }
        }
        if (::bind(m_iFileDescriptor, reinterpret_cast<struct sockaddr *>(&tcp_addr), sizeof(tcp_addr)) < 0) {
            std::cerr << "tcp bind failed: " << std::strerror(errno) << '\n';
            return -1;
        }
    }

    return 0;
}

int Socket::listen(int backlog)
{
    if (m_iFileDescriptor < 0) {
        std::cerr << "Socket not initialized\n";
        return -1;
    }
    if (::listen(m_iFileDescriptor, backlog) < 0) {
        std::cerr << "listen failed: " << std::strerror(errno) << '\n';
        return -1;
    }

    return 0;
}

int Socket::accept()
{
    if (m_iFileDescriptor < 0) {
        std::cerr << "Socket not initialized\n";
        return -1;
    }

    int client_fd = ::accept(m_iFileDescriptor, nullptr, nullptr);
    if (client_fd < 0) {
        if (errno != EINTR) {
            std::cerr << "accept failed: " << std::strerror(errno) << '\n';
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
            std::cerr << "close failed: " << std::strerror(errno) << '\n';
            return -1;
        }
        m_iFileDescriptor = -1;
    }

    return 0;
}

int Socket::cleanupUnixSocket()
{
    if (m_eSocketType != SocketType::UNIX_DOMAIN) {
        std::cerr << "cleanupUnixSocket: not a Unix domain socket\n";
        return -1;
    }
    if (unlink(m_sUnixSocketPath.c_str()) < 0) {
        std::cerr << "unlink failed: " << std::strerror(errno) << '\n';
        return -1;
    }

    return 0;
}

int Socket::setTcpSocketOptions()
{
    int opt = 1;
    if (setsockopt(m_iFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed: " << std::strerror(errno) << '\n';
        return -1;
    }

    return 0;
}
