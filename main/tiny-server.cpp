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

#define LISTENQ  1024  /* second argument to listen() */
#define MAXLINE 1024   /* max length of a line */
#define RIO_BUFSIZE 1024

#define USER_DATA_PATH "./data/user/user.csv"
#define PROBLEM_DATA_PATH  "./data/problem/problem.csv"
#define LOGIN_MSG_PATH "./msg/login.txt"
#define VERSION "1.0.0"

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 9999 /* use this port if none given as arg to main() */
#endif

#ifndef FORK_COUNT
#define FORK_COUNT 4
#endif

#ifndef NO_LOG_ACCESS
#define LOG_ACCESS
#endif

static fd_set readfds;
int serverfd;

#include "controllers/SystemController.h"
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
    char filename[512];
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

mime_map meme_types [] = {
    {".aac",  "audio/aac"},
    {".abw",  "application/x-abiword"},
    {".arc",  "application/x-freearc"},
    {".avi",  "video/x-msvideo"},
    {".azw",  "application/vnd.amazon.ebook"},
    {".bin",  "application/octet-stream"},
    {".bmp",  "image/bmp"},
    {".bz",   "application/x-bzip"},
    {".bz2",  "application/x-bzip2"},
    {".csh",  "application/x-csh"},
    {".css",  "text/css"},
    {".csv",  "text/csv"},
    {".doc",  "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot",  "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gz",   "application/gzip"},
    {".gif",  "image/gif"},
    {".htm",  "text/html"},
    {".html", "text/html"},
    {".ico",  "image/vnd.microsoft.icon"},
    {".ics",  "text/calendar"},
    {".jar",  "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg",  "image/jpeg"},
    {".js",   "text/javascript"},
    {".json",  "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid",   "audio/midi audio/x-midi"},
    {".midi",  "audio/midi audio/x-midi"},
    {".mjs",   "text/javascript"},
    {".mp3",   "audio/mpeg"},
    {".mp4",   "video/mp4"},
    {".mpeg",  "video/mpeg"},
    {".mpkg",  "application/vnd.apple.installer+xml"},
    {".odp",   "application/vnd.oasis.opendocument.presentation"},
    {".ods",   "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt",   "application/vnd.oasis.opendocument.text"},
    {".oga",   "audio/ogg"},
    {".ogv",   "video/ogg"},
    {".ogx",   "application/ogg"},
    {".opus",  "audio/opus"},
    {".otf",   "font/otf"},
    {".png",   "image/png"},
    {".pdf",   "application/pdf"},
    {".php",   "application/x-httpd-php"},
    {".ppt",   "application/vnd.ms-powerpoint"},
    {".pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar",   "application/vnd.rar"},
    {".rtf",   "application/rtf"},
    {".sh",    "application/x-sh"},
    {".svg",   "image/svg+xml"},
    {".swf",   "application/x-shockwave-flash"},
    {".tar",   "application/x-tar"},
    {".tif",   "image/tiff"},
    {".tiff",  "image/tiff"},
    {".ts",    "video/mp2t"},
    {".ttf",   "font/ttf"},
    {".txt",   "text/plain"},
    {".vsd",   "application/vnd.visio"},
    {".wav",   "audio/wav"},
    {".weba",  "audio/webm"},
    {".webm",  "video/webm"},
    {".webp",  "image/webp"},
    {".woff",  "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls",   "application/vnd.ms-excel"},
    {".xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml",   "text/xml"},
    {".xul",   "application/vnd.mozilla.xul+xml"},
    {".zip",   "application/zip"},
    {".3gp",   "video/3gpp"},
    {".3g2",   "video/3gpp2"},
    {".7z",    "application/x-7z-compressed"},
    {NULL, NULL},
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
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
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
    int n, rc;
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

void handle_directory_request(int out_fd, int dir_fd, char *filename) {
    char buf[MAXLINE], m_time[32], size[16];
    struct stat statbuf;
    sprintf(buf, "HTTP/1.1 200 OK\r\n%s%s%s%s%s",
            "Content-Type: text/html\r\n\r\n",
            "<html><head><style>",
            "body{font-family: monospace; font-size: 13px;}",
            "td {padding: 1.5px 6px;}",
            "</style><link rel=\"shortcut icon\" href=\"#\">"
            "</head><body><table>\n");
    writen(out_fd, buf, strlen(buf));
    DIR *d = fdopendir(dir_fd);
    struct dirent *dp;
    int ffd;
    while ((dp = readdir(d)) != NULL) {
        if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
            continue;
        }
        if ((ffd = openat(dir_fd, dp->d_name, O_RDONLY)) == -1) {
            perror(dp->d_name);
            continue;
        }
        fstat(ffd, &statbuf);
        strftime(m_time, sizeof(m_time),
                 "%Y-%m-%d %H:%M", localtime(&statbuf.st_mtime));
        format_size(size, &statbuf);
        if(S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)) {
            const char *d = S_ISDIR(statbuf.st_mode) ? "/" : "";
            sprintf(buf, "<tr><td><a href=\"%s%s\">%s%s</a></td><td>%s</td><td>%s</td></tr>\n",
                    dp->d_name, d, dp->d_name, d, m_time, size);
            writen(out_fd, buf, strlen(buf));
        }
        close(ffd);
    }
    sprintf(buf, "</table></body></html>");
    writen(out_fd, buf, strlen(buf));
    closedir(d);
}

static const char* get_mime_type(char *filename) {
    char *dot = strrchr(filename, '.');
    if(dot) { // strrchar Locate last occurrence of character in string
        mime_map *map = meme_types;
        while(map->extension) {
            if(strcmp(map->extension, dot) == 0) {
                return map->mime_type;
            }
            map++;
        }
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
            filename = ".";
        } else {
            for (int i=0; i < length; ++i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }
    url_decode(filename, req->filename, MAXLINE);
}

void client_error(int fd, int status, char *msg, char *longmsg) {
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
            get_mime_type(req->filename));

    writen(out_fd, buf, strlen(buf));
    off_t offset = req->offset; /* copy */
    while(offset < req->end) {
        if(sendfile(out_fd, in_fd, &offset, req->end - req->offset) <= 0) {
            break;
        }
#ifdef LOG_ACCESS
        printf("offset: %d \n\n", (unsigned int)offset);
#endif
        close(out_fd);
        break;
    }
}

// New function to handle API requests
void handle_request(int fd, http_request *req, struct sockaddr_in *clientaddr) {
    char response[4096];
    char username[256], password[256], problem_id[256], code[2048];
    
    // API endpoint for login
    if (strncmp(req->request_path, "/api/login", 10) == 0) {
        if (strcasecmp(req->method, "GET") == 0) {
            // If user is not logged in, show login page
            if(!globalSystemController->getUserRepo().getIsLogin()){
                serve_login_page(fd);
            }
            else{
                serve_submission_page(fd, globalSystemController->getUserRepo().getCurrentUser().c_str());
            }
        } else {
            client_error(fd, 405, "Method Not Allowed", "Only GET requests are allowed for login");
        }
    }
    // API endpoint for login
    else if (strncmp(req->request_path, "/api/user_login", 15) == 0) {
        char buf[MAXLINE];
        if (strcasecmp(req->method, "POST") == 0) {
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
                
                // Call the judge system login function
                globalSystemController->getUserRepo().setCurrentUser(username);

                sprintf(buf, "HTTP/1.1 303 See Other\r\nLocation: /api/login\r\n\r\n");
                writen(fd, buf, strlen(buf));
                return;
                
            } else {
                sprintf(response, "{\"success\": false, \"message\": \"Missing username or password\"}");
            }
            
            // Send the response
            sprintf(buf, "HTTP/1.1 200 OK\r\n");
            sprintf(buf + strlen(buf), "Content-length: %lu\r\n", strlen(response));
            sprintf(buf + strlen(buf), "Content-type: application/json\r\n\r\n");
            sprintf(buf + strlen(buf), "%s", response);
            writen(fd, buf, strlen(buf));
        } else {
            printf("%s\n", req->method);
            client_error(fd, 405, "Method Not Allowed", "Only POST requests are allowed for check_login");
        }
    }
    // API endpoint for getting login user name
    else if (strncmp(req->request_path, "/api/opt/1", 10) == 0) {
        if (strcasecmp(req->method, "GET") == 0) {
            // Call the judge system function to get version
            std::string username = globalSystemController->getAuthController().getCurrentUser();
            const char* name = username.c_str();
            
            // Format HTML response body
            char response[MAXLINE];
            snprintf(response, sizeof(response), "<p>Hello %s</p>", name);

            // Build HTTP response
            char buf[MAXLINE * 2]; // Just to be safe
            snprintf(buf, sizeof(buf),
                "HTTP/1.1 200 OK\r\n"
                "Content-length: %lu\r\n"
                "Content-type: text/html\r\n\r\n"
                "%s", strlen(response), response);
            writen(fd, buf, strlen(buf));
        } else {
            client_error(fd, 405, "Method Not Allowed", "Only GET requests are allowed for getting version");
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
        handle_request(fd, &req, clientaddr);
        return;
    }

    // Open the file
    struct stat sbuf;
    int status = stat(req.filename, &sbuf);
    if (status < 0) {
        client_error(fd, 404, "Not found", "File not found");
        return;
    }

    // Check if it's a directory
    if (S_ISDIR(sbuf.st_mode)) {
        // Handle directory request
        int dir_fd = open(req.filename, O_RDONLY);
        if (dir_fd < 0) {
            client_error(fd, 500, "Internal Server Error", "Failed to open directory");
            return;
        }
        handle_directory_request(fd, dir_fd, req.filename);
        close(dir_fd);
        return;
    }

    // Handle regular file
    int file_fd = open(req.filename, O_RDONLY);
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
    printf("select listening\n");
    int rv = select(serverfd+1, &readfds, NULL, NULL, NULL);
    printf("select get\n");
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