// models/UserRepository.cpp
#include "UserRepository.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Static variable to hold current user across instances
static std::string globalCurrentUser;

void UserRepository::initialize(const std::string& path) {
    filePath = path;
    loadUsers();
}

void UserRepository::loadUsers() {
    try {
        std::ifstream file(filePath);
        if (!file) {
            throw std::runtime_error("Error: File does not exist - " + filePath);
        }

        std::string read_line;
        while (getline(file, read_line)) {
            std::string username, password;
            
            std::stringstream line(read_line);

            if (getline(line, username, ',') && getline(line, password)) {            
                addUser(username, password);
            }
        }
        file.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
    }
}

void UserRepository::saveUsers() {
    try {
        std::ofstream info_out(filePath);
        if (!info_out) {
            throw std::runtime_error("Error: File does not exist - " + filePath);
        }

        for (const User& user : users) {
            info_out << user.getUsername() << ","
                     << user.getPassword() << "\n";
        }

        info_out.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
    }
}

User* UserRepository::findByUsername(const std::string& username) {
    for (size_t i = 0; i < users.size(); i++) {
        if (users[i].getUsername() == username) return &users[i];
    }
    return nullptr;
}

bool UserRepository::addUser(const std::string& username, const std::string& password) {
    // Check if user already exists
    if (findByUsername(username) != nullptr) {
        return false;
    }
    
    User newUser(username, password);
    users.push_back(newUser);
    saveUsers();
    return true;
}

std::vector<User>& UserRepository::getAllUsers() {
    return users;
}

void UserRepository::setCurrentUser(const std::string& username) {
    currentUser = username;
    globalCurrentUser = username;
}

std::string UserRepository::getCurrentUser() const {
    return currentUser;
}

std::string UserRepository::getCurrentUserName() {
    return globalCurrentUser;
}