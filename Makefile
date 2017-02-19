INC= -I hiredis/
CFLAGS = -O3

all:
	gcc $(CFLAGS) $(INC) -c helper.c -o helper.o
	gcc $(CFLAGS) $(INC) redis.c libhiredis.a helper.o -o redis

clean:
	rm -f helper.o redis