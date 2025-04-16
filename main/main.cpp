// main.cpp
#include <iostream>
#include "controllers/SystemController.h"

#define USER_DATA_PATH "./data/user/user.csv"
#define PROBLEM_DATA_PATH  "./data/problem/problem.csv"
#define LOGIN_MSG_PATH "./msg/login.txt"
#define VERSION "1.0.0"

SystemController* globalSystemController = nullptr;

int main() {
    #ifdef _WIN32
        system("cls");
    #else
        //system("clear");
    #endif

    globalSystemController = new SystemController(USER_DATA_PATH, PROBLEM_DATA_PATH, LOGIN_MSG_PATH, VERSION);
    std::cout << BLUE_TEXT("Simple Judge System start! Version: ") << VERSION << "\n";

    int result = 0;
    
    do {
        result = globalSystemController->mainPage();
    } while (result == 0);

    delete globalSystemController;
    return 0;
}
