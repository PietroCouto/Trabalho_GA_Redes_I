/**
 * @author Pietro do Couto Freitas
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>

/**
 * Prototypes
 */
void show_startup_message(std::string machine_name);
void clear_screen();
int error_message(std::string error_message);

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
 * Simple function to clear the shell on linux
 */
void clear_screen()
{
    system("clear");
}

int error_message(std::string error_message) {
    std::cout << error_message << std::endl;
    return -1;
};

#endif