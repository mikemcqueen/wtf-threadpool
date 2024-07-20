CC := g++
CC_FLAGS := -O3 -std=c++23 -Wall -Werror -pedantic -I..
BINDIR := bin
TARGETS := thread pool

all: $(BINDIR) $(addprefix $(BINDIR)/,$(TARGETS))

$(BINDIR)/thread: thread.cpp
	$(CC) $< $(CC_FLAGS) -o $@

$(BINDIR)/pool: pool.cpp
	$(CC) $< $(CC_FLAGS) -o $@

$(BINDIR): 
	@mkdir $@
