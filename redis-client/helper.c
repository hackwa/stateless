#include "helper.h"

void bench_local(redisContext* c, char* command, char* type)
{
    long microseconds=0;
    int i;
    int numiters=1000;
    for(i=0; i<numiters; i++){
    gettimeofday(&tv1, NULL);
    benchreply = redisCommand(c,command);
    gettimeofday(&tv2, NULL);
    if(i==numiters-1)
    printf("%s: %s\n", command,benchreply->str);
    freeReplyObject(benchreply);
    if (tv2.tv_sec != tv1.tv_sec)printf("seconds not equal!\n");
    microseconds += (long) (tv2.tv_usec - tv1.tv_usec);
    }
    printf ("Average Total time for %s request = %d micro seconds\n",
         type, (int) microseconds/numiters);
}

void bench_local_pipelined(redisContext* c, char* command, char* type)
{
    long microseconds=0;
    int i;
    int numiters=100;
    for(i=0; i<numiters; i++){
    redisAppendCommand(c,command);
    }
    gettimeofday(&tv1, NULL);
    for(i=0; i<numiters; i++){
    redisGetReply(c,(void *)&benchreply);
//    freeReplyObject(reply);
    }
    gettimeofday(&tv2, NULL);
    printf("%s: %s\n", command,benchreply->str);
    freeReplyObject(benchreply);
    microseconds = (long) (tv2.tv_usec - tv1.tv_usec);
    printf ("Total time for %d pipelined requests = %d micro seconds\n",
         numiters,(int) microseconds);
}

void bench_cluster(redisClusterContext* c, char* command, char* type)
{
    long microseconds=0;
    int i;
    int numiters=1000;
    for(i=0; i<numiters; i++){
    gettimeofday(&tv1, NULL);
    benchreply = redisClusterCommand(c,command);
    gettimeofday(&tv2, NULL);
    if(i==numiters-1)
    printf("%s: %s\n", command,benchreply->str);
    freeReplyObject(benchreply);
    if (tv2.tv_sec != tv1.tv_sec)printf("seconds not equal!\n");
    microseconds += (long) (tv2.tv_usec - tv1.tv_usec);
    }
    printf ("Average Cluster time for %s request = %d micro seconds\n",
         type, (int) microseconds/numiters);
}