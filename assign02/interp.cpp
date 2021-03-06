#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include "cpputil.h"
#include "util.h"
#include "node.h"
#include "grammar_symbols.h"
#include "interp.h"

////////////////////////////////////////////////////////////////////////
// Interp class
////////////////////////////////////////////////////////////////////////

struct Environment {
    std::map<std::string, Value> vars;
    struct Environment *parent;

public:
    Environment();
    ~Environment();
    void init_val(const char* name);
    Value find_val(const char* name);
    bool val_exists(const char*name);
    void set_val(const char*, Value val);
};

Environment::Environment() {

}

Environment::~Environment() {

}

void Environment::init_val(const char* name) {
    if (val_exists(name)) {
        err_fatal("Error: Variable '%s' cannot be redefined\n", name);
    }
    vars[name] = val_create_ival(0);
}

struct Value Environment::find_val(const char* name) {
    std::map<std::string, Value>::const_iterator i = vars.find(name);
    if (i == vars.end()) {
        // did not find the value
        if (parent == nullptr) {
            err_fatal("Undefined variable '%s'\n", name);
        }
        return parent->find_val(name);
    }
    return i->second;
}

bool Environment::val_exists(const char* name) {
    std::map<std::string, Value>::const_iterator i = vars.find(name);
    if (i == vars.end()) {
        return false;
    }
    return true;
}

void Environment::set_val(const char* name, Value val) {
    if (val.kind == VAL_INT) {
        if (val_exists(name)) {
            vars[name] = val;
        } else if (parent != nullptr && parent->val_exists(name)) {
            parent->set_val(name, val);
        } else {
            err_fatal("Error: Variable '%s' has not been declared\n", name);
        }
    } else {
        vars[name] = val;
    }
}

struct Interp {
private:
  struct Node *m_tree;

public:
  Interp(struct Node *t);
  ~Interp();

  struct Value exec();

private:
  //struct Value eval(struct Node *n);
  struct Value eval_all(struct Node *statements, Environment *env);
  struct Value eval_st(struct Node *statement, Environment *env);
  struct Value eval_fn(struct Function *fn, struct Node *args, Environment *parent);
  bool val_is_truthy(Value val);
};

Interp::Interp(struct Node *t) : m_tree(t) {
}

Interp::~Interp() {
}

struct Value Interp::exec() {
    struct Value result = val_create_void();
    struct Node *unit = m_tree;
    struct Environment *global = env_create(nullptr);

    // evaluation of statements or functions should go on stack
    // previous values may affect later values

    result = eval_all(unit, global);
    return result;
}

struct Value Interp::eval_all(struct Node *statements, Environment *env) {
    struct Value result = val_create_void();
    int num_stmts = node_get_num_kids(statements);
    int index = 0;

    if (num_stmts > 0) {
        while (index < num_stmts) {
            struct Node *statement = node_get_kid(statements, index);
            result = eval_st(statement, env);
            index++;
        }
    }

    return result;
}

bool Interp::val_is_truthy(Value val) {
    if (val.kind == VAL_FN) {
        return true;
    }
    return val.ival >= 1;
}

struct Value Interp::eval_fn(struct Function *fn, struct Node* args, Environment *parent) {
    struct Value result = val_create_void();
    struct Node* func = fn->ast;
    struct Environment* local = env_create(parent);

    const char* func_name = node_get_str(node_get_kid(func, 0));
    struct Node *expected_args = node_get_kid(func, 1);
    struct Node *statements = node_get_kid(func, 2);

    int num_args = node_get_num_kids(expected_args);
    int num_actual = node_get_num_kids(args);

    // check number of args
    if (num_args != num_actual) {
        err_fatal("Error: Invalid number of arguments for function '%s'\n", func_name);
    }

    int index = 0;
    while (index < num_args) {
        struct Node* arg = node_get_kid(expected_args, index);
        const char* name = node_get_str(arg);

        struct Node *real = node_get_kid(args, index);

        local->init_val(name);
        local->set_val(name, eval_st(real, parent));

        index++;
    }

    result = eval_all(statements, local);

    return result;
}

struct Value Interp::eval_st(struct Node *statement, Environment *env) {
    int tag = node_get_tag(statement);

    if (tag == NODE_INT_LITERAL) {
        return val_create_ival(strtol(node_get_str(statement), nullptr, 10));
    }

    if (tag == NODE_IDENTIFIER) {
        const char* name = node_get_str(statement);
        return env->find_val(name);
    }

    if (tag == NODE_AST_VAR_DEC) {
        int num_kids = node_get_num_kids(statement);
        int index = 0;
        while (index < num_kids) {
            struct Node *var = node_get_kid(statement, index);
            const char* name = node_get_str(var);
            env->init_val(name); // for all declared variables, assign to 0;
            index++;
        }
        return val_create_void();
    }

    if (tag == NODE_AST_IF) {
        struct Node *condition = node_get_kid(statement, 0);
        struct Node *if_clause = node_get_kid(statement, 1);
        struct Node *else_clause = nullptr;
        if (node_get_num_kids(statement) == 3) {    // there is an else clause
            else_clause = node_get_kid(statement, 2);
        }

        if (val_is_truthy(eval_st(condition, env))) {
            eval_all(if_clause, env);
        } else {
            if (else_clause) {
                eval_all(else_clause, env);
            }
        }
        // the result of evaluating an if or if/else statement is a VAL_VOID value
        return val_create_void();
    }

    if (tag == NODE_AST_WHILE) {
        struct Node *condition = node_get_kid(statement, 0);
        struct Node *while_clause= node_get_kid(statement, 1);

        while (val_is_truthy(eval_st(condition, env))) {
            eval_all(while_clause, env);
        }

        return val_create_void();
    }

    if (tag == NODE_AST_FUNC_DEF) {
        Function func = function_create(statement);
        Value function = val_create_fn(&func);
        const char* func_name = node_get_str(node_get_kid(statement, 0));
        env->set_val(func_name, function);
        return val_create_void();
    }

    if (tag == NODE_AST_FUNC_CALL) {
        struct Node *function = node_get_kid(statement, 0);
        const char *func_name = node_get_str(function);
        Value func = env->find_val(func_name);
        if (func.kind != VAL_FN) {
            err_fatal("Error: Cannot call '%s' because it isn’t a function\n", func_name);
        }
        struct Node* args = node_get_kid(statement, 1);
        return eval_fn(func.fn, args, env);
    }

    // left and right operands follow
    struct Node *left = node_get_kid(statement, 0);
    struct Node *right = node_get_kid(statement, 1);

    if (tag == NODE_AST_ASSIGN) {
        const char* varname = node_get_str(left);
        struct Value val = eval_st(right, env);
        if (val.kind != VAL_INT) {
            err_fatal("Error: Cannot assign non-int value to variable '%s'\n", varname);
        }

        env->set_val(varname, val);

        // Feels like assignment should be a VAL_VOID type
        // public test function01.in says otherwise though
        // TODO: Confirm with prof
        return val;
    }

    switch (tag) {
        case NODE_AST_PLUS:
            return val_create_ival(eval_st(left, env).ival + eval_st(right, env).ival);
        case NODE_AST_MINUS:
            return val_create_ival(eval_st(left, env).ival - eval_st(right, env).ival);
        case NODE_AST_TIMES:
            return val_create_ival(eval_st(left, env).ival * eval_st(right, env).ival);
        case NODE_AST_DIVIDE:
            if (eval_st(right, env).ival == 0) {
                err_fatal("Error: Cannot divide by 0\n");
            }
            return val_create_ival(eval_st(left, env).ival / eval_st(right, env).ival);
        case NODE_AST_AND:
            if (val_is_truthy(eval_st(left, env)) && val_is_truthy(eval_st(right, env))) {
                return val_create_true();
            }
            return val_create_false();
        case NODE_AST_OR:
            if (val_is_truthy(eval_st(left, env)) || val_is_truthy(eval_st(right, env))) {
                return val_create_true();
            }
            return val_create_false();
        case NODE_AST_EQ:
            if (eval_st(left, env).ival == eval_st(right, env).ival) {
                return val_create_true();
            }
            return val_create_false();
        case NODE_AST_NE:
            if (eval_st(left, env).ival != eval_st(right, env).ival) {
                return val_create_true();
            }
            return val_create_false();
        case NODE_AST_LT:
            if (eval_st(left, env).ival < eval_st(right, env).ival) {
                return val_create_true();
            }
            return val_create_false();
        case NODE_AST_LE:
            if (eval_st(left, env).ival <= eval_st(right, env).ival) {
                return val_create_true();
            }
            return val_create_false();
        case NODE_AST_GT:
            if (eval_st(left, env).ival > eval_st(right, env).ival) {
                return val_create_true();
            }
            return val_create_false();
        case NODE_AST_GE:
            if (eval_st(left, env).ival >= eval_st(right, env).ival) {
                return val_create_true();
            }
            return val_create_false();
        default:
            err_fatal("Unknown operator: %d\n", tag);
            return val_create_error();
    }
}

////////////////////////////////////////////////////////////////////////
// API functions
////////////////////////////////////////////////////////////////////////

struct Interp *interp_create(struct Node *t) {
    Interp *interp = new Interp(t);
    return interp;
}

void interp_destroy(struct Interp *interp) {
    delete interp;
}

struct Value interp_exec(struct Interp *interp) {
    return interp->exec();
}

struct Environment *env_create(struct Environment *parent) {
    Environment *env = new Environment();
    env->parent = parent;
    return env;
}