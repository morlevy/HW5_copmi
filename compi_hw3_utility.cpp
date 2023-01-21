//
// Created by alonb on 10/12/2022.
//
#include "hw3_output.hpp"
#include "compi_hw3_utility.h"
#include <iostream>
extern int yylineno;
bool canImplicitlyAssign(Typename src, Typename dst){
    return src == dst || (src == TN_BYTE && dst == TN_INT);
}

bool canExplicitlyAssign(Typename src, Typename dst){
    return (src == TN_INT && dst == TN_BYTE) || (src == TN_BYTE && dst == TN_INT);
}

bool canImplicitlyAssign(const Typeable& src, const Typeable& dst){
    return canImplicitlyAssign(src.type, dst.type);
}

void TypeAssert(const Typeable *typeable, Typename tn) noexcept {
    if(typeable->type == tn) return;
    output::errorMismatch(yylineno);
    exit(0);
}

bool isNumeric(Typename tn) {
    return tn == TN_BYTE || tn == TN_INT;
}

Typeable::Typeable(Typename type) : type{type} {
}
