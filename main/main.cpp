// main.cpp
#include <iostream>
#include "controllers/SystemController.h"

#define USER_DATA_PATH "./data/user/user.csv"
#define PROBLEM_DATA_PATH  "./data/problem/problem.csv"
#define LOGIN_MSG_PATH "./msg/login.txt"
#define VERSION "1.0.0"

int main() {
    #ifdef _WIN32
        system("cls");
    #else
        //system("clear");
    #endif

    SystemController system(USER_DATA_PATH, PROBLEM_DATA_PATH, LOGIN_MSG_PATH, VERSION);
    std::cout << BLUE_TEXT("Simple Judge System start! Version: ") << VERSION << "\n";

    int result = 0;
    
    do {
        result = system.mainPage();
    } while (result == 0);

    return 0;
}
