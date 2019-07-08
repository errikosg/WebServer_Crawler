OBJS	= myhttpd.o functions.o queue1.o trie.o plist.o mlist.o
SOURCE	= myhttpd.c functions.c queue1.c trie.c plist.c mlist.c
HEADER	= functions.h queue1.h  queue2.h hashtable.h
OUT = myhttpd
CC = gcc
FLAGS = -g -c

all: myhttpd mycrawler Worker

$(OUT): $(OBJS)
	$(CC) -g $(OBJS) -o $@ -lpthread

mycrawler: mycrawler.o functions.o queue2.o hashtable.o mlist.o plist.o trie.o
	$(CC) -g mycrawler.o functions.o queue2.o hashtable.o mlist.o plist.o trie.o -o $@ -lpthread

Worker: Worker.o mlist.o plist.o trie.o functions.o queue1.o
	$(CC) -g Worker.o mlist.o plist.o trie.o functions.o queue1.o -o $@


#create-compile individual files.
myhttpd.o: myhttpd.c
	$(CC) $(FLAGS) myhttpd.c

functions.o: functions.c
	$(CC) $(FLAGS) functions.c

queue1.o: queue1.c
	$(CC) $(FLAGS) queue1.c

queue2.o: queue2.c
	$(CC) $(FLAGS) queue2.c

cmd-client.o: cmd-client.c
	$(CC) $(FLAGS) cmd-client.c

mycrawler.o: mycrawler.c
	$(CC) $(FLAGS) mycrawler.c

hashtable.o: hashtable.c
	$(CC) $(FLAGS) hashtable.c

plist.o: plist.c
	$(CC) $(FLAGS) plist.c

mlist.o: mlist.c
	$(CC) $(FLAGS) mlist.c

trie.o: trie.c
	$(CC) $(FLAGS) trie.c

Worker.o: Worker.c
	$(CC) $(FLAGS) Worker.c



#clean place
clean:
	rm -f $(OBJS) $(OUT) sample_request sample.txt mycrawler.o mycrawler queue2.o hashtable.o Worker Worker.o

#acounting
count:
	wc $(SOURCE) $(HEADER) sample_request.c mycrawler.c queue2.c hashtable.c Worker.c
