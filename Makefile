TARGET = ext2cat

CC = gcc

CFLAGS = -Wall -Wextra -std=c99 -g

SRCS = main.c path.c cat.c inode.c print.c touch.c ext2_global.c cd.c rm.c

OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean