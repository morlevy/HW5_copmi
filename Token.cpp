//
// Created by alonb on 08/12/2022.
//

#include "Token.h"
#include <iostream>
#include <cstring>

inline namespace lexical {

    Boolean::Boolean(bool flag):value(flag) {
    }

    Relop::Relop(char *value) {
        if(strcmp(value,"<=") == 0){
            this->value = LTE;
        }
        if(strcmp(value,"<") == 0){
            this->value = LT;
        }
        if(strcmp(value,">=") == 0){
            this->value = GTE;
        }
        if(strcmp(value,">") == 0){
            this->value = GT;
        }
        if(strcmp(value,"==") == 0){
            this->value = EQ;
        }
        if(strcmp(value,"!=") == 0){
            this->value = DIF;
        }
    }

    Binop::Binop(char value) {
        if(value == '*'){
            this->value = MUL;
        }
        if(value == '+'){
            this->value = ADD;
        }
        if(value == '-'){
            this->value = SUB;
        }
        if(value == '/'){
            this->value = DIV;
        }
    }

    Type::Type(Typename type) : Typeable(type) {

    }

    String::String(char * string) : value(string) {

    }

    Num::Num(int n) : value(n) {

    }

    Id::Id(char *value): name(value){
    }
}