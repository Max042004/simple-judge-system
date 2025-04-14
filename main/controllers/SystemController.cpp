// controllers/SystemController.cpp
#include "SystemController.h"
#include "../tiny-server.h"
#include "../Global.h"
#include <iostream>
#include <cstdlib>

SystemController::SystemController(const std::string& userPath, const std::string& problemPath, 
                                  const std::string& msgPath, const std::string& versionStr)
    : authController(userRepo, loginView)
{
    userDataPath = userPath;
    problemDataPath = problemPath;
    loginMsgPath = msgPath;
    version = versionStr;
    status = "NOT READY";
}

void SystemController::clearWindow() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void SystemController::loadData() {
    // Initialize user repository
    userRepo.initialize(userDataPath);
    mainMenuView.displayLoadingMessage("Status - Loading user data...", false);
    mainMenuView.displayLoadingMessage("Status - Loading user data...", true);
    
    // Problem loading would go here
    mainMenuView.displayLoadingMessage("Status - Loading problem data...", false);
    mainMenuView.displayLoadingMessage("Status - Loading problem data...", true);
    
    // Display welcome message
    mainMenuView.displayWelcomeMessage(loginMsgPath);
}

int SystemController::mainPage() {
    if (status == "NOT READY") {
        loadData();
        status = "USER LOGIN";
        return 0;
    }
    
    if (status == "USER LOGIN") {
        if (authController.login()) {
            status = "READY";
            start_server();
        } else {
            return 0;
        }
    }
    
    mainMenuView.displayMainMenu();
    AcceptRequest();
    int opt = mainMenuView.getMenuSelection();
    
    switch (opt) {
        case 1: // Who am I
            mainMenuView.displayUserInfo(authController.getCurrentUser());
            break;
        
        case 2: // Query judge version
            mainMenuView.displayVersionInfo(version);
            break;
        
        // Other cases would be implemented here
        
        default:
            break;
    }
    
    return 0;
}