all:
	gcc message_server.c -lpthread -o message_server
	gcc client.c -o client
