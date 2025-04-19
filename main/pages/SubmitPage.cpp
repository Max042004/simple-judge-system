#include "../tiny-server.h"
#include <string.h>
#include <stdio.h>

// Function to serve code submission page (after login)
void serve_submission_page(int client_socket, const char* username) {
    // Template for the code submission page
    const char* submission_template = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>Online Judge - Code Submission</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; }\n"
        "        .header { display: flex; justify-content: space-between; align-items: center; }\n"
        "        .welcome { font-size: 18px; }\n"
        "        textarea { width: 100%%; height: 400px; padding: 10px; border: 1px solid #ddd; border-radius: 3px; margin: 10px 0; font-family: monospace; }\n"
        "        select { padding: 8px; margin: 10px 0; }\n"
        "        button { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 3px; cursor: pointer; }\n"
        "        button:hover { background-color: #45a049; }\n"
        "        .result { margin-top: 20px; padding: 15px; border: 1px solid #ddd; border-radius: 3px; background-color: #fff; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class='header'>\n"
        "        <div class='welcome'>Welcome, %s!</div>\n"
        "        <a href='/api/logout'>Logout</a>\n"
        "    </div>\n"
        "    <h2>Submit Code</h2>\n"
        "    <form action='/api/submit' method='post'>\n"
        "        <select name='problem'>\n"
        "            <option value='1'>Problem 1: Hello World</option>\n"
        "            <option value='2'>Problem 2: Sum of Two Numbers</option>\n"
        "        </select>\n"
        "        <textarea name='code' placeholder='Write your C code here...'></textarea>\n"
        "        <button type='submit'>Submit</button>\n"
        "    </form>\n"
        "    <div class='result'>\n"
        "        <h3>Results</h3>\n"
        "        <p>Submit your code to see results.</p>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";

    char response[8192];
    sprintf(response, submission_template, username);
    
    writen(client_socket, response, strlen(response));
}