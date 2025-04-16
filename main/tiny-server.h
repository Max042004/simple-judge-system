#include <cstddef>

int start_server();

size_t writen(int fd, void *usrbuf, size_t n);

void AcceptRequest();