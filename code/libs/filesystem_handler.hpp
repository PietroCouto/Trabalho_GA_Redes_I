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
    void stop_watch();

private:
    std::queue<std::string> event_queue;
    std::mutex mtx;
    std::atomic<bool> running;
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
    this->running = false;
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
 *
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

    this->running = true;

    // Watcher loop
    while (this->running)
    {
        ssize_t length = read(fd, buffer, BUF_LEN);

        if (length < 0)
            break;

        ssize_t i = 0;

        while (i < length)
        {
            auto *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

            // Check if an event was recieved
            if (event->len > 0)
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

bool FileSystemHandler::has_events() {
    std::lock_guard<std::mutex> lock(this->mtx);

    return this->event_queue.size() != 0;
};

std::string FileSystemHandler::get_event() {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (this->event_queue.size() == 0)
        return "";

    std::string event = this->event_queue.front();

     this->event_queue.pop();

    return event;
};

void FileSystemHandler::stop_watch() {
    this->running = false;
}

#endif