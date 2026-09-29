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
#include <chrono>
#include <condition_variable>

/**
 * Handmade libs
 */
#include "./utils.hpp"

#define MAX_ATTEMPTS 5

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
    std::mutex reply_mtx;
    std::condition_variable reply_cv;
    package reply;

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
    this->reply = package();
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
        bytes_received = recvfrom(this->socket_fd, message, sizeof(message), 0, (struct sockaddr *)&sender_address, &sender_address_size);

        // Check for errors
        if (bytes_received == -1)
        {
            this->listening_error_count++;
            continue;
        }

        std::string message_string(message, bytes_received);

        // Check if is just a confirmation message
        if (bytes_received == 2 && message_string == "OK")
        {
            // Lock the reply mutex
            std::lock_guard<std::mutex> lock(this->reply_mtx);

            // Save the message and the sender
            this->reply = {message_string, sender_address};

            // Notify the thread waiting for the response
            this->reply_cv.notify_all();

            // Skip the queueing
            continue;
        }

        // Inform that the message was received
        sendto(this->socket_fd, "OK", 2, 0, (struct sockaddr *)&sender_address, sizeof(sender_address));

        // Prevent racing condition on the queue
        std::lock_guard<std::mutex> lock(this->mtx);

        // Push the message into the queue
        this->message_queue.push(package{message_string, sender_address});

        // Check if the sender address is a new peer
        if (is_new_address(sender_address))
            this->peer_addresses.push_back(sender_address);
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
    std::lock_guard<std::mutex> lock(this->mtx);

    bool is_empty = this->message_queue.size() == 0;

    return !is_empty;
}

/**
 * Get the next message on the queue
 * @returns The next message on the queue
 */
package Server::get_message()
{
    // Prevent racing condition on the queue
    std::lock_guard<std::mutex> lock(this->mtx);

    // Check if there is a message on the queue
    if (this->message_queue.empty())
        return package{};

    // Extract the message
    package message = message_queue.front();

    // Remove the message from the queue
    message_queue.pop();

    return message;
}

/**
 * Broadcast a message to every peer on the network
 * @param message The message to be sent
 */
int Server::broadcast(std::string message)
{
    std::vector<sockaddr_in> peers_copy;

    {
        std::lock_guard<std::mutex> lock(this->mtx);
        peers_copy = this->peer_addresses;
    }

    // Iterate every peer
    for (int i = 0; i < peers_copy.size(); i++)
    {
        // Send the message to the current peer
        if (this->send_message(message, peers_copy.at(i)) == -1)
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
    int attempts = 0;

    // Try to send the message for MAX_ATTEMPTS
    while (attempts < MAX_ATTEMPTS)
    {
        // Try to send the message to the destination
        if (sendto(this->socket_fd, message.data(), message.size(), 0, (struct sockaddr *)&destination_address, sizeof(destination_address)) == -1)
            return -1;

        std::unique_lock<std::mutex> lock(this->reply_mtx);

        bool received = this->reply_cv.wait_for(lock, std::chrono::seconds(1), [this, &destination_address]()
                                                { return (this->reply.message == "OK" &&
                                                          this->reply.sender_address.sin_addr.s_addr == destination_address.sin_addr.s_addr &&
                                                          this->reply.sender_address.sin_port == destination_address.sin_port); });

        if (received)
        {
            this->reply = {};
            return 0;
        }

        attempts++;
    }

    return -1;
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

/**
 * Displays server errors
 */
void Server::show_errors()
{
    std::cout << "---- ERROS ----" << std::endl;

    // Show the total listening errors
    std::cout << "Total de erros de escuta: ";
    std::cout << this->listening_error_count << std::endl;

    // Show the total broadcasting errors
    std::cout << "Total de erros de broadcast: ";
    std::cout << this->broadcasting_error_count << std::endl;
}

#endif