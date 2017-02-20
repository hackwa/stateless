#include "clientdefs.h"

int main(int argc, char **argv) 
{
	setIfAddr();
	char *unixsocket = "/run/redis.sock";
	char *clusterid = "127.0.0.1:30001,127.0.0.1:30002,127.0.0.1:30003";
	printf("Unix socket at %s\n",unixsocket);
	static client cli; //
	cli = createClient(clusterid,unixsocket);
	printf("\tInterface : <%s>\n",LOCAL_IFACE);
    printf("\t  Address : <%s>\n", local_ipaddr);
	clusterCmd(cli->cluster_context,"set cat meow");
	clusterCmd(cli->cluster_context,"get cat");
	printf("GET CAT %s\n", creply->str);
	char *cmdlist[] = {"SET hal wa","GET hal"};
	int arraylen = 2;
	clusterAsyncCmd(cli->acc,cmdlist,&arraylen);
//	usleep(10000000);
	event_base_dispatch(_ebase);
	return 0;
}
