# Makefile for the TTFTP program
CC = g++
CFLAGS = -std=c++11 -Wall -g -Werror -pedantic-errors -DNDEBUG -pthread
CCLINK = $(CC)
OBJS = main.o #readers_writers.o
RM = rm -f

# Creating the  executable
ttftps: $(OBJS)
	$(CCLINK) $(CFLAGS) -o ttftps $(OBJS)
# Creating the object files
main.o: main.cpp readers_writers.h
	$(CC) $(CFLAGS) -c main.cpp

# not sure if relevant yet
# readers_writers.o: readers_writers.cpp readers_writers.h
#	$(CC) $(CFLAGS) -c readers_writers.cpp


# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ "#"* core.* ttftps
