// Serve the sign‑up page (GET /signup)
void serve_signup_page(int client_socket) {
    const char* signup_template =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <title>Online Judge - Sign Up</title>\n"
        "  <style>"
        "       body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f4; }\n"
        "       .container { width: 300px; margin: 100px auto; padding: 20px; background-color: #fff; border-radius: 5px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }\n"
        "       h2 { text-align: center; color: #333; }\n"
        "       input[type='text'], input[type='password'] { width: 100%%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 3px; box-sizing: border-box; }\n"
        "       input[type='submit'] { width: 100%%; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 3px; cursor: pointer; }\n"
        "       input[type='submit']:hover { background-color: #45a049; }\n"
        "       .error { color: red; text-align: center; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class='container'>\n"
        "    <h2>Sign Up</h2>\n"
        "    <form action='/api/user_signup' method='post'>\n"
        "      <input type='text' name='username' placeholder='Choose a username' required>\n"
        "      <input type='password' name='password' placeholder='Choose a password' required>\n"
        "      <input type='password' name='confirm' placeholder='Confirm password' required>\n"
        "      <input type='submit' value='Register'>\n"
        "    </form>\n"
        "    <p><a href='/login'>Already have an account? Login</a></p>\n"
        "    <p class='error'>%s</p>\n"
        "  </div>\n"
        "</body>\n"
        "</html>";

    char response[4096];
    sprintf(response, signup_template, "");
    writen(client_socket, response, strlen(response));
}