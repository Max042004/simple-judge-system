// controllers/AuthController.h
#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <string>
#include "../models/UserRepository.h"
#include "../views/LoginView.h"

class AuthController {
private:
    UserRepository& userRepo;
    LoginView& loginView;
    
public:
    AuthController(UserRepository& repo, LoginView& view);
    
    bool login();
    bool registerUser();
    std::string getCurrentUser() const;
};

#endif // AUTH_CONTROLLER_H