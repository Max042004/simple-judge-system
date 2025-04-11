// controllers/SystemController.h
#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include "../models/UserRepository.h"
#include "../views/LoginView.h"
#include "../views/MainMenuView.h"
#include "AuthController.h"

class SystemController {
private:
    UserRepository userRepo;
    LoginView loginView;
    MainMenuView mainMenuView;
    AuthController authController;
    
    std::string userDataPath;
    std::string problemDataPath;
    std::string loginMsgPath;
    std::string version;
    std::string status;
    
    void loadData();
    void clearWindow();
    
public:
    SystemController(const std::string& userPath, const std::string& problemPath, 
                    const std::string& msgPath, const std::string& versionStr);
    
    int mainPage();
    std::string getUserPath() const { return userDataPath; }
    std::string getProblemPath() const { return problemDataPath; }
    std::string getMsgPath() const { return loginMsgPath; }
    std::string getVersion() const { return version; }
};

#endif // SYSTEM_CONTROLLER_H