/**
 * @author Pietro do Couto Freitas
 */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
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
     * Methods
     */
    int start();

private:
    /**
     * Attributes
     */
    sockaddr_in address;
    std::string name;
    int socket_fd;
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

#endif