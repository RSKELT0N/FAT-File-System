#include "Terminal.h"

bool valid_mkdir(const std::vector<std::string>&);
bool valid_ls(const std::vector<std::string>&);
bool valid_cd(const std::vector<std::string>&);
bool valid_cat(const std::vector<std::string>&);
bool valid_touch(const std::vector<std::string>&);

Terminal::Terminal() {
    inputCommands = new std::unordered_map<std::string, input_t>();

    path = "/";
    disk = new Disk();
    current_directory_index = 0;
    current_directory = disk->read_dir(current_directory_index);

    define_input_commands();
    run();
    }

Terminal::~Terminal() {
    free(disk);
    free(inputCommands);
    }

void Terminal::define_input_commands() {
    (*inputCommands)["mkdir"] = { mkdir, &valid_mkdir, "mkdir - creates an empty directory within the user's pwd" };
    (*inputCommands)["ls"] = { ls, &valid_ls, "ls - lists the entries within the pwd" };
    (*inputCommands)["touch"] = { touch, &valid_touch, "touch - creates an empty file within the user's pwd" };
    (*inputCommands)["cd"] = { cd, &valid_cd, "cd - chooses a directory within the user's pwd" };
    }

void Terminal::run() {
    std::cout << std::endl;
    while (1) {
        std::cout << path << "> ";
        getline(std::cin, inputLine);

        std::cout << "\n";

        if (inputLine.empty())
            continue;

        std::vector<std::string> tokens = split(inputLine, SEPERATOR);
        Command cmd = validate_command(tokens);
        uint32_t index;

        switch (cmd) {
            case invalid:
                // LOG(Log::WARNING, "invalid command");
                break;

            case mkdir:
                index = get_dir_start_cluster(current_directory);
                disk->insert_dir(tokens[1].c_str(), current_directory, index);
                break;

            case ls:
                disk->print_dir(*current_directory);
                break;

            case cd:
                cd_command(tokens);
                break;

            case cat:
                break;
            }
        current_directory = disk->read_dir(current_directory_index);
        }
    }

void Terminal::cd_command(std::vector<std::string>& tokens) {
    int index;

    index = disk->find_entry(current_directory, tokens[1].c_str(), DIRECTORY_FLAG);

    if (index != -1) {
        current_directory_index = index;
        current_directory = disk->read_dir(current_directory_index);
        }

    int i = 0;

    if (tokens[1] == ".")
        return;
    else if (tokens[1] == "..") {
        for (i = path.size() - 2; i >= 0; i--) {
            if (path[i] == '/')
                break;
            }
        path = path.substr(0, i);
        path += "/";

        return;
        }

    path += tokens[1] + "/";

    return;
    }

Terminal::Command Terminal::validate_command(std::vector<std::string>& tokens) {
    if (inputCommands->find(tokens[0]) == inputCommands->end() || tokens.empty())
        return invalid;

    if (inputCommands->find(tokens[0])->second.valid(tokens) == 0)
        return invalid;
    else
        return inputCommands->find(tokens[0])->second.comm;
    }

std::vector<std::string> Terminal::split(const std::string& line, const char seperator) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string x;

    while ((getline(ss, x, seperator)))
        if (x != "")
            tokens.push_back(x);

    return tokens;
    }

bool valid_mkdir(const std::vector<std::string>& tokens) {
    if (tokens.size() != 2)
        return 0;

    return 1;
    }

bool valid_ls(const std::vector<std::string>& tokens) {
    if (tokens.size() != 1)
        return 0;

    return 1;
    }

bool valid_cd(const std::vector<std::string>& tokens) {
    if (tokens.size() != 2)
        return 0;

    return 1;
    }

bool valid_cat(const std::vector<std::string>& tokens) {
    if (tokens.size() != 2)
        return 0;

    return 1;
    }

bool valid_touch(const std::vector<std::string>& tokens) {
    if (tokens.size() != 2)
        return 0;

    return 1;
    }

uint32_t Terminal::get_dir_start_cluster(Disk::dir_t* dir) {
    return dir->entries[0].start_cluster;
    }
