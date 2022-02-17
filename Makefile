# Define variable CC to be the compiler we want to use
CC = clang

# Define CFLAGS for the flags we will want to use with clang
CLANG = -g -Wall

TARGETS = clean penn-sh

#SRCS=tokenizer.c penn-shredder.c 
#OBJS=tokenizer.o penn-shredder.o 
#LDFLAGS=
#LIBS=

.PHONY: penn-sh

# If no arguments are passed to make, it will attempt the 'penn-shredder' target
default: penn-sh

# This runs the 'clean' and 'penn-shredder' targets when 'make all' is run
all: $(TARGETS)

# This will construct the binary 'penn-shredder'
# $^ = names of all the dependent files, deduped and with spaces
# $@ = complete name of the target
# penn-shredder: penn-shredder.c 
# 	clang -Wall -g $< -o $@
#penn-sh: $(OBJS)
#	$(CC) $(LDFLAGS) $(LIBS) -o penn-shredder $(OBJS)
penn-sh: tokenizer.c penn-sh.c
		clang -Wall -g $^ -o $@

# $(RM) is the platform agnostic way to delete a file (here rm -f)
clean:
	$(RM) penn-sh
