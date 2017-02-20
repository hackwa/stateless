#include "clientdefs.h"

int main(int argc, char **argv) 
{
	setIfAddr();
	char *unixsocket = "/run/redis.sock";
	printf("Unix socket at %s\n",unixsocket);
	static client cli; //
	cli = createClient(NULL,unixsocket);
	printf("\tInterface : <%s>\n",LOCAL_IFACE);
    printf("\t  Address : <%s>\n", local_ipaddr);
	simpleCmd(cli->local_context,"PING");
	printf("PING %s\n", reply->str);
	return 0;
}
