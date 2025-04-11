// models/EnhancedCSVRepository.h
#ifndef ENHANCED_CSV_REPOSITORY_H
#define ENHANCED_CSV_REPOSITORY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <mutex>
#include "User.h"

class EnhancedCSVRepository {
private:
    // File path and state tracking
    std::string filePath;
    bool isDirty = false;
    std::mutex fileMutex;
    
    // In-memory cache
    std::vector<User> users;
    std::unordered_map<std::string, size_t> usernameIndex;
    
    // CSV operations
    bool parseCSVLine(const std::string& line, std::string& username, std::string& password);
    std::string createCSVLine(const User& user);
    
    // File operations
    void loadData();
    bool saveData();
    bool appendRecord(const User& user);
    
    // User state
    std::string currentUsername;
    
public:
    EnhancedCSVRepository() = default;
    ~EnhancedCSVRepository();
    
    // Initialization
    bool initialize(const std::string& path);
    
    // CRUD operations
    bool addUser(const std::string& username, const std::string& password);
    bool updateUser(const User& user);
    bool deleteUser(const std::string& username);
    User* findByUsername(const std::string& username);
    std::vector<User> getAllUsers();
    
    // Session management
    void setCurrentUser(const std::string& username);
    std::string getCurrentUser() const;
    
    // Persistence control
    bool commit();  // Force write to disk
    
    // Static access for global state
    static std::string getCurrentUserName();
};

#endif // ENHANCED_CSV_REPOSITORY_H