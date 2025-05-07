CC := gcc
CFLAGS := -g -Wall

OBJ := main.o httpd.o router.o

server: $(OBJ)
	$(CC) $(CFLAGS) -o server $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server *.o users.db
