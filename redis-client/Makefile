INC=  -I ../hiredis-vip/
CFLAGS = -Ofast -flto -g3 #-march=native

all:
	gcc $(CFLAGS) $(INC) -c client.c -o client.o
	gcc $(CFLAGS) $(INC) testapp.c libhiredis_vip.a client.o -Wl,--start-group -levent -Wl,--end-group -o redis

clean:
	rm -f client.o redis
