/**
 * @author Pietro do Couto Freitas
 */

#include "config_handler.hpp"

/**
 * Handles the config startup process
 * @param machine_address Pointer to the machine address variable
 * @param peer_addresses Pointer to the peer addresses array
 * @param machine_name The name of the current machine
 * @returns -1 for error, 0 for success
 */
int config_handler(sockaddr_in *machine_address, std::vector<sockaddr_in> *peer_addresses, std::string machine_name)
{
    // Get the config file path from the user
    std::string path = get_path();

    clear_screen();

    std::ifstream *config_file;

    // Try to open the file
    if (get_file(path, &config_file) == -1)
        return error_message("Não foi possível abrir o arquivo de configuração!");

    // Display a friendly success message
    std::cout
        << "Arquivo " << path << " aberto com sucesso!" << std::endl
        << std::endl;

    // Try to extract the config from the file
    if (find_config_on_file(&config_file, machine_address, peer_addresses, machine_name) == -1)
        return error_message("Não foi possível obter as configurações do arquivo!");

    // Display a friendly success message
    std::cout << "Configurações extraídas com sucesso!" << std::endl;

    return 0;
};

/**
 * Opens a file provided by the user
 * @param path The path of the file
 * @param config_file The address of the ifstream pointer
 * @returns -1 for error, 0 for success
 */
int get_file(std::string path, std::ifstream **config_file)
{
    // Try to open the provided file path
    *config_file = new std::ifstream(path.c_str(), std::ios::in);

    // Check if the file has been successfully openned
    if (!(*config_file)->is_open())
        return -1;

    return 0;
};

/**
 * @returns -1 for error, 0 for success
 */
int find_config_on_file(std::ifstream **config_file, sockaddr_in *machine_address, std::vector<sockaddr_in> *peer_addresses, std::string machine_name)
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

        // Check if this machine config has been found and extract it
        if (line.find(machine_name) != std::string::npos && get_machine_config(config_file, machine_address) == -1)
            return -1;

        // If it is a peer config and extract it
        else if (line.find("MACHINE") != std::string::npos && get_peer_config(config_file, peer_addresses) == -1)
            return -1;

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

    // Get the input from the user
    std::cin >> path;

    return path;
}

/**
 * Reads the config block containing the machine info
 * @param config_file Pointer to the open file
 * @param machine_address Pointer to the machine address structure
 * @returns -1 for error, 0 for success
 */
int get_machine_config(std::ifstream **config_file, sockaddr_in *machine_address)
{
    std::string line;

    machine_address->sin_family = AF_INET;

    // Read the entire block
    while (true)
    {
        // Try to read the next line
        if (!std::getline(**config_file, line) && !(*config_file)->eof())
            return -1;

        // Check for the blank line that separates config blocks or EOF
        if (line.compare("") == 0 || (*config_file)->eof())
            return 0;

        // Try to get the address
        if (line.find("ADDRESS") != std::string::npos && get_address(line, machine_address) == -1)
            return error_message("Erro ao obter o endereço do Peer!");

        // Try to get the port
        if (line.find("PORT") != std::string::npos && get_port(line, machine_address) == -1)
            return error_message("Erro ao obter a porta do Peer!");
    }

    return 0;
}

/**
 * Reads the config block containing a peer machine info
 * @param config_file Pointer to the open file
 * @param peer_addresses Pointer to the peer addresses structure
 * @returns -1 for error, 0 for success
 */
int get_peer_config(std::ifstream **config_file, std::vector<sockaddr_in> *peer_addresses)
{
    std::string line;

    // Define a new address structure
    sockaddr_in peer_address;

    // Empty the memory structure
    memset(&peer_address, 0, sizeof(sockaddr_in));

    // Define the sin_family
    peer_address.sin_family = AF_INET;

    // Read the entire block
    while (true)
    {
        // Attempts to read the next line
        if (!std::getline(**config_file, line) && !(*config_file)->eof())
            return -1;

        // Check for the blank line that separates configs or EOF
        if (line.compare("") == 0 || (*config_file)->eof())
            break;

        // Try to get the address
        if (line.find("ADDRESS") != std::string::npos && get_address(line, &peer_address) == -1)
            return error_message("Erro ao obter o endereço do Peer!");

        // Try to get the port
        if (line.find("PORT") != std::string::npos && get_port(line, &peer_address) == -1)
            return error_message("Erro ao obter a porta do Peer!");
    }

    // Push the address into the vector
    peer_addresses->push_back(peer_address);

    return 0;
};

/**
 * Extract the port from the file line
 * @param line Line to extract from
 * @param address Address struct to save the port to
 * @returns -1 for error, 0 for success
 */
int get_port(std::string line, sockaddr_in *address)
{
    // Validate the length of the port
    if (line.length() <= 5 || line.length() > 10)
        return -1;

    // Extract the substring    
    std::string port_substring = line.substr(5);

    // String to int
    int port = std::stoi(line.substr(5));

    // Int to NS
    address->sin_port = htons(port);

    return 0;
}

/**
 * Extract the address from the file line
 * @param line Line to extract from
 * @param address Address struct to save the address to
 * @returns -1 for error, 0 for success
 */
int get_address(std::string line, sockaddr_in *address)
{
    // Validate the length of the port
    if (line.length() <= 15 || line.length() > 23)
        return -1;

    // Int to NS
    inet_pton(AF_INET, line.substr(8).c_str(), &address->sin_addr);

    return 0;
}