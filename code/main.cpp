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

void display_info(Server *server, FileSystemHandler *fs, int *broadcasting_errors, int *listening_errors);

int main(int argc, char *argv[])
{
    // Validate that the machine name has been provided
    if (argc == 1)
        return error_message("O nome da máquina deve ser fornecido!");

    // Display startup message
    show_startup_message(argv[1]);

    // Variables to store the machine and peers addresses
    sockaddr_in machine_address;
    std::vector<sockaddr_in> peer_addresses;

    // Clean the memory structure for machine_address
    memset(&machine_address, 0, sizeof(sockaddr_in));

    // Try to open and read a provided config file
    if (config_handler(&machine_address, &peer_addresses, argv[1]) == -1)
        return -1;

    // Check if the machine has been found on the file
    if (machine_address.sin_addr.s_addr == 0 || machine_address.sin_port == 0)
        return error_message("Máquina não encontrada no arquivo de configuração!");

    // Create the server object
    Server server(argv[1], machine_address, peer_addresses);

    // Try to start the server
    if (server.start() == -1)
        return -1;

    int broadcasting_errors = -1;
    int listening_errors = -1;

    // iniciar o filesystem
    FileSystemHandler fs = FileSystemHandler();

    // fazer snapshot do filesystem

    // Start the listening thread
    std::thread listening_thread(&Server::listen, &server);

    server.broadcast("teste");

    // Main loop
    while (true)
    {
        display_info(&server, &fs, &broadcasting_errors, &listening_errors);

        if (server.has_message())
        {
            std::cout << server.get_message().message << std::endl;
        }
    }

    // Join the thread upon ending
    listening_thread.join();

    // Show ending message
    std::cout << "Programa finalizado!" << std::endl;
    std::cout << std::endl;

    return 0;
}

void display_info(Server *server, FileSystemHandler *fs, int *broadcasting_errors, int *listening_errors)
{
    if (server->get_broadcasting_error_count() == *broadcasting_errors && server->get_listening_error_count() == *listening_errors)
        return;

    *broadcasting_errors = server->get_broadcasting_error_count();
    *listening_errors = server->get_listening_error_count();

    clear_screen();

    server->show_errors();

    std::cout << std::endl;

    fs->show_files();
}