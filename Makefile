all: client client_gtk server

client.o: client.c common.h
	gcc -c client.c `pkg-config --cflags gtk4`

client_gtk.o: client_gtk.c common.h
	gcc -c client_gtk.c `pkg-config --cflags gtk4`

common.o: common.c common.h
	gcc -c common.c

client_array.o: client_array.c client_array.h
	gcc -c client_array.c

server.o: server.c common.h client_array.h
	gcc -c server.c

client: client.o common.o
	gcc -o client client.o common.o `pkg-config --cflags --libs gtk4`

client_gtk: client_gtk.o common.o
	gcc -o client_gtk client_gtk.o common.o `pkg-config --cflags --libs gtk4`

server: server.o common.o client_array.o
	gcc -o server server.o common.o client_array.o

.PHONY: clean
clean:
	rm -f *.o client client_gtk server