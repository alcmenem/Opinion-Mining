CC = g++
CFLAGS  = -g -Wall -std=c++11
OBJS = main.o meansmethod.o tagsmethod.o knnmethod.o

APPNAME = OpinionMining

all: prog

default: prog

prog: $(OBJS)
	$(CC) $(CFLAGS) -o $(APPNAME) $(OBJS)

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

meansmethod.o: meansmethod.cpp meansmethod.h
	$(CC) $(CFLAGS) -c meansmethod.cpp

tagsmethod.o: tagsmethod.cpp tagsmethod.h
	$(CC) $(CFLAGS) -c tagsmethod.cpp

knnmethod.o: knnmethod.cpp knnmethod.h
	$(CC) $(CFLAGS) -c knnmethod.cpp

clean:
	$(RM) $(APPNAME) *.o *~