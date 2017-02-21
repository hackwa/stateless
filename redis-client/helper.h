#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "hircluster.h"


struct timeval  tv1, tv2;
redisReply *benchreply;

void bench_local(redisContext *c, char *command, char *type);
void bench_local_pipelined(redisContext *c, char *command, char *type);
void bench_cluster(redisClusterContext* c, char* command, char* type);