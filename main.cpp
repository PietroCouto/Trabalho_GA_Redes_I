/**
 * @author Pietro do Couto Freitas
 */

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>

/**
 * Handmade libs
 */
#include "./libs/utils.hpp"
#include "./libs/config_handler/config_handler.cpp"

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

    // configurar a maquina

    // tentar conectar com as demais ao startar

    return 0;
}