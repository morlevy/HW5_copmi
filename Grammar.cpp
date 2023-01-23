//
// Created by alonb on 08/12/2022.
//
#include "hw3_output.hpp"
#include "Grammar.h"
#include "Node.h"
#include "SymbolTable.h"
#include <iostream>
#include "bp.hpp"
#include "GenerateRegister.hpp"

using namespace std;
SymbolTable* symbol_table;
CodeBuffer& code_buffer = CodeBuffer::instance();
GenerateRegister *generate_register = new GenerateRegister();
int curr_scope = 0;
string current_function;
int currinstr = 0;
int while_ = 0;
int num_args = 0;

inline namespace grammar {

    Program::Program() {
        symbol_table = new SymbolTable();
        code_buffer.emitGlobal("declare i32 @printf(i8*, ...)");
        code_buffer.emitGlobal("declare void @exit(i32)");

        code_buffer.emitGlobal("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
        code_buffer.emitGlobal("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
        code_buffer.emitGlobal("@DavidThrowsZeroExcp = constant [22 x i8] c\"Error division by zero\"");

        code_buffer.emitGlobal("define void @printi(i32) {");
        code_buffer.emitGlobal(
                "call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0), i32 %0)");
        code_buffer.emitGlobal("ret void");
        code_buffer.emitGlobal("}");


        code_buffer.emitGlobal("define void @print(i8*) {");
        code_buffer.emitGlobal(
                "call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0), i8* %0)");
        code_buffer.emitGlobal("ret void");
        code_buffer.emitGlobal("}");
        //
    }

    std::string convert_type(Typename type) {
        switch (type) {
            case TN_BOOL:
                return "BOOL";
            case TN_BYTE:
                return "BYTE";
            case TN_VOID:
                return "VOID";
            case TN_INT:
                return "INT";
            case TN_STR:
                return "STRING";
            default:
                return "ERROR";
        }
    }

    Label::Label() {
        label_name = code_buffer.genLabel();
    }



    void startScope() {
        symbol_table->scopes.emplace_back();
        symbol_table->offset_stack.push_back(symbol_table->offset_stack.back());
    }

    std::string convert_type_llvm(Typename t) {
        switch (t) {
            case TN_BOOL:
                return "i1";
            case TN_BYTE:
                return "i8";
            case TN_VOID:
                return "void";
            case TN_INT:
                return "i32";
            case TN_STR:
                return "i8*";
            default:
                return "ERROR";
        }
    }

    std::string loadRegister(int offset, Typename type) {
        std::string reg = generate_register->nextRegister();
        std::string ptr = generate_register->nextRegister();
        if (offset < 0) {
            currinstr = code_buffer.emit(
                    "%" + ptr + " = getelementptr [ " + to_string(num_args) + " x i32], [ " + to_string(num_args) +
                    " x i32]* %args, i32 0, i32 " + to_string(num_args + offset));
        } else {
            currinstr = code_buffer.emit(
                    "%" + ptr + " = getelementptr [ 50 x i32], [ 50 x i32]* %stack, i32 0, i32 " + to_string(offset));
        }
        currinstr = code_buffer.emit("%" + reg + " = load i32, i32* %" + ptr);
        if (type != TN_INT) {
            std::string reg2 = generate_register->nextRegister();
            currinstr = code_buffer.emit("%" + reg2 + " = trunc i32 %" + reg + " to " + convert_type_llvm(type));
            return reg2;
        }
        return reg;
    }

    void closeScope() {
        //output::endScope();
        SymbolTable::Scope curr_scope = symbol_table->scopes.back();
        for (auto symbol: curr_scope.symbols) {
            if (symbol->getTypes().size() == 1) { // symbol is var
                std::string type = convert_type(symbol->getTypes()[0]);
                //output::printID(symbol->getName(), symbol->getOffset(), type);//variables
            } else { // symbol is function
                auto ret_val = convert_type(symbol->getTypes().back());

                vector<Typename> types = symbol->getTypes();
                types.pop_back();
                if (types.back() == TN_VOID) { //TODO check
                    //check if no args
                    types.pop_back();
                }

                std::vector<std::string> str_types;
                for (int i = 0; i < types.size(); i++) {
                    str_types.push_back(convert_type(types[i]));// maybe opposite
                }
                /**output::printID(symbol->getName(), symbol->getOffset(), output::makeFunctionType(ret_val,
                                                                                                 str_types));//function**/
            }
        }
        /**
        DO_DEBUG(
                cout << "______Global:______\n";
                CodeBuffer::instance().printGlobalBuffer();
                cout << "______Code:______\n";
                CodeBuffer::instance().printCodeBuffer();
        );**/
        curr_scope.symbols.clear();
        symbol_table->scopes.pop_back();
        symbol_table->offset_stack.pop_back();
    }

    void endProgram() {
        //find main function
        std::vector<SymbolTable::FunctionSymbolData> global_functions = symbol_table->global_functions;
        bool flag = false;
        for (auto &global_function: global_functions) {
            if (global_function.getName() == "main") {
                if (global_function.getTypes().size() == 2 && global_function.getTypes().front() == TN_VOID &&
                    global_function.getType() == TN_VOID) {
                    flag = true;
                }
            }
        }
        if (!flag) {//no main function
            output::errorMainMissing();
            exit(0);
        }
        closeScope();
        code_buffer.printGlobalBuffer();
        code_buffer.printCodeBuffer();
    }

    Funcs::Funcs() {}

    bool idInSymbolTable(const string& id) {
        bool flag = false;
        for (int i = symbol_table->scopes.size() - 1; i >= 0; i--) {
            for (auto &symbol: symbol_table->scopes[i].symbols) {
                if (id == symbol->getName())
                    flag = true;
            }
        }
        return flag;
    }

    FuncDecl::FuncDecl(RetType *ret_type, Id *id, Formals *formulas) {
        if (idInSymbolTable(id->name)) {
            output::errorDef(yylineno, id->name);
            exit(0);
        }
        for (int i = 0; i < formulas->formals.size(); i++) {
            if (idInSymbolTable(formulas->formals[i]->name) || formulas->formals[i]->name == id->name) {
                output::errorDef(yylineno, formulas->formals[i]->name);
                exit(0);
            }
        }
        for (int i = 0; i < formulas->formals.size(); i++) {
            for (int j = i + 1; j < formulas->formals.size(); ++j) {
                if (formulas->formals[i]->name == formulas->formals[j]->name) {
                    output::errorDef(yylineno, formulas->formals[i]->name);
                    exit(0);
                }
            }
        }
        string return_llvm_type = convert_type_llvm(ret_type->type);
        string formals_llvm_list;
        current_function = id->name;
        num_args = formulas->formals.size();
        name = id->name;
        if (!formulas->formals.empty()) {
            for (auto formal: formulas->formals) {
                types.push_back(formal->type);
                formals_llvm_list += convert_type_llvm(formal->type) + ", ";
            }
        } else {
            formals_llvm_list = "()";
            types.emplace_back(TN_VOID);
        }
        formals_llvm_list = "("+formals_llvm_list.substr(0, formals_llvm_list.size() - 2)+")";
        currinstr = code_buffer.emit("define " + return_llvm_type + " @" + id->name + formals_llvm_list + " {");
        currinstr = code_buffer.emit("%stack = alloca [50 x i32]");
        currinstr = code_buffer.emit("%args = alloca [ " + to_string(num_args) + " x i32]");
        for (int i = 0; i < formulas->formals.size(); i++) {
            std::string reg = generate_register->nextRegister();
            currinstr = code_buffer.emit("%" + reg + " = getelementptr [ " + to_string(num_args) + " x i32], [ " + to_string(num_args) +
                                         " x i32]* %args, i32 0, i32 " + to_string(num_args - i - 1));
            if(convert_type_llvm(formulas->formals[i]->type) != "i32"){
                string new_reg = generate_register->nextRegister();
                currinstr = code_buffer.emit("%" + new_reg + " =zext " + convert_type_llvm(formulas->formals[i]->type) + " %" + to_string(i) + " to i32");
                currinstr = code_buffer.emit("store i32 %" + new_reg + ", i32* %" + reg);
            } else {
                currinstr = code_buffer.emit("store i32 %" + formulas->formals[i]->name + ", i32* %" + reg);
            }
        }
        types.emplace_back(ret_type->type);
        symbol_table->scopes.back().symbols.push_back(new SymbolTable::SymbolData(0, name, types, ""));
        if (curr_scope == 0) {
            symbol_table->global_functions.emplace_back(yylineno, name, types);
        }

        startScope();
        current_function = id->name;
        for (int i = 0; i < formulas->formals.size(); i++) {
            symbol_table->scopes.back().symbols.push_back(
                    new SymbolTable::SymbolData(-i - 1, formulas->formals[i]->name, formulas->formals[i]->type, ""));
        }
    }

    N::N() {
        next_list = CodeBuffer::makelist({currinstr+1,FIRST});
        currinstr = code_buffer.emit("br label @");
    }

    void closeFunction(RetType *retType) {
        string extra = retType->type == TN_VOID ? "" : " 0";
        currinstr = code_buffer.emit("ret " + convert_type_llvm(retType->type) + extra);
        currinstr = code_buffer.emit("}");
        current_function = "";
        num_args = 0;
    }

    RetType::RetType(Typename type) : type(type) {
    }


    Formals::Formals() {}

    Formals::Formals(FormalsList *formulas_list) {
        this->formals = vector<FormalDecl *>(formulas_list->list);
    }

    FormalsList::FormalsList(FormalDecl *formal_decl) {
        list.push_back(formal_decl);
    }

    FormalsList::FormalsList(FormalDecl *formal_decl, FormalsList *formulas_list) {
        list.push_back(formal_decl);
        list.insert(list.end(), formulas_list->list.begin(), formulas_list->list.end());
    }

    FormalDecl::FormalDecl(Type *type, Id *id) : type(type->type), name(id->name) {
    }

    Statements::Statements(Statement *statement) {
        cout << "statement next list size: " << statement->next_list.size() << endl;
        this->next_list = statement->next_list;
        cout << "statement2" << endl;
    }

    Statements::Statements(Statements *statements, Statement *statement, Label* label) {
        code_buffer.bpatch(statements->next_list, label->label_name);
        this->next_list = statement->next_list;
    }

    Statement::Statement(Type *type, Id *id) {
        if (idInSymbolTable(id->name)) {
            output::errorDef(yylineno, id->name);
            exit(0);
        }
        symbol_table->scopes.back().symbols.emplace_back(
                new SymbolTable::SymbolData(symbol_table->offset_stack.back()++, id->name, type->type, ""));
    }

    Statement::Statement(Type *type, Id *id, Exp *exp) {
        if (!canImplicitlyAssign(*exp, *type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if (idInSymbolTable(id->name)) {
            output::errorDef(yylineno, id->name);
            exit(0);
        }
        this->value = generate_register->nextRegister();
        std::string expReg = exp->reg;
        if (type->type == TN_INT && exp->type == TN_BYTE) {
            expReg = generate_register->nextRegister();
            currinstr = code_buffer.emit("%" + expReg + " = zext i8 %" + exp->reg + " to i32");
        }
        currinstr = code_buffer.emit("%" + this->value + " = add " + convert_type_llvm(exp->type) + " 0,%" + expReg);
        int offset = symbol_table->offset_stack.back()++;
        string ptr = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + ptr +
                                     " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                                     to_string(offset));
        expReg = this->value;
        if (exp->type != TN_INT) {
            //%X = zext i8 %t3 to i32
            expReg = generate_register->nextRegister();
            currinstr = code_buffer.emit(
                    "%" + expReg + " = zext " + convert_type_llvm(exp->type) + " %" + this->value + " to i32");
        }
        currinstr = code_buffer.emit("store i32 %" + expReg + ", i32* %" + ptr);
        exp->reg = expReg;
        symbol_table->scopes.back().symbols.emplace_back(
                new SymbolTable::SymbolData(offset, id->name, type->type, expReg));
    }

    Statement::Statement(Id *id, Assign *assign, Exp *exp) {
        SymbolTable::SymbolData *symbol_type = symbol_table->getSymbol(id->name);
        if (symbol_type == nullptr) {
            output::errorUndef(yylineno, id->name);
            exit(0);
        }
        //check if function
        if (symbol_type->getTypes().size() > 1) {
            output::errorUndef(yylineno, id->name);
            exit(0);
        }
        if (!canImplicitlyAssign(exp->type, symbol_type->getTypes().back())) { //maybe get symbol problem
            output::errorMismatch(yylineno);
            exit(0);
        }
        symbol_type->setRegister(exp->reg);
        //this->value = generate_register->nextRegister();

    }

    Statement::Statement(Call *call): next_list({}) {

    }

    Statement::Statement(Return *_return) {
        SymbolTable::SymbolData *function = symbol_table->getSymbol(current_function);
        if (function == nullptr) {//shouldnt get inside
            EXIT_PRINT(0);
        }
        if (function->getTypes().back() != TN_VOID) {
            output::errorMismatch(yylineno);
            exit(0);
        }
    }

    Statement::Statement(Return *_return, Exp *exp) {
        SymbolTable::SymbolData *function = symbol_table->getSymbol(current_function);
        if (function == nullptr) {
            EXIT_PRINT(0);
        }
        if (function->getTypes().back() != exp->type) {
            output::errorMismatch(yylineno);
            exit(0);
        }
    }

    Statement::Statement(If *if_, Exp *exp,Label* label, Statement* statement) {
        TypeAssert(exp, TN_BOOL);
        cout << "if" << endl;
        code_buffer.bpatch(exp->true_list, label->label_name);
        cout << "arrived here 1\n";
        this->next_list = CodeBuffer::merge(exp->false_list, statement->next_list);
        cout << "arrived here 2\n";
    }

    Statement::Statement(If *_if, Exp *exp,Label* label1,Statement* statement1,N* n, Else *_else, Label* label2, Statement* statement2) {
        TypeAssert(exp, TN_BOOL);
        code_buffer.bpatch(exp->true_list, label1->label_name);
        code_buffer.bpatch(exp->false_list, label2->label_name);
        this->next_list = CodeBuffer::merge(CodeBuffer::merge(statement1->next_list, n->next_list), statement2->next_list);
        cout << "arrived here 6\n" << "and next list size is " << this->next_list.size() << endl;
    }

    Statement::Statement(While *_while, Exp *exp) {
        TypeAssert(exp, TN_BOOL);
    }

    void start_while() {
        while_++;
    }

    void end_while() {
        while_--;
    }

    Statement::Statement(Break *_break) {
        if (while_ <= 0) {
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }
    }

    Statement::Statement(Continue *_continue) {
        if (while_ <= 0) {
            output::errorUnexpectedContinue(yylineno);
            exit(0);
        }
    }

    Statement::Statement(Statements *s) {
        cout << "arrived here 4 size is: " << s->next_list.size() << endl;
        this->next_list = s->next_list;
        cout << "arrived here 5\n";
    }

    Call::Call(Id *id, ExpList *expList) : Typeable(TN_VOID) {
        auto func = symbol_table->getFunctionSymbol(id->name);
        if (func == nullptr) {
            output::errorUndefFunc(yylineno, id->name);
            exit(0);
        }
        auto func_types = func->getTypes();
        type = func_types.back();
        func_types.pop_back();
        vector<string> vector2;
        for (auto type2: func_types) {
            vector2.push_back(convert_type(type2));
        }
        if (func_types.size() != expList->exp_list.size()) {
            output::errorPrototypeMismatch(yylineno, id->name, vector2);
            exit(0);
        }
        for (int i = 0; i < func_types.size(); i++) {
            if (func_types[i] != expList->exp_list[i]->type &&
                !(func_types[i] == TN_INT && expList->exp_list[i]->type == TN_BYTE)) {
                output::errorPrototypeMismatch(yylineno, id->name, vector2);
                exit(0);
            }
        }
    }

    Call::Call(Id *id) : Typeable(TN_VOID) {
        auto func = symbol_table->getFunctionSymbol(id->name);

        if (func == nullptr) {
            output::errorUndefFunc(yylineno, id->name);
            exit(0);
        }
        auto func_types = func->getTypes();
        type = func_types.back();
        func_types.pop_back();

        if (func_types.back() != TN_VOID) {
            vector<string> vec{};
            for (auto type: func_types) {
                vec.push_back(convert_type(type));
            }
            output::errorPrototypeMismatch(yylineno, id->name, vec);
            exit(0);
        }
    }

    ExpList::ExpList(Exp *exp) {
        exp_list.push_back(exp);
    }

    ExpList::ExpList(Exp *exp, ExpList *expList) {
        exp_list.push_back(exp);
        exp_list.insert(exp_list.end(), expList->exp_list.begin(), expList->exp_list.end());
    }

    Exp::Exp(Id *id) : Typeable(TN_VOID) {
        const auto &symbol_data_opt = symbol_table->getSymbol(id->name);
        //std::cout <<"id is: "<< id->name << " type: " << symbol_data_opt->getTypes().back() << std::endl;
        if (symbol_data_opt == nullptr) {
            output::errorUndef(yylineno, id->name);
            exit(0);
        } //this exception happened in get symbol
        if (symbol_data_opt->getTypes().size() > 1) {
            output::errorUndef(yylineno, id->name);
            exit(0);
        }
        this->type = symbol_data_opt->getTypes().back();
        this->value = symbol_data_opt->getName();
        this->reg = loadRegister(symbol_data_opt->getOffset(), symbol_data_opt->getTypes().back());
    }

    Exp::Exp(Call *call) : Typeable(call->type) {

    }

    Exp::Exp(Num *num) : Typeable(TN_INT), value(to_string(num->value)) {
        //std::cout << "type is " << type << std::endl;
        this->reg = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + this->reg + " = add i32 0, " + value);
    }

    Exp::Exp(Num *num, B *b) : Typeable(TN_BYTE), value(to_string(num->value)) {
        if (num->value > 255) {
            output::errorByteTooLarge(yylineno, to_string(num->value));
            exit(0);
        }
        this->reg = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + this->reg + " = add i8 0," + value);
    }

    Exp::Exp(String *str) : Typeable(TN_STR), value(str->value) {
        this->reg = generate_register->nextRegister();
        code_buffer.emitGlobal("@" + reg + "= constant [" + to_string(value.size() - 2) + " x i8] c" + value);
        currinstr = code_buffer.emit(
                "%" + reg + "= getelementptr [" + to_string(value.size() - 2 ) + " x i8], [" + to_string(value.size() - 2) +
                " x i8]* @" + reg + ", i8 0, i8 0");
    }

    Exp::Exp(Boolean *boolean) : Typeable(TN_BOOL), value(to_string(boolean->value)) {
        this->reg = generate_register->nextRegister();
        //currinstr = code_buffer.emit("%" + this->reg + " = add i1 0," + value);
        currinstr = code_buffer.emit("br label @");
        auto list = CodeBuffer::makelist({currinstr,FIRST});
        this->false_list ={};
        this->true_list = {};
        if(boolean->value){
            this->true_list = list;
            cout << "true\n";
        } else {
            this->false_list = list;
        }
        cout << "arrived here3\n";
    }

    Exp::Exp(Not *_not, Exp *exp) : Typeable(TN_BOOL) {
        TypeAssert(exp, TN_BOOL);
        if (exp->value == "0") {
            this->value = "1";
        } else {
            this->value = "0";
        }
        //this->reg = generate_register->nextRegister();
        //currinstr = code_buffer.emit("%" + this->reg + " = add i1 1," + exp->reg);
        this->false_list = exp->true_list;
        this->true_list = exp->false_list;
    }

    Exp::Exp(Exp *exp1, And *_and, Exp *exp2, Label* label) : Typeable(TN_BOOL) {
        TypeAssert(exp1, TN_BOOL);
        TypeAssert(exp2, TN_BOOL);

        //int label_location = code_buffer.emit("br i1 %" + exp1->reg + ", label @, label @");
        //this->reg = generate_register->nextRegister();
        code_buffer.bpatch(exp1->true_list,label->label_name);
        this->true_list = exp2->true_list;
        this->false_list = CodeBuffer::merge(exp1->false_list,exp2->false_list);

        /**
        int false_location = code_buffer.emit("br label @");
        string false_label = code_buffer.genLabel();
        int end_location = code_buffer.emit("br label @");
        string end_label = code_buffer.genLabel();


        currinstr = code_buffer.emit("%" + this->reg + " = phi i1 [%" + exp2->reg + ", %" + label->label_name + "],[0, %" + false_label + "]");
        code_buffer.bpatch(CodeBuffer::makelist({label->label_location, FIRST}), label->label_name);
        code_buffer.bpatch(CodeBuffer::makelist({label->label_location, SECOND}), false_label);
        code_buffer.bpatch(CodeBuffer::makelist({false_location, FIRST}), end_label);
        code_buffer.bpatch(CodeBuffer::makelist({end_location, FIRST}), end_label);**/
    }

    Exp::Exp(Exp *exp1, Or *_or, Exp *exp2, Label* label) : Typeable(TN_BOOL) {
        TypeAssert(exp1, TN_BOOL);
        TypeAssert(exp2, TN_BOOL);

        code_buffer.bpatch(exp1->false_list,label->label_name);
        this->false_list = exp2->false_list;
        this->true_list = CodeBuffer::merge(exp1->true_list,exp2->true_list);
        /**
        this->reg = generate_register->nextRegister();
        this->false_list = vector<pair<int, BranchLabelIndex>>();
        this->true_list = vector<pair<int, BranchLabelIndex>>();

        int true_location = code_buffer.emit("br label @");
        string true_label = code_buffer.genLabel();
        int end_location = code_buffer.emit("br label @");
        string end_label = code_buffer.genLabel();


        currinstr = code_buffer.emit("%" + this->reg + " = phi i1 [%" + exp2->reg + ", %" + label->label_name + "],[1, %" + true_label + "]");
        code_buffer.bpatch(CodeBuffer::makelist({label->label_location, FIRST}), true_label);
        code_buffer.bpatch(CodeBuffer::makelist({label->label_location, SECOND}), label->label_name);
        code_buffer.bpatch(CodeBuffer::makelist({true_location, FIRST}), end_label);
        code_buffer.bpatch(CodeBuffer::makelist({end_location, FIRST}), end_label);**/
    }

    string convert_to_llvm_relop(Relop::RelopValue val){
        switch(val){
            case Relop::RelopValue::EQ:
                return "eq";
            case Relop::RelopValue::GTE:
                return "sge";
            case Relop::RelopValue::LT:
                return "slt";
            case Relop::RelopValue::LTE:
                return "sle";
            case Relop::RelopValue::GT:
                return "sgt";
            case Relop::RelopValue::DIF:
                return "ne";
            default:
                return "ERROR";
        }
    }

    Exp::Exp(Exp *exp1, Relop *relop, Exp *exp2) : Typeable(TN_BOOL) {
        if (!isNumeric(exp1->type) || !isNumeric(exp2->type)) {
            //std::cout << "error in relop" << "\n";
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->reg = generate_register->nextRegister();
        //code_buffer.emit("relop value: " + to_string((relop->value)));
        currinstr = code_buffer.emit("%" + this->reg + " = icmp " + convert_to_llvm_relop(relop->value) + " i32 %" + exp1->reg + ", %" + exp2->reg);
        this->true_list = CodeBuffer::makelist({currinstr,FIRST});
        this->false_list = CodeBuffer::makelist({currinstr,SECOND});
        code_buffer.emit("br i1 %" + this->reg + ", label @, label @");
    }

    Exp::Exp(Type *type, Exp *exp) : Typeable(type->type) {
        if (!canExplicitlyAssign(exp->type, type->type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->reg = generate_register->nextRegister();
    }

    Exp::Exp(Exp *exp) : Typeable(exp->type), reg(exp->reg), false_list(exp->false_list), true_list(exp->true_list) {
        code_buffer.emit("(exp)="+convert_type(exp->type));
    }


    Exp::Exp(Exp *exp1, If *_if, Exp *exp2, Else *_else, Exp *exp3) : Typeable(exp3->type) {
        TypeAssert(exp2, TN_BOOL);

        if (exp1->type != exp3->type and !canExplicitlyAssign(exp1->type, exp3->type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if (exp1->type == TN_INT or exp3->type == TN_INT) {
            type = TN_INT;
        }


    }


    Exp::Exp(Exp *exp1, Binop *binop, Exp *exp2) : Typeable(
            exp1->type == TN_INT || exp2->type == TN_INT ? TN_INT : TN_BYTE) {
        if (!isNumeric(exp1->type) || !isNumeric(exp2->type)) {
            output::errorMismatch(yylineno);
            //std::cout << "exp1 type: " << exp1->type << " exp2 type: " << exp2->type << "\n";
            exit(0);
        }
        //cout << "regs: " << exp1->reg << " " << exp2->reg << endl;
        //TODO: verify signed vs unsigned, implement void @error_division_by_zero()
        this->reg = generate_register->nextRegister();
        std::string reg_left = exp1->reg;
        std::string reg_right = exp2->reg;
        //PRINT_PARAM(exp1->reg);
        //PRINT_PARAM(exp2->reg);
        const bool is_any_int = exp1->type == TN_INT || exp2->type == TN_INT;
        const std::string exp_size = is_any_int ? "i32" : "i8";
        const std::string op = [&]() {
            switch (binop->value) {
                case Binop::BinopValue::ADD:
                    return "add";
                case Binop::BinopValue::SUB:
                    return "sub";
                case Binop::BinopValue::MUL:
                    return "mul";
                case Binop::BinopValue::DIV:
                    return "sdiv";
                default:
                    output::errorMismatch(yylineno);
                    break;
            }
            return "";
        }();
        if (is_any_int && exp1->type == TN_BYTE) {
            reg_left = generate_register->nextRegister();
            currinstr = code_buffer.emit("%" + reg_left + " = zext i8 %" + exp1->reg + " to i32");
        }
        if (is_any_int && exp2->type == TN_BYTE) {
            reg_right = generate_register->nextRegister();
            currinstr = code_buffer.emit("%" + reg_right + " = zext i8 %" + exp2->reg + " to i32");
        }
        if (binop->value == Binop::DIV) {
            // * division by zero
            const auto &reg1 = generate_register->nextRegister("div_by_zero");
            currinstr = code_buffer.emit(reg1 + " = icmp eq " + exp_size + " " + exp2->reg + ", 0");
            const int need_back_patch = currinstr = code_buffer.emit("br i1 %" + reg1 + ", label @, label @");
            const std::string &yes_div_zero = code_buffer.genLabel();
            currinstr = code_buffer.emit("call void @error_division_by_zero()");
            const std::string &no_div_zero = code_buffer.genLabel();
            code_buffer.bpatch(std::vector{make_pair(need_back_patch, FIRST)}, yes_div_zero);
            code_buffer.bpatch(std::vector{make_pair(need_back_patch, SECOND)}, no_div_zero);
        }
        currinstr = code_buffer.emit("%" + this->reg + " = " + op + " " + exp_size + " %" + reg_left + ", %" + reg_right);
    }

    Exp::Exp(const string& x, Exp *exp) : Typeable(TN_BOOL), reg(exp->reg), false_list(exp->false_list), true_list(exp->true_list) {
        TypeAssert(exp, TN_BOOL);
        cout << "not exp" << endl;
    }

    void end_function(RetType* ret) {
        current_function = "";
        closeScope();
        closeFunction(ret);
    }
}