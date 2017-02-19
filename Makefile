INC= -I hiredis/
CFLAGS = -Ofast -march=native -flto

all:
	gcc $(CFLAGS) $(INC) -c helper.c -o helper.o
	gcc $(CFLAGS) $(INC) redis.c libhiredis_vip.a helper.o -o redis

clean:
	rm -f helper.o redis
