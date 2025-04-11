// controllers/AuthController.cpp
#include "AuthController.h"

AuthController::AuthController(UserRepository& repo, LoginView& view)
    : userRepo(repo), loginView(view) {
}

bool AuthController::login() {
    std::string username, password;
    User* user = nullptr;
    int passwordWrongCount = 1;
    
    while (true) {
        // Get username
        loginView.showLoginPrompt();
        username = loginView.getUsernameInput();
        
        // Check for registration request
        if (username == "-1") {
            if (registerUser()) {
                continue;
            }
        }
        
        // Find user
        user = userRepo.findByUsername(username);
        if (!user) {
            loginView.showUserNotFound();
            continue;
        }
        
        // Get password
        password = loginView.getPasswordInput();
        
        // Validate password
        while (password != user->getPassword()) {
            loginView.showPasswordIncorrect();
            password = loginView.getUsernameInput(); // Using this as getPasswordInput without prompt
            passwordWrongCount++;
            if (passwordWrongCount == 3) break;
        }
        
        if (passwordWrongCount == 3) {
            continue;
        }
        
        // Login successful
        userRepo.setCurrentUser(user->getUsername());
        loginView.showLoginSuccess(user->getUsername());
        return true;
    }
}

bool AuthController::registerUser() {
    std::string username, password, confirmPassword;
    int isPasswordWrong = 0;
    
    // Get username
    loginView.showRegisterPrompt();
    username = loginView.getUsernameInput();
    
    // Get and confirm password
    while (true) {
        if (isPasswordWrong == 0) {
            password = loginView.getPasswordInput();
        } else {
            loginView.showPasswordMismatch();
            password = loginView.getUsernameInput(); // Using this as getPasswordInput without prompt
        }
        
        confirmPassword = loginView.getPasswordConfirmation();
        
        if (confirmPassword == password) break;
        isPasswordWrong = 1;
    }
    
    // Add user and login
    if (userRepo.addUser(username, password)) {
        userRepo.setCurrentUser(username);
        loginView.showRegistrationSuccess(username);
        return true;
    }
    
    return false;
}

std::string AuthController::getCurrentUser() const {
    return userRepo.getCurrentUser();
}