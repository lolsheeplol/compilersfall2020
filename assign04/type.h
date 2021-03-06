//
// Created by Jesse Li on 10/31/20.
//

#ifndef ASSIGN03_TYPE_H
#define ASSIGN03_TYPE_H

#include <string>
#include "symtab.h"

struct SymbolTable;

// fixme: Changing the enum starting value to *not* 0 fixes segfault on array03, input02, etc, but RECORD will stop printing their fields
enum RealType {
    PRIMITIVE = 0,
    ARRAY,
    RECORD
};

struct Type {
    // contain all variables for all types
    // use enum for the explicit type, e.g. PRIMITIVE, ARRAY, RECORD
public:
    int realType;

    long arraySize;
    Type* arrayElementType;

    long size;

    const char* name;

    SymbolTable* symtab;
public:
    Type(int realType);
    ~Type();
    std::string to_string();
    long get_size();
};

Type* type_create_integer();

Type* type_create_char();

Type* type_create_array(long size, Type* elementType);

Type* type_create_record(SymbolTable* symbolTable);

#endif //ASSIGN03_TYPE_H
