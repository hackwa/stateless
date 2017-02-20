#include "clientdefs.h"

int main(int argc, char **argv) 
{
	char *unixsocket = "/run/redis.sock";
	printf("Unix socket at %s\n",unixsocket);
	static client cli; //
	cli = createClient(NULL,unixsocket);
	return 0;
}
