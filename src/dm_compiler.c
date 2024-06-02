#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <dm_compiler.h>
#include <dm_chunk.h>
#include <dm_state.h>

//#############################################################

typedef enum {
	DM_TOKEN_LEFT_PAREN, DM_TOKEN_RIGHT_PAREN,
	DM_TOKEN_LEFT_BRACE, DM_TOKEN_RIGHT_BRACE,
	DM_TOKEN_LEFT_BRACKET, DM_TOKEN_RIGHT_BRACKET,
	DM_TOKEN_COMMA, DM_TOKEN_DOT, DM_TOKEN_MINUS, DM_TOKEN_PLUS,
	DM_TOKEN_SEMICOLON, DM_TOKEN_SLASH, DM_TOKEN_STAR, DM_TOKEN_COLON,
	DM_TOKEN_PERCENT,

	DM_TOKEN_BANG, DM_TOKEN_BANG_EQUAL,
	DM_TOKEN_EQUAL, DM_TOKEN_EQUAL_EQUAL,
	DM_TOKEN_GREATER, DM_TOKEN_GREATER_EQUAL,
	DM_TOKEN_LESS, DM_TOKEN_LESS_EQUAL,

	DM_TOKEN_IDENTIFIER, DM_TOKEN_STRING, DM_TOKEN_INTEGER, DM_TOKEN_FLOAT,

	DM_TOKEN_AND, DM_TOKEN_BREAK, DM_TOKEN_DO, DM_TOKEN_ELSE, DM_TOKEN_ELSIF, DM_TOKEN_END, DM_TOKEN_FALSE,
	DM_TOKEN_FOR, DM_TOKEN_FUNCTION, DM_TOKEN_IF, DM_TOKEN_NEXT, DM_TOKEN_NIL, DM_TOKEN_NOT,
	DM_TOKEN_OR, DM_TOKEN_RETURN, DM_TOKEN_SELF, DM_TOKEN_THEN, DM_TOKEN_TRUE,
	DM_TOKEN_WHILE,

	DM_TOKEN_IMPORT, DM_TOKEN_GLOBAL,

	DM_TOKEN_ERROR, DM_TOKEN_EOF
} dm_tokentype;

typedef struct {
	dm_tokentype type;
	const char *begin;
	int len;
	int line;
} dm_token;

typedef struct {
	const char *start;
	const char *current;
	int line;
} dm_lexer;

struct reserved_word {
	const char *name;
	dm_tokentype type;
};

static struct reserved_word reserved_words[] = {
	{"and",      DM_TOKEN_AND},
	{"break",    DM_TOKEN_BREAK},
	{"do",       DM_TOKEN_DO},
	{"else",     DM_TOKEN_ELSE},
	{"elsif",    DM_TOKEN_ELSIF},
	{"end",      DM_TOKEN_END},
	{"false",    DM_TOKEN_FALSE},
	{"for",      DM_TOKEN_FOR},
	{"function", DM_TOKEN_FUNCTION},
	{"global",   DM_TOKEN_GLOBAL},
	{"if",       DM_TOKEN_IF},
	{"import",   DM_TOKEN_IMPORT},
	{"next",     DM_TOKEN_NEXT},
	{"nil",      DM_TOKEN_NIL},
	{"not",      DM_TOKEN_NOT},
	{"or",       DM_TOKEN_OR},
	{"return",   DM_TOKEN_RETURN},
	{"self",     DM_TOKEN_SELF},
	{"then",     DM_TOKEN_THEN},
	{"true",     DM_TOKEN_TRUE},
	{"while",    DM_TOKEN_WHILE},
	{NULL,       DM_TOKEN_EOF}
};


static dm_token ltoken_new(dm_lexer *lexer, dm_tokentype type) {
	dm_token token;
	token.type  = type;
	token.begin = lexer->start;
	token.len   = (int)(lexer->current - lexer->start);
	token.line  = lexer->line;
	return token;
}

static dm_token ltoken_error(dm_lexer *lexer, const char *message) {
	dm_token token;
	token.type  = DM_TOKEN_ERROR;
	token.begin = message;
	token.len   = (int) strlen(message);
	token.line  = lexer->line;
	return token;
}

static bool lis_eof(dm_lexer *lexer) {
	return *lexer->current == '\0';
}

static char lpeek(dm_lexer *lexer) {
	return *lexer->current;
}

static char lpeek_next(dm_lexer *lexer) {
	if (lis_eof(lexer)) return '\0';
	return lexer->current[1];
}

static char lnext(dm_lexer *lexer) {
	lexer->current++;
	return lexer->current[-1];
}

static bool lmatch(dm_lexer *lexer, char expected) {
	if (lis_eof(lexer)) return false;
	if (*lexer->current != expected) return false;
	lexer->current++;
	return true;
}

static bool lis_digit(char c) {
	return c >= '0' && c <= '9';
}

static bool lis_alpha(char c) {
	return (c >= 'a' && c <= 'z') ||
		   (c >= 'A' && c <= 'Z') ||
		   (c == '_');
}

static void lskip_whitespace(dm_lexer *lexer) {
	while (1) {
		char c = lpeek(lexer);
		if (c == ' ' || c == '\r' || c == '\t') {
			lnext(lexer);
		} else if (c == '\n') {
			lexer->line++;
			lnext(lexer);
		} else if (c == '#') {
			while (lpeek(lexer) != '\n' && !lis_eof(lexer)) lnext(lexer);
		} else {
			return;
		}
	}
}

static dm_token lread_string(dm_lexer *lexer) {
	while (lpeek(lexer) != '"' && !lis_eof(lexer)) {
		if (lpeek(lexer) == '\n') lexer->line++;
		lnext(lexer);
	}

	if (lis_eof(lexer))
		return ltoken_error(lexer, "found unterminated string");

	lnext(lexer);
	return ltoken_new(lexer, DM_TOKEN_STRING);
}

static dm_token lnumber(dm_lexer *lexer) {
	while (lis_digit(lpeek(lexer))) lnext(lexer);

	if (lpeek(lexer) == '.' && lis_digit(lpeek_next(lexer))) {
		lnext(lexer);
		while (lis_digit(lpeek(lexer))) lnext(lexer);
		return ltoken_new(lexer, DM_TOKEN_FLOAT);
	}

	return ltoken_new(lexer, DM_TOKEN_INTEGER);
}

static dm_tokentype lidentifier_type(dm_lexer *lexer) {
	struct reserved_word *word = &reserved_words[0];
	unsigned int len = lexer->current - lexer->start;
	for (; word->name != NULL; word++) {
		if (len == strlen(word->name) && strncmp(word->name, lexer->start, len) == 0) {
			return word->type;
		}
	}
	return DM_TOKEN_IDENTIFIER;
}

static dm_token lidentifier(dm_lexer *lexer) {
	while (lis_alpha(lpeek(lexer)) || lis_digit(lpeek(lexer))) lnext(lexer);
	return ltoken_new(lexer, lidentifier_type(lexer));
}

dm_token lex(dm_lexer *lexer) {
	lskip_whitespace(lexer);
	lexer->start = lexer->current;

	if (lis_eof(lexer))
		return ltoken_new(lexer, DM_TOKEN_EOF);

	char c = lnext(lexer);

	if (lis_digit(c)) return lnumber(lexer);
	if (lis_alpha(c)) return lidentifier(lexer);

	switch(c) {
		case '(': return ltoken_new(lexer, DM_TOKEN_LEFT_PAREN);
		case ')': return ltoken_new(lexer, DM_TOKEN_RIGHT_PAREN);
		case '{': return ltoken_new(lexer, DM_TOKEN_LEFT_BRACE);
		case '}': return ltoken_new(lexer, DM_TOKEN_RIGHT_BRACE);
		case '[': return ltoken_new(lexer, DM_TOKEN_LEFT_BRACKET);
		case ']': return ltoken_new(lexer, DM_TOKEN_RIGHT_BRACKET);
		case ';': return ltoken_new(lexer, DM_TOKEN_SEMICOLON);
		case ':': return ltoken_new(lexer, DM_TOKEN_COLON);
		case ',': return ltoken_new(lexer, DM_TOKEN_COMMA);
		case '.': return ltoken_new(lexer, DM_TOKEN_DOT);
		case '-': return ltoken_new(lexer, DM_TOKEN_MINUS);
		case '+': return ltoken_new(lexer, DM_TOKEN_PLUS);
		case '/': return ltoken_new(lexer, DM_TOKEN_SLASH);
		case '%': return ltoken_new(lexer, DM_TOKEN_PERCENT);
		case '*': return ltoken_new(lexer, DM_TOKEN_STAR);
		case '!': return ltoken_new(lexer, lmatch(lexer, '=') ? DM_TOKEN_BANG_EQUAL    : DM_TOKEN_BANG);
		case '=': return ltoken_new(lexer, lmatch(lexer, '=') ? DM_TOKEN_EQUAL_EQUAL   : DM_TOKEN_EQUAL);
		case '<': return ltoken_new(lexer, lmatch(lexer, '=') ? DM_TOKEN_LESS_EQUAL    : DM_TOKEN_LESS);
		case '>': return ltoken_new(lexer, lmatch(lexer, '=') ? DM_TOKEN_GREATER_EQUAL : DM_TOKEN_GREATER);
		case '"': return lread_string(lexer);
	}

	return ltoken_error(lexer, "Unexpected character.");
}

//#############################################################

typedef struct dm_parser {
	dm_state *dm;
	dm_lexer *lexer;
	dm_chunk *chunk;
	dm_token current;
	dm_token previous;
	bool had_error;
	bool panic_mode;
} dm_parser;

typedef enum {
	DM_PREC_NONE,
	DM_PREC_ASSIGNEMENT,
	DM_PREC_OR,
	DM_PREC_AND,
	DM_PREC_EQUALITY,
	DM_PREC_COMPARISON,
	DM_PREC_TERM,
	DM_PREC_FACTOR,
	DM_PREC_UNARY,
	DM_PREC_CALL,
	DM_PREC_PRIMARY
} dm_precedence;

typedef void (*parsefn)(dm_parser*);

typedef struct {
	parsefn prefix;
	parsefn infix;
	dm_precedence precedence;
} dm_parserule;

dm_parserule rules[];


static dm_parserule *pget_rule(dm_tokentype type) {
	return &rules[type];
}

static void perr_at(dm_parser *parser, dm_token *token, const char *message) {
	if (parser->panic_mode) return;
	parser->panic_mode = true;
	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == DM_TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type != DM_TOKEN_ERROR) {
		fprintf(stderr, " at '%.*s'", token->len, token->begin);
	}

	fprintf(stderr, ": %s\n", message);
	parser->had_error = true;
}

static void perr(dm_parser *parser, const char *message) {
	perr_at(parser, &parser->current, message);
}

static void pnext(dm_parser *parser) {
	parser->previous = parser->current;
	for (;;) {
		parser->current = lex(parser->lexer);
		if (parser->current.type != DM_TOKEN_EOF) {
			dm_chunk_set_line(parser->chunk, parser->current.line);
		}
		if (parser->current.type != DM_TOKEN_ERROR) {
			break;
		}

		perr(parser, parser->current.begin);
	}
}

static bool pmatch(dm_parser *parser, dm_tokentype type) {
	if (parser->current.type != type) {
		return false;
	}
	pnext(parser);
	return true;
}

static bool pcheck(dm_parser *parser, dm_tokentype type) {
	return parser->current.type == type;
}

static void pconsume(dm_parser *parser, dm_tokentype type, const char *message) {
	if (parser->current.type == type) {
		pnext(parser);
		return;
	}
	perr(parser, message);
}

static void pparse_precedence(dm_parser *parser, dm_precedence prec) {
	pnext(parser);
	parsefn f = pget_rule(parser->previous.type)->prefix;
	if (f == NULL) {
		perr(parser, "expect expression");
		return;
	}

	f(parser);

	while (prec <= pget_rule(parser->current.type)->precedence) {
		pnext(parser);
		parsefn f = pget_rule(parser->previous.type)->infix;
		f(parser);
	}
}

static void pexpression(dm_parser *parser) {
	pparse_precedence(parser, DM_PREC_ASSIGNEMENT);
}

static void pident(dm_parser *parser) {
	const char *var = parser->previous.begin;
	int len = parser->previous.len;
	if (pmatch(parser, DM_TOKEN_EQUAL)) {
		pexpression(parser);
		int ident = dm_chunk_add_var(parser->chunk, var, len);
		dm_chunk_emit_arg16(parser->chunk, DM_OP_VARSET, ident);
	} else {
		int index = dm_chunk_find_var(parser->chunk, var, len);
		if (index == -1) {
			perr_at(parser, &parser->previous, "variable does not exist in current scope!");
		}
		dm_chunk_emit_arg16(parser->chunk, DM_OP_VARGET, index);
	}
}

static void pglobal(dm_parser *parser) {
	pconsume(parser, DM_TOKEN_IDENTIFIER, "expect identifier after global keyword");
	const char *var = parser->previous.begin;
	int len = parser->previous.len;
	dm_chunk *chunk = (dm_chunk*) parser->chunk->parent;
	int ups = 1;
	int index = -1;
	while (chunk != NULL) {
		index = dm_chunk_find_var(chunk, var, len);
		if (index != -1) {
			break;
		}
		ups++;
		chunk = (dm_chunk*) chunk->parent;
	}
	if (index == -1) {
		perr_at(parser, &parser->previous, "global variable does not exist!");
	}
	if (pmatch(parser, DM_TOKEN_EQUAL)) {
		pexpression(parser);
		dm_chunk_emit_arg8_arg16(parser->chunk, DM_OP_VARSET_UP, ups, index);
	} else {
		dm_chunk_emit_arg8_arg16(parser->chunk, DM_OP_VARGET_UP, ups, index);
	}
}

static void parraylit(dm_parser *parser) {
	int nelems = 0;
	if (!pcheck(parser, DM_TOKEN_RIGHT_BRACKET)) {
		do {
			pexpression(parser);
			nelems++;
		} while (pmatch(parser, DM_TOKEN_COMMA));
	}
	pconsume(parser, DM_TOKEN_RIGHT_BRACKET, "expect ']'");
	dm_chunk_emit_arg16(parser->chunk, DM_OP_ARRAYLIT, nelems);
}

static void parrayget(dm_parser *parser) {
	pexpression(parser);
	pconsume(parser, DM_TOKEN_RIGHT_BRACKET, "expect ']'");
	if (pmatch(parser, DM_TOKEN_EQUAL)) {
		pexpression(parser);
		dm_chunk_emit(parser->chunk, DM_OP_FIELDSET);
	} else {
		dm_chunk_emit(parser->chunk, DM_OP_FIELDGET);
	}
}

static void ptablelit(dm_parser *parser) {
	int nelems = 0;
	if (!pcheck(parser, DM_TOKEN_RIGHT_BRACE)) {
		do {
			pexpression(parser);
			pconsume(parser, DM_TOKEN_COLON, "expect ':'");
			pexpression(parser);
			nelems++;
		} while (pmatch(parser, DM_TOKEN_COMMA));
	}
	pconsume(parser, DM_TOKEN_RIGHT_BRACE, "expect '}'");
	dm_chunk_emit_arg16(parser->chunk, DM_OP_TABLELIT, nelems);
}

static void pstring(dm_parser *parser) {
	const char *s = parser->previous.begin + 1;
	int len = parser->previous.len - 2;
	dm_chunk_emit_constant(parser->chunk, dm_value_string_len(parser->dm, s, len));
}

static void pinteger(dm_parser *parser) {
	long value = strtol(parser->previous.begin, NULL, 10);
	if (value <= UINT16_MAX) {
		dm_chunk_emit_arg16(parser->chunk, DM_OP_CONSTANT_SMALLINT, value);
	} else {
		dm_chunk_emit_constant(parser->chunk, dm_value_int(value));
	}
}

static void pfloating(dm_parser *parser) {
	double value = strtod(parser->previous.begin, NULL);
	dm_chunk_emit_constant(parser->chunk, dm_value_float(value));
}

static void pboolean(dm_parser *parser) {
	if (parser->previous.type == DM_TOKEN_TRUE) {
		dm_chunk_emit(parser->chunk, DM_OP_TRUE);
	} else {
		dm_chunk_emit(parser->chunk, DM_OP_FALSE);
	}
}

static void pnil(dm_parser *parser) {
	dm_chunk_emit(parser->chunk, DM_OP_NIL);
}

static void pself(dm_parser *parser) {
	dm_chunk_emit(parser->chunk, DM_OP_SELF);
}

static void pgrouping(dm_parser *parser) {
	pexpression(parser);
	pconsume(parser, DM_TOKEN_RIGHT_PAREN, "expect ')' after expression");
}

static int pparlist(dm_parser *parser) {
	int nparams = 0;
	if (!pcheck(parser, DM_TOKEN_RIGHT_PAREN)) {
		do {
			pexpression(parser);
			nparams++;
		} while (pmatch(parser, DM_TOKEN_COMMA));
	}
	pconsume(parser, DM_TOKEN_RIGHT_PAREN, "expect ')'");
	return nparams;
}

static void pcall(dm_parser *parser) {
	int line = parser->lexer->line;
	int nparams = pparlist(parser);
	dm_chunk_set_line(parser->chunk, line);
	dm_chunk_emit_arg8(parser->chunk, DM_OP_CALL, nparams);
}

static void pcall_with_parent(dm_parser *parser) {
	int line = parser->lexer->line;
	int nparams = pparlist(parser);
	dm_chunk_set_line(parser->chunk, line);
	dm_chunk_emit_arg8(parser->chunk, DM_OP_CALL_WITHPARENT, nparams);
}

static void pdot(dm_parser *parser) {
	if (!pmatch(parser, DM_TOKEN_IDENTIFIER)) {
		perr(parser, ". can only be followed by an identifier");
	}

	const char *var = parser->previous.begin;
	int len = parser->previous.len;
	int index = dm_chunk_index_of_string_constant(parser->chunk, var, len);
	if (index == -1) {
		dm_chunk_emit_constant(parser->chunk, dm_value_string_len(parser->dm, var, len));
	} else {
		dm_chunk_emit_constant_i(parser->chunk, index);
	}

	if (pmatch(parser, DM_TOKEN_EQUAL)) {
		pexpression(parser);
		dm_chunk_emit(parser->chunk, DM_OP_FIELDSET_S);
	} else if (pmatch(parser, DM_TOKEN_LEFT_PAREN)) {
		dm_chunk_emit(parser->chunk, DM_OP_FIELDGET_S_PUSHPARENT);
		pcall_with_parent(parser);
	} else {
		dm_chunk_emit(parser->chunk, DM_OP_FIELDGET_S);
	}
}

static void passign(dm_parser *parser) {
	perr(parser, "Can't assign to constant");
}

static void punary(dm_parser *parser) {
	dm_tokentype optype = parser->previous.type;
	pparse_precedence(parser, DM_PREC_UNARY);
	switch (optype) {
		case DM_TOKEN_MINUS: dm_chunk_emit(parser->chunk, DM_OP_NEGATE); break;
		case DM_TOKEN_BANG:
		case DM_TOKEN_NOT:   dm_chunk_emit(parser->chunk, DM_OP_NOT); break;
		default: return;
	}
}

static void pand(dm_parser *parser) {
	int jump_if_false_patch = dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP_IF_FALSE_OR_POP, 0);
	pexpression(parser);
	dm_chunk_patch_jump(parser->chunk, jump_if_false_patch);
}

static void por(dm_parser *parser) {
	int jump_if_false_patch = dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP_IF_TRUE_OR_POP, 0);
	pexpression(parser);
	dm_chunk_patch_jump(parser->chunk, jump_if_false_patch);
}

static void pbinary(dm_parser *parser) {
	dm_tokentype optype = parser->previous.type;
	dm_parserule *rule = pget_rule(optype);
	pparse_precedence(parser, (dm_precedence)(rule->precedence + 1));

	switch (optype) {
		case DM_TOKEN_PLUS:          dm_chunk_emit(parser->chunk, DM_OP_PLUS);         break;
		case DM_TOKEN_MINUS:         dm_chunk_emit(parser->chunk, DM_OP_MINUS);        break;
		case DM_TOKEN_STAR:          dm_chunk_emit(parser->chunk, DM_OP_MUL);          break;
		case DM_TOKEN_SLASH:         dm_chunk_emit(parser->chunk, DM_OP_DIV);          break;
		case DM_TOKEN_PERCENT:       dm_chunk_emit(parser->chunk, DM_OP_MOD);          break;
		case DM_TOKEN_BANG_EQUAL:    dm_chunk_emit(parser->chunk, DM_OP_NOTEQUAL);     break;
		case DM_TOKEN_EQUAL_EQUAL:   dm_chunk_emit(parser->chunk, DM_OP_EQUAL);        break;
		case DM_TOKEN_LESS:          dm_chunk_emit(parser->chunk, DM_OP_LESS);         break;
		case DM_TOKEN_LESS_EQUAL:    dm_chunk_emit(parser->chunk, DM_OP_LESSEQUAL);    break;
		case DM_TOKEN_GREATER:       dm_chunk_emit(parser->chunk, DM_OP_GREATER);      break;
		case DM_TOKEN_GREATER_EQUAL: dm_chunk_emit(parser->chunk, DM_OP_GREATEREQUAL); break;
		default: return;
	}
}

static bool pis_end_of_if_body(dm_parser *parser) {
	return pcheck(parser, DM_TOKEN_END) || pcheck(parser, DM_TOKEN_ELSIF) || pcheck(parser, DM_TOKEN_ELSE) || pcheck(parser, DM_TOKEN_EOF);
}

static int pelsif(dm_parser *parser) {
	int jump_to_end_addr;

	pexpression(parser);
	pconsume(parser, DM_TOKEN_THEN, "expect 'then' after if expression");

	int jump_if_false_patch = dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP_IF_FALSE, 0);

	if (!pis_end_of_if_body(parser)) {
		pexpression(parser);
		while (!pis_end_of_if_body(parser)) {
			if (pmatch(parser, DM_TOKEN_SEMICOLON)) {
				continue;
			}
			dm_chunk_emit(parser->chunk, DM_OP_POP);
			pexpression(parser);
		}
	} else {
		dm_chunk_emit(parser->chunk, DM_OP_NIL);
	}

	int jump_end_patch = dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, 0);
	dm_chunk_patch_jump(parser->chunk, jump_if_false_patch);

	if (pmatch(parser, DM_TOKEN_END)) {
		jump_to_end_addr = dm_chunk_current_address(parser->chunk);
		dm_chunk_emit(parser->chunk, DM_OP_NIL);
	} else if (pmatch(parser, DM_TOKEN_ELSE)) {
		if (!pmatch(parser, DM_TOKEN_END)) {
			pexpression(parser);
			while (!pmatch(parser, DM_TOKEN_END)) {
				if (pmatch(parser, DM_TOKEN_SEMICOLON)) {
					continue;
				}
				dm_chunk_emit(parser->chunk, DM_OP_POP);
				pexpression(parser);
			}
		} else {
			dm_chunk_emit(parser->chunk, DM_OP_NIL);
		}
		jump_to_end_addr = dm_chunk_current_address(parser->chunk);
	} else if (pmatch(parser, DM_TOKEN_ELSIF)) {
		jump_to_end_addr = pelsif(parser);
	} else {
		jump_to_end_addr = 0;
	}

	dm_chunk_patch_jump(parser->chunk, jump_end_patch);

	return jump_to_end_addr;
}

static void pif(dm_parser *parser) {
	(void) pelsif(parser);
}

static void pwhile(dm_parser *parser) {
	dm_chunk_emit(parser->chunk, DM_OP_NIL);
	int loop_start_addr = dm_chunk_current_address(parser->chunk);
	pexpression(parser);
	pconsume(parser, DM_TOKEN_DO, "expect 'do' after while expression");

	int jump_if_false_patch = dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP_IF_FALSE, 0);

	while (!pcheck(parser, DM_TOKEN_END) && !pcheck(parser, DM_TOKEN_EOF)) {
		if (pmatch(parser, DM_TOKEN_SEMICOLON)) {
			continue;
		} else if (pmatch(parser, DM_TOKEN_NEXT)) {
			dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, loop_start_addr);
		} else if (pmatch(parser, DM_TOKEN_BREAK)) {
			dm_chunk_emit(parser->chunk, DM_OP_FALSE);
			dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP_IF_FALSE, jump_if_false_patch);
		} else {
			dm_chunk_emit(parser->chunk, DM_OP_POP);
			pexpression(parser);
		}
	}

	pconsume(parser, DM_TOKEN_END, "expect end after while block");

	dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, loop_start_addr);
	dm_chunk_patch_jump(parser->chunk, jump_if_false_patch);
}

static void pfor(dm_parser *parser) {
	pexpression(parser);
	pconsume(parser, DM_TOKEN_COMMA, "expect ',' after init expression");
	int loop_start_addr = dm_chunk_current_address(parser->chunk);

	pexpression(parser);
	pconsume(parser, DM_TOKEN_COMMA, "expect ',' after condition expression");

	int jump_if_false_patch = dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP_IF_FALSE, 0);
	int loop_body_patch = dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, 0);

	int update_addr = dm_chunk_current_address(parser->chunk);
	pexpression(parser);
	dm_chunk_emit(parser->chunk, DM_OP_POP);
	pconsume(parser, DM_TOKEN_DO, "expect do in for expression");
	dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, loop_start_addr);

	dm_chunk_patch_jump(parser->chunk, loop_body_patch);
	while (!pcheck(parser, DM_TOKEN_END) && !pcheck(parser, DM_TOKEN_EOF)) {
		if (pmatch(parser, DM_TOKEN_SEMICOLON)) {
			continue;
		} else if (pmatch(parser, DM_TOKEN_NEXT)) {
			dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, update_addr);
		} else if (pmatch(parser, DM_TOKEN_BREAK)) {
			dm_chunk_emit(parser->chunk, DM_OP_FALSE);
			dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, jump_if_false_patch);
		} else {
			dm_chunk_emit(parser->chunk, DM_OP_POP);
			pexpression(parser);
		}
	}

	pconsume(parser, DM_TOKEN_END, "expect end after for block");

	dm_chunk_emit_jump(parser->chunk, DM_OP_JUMP, update_addr);
	dm_chunk_patch_jump(parser->chunk, jump_if_false_patch);
}

static void preturn(dm_parser *parser) {
	if (pcheck(parser, DM_TOKEN_END) || pcheck(parser, DM_TOKEN_ELSIF) || pcheck(parser, DM_TOKEN_ELSE) ||
		pmatch(parser, DM_TOKEN_SEMICOLON) || pmatch(parser, DM_TOKEN_EOF)) {
		dm_chunk_emit(parser->chunk, DM_OP_NIL);
	} else {
		pexpression(parser);
	}
	dm_chunk_emit(parser->chunk, DM_OP_RETURN);
}

static int parglist(dm_parser *parser, bool *takes_self) {
	int nargs = 0;
	*takes_self = false;
	pconsume(parser, DM_TOKEN_LEFT_PAREN, "parameter list must start with '('");
	if (!pcheck(parser, DM_TOKEN_RIGHT_PAREN)) {
		do {
			if (nargs == 0 && pmatch(parser, DM_TOKEN_SELF)) {
				*takes_self = true;
			} else {
				pconsume(parser, DM_TOKEN_IDENTIFIER, "function parameter must be an identifier");
			}
			dm_chunk_add_var(parser->chunk, parser->previous.begin, parser->previous.len);
			nargs++;
		} while (pmatch(parser, DM_TOKEN_COMMA));
	}
	pconsume(parser, DM_TOKEN_RIGHT_PAREN, "expect ')'");
	return nargs;
}

static dm_value pcompiler_end(dm_parser *parser, dm_value f, int nargs, bool takes_self) {
	dm_chunk_emit(parser->chunk, DM_OP_RETURN);
	if (dm_value_is(f, DM_TYPE_NIL)) {
		f = dm_value_function(parser->dm, parser->chunk, nargs, takes_self);
	} else {
		f.func_val->chunk = parser->chunk;
		f.func_val->nargs = nargs;
		f.func_val->takes_self = takes_self;
	}
	return f;
}

static void pfunction(dm_parser *parser) {
	int func_name = -1;
	if (pmatch(parser, DM_TOKEN_IDENTIFIER)) {
		func_name = dm_chunk_add_var(parser->chunk, parser->previous.begin, parser->previous.len);
	}

	dm_chunk *parent_chunk = parser->chunk;
	parser->chunk = malloc(sizeof(dm_chunk));
	dm_chunk_init(parser->chunk);
	dm_chunk_set_parent(parser->chunk, parent_chunk);

	bool takes_self;
	int nargs = parglist(parser, &takes_self);

	if (!pcheck(parser, DM_TOKEN_END)) {
		pexpression(parser);
		while (!pcheck(parser, DM_TOKEN_END) && !pcheck(parser, DM_TOKEN_EOF)) {
			dm_chunk_emit(parser->chunk, DM_OP_POP);
			pexpression(parser);
		}
	} else {
		dm_chunk_emit(parser->chunk, DM_OP_NIL);
	}

	pconsume(parser, DM_TOKEN_END, "expect 'end' at end of function");

	dm_value func = pcompiler_end(parser, dm_value_nil(), nargs, takes_self);
	parser->chunk = parent_chunk;
	dm_chunk_emit_constant(parser->chunk, func);
	if (func_name != -1) {
		dm_chunk_emit_arg16(parser->chunk, DM_OP_VARSET, func_name);
	}
}

static void pimport(dm_parser *parser) {
	pconsume(parser, DM_TOKEN_LEFT_PAREN, "expect '(' after import");
	pconsume(parser, DM_TOKEN_STRING, "expect string for import");
	pstring(parser);
	pconsume(parser, DM_TOKEN_RIGHT_PAREN, "expect ')' after import name");
	dm_chunk_emit(parser->chunk, DM_OP_IMPORT);
}

int dm_compile(dm_state *dm, dm_value *main, char *prog) {
	dm_lexer lexer = {prog, prog, 1};
	dm_parser parser = {dm, &lexer, NULL, {}, {}, false, false};

	if (main == NULL) {
		return 1;
	}

	if (dm_value_is(*main, DM_TYPE_NIL)) {
		parser.chunk = malloc(sizeof(dm_chunk));
		dm_chunk_init(parser.chunk);
	} else {
		parser.chunk = (dm_chunk*) main->func_val->chunk;
		dm_chunk_reset_code(parser.chunk);
	}

	pnext(&parser);

	if (!pcheck(&parser, DM_TOKEN_EOF) && !pcheck(&parser, DM_TOKEN_SEMICOLON)) {
		pexpression(&parser);
		while (!pmatch(&parser, DM_TOKEN_EOF)) {
			if (pmatch(&parser, DM_TOKEN_SEMICOLON)) {
				continue;
			}
			dm_chunk_emit(parser.chunk, DM_OP_POP);
			pexpression(&parser);
		}
	} else {
		dm_chunk_emit(parser.chunk, DM_OP_NIL);
	}

	if (parser.had_error) {
		return 1;
	}

	*main = pcompiler_end(&parser, *main, 0, false);
	return 0;
}


dm_parserule rules[] = {
	[DM_TOKEN_LEFT_PAREN]    = {pgrouping, pcall,    DM_PREC_CALL},
	[DM_TOKEN_RIGHT_PAREN]   = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_LEFT_BRACE]    = {ptablelit, parrayget,DM_PREC_CALL},
	[DM_TOKEN_RIGHT_BRACE]   = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_LEFT_BRACKET]  = {parraylit, parrayget,DM_PREC_CALL},
	[DM_TOKEN_RIGHT_BRACKET] = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_COMMA]         = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_DOT]           = {NULL,      pdot,     DM_PREC_CALL},
	[DM_TOKEN_MINUS]         = {punary,    pbinary,  DM_PREC_TERM},
	[DM_TOKEN_PLUS]          = {NULL,      pbinary,  DM_PREC_TERM},
	[DM_TOKEN_SEMICOLON]     = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_SLASH]         = {NULL,      pbinary,  DM_PREC_FACTOR},
	[DM_TOKEN_PERCENT]       = {NULL,      pbinary,  DM_PREC_FACTOR},
	[DM_TOKEN_STAR]          = {NULL,      pbinary,  DM_PREC_FACTOR},
	[DM_TOKEN_BANG]          = {punary,    NULL,     DM_PREC_UNARY},
	[DM_TOKEN_BANG_EQUAL]    = {NULL,      pbinary,  DM_PREC_COMPARISON},
	[DM_TOKEN_EQUAL]         = {NULL,      passign,  DM_PREC_ASSIGNEMENT},
	[DM_TOKEN_EQUAL_EQUAL]   = {NULL,      pbinary,  DM_PREC_EQUALITY},
	[DM_TOKEN_GREATER]       = {NULL,      pbinary,  DM_PREC_COMPARISON},
	[DM_TOKEN_GREATER_EQUAL] = {NULL,      pbinary,  DM_PREC_COMPARISON},
	[DM_TOKEN_LESS]          = {NULL,      pbinary,  DM_PREC_COMPARISON},
	[DM_TOKEN_LESS_EQUAL]    = {NULL,      pbinary,  DM_PREC_COMPARISON},
	[DM_TOKEN_IDENTIFIER]    = {pident,    NULL,     DM_PREC_NONE},
	[DM_TOKEN_STRING]        = {pstring,   NULL,     DM_PREC_NONE},
	[DM_TOKEN_INTEGER]       = {pinteger,  NULL,     DM_PREC_NONE},
	[DM_TOKEN_FLOAT]         = {pfloating, NULL,     DM_PREC_NONE},
	[DM_TOKEN_AND]           = {NULL,      pand,     DM_PREC_AND},
	[DM_TOKEN_DO]            = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_ELSE]          = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_FALSE]         = {pboolean,  NULL,     DM_PREC_NONE},
	[DM_TOKEN_FOR]           = {pfor,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_FUNCTION]      = {pfunction, NULL,     DM_PREC_NONE},
	[DM_TOKEN_IF]            = {pif,       NULL,     DM_PREC_NONE},
	[DM_TOKEN_NIL]           = {pnil,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_NOT]           = {punary,    NULL,     DM_PREC_UNARY},
	[DM_TOKEN_OR]            = {NULL,      por,      DM_PREC_OR},
	[DM_TOKEN_RETURN]        = {preturn,   NULL,     DM_PREC_NONE},
	[DM_TOKEN_THEN]          = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_TRUE]          = {pboolean,  NULL,     DM_PREC_NONE},
	[DM_TOKEN_WHILE]         = {pwhile,    NULL,     DM_PREC_NONE},
	[DM_TOKEN_BREAK]         = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_NEXT]          = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_SELF]          = {pself,     NULL,     DM_PREC_NONE},
	[DM_TOKEN_IMPORT]        = {pimport,   NULL,     DM_PREC_NONE},
	[DM_TOKEN_GLOBAL]        = {pglobal,   NULL,     DM_PREC_NONE},

	[DM_TOKEN_ERROR]         = {NULL,      NULL,     DM_PREC_NONE},
	[DM_TOKEN_EOF]           = {NULL,      NULL,     DM_PREC_NONE}
};

/*
prog ::= {declaration}
declaration ::= import | statement | functiondef
import ::= 'import(' string ')'
statement ::= 	var '=' exp |
				'while' exp 'do' {statement | 'break' | 'next'} 'end' |
				'if' exp 'then' {statement} {'elsif' exp 'then' {statement}} ['else' {statement}] 'end' |
				'for' name '=' exp ',' exp [',' exp] 'do' {statement | 'break' | 'next'} 'end' |
				exp

name ::= identifier
functiondef ::= 'function' name '(' [parlist] ')' functionbody 'end'
parlist ::= name {',' name}
functionbody ::= {statement | return exp}
var ::= name | prefixexp '[' exp ']' | prefixexp '.' name
prefixexp ::= var | functioncall | '(' exp ')'
functioncall ::= prefixexp '(' [arglist] ')'
exp ::=	'nil' |
		'false' |
		'true' |
		number |
		string |
		prefixexp |
		arrayconstructor |
		tableconstructor |
		functioncall |
		exp binop exp |
		unop exp

arglist ::= exp {',' exp}
arrayconstructor ::= '[' [arraylist] ']'
arraylist ::= exp {',' exp} [',']
tableconstructor ::= '{' [tablelist] '}'
tablelist ::= exp '=>' exp {',' exp '=>' exp} [',']
binop ::= '+' | '-' | '*' | '/' | '%' | '<' | '<=' | '>' | '>=' | '==' | '!=' | 'and' | 'or'
unop ::= '-' | 'not'

primitives are nil, bool, int, float, string, array, table, function
*/
