// views/LoginView.h
#ifndef LOGIN_VIEW_H
#define LOGIN_VIEW_H

#include <string>


class LoginView {
public:
    void showLoginPrompt();
    void showRegisterPrompt();
    void showLoginSuccess(const std::string& username);
    void showRegistrationSuccess(const std::string& username);
    void showPasswordMismatch();
    void showUserNotFound();
    void showPasswordIncorrect();
    
    std::string getUsernameInput();
    std::string getPasswordInput();
    std::string getPasswordConfirmation();
    int getLoginAttempt();
};

#endif // LOGIN_VIEW_H