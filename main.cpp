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

    std::cout << "Servidor inicializado!" << std::endl;
    std::cout << std::endl;

    // iniciar o filesystem

    // fazer snapshot do filesystem

    // Start the listening thread
    std::thread listening_thread(&Server::listen, &server);

    server.broadcast("teste");

    // Main loop
    while (true)
    {
        if (server.has_message())
        {
            std::cout << "Mensagem!" << std::endl;
            server.get_message();
        }
    }

    // Join the thread upon ending
    listening_thread.join();

    // Show the total listening errors
    std::cout << "Total de erros de escuta: ";
    std::cout << server.get_listening_error_count() << std::endl;

    // Show ending message
    std::cout << "Programa finalizado!" << std::endl;
    std::cout << std::endl;

    return 0;
}