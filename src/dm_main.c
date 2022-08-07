#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

#include <dm_vm.h>
#include <dm_state.h>

#define DM_REPL_PROMPT "> "

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
		int status = dm_vm_exec(dm, line);
		if (status == DM_VM_RUNTIME_ERROR) {
			fprintf(stderr, "Runtime Error!\n");
		}
		free(line);
	}
}

static void run_file(dm_state *dm, char *path) {
	char *source = read_file(path);
	int result = dm_vm_exec(dm, source);
	free(source);

	if (result == DM_VM_COMPILE_ERROR)
		exit(1);
	if (result == DM_VM_RUNTIME_ERROR)
		exit(1);
}

int main(int argc, char **argv) {
	dm_state *dm = dm_open();

	if (argc == 1) {
		repl(dm);
	} else if (argc == 2) {
		run_file(dm, argv[1]);
	} else {
		fprintf(stderr, "Usage: %s <path>\n", argv[0]);
		exit(1);
	}

	dm_close(dm);
	return 0;
}