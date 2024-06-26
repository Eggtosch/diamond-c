CC     := gcc
RM     := rm -f

LDFLAGS := -pipe -flto
LIBS    := -lreadline

CFILES := $(wildcard src/dm_*.c)

FLAGS := -Wall -Wextra -Isrc/ -pipe -O2 -flto -march=native -s -MMD -MP
OBJDIR := bin
BINARY := bin/diamond
OBJS   := $(CFILES:%.c=$(OBJDIR)/%.o)
HEADER_DEPS := $(CFILES:%.c=$(OBJDIR)/%.d)

.PHONY: all
all: $(BINARY)

$(OBJDIR):
	mkdir -p $(OBJDIR)/src

$(BINARY): $(OBJDIR) $(OBJS)
	$(CC) $(LDFLAGS) -o $(BINARY) $(OBJS) $(LIBS)

-include $(HEADER_DEPS)
$(OBJDIR)/%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

install: $(BINARY)
	cp $(BINARY) ~/.local/bin/

.PHONY: tests
tests: $(BINARY)
	tests/run_tests.sh

.PHONY: tests-dry
tests-dry: $(BINARY)
	tests/run_tests.sh --dry

.PHONY: clean
clean:
	$(RM) $(BINARY)
	$(RM) $(OBJS)
	$(RM) $(HEADER_DEPS)
	$(RM) -r $(OBJDIR)
