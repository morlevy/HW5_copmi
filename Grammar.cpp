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
int while_ = 0;
int num_args = 0;

inline namespace grammar{
    
    Program::Program() {
        symbol_table = new SymbolTable();
        //
    }

    std::string convert_type(Typename type){
        switch (type) {
            case TN_BOOL: return "BOOL";
            case TN_BYTE: return "BYTE";
            case TN_VOID: return "VOID";
            case TN_INT: return "INT";
            case TN_STR: return "STRING";
            default: return "ERROR";
        }
    }

    void end_function(){
        current_function = "";
        closeScope();
    }

    void startScope() {
        symbol_table->scopes.emplace_back();
        symbol_table->offset_stack.push_back(symbol_table->offset_stack.back());
    }

    std::string convert_type_llvm(Typename t){
        switch (t) {
            case TN_BOOL: return "i1";
            case TN_BYTE: return "i8";
            case TN_VOID: return "void";
            case TN_INT: return "i32";
            case TN_STR: return "i8*";
            default: return "ERROR";
        }
    }

    std::string loadRegister(int offset, Typename type){
        std::string reg = generate_register->nextRegister();
        std::string ptr = generate_register->nextRegister();
        if(offset < 0){
            code_buffer.emit("%" + ptr + " = getelementptr [ " + to_string(num_args) + " x i32], [ " + to_string(num_args) + " x i32]* %args, i32 0, i32 " + to_string(num_args + offset));
        } else {
            code_buffer.emit("%" + ptr + " = getelementptr [ 50 x i32], [ 50 x i32]* %stack, i32 0, i32 " + to_string(offset));
        }
        code_buffer.emit("%" + reg + " = load i32, i32* %" + ptr);
        if(type != TN_INT){
            std::string reg2 = generate_register->nextRegister();
            code_buffer.emit("%" + reg2 + " = trunc i32 %" + reg + " to " + convert_type_llvm(type));
            return reg2;
        }
        return reg;
    }

    void closeScope(){
        output::endScope();
        SymbolTable::Scope curr_scope = symbol_table->scopes.back();
        for (auto symbol:curr_scope.symbols) {
            if (symbol->getTypes().size() == 1) { // symbol is var
                std::string type = convert_type(symbol->getTypes()[0]);
                output::printID(symbol->getName(), symbol->getOffset(), type);//variables
            } else { // symbol is function
                auto ret_val = convert_type(symbol->getTypes().back());

                vector<Typename> types = symbol->getTypes();
                types.pop_back();
                if (types.back() == TN_VOID) { //TODO check
                    //check if no args
                    types.pop_back();
                }

                std::vector<std::string> str_types;
                for(int i = 0 ; i < types.size()  ; i++){
                    str_types.push_back(convert_type(types[i]));// maybe opposite
                }
                output::printID(symbol->getName(), symbol->getOffset(), output::makeFunctionType(ret_val,
                                                                                                 str_types));//function
            }
        }
        curr_scope.symbols.clear();
        symbol_table->scopes.pop_back();
        symbol_table->offset_stack.pop_back();
    }

    void endProgram(){
        //find main function
        std::vector<SymbolTable::FunctionSymbolData> global_functions = symbol_table->global_functions;
        bool flag = false;
        for(auto & global_function : global_functions) {
            if (global_function.getName() == "main") {
                if (global_function.getTypes().size() == 2 && global_function.getTypes().front() == TN_VOID && global_function.getType() == TN_VOID) {
                    flag = true;
                }
            }
        }
        if (!flag) {//no main function
            output::errorMainMissing();
            exit(0);
        }
         closeScope();
        code_buffer.printCodeBuffer();
    }

    Funcs::Funcs() {}

    bool idInSymbolTable(string id){
        bool flag = false;
        for (int i = symbol_table->scopes.size() - 1; i >= 0; i--) {
            for (auto & symbol : symbol_table->scopes[i].symbols) {
                if (id == symbol->getName())
                    flag = true;
            }
        }
        return flag;
    }

    FuncDecl::FuncDecl(RetType*  ret_type, Id*  id, Formals*  formulas) {
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
        for(int i = 0; i < formulas->formals.size(); i++){
            for (int j = i + 1; j < formulas->formals.size(); ++j) {
                if (formulas->formals[i]->name == formulas->formals[j]->name) {
                    output::errorDef(yylineno, formulas->formals[i]->name);
                    exit(0);
                }
            }
        }
        name = id->name;
        if (!formulas->formals.empty()) {
            for (auto formal : formulas->formals) {
                types.push_back(formal->type);
            }
        } else {
            types.emplace_back(TN_VOID);
        }

        types.emplace_back(ret_type->type);
        symbol_table->scopes.back().symbols.push_back(new  SymbolTable::SymbolData(0, name, types,""));
        if(curr_scope == 0){
            symbol_table->global_functions.emplace_back(yylineno,name,types);
        }

        startScope();
        current_function = id->name;
        for(int i = 0 ; i < formulas->formals.size(); i++){
            symbol_table->scopes.back().symbols.push_back(new SymbolTable::SymbolData(-i-1 ,formulas->formals[i]->name, formulas->formals[i]->type,""));
        }
    }

    RetType::RetType(Typename type) :type(type) {
    }


    Formals::Formals() {}
    Formals::Formals(FormalsList*  formulas_list)  {
        this->formals = vector<FormalDecl *>(formulas_list->list);
    }

    FormalsList::FormalsList(FormalDecl*  formal_decl)  {
        list.push_back(formal_decl);
    }
    FormalsList::FormalsList(FormalDecl*  formal_decl, FormalsList*  formulas_list)  {
        list.push_back(formal_decl);
        list.insert(list.end(),formulas_list->list.begin(),formulas_list->list.end());
    }

    FormalDecl::FormalDecl(Type*  type , Id*  id):type(type->type), name(id->name)  {
    }

    Statements::Statements(Statement*  statement)  {
        
    }
    Statements::Statements(Statements*  statements, Statement*  statement)  {
        
        
    }
    Statement::Statement(Type*  type, Id*  id)  {
        if(idInSymbolTable(id->name)){
            output::errorDef(yylineno,id->name);
            exit(0);
        }
        symbol_table->scopes.back().symbols.emplace_back(new SymbolTable::SymbolData(symbol_table->offset_stack.back()++,id->name, type->type,""));
    }
    Statement::Statement(Type* type, Id* id, Exp*  exp) {
        if (!canImplicitlyAssign(*exp, *type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if(idInSymbolTable(id->name)){
            output::errorDef(yylineno,id->name);
            exit(0);
        }
        this->value = generate_register->nextRegister();
        std::string expReg = exp->reg;
        if(type->type == TN_INT && exp->type == TN_BYTE){
            expReg = generate_register->nextRegister();
            code_buffer.emit("%" + expReg + " = zext i8 %" + exp->reg + " to i32");
        }
        code_buffer.emit("%" + this->value + " = add " + convert_type_llvm(exp->type) + " 0,%" + expReg);
        int offset = symbol_table->offset_stack.back()++;
        string ptr = generate_register->nextRegister();
        code_buffer.emit("%" + ptr +
                    " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                    to_string(offset));
        expReg = this->value;
        if (exp->type != TN_INT) {
            //%X = zext i8 %t3 to i32
            expReg = generate_register->nextRegister();
            code_buffer.emit(
                    "%" + expReg + " = zext " + convert_type_llvm(exp->type) + " %" + this->value + " to i32");
        }
        code_buffer.emit("store i32 %" + expReg + ", i32* %" + ptr);
        exp->reg = expReg;
        symbol_table->scopes.back().symbols.emplace_back(new SymbolTable::SymbolData(offset,id->name, type->type,expReg));
    }

    Statement::Statement(Id*  id, Assign*  assign, Exp*  exp)  {
        SymbolTable::SymbolData* symbol_type = symbol_table->getSymbol(id->name);
        if(symbol_type == nullptr){
            output::errorUndef(yylineno,id->name);
            exit(0);
        }
        //check if function
        if(symbol_type->getTypes().size() > 1){
            output::errorUndef(yylineno,id->name);
            exit(0);
        }
        if(!canImplicitlyAssign(exp->type, symbol_type->getTypes().back())) { //maybe get symbol problem
            output::errorMismatch(yylineno);
            exit(0);
        }
        symbol_type->setRegister(exp->reg);
        //this->value = generate_register->nextRegister();

    }
    Statement::Statement(Call*  call)  {

    }
    Statement::Statement(Return*  _return)  {
        SymbolTable::SymbolData* function = symbol_table->getSymbol(current_function);
        if(function == nullptr){//shouldnt get inside
            EXIT_PRINT(0);
        }
        if(function->getTypes().back() != TN_VOID){
            output::errorMismatch(yylineno);
            exit(0);
        }
    }
    Statement::Statement(Return*  _return, Exp*  exp)  {
        SymbolTable::SymbolData* function = symbol_table->getSymbol(current_function);
        if(function == nullptr){
            EXIT_PRINT(0);
        }
        if(function->getTypes().back() != exp->type){
            output::errorMismatch(yylineno);
            exit(0);
        }
    }
    Statement::Statement(If* if_, Exp*  exp)  {
        TypeAssert(exp,TN_BOOL);
    }
    Statement::Statement(If*  _if, Exp*  exp, Else*  _else)  {
        TypeAssert(exp,TN_BOOL);
    }
    Statement::Statement(While*  _while,Exp*  exp)  {
        TypeAssert(exp,TN_BOOL);
    }

    void start_while(){
        while_++;
    }
    void end_while(){
        while_--;
    }
    Statement::Statement(Break*  _break)  {
        if(while_ <= 0){
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }
    }
    Statement::Statement(Continue*  _continue)  {
        if(while_ <= 0){
            output::errorUnexpectedContinue(yylineno);
            exit(0);
        }
    }

    Statement::Statement(Statements *) {

    }

    Call::Call(Id*  id,  ExpList*  expList) : Typeable(TN_VOID) {
        auto func = symbol_table->getFunctionSymbol(id->name);
        if(func == nullptr){
            output::errorUndefFunc(yylineno,id->name);
            exit(0);
        }
        auto func_types = func->getTypes();
        type = func_types.back();
        func_types.pop_back();
        vector<string> vector2;
        for(auto type2: func_types){
            vector2.push_back(convert_type(type2));
        }
        if(func_types.size() != expList->exp_list.size()){
            output::errorPrototypeMismatch(yylineno,id->name,vector2);
            exit(0);
        }
        for(int i = 0; i < func_types.size() ; i++){
            if(func_types[i] != expList->exp_list[i]->type && !(func_types[i] == TN_INT && expList->exp_list[i]->type == TN_BYTE)){
                output::errorPrototypeMismatch(yylineno,id->name, vector2);
                exit(0);
            }
        }
    }
    Call::Call(Id*  id) : Typeable(TN_VOID) {
        auto func = symbol_table->getFunctionSymbol(id->name);

        if(func == nullptr){
            output::errorUndefFunc(yylineno,id->name);
            exit(0);
        }
        auto func_types = func->getTypes();
        type = func_types.back();
        func_types.pop_back();

        if(func_types.back() != TN_VOID){
            vector<string> vec{};
            for(auto type: func_types){
                vec.push_back(convert_type(type));
            }
            output::errorPrototypeMismatch(yylineno,id->name, vec);
            exit(0);
        }
    }

    ExpList::ExpList(Exp*  exp)  {
        exp_list.push_back(exp);
    }
    ExpList::ExpList(Exp*  exp, ExpList*  expList)  {
        exp_list.push_back(exp);
        exp_list.insert(exp_list.end(),expList->exp_list.begin(),expList->exp_list.end());
    }

    Exp::Exp(Id*  id) : Typeable(TN_VOID){
        const auto& symbol_data_opt = symbol_table->getSymbol(id->name);
        //std::cout <<"id is: "<< id->name << " type: " << symbol_data_opt->getTypes().back() << std::endl;
        if(symbol_data_opt == nullptr){
            output::errorUndef(yylineno,id->name);
            exit(0);
        } //this exception happened in get symbol
        if(symbol_data_opt->getTypes().size() > 1 ){
            output::errorUndef(yylineno,id->name);
            exit(0);
        }
        this->type = symbol_data_opt->getTypes().back();
        this->value = symbol_data_opt->getName();
        this->reg = loadRegister(symbol_data_opt->getOffset() , symbol_data_opt->getTypes().back());
    }
    Exp::Exp(Call*  call) : Typeable(call->type){
        
    }
    Exp::Exp(Num*  num) : Typeable(TN_INT), value(to_string(num->value)) {
        //std::cout << "type is " << type << std::endl;
        this->reg = generate_register->nextRegister();
        code_buffer.emit("%" + this->reg + " = add i32 0, " + value);
    }

    Exp::Exp(Num*  num, B*  b) : Typeable(TN_BYTE), value(to_string(num->value)) {
        if(num->value > 255){
            output::errorByteTooLarge(yylineno,to_string(num->value));
            exit(0);
        }
        this->reg = generate_register->nextRegister();
        code_buffer.emit("%" + this->reg + " = add i8 0," + value);
    }
    Exp::Exp(String*  str) : Typeable(TN_STR), value(str->value) {
        this->reg = generate_register->nextRegister();
        code_buffer.emitGlobal("@" + reg + "= constant [" + to_string(value.size()) + " x i8] c\"" + value + "\"");
        code_buffer.emit("%" + reg + "= getelementptr [" + to_string(value.size()) + " x i8], [" + to_string(value.size()) + " x i8]* @" + reg + ", i8 0, i8 0");
    }
    Exp::Exp(Boolean*  boolean) : Typeable(TN_BOOL), value(to_string(boolean->value)) {
        this->reg = generate_register->nextRegister();
        code_buffer.emit("%" + this->reg + " = add i1 0," + value);
    }
    Exp::Exp(Not*  _not, Exp*  exp) : Typeable(TN_BOOL) {
        TypeAssert(exp, TN_BOOL);
        if(exp->value == "0"){
            this->value = "1";
        } else{
            this->value = "0";
        }
        this->reg = generate_register->nextRegister();
        code_buffer.emit("%" + this->reg + " = add i1 1," + exp->reg);
    }
    Exp::Exp(Exp*  exp1, And*  _and, Exp*  exp2) : Typeable(TN_BOOL) {
        TypeAssert(exp1, TN_BOOL);
        TypeAssert(exp2, TN_BOOL);
    }
    Exp::Exp(Exp*  exp1, Or*  _or, Exp*  exp2) : Typeable(TN_BOOL) {
        TypeAssert(exp1, TN_BOOL);
        TypeAssert(exp2, TN_BOOL);
    }
    Exp::Exp(Exp*  exp1, Relop*  relop, Exp*  exp2) : Typeable(TN_BOOL) {

        if(!isNumeric(exp1->type) || !isNumeric(exp2->type)){
            //std::cout << "error in relop" << "\n";
            output::errorMismatch(yylineno);
            exit(0);
        }

    }
    Exp::Exp(Type*  type, Exp*  exp) : Typeable(type->type) {
        if(!canExplicitlyAssign(exp->type, type->type)){
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->reg = generate_register->nextRegister();
    }

    Exp::Exp(Exp * exp) : Typeable(exp->type){
    }

    Exp::Exp(Exp *exp1, If *_if, Exp * exp2, Else *_else, Exp * exp3) : Typeable(exp3->type) {
        TypeAssert(exp2,TN_BOOL);

        if(exp1->type != exp3->type and !canExplicitlyAssign(exp1->type,exp3->type)){
            output::errorMismatch(yylineno);
            exit(0);
        }
        if(exp1->type == TN_INT or exp3->type == TN_INT){
            type = TN_INT;
        }
    }



    Exp::Exp(Exp *exp1, Binop *binop, Exp *exp2) : Typeable(exp1->type == TN_BYTE && exp2->type == TN_BYTE ? TN_BYTE : TN_INT){
        if(!isNumeric(exp1->type) ||!isNumeric(exp2->type)){
            output::errorMismatch(yylineno);
            //std::cout << "exp1 type: " << exp1->type << " exp2 type: " << exp2->type << "\n";
            exit(0);
        }
        //cout << "regs: " << exp1->reg << " " << exp2->reg << endl;
        this->reg = generate_register->nextRegister();
        string reg_left = exp1->reg;
        string reg_right = exp2->reg;
        this->type = TN_BYTE;
        string exp_size = "i8";
        if (exp1->type == TN_INT || exp2->type == TN_INT) {
            this->type = TN_INT;
            exp_size = "i32";
            if (exp1->type == TN_BYTE) {
                reg_left = generate_register->nextRegister();
                code_buffer.emit("%" + reg_left + " = zext i8 %" + exp1->reg + " to i32");
            }
            if (exp2->type == TN_BYTE) {
                reg_right = generate_register->nextRegister();
                code_buffer.emit("%" + reg_right + " = zext i8 %" + exp2->reg + " to i32");
            }
        }
        std::string op1;
        if(binop->value == Binop::MUL || binop->value == Binop::ADD) {
            op1 = binop->value == Binop::MUL ? "mul" : "add";
            code_buffer.emit("%" + this->reg + " = " + op1 + " " + exp_size + " %" + reg_left + ", %" + reg_right);
        }else{//div and sub
            //something
            //op1 = binop->value == Binop::DIV ? "sdiv" : "sub";
        }


    }

    Exp::Exp(string x, Exp* exp) : Typeable(TN_BOOL){
        TypeAssert(exp,TN_BOOL);
    }


}