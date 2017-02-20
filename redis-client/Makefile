INC=  -I ../hiredis-vip/
CFLAGS = -Ofast -march=native -flto

all:
	gcc $(CFLAGS) $(INC) -c client.c -o client.o
	gcc $(CFLAGS) $(INC) testapp.c libhiredis_vip.a client.o -o redis

clean:
	rm -f client.o redis
