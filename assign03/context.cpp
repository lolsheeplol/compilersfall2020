#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>
#include <set>
#include "util.h"
#include "cpputil.h"
#include "node.h"
#include "type.h"
#include "symbol.h"
#include "symtab.h"
#include "ast.h"
#include "astvisitor.h"
#include "context.h"

////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////

struct Context {
private:
    Node *root;
    SymbolTable *global;
    bool flag_print;

public:
  Context(struct Node *ast);
  ~Context();

  void set_flag(char flag);

  void build_symtab();
  void print_err(Node* node, const char *fmt, ...);
};

// Known issues:
// bug: Modifying enum start value in enums Kind and RealType change the behavior of record printing (???)
// unimplemented: Consts can be dereferenced and used in subsequent declarations
// unimplemented: Consts can be checked for variable references and throw an error
// unimplemented: array and field references are not being type checked
// unimolemented: READ and WRITE operands are not being checked
//

class SymbolTableBuilder : public ASTVisitor {
private:
    SymbolTable* scope;
    Type* integer_type;
    Type* char_type;
public:

    void print_err(Node* node, const char* fmt, ...) {

        SourceInfo info = node_get_source_info(node);
        //std::string error_start_str = cpputil::format("%s:%d:%d: Error: %s", info.filename, info.line, info.col, fmt);

        fprintf(stderr, "%s:%d:%d: Error: ", info.filename, info.line, info.col);

        va_list args;
        va_start(args, fmt);
        err_fatal(fmt, args);
        va_end(args);
    }

    SymbolTable* get_symtab() {
        return scope;
    }

    SymbolTableBuilder(SymbolTable* symbolTable) {
        scope = symbolTable;
        integer_type = type_create_primitive("INTEGER");
        char_type = type_create_primitive("CHAR");
    }

    void visit_constant_def(struct Node *ast) override {
        recur_on_children(ast);

        // find out type on right
        Node* right = node_get_kid(ast, 1);
        Type* type = right->get_type();

        // get left identifier
        Node* left = node_get_kid(ast, 0);
        const char* name = node_get_str(left);

        // set entry in symtab for name, type
        Symbol* sym = symbol_create(name, type, CONST);
        if (scope->s_exists(name)) {
            SourceInfo info = node_get_source_info(left);
            err_fatal("%s:%d:%d: Error: Name '%s' is already defined\n", info.filename, info.line, info.col, name);
        } else {
            scope->insert(*sym);
        }
    }

    void visit_var_def(struct Node *ast) override {
        recur_on_children(ast);

        // find out type on right
        Node* right = node_get_kid(ast, 1);
        Type* type = right->get_type();

        // get left identifier(s)
        Node* left = node_get_kid(ast, 0);
        int num_kids = node_get_num_kids(left);

        for (int i = 0; i < num_kids; i++) {
            Node* id = node_get_kid(left, i);
            const char* name = node_get_str(id);
            // set entry in symtab for name, type
            Symbol* sym = symbol_create(name, type, VARIABLE);
            if (scope->s_exists(name)) {
                SourceInfo info = node_get_source_info(left);
                err_fatal("%s:%d:%d: Error: Name '%s' is already defined\n", info.filename, info.line, info.col, name);
            } else {
                scope->insert(*sym);
            }
        }
    }

    void visit_type_def(struct Node *ast) override {
        recur_on_children(ast);

        // find out type on right
        Node* right = node_get_kid(ast, 1);
        Type* type = right->get_type();

        // get left identifier
        Node* left = node_get_kid(ast, 0);
        const char* name = node_get_str(left);

        // set entry in symtab for name, type
        Symbol* sym = symbol_create(name, type, TYPE);
        if (scope->s_exists(name)) {
            SourceInfo info = node_get_source_info(left);
            err_fatal("%s:%d:%d: Error: Name '%s' is already defined\n", info.filename, info.line, info.col, name);
        } else {
            scope->insert(*sym);
        }
    }

    void visit_named_type(struct Node *ast) override {
        Node* type = node_get_kid(ast, 0);

        const char* type_str = node_get_str(type);
        Type* named_type;

        if (std::strcmp(type_str, "INTEGER") == 0) {
            named_type = integer_type;
        } else if (std::strcmp(type_str, "CHAR") == 0) {
            named_type = char_type;
        } else {
            // perform lookup
            if (scope->s_exists(type_str)) {
                Symbol typeSymbol = scope->lookup(type_str);
                named_type = typeSymbol.get_type();
            } else {
                SourceInfo info = node_get_source_info(type);
                err_fatal("%s:%d:%d: Error: Unknown type '%s'\n", info.filename, info.line, info.col, type_str);
                named_type = nullptr;
            }
        }
        // set the type of the current node
        ast->set_type(named_type);
    }

    void visit_array_type(struct Node *ast) override {
        recur_on_children(ast);

        Node* right = node_get_kid(ast, 1);
        Type* type = right->get_type();

        // get left size
        Node* left = node_get_kid(ast, 0);
        long size = node_get_ival(left);

        Type* arrayType = type_create_array(size, type);
        ast->set_type(arrayType);
    }

    void visit_record_type(struct Node *ast) override {
        // Records will have their own "scope"
        // records store their fields in an ordered list aka <vector>
        // records print their "inner fields" before printing the record type line

        SymbolTable* nestedSymTab = new SymbolTable(scope);
        scope = nestedSymTab;

        recur_on_children(ast); // will populate the nested scope with values

        scope = scope->get_parent();    // bring it back to parent scope

        Type* recordType = type_create_record(nestedSymTab);
        ast->set_type(recordType);
    }

    void visit_var_ref(struct Node *ast) override {
        Node* ident = node_get_kid(ast, 0);
        const char* varname = node_get_str(ident);

        if (scope->s_exists(varname)) {
            // if name references a TYPE or RECORD, is also wrong
        } else {
            SourceInfo info = node_get_source_info(ident);
            err_fatal("%s:%d:%d: Error: Undefined variable '%s'\n", info.filename, info.line, info.col, varname);
        }
        Symbol sym = scope->lookup(varname);
        ast->set_str(varname);
        ast->set_type(sym.get_type());
        ast->set_source_info(node_get_source_info(ident));
    }

    void visit_identifier(struct Node *ast) override {
        ASTVisitor::visit_identifier(ast);

        // TODO: Consts can be deferenced and have a long value set to the node
        const char* identifier = node_get_str(ast);
    }

    void visit_int_literal(struct Node *ast) override {
        // set literal value
        ast->set_ival(strtol(node_get_str(ast), nullptr, 10));
        // set type to integer
        ast->set_type(integer_type);
    }
};

////////////////////////////////////////////////////////////////////////
// Context class implementation
////////////////////////////////////////////////////////////////////////

Context::Context(struct Node *ast) {
    root = ast;
    global = new SymbolTable(nullptr);
    flag_print = false;
}

Context::~Context() {
}

void Context::set_flag(char flag) {
  if (flag == 's') {
      flag_print = true;
  }
}

void Context::build_symtab() {

    // give symtabbuilder a symtab in constructor?
    SymbolTableBuilder *visitor = new SymbolTableBuilder(global);
    visitor->visit(root);

    if (flag_print) {
      // print symbol table
      visitor->get_symtab()->print_sym_tab();
    }
}

// TODO: implementation of additional Context member functions

// TODO: implementation of member functions for helper classes

////////////////////////////////////////////////////////////////////////
// Context API functions
////////////////////////////////////////////////////////////////////////

struct Context *context_create(struct Node *ast) {
  return new Context(ast);
}

void context_destroy(struct Context *ctx) {
  delete ctx;
}

void context_set_flag(struct Context *ctx, char flag) {
  ctx->set_flag(flag);
}

void context_build_symtab(struct Context *ctx) {
  ctx->build_symtab();
}

void context_check_types(struct Context *ctx) {
}
