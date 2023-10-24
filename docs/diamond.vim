" Vim syntax file
" Language:     Diamond programming language
" Maintainer:   Oskar Munz
" Last Change:  24.10.2023

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn case match

" syncing method
syn sync minlines=1000

" syn keyword dmMetaMethod __add __sub __mul __div __pow __unm __concat
" syn keyword dmMetaMethod __eq __lt __le

" catch errors caused by wrong parenthesis and wrong curly brackets or
" keywords placed outside their respective blocks
syn region dmParen transparent start='(' end=')' contains=TOP,dmParenError
syn match  dmParenError ")"
syn match  dmError "}"
syn match  dmError "\<\%(end\|else\|elsif\|then\)\>"

" Function declaration
syn region dmFunctionBlock transparent matchgroup=dmFunction start="\<function\>" end="\<end\>" contains=TOP
" else
syn keyword dmCondElse matchgroup=dmCond contained containedin=dmCondEnd else
" then ... end
syn region dmCondEnd contained transparent matchgroup=dmCond start="\<then\>" end="\<end\>" contains=TOP
" elsif ... then
syn region dmCondElsif contained containedin=dmCondEnd transparent matchgroup=dmCond start="\<elsif\>" end="\<then\>" contains=TOP
" if ... then
syn region dmCondStart transparent matchgroup=dmCond start="\<if\>" end="\<then\>"me=e-4 contains=TOP nextgroup=dmCondEnd skipwhite skipempty
" do ... end
syn region dmBlock transparent matchgroup=dmStatement start="\<do\>" end="\<end\>" contains=TOP
" while ... do
syn region dmWhile transparent matchgroup=dmRepeat start="\<while\>" end="\<do\>"me=e-2 contains=TOP nextgroup=dmBlock skipwhite skipempty

" for ... do
syn region dmFor transparent matchgroup=dmRepeat start="\<for\>" end="\<do\>"me=e-2 contains=TOP nextgroup=dmBlock skipwhite skipempty

" other keywords
syn keyword dmStatement return break

" operators
syn keyword dmOperator and or not

syn match dmSymbolOperator "[<>=*/%+-]\|\.\{2,3}"

" comments
syn keyword dmTodo            contained TODO FIXME XXX
syn match   dmComment         "#.*$" contains=dmTodo,@Spell

" first line may start with #!
syn match dmComment "\%^#!.*"

syn keyword dmConstant nil true false

" strings
syn match dmSpecial contained #\\[\\abfnrtv'"[\]]\|\\[x]\x\{,3}\|\\[[:digit:]]\{,3}#
syn region dmString matchgroup=dmStringDelimiter start=+"+ end=+"+ skip=+\\\\\|\\"+ contains=dmSpecial,@Spell

" integer number
syn match dmNumber "\<\d\+\>"
" floating point number, with dot, optional exponent
syn match dmNumber  "\<\d\+\.\d*"
" hex and binary numbers
syn match dmNumber "\<0[x]\x\+\>"
syn match dmNumber "\<0[b]\x\+\>"

" tables
syn region dmTableBlock transparent matchgroup=dmTable start="{" end="}" contains=TOP,dmStatement

" example for builtin functions
" syn keyword dmFunc print
" syn match dmFunc /\<io\.print\>/

" Define the default highlighting.
" Only when an item doesn't have highlighting yet

hi def link dmStatement        Statement
hi def link dmRepeat           Repeat
hi def link dmFor              Repeat
hi def link dmString           String
hi def link dmString2          String
hi def link dmStringDelimiter  dmString
hi def link dmNumber           Number
hi def link dmOperator         Keyword
hi def link dmConstant         Constant
hi def link dmCond             Conditional
hi def link dmCondElse         Conditional
hi def link dmFunction         Keyword
hi def link dmMetaMethod       Function
hi def link dmComment          Comment
hi def link dmCommentDelimiter dmComment
hi def link dmTodo             Todo
" hi def link dmTable            Structure
hi def link dmError            Error
hi def link dmParenError       Error
hi def link dmSpecial          SpecialChar
hi def link dmFunc             Identifier
hi def link dmLabel            Label


let b:current_syntax = "diamond"

let &cpo = s:cpo_save
unlet s:cpo_save
" vim: et ts=8 sw=2
