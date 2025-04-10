#include <iostream>
#include <fstream>
#include <utility>
#include <sstream>
#include "AccountSystem.h"
#define red(text)    "\033[31m" text "\033[0m"
#define green(text)  "\033[32m" text "\033[0m"
#define yellow(text) "\033[33m" text "\033[0m"
#define blue(text)   "\033[34m" text "\033[0m"
#define magenta(text) "\033[35m" text "\033[0m"
#define cyan(text)   "\033[36m" text "\033[0m"
#define white(text)  "\033[37m" text "\033[0m"

User* AccountSystem::search(std::string name) {
    for(size_t i=0;i<user_list.size();i++) {
        if(user_list[i].getUsername() == name) return &user_list[i];
    }
    return nullptr;
}
void AccountSystem::init(std::string USER_DATA_PATH) {

    AccountSystem::USER_DATA_PATH = USER_DATA_PATH;

    try {

        std::ifstream file(USER_DATA_PATH); // 開啟檔案
        if( !file ) {
            throw std::runtime_error("Error: File does not exist - " + USER_DATA_PATH);
        }

        std::string read_line;
        while( getline(file, read_line) ) {
            std::string username, password;
            
            std::stringstream line(read_line);

            if(getline(line, username, ',') && getline(line ,password))            
                AccountSystem::adding_user(username, password);
        }
        file.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
    }
}
void AccountSystem::sign_up() {
    std::string user_name = "", password = "", confirm_pwd = "";
    int is_pwd_wrong = 0;
    std::cout << "Enter your register user name: ";
    std::cin >> user_name;
    while(1){
        if(is_pwd_wrong == 0) std::cout << "Enter your password: ";
        else std::cout << "Different password, Enter your password again: ";
        std::cin >> password;
        std::cout << "confirm your password: ";
        std::cin >> confirm_pwd;
        if(confirm_pwd == password) break;
        else is_pwd_wrong = 1;
    }
    login_user = user_name;
    adding_user(user_name, password);
    return;
}
std::pair<bool, std::string> AccountSystem::login() {
    std::pair<bool, std::string> result = std::make_pair(false, "");
    std::string user_name = "", password = "";
    User* user;
    
    while(1){
        int pwd_wrong_count = 0;
        do{
            if(user_name == "") std::cout << "Enter your User name: ";
            else std::cout << "User name unfound\nPlease Enter your User name again: ";
            std::cin >> user_name;
            if(user_name == "-1"){
                sign_up();
                std::cout << "Enter your User name: ";
                std::cin >> user_name;
            }
            user = search(user_name);
        }
        while(!user);

        std::cout << "Enter your password: ";
        std::cin >> password;
        while(password != user->getPassword()){
            std::cout << "password incorrect... please try again: ";
            std::cin >> password;
            pwd_wrong_count++;
            if(pwd_wrong_count == 3) break;
        }
        if(pwd_wrong_count == 3) continue;
        else break;
    }
    login_user = user->getUsername();
    return result = std::make_pair(true, user_name);

    return result;
}
void AccountSystem::adding_user(std::string username, std::string password) {
    User new_user(username, password);
    user_list.push_back(new_user);
    AccountSystem::userdataUpdate();
    return;
}
std::string AccountSystem::getuserLogin() {
    return AccountSystem::login_user;
}
void AccountSystem::userdataUpdate() {
    
    try {
        std::ofstream info_out(AccountSystem::USER_DATA_PATH);
        if( !info_out ) {
            throw std::runtime_error("Error: File does not exist - " + USER_DATA_PATH);
        }

        for (const User& user : user_list){
            info_out << user.getUsername() << ","
                     << user.getPassword() << "\n";
        }

        info_out.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
    }
}

std::string AccountSystem::getCurrentUserName() {
    AccountSystem account;
    return account.login_user;
}