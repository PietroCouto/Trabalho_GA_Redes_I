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
#include <vector>

#include "../utils.hpp"

#define CONFIG_PATH "./config/config.txt"

/**
 * Prototypes
 */
int config_handler(sockaddr_in *machine_address, std::vector<sockaddr_in> *peer_addresses, std::string machine_name);
int get_file(std::ifstream **config_file);
int find_config_on_file(std::ifstream **config_file, sockaddr_in *machine_address, std::vector<sockaddr_in> *peer_addresses, std::string machine_name);
int get_machine_config(std::ifstream **config_file, sockaddr_in *machine_address);
int get_peer_config(std::ifstream **config_file, std::vector<sockaddr_in> *peer_addresses);
int get_port(std::string line, sockaddr_in *address);
int get_address(std::string line, sockaddr_in *address);
void display_extracted_configs(sockaddr_in *machine_address, std::vector<sockaddr_in> *peer_addresses, std::string machine_name);

#endif