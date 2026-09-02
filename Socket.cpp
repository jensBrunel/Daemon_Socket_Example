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
    : m_socket_type_(type), m_file_descriptor_(-1), m_tcp_port_(0)
{
}

Socket::~Socket()
{
    if (m_file_descriptor_ >= 0) {
        close();
    }
}

int Socket::initUnixSocket(const std::string& socket_path)
{
    if (m_socket_type_ != SocketType::UNIX_DOMAIN) {
        perror("Socket type mismatch: expected UNIX_DOMAIN");
        return -1;
    }
    m_unix_socket_path_ = socket_path;

    m_file_descriptor_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_file_descriptor_ < 0) {
        perror("unix socket creation failed");
        return -1;
    }

    return 0;
}

int Socket::initTcpSocket(const std::string& ip, int port)
{
    if (m_socket_type_ != SocketType::TCP) {
        perror("Socket type mismatch: expected TCP");
        return -1;
    }
    m_tcp_port_ = port;
    m_tcp_ip_ = ip;

    m_file_descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
    if (m_file_descriptor_ < 0) {
        perror("tcp socket creation failed");
        return -1;
    }

    if (setTcpSocketOptions() < 0) {
        ::close(m_file_descriptor_);
        m_file_descriptor_ = -1;
        return -1;
    }

    return 0;
}

int Socket::bind()
{
    if (m_file_descriptor_ < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    if (m_socket_type_ == SocketType::UNIX_DOMAIN) {
        struct sockaddr_un address;
        memset(&address, 0, sizeof(address));
        address.sun_family = AF_UNIX;
        snprintf(address.sun_path, sizeof(address.sun_path), "%s", m_unix_socket_path_.c_str());

        if (::bind(m_file_descriptor_, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("unix bind failed");
            return -1;
        }
    } else if (m_socket_type_ == SocketType::TCP) {
        struct sockaddr_in tcp_addr;
        memset(&tcp_addr, 0, sizeof(tcp_addr));
        tcp_addr.sin_family = AF_INET;
        tcp_addr.sin_port = htons(m_tcp_port_);
        // Convert textual IP to binary
        if (m_tcp_ip_.empty()) {
            tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            if (inet_pton(AF_INET, m_tcp_ip_.c_str(), &tcp_addr.sin_addr) != 1) {
                fprintf(stderr, "invalid IP address: %s\n", m_tcp_ip_.c_str());
                return -1;
            }
        }
        if (::bind(m_file_descriptor_, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
            perror("tcp bind failed");
            return -1;
        }
    }

    return 0;
}

int Socket::listen(int backlog)
{
    if (m_file_descriptor_ < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }
    if (::listen(m_file_descriptor_, backlog) < 0) {
        perror("listen failed");
        return -1;
    }

    return 0;
}

int Socket::accept()
{
    if (m_file_descriptor_ < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    int client_fd = ::accept(m_file_descriptor_, NULL, NULL);
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
    return m_file_descriptor_;
}

SocketType Socket::getSocketType() const
{
    return m_socket_type_;
}

int Socket::close()
{
    if (m_file_descriptor_ >= 0) {
        if (::close(m_file_descriptor_) < 0) {
            perror("close failed");
            return -1;
        }
        m_file_descriptor_ = -1;
    }

    return 0;
}

int Socket::cleanupUnixSocket()
{
    if (m_socket_type_ != SocketType::UNIX_DOMAIN) {
        fprintf(stderr, "cleanupUnixSocket: not a Unix domain socket\n");
        return -1;
    }
    if (unlink(m_unix_socket_path_.c_str()) < 0) {
        perror("unlink failed");
        return -1;
    }

    return 0;
}

int Socket::setTcpSocketOptions()
{
    int opt = 1;
    if (setsockopt(m_file_descriptor_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return -1;
    }

    return 0;
}
