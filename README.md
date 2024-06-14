# Diamond Programming Language

## neovim syntax highlighting

Copy the `docs/dm.vim` file to `~/.config/nvim/ftdetect/dm.vim`, so neovim will detect files
that end with `.dm` as diamond files.

Copy the `docs/diamond.vim` file to `~/.config/nvim/after/syntax/diamond.vim`, so neovim will
add the syntax rules to diamond files.

Copy the `docs/diamond.lua` file to `~/.config/nvim/after/ftplugin/diamond.lua`, so neovim will
start the diamond lsp server when a diamond file is opened.
One may has to adjust the command path the server is at.

## bash completions

Copy the `docs/diamond` file to `~/.local/share/bash-completion/completions/`, so bash will auto
complete the diamond command.

## unofficial and maybe uncomplete/incorrect ebnf

```
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
tablelist ::= exp ':' exp {',' exp ':' exp} [',']
binop ::= '+' | '-' | '*' | '/' | '%' | '<' | '<=' | '>' | '>=' | '==' | '!=' | 'and' | 'or'
unop ::= '-' | 'not'

primitives are nil, bool, int, float, string, array, table, function
```

## future ideas and things to test

- test register based vm
- test fixed size opcodes
- make opcodes for loops to use fewer instructions
- ffi that automatically reads header files (and shared libraries?)
