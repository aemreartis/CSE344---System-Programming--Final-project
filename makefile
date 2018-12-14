all: 
	gcc server.c -o homeworkServer -lpthread -lm
	gcc client.c -o clientApp

