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

    // Start the filesystem handler
    FileSystemHandler fs = FileSystemHandler();

    // Start the listening thread
    std::thread listening_thread(&Server::listen, &server);
    std::thread watcher_thread(&FileSystemHandler::watch_dir, &fs);

    // TODO pedir lista de arquivos para os outros peers

    // Main loop
    while (true)
    {
        display_info(&server, &fs, &broadcasting_errors, &listening_errors);

        // Check if peers have sent a message
        if (server.has_message())
        {
            package packet = server.get_message();

            std::cout << packet.message << std::endl;

            // Check if a peer is anouncing a file
            if (packet.message.find("ANUNCIO") != std::string::npos) {
                
            }

            // Check if a peer is asking for a file
            if (packet.message.find("PEDIR") != std::string::npos) {}

            // Check if a peer is sending a file piece
            if (packet.message.find("DADOS") != std::string::npos) {}

            // Check if a peer has removed a file
            if (packet.message.find("REMOVIDO") != std::string::npos) {}

            // Check if a peer has asked for the directory list
            if (packet.message.find("LISTA") != std::string::npos) {
                std::string file_list = fs.get_file_list();

                server.send_message(file_list, packet.sender_address);
            }
        }

        // Check if it has a local change
        if (fs.has_events())
        {
            std::string event = fs.get_event();

            /**
             * Lembre-se de que o UDP não garante a entrega: datagramas podem se perder, chegar fora
             * de ordem ou duplicados. O seu programa precisa tolerar isso na própria aplicação, por
             * exemplo reenviando um anúncio até ter certeza de que os outros peers o receberam, ou
             * pedindo de novo um pedaço de arquivo que não chegou.
             */

            // Share the changes
            server.broadcast(event);
        }
    }

    // Join the threads upon ending
    listening_thread.join();
    watcher_thread.join();

    // Show ending message
    std::cout << "Programa finalizado!" << std::endl;
    std::cout << std::endl;

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
    // Check if the error count has changed
    if (server->get_broadcasting_error_count() == *broadcasting_errors && server->get_listening_error_count() == *listening_errors)
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