# Makefile for the TTFTP program
CC = g++
CFLAGS = -std=c++11 -Wall -g -Werror -pedantic-errors -DNDEBUG
CCLINK = $(CC)
OBJS = main.o
RM = rm -f

# Creating the  executable
ttftps: $(OBJS)
	$(CCLINK) $(CFLAGS) -o ttftps $(OBJS)
# Creating the object files
main.o: main.cpp 
	$(CC) $(CFLAGS) -c main.cpp



# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ "#"* core.* ttftps
