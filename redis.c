#include "helper.h"

int main(int argc, char **argv) {
    unsigned int j;
    redisContext *c, *c1;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;

    printf ("..........Begin Testing on Localhost...........\n");
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    bench_ping(c);
    bench_set(c);
    bench_get(c);

    printf ("..........Begin Testing on Unix...........\n");
    char *socket = "/run/redis.sock";
    c1 = redisConnectUnix(socket);
    if (c1 == NULL || c->err) {
        if (c1) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c1);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    bench_ping(c);
    bench_set(c1);
    bench_get(c1);

    return 0;
}

