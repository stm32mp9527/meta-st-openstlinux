/*
 * Simple TCP server template for STM32MP1 app development.
 * Usage: tcp-server [port]
 * Default port: 9000
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PORT 9000
#define BUF_SIZE 1024

static volatile sig_atomic_t g_running = 1;

static void on_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

int main(int argc, char **argv)
{
    int port = DEFAULT_PORT;
    if (argc > 1)
        port = atoi(argv[1]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* no SA_RESTART: accept/read must return EINTR */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(srv);
        return 1;
    }
    if (listen(srv, 8) < 0) {
        perror("listen");
        close(srv);
        return 1;
    }

    printf("tcp-server listening on 0.0.0.0:%d\n", port);
    fflush(stdout);

    while (g_running) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int cli = accept(srv, (struct sockaddr *)&peer, &peer_len);
        if (cli < 0) {
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }

        char ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        printf("client %s:%d connected\n", ip, ntohs(peer.sin_port));
        fflush(stdout);

        char buf[BUF_SIZE];
        ssize_t n;
        while ((n = read(cli, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("recv: %s", buf);
            fflush(stdout);
            const char *reply = "OK\n";
            if (write(cli, reply, strlen(reply)) < 0)
                break;
        }
        close(cli);
        printf("client disconnected\n");
        fflush(stdout);
    }

    close(srv);
    return 0;
}
