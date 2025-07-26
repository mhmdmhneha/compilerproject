CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =

SOURCES = prnttree.c loop_analysis.c main.c
test_program.c
OBJECTS = $(SOURCES:.c=.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ prnttree.o loop_analysis.o main.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGET) analysis_output.txt 