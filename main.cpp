/**
 * @author Pietro do Couto Freitas
 */

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <bits/stdc++.h>

/**
 * Prototypes
 */
void show_startup_message(std::string machine_name);
int config_handler(sockaddr_in *machine_address, sockaddr_in *peer_addresses);
int get_config_file(std::string path, std::ifstream **config_file);

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
    sockaddr_in machine_address;
    sockaddr_in peer_addresses;

    // Try to open and read a provided config file
    if (config_handler(&machine_address, &peer_addresses) == -1)
        return -1;

    // configurar a maquina

    // tentar conectar com as demais ao startar

    return 0;
}

/**
 * Shows the program startup message
 * @param machine_name the name of the machine to be spun up
 */
void show_startup_message(std::string machine_name)
{
    std::cout << "Bem vindo!" << std::endl;
    std::cout << "Nome da máquina: " << machine_name << std::endl;
};

/**
 *
 */
int config_handler(sockaddr_in *machine_address, sockaddr_in *peer_addresses)
{
    std::string path = get_path();

    std::ifstream *config_file;

    // Try to open the file
    if (get_config_file(path, &config_file) == -1)
    {
        std::cout << "Não foi possível abrir o arquivo de configuração!" << std::endl;
        return -1;
    }

    std::cout << "Arquivo aberto com sucesso!" << std::endl;

    std::string line;

    if (config_file->eof())
        std::cout << "Final do arquivo atingido!" << std::endl;

    if (!std::getline(*config_file, line))
    {
        std::cout << "Falha ao ler o arquivo de configuração!" << std::endl;
        return -1;
    };

    std::cout << line << std::endl;

    // Close the config file
    config_file->close();

    return 0;
};

/**
 * 
 */
std::string get_path() {
    std::string path;

    // Ask for the file path
    std::cout << "Digite o path do arquivo de configuração: ";

    std::cin >> path;

    return path;
}

/**
 * Opens a file provided by the user
 * @param path The path of the file
 * @param config_file The address of the ifstream pointer
 */
int get_config_file(std::string path, std::ifstream **config_file)
{
    // Try to open the provided file path
    *config_file = new std::ifstream(path.c_str(), std::ios::in);

    // Check if the file has been successfully read
    if (!(*config_file)->is_open())
        return -1;

    return 0;
};