#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <dm_lsp.h>

#define CONTENT_LENGTH_PREFIX "Content-Length: "
#define CONTENT_TYPE_PREFIX "Content-Type: "

struct lsp_msg_header {
	int content_length;
	const char *content_type;
	const char *encoding;
};

struct lsp_msg {
	int id;
};

static void lsp_errno_error(void) {
	perror("[dm-lsp] Unrecoverable error");
	exit(1);
}

static void lsp_fmt_error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[dm-lsp] Unrecoverable error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
}

static void lsp_log(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[dm-lsp] ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

static void parse_content_length(struct lsp_msg_header *header, const char *s) {
	if (sscanf(s, CONTENT_LENGTH_PREFIX "%d\r\n", &header->content_length) != 1) {
		lsp_fmt_error("Could not parse Content-Length header: %s", s);
	}
}

static void parse_content_type(struct lsp_msg_header *header, const char *s) {
	const char *content_type_start = NULL;
	const char *encoding_start = NULL;
	const char *encoding_start2 = NULL;

	if ((content_type_start = strchr(s, ' ')) == NULL) {
		lsp_fmt_error("Could not find start of content_type in header: %s", s);
	}

	if ((encoding_start = strchr(s, ';')) == NULL) {
		lsp_fmt_error("Could not find encoding in Content-Type header: %s", s);
	}
	if ((encoding_start2 = strchr(encoding_start, '=')) == NULL) {
		lsp_fmt_error("Could not find encoding value: %s", encoding_start);
	}

	encoding_start2++;
	int encoding_length = strchr(encoding_start2, '\r') - encoding_start2;
	free((void*) header->encoding);
	header->encoding = strndup(encoding_start2, encoding_length);

	content_type_start++;
	int content_type_length = encoding_start - content_type_start;
	free((void*) header->content_type);
	header->content_type = strndup(content_type_start, content_type_length);

	if (strcmp(header->content_type, "utf8") == 0) {
		free((void*) header->content_type);
		header->content_type = strdup("utf-8");
	}
}

static struct lsp_msg_header read_header(dm_state *dm) {
	(void) dm;
	struct lsp_msg_header header = {
		.content_length = -1,
		.content_type = strdup("application/vscode-jsonrpc"),
		.encoding = strdup("utf-8"),
	};

	while (1) {
		char *buffer = NULL;
		size_t i;
		if (getline(&buffer, &i, stdin) == -1) {
			lsp_errno_error();
		}

		if (strncmp(buffer, CONTENT_LENGTH_PREFIX, strlen(CONTENT_LENGTH_PREFIX)) == 0) {
			parse_content_length(&header, buffer);
		} else if (strncmp(buffer, CONTENT_TYPE_PREFIX, strlen(CONTENT_TYPE_PREFIX)) == 0) {
			parse_content_type(&header, buffer);
		} else if (strcmp(buffer, "\r\n") == 0) {
			free(buffer);
			break;
		} else {
			lsp_fmt_error("Unknown header: %s", buffer);
		}

		free(buffer);
		buffer = NULL;
	}

	return header;
}

static void process_json(dm_state *dm, const char *json) {
	(void) dm;
	lsp_log("%s", json);
}

void dm_lsp_run(dm_state *dm) {
	(void) dm;

	while (1) {
		struct lsp_msg_header header = read_header(dm);
		if (header.content_length <= 0) {
			lsp_log("Could not read content length");
			continue;
		}

		int len = header.content_length;
		char *buffer = malloc(len + 1);
		int n = fread(buffer, 1, len, stdin);
		buffer[len] = '\0';
		if (n != len) {
			lsp_log("Could not read whole message");
			continue;
		}

		process_json(dm, buffer);
		free(buffer);
	}
}
