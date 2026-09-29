/**
 * @author Pietro do Couto Freitas
 */

#ifndef FILESYSTEM_HANDLER_HPP
#define FILESYSTEM_HANDLER_HPP

#include <iostream>
#include <filesystem>
#include <sys/inotify.h>
#include <queue>
#include <unistd.h>

#define PATH "./tmp"
#define EVENT_SIZE sizeof(struct inotify_event)
#define BUF_LEN 1024 * (EVENT_SIZE + 16)

/**
 * FileChunk structure
 */
struct FileChunk
{
    int sequence_number;
    std::string payload;
};

/**
 * FileBuffer structure
 */
struct FileBuffer
{
    std::string name;
    int total_packets;
    std::vector<FileChunk> chunks;
};

/**
 * FileSystemHandler Class to manage the system directory
 */
class FileSystemHandler
{
public:
    /**
     * Constructors
     */
    FileSystemHandler();

    /**
     * Methods
     */
    void show_files();
    int watch_dir();
    bool has_events();
    std::string get_event();
    std::string get_file_list();
    int remove_file(std::string file_name);
    std::vector<std::string> get_file(std::string file_name);
    void create_file_buffer(std::string file_name, int file_size);
    int save_to_buffer(std::string file_name, int sequence_number, std::string data);

private:
    /**
     * Attributes
     */
    std::queue<std::string> event_queue;
    std::mutex mtx;
    std::mutex ignore_file_mutex;
    std::string ignore_file;
    std::vector<FileBuffer> file_buffers;

    /**
     * Methods
     */
    int save_file(std::string file_name, std::vector<FileChunk> chunks);
    int calculate_fragmentation(int file_size, int file_name_size);
};

/**
 * Constructor
 */
FileSystemHandler::FileSystemHandler()
{
    // Check if the directory exists
    if (!std::filesystem::exists(PATH))
        std::filesystem::create_directory(PATH);

    // Create a new event queue
    this->event_queue = std::queue<std::string>();

    this->ignore_file = "";

    // Create the buffer to hold the files
    this->file_buffers = std::vector<FileBuffer>();
}

/**
 * Display every file on the tmp directory
 */
void FileSystemHandler::show_files()
{
    std::cout << "-- ARQUIVOS --" << std::endl;

    for (const auto &entry :
         std::filesystem::directory_iterator(PATH))
    {
        std::cout << entry.path() << " ";
        std::cout << entry.file_size() << " bytes" << std::endl;
    }
}

/**
 * Creates the directory watcher
 * @returns -1 if it fails
 */
int FileSystemHandler::watch_dir()
{
    // Create the inotify
    int fd = inotify_init();

    // Check if the creation was successful
    if (fd < 0)
        return -1;

    // Add the watcher to the inotify instance
    int wd = inotify_add_watch(fd, std::string(PATH).c_str(), IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

    // Check if the watcher was added successfully
    if (wd < 0)
    {
        // Close the inotify before exiting
        close(fd);
        return -1;
    }

    char buffer[BUF_LEN];

    // Watcher loop
    while (true)
    {
        ssize_t length = read(fd, buffer, BUF_LEN);

        if (length < 0)
            break;

        ssize_t i = 0;

        while (i < length)
        {
            auto *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

            std::string ignore;

            // Get the ignore file safely
            {
                std::lock_guard<std::mutex> lock(ignore_file_mutex);
                ignore = this->ignore_file;
            }

            // Check if an event was received and should not be ignored
            if (event->len > 0 && (ignore == "" || std::string(event->name).find(ignore) == std::string::npos))
            {
                // Check if it was a creation event
                if ((event->mask & IN_CREATE) || (event->mask & IN_MOVED_TO))
                {
                    // Obtain the file size
                    int size = std::filesystem::file_size(std::string(PATH) + "/" + event->name);

                    // Prevent racing condition on the event queue
                    std::lock_guard<std::mutex> lock(this->mtx);

                    // Format and add the event to the queue
                    this->event_queue.push("ANUNCIO " + std::string(event->name) + " " + std::to_string(size));
                }

                // Check if it was a removal event
                else if ((event->mask & IN_DELETE) || (event->mask & IN_MOVED_FROM))
                {
                    // Prevent racing condition on the event queue
                    std::lock_guard<std::mutex> lock(this->mtx);

                    // Add the event to the queue
                    this->event_queue.push("REMOVIDO " + std::string(event->name));
                }
            }

            i += EVENT_SIZE + event->len;
        }
    }

    // Destroy the watcher
    inotify_rm_watch(fd, wd);

    // Close the inotify file descriptor
    close(fd);

    return 0;
}

/**
 * Check if there is a new event
 * @returns Boolean
 */
bool FileSystemHandler::has_events()
{
    // Lock the queue to prevent race conditions
    std::lock_guard<std::mutex> lock(this->mtx);

    return this->event_queue.size() != 0;
};

/**
 * Get the next event on the queue
 * @returns String representing the event
 */
std::string FileSystemHandler::get_event()
{
    // Lock the queue to prevent race conditions
    std::lock_guard<std::mutex> lock(this->mtx);

    // Check if the queue is empty
    if (this->event_queue.size() == 0)
        return "";

    // Get the next event
    std::string event = this->event_queue.front();

    // Remove it from the queue
    this->event_queue.pop();

    return event;
};

/**
 * Get the list of files on the PATH directory
 * @returns String list of files
 */
std::string FileSystemHandler::get_file_list()
{
    std::string file_list;

    for (const auto &entry :
         std::filesystem::directory_iterator(PATH))
    {
        file_list.append(std::string(entry.path()).append("\n"));
    }

    return file_list;
};

/**
 * Removes a file from the PATH directory
 * @param file The file to be removed
 * @returns -1 for error, 0 for success
 */
int FileSystemHandler::remove_file(std::string file_name)
{
    // Check if the file exists
    if (!std::filesystem::exists(std::filesystem::path(PATH) / file_name))
        return -1;

    // Ignore the change
    {
        std::lock_guard<std::mutex> lock(this->ignore_file_mutex);
        this->ignore_file = file_name;
    }

    // Remove the file
    std::filesystem::remove(std::filesystem::path(PATH) / file_name);

    // Reset the ignore
    {
        std::lock_guard<std::mutex> lock(this->ignore_file_mutex);
        this->ignore_file = "";
    }

    return 0;
};

/**
 * Prepares a file to be sent
 * @param file The file name
 * @returns Vector containing the file parts as messages
 */
std::vector<std::string> FileSystemHandler::get_file(std::string file_name)
{
    // Create the vector to hold the file packets
    std::vector<std::string> file;

    // Assemble the file path
    std::filesystem::path file_path = std::filesystem::path(PATH) / file_name;

    // Check if the requested file exists
    if (!std::filesystem::exists(file_path))
        return file;

    // Get the file size
    int size = std::filesystem::file_size(file_path);

    if (size == 0)
        return file;

    // Try to open the file in binary mode
    std::ifstream config_file(file_path, std::ios::in | std::ios::binary);

    // Check if it was successfully opened
    if (!config_file.is_open())
        return file;

    // Calculate how many messages will be need
    int total_messages = calculate_fragmentation(size, file_name.size());

    // Iterate through the total messages
    for (int i = 0; i < total_messages; i++)
    {
        // Create the message header
        std::string header = "DADOS " + file_name + " " + std::to_string(i + 1) + " ";
        
        // Get the maximum payload size
        int payload_capacity = 1472 - static_cast<int>(header.size());

        // Check if the header exceeds the maximum size
        if (payload_capacity <= 0)
            break;

        // Create the buffer
        std::vector<char> buffer(payload_capacity);

        // Read the file into the buffer
        config_file.read(buffer.data(), payload_capacity);

        // Get the total bytes read
        std::streamsize bytes_read = config_file.gcount();

        if (bytes_read <= 0)
            break;

        // Create the packet
        std::string packet = header;

        // Add the payload
        packet.append(buffer.data(), bytes_read);

        // Push the packet into the vector
        file.push_back(packet);
    }

    return file;
}

/**
 * Saves a bufferized file to the directory
 * @param file_name The name of the file to be saved
 * @param chunks The chunks received
 * @returns -1 for error, 0 for success
 */
int FileSystemHandler::save_file(std::string file_name, std::vector<FileChunk> chunks)
{
    // Sort the data packets
    std::sort(chunks.begin(), chunks.end(), [](const FileChunk &a, const FileChunk &b)
              { return a.sequence_number < b.sequence_number; });

    // Assemble the file path
    std::filesystem::path output_path = std::filesystem::path(PATH) / file_name;

    // Ignore changes to the file path
    {
        std::lock_guard<std::mutex> lock(this->ignore_file_mutex);
        this->ignore_file = file_name;
    }

    // Try to open the file
    std::ofstream output_file(output_path, std::ios::out | std::ios::binary);

    // Check if the file was successfully opened
    if (!output_file.is_open())
        return -1;

    // Write every chunk onto the file
    for (const auto &chunk : chunks)
        output_file.write(chunk.payload.data(), chunk.payload.size());

    // Close the file stream
    output_file.close();

    // Reset the ignore_file
    {
        std::lock_guard<std::mutex> lock(this->ignore_file_mutex);
        this->ignore_file = "";
    }

    return 0;
}

/**
 * Calculates how many packets are needed to fit the file
 * @param file_size The size of the file to be receieved
 * @returns The amount of packets needed
 */
int FileSystemHandler::calculate_fragmentation(int file_size, int file_name_size)
{
    // Maximum packet size minus the protocol words
    int divide_by = 1472 - (12 + file_name_size);

    // Return the total messages needed
    return (file_size + divide_by - 1) / divide_by;
}

/**
 * Creates a new buffer to store a file
 * @param file_name The name of the file to be bufferized
 * @param file_size The size of the file
 */
void FileSystemHandler::create_file_buffer(std::string file_name, int file_size)
{
    // Calculate total packets
    int total_messages = calculate_fragmentation(file_size, file_name.size());

    // Create a new slot on the buffer
    this->file_buffers.push_back({file_name, total_messages, std::vector<FileChunk>()});
};

/**
 * Stores a file piece into the file buffer
 * @param file_name The file name
 * @param sequence_number The packet sequence number
 * @param data The data to be saved
 * @returns -1 for error, 0 for success
 */
int FileSystemHandler::save_to_buffer(std::string file_name, int sequence_number, std::string data)
{
    int position = -1;

    // Find the buffer position
    for (int i = 0; i < this->file_buffers.size(); i++)
    {
        if (this->file_buffers.at(i).name == file_name)
            position = i;
    }

    // If the file was not found
    if (position == -1)
        return -1;

    // Save the data to the buffer
    this->file_buffers.at(position).chunks.push_back({sequence_number, data});

    // Check if all the packets have been received
    if (this->file_buffers.at(position).total_packets == this->file_buffers.at(position).chunks.size())
    {
        // Save the file
        this->save_file(file_name, this->file_buffers.at(position).chunks);

        // Delete the buffer
        this->file_buffers.erase(this->file_buffers.begin() + position);
    }

    return 0;
};

#endif