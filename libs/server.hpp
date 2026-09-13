/**
 * @author Pietro do Couto Freitas
 */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <queue>

/**
 * Handmade libs
 */
#include "./utils.hpp"

class Server
{
public:
    /**
     * Constructors
     */
    Server(std::string name, sockaddr_in address);

    /**
     * Destructor
     */
    ~Server();

    /**
     * Methods
     */
    int start();
    int listen();

private:
    /**
     * Attributes
     */
    sockaddr_in address;
    std::string name;
    int socket_fd;
    std::queue<std::string> message_queue;
};

/**
 * Constructor
 * @param name The server's name
 * @param address The server's adress
 */
Server::Server(std::string name, sockaddr_in address)
{
    this->name = name;
    this->address = address;
    this->message_queue = std::queue<std::string>();
};

/**
 * Destructor
 */
Server::~Server()
{
    // Validate that the socket is open
    if (this->socket_fd != -1)
    {
        // Close the socket
        close(this->socket_fd);

        // Invalidate the use
        this->socket_fd = -1;
    }
};

/**
 * Starts the server
 * @returns -1 for error, 0 for success
 */
int Server::start()
{
    // Try to start a UDP socket
    this->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // Check if the socket has been created
    if (this->socket_fd == -1)
        return error_message("Falha ao iniciar o socket!");

    // Try to bind the socket
    if (bind(this->socket_fd, (struct sockaddr *)&this->address, sizeof(this->address)) == -1)
        return error_message("Erro ao realizar o bind!");

    return 0;
}

int Server::listen() {
    char *message[1472];

    while (true) {
        if (recvfrom(this->socket_fd, message, sizeof(message), 0) == -1)
            return -1;
    }
}

#endif