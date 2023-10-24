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

.PHONY: clean
clean:
	$(RM) $(BINARY)
	$(RM) $(OBJS)
	$(RM) $(HEADER_DEPS)
	$(RM) -r $(OBJDIR)

ALL_C_H_FILES := $(shell find . -type f -name "*.c" -o -name "*.h")

.PHONY: lines
lines:
	@cat $(ALL_C_H_FILES) | wc -l

