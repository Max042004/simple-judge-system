// views/MainMenuView.cpp
#include "MainMenuView.h"
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <regex>
#include <unistd.h>

void MainMenuView::displayMainMenu() {
    std::cout << "+";
    for (int i = 0; i < 45; ++i) {
        std::cout << "-";
    }
    std::cout << "+\n";
    std::cout << YELLOW_TEXT("Please choose an operation:") << "\n";
    std::cout << "+";
    for (int i = 0; i < 45; ++i) {
        std::cout << "-";
    }
    std::cout << "+\n";

    std::cout << GREEN_TEXT("(1) Who am I") << "\n";
    std::cout << GREEN_TEXT("(2) Query judge version") << "\n";
    std::cout << GREEN_TEXT("(3) List all problem") << "\n";
    std::cout << GREEN_TEXT("(4) Random some problem") << "\n";
    std::cout << GREEN_TEXT("(5) Submit code") << "\n";
    std::cout << RED_TEXT("(6) Add a new problem (admin only)") << "\n";
    std::cout << YELLOW_TEXT("(7) Exit the process") << "\n";
    
    std::cout << "+";
    for (int i = 0; i < 45; ++i) {
        std::cout << "-";
    }
    std::cout << "+\n";
}

int MainMenuView::getMenuSelection() {
    char input;
    int nread = read(STDIN_FILENO, &input, 100);
    if(nread <= 0) return 0;
    static const std::regex pattern("^[1-7]$");
    std::string s(1, input);
    if(std::regex_match(s, pattern)) return (input - 48);
    else return -1;
}

void MainMenuView::displayUserInfo(const std::string& username) {
    std::cout << "Current user: " << BLUE_TEXT(username) << "\n";
}

void MainMenuView::displayVersionInfo(const std::string& version) {
    std::cout << "System version: " << CYAN_TEXT(version) << "\n";
}

void MainMenuView::displayLoadingMessage(const std::string& content, bool completed) {
    static const char* spinner[] = {"|", "/", "-", "\\"};
    
    if (!completed) {
        for (int i = 0, j = 0; j < 20; j++) { 
            std::cout << YELLOW_TEXT(content) << spinner[i] << "\r";
            std::flush(std::cout);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            i = (i + 1) % 4;
        }
    } else {
        std::cout << YELLOW_TEXT(content) << GREEN_TEXT("OK!\n");
    }
}

void MainMenuView::displayWelcomeMessage(const std::string& filepath) {
    try {
        std::ifstream inputFile(filepath);
        if (!inputFile) {
            throw std::runtime_error("Error: File does not exist - " + filepath);
        }

        std::string line;
        std::cout << CYAN_TEXT("");
        while (std::getline(inputFile, line)) {
            std::cout << line << "\n";
        }
        std::cout << "\033[0m";
        inputFile.close(); 
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
    }
}