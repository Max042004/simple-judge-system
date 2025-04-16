#include <cstddef>

int start_server();

size_t writen(int fd, void *usrbuf, size_t n);

void process(int fd, struct sockaddr_in *clientaddr);

void AcceptRequest();