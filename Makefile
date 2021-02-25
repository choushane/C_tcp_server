all: server

#%.o: %.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
server: main.o tcp_server.o
	$(CC) -o $@ $^ $(LDFLAGS) 

clean:
	rm -f *.o $(all)
