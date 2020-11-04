//
// Created by Jesse Li on 10/31/20.
//

#ifndef ASSIGN03_TYPE_H
#define ASSIGN03_TYPE_H

#include <string>
#include "symtab.h"

struct SymbolTable;

enum RealType {
    PRIMITIVE,
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

    const char* name;

    SymbolTable* symtab;
public:
    Type(int realType);
    ~Type();
    std::string to_string();
};

Type* type_create_primitive(const char* name);

Type* type_create_array(long size, Type* elementType);

Type* type_create_record(SymbolTable* symbolTable);

#endif //ASSIGN03_TYPE_H
