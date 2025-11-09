#include <iostream>
#include <string>
#include <vector>
#include <sstream>      // To parse user input
#include <dirent.h>     // For opendir, readdir, closedir
#include <sys/stat.h>   // For stat, mkdir, AND chmod
#include <unistd.h>     // For getcwd, chdir
#include <fstream>      // For file streams (touch, cp)
#include <cstdio>       // For rename (mv) and remove (rm)
#include <iomanip>      // New: For formatting output (std::setw)

// --- Helper Functions ---

bool isDirectory(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

// --- NEW: Helper function to get permission string (Day 5) ---
std::string getPermissionString(mode_t mode) {
    std::string perms = "----------";
    if (S_ISDIR(mode)) perms[0] = 'd';

    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';

    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';

    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';
    
    return perms;
}

// --- UPDATED: listFiles function (Day 5) ---
// Now shows permissions for each file
void listFiles(const std::string& path) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(path.c_str())) != NULL) {
        std::cout << "--- Listing for: " << path << " ---" << std::endl;
        while ((entry = readdir(dir)) != NULL) {
            std::string entryName = entry->d_name;
            if (entryName == ".") continue;
            
            std::string fullPath = path + "/" + entryName;
            
            // Get file stats to read permissions
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) != 0) {
                // On error, just print the name
                std::cout << "  [?] " << entryName << std::endl;
                continue;
            }
            
            std::string perms = getPermissionString(statbuf.st_mode);
            std::string type = S_ISDIR(statbuf.st_mode) ? "[DIR] " : "[FILE]";

            // std::left and std::setw(12) are for nice formatting
            std::cout << perms << " " << std::left << std::setw(6) << type
                      << entryName << (S_ISDIR(statbuf.st_mode) ? "/" : "") << std::endl;
        }
        closedir(dir);
        std::cout << "-----------------------------------" << std::endl;
    } else {
        perror("opendir() error");
    }
}

std::string getCurrentPath() {
    char cwd_buffer[FILENAME_MAX];
    if (getcwd(cwd_buffer, sizeof(cwd_buffer)) != NULL) {
        return std::string(cwd_buffer);
    }
    perror("getcwd() error");
    return "";
}

bool copyFile(const std::string& source, const std::string& destination) {
    std::ifstream src(source, std::ios::binary);
    if (!src) return false;
    std::ofstream dest(destination, std::ios::binary);
    if (!dest) return false;
    dest << src.rdbuf();
    return true;
}

void findFile(const std::string& dirPath, const std::string& fileName) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(dirPath.c_str())) == NULL) return;

    while ((entry = readdir(dir)) != NULL) {
        std::string entryName = entry->d_name;
        std::string fullPath = dirPath + "/" + entryName;
        if (entryName == "." || entryName == "..") continue;
        if (entryName == fileName) {
            std::cout << "Found: " << fullPath << std::endl;
        }
        if (isDirectory(fullPath)) {
            findFile(fullPath, fileName);
        }
    }
    closedir(dir);
}

int main() {
    std::string currentPath = getCurrentPath();
    std::string inputLine;
    
    while (true) {
        // Reset output formatting
        std::cout.copyfmt(std::ios(NULL));

        // 1. List files
        listFiles(currentPath);

        // 2. Get input
        std::cout << "\nCommands: cd, cp, mv, rm, touch, mkdir, find, chmod, quit\n> ";
        std::getline(std::cin, inputLine);

        // 3. Parse input
        std::stringstream ss(inputLine);
        std::string command, arg1, arg2;
        ss >> command >> arg1 >> arg2;

        std::string fullArg1, fullArg2;
        if (!arg1.empty()) fullArg1 = (arg1[0] == '/') ? arg1 : (currentPath + "/" + arg1);
        if (!arg2.empty()) fullArg2 = (arg2[0] == '/') ? arg2 : (currentPath + "/" + arg2);
        
        // 4. Handle commands
        if (command == "quit") {
            std::cout << "Exiting." << std::endl;
            break;
        } 
        // ... (all the old commands: cd, touch, mkdir, cp, mv, rm, find) ...
        else if (command == "cd") {
            if (arg1.empty()) { std::cout << "Usage: cd <dir>" << std::endl; }
            else {
                std::string targetPath = (arg1[0] == '/') ? arg1 : (currentPath + "/" + arg1);
                if (chdir(targetPath.c_str()) == 0) {
                    currentPath = getCurrentPath();
                } else {
                    perror("Error changing directory");
                }
            }
        } else if (command == "touch") {
            if (arg1.empty()) { std::cout << "Usage: touch <file>" << std::endl; }
            else { std::ofstream(fullArg1.c_str()); }
        } else if (command == "mkdir") {
            if (arg1.empty()) { std::cout << "Usage: mkdir <dir>" << std::endl; }
            else {
                if (mkdir(fullArg1.c_str(), 0777) != 0) {
                    perror("Error creating directory");
                }
            }
        } else if (command == "cp") {
            if (arg1.empty() || arg2.empty()) { std::cout << "Usage: cp <src> <dest>" << std::endl; }
            else {
                if (!copyFile(fullArg1, fullArg2)) {
                    std::cout << "File copy failed." << std::endl;
                }
            }
        } else if (command == "mv") {
            if (arg1.empty() || arg2.empty()) { std::cout << "Usage: mv <src> <dest>" << std::endl; }
            else {
                if (rename(fullArg1.c_str(), fullArg2.c_str()) != 0) {
                    perror("Error moving file");
                }
            }
        } else if (command == "rm") {
            if (arg1.empty()) { std::cout << "Usage: rm <file>" << std::endl; }
            else {
                if (remove(fullArg1.c_str()) != 0) {
                    perror("Error deleting file");
                }
            }
        } else if (command == "find") {
            if (arg1.empty()) { std::cout << "Usage: find <filename>" << std::endl; }
            else {
                std::cout << "Searching for '" << arg1 << "' in " << currentPath << "..." << std::endl;
                findFile(currentPath, arg1);
            }
        }
        
        // --- NEW: Handle 'chmod' command (Day 5) ---
        else if (command == "chmod") {
            if (arg1.empty() || arg2.empty()) {
                std::cout << "Usage: chmod <mode> <filename>" << std::endl;
                std::cout << "Example: chmod 755 myfile.txt" << std::endl;
            } else {
                // We need to convert the mode string (e.g., "755") to an octal number
                // std::stoul with base 0 automatically handles "0755" (octal)
                // We'll use base 8 explicitly to be clear
                try {
                    mode_t mode = std::stoul(arg1, nullptr, 8); // Base 8 (octal)
                    
                    // We use fullArg2 for the file path
                    if (chmod(fullArg2.c_str(), mode) != 0) {
                        perror("Error changing permissions");
                    }
                } catch (const std::exception& e) {
                    std::cout << "Error: Invalid mode. Use octal numbers (e.g., 755, 644)." << std::endl;
                }
            }
        }
        
        // --- End of commands ---
        
        else if (command.empty()) {
            // User just pressed Enter
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
        
        std::cout << std::endl;
    }

    return 0;
}
