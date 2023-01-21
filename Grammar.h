//
// Created by alonb on 08/12/2022.
//

#ifndef HW3_GRAMMAR_H
#define HW3_GRAMMAR_H

#include "SymbolTable.h"
#include "Token.h"
#include "Node.h"
#include "compi_hw3_utility.h"
#include <vector>
#include <memory>
#include <string>
extern int yylineno;

inline namespace grammar{

    bool idInSymbolTable(const std::string& id);
    void endProgram();
    void closeScope();
    void startScope();
    void end_function();
    void start_while();
    void end_while();

    struct Program;
    struct Funcs;
    struct FuncDecl;
    struct RetType;
    struct Formals;
    struct FormalsList;
    struct FormalDecl;
    struct Statements;
    struct Statement;
    struct Call;
    struct ExpList;
    struct Exp;

    struct Program : public Node {
        Program();
    };

    struct Funcs : public Node {
        Funcs();
    };

    struct FuncDecl : public Node {
        std::vector<Typename> types;
        std::string name;
        FuncDecl(RetType*, Id*, Formals*);
    };

    struct RetType : public Node {
        Typename type;
        explicit RetType(Typename type);
    };
    
    struct Formals : public Node {
        std::vector<FormalDecl*> formals;
        Formals();
        explicit Formals(FormalsList*);
    };

    struct FormalsList : public Node {
        std::vector<FormalDecl*> list;
        explicit FormalsList(FormalDecl*);
        FormalsList(FormalDecl*, FormalsList*);
    };

    struct FormalDecl : public Node {
        Typename type;
        std::string name;
        FormalDecl(Type*, Id*);
    };

    struct Statements : public Node {
        explicit Statements(Statement*) ;
        Statements(Statements*, Statement*) ;
    };

    struct Statement : public Node {
        std::string value;
        Statement(Statements*);
        Statement(Type*, Id*);
        Statement(Type*, Id*, Exp*);
        Statement(Id*, Assign*, Exp*);
        Statement(Call*);
        Statement(Return*);
        Statement(Return*, Exp*);
        Statement(If*, Exp*);
        Statement(If*,Exp*, Else*);
        Statement(While*,Exp*);
        Statement(Break*);
        Statement(Continue*);
    };

    struct Call : public Node, public Typeable {
        Call(Id*, ExpList*);
        Call(Id*);

    };

    struct ExpList : public Node {
        std::vector<Exp*> exp_list;
        explicit ExpList(Exp*);
        ExpList(Exp*, ExpList*);
    };

    struct Exp : public Node, public Typeable {
        std::string value;
        std::string reg;

        explicit Exp(Id*);
        explicit Exp(Call*);
        explicit Exp(Num*);
        Exp(Num*, B*);
        explicit Exp(String*);
        explicit Exp(Boolean*);
        Exp(Exp*);
        Exp(const std::string& , Exp*);
        Exp(Exp* , If*, Exp*, Else*, Exp*);
        Exp(Exp*, Binop*, Exp*);
        Exp(Not*, Exp*);
        Exp(Exp*, And*, Exp*);
        Exp(Exp*, Or*, Exp*);
        Exp(Exp*, Relop*, Exp*);
        Exp(Type*, Exp*);
    };

    
}

#endif //HW3_GRAMMAR_H
