/**
 * @author Pietro do Couto Freitas
 */

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "./libs/utils.hpp"
#include "./libs/config_handler.hpp"

int main(int argc, char *argv[])
{
    // Validate that the machine name has been provided
    if (argc == 1)
    {
        std::cout << "O nome da máquina deve ser fornecido!" << std::endl;
        return -1;
    }

    // Display startup message
    show_startup_message(argv[1]);

    // Variables to store the machine and peers addresses
    sockaddr_in *machine_address;
    sockaddr_in *peer_addresses;

    // Try to open and read a provided config file
    if (config_handler(machine_address, peer_addresses, argv[1]) == -1)
        return -1;

    std::cout << machine_address->sin_addr.s_addr << std::endl;

    // configurar a maquina

    // tentar conectar com as demais ao startar

    return 0;
}