INC=  -I ../hiredis-vip/
CFLAGS = -Ofast -flto -g3 #-march=native
OBJS = client.o helper.o libhiredis_vip.a

all:
	gcc $(CFLAGS) $(INC) -c client.c -o client.o
	gcc $(CFLAGS) $(INC) -c helper.c -o helper.o
	gcc $(CFLAGS) $(INC) testapp.c $(OBJS) -levent -o redis

clean:
	rm -f client.o redis
