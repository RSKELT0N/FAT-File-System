#ifndef _TERMINAL_H
#define _TERMINAL_H

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

#include "Disk.h"
#include "Log.h"

#define SEPERATOR ' '

class Terminal {
private:
    enum Command     {
        mkdir,
        ls,
        cd,
        touch,
        cat,
        invalid
        };

    typedef struct     {
        Command comm;
        bool (*valid)(const std::vector<std::string>&);
        std::string desc;
        } input_t;

    void run();
    void define_input_commands();
    std::vector<std::string> split(const std::string&, const char separator);
    Command validate_command(std::vector<std::string>& tokens);
    uint32_t get_dir_start_cluster(Disk::dir_t* dir);
    void cd_command(std::vector<std::string>& tokens);

    std::string path;
    std::unordered_map<std::string, input_t>* inputCommands;
    std::string inputLine;
    Disk* disk;
    uint32_t current_directory_index;
    Disk::dir_t* current_directory;

public:
    Terminal();
    ~Terminal();

    friend bool valid_mkdir(const std::vector<std::string>&);
    friend bool valid_ls(const std::vector<std::string>&);
    friend bool valid_cd(const std::vector<std::string>&);
    friend bool valid_cat(const std::vector<std::string>&);
    friend bool valid_touch(const std::vector<std::string>&);
    };

#endif