// views/LoginView.cpp
#include "LoginView.h"
#include "MainMenuView.h"
#include <iostream>

void LoginView::showLoginPrompt() {
    std::cout << "Enter your User name: ";
}

void LoginView::showRegisterPrompt() {
    std::cout << "Enter your register user name: ";
}

void LoginView::showLoginSuccess(const std::string& username) {
    std::cout << GREEN_TEXT("Login successful! Welcome, ") << username << "!\n";
}

void LoginView::showRegistrationSuccess(const std::string& username) {
    std::cout << username << GREEN_TEXT("Registration successful!\n");
}

void LoginView::showPasswordMismatch() {
    std::cout << RED_TEXT("Different password. Enter your password again: ");
}

void LoginView::showUserNotFound() {
    std::cout << RED_TEXT("User name not found\nPlease Enter your User name again: ");
}

void LoginView::showPasswordIncorrect() {
    std::cout << RED_TEXT("Password incorrect... please try again: ");
}

std::string LoginView::getUsernameInput() {
    std::string username;
    std::cin >> username;
    return username;
}

std::string LoginView::getPasswordInput() {
    std::cout << "Enter your password: ";
    std::string password;
    std::cin >> password;
    return password;
}

std::string LoginView::getPasswordConfirmation() {
    std::cout << "Confirm your password: ";
    std::string confirmation;
    std::cin >> confirmation;
    return confirmation;
}

int LoginView::getLoginAttempt() {
    return -1; // Return -1 to register, other value to retry login
}