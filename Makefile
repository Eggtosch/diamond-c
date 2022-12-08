CC     := gcc
RM     := rm -f

ifeq (,$(findstring release,$(MAKECMDGOALS)))
	CFLAGS := -Wall -Wextra -Isrc/ -pipe -ggdb -O2 -march=native -MMD -MP
else
	CFLAGS := -Wall -Wextra -Werror -Isrc/ -pipe -Os -march=native -s -MMD -MP
endif

LDFLAGS := -pipe -flto
LIBS    := -lreadline

BINARY       := diamond
OBJDIR       := src/.bin
CFILES       := src/dm_main.c src/dm_state.c src/dm_vm.c src/dm_compiler.c \
				src/dm_chunk.c src/dm_value.c src/dm_generic_array.c src/dm_gc.c
OBJS         := $(CFILES:%.c=$(OBJDIR)/%.o)
HEADER_DEPS  := $(CFILES:%.c=$(OBJDIR)/%.d)

.PHONY: all release
all: $(OBJDIR) $(BINARY)
release: $(OBJDIR) $(BINARY)

$(OBJDIR):
	mkdir -p $(OBJDIR)/src

$(BINARY): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BINARY) $(OBJS) $(LIBS)

-include $(HEADER_DEPS)
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(BINARY)
	$(RM) $(OBJS)
	$(RM) $(HEADER_DEPS)

ALL_C_H_FILES := $(shell find . -type f -name "*.c" -o -name "*.h")

.PHONY: lines
lines:
	@cat $(ALL_C_H_FILES) | wc -l

