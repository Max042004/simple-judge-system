#include <arpa/inet.h> /* inet_ntoa */
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include "pages/LoginPage.h"
#include "pages/SignupPage.h"
#include "controllers/SystemController.h"

#define LISTENQ  1024  /* second argument to listen() */
#define MAXLINE 1024   /* max length of a line */
#define RIO_BUFSIZE 1024

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 9998 /* use this port if none given as arg to main() */
#endif

#ifndef NO_LOG_ACCESS
#define LOG_ACCESS
#endif

// fd set
static fd_set readfds;
int serverfd;

extern SystemController* globalSystemController;

typedef struct {
    int rio_fd;                 /* descriptor for this buf */
    int rio_cnt;                /* unread byte in this buf */
    char *rio_bufptr;           /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE];  /* internal buffer */
} rio_t;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

typedef struct {
    char file_name[512];
    off_t offset;              /* for support Range */
    size_t end;
    char method[8];            /* GET or POST */
    char request_path[512];    /* Full request path */
    char query_string[512];    /* Query parameters */
    char post_data[4096];      /* POST data */
    int  content_length;       /* Content-Length for POST requests */
} http_request;

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;

//https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types

mime_map mime_types[] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".txt", "text/plain"},
    {".png", "image/png"},
    {".ico", "image/vnd.microsoft.icon"},
    {NULL, NULL}
};

const char* default_mime_type = "text/plain";

void rio_readinitb(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

ssize_t writen(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char*)usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) {  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            }
            else{
                return -1;       /* errorno set by write() */
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, int n) {
    int cnt;
    while (rp->rio_cnt <= 0) {  /* refill if buf is empty */

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) { /* interrupted by sig handler return */
                return -1;
            }
        }
        else if (rp->rio_cnt == 0) {  /* EOF */
            return 0;
        }
        else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n) {
        cnt = rp->rio_cnt;
    }
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    size_t n, rc;
    char c, *bufp = (char*)usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0; /* EOF, no data read */
            }
            else{
                break;    /* EOF, some data was read */
            }
        } else{
            return -1;    /* error */
        }
    }
    *bufp = 0;
    return n;
}

void format_size(char* buf, struct stat *stat) {
    if(S_ISDIR(stat->st_mode)) {
        sprintf(buf, "%s", "[DIR]");
    } else {
        off_t size = stat->st_size;
        if(size < 1024) {
            sprintf(buf, "%lu", size);
        } else if (size < 1024 * 1024) {
            sprintf(buf, "%.1fK", (double)size / 1024);
        } else if (size < 1024 * 1024 * 1024) {
            sprintf(buf, "%.1fM", (double)size / 1024 / 1024);
        } else {
            sprintf(buf, "%.1fG", (double)size / 1024 / 1024 / 1024);
        }
    }
}

static const char* get_mime_type(char *filename) {
    char *dot = strrchr(filename, '.');
    if(dot) { // strrchar Locate last occurrence of character in string
        mime_map *map = mime_types;
        while(map->extension) {
            if(strcmp(map->extension, dot) == 0) {
                return map->mime_type;
            }
            map++;
        }
        fprintf(stderr, "Warning: Unknown file type: %s\n", dot);
    }
    return default_mime_type;
}

int open_serverfd(int port) {
    int optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0) {
        return -1;
    }

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s
    if (setsockopt(serverfd, 6, TCP_CORK,
                   (const void *)&optval , sizeof(int)) < 0) {
        return -1;
    }

    /* serverfd will be an endpoint for all requests to port
       on any IP address for this host */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(serverfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) {
        return -1;
    }

    /* Make it a listening socket ready to accept connection requests */
    if (listen(serverfd, LISTENQ) < 0) {
        return -1;
    }
    return serverfd;
}

void url_decode(char* src, char* dest, int max) {
    char *p = src;
    char code[3] = { 0 };
    while(*p && --max) {
        if(*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else if (*p == '+') {
            *dest++ = ' ';
            p++;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

void parse_query_string(char* query_string, char* key, char* value, int max_len) {
    char* p = query_string;
    char* key_start = query_string;
    char* value_start = NULL;
    
    while (*p) {
        if (*p == '=') {
            *p = '\0';
            value_start = p + 1;
        } else if (*p == '&') {
            *p = '\0';
            if (strcmp(key_start, key) == 0 && value_start) {
                strncpy(value, value_start, max_len);
                return;
            }
            key_start = p + 1;
            value_start = NULL;
        }
        p++;
    }
    
    if (strcmp(key_start, key) == 0 && value_start) {
        strncpy(value, value_start, max_len);
    }
}

void parse_request(int fd, http_request *req) {
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char* query_string_ptr;
   
    // Initialize request
    req->offset = 0;
    req->end = 0;
    req->content_length = 0;
    strcpy(req->post_data, "");
    strcpy(req->query_string, "");
    
    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
   
    // Store method
    strcpy(req->method, method);
    
    // Store full request path
    strcpy(req->request_path, uri);
    
    // Check for query string
    query_string_ptr = strchr(uri, '?');
    if (query_string_ptr) {
        *query_string_ptr = '\0';
        strcpy(req->query_string, query_string_ptr + 1);
    }
    
    /* read all headers */
    while(buf[0] != '\n' && buf[1] != '\n') { /* \n || \r\n */
        rio_readlineb(&rio, buf, MAXLINE);
        if(buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'n') {
            sscanf(buf, "Range: bytes=%lu-%lu", &req->offset, &req->end);
            // Range: [start, end]
            if(req->end != 0) {req->end++;}
        } else if (strncasecmp(buf, "Content-Length:", 15) == 0) {
            sscanf(buf + 15, "%d", &req->content_length);
        }
    }

    // Read POST data if present
    if (strcasecmp(method, "POST") == 0 && req->content_length > 0) {
        int bytes_read = 0;
        while (bytes_read < req->content_length) {
            int len = rio_read(&rio, req->post_data + bytes_read, 
                              req->content_length - bytes_read);
            if (len <= 0) break;
            bytes_read += len;
        }
        req->post_data[bytes_read] = '\0';
    }

    char* filename = uri;
    if(uri[0] == '/') {
        filename = uri + 1;
        int length = strlen(filename);
        if (length == 0) {
            filename = (char*)".";
        } else {
            for (int i=0; i < length; ++i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }
    url_decode(filename, req->file_name, MAXLINE);
}

void client_error(int fd, int status, const char *msg, const char *longmsg) {
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(buf + strlen(buf),
            "Content-length: %lu\r\n\r\n", strlen(longmsg));
    sprintf(buf + strlen(buf), "%s", longmsg);
    writen(fd, buf, strlen(buf));
}

void serve_static(int out_fd, int in_fd, http_request *req,
                  size_t total_size) {
    char buf[256];
    if (req->offset > 0) {
        sprintf(buf, "HTTP/1.1 206 Partial\r\n");
        sprintf(buf + strlen(buf), "Content-Range: bytes %lu-%lu/%lu\r\n",
                req->offset, req->end, total_size);
    } else {
        sprintf(buf, "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\n");
    }
    sprintf(buf + strlen(buf), "Cache-Control: no-cache\r\n");
    sprintf(buf + strlen(buf), "Content-length: %lu\r\n",
            req->end - req->offset);
    sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n",
            get_mime_type(req->file_name));

    writen(out_fd, buf, strlen(buf));
    // Copy file content
    off_t offset = req->offset;
    size_t remaining = req->end - req->offset;
    while (remaining > 0) {
        ssize_t sent = sendfile(out_fd, in_fd, &offset, remaining);
        if (sent <= 0) {
            perror("sendfile");
            break;
        }
        remaining -= static_cast<size_t>(sent);
#ifdef LOG_ACCESS
        printf("sendfile(): %zd bytes, %zu remaining\n", sent, remaining);
#endif
    }
}

// New function to handle API requests
void handle_request(int fd, http_request *req) {
    // API endpoint for signup
    if (strncmp(req->request_path, "/api/signup", 11) == 0) {
        if (strcasecmp(req->method, "GET") == 0) {
            char buf[MAXLINE];
            // If user is not logged in, show login page
            if(!globalSystemController->getUserRepo().getIsLogin()){
                serve_signup_page(fd);
            }
            else{
                // 303 redirect
                snprintf(buf, sizeof(buf),
                "HTTP/1.1 303 See Other\r\n"
                "Location: /api/submission\r\n"
                "\r\n");
                writen(fd, buf, strlen(buf));
            }
        } else {
            client_error(fd, 405, "Method Not Allowed", "Only GET requests are allowed for login");
        }
    }
    // API endpoint for user signup
else if (strncmp(req->request_path, "/api/user_signup", 16) == 0) {
    if (strcasecmp(req->method, "POST") == 0) {
        char buf[MAXLINE * 2];  // 回應用 buffer
        char username[256] = {0}, password[256] = {0};
        char *username_str = strstr(req->post_data, "username=");
        char *password_str = strstr(req->post_data, "password=");
        
        if (username_str && password_str) {
            // 解析 username
            username_str += strlen("username=");
            char *end = strchr(username_str, '&');
            if (end) *end = '\0';
            url_decode(username_str, username, sizeof(username));
            
            // 解析 password
            password_str += strlen("password=");
            end = strchr(password_str, '&');
            if (end) *end = '\0';
            url_decode(password_str, password, sizeof(password));
            
            // 密碼確認（可根據需求加上更嚴格檢查）
            if (strlen(password) < 6) {
                // 密碼太短
                int len = snprintf(buf, sizeof(buf),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "\r\n"
                    "<!DOCTYPE html>"
                    "<html><head><meta charset=\"utf-8\"></head><body>"
                    "<script>"
                    "alert('Password must be at least 6 characters');"
                    "window.location.href = '/api/signup';"
                    "</script>"
                    "</body></html>");
                writen(fd, buf, len);
                return;
            }
            
            // 呼叫你的註冊 API，回傳 true 表示註冊成功
            if (globalSystemController->getAuthController().registerUserAPI(username, password)) {
                // 順便在 session or repo 設定當前使用者（可選）
                globalSystemController->getUserRepo().setCurrentUser(username);
                
                // 303 重導到登入頁，或直接導到登入後能看到的頁面
                snprintf(buf, sizeof(buf),
                    "HTTP/1.1 303 See Other\r\n"
                    "Location: /api/login\r\n"
                    "\r\n");
                writen(fd, buf, strlen(buf));
                return;
            } else {
                // 帳號已存在或其他錯誤
                int len = snprintf(buf, sizeof(buf),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "\r\n"
                    "<!DOCTYPE html>"
                    "<html><head><meta charset=\"utf-8\"></head><body>"
                    "<script>"
                    "alert('Username already taken or signup failed');"
                    "window.location.href = '/api/signup';"
                    "</script>"
                    "</body></html>");
                writen(fd, buf, len);
                return;
            }
        } else {
            // 缺少參數
            int len = snprintf(buf, sizeof(buf),
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: application/json\r\n"
                "\r\n"
                "{\"success\":false,\"message\":\"Missing username or password\"}");
            writen(fd, buf, len);
            return;
        }
    } else {
        // 非 POST 拒絕
        client_error(fd, 405, "Method Not Allowed",
                     "Only POST is allowed for /api/user_signup");
        return;
    }
}
    // API endpoint for login
    if (strncmp(req->request_path, "/api/login", 10) == 0) {
        if (strcasecmp(req->method, "GET") == 0) {
            char buf[MAXLINE];
            // If user is not logged in, show login page
            if(!globalSystemController->getUserRepo().getIsLogin()){
                serve_login_page(fd);
            }
            else{
                // 303 redirect
                snprintf(buf, sizeof(buf),
                "HTTP/1.1 303 See Other\r\n"
                "Location: /api/submission\r\n"
                "\r\n");
                writen(fd, buf, strlen(buf));
            }
        } else {
            client_error(fd, 405, "Method Not Allowed", "Only GET requests are allowed for login");
        }
    }
    // API endpoint for login
else if (strncmp(req->request_path, "/api/user_login", 15) == 0) {

    if (strcasecmp(req->method, "POST") == 0) {
        size_t len = 0; //track written bytes in buf
        char response[MAXLINE];
        char buf[MAXLINE*2]; // bigger than response to avoid buffer overflow
        char username[256], password[256];
        // Parse POST data for username and password
        // This is a very simple parser for "username=foo&password=bar" format
        char *username_str = strstr(req->post_data, "username=");
        char *password_str = strstr(req->post_data, "password=");
        
        if (username_str && password_str) {
            // Extract username
            username_str += 9; // Skip "username="
            char *end = strchr(username_str, '&');
            if (end) *end = '\0';
            url_decode(username_str, username, 256);
            printf("%s\n", username);
            
            // Extract password
            password_str += 9; // Skip "password="
            end = strchr(password_str, '&');
            if (end) *end = '\0';
            url_decode(password_str, password, 256);
            
            // validate
            if(globalSystemController->getAuthController().loginUserAPI(username, password)){
                globalSystemController->getUserRepo().setCurrentUser(username);
                // 303 redirect
                snprintf(buf, sizeof(buf),
                "HTTP/1.1 303 See Other\r\n"
                "Location: /api/submission\r\n"
                "\r\n");
                writen(fd, buf, strlen(buf));
                
                // 
                return;
            }
            else {
                // send wrong password or user not found
                const char *msg = "Invalid username or password";
                len = snprintf(buf, sizeof(buf),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "\r\n"
                    "<!DOCTYPE html>"
                    "<html><head><meta charset=\"utf-8\"></head><body>"
                    "<script>"
                    "alert('%s');"
                    "window.location.href = '/api/login';"
                    "</script>"
                    "</body></html>",
                    msg);
                writen(fd, buf, len);
            }
        } else {
            snprintf(response, sizeof(response),
                     "{\"success\": false, \"message\": \"Missing username or password\"}");
        }
    } else {
        printf("%s\n", req->method);
        client_error(fd, 405, "Method Not Allowed", "Only POST requests are allowed for check_login");
    }
}
// API endpoint for getting login user name
else if (strncmp(req->request_path, "/api/logout", 11) == 0) {
    if (strcasecmp(req->method, "GET") == 0) {
        char buf[MAXLINE];
        if(globalSystemController->getAuthController().logoutAPI()){
            // 303 redirect
            snprintf(buf, sizeof(buf),
            "HTTP/1.1 303 See Other\r\n"
            "Location: /api/login\r\n"
            "\r\n");
            writen(fd, buf, strlen(buf));
        }
        else{ 
            size_t len = 0;
            const char *msg = "Only login user is allowed to logout";
            len = snprintf(buf, sizeof(buf),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "\r\n"
                "<!DOCTYPE html>"
                "<html><head><meta charset=\"utf-8\"></head><body>"
                "<script>"
                "alert('%s');"
                "window.location.href = '/api/login';"
                "</script>"
                "</body></html>",
                msg);
            writen(fd, buf, len);
        }
    } else {
        client_error(fd, 405, "Method Not Allowed", "Only GET requests are allowed for logout");
    }
}

    // API endpoint for submission
else if (strncmp(req->request_path, "/api/submission", 15) == 0) {
    if (strcasecmp(req->method, "GET") == 0) {
        char buf[MAXLINE];
        size_t len = 0;
        if (globalSystemController->getUserRepo().getIsLogin()){
            serve_submission_page(fd, globalSystemController->getAuthController().getCurrentUser().c_str());
        } else {
            // 303 redirect
            snprintf(buf, sizeof(buf),
                     "HTTP/1.1 303 See Other\r\n"
                     "Location: /api/login\r\n"
                     "\r\n");
            writen(fd, buf, strlen(buf));
        }
    } else {
        printf("%s\n", req->method);
        client_error(fd, 405, "Method Not Allowed", "Only GET or POST are allowed for submission");
    }
}
// API endpoint for submission
else if (strncmp(req->request_path, "/api/submit", 11) == 0) {
    char response[MAXLINE*8];
    char buf[MAXLINE*9];
    if (strcasecmp(req->method, "POST") == 0) {
        // Check if user is logged in
        if (!globalSystemController->getUserRepo().getIsLogin()) {
            // Redirect to login page if not logged in
            snprintf(buf, sizeof(buf),
                "HTTP/1.1 303 See Other\r\n"
                "Location: /api/login\r\n"
                "\r\n");
            writen(fd, buf, strlen(buf));
            return;
        }
        
        // Parse the form data to extract problem ID and code
        char problem_id[10] = {0};
        char code[4096] = {0};
        char *problem_str = strstr(req->post_data, "problem=");
        char *code_str = strstr(req->post_data, "code=");
        
        if (problem_str && code_str) {
            // Extract problem ID
            problem_str += 8; // Skip "problem="
            char *end = strchr(problem_str, '&');
            if (end) *end = '\0';
            url_decode(problem_str, problem_id, sizeof(problem_id));
            
            // Extract code
            code_str += 5; // Skip "code="
            url_decode(code_str, code, sizeof(code));
            
            // Get current username
            std::string username = globalSystemController->getAuthController().getCurrentUser();

            // 1. 把 submission code 儲存到臨時檔
            char src_file[256];
            snprintf(src_file, sizeof(src_file),
                    "/tmp/sub_%d_%ld.c", getpid(), time(NULL));

            FILE* fp = fopen(src_file, "w");
            fputs(code, fp);      // code = 使用者 POST 上來的程式碼
            fclose(fp);

            // 2. Run sandbox, mounting the whole directory
            char cmd[1024];
            snprintf(cmd, sizeof(cmd),
                    "unshare --user --map-root-user --mount --pid --fork --mount-proc=/proc "
                    "bash -c '"
                        "set -e; "
                        "TMPROOT=$(mktemp -d); chmod 700 \"$TMPROOT\"; "
                        "mount -t tmpfs -o size=128M tmpfs \"$TMPROOT\"; "
                        "cp %s \"$TMPROOT/sub.c\"; cd \"$TMPROOT\"; "
                        "ulimit -t 2 -v $((64*1024)) -f $((1*1024)); "
                        "gcc -O2 -static sub.c -o prog; "
                        "./prog"
                    "' 2>&1",
                    src_file);

            // 用 popen 以讀取模式打開
            FILE* pipe = popen(cmd, "r");
            if (!pipe) {
                client_error(fd, 500, "Internal Server Error",
                            "Failed to launch Docker compiler");
                return;
            }

            // 讀取 docker run 的 stdout（包含編譯與執行輸出）
            char output[1024];
            size_t pos = 0;
            while (fgets(output + pos, sizeof(output) - pos, pipe) != nullptr) {
                pos = strlen(output);
                if (pos + 1 >= sizeof(output)) break;
            }

            // 關閉管道，並拿到 exit code
            int status = pclose(pipe);
            int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

            // … Clean up submission file if needed …

            // 準備 HTML 回應
            const char* result_template =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<html><body>"
                "<h2>Results for Problem %s</h2>"
                "<pre>%s</pre>"
                "<p style='color:%s'>Status: %s (code %d)</p>"
                "<a href='/api/submission'>Try again</a>"
                "</body></html>";

            const char* status_class = exit_code == 0 ? "green" : "red";
            const char* status_text  = exit_code == 0 
                ? "Executed Successfully" 
                : "Execution Failed";

            char response[16384];
            int n = snprintf(response, sizeof(response),
                result_template,
                problem_id,
                output,
                status_class,
                status_text,
                exit_code);

            writen(fd, response, n);
        } else {
            client_error(fd, 400, "Bad Request", "Missing problem ID or code");
        }
    } else {
        client_error(fd, 405, "Method Not Allowed", "Only POST requests are allowed for submitting code");
    }
}
    // API endpoint for getting login user name
    else if (strncmp(req->request_path, "/api/opt/1", 10) == 0) {
        char response[MAXLINE];
        char buf[MAXLINE*2];
        if (strcasecmp(req->method, "GET") == 0) {
            // Call the judge system function to get version
            std::string username = globalSystemController->getAuthController().getCurrentUser();
            const char* name = username.c_str();

            snprintf(response, sizeof(response), "<p>Hello %s</p>", name);

            snprintf(buf, sizeof(buf),
                "HTTP/1.1 200 OK\r\n"
                "Content-length: %lu\r\n"
                "Content-type: text/html\r\n\r\n"
                "%s", strlen(response), response);
            writen(fd, buf, strlen(buf));
        } else {
            client_error(fd, 405, "Method Not Allowed", "Only GET requests are allowed for getting username");
        }
    }
    // Unknown API endpoint
    else {
        client_error(fd, 404, "Not Found", "API endpoint not found");
    }
}

void process(int fd, struct sockaddr_in *clientaddr) {
    http_request req;
    parse_request(fd, &req);

#ifdef LOG_ACCESS
    printf("Connection from %s - Request: %s %s\n", 
            inet_ntoa(clientaddr->sin_addr), 
            req.method, 
            req.request_path);
#endif

    // Handle API requests
    if (strncmp(req.request_path, "/api/", 5) == 0) {
        handle_request(fd, &req);
        return;
    }

    // client only allow to acccess main/pages .html files
    if (strncmp(req.file_name, "main/pages/", 11) != 0 || 
        strstr(req.file_name, "..") != NULL || 
        strstr(req.file_name, "/") == NULL || 
        strlen(req.file_name) < 6 || 
        strcmp(req.file_name + strlen(req.file_name) - 5, ".html") != 0) {
        
        client_error(fd, 403, "Forbidden", "You are not allowed to access this file.");
        return;
    }

    // Open the file
    struct stat sbuf;
    int status = stat(req.file_name, &sbuf);
    if (status < 0) {
        client_error(fd, 404, "Not found", "File not found");
        return;
    }

    // Handle regular file
    int file_fd = open(req.file_name, O_RDONLY);
    if (file_fd < 0) {
        client_error(fd, 500, "Internal Server Error", "Failed to open file");
        return;
    }

    // Set file size and range
    size_t file_size = sbuf.st_size;
    if (req.end == 0) {
        req.end = file_size;
    }

    // Serve the file content
    serve_static(fd, file_fd, &req, file_size);
    close(file_fd);
}

int start_server() {
    int default_port = DEFAULT_PORT;
    FD_ZERO(&readfds);
    
    // Ignore SIGPIPE signal to prevent server crash when a client disconnects
    signal(SIGPIPE, SIG_IGN);
    
    // Create and configure the listening socket
    serverfd = open_serverfd(default_port);
    if (serverfd < 0) {
        perror("Failed to initialize listening socket");
        exit(1);
    }
    int server_flags = fcntl(serverfd, F_GETFL);
    fcntl(serverfd, F_SETFL, server_flags | O_NONBLOCK);

    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);
    
    printf("Server listening on port %d\n", default_port);
    
    return 0;
}

void AcceptRequest(){
    int connfd;

    FD_SET(serverfd, &readfds);
    FD_SET(STDIN_FILENO, &readfds);
    int rv = select(serverfd+1, &readfds, NULL, NULL, NULL);
    if (rv < 0) {
        perror("select");
        return;
    }

    if (serverfd > 0 && FD_ISSET(serverfd, &readfds)) {
        FD_CLR(serverfd, &readfds);
        struct sockaddr_in clientaddr;
        socklen_t clientlen = sizeof(clientaddr);
        connfd = accept(serverfd, (SA *)&clientaddr, &clientlen);
        if (connfd >= 0) {
            process(connfd, &clientaddr);
            close(connfd);
        }
    }
    FD_CLR(STDIN_FILENO, &readfds);
}