%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "grammar_symbols.h"
#include "util.h"
#include "ast.h"
#include "node.h"

struct Node *g_program;

void yyerror(const char *fmt, ...);

int yylex(void);
%}

%union {
  struct Node *node;
}

%token<node> TOK_IDENT TOK_INT_LITERAL

%token<node> TOK_PROGRAM TOK_BEGIN TOK_END TOK_CONST TOK_TYPE TOK_VAR
%token<node> TOK_ARRAY TOK_OF TOK_RECORD TOK_DIV TOK_MOD TOK_IF
%token<node> TOK_THEN TOK_ELSE TOK_REPEAT TOK_UNTIL TOK_WHILE TOK_DO
%token<node> TOK_READ TOK_WRITE

%token<node> TOK_ASSIGN
%token<node> TOK_SEMICOLON TOK_EQUALS TOK_COLON TOK_PLUS TOK_MINUS TOK_TIMES
%token<node> TOK_HASH TOK_LT TOK_GT TOK_LTE TOK_GTE TOK_LPAREN
%token<node> TOK_RPAREN TOK_LBRACKET TOK_RBRACKET TOK_DOT TOK_COMMA

%type<node> program
%type<node> opt_declarations declarations declaration
/*
%type<node> constdecl constdefn_list constdefn
%type<node> typedecl typedefn_list typedefn
*/
%type<node> vardecl type vardefn_list vardefn
%type<node> opt_instructions instructions instruction
/*
%type<node> expression term factor primary
%type<node> assignstmt ifstmt repeatstmt whilestmt condition writestmt readstmt
%type<node> designator expression_list
*/
%type<node> identifier_list

%%

program
    : TOK_PROGRAM TOK_IDENT TOK_SEMICOLON opt_declarations TOK_BEGIN opt_instructions TOK_END TOK_DOT
        { $$ = g_program = node_build2(NODE_program, $4, $6); }
    ;

opt_declarations
    : declarations
    | /* epsilon */
    ;

declarations
    : declarations declaration { $$ = $1, node_add_kid($1, $2); }
    | declaration { $$ = node_build1(NODE_declarations, $1); }
    ;

declaration
    : vardecl { $$ = node_build1(NODE_vardecl, $1); }
    ;

vardecl
    : TOK_VAR vardefn_list { $$ = $2; }
    ;

vardefn_list
    : vardefn_list vardefn { $$ = $2; }
    | vardefn { $$ = $1; }
    ;

vardefn
    : identifier_list TOK_COLON type TOK_SEMICOLON { $$ = node_build2(NODE_vardefn, $1, $3); }
    ;

identifier_list
    : identifier_list TOK_COMMA TOK_IDENT { $$ = $1; node_add_kid($1, $3); }
    | TOK_IDENT { $$ = node_build1(NODE_identifier_list, $1); }
    ;

type
    /* Fixme: type */
    : TOK_IDENT { $$ = node_build1(NODE_type, $1); }
    ;

opt_instructions
    : instructions { $$ = node_build1(NODE_instructions, $1); }
    | /* epsilon */ { $$ = node_build0(NODE_instructions); }
    ;

instructions
    : instructions instruction { $$ = node_build1(NODE_instruction, $1); }
    | instruction { $$ = node_build1(NODE_instruction, $1); }
    ;

instruction
    : TOK_BEGIN  { $$ = $1; }
    ;

%%

void yyerror(const char *fmt, ...) {
  extern char *g_srcfile;
  extern int yylineno, g_col;

  va_list args;

  va_start(args, fmt);
  fprintf(stderr, "%s:%d:%d: Error: ", g_srcfile, yylineno, g_col);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);

  exit(1);
}
