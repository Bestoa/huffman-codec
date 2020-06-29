GCC = gcc
CFLAGS = -Wall -Werror
CFLAGS += -O3
#CFLAGS += -DDEBUG

.PHONY: all
all:
	$(GCC) $(CFLAGS) *.c -o huffman

.PHONY: clean
clean:
	rm -rf huffman out*
