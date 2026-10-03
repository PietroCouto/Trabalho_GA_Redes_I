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
#include "./libs/integration.hpp"

/**
 * Main
 */
int main(int argc, char *argv[])
{
    std::string machine_name = "";

    // Check if the machine name was provided as a parameter
    if (argc == 2)
        machine_name = std::string(argv[1]);

    // Try to get the machine name from docker env
    else if (std::getenv("CONTAINER_NAME") != NULL)
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