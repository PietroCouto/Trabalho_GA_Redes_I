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
    Server(std::string name, sockaddr_in address, std::vector<sockaddr_in> peer_addresses);

    /**
     * Destructor
     */
    ~Server();

    /**
     * Methods
     */
    int start();
    int listen();
    int broadcast(std::string message);
    bool has_message();
    std::string get_message();

private:
    /**
     * Attributes
     */
    sockaddr_in address;
    std::string name;
    int socket_fd;
    std::vector<sockaddr_in> peer_addresses;

    // TODO converter a queue em uma estrutura que armazene o endereço do remetente
    std::queue<std::string> message_queue;
};

/**
 * Constructor
 * @param name The server's name
 * @param address The server's adress
 */
Server::Server(std::string name, sockaddr_in address, std::vector<sockaddr_in> peer_addresses) {
    this->name = name;
    this->address = address;
    this->message_queue = std::queue<std::string>();
    this->peer_addresses = peer_addresses;
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

/**
 * Listen for messages on the server socket
 * @returns -1 for error, 0 for success
 */
int Server::listen()
{
    // Variables to store the sender address
    sockaddr_in sender_address;
    socklen_t sender_address_size;

    // Variables to store the message
    char message[1472];
    int bytes_received = 0;

    // Keep the function alive
    while (true)
    {
        // Clean the memory structure for machine_address
        memset(&sender_address, 0, sizeof(sockaddr_in));

        // Clean message memory
        memset(&message, 0, sizeof(message));

        // Get the size of the address structure
        sender_address_size = sizeof(sender_address);

        // Receive messages from the socket
        bytes_received = recvfrom(this->socket_fd, message, sizeof(message) - 1, 0, (struct sockaddr *)&sender_address, &sender_address_size);

        // Check for errors
        if (bytes_received == -1)
            continue;

        // Valid messages need to have the string terminator set
        message[bytes_received] = '\0';

        std::cout << std::string(message) << std::endl;

        // Push the message into the queue
        this->message_queue.push(std::string(message));
    }

    return 0;
}

/**
 * Check if the server has queued messages
 * @returns Boolean
 */
bool Server::has_message()
{
    return this->message_queue.size() != 0;
}

/**
 * Get the next message on the queue
 * @returns The next message on the queue
 */
std::string Server::get_message()
{
    return message_queue.front();
}

/**
 * Broadcast a message to every peer on the network
 */
int Server::broadcast(std::string message)
{
    for (int i = 0; i < this->peer_addresses.size(); i++) {
        sendto(this->socket_fd, message.c_str(), message.length(), 0, (struct sockaddr *)&this->peer_addresses.at(i), sizeof(this->peer_addresses.at(i)));
    }

    return 0;
}

#endif