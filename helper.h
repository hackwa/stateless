#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <hiredis.h>


struct timeval  tv1, tv2;
redisReply *reply;

void bench_set(redisContext *c);
void bench_get(redisContext *c);
void bench_ping(redisContext *c);
void bench_cas(redisContext *c);
void tcp_connect_helper(redisContext *c, const char* hostname, int port);
