// models/UserRepository.h
#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <string>
#include <vector>
#include "User.h"

class UserRepository {
private:
    std::vector<User> users;
    std::string filePath;
    std::string currentUser;
    
public:
    UserRepository() = default;
    void initialize(const std::string& path);
    
    // Data access methods
    void loadUsers();
    void saveUsers();
    User* findByUsername(const std::string& username);
    bool addUser(const std::string& username, const std::string& password);
    std::vector<User>& getAllUsers();
    
    // Current user management
    void setCurrentUser(const std::string& username);
    std::string getCurrentUser() const;
    
    // Static access for global state
    static std::string getCurrentUserName();
};

#endif // USER_REPOSITORY_H