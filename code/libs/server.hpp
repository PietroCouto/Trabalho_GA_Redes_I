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
#include <unistd.h>

/**
 * Handmade libs
 */
#include "./utils.hpp"

struct package
{
    std::string message;
    sockaddr_in sender_address;
};

/**
 * Class for handling the conections
 */
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
    int send_message(std::string message, sockaddr_in destination_address);
    bool has_message();
    package get_message();
    int get_listening_error_count();
    int get_broadcasting_error_count();
    void show_errors();

private:
    /**
     * Attributes
     */
    sockaddr_in address;
    std::string name;
    int socket_fd;
    std::vector<sockaddr_in> peer_addresses;
    int listening_error_count;
    int broadcasting_error_count;
    std::mutex mtx;

    // TODO converter a queue em uma estrutura que armazene o endereço do remetente
    std::queue<package> message_queue;

    /**
     * Methods
     */
    bool is_new_address(sockaddr_in new_address);
};

/**
 * Constructor
 * @param name The server's name
 * @param address The server's adress
 */
Server::Server(std::string name, sockaddr_in address, std::vector<sockaddr_in> peer_addresses)
{
    this->name = name;
    this->address = address;
    this->message_queue = std::queue<package>();
    this->peer_addresses = peer_addresses;
    this->socket_fd = -1;
    this->listening_error_count = 0;
    this->broadcasting_error_count = 0;
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
 * Methods
 */

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
        {
            this->listening_error_count++;
            continue;
        }

        // Valid messages need to have the string terminator set
        message[bytes_received] = '\0';

        // Prevent racing condition on the queue
        this->mtx.lock();

        // Push the message into the queue
        this->message_queue.push(package{std::string(message), sender_address});

        // Unlock the mutex
        this->mtx.unlock();

        // Check if the sender address is a new one
        if (is_new_address(sender_address))
        {
            // Add the new address to peers
            this->peer_addresses.push_back(sender_address);

            std::cout << "Novo Peer conectado!" << std::endl;
        }
    }

    return 0;
}

/**
 * Check if the server has queued messages
 * @returns Boolean
 */
bool Server::has_message()
{
    // Prevent racing condition on the queue
    this->mtx.lock();

    bool is_empty = this->message_queue.size() == 0;

    // Unlock the mutex
    this->mtx.unlock();

    return !is_empty;
}

/**
 * Get the next message on the queue
 * @returns The next message on the queue
 */
package Server::get_message()
{
    // Check if there is a message on the queue
    if (this->has_message())
    {
        // Prevent racing condition on the queue
        this->mtx.lock();

        // Extract the message
        package message = message_queue.front();

        // Remove the message from the queue
        message_queue.pop();

        // Unlock the mutex
        this->mtx.unlock();

        return message;
    }

    return package{};
}

/**
 * Broadcast a message to every peer on the network
 * @param message The message to be sent
 */
int Server::broadcast(std::string message)
{
    // Iterate every peer
    for (int i = 0; i < this->peer_addresses.size(); i++)
    {
        // Send the message to the current peer
        if (sendto(this->socket_fd, message.c_str(), message.length(), 0, (struct sockaddr *)&this->peer_addresses.at(i), sizeof(this->peer_addresses.at(i))) == -1)
            this->broadcasting_error_count++;
    }

    return 0;
}

/**
 * Send a message to a specific destination
 * @param message The message to be sent
 * @param destination_address The address of the destination
 * @returns -1 for error, 0 for success
 */
int Server::send_message(std::string message, sockaddr_in destination_address)
{
    // Try to send the message to the destination
    if (sendto(this->socket_fd, message.c_str(), message.length(), 0, (struct sockaddr *)&destination_address, sizeof(destination_address)) == -1)
        return -1;

    return 0;
};

/**
 * Check if an address already exists on the peers
 * @param new_address The address to be checked
 * @returns Boolean indicating if it is new
 */
bool Server::is_new_address(sockaddr_in new_address)
{
    // Iterate through every peer address
    for (int i = 0; i < this->peer_addresses.size(); i++)
    {
        // Get the current address
        sockaddr_in current = this->peer_addresses.at(i);

        // Check if the port and address are already present
        if (current.sin_addr.s_addr == new_address.sin_addr.s_addr &&
            current.sin_port == new_address.sin_port)
            return false;
    }

    return true;
};

/**
 * Get the total listening error amount
 * @returns Integer for total errors
 */
int Server::get_listening_error_count()
{
    return this->listening_error_count;
};

/**
 * Get the total broadcasting error amount
 * @returns Integer for total errors
 */
int Server::get_broadcasting_error_count()
{
    return this->broadcasting_error_count;
};

void Server::show_errors()
{
    // Show the total listening errors
    std::cout << "Total de erros de escuta: ";
    std::cout << this->listening_error_count << std::endl;

    // Show the total broadcasting errors
    std::cout << "Total de erros de broadcast: ";
    std::cout << this->broadcasting_error_count << std::endl;
}

#endif