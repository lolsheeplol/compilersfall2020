#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "value.h"
#include "node.h"

// Initialize value by setting the kind field and clearing
// all of the data fields.
static void val_init(struct Value *val, enum ValueKind kind) {
  val->kind = kind;
  val->ival = 0L;
  val->fn = NULL;
  val->intrinsic_fn = NULL;
}

struct Value val_create_void(void) {
  struct Value val;
  val_init(&val, VAL_VOID);
  return val;
}

struct Value val_create_error(void) {
  struct Value val;
  val_init(&val, VAL_ERROR);
  return val;
}

struct Value val_create_ival(long ival) {
  struct Value val;
  val_init(&val, VAL_INT);
  val.ival = ival;
  return val;
}

struct Value val_create_true() {
    return val_create_ival(1);
}

struct Value val_create_false() {
    return val_create_ival(0);
}

struct Value val_create_fn(struct Function *fn) {
  assert(fn != NULL);
  struct Value val;
  val_init(&val, VAL_FN);
  val.fn = fn;
  return val;
}

struct Value val_create_intrinsic(IntrinsicFunction *intrinsic_fn) {
  assert(intrinsic_fn != NULL);
  struct Value val;
  val_init(&val, VAL_INTRINSIC);
  val.intrinsic_fn = intrinsic_fn;
  return val;
}

char *val_stringify(struct Value val) {
  char *s = xmalloc(128);
  switch (val.kind) {
  case VAL_VOID:
    strcpy(s, "<void>");
    break;
  case VAL_ERROR:
    strcpy(s, "<error>");
    break;
  case VAL_INT:
    sprintf(s, "%ld", val.ival);
    break;
  case VAL_FN:
    strcpy(s, "<function>");
    break;
  case VAL_INTRINSIC:
    strcpy(s, "<intrinsic>");
    break;
  default:
    strcpy(s, "<unknown>");
    break;
  }

  return s;
}

struct Function function_create(struct Node *ast) {
    // ast will contain function name, arg list, and statements
    struct Function func;
    func.ast = ast;
    return func;
}

int fn_get_num_args(struct Function *fn) {
    return node_get_num_kids(node_get_kid(fn->ast, 1));
}
