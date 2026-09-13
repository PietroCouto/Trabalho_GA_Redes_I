/**
 * @author Pietro do Couto Freitas
 */

#ifndef CONFIG_HANDLER_HPP
#define CONFIG_HANDLER_HPP

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "utils.hpp"

/**
 * Prototypes
 */
int config_handler(sockaddr_in *machine_address, sockaddr_in *peer_addresses, std::string machine_name);
int get_file(std::string path, std::ifstream **config_file);
std::string get_path();
int find_config_on_file(std::ifstream **config_file, sockaddr_in *machine_address, sockaddr_in *peer_addresses, std::string machine_name);
int get_machine_config(std::ifstream **config_file, sockaddr_in *machine_address);
int get_peer_config(std::ifstream **config_file, sockaddr_in *peer_addresses);

/**
 *
 */
int config_handler(sockaddr_in *machine_address, sockaddr_in *peer_addresses, std::string machine_name)
{
    // Get the config file path from the user
    std::string path = get_path();

    clear_screen();

    std::ifstream *config_file;

    // Try to open the file
    if (get_file(path, &config_file) == -1)
    {
        std::cout << "Não foi possível abrir o arquivo de configuração!" << std::endl;
        return -1;
    }

    std::cout << "Arquivo " << path << "  aberto com sucesso!" << std::endl
              << std::endl;

    // Try to extract the config from the file
    if (find_config_on_file(&config_file, machine_address, peer_addresses, machine_name) == -1)
    {
        std::cout << "Não foi possível obter as configurações do arquivo!" << std::endl;
        return -1;
    }

    std::cout << "Configurações extraídas com sucesso!" << std::endl;

    return 0;
};

/**
 *
 */
int find_config_on_file(std::ifstream **config_file, sockaddr_in *machine_address, sockaddr_in *peer_addresses, std::string machine_name)
{
    std::string line;

    // Check if the file has error
    if (!(*config_file)->good())
        return -1;

    // Read the entire file
    while (true)
    {
        // Attempts to read the next line
        if (!std::getline(**config_file, line) && !(*config_file)->eof())
            return -1;

        // Check if this machine config has been found
        if (line.find(machine_name) != std::string::npos)
            get_machine_config(config_file, machine_address);

        // If it is a peer config
        // else if (line.find("MACHINE") != std::string::npos)
        //     get_peer_config(config_file, peer_addresses);

        if ((*config_file)->eof())
            break;
    }

    // Close the config file
    (*config_file)->close();

    // Free the memory pointer
    delete *config_file;

    return 0;
};

/**
 * Ask the file path to the user
 * @returns The string path
 */
std::string get_path()
{
    std::string path;

    // Ask for the file path
    std::cout << "Digite o path do arquivo de configuração: ";

    std::cin >> path;

    return path;
}

int get_machine_config(std::ifstream **config_file, sockaddr_in *machine_address)
{
    std::string line;

    // Read the entire file
    while (true)
    {
        // Attempts to read the next line
        if (!std::getline(**config_file, line) && !(*config_file)->eof())
            return -1;

        // Check for the blank line that separates configs
        if (line.compare("\n") == std::string::npos)
            return 0;

        // Get the address
        if (line.find("ADDRESS") != std::string::npos)
            inet_pton(AF_INET, line.substr(8).c_str(), &(*machine_address).sin_addr);

        // Get the port
        if (line.find("PORT") != std::string::npos)
            (*machine_address).sin_port = htons(std::stoi(line.substr(5)));

        if ((*config_file)->eof())
            break;
    }

    return 0;
}

int get_peer_config(std::ifstream **config_file, sockaddr_in *peer_addresses)
{
    std::string line;

    // Read the entire file
    while (true)
    {
        // Attempts to read the next line
        if (!std::getline(**config_file, line) && !(*config_file)->eof())
            return -1;

        // Check for the blank line that separates configs
        if (line.compare("\n") == std::string::npos)
            return 0;

        // Get the address
        if (line.find("ADDRESS") != std::string::npos)
            inet_pton(AF_INET, line.substr(8).c_str(), &(*peer_addresses).sin_addr);

        // Get the port
        if (line.find("PORT") != std::string::npos)
            (*peer_addresses).sin_port = htons(std::stoi(line.substr(5)));

        if ((*config_file)->eof())
            break;
    }

    return 0;
};

/**
 * Opens a file provided by the user
 * @param path The path of the file
 * @param config_file The address of the ifstream pointer
 */
int get_file(std::string path, std::ifstream **config_file)
{
    // Try to open the provided file path
    *config_file = new std::ifstream(path.c_str(), std::ios::in);

    // Check if the file has been successfully read
    if (!(*config_file)->is_open())
        return -1;

    return 0;
};

#endif