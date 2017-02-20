#include "clientdefs.h"

int main(int argc, char **argv) 
{
	char *unixsocket = "/run/redis.sock";
	printf("Unix socket at %s\n",unixsocket);
	static client cli; //
	cli = createClient(NULL,unixsocket);
	simpleCmd(cli->local_context,"PING");
	printf("PING %s\n", reply->str);
	return 0;
}
