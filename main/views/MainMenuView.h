// views/MainMenuView.h
#ifndef MAIN_MENU_VIEW_H
#define MAIN_MENU_VIEW_H

#include <string>
// Color definitions
#define RED_TEXT(text)     (std::string("\033[31m") + (text) + std::string("\033[0m"))
#define GREEN_TEXT(text)   (std::string("\033[32m") + (text) + std::string("\033[0m"))
#define YELLOW_TEXT(text)  (std::string("\033[33m") + (text) + std::string("\033[0m"))
#define BLUE_TEXT(text)    (std::string("\033[34m") + (text) + std::string("\033[0m"))
#define MAGENTA_TEXT(text) (std::string("\033[35m") + (text) + std::string("\033[0m"))
#define CYAN_TEXT(text)    (std::string("\033[36m") + (text) + std::string("\033[0m"))
#define WHITE_TEXT(text)   (std::string("\033[37m") + (text) + std::string("\033[0m"))

class MainMenuView {
public:
    void displayMainMenu();
    int getMenuSelection();
    void displayUserInfo(const std::string& username);
    void displayVersionInfo(const std::string& version);
    void displayLoadingMessage(const std::string& content, bool completed);
    void displayWelcomeMessage(const std::string& filepath);
};

#endif // MAIN_MENU_VIEW_H