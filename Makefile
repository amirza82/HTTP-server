CC = gcc
CFLAGS = -Wall -Wextra
LIBS =
SRC = server.c client.c utils.c
SHARED_OBJ = utils.o
S_OBJ = server.o
C_OBJ = client.o
HEADS = head.h

# Run the server by default
all: server 
	./server

c: client

%.o:%.c head.h
	$(CC) -c -o $@ $(CFLAGS) $<

server: $(S_OBJ) $(SHARED_OBJ)
	$(CC) -o $@ $(S_OBJ) $(SHARED_OBJ) $(LIBS)

client: $(C_OBJ) $(SHARED_OBJ)
	$(CC) -o $@ $(C_OBJ) $(SHARED_OBJ) $(LIBS)

clean:
	rm -f $(SHARED_OBJ)
	rm -f $(S_OBJ)
	rm -f $(C_OBJ)
	rm -f server client a.out test log.txt

