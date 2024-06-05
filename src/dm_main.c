#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

#include <dm_vm.h>
#include <dm_state.h>
#include <dm_lsp.h>

#define DM_REPL_PROMPT "> "

static void usage(char **argv) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  run repl:       %s\n", argv[0]);
	fprintf(stderr, "  run script:     %s <path>\n", argv[0]);
	fprintf(stderr, "  run lsp:        %s --lsp\n", argv[0]);
	fprintf(stderr, "  print help:     %s --help\n", argv[0]);
	fprintf(stderr, "Additional options:\n");
	fprintf(stderr, "  enable debug:   --debug\n");
}

static void run(dm_state *dm, char *prog, bool repl) {
	dm_value result;
	int success = dm_vm_exec(dm, prog, &result, repl);
	if (success != 0) {
		return;
	}

	if (dm_debug_enabled(dm)) {
		printf("Result: ");
	}

	dm_value_inspect(dm, result);
	printf("\n");
}

static void repl(dm_state *dm) {
	char *line;
	int linenum = 1;
	char buf[10 + sizeof(DM_REPL_PROMPT)];
	while (1) {
		sprintf(buf, "%d" DM_REPL_PROMPT, linenum);
		linenum++;
		line = readline(buf);
		if (line == NULL) {
			break;
		}
		add_history(line);
		run(dm, line, true);
		free(line);
	}
}

static void run_file(dm_state *dm, const char *path) {
	char *source = dm_read_file(path);
	run(dm, source, false);
	free(source);
}

int main(int argc, char **argv) {
	dm_state *dm = dm_open();
	if (dm == NULL) {
		return 1;
	}

	const char *script = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			usage(argv);
			return 0;
		} else if (strcmp(argv[i], "--argument-list") == 0) {
			printf("--help --lsp --debug\n");
			return 0;
		} else if (strcmp(argv[i], "--lsp") == 0) {
			dm_lsp_run(dm);
			return 0;
		} else if (strcmp(argv[i], "--debug") == 0) {
			dm_enable_debug(dm);
		} else {
			if (script == NULL) {
				script = argv[i];
			}
		}
	}

	if (script == NULL) {
		repl(dm);
	} else {
		run_file(dm, script);
	}

	dm_close(dm);
	return 0;
}
