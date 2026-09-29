/**
 * @author Pietro do Couto Freitas
 */

#include <iostream>
#include <netinet/in.h>
#include <vector>
#include <thread>

/**
 * Handmade libs
 */
#include "./libs/utils.hpp"
#include "./libs/config_handler/config_handler.cpp"
#include "./libs/server.hpp"
#include "./libs/filesystem_handler.hpp"

/**
 * Prototypes
 */
void display_info(Server *server, FileSystemHandler *fs, int *broadcasting_errors, int *listening_errors);
void remove_file(std::string file_name, FileSystemHandler *fs);
void send_file_list(sockaddr_in sender_address, Server *server, FileSystemHandler *fs);
void send_file(package packet, Server *server, FileSystemHandler *fs);
void request_file(package packet, Server *server, FileSystemHandler *fs);
void save_to_buffer(std::string message, FileSystemHandler *fs);
void share_events(Server *server, FileSystemHandler *fs);
void request_file_list(Server *server, std::vector<sockaddr_in> *peer_addresses);

/**
 * Main
 */
int main(int argc, char *argv[])
{
    std::string machine_name;

    // Check if the machine name was provided as a parameter
    if (argc == 2)
        machine_name = std::string(argv[1]);

    // Try to get the machine name from docker env
    else
        machine_name = std::getenv("CONTAINER_NAME");

    // Validate that the machine name has been provided
    if (machine_name.size() == 0)
        return error_message("O nome da máquina deve ser fornecido!");

    // Display startup message
    show_startup_message(machine_name);

    // Variables to store the machine and peers addresses
    sockaddr_in machine_address;
    std::vector<sockaddr_in> peer_addresses;

    // Clean the memory structure for machine_address
    memset(&machine_address, 0, sizeof(sockaddr_in));

    // Try to open and read a provided config file
    if (config_handler(&machine_address, &peer_addresses, machine_name) == -1)
        return -1;

    // Check if the machine has been found on the file
    if (machine_address.sin_addr.s_addr == 0 || machine_address.sin_port == 0)
        return error_message("Máquina não encontrada no arquivo de configuração!");

    // Create the server object
    Server server(machine_name, machine_address, peer_addresses);

    // Try to start the server
    if (server.start() == -1)
        return -1;

    int broadcasting_errors = -1;
    int listening_errors = -1;

    // Start the filesystem handler
    FileSystemHandler fs = FileSystemHandler();

    // Start the listening thread
    std::thread listening_thread(&Server::listen, &server);
    std::thread watcher_thread(&FileSystemHandler::watch_dir, &fs);

    // Try to request the file list from other peers
    request_file_list(&server, &peer_addresses);

    // Main loop
    while (true)
    {
        display_info(&server, &fs, &broadcasting_errors, &listening_errors);

        // Check if peers have sent a message
        if (server.has_message())
        {
            package packet = server.get_message();

            // Check if a peer is anouncing a file
            if (packet.message.find("ANUNCIO") != std::string::npos)
                request_file(packet, &server, &fs);

            // Check if a peer is asking for a file
            if (packet.message.find("PEDIR") != std::string::npos)
                send_file(packet, &server, &fs);

            // Check if a peer has removed a file
            if (packet.message.find("REMOVIDO") != std::string::npos)
                remove_file(packet.message.substr(9), &fs);

            // Check if a peer has asked for the directory list
            if (packet.message.find("LISTA") != std::string::npos)
                send_file_list(packet.sender_address, &server, &fs);

            // Check if a peer has sent a file chunk
            if (packet.message.find("DADOS") != std::string::npos)
                save_to_buffer(packet.message.substr(6), &fs);
        }

        // Check if it has a local change
        if (fs.has_events())
        {
            // Display the filesystem changes before removing the event
            display_info(&server, &fs, &broadcasting_errors, &listening_errors);

            // Share the change amongst the peers
            share_events(&server, &fs);
        }
    }

    return 0;
}

/**
 * Displays current system information
 * @param server Pointer to the Server instance
 * @param fs Pointer to the FileSystemHandler instance
 * @param broadcasting_errors Pointer to the variable storing the broadcasting errors
 * @param listening_errors Pointer to the variable storing the listening errors
 */
void display_info(Server *server, FileSystemHandler *fs, int *broadcasting_errors, int *listening_errors)
{
    // Check if the error count or the filesystem has changed
    if (!fs->has_events() && server->get_broadcasting_error_count() == *broadcasting_errors && server->get_listening_error_count() == *listening_errors)
        return;

    // Update error count
    *broadcasting_errors = server->get_broadcasting_error_count();
    *listening_errors = server->get_listening_error_count();

    clear_screen();

    // Display server errors
    server->show_errors();

    std::cout << std::endl;

    // Display server files
    fs->show_files();

    std::cout << std::endl;
}

/**
 * Removes a file from the filesystem
 * @param file_name The name of the file to remove
 * @param fs Pointer to the filesystem handler class
 */
void remove_file(std::string file_name, FileSystemHandler *fs)
{
    fs->remove_file(file_name);
}

/**
 * Send the list of files on the filesystem to the requesting peer
 * @param sender_address The peer requesting the file
 * @param server Pointer to the server instance
 * @param fs Pointer to the filesystem handler instance
 */
void send_file_list(sockaddr_in sender_address, Server *server, FileSystemHandler *fs)
{
    // Get the directory file list
    std::vector<std::string> file_list = fs->get_file_list();

    // Send the list
    for (int i = 0; i < file_list.size(); i++)
        server->send_message(file_list.at(i), sender_address);
}

/**
 * Send a file to the requesting peer
 * @param packet The requesting peer packet
 * @param server Pointer to the server instance
 * @param fs Pointer to the filesystem handler instance
 */
void send_file(package packet, Server *server, FileSystemHandler *fs)
{
    // Get the file
    std::vector<std::string> file = fs->get_file(packet.message.substr(6));

    // Check if the file was found
    if (file.size() == 0)
        return;

    // Send file
    for (int i = 0; i < file.size(); i++)
        server->send_message(file.at(i), packet.sender_address);
}

/**
 * Requests a file from a peer
 * @param packet The packet that anounced the file
 * @param server Pointer to the server instance
 * @param fs Pointer to the filesystem handler instance
 */
void request_file(package packet, Server *server, FileSystemHandler *fs)
{
    // Header without the keyword
    std::string header = packet.message.substr(8);

    // Get the file name
    std::string file_name = header.substr(0, header.find_first_of(" "));

    // Get the announced file size
    int size = std::stoi(header.substr(header.find_first_of(" ") + 1));

    // Prepare the filesystem to receive the file
    fs->create_file_buffer(file_name, size);

    // Assemble the request message
    std::string message = "PEDIR " + file_name;

    // Request the file
    server->send_message(message, packet.sender_address);
}

/**
 * Extracts the file data from the packet and save it to the filesystem buffer
 * @param message The packet message
 * @param fs Pointer to the filesystem handler instance
 */
void save_to_buffer(std::string message, FileSystemHandler *fs)
{
    // Get the first space position
    int first_space = message.find(' ');

    // Check for bad formatting
    if (first_space == std::string::npos)
        return;

    // Get the file name
    std::string file_name = message.substr(0, first_space);

    // Get the second space position
    int second_space = message.find(' ', first_space + 1);

    // Check for bad formatting
    if (second_space == std::string::npos)
        return;

    int sequence_number = 0;

    // Get the sequence number
    sequence_number = std::stoi(message.substr(first_space + 1, second_space - (first_space + 1)));

    // Remove the headers
    std::string payload = message.substr(second_space + 1);

    // Save it to the filesystem buffer
    fs->save_to_buffer(file_name, sequence_number, payload);
}

/**
 * Get the filesystem changes and broadcasts to the other peers
 * @param server Pointer to the server instance
 * @param fs Pointer to the filesystem handler instance
 */
void share_events(Server *server, FileSystemHandler *fs)
{
    std::string event = fs->get_event();

    // Share the changes
    server->broadcast(event);
}

/**
 * Try to ask every peer for the list of files until one receives it
 * @param server Pointer to the server instance
 * @param peer_addresses Pointer to the peer addresses array
 */
void request_file_list(Server *server, std::vector<sockaddr_in> *peer_addresses)
{
    std::cout << "Solicitando lista de arquivos..." << std::endl;
    
    for (int i = 0; i < peer_addresses->size(); i++)
    {
        if (server->send_message("LISTA", peer_addresses->at(i)) == -1)
            continue;

        break;
    }
}