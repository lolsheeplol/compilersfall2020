#include <cstdio>
#include <cstdlib>
#include <unistd.h> // for getopt
#include "node.h"
#include "util.h"
#include "grammar_symbols.h"
#include "ast.h"
#include "treeprint.h"
#include "context.h"

extern "C" {
int yyparse(void);
void lexer_set_source_file(const char *filename);
}

void print_usage(void) {
  err_fatal(
    "Usage: compiler [options] <filename>\n"
    "Options:\n"
    "   -p    print AST\n"
    "   -g    print AST as graph (DOT/graphviz)\n"
    "   -s    print symbol table information\n"
    "   -h    print high-level instruction translation\n"
    "   -o    perform optimization on emitted assembly\n"
  );
}

enum Mode {
  PRINT_AST,
  PRINT_AST_GRAPH,
  PRINT_SYMBOL_TABLE,
  PRINT_HINS,
  OPTIMIZE,
  COMPILE,
};

int main(int argc, char **argv) {
  extern FILE *yyin;
  extern struct Node *g_program;

  int mode = COMPILE;
  int opt;

  while ((opt = getopt(argc, argv, "pgsho")) != -1) {
    switch (opt) {
    case 'p':
      mode = PRINT_AST;
      break;

    case 'g':
      mode = PRINT_AST_GRAPH;
      break;

    case 's':
      mode = PRINT_SYMBOL_TABLE;
      break;

    case 'h':
      mode = PRINT_HINS;
      break;

    case 'o':
      mode = OPTIMIZE;
      break;

    case '?':
      print_usage();
      break;

    default:
      mode = COMPILE;
      break;
    }
  }

  if (optind >= argc) {
    print_usage();
  }

  const char *filename = argv[optind];

  yyin = fopen(filename, "r");
  if (!yyin) {
    err_fatal("Could not open input file \"%s\"\n", filename);
  }
  lexer_set_source_file(filename);

  yyparse();
  struct Context *ctx = context_create(g_program);

  if (mode == PRINT_AST) {
    treeprint(g_program, ast_get_tag_name);
  } else if (mode == PRINT_AST_GRAPH) {
    ast_print_graph(g_program);
  } else if (mode == PRINT_SYMBOL_TABLE) {
      context_set_flag(ctx, 's');
  } else if (mode == PRINT_HINS) {
      context_set_flag(ctx, 'h');
  } else if (mode == OPTIMIZE) {
      context_set_flag(ctx, 'o');
      context_set_flag(ctx, 'c');
  } else {
      // mode is only compile
      context_set_flag(ctx, 'c');
  }

  context_build_symtab(ctx);
  context_gen_code(ctx);

  return 0;
}
