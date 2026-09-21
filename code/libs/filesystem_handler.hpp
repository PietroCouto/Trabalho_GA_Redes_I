/**
 * @author Pietro do Couto Freitas
 */

#ifndef FILESYSTEM_HANDLER_HPP
#define FILESYSTEM_HANDLER_HPP

#include <iostream>
#include <filesystem>

#define PATH "./tmp"

class FileSystemHandler
{
public:
    /**
     * Constructors
     */
    FileSystemHandler();

    void show_files();

private:
};

/**
 * Constructor
 */
FileSystemHandler::FileSystemHandler()
{
    if (!std::filesystem::exists(PATH))
        std::filesystem::create_directory(PATH);
}

void FileSystemHandler::show_files()
{
    std::cout << "-- ARQUIVOS --" << std::endl;

    if (std::filesystem::is_empty(PATH))
        std::cout << "VAZIO";

    for (const auto &entry :
         std::filesystem::directory_iterator(PATH))
    {
        std::cout << entry.path() << " ";
        std::cout << entry.file_size() << " bytes" << std::endl;
    }
}

#endif