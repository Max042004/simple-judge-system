// models/User.h
#ifndef USER_H
#define USER_H

#include <string>

class User {
private:
    std::string username;
    std::string password;
    
public:
    User() = default;
    User(std::string name, std::string passwd);
    
    // Getters
    std::string getUsername() const { return username; }
    std::string getPassword() const { return password; }
};

#endif // USER_H