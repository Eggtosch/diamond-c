CC     := gcc
RM     := rm -f

LDFLAGS := -pipe -flto
LIBS    := -lreadline

CFILES         := src/dm_main.c src/dm_state.c src/dm_vm.c src/dm_compiler.c \
				  src/dm_chunk.c src/dm_value.c src/dm_gc.c

DEBUG_FLAGS := -Wall -Wextra -Isrc/ -pipe -ggdb -O2 -march=native -MMD -MP
DEBUG_OBJDIR   := bin/debug
DEBUG_BINARY   := bin/debug/diamond
DEBUG_OBJS     := $(CFILES:%.c=$(DEBUG_OBJDIR)/%.o)
DEBUG_HEADER_DEPS   := $(CFILES:%.c=$(DEBUG_OBJDIR)/%.d)

RELEASE_FLAGS := -Wall -Wextra -Werror -Isrc/ -pipe -O2 -march=native -s -MMD -MP
RELEASE_OBJDIR := bin/release
RELEASE_BINARY := bin/release/diamond
RELEASE_OBJS   := $(CFILES:%.c=$(RELEASE_OBJDIR)/%.o)
RELEASE_HEADER_DEPS := $(CFILES:%.c=$(RELEASE_OBJDIR)/%.d)


.PHONY: all release
all: $(DEBUG_OBJDIR) $(DEBUG_BINARY)
release: $(RELEASE_OBJDIR) $(RELEASE_BINARY)


$(DEBUG_OBJDIR):
	mkdir -p $(DEBUG_OBJDIR)/src

$(DEBUG_BINARY): $(DEBUG_OBJS)
	$(CC) $(LDFLAGS) -o $(DEBUG_BINARY) $(DEBUG_OBJS) $(LIBS)

-include $(DEBUG_HEADER_DEPS)
$(DEBUG_OBJDIR)/%.o: %.c
	$(CC) $(DEBUG_FLAGS) -c $< -o $@


$(RELEASE_OBJDIR):
	mkdir -p $(RELEASE_OBJDIR)/src

$(RELEASE_BINARY): $(RELEASE_OBJS)
	$(CC) $(LDFLAGS) -o $(RELEASE_BINARY) $(RELEASE_OBJS) $(LIBS)

-include $(RELEASE_HEADER_DEPS)
$(RELEASE_OBJDIR)/%.o: %.c
	$(CC) $(RELEASE_FLAGS) -c $< -o $@


.PHONY: clean
clean:
	$(RM) $(DEBUG_BINARY) $(RELEASE_BINARY)
	$(RM) $(DEBUG_OBJS) $(RELEASE_OBJS)
	$(RM) $(DEBUG_HEADER_DEPS) $(RELEASE_HEADER_DEPS)
	$(RM) -r $(DEBUG_OBJDIR) $(RELEASE_OBJDIR)


ALL_C_H_FILES := $(shell find . -type f -name "*.c" -o -name "*.h")

.PHONY: lines
lines:
	@cat $(ALL_C_H_FILES) | wc -l

