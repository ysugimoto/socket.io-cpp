CC = g++
CFLAGS = -stdlib=libc++ -std=c++11

socketio: libwebsockets websocket main
	$(CC) $(CFLAGS) \
		$(CURDIR)/compiled/main.o \
		$(CURDIR)/compiled/websocket.o \
		-lwebsockets \
		-o $(CURDIR)/socketio

main : $(CURDIR)/test/main.cpp
	$(CC) $(CFLAGS) -c -I $(CURDIR)/tmp -I $(CURDIR)/compiled/include $(CURDIR)/test/main.cpp -o compiled/main.o

websocket: $(CURDIR)/tmp/WebSocketClient.cpp
	$(CC) $(CFLAGS) -c -I $(CURDIR)/compiled/include $(CURDIR)/tmp/WebSocketClient.cpp -o compiled/websocket.o

libwebsockets:
	@ echo "Build libwebsokets.c"
	@ $(MAKE) -C build -f Makefile.ws
