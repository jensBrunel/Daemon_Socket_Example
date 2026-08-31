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

#include "Socket.hpp"
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>

Socket::Socket(SocketType type)
    : socket_type_(type), file_descriptor_(-1), tcp_port_(0)
{
}

Socket::~Socket()
{
    if (file_descriptor_ >= 0) {
        close();
    }
}

int Socket::initUnixSocket(const std::string& socket_path)
{
    if (socket_type_ != SocketType::UNIX_DOMAIN) {
        perror("Socket type mismatch: expected UNIX_DOMAIN");
        return -1;
    }

    unix_socket_path_ = socket_path;

    file_descriptor_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (file_descriptor_ < 0) {
        perror("unix socket creation failed");
        return -1;
    }

    return 0;
}

int Socket::initTcpSocket(int port)
{
    if (socket_type_ != SocketType::TCP) {
        perror("Socket type mismatch: expected TCP");
        return -1;
    }

    tcp_port_ = port;

    file_descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
    if (file_descriptor_ < 0) {
        perror("tcp socket creation failed");
        return -1;
    }

    if (setTcpSocketOptions() < 0) {
        ::close(file_descriptor_);
        file_descriptor_ = -1;
        return -1;
    }

    return 0;
}

int Socket::bind()
{
    if (file_descriptor_ < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    if (socket_type_ == SocketType::UNIX_DOMAIN) {
        struct sockaddr_un address;
        memset(&address, 0, sizeof(address));
        address.sun_family = AF_UNIX;
        snprintf(address.sun_path, sizeof(address.sun_path), "%s", unix_socket_path_.c_str());

        if (::bind(file_descriptor_, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("unix bind failed");
            return -1;
        }
    } else if (socket_type_ == SocketType::TCP) {
        struct sockaddr_in tcp_addr;
        memset(&tcp_addr, 0, sizeof(tcp_addr));
        tcp_addr.sin_family = AF_INET;
        tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        tcp_addr.sin_port = htons(tcp_port_);

        if (::bind(file_descriptor_, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
            perror("tcp bind failed");
            return -1;
        }
    }

    return 0;
}

int Socket::listen(int backlog)
{
    if (file_descriptor_ < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    if (::listen(file_descriptor_, backlog) < 0) {
        perror("listen failed");
        return -1;
    }

    return 0;
}

int Socket::accept()
{
    if (file_descriptor_ < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    int client_fd = ::accept(file_descriptor_, NULL, NULL);
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
    return file_descriptor_;
}

SocketType Socket::getSocketType() const
{
    return socket_type_;
}

int Socket::close()
{
    if (file_descriptor_ >= 0) {
        if (::close(file_descriptor_) < 0) {
            perror("close failed");
            return -1;
        }
        file_descriptor_ = -1;
    }

    return 0;
}

int Socket::cleanupUnixSocket()
{
    if (socket_type_ != SocketType::UNIX_DOMAIN) {
        fprintf(stderr, "cleanupUnixSocket: not a Unix domain socket\n");
        return -1;
    }

    if (unlink(unix_socket_path_.c_str()) < 0) {
        perror("unlink failed");
        return -1;
    }

    return 0;
}

int Socket::setTcpSocketOptions()
{
    int opt = 1;
    if (setsockopt(file_descriptor_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return -1;
    }

    return 0;
}
