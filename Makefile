CC = gcc

arc: arcx.c
	$(CC) -o arcx arcx.c

clean:
	rm -f arc
