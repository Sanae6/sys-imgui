#include "logger.hpp"

#include <switch.h>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdarg>

static int sock = -1;

#ifndef SERVER
#define SERVER "64.201.219.20"
#endif

Result redirectStdoutToLogServer() {
    int ret = -1;
    struct sockaddr_in srv_addr{
            .sin_family = AF_INET,
            .sin_port = htons(3080),
            .sin_addr = {inet_addr(SERVER)},
    };

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!sock) {
        return ret;
    }

    ret = connect(sock, (struct sockaddr*) &srv_addr, sizeof(srv_addr));
    if (ret != 0) {
        close(sock);
        return -1;
    }

    static const char* lol = "text\n";
    write(sock, lol, std::strlen(lol));
    // redirect stdout
    fflush(stdout);
    dup2(sock, STDOUT_FILENO);
    printf("beep boop bop\n");

    // redirect stderr
    fflush(stderr);
    dup2(sock, STDERR_FILENO);
    fprintf(stderr, "bap bop beep\n");

    return R_OK;
}

void abortWithResult(Result res) {
    printf("Aborted with result! rc=%d, module=%d, desc=%d\n", res, R_MODULE(res), R_DESCRIPTION(res));
    fflush(stdout);
    svcBreak(res, R_MODULE(res), R_DESCRIPTION(res));
}

void log(const char* text, ...) {
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    fflush(stdout);
}