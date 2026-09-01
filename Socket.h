/**
 * @file Socket.hpp
 * @brief Generic socket class for Unix domain and TCP sockets.
 *
 * This class provides a wrapper for creating and managing both Unix domain
 * sockets and TCP sockets with a unified interface.
 *
 * @version 1.0
 * @date 2026-08-31
 */

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>

/**
 * @enum SocketType
 * @brief Enumeration for socket types.
 */
enum class SocketType {
    UNIX_DOMAIN,  /**< Unix domain socket (AF_UNIX) */
    TCP           /**< TCP socket (AF_INET) */
};

/**
 * @class Socket
 * @brief Generic socket class for Unix domain and TCP sockets.
 *
 * This class encapsulates socket creation, binding, and listening
 * operations for both Unix domain and TCP sockets.
 */
class Socket {
public:
    /**
     * @brief Constructor for Socket.
     *
     * @param type The type of socket (UNIX_DOMAIN or TCP).
     */
    explicit Socket(SocketType type);

    /**
     * @brief Destructor for Socket.
     *
     * Closes the socket file descriptor if it's open.
     */
    ~Socket();

    /**
     * @brief Initialize a Unix domain socket.
     *
     * @param socket_path The path for the Unix domain socket.
     * @return 0 on success, -1 on failure.
     */
    int initUnixSocket(const std::string& socket_path);

    /**
     * @brief Initialize a TCP socket.
        *
        * @param ip The IP address to bind to (e.g. "127.0.0.1" or "0.0.0.0").
        * @param port The port number for the TCP socket.
        * @return 0 on success, -1 on failure.
     */
        int initTcpSocket(const std::string& ip, int port);

    /**
     * @brief Bind the socket to an address.
     *
     * @return 0 on success, -1 on failure.
     */
    int bind();

    /**
     * @brief Listen for incoming connections.
     *
     * @param backlog The maximum number of pending connections.
     * @return 0 on success, -1 on failure.
     */
    int listen(int backlog = 5);

    /**
     * @brief Accept an incoming connection.
     *
     * @return The file descriptor of the accepted connection, or -1 on failure.
     */
    int accept();

    /**
     * @brief Get the socket file descriptor.
     *
     * @return The file descriptor of the socket.
     */
    int getFileDescriptor() const;

    /**
     * @brief Get the socket type.
     *
     * @return The type of the socket.
     */
    SocketType getSocketType() const;

    /**
     * @brief Close the socket.
     *
     * @return 0 on success, -1 on failure.
     */
    int close();

    /**
     * @brief Clean up Unix domain socket file.
     *
     * Removes the socket file from the filesystem.
     *
     * @return 0 on success, -1 on failure.
     */
    int cleanupUnixSocket();

private:
    SocketType socket_type_;      /**< The type of socket */
    int file_descriptor_;         /**< The file descriptor for the socket */
    std::string unix_socket_path_; /**< Path for Unix domain socket */
    int tcp_port_;                /**< Port number for TCP socket */
    std::string tcp_ip_;          /**< IP address for TCP socket */

    /**
     * @brief Set socket options for TCP socket.
     *
     * @return 0 on success, -1 on failure.
     */
    int setTcpSocketOptions();
};

#endif // SOCKET_HPP
