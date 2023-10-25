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
	fprintf(stderr, "  run repl  : %s\n", argv[0]);
	fprintf(stderr, "  run script: %s <path>\n", argv[0]);
	fprintf(stderr, "  run lsp   : %s --lsp\n", argv[0]);
	fprintf(stderr, "  print help: %s --help\n", argv[0]);
}

static char *read_file(const char *path) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(1);
	}

	fseek(file, 0L, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);

	char *buffer = malloc(file_size + 1);

	size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
	if (bytes_read < file_size) {
		fprintf(stderr, "Could not read whole file \"%s\".\n", path);
		exit(1);
	}

	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

static void run(dm_state *dm, char *prog) {
	dm_value ret = dm_vm_exec(dm, prog);
	printf("Result: ");
	dm_value_inspect(ret);
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
		run(dm, line);
		free(line);
	}
}

static void run_file(dm_state *dm, char *path) {
	char *source = read_file(path);
	run(dm, source);
	free(source);
}

int main(int argc, char **argv) {
	dm_state *dm = dm_open();

	if (argc == 1) {
		repl(dm);
	} else if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0) {
			usage(argv);
		} else if (strcmp(argv[1], "--lsp") == 0) {
			dm_lsp_run(dm);
		} else {
			run_file(dm, argv[1]);
		}
	} else {
		usage(argv);
		return 1;
	}

	dm_close(dm);
	return 0;
}

