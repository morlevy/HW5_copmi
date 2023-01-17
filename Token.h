//
// Created by alonb on 08/12/2022.
//

#ifndef HW3_TOKEN_H
#define HW3_TOKEN_H

#include "Macros.h"
#include "Node.h"
#include <string>
#include <variant>

inline namespace lexical {
    struct Token : public Node{
#if LEXEME_IN_TOKEN
        std::string lexeme;
#endif //LEXEME_IN_TOKEN
    };

//Literal
    struct Literal : public Token {
    };

    struct Num : public Literal {
        int value;
        explicit Num(int );
    };
    struct Boolean : public Literal {
        bool value;
        explicit Boolean(bool flag);
    };
    struct String : public Literal {
        std::string value;
        explicit String(char* string);
    };

//Binary Operator
    struct Binop : public Token {
        using BinopValue = enum {
            ADD, SUB, MUL, DIV
        };
        BinopValue value;
        explicit Binop(char value);
    };
    struct Assign : public Token {};

//Relational Operator
    struct Relop : public Token {
        using RelopValue = enum {
            EQ, DIF, GT, GTE, LT, LTE
        };
        RelopValue value;
        explicit Relop(char* value);
    };

//TypeName
    struct Type : public Token , public Typeable {
        explicit Type(Typename type);
    };
    struct Void : public Token {};

//Saved Word

    struct SavedWord : public Token {};

    struct Return : public SavedWord {};
    struct If : public SavedWord {};
    struct Else : public SavedWord {};
    struct While : public SavedWord {};
    struct Break : public SavedWord {};
    struct Continue : public SavedWord {};
    struct Not : public SavedWord {};
    struct And : public SavedWord {};
    struct Or : public SavedWord {};


//Punctuation Marks
    struct Sc : public Token {
    };
    struct Comma : public Token {
    };
    struct Lparen : public Token {
    };
    struct Rparen : public Token {
    };
    struct Lbrace : public Token {
    };
    struct Rbrace : public Token {
    };

//Id
    struct Id : public Token {
        std::string name;
        Id(char* value);
    };

    struct B : public Token {
    };
}
#endif //HW3_TOKEN_H
