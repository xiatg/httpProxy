all: proxy

proxy: main.c
	gcc -pthread -o proxy main.c

clean:
	rm proxy