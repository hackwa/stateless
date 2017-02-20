#include "clientdefs.h"

client createClient(char *clusterid, char* unixsocket) 
{
    int j;
    client c = malloc(sizeof(struct _client));
    if(clusterid != NULL)
    c->cluster_context = redisClusterConnect(clusterid,HIRCLUSTER_FLAG_NULL);
    c->local_context = redisConnectUnix(unixsocket);

    if (c->cluster_context == NULL || c->cluster_context->err) {
        if (c->cluster_context) {
            printf("Cluster Connection error: %s\n", c->cluster_context->errstr);
            redisClusterFree(c->cluster_context);
        } else {
            printf("Cluster Connection error: can't allocate redis context\n");
        }
//        exit(1);
    }

    if (c->local_context == NULL || c->local_context->err) {
    if (c->local_context) {
        printf("Local Connection error: %s\n", c->local_context->errstr);
        redisFree(c->local_context);
    } else {
        printf("Local Connection error: can't allocate redis context\n");
    }
    exit(1);
    }

    printf("Successfully connectected to cluster and local databases\n");
    return c;
}

/*
    The invoking function is responsible for keeping track of
    reply and creply objects.
*/

void simpleCmd(redisContext *c, char* cmd)
{
    if(reply != NULL)
        freeReplyObject(reply);
    reply = redisCommand(c,cmd);
    CHECK(reply);
}

void simplePipeline(redisContext *c , char **cmdlist, int num)
{
    if(reply != NULL)
        freeReplyObject(reply);
    int i;
    for (i=0; i<num; i++){
        redisAppendCommand(c,cmdlist[i]);
    }
    /* This writes the entire buffer to socket
       And waits for a single reply.*/
    redisGetReply(c,(void *)&reply);
}

void clusterCmd(redisClusterContext *cc, char* cmd)
{
    if(creply != NULL)
        freeReplyObject(creply);
    creply = redisClusterCommand(cc,cmd);
    CHECK(creply);
}

void clusterPipelineCmd(redisClusterContext *cc, char **cmdlist, int num)
{
    if(creply != NULL)
        freeReplyObject(creply);
    int i;
    for (i=0; i<num; i++){
        redisClusterAppendCommand(cc,cmdlist[i]);
    }
    /* This writes the entire buffer to socket
       And waits for a single reply.*/
    redisClusterGetReply(cc,(void *)&creply);
}

/*
    We need IP Address to be able to store it inside cluster
*/

void setIfAddr()
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host,
            NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name,LOCAL_IFACE)==0)&&(ifa->ifa_addr->sa_family==AF_INET)) {
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(1);
            }
            // Assume safe
            strcpy(local_ipaddr,host);
        }
    }
    freeifaddrs(ifaddr);
}