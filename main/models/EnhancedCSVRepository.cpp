// models/EnhancedCSVRepository.cpp
#include "EnhancedCSVRepository.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

// Static global variable for current user tracking
static std::string globalCurrentUser;

EnhancedCSVRepository::~EnhancedCSVRepository() {
    // Ensure any unsaved changes are written to disk
    if (isDirty) {
        commit();
    }
}

bool EnhancedCSVRepository::initialize(const std::string& path) {
    filePath = path;
    loadData();
    return true;
}

bool EnhancedCSVRepository::parseCSVLine(const std::string& line, std::string& username, std::string& password) {
    std::stringstream ss(line);
    if (std::getline(ss, username, ',') && std::getline(ss, password)) {
        // Remove any trailing whitespace from password
        password.erase(std::find_if(password.rbegin(), password.rend(), 
                                    [](unsigned char ch) { return !std::isspace(ch); }).base(), 
                        password.end());
        return true;
    }
    return false;
}

std::string EnhancedCSVRepository::createCSVLine(const User& user) {
    return user.getUsername() + "," + user.getPassword();
}

void EnhancedCSVRepository::loadData() {
    std::lock_guard<std::mutex> lock(fileMutex);
    
    users.clear();
    usernameIndex.clear();
    
    try {
        std::ifstream file(filePath);
        if (!file) {
            std::cerr << "Warning: Could not open file " << filePath << std::endl;
            return;
        }
        
        std::string line;
        size_t index = 0;
        
        while (std::getline(file, line)) {
            std::string username, password;
            
            if (parseCSVLine(line, username, password)) {
                User user(username, password);
                users.push_back(user);
                usernameIndex[username] = index++;
            }
        }
        
        file.close();
        isDirty = false;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during file loading: " << e.what() << std::endl;
    }
}

bool EnhancedCSVRepository::saveData() {
    std::lock_guard<std::mutex> lock(fileMutex);
    
    try {
        std::ofstream file(filePath);
        if (!file) {
            std::cerr << "Error: Could not open file for writing: " << filePath << std::endl;
            return false;
        }
        
        for (const User& user : users) {
            file << createCSVLine(user) << std::endl;
        }
        
        file.close();
        isDirty = false;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during file saving: " << e.what() << std::endl;
        return false;
    }
}

bool EnhancedCSVRepository::appendRecord(const User& user) {
    std::lock_guard<std::mutex> lock(fileMutex);
    
    try {
        std::ofstream file(filePath, std::ios::app);
        if (!file) {
            std::cerr << "Error: Could not open file for appending: " << filePath << std::endl;
            return false;
        }
        
        file << createCSVLine(user) << std::endl;
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during record appending: " << e.what() << std::endl;
        return false;
    }
}

bool EnhancedCSVRepository::addUser(const std::string& username, const std::string& password) {
    // Check if username already exists
    if (findByUsername(username) != nullptr) {
        return false;
    }
    
    // Create new user
    User newUser(username, password);
    
    // Add to in-memory cache
    size_t newIndex = users.size();
    users.push_back(newUser);
    usernameIndex[username] = newIndex;
    
    // Append to file for immediate persistence
    if (!appendRecord(newUser)) {
        // If file operation fails, still keep in memory but mark as dirty
        isDirty = true;
    }
    
    return true;
}

bool EnhancedCSVRepository::updateUser(const User& user) {
    std::string username = user.getUsername();
    auto it = usernameIndex.find(username);
    
    if (it == usernameIndex.end()) {
        return false;
    }
    
    // Update in-memory cache
    users[it->second] = user;
    isDirty = true;
    
    return true;
}

bool EnhancedCSVRepository::deleteUser(const std::string& username) {
    auto it = usernameIndex.find(username);
    
    if (it == usernameIndex.end()) {
        return false;
    }
    
    // Remove from in-memory cache
    size_t index = it->second;
    users.erase(users.begin() + index);
    usernameIndex.erase(it);
    
    // Update indices for all users after the deleted one
    for (auto& pair : usernameIndex) {
        if (pair.second > index) {
            pair.second--;
        }
    }
    
    isDirty = true;
    return true;
}

User* EnhancedCSVRepository::findByUsername(const std::string& username) {
    auto it = usernameIndex.find(username);
    
    if (it == usernameIndex.end()) {
        return nullptr;
    }
    
    return &users[it->second];
}

std::vector<User> EnhancedCSVRepository::getAllUsers() {
    return users;
}

void EnhancedCSVRepository::setCurrentUser(const std::string& username) {
    currentUsername = username;
    globalCurrentUser = username;
}

std::string EnhancedCSVRepository::getCurrentUser() const {
    return currentUsername;
}

std::string EnhancedCSVRepository::getCurrentUserName() {
    return globalCurrentUser;
}

bool EnhancedCSVRepository::commit() {
    if (isDirty) {
        return saveData();
    }
    return true;
}