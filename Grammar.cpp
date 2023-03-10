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
#include "parser.tab.hpp"

using namespace std;
SymbolTable *symbol_table;
CodeBuffer &code_buffer = CodeBuffer::instance();
GenerateRegister *generate_register = new GenerateRegister();
int curr_scope = 0;
string current_function;
int currinstr = 0;
int num_args = 0;

inline namespace grammar {

    Program::Program() {
        FUNC_IN
        symbol_table = new SymbolTable();
        code_buffer.emitGlobal("declare i32 @printf(i8*, ...)");
        code_buffer.emitGlobal("declare void @exit(i32)");

        code_buffer.emitGlobal("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
        code_buffer.emitGlobal("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
        code_buffer.emitGlobal("@DavidThrowsZeroExcp = constant [24 x i8] c\"Error division by zero\\0A\\00\"");

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
        code_buffer.emitGlobal(
R"benjo(define void @error_division_by_zero() {
call i32 (i8*, ...) @printf(i8* getelementptr ([24 x i8], [24 x i8]* @DavidThrowsZeroExcp, i32 0, i32 0))
call void @exit(i32 0)
ret void
}
)benjo"
                );
        FUNC_OUT
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
        FUNC_IN
        label_name = code_buffer.genLabel();
        FUNC_OUT
    }


    void startScope() {
        FUNC_IN
        symbol_table->scopes.emplace_back();
        symbol_table->offset_stack.push_back(symbol_table->offset_stack.back());
        FUNC_OUT
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
        FUNC_IN
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
            FUNC_OUT
            return reg2;
        }
        FUNC_OUT
        return reg;
    }

    void closeScope() {
        //output::endScope();
        FUNC_IN
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

        curr_scope.symbols.clear();
        symbol_table->scopes.pop_back();
        symbol_table->offset_stack.pop_back();
        FUNC_OUT
    }

    void endProgram() {
        FUNC_IN
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

    bool idInSymbolTable(const string &id) {
        FUNC_IN
        bool flag = false;
        for (int i = symbol_table->scopes.size() - 1; i >= 0; i--) {
            for (auto &symbol: symbol_table->scopes[i].symbols) {
                if (id == symbol->getName())
                    flag = true;
            }
        }
        FUNC_OUT
        return flag;
    }

    FuncDecl::FuncDecl(RetType *ret_type, Id *id, Formals *formulas) {
        FUNC_IN
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
        formals_llvm_list = "(" + formals_llvm_list.substr(0, formals_llvm_list.size() - 2) + ")";
        currinstr = code_buffer.emit("define " + return_llvm_type + " @" + id->name + formals_llvm_list + " {");
        currinstr = code_buffer.emit("%stack = alloca [50 x i32]");
        currinstr = code_buffer.emit("%args = alloca [ " + to_string(num_args) + " x i32]");
        for (int i = 0; i < formulas->formals.size(); i++) {
            std::string reg = generate_register->nextRegister();
            currinstr = code_buffer.emit(
                    "%" + reg + " = getelementptr [ " + to_string(num_args) + " x i32], [ " + to_string(num_args) +
                    " x i32]* %args, i32 0, i32 " + to_string(num_args - i - 1));
            if (convert_type_llvm(formulas->formals[i]->type) != "i32") {
                string new_reg = generate_register->nextRegister();
                currinstr = code_buffer.emit(
                        "%" + new_reg + " = zext " + convert_type_llvm(formulas->formals[i]->type) + " %" +
                        to_string(i) + " to i32");
                currinstr = code_buffer.emit("store i32 %" + new_reg + ", i32* %" + reg);
            } else {
                int index_param = 0;
                currinstr = code_buffer.emit("store i32 %" + to_string(i)/*formulas->formals[i]->name*/ + ", i32* %" + reg);
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
        FUNC_OUT
    }

    N::N(bool branch, Exp *exp, N* n) : label_stack(n!=nullptr ? n->label_stack : decltype(n->label_stack){}){
        FUNC_IN
        if (branch) {
            currinstr = code_buffer.emit("br i1 %" + exp->reg + ", label @ , label @ ; n-if");
            exp->true_list = CodeBuffer::merge(exp->true_list, CodeBuffer::makelist({currinstr, FIRST}));
            exp->false_list = CodeBuffer::merge(exp->false_list, CodeBuffer::makelist({currinstr, SECOND}));
            exp->label = code_buffer.genLabel();
            this->label = exp->label;
        } else {
            currinstr = code_buffer.emit("br label @ ; n-else");
            this->label = code_buffer.genLabel();
            this->next_list = n != nullptr ?
                              CodeBuffer::merge(n->next_list, CodeBuffer::makelist({currinstr, FIRST})) :
                              CodeBuffer::makelist({currinstr, FIRST});

            label_stack.push_back(this->label);
            PRINT_PARAM("STACK PUSH" + this->label);
        }
        PRINT_PARAM(label_stack.size());
        FUNC_OUT
    }

    void N::merge(N* n) {

    }



    void closeFunction(RetType *retType) {
        FUNC_IN
        string extra = retType->type == TN_VOID ? "" : " 0";
        currinstr = code_buffer.emit("ret " + convert_type_llvm(retType->type) + extra);
        currinstr = code_buffer.emit("}");
        current_function = "";
        num_args = 0;
        FUNC_OUT
    }

    RetType::RetType(Typename type) : type(type) {
    }


    Formals::Formals() {}

    Formals::Formals(FormalsList *formulas_list) {
        FUNC_IN
        this->formals = vector<FormalDecl *>(formulas_list->list);
        FUNC_OUT
    }

    FormalsList::FormalsList(FormalDecl *formal_decl) {
        FUNC_IN
        list.push_back(formal_decl);
        FUNC_OUT
    }

    FormalsList::FormalsList(FormalDecl *formal_decl, FormalsList *formulas_list) {
        FUNC_IN
        list.push_back(formal_decl);
        list.insert(list.end(), formulas_list->list.begin(), formulas_list->list.end());
        FUNC_OUT
    }

    FormalDecl::FormalDecl(Type *type, Id *id) : type(type->type), name(id->name) {
    }

    Statements::Statements(Statement *statement) {
        FUNC_IN
        ////<< "statement next list size: " << statement->next_list.size() << endl;
        this->next_list = statement->next_list;
        this->next_list = statement->next_list;
        this->false_list = statement->false_list;
        ////<< "statement2" << endl;
        FUNC_OUT
    }

    Statements::Statements(Statements *statements, Statement *statement) {
        FUNC_IN
        //code_buffer.bpatch(statements->next_list, label->label_name);
        this->next_list = CodeBuffer::merge(statements->next_list, statement->next_list);
        this->false_list = CodeBuffer::merge(statements->false_list, statement->false_list);
        this->true_list = CodeBuffer::merge(statements->true_list, statement->true_list);
        FUNC_OUT
    }

    Statement::Statement(Type *type, Id *id) : Statement(type, id,
                                                         type->type == TN_BYTE ? new Exp(new Num(0), new B()) :
                                                         type->type == TN_INT  ? new Exp(new Num(0)) :
                                                         type->type == TN_BOOL ? new Exp(new Boolean(false)) :
                                                         new Exp(new String("")))
    {
        FUNC_IN
        /*
        if (idInSymbolTable(id->name)) {
            output::errorDef(yylineno, id->name);
            exit(0);
        }
        symbol_table->scopes.back().symbols.emplace_back(
                new SymbolTable::SymbolData(symbol_table->offset_stack.back()++, id->name, type->type, ""));
        */
        FUNC_OUT
    }

    Statement::Statement(Type *type, Id *id, Exp *exp) {
        FUNC_IN
        if (!canImplicitlyAssign(*exp, *type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if (idInSymbolTable(id->name)) {
            output::errorDef(yylineno, id->name);
            exit(0);
        }
        std::string expReg = exp->reg;
        if(type->type == TN_BOOL && exp->value != "true" && exp->value != "false" && exp->value != "0" && exp->value != "1"){
            expReg = generate_register->nextRegister();
            currinstr = code_buffer.emit("br label @");
            string ltrue = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::merge(exp->true_list, CodeBuffer::makelist({currinstr, FIRST})), ltrue);
            int true_line = code_buffer.emit("br label @");
            string lfalse = code_buffer.genLabel();
            code_buffer.bpatch(exp->false_list, lfalse);
            int false_line = code_buffer.emit("br label @");
            string phi = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::makelist({true_line, FIRST}), phi);
            code_buffer.bpatch(CodeBuffer::makelist({false_line, FIRST}), phi);
            currinstr = code_buffer.emit("%"+expReg + " = phi i1 [ 1, %" + ltrue + "], [ 0, %" + lfalse + "]");
            exp->reg = expReg;
        }

        this->value = generate_register->nextRegister();
        Typename currType = exp->type;
        if (type->type == TN_INT && exp->type == TN_BYTE) {
            expReg = generate_register->nextRegister();
            currinstr = code_buffer.emit("%" + expReg + " = zext i8 %" + exp->reg + " to i32");
            currType = TN_INT;
        }
        currinstr = code_buffer.emit("%" + this->value + " = add " + convert_type_llvm(currType) + " 0,%" + expReg);
        int offset = symbol_table->offset_stack.back()++;
        string ptr = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + ptr +
                                     " = getelementptr [50 x i32], [50 x i32]* %stack, i32 0, i32 " +
                                     to_string(offset));
        expReg = this->value;
        if (currType != TN_INT) {
            //%X = zext i8 %t3 to i32
            expReg = generate_register->nextRegister();
            currinstr = code_buffer.emit(
                    "%" + expReg + " = zext " + convert_type_llvm(currType) + " %" + this->value + " to i32");
        }
        currinstr = code_buffer.emit("store i32 %" + expReg + ", i32* %" + ptr);
        exp->reg = expReg;
        symbol_table->scopes.back().symbols.push_back(
                new SymbolTable::SymbolData(offset, id->name, type->type, expReg, exp->value));
        FUNC_OUT
    }

    Statement::Statement(Id *id, Assign *assign, Exp *exp) {
        FUNC_IN
        SymbolTable::SymbolData *symbol_type = symbol_table->getSymbol(id->name);
        if (symbol_type == nullptr) {
            output::errorUndef(yylineno, id->name);
            exit(0);
        }
        //check if function
        if (symbol_type->getTypes().size() > 1) {
            output::errorUndef(yylineno, id->name);
            FUNC_OUT
            exit(0);
        }
        if (!canImplicitlyAssign(exp->type, symbol_type->getTypes().back())) { //maybe get symbol problem
            output::errorMismatch(yylineno);
            FUNC_OUT
            exit(0);
        }

        if(TN_BOOL == exp->type) {
            auto label_true = exp->label;
            code_buffer.bpatch(exp->true_list, label_true);
            int label_true_loc = code_buffer.emit("br label @ ; t");
            auto label_false = code_buffer.genLabel();
            code_buffer.bpatch(exp->false_list, label_false);
            int label_false_loc = code_buffer.emit("br label @ ; f");
            auto phi_label = code_buffer.genLabel();
            code_buffer.bpatch(code_buffer.makelist({label_true_loc, FIRST}), phi_label);
            code_buffer.bpatch(code_buffer.makelist({label_false_loc, FIRST}), phi_label);
            exp->reg = generate_register->nextRegister();
            code_buffer.emit("%" + exp->reg + " = phi i1 [ 1, %" + label_true + "], [ 0, %" + label_false + "]");
        }

        if(exp->type != TN_INT) {
            auto oldreg = exp->reg;
            exp->reg = generate_register->nextRegister();
            code_buffer.emit("%" + exp->reg + " = zext " + convert_type_llvm(exp->type) + " %" + oldreg + " to i32");
        }
        std::string ptr = generate_register->nextRegister();
        if (symbol_type->getOffset() < 0) {
            currinstr = code_buffer.emit(
                    "%" + ptr + " = getelementptr [ " + to_string(num_args) + " x i32], [ " + to_string(num_args) +
                    " x i32]* %args, i32 0, i32 " + to_string(num_args + symbol_type->getOffset()));
        } else {
            currinstr = code_buffer.emit(
                    "%" + ptr + " = getelementptr [ 50 x i32], [ 50 x i32]* %stack, i32 0, i32 " + to_string(symbol_type->getOffset()));
        }
        code_buffer.emit("store i32 %" + exp->reg + ", i32* %" + ptr);
        symbol_type->setRegister(exp->reg);
        symbol_type->setValue(exp->value);



        //this->next_list = CodeBuffer::makelist({currinstr + 1, FIRST});
        //this->value = generate_register->nextRegister();
        FUNC_OUT
    }

    Statement::Statement(Call *call) : next_list({}) {
        currinstr = CodeBuffer::instance().emit(call->calling_line);
        //next_list = CodeBuffer::makelist({currinstr + 1, FIRST});
    }

    Statement::Statement(Return *_return) {
        FUNC_IN
        currinstr = code_buffer.emit("ret void");
        SymbolTable::SymbolData *function = symbol_table->getSymbol(current_function);
        if (function == nullptr) {//shouldnt get inside
            EXIT_PRINT(0);
        }
        if (function->getTypes().back() != TN_VOID) {
            output::errorMismatch(yylineno);

            exit(0);
        }
        FUNC_OUT
    }

    Statement::Statement(Return *_return, Exp *exp) {
        FUNC_IN
        SymbolTable::SymbolData *function = symbol_table->getSymbol(current_function);
        if (function == nullptr) {
            EXIT_PRINT(0);
        }
        if (function->getTypes().back() != exp->type) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        string temp_reg = exp->reg;
        if(exp->type == TN_BOOL && exp->value != "true" && exp->value != "false" && exp->value != "0" && exp->value != "1") {
            temp_reg = generate_register->nextRegister();
            currinstr = code_buffer.emit("br label @");
            string ltrue = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::merge(exp->true_list, CodeBuffer::makelist({currinstr, FIRST})), ltrue);
            int true_line = code_buffer.emit("br label @");
            string lfalse = code_buffer.genLabel();
            code_buffer.bpatch(exp->false_list, lfalse);
            int false_line = code_buffer.emit("br label @");
            string phi = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::makelist({true_line, FIRST}), phi);
            code_buffer.bpatch(CodeBuffer::makelist({false_line, FIRST}), phi);
            currinstr = code_buffer.emit("%"+temp_reg + " = phi i1 [ 1, %" + ltrue + "], [ 0, %" + lfalse + "]");

        }
        code_buffer.emit("ret " + convert_type_llvm(function->getTypes().back())+ " %" + temp_reg);
        FUNC_OUT
    }

    Statement::Statement(If *if_, Exp *exp, Statement *statement) {
        FUNC_IN
        TypeAssert(exp, TN_BOOL);
        ////<< "if" << endl;
        //code_buffer.bpatch(exp->true_list, label->label_name);
        ////<< "arrived here 1\n";
        this->next_list = CodeBuffer::merge(exp->false_list, statement->next_list);
        ////<< "arrived here 2\n";

        currinstr = code_buffer.emit("br label @");
        string label = code_buffer.genLabel();
        code_buffer.bpatch(CodeBuffer::merge(exp->false_list, CodeBuffer::makelist({currinstr, FIRST})),
                           label);
        FUNC_OUT
    }

    Statement::Statement(If *_if, Exp *exp, Statement *statement1, N *n, Else *_else, Statement *statement2) {
        FUNC_IN
        TypeAssert(exp, TN_BOOL);
        //code_buffer.bpatch(exp->true_list, label1->label_name);
        //code_buffer.bpatch(exp->false_list, label2->label_name);

        ////<< "size of statement1 next list: " << statement1->next_list.size() << endl;
        ////<< "size of statement2 next list: " << statement2->next_list.size() << endl;
        ////<< "size of n next list: " << n->next_list.size() << endl;
        //this->next_list = CodeBuffer::merge(CodeBuffer::merge(statement1->next_list, n->next_list), statement2->next_list);
        ////<< "arrived here 6\n" << "and next list size is " << this->next_list.size() << endl;

        currinstr = code_buffer.emit("br label @");
        string label2 = code_buffer.genLabel();
        code_buffer.bpatch(CodeBuffer::merge(n->next_list, CodeBuffer::makelist({currinstr, FIRST})), label2);
        auto label = n->label_stack.back();
        code_buffer.bpatch(exp->false_list, label);
        n->label_stack.pop_back();
        PRINT_PARAM("STACK POP" + label);
        PRINT_PARAM(n->label_stack.size());

        next_list = CodeBuffer::merge(statement1->next_list, statement2->next_list);
        FUNC_OUT
    }

    Statement::Statement(While *_while, Exp *exp) {
        FUNC_IN
        TypeAssert(exp, TN_BOOL);
        FUNC_OUT
    }

    std::vector<std::string> while_stack_labels{};

    void start_while() {
        FUNC_IN
        auto loc = code_buffer.emit("br label @");
        auto w_label = code_buffer.genLabel();
        code_buffer.bpatch(CodeBuffer::makelist({loc, FIRST}), w_label);
        while_stack_labels.push_back(w_label);
        FUNC_OUT
    }

    void end_while(Exp *e, Statement *s) {
        FUNC_IN
        auto w_label = while_stack_labels.front();
        code_buffer.emit("br label %" + w_label);
        code_buffer.bpatch(CodeBuffer::merge(e->false_list, s->next_list), code_buffer.genLabel());
        code_buffer.bpatch(s->true_list, w_label);
        while_stack_labels.pop_back();
        FUNC_OUT
    }

    Statement::Statement(Break *_break) {
        FUNC_IN
        if (while_stack_labels.empty()) {
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }
        int loc = code_buffer.emit("br label @");
        code_buffer.genLabel();
        this->next_list = CodeBuffer::makelist({loc, FIRST});
        FUNC_OUT
    }

    Statement::Statement(Continue *_continue) {
        FUNC_IN
        if (while_stack_labels.empty()) {
            output::errorUnexpectedContinue(yylineno);
            exit(0);
        }
        int loc = code_buffer.emit("br label %" + while_stack_labels.front());
        //code_buffer.genLabel();
        //this->true_list = CodeBuffer::makelist({loc, FIRST});
        FUNC_OUT
    }

    Statement::Statement(Statements *s) {
        FUNC_IN
        // << "arrived here 4 size is: " << s->next_list.size() << endl;
        this->next_list = s->next_list;
        this->false_list = s->false_list;
        this->true_list = s->true_list;
        // << "arrived here 5\n";
        FUNC_OUT
    }

    Call::Call(Id *id, ExpList *expList) : Typeable(TN_VOID) {
        FUNC_IN
        DO_DEBUG(
                cout << "\n-------------------------- CALL --------------------------\n\n";
        )

        auto func = symbol_table->getFunctionSymbol(id->name);
        if (func == nullptr) {
            output::errorUndefFunc(yylineno, id->name);
            exit(0);
        }
        auto func_types = func->getTypes();
        type = func_types.back();
        func_types.pop_back();
        vector<string> vector2{func_types.size()};
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

        std::string line = "call " + convert_type_llvm(type) + " @" + id->name + "(";
        for (auto i = 0; i < func_types.size(); i++) {
            Exp &exp = *expList->exp_list[i];
            auto arg_type = func_types[i];
            if (arg_type == TN_INT && exp.type == TN_BYTE) {
                auto old_reg = exp.reg;
                exp.reg = generate_register->nextRegister();
                CodeBuffer::instance().emit("%" + exp.reg + " = zext i8 %" + old_reg + " to i32");
                exp.type = TN_INT;
            }
            if(exp.type == TN_BOOL){
                code_buffer.bpatch(exp.true_list, exp.label);
                code_buffer.bpatch(exp.false_list, exp.label);
                code_buffer.bpatch(exp.next_list, exp.label);
            }
            line += convert_type_llvm(exp.type) + "%" + exp.reg;
            if (i + 1 < func_types.size()) {
                line += ", ";
            }
        }
        line += ")";
        calling_line = line;
        FUNC_OUT
    }

    Call::Call(Id *id) : Typeable(TN_VOID) {
        FUNC_IN
        auto func = symbol_table->getFunctionSymbol(id->name);
        DO_DEBUG(
                    cout << "\n-------------------------- CALL --------------------------\n\n";
                )
        if (func == nullptr) {
            output::errorUndefFunc(yylineno, id->name);
            exit(0);
        }
        auto func_types = func->getTypes();
        type = func_types.back();
        func_types.pop_back();

        if (func_types.back() != TN_VOID) {
            vector<string> vec{func_types.size()};
            for (auto type: func_types) {
                vec.push_back(convert_type(type));
            }
            output::errorPrototypeMismatch(yylineno, id->name, vec);
            exit(0);
        }
        calling_line = "call " + convert_type_llvm(type) + " @" + id->name + "()";
        FUNC_OUT
    }

    ExpList::ExpList(Exp *exp) {
        FUNC_IN
        exp_list.push_back(exp);
        if(exp->type == TN_BOOL && exp->value != "true" && exp->value != "false" && exp->value != "0" && exp->value != "1"){
            int first = code_buffer.emit("br label @");
            string ltrue = code_buffer.genLabel();
            code_buffer.bpatch(exp->true_list, ltrue);
            int second = code_buffer.emit("br label @");
            string false_label = code_buffer.genLabel();
            code_buffer.bpatch(exp->false_list, false_label);
            int third = code_buffer.emit("br label @");
            std::string phi = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::makelist({second, FIRST}), phi);
            code_buffer.bpatch(CodeBuffer::makelist({third, FIRST}), phi);
            exp->reg = generate_register->nextRegister();
            currinstr = code_buffer.emit("%"+exp->reg + " = phi i1 [ 1, %" + ltrue + "], [ 0, %" + false_label + "]");
            currinstr = code_buffer.emit("br label %" + exp->label);
            string jump_label = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::makelist({first, FIRST}), jump_label);
        }
        FUNC_OUT
    }

    ExpList::ExpList(Exp *exp, ExpList *expList) {
        FUNC_IN
        exp_list.push_back(exp);
        if(exp->type == TN_BOOL && exp->value != "true" && exp->value != "false" && exp->value != "0" && exp->value != "1"){
            int first = code_buffer.emit("br label @");
            string ltrue = code_buffer.genLabel();
            code_buffer.bpatch(exp->true_list, ltrue);
            int second = code_buffer.emit("br label @");
            string false_label = code_buffer.genLabel();
            code_buffer.bpatch(exp->false_list, false_label);
            int third = code_buffer.emit("br label @");
            std::string phi = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::makelist({second, FIRST}), phi);
            code_buffer.bpatch(CodeBuffer::makelist({third, FIRST}), phi);
            exp->reg = generate_register->nextRegister();
            currinstr = code_buffer.emit("%"+exp->reg + " = phi i1 [ 1, %" + ltrue + "], [ 0, %" + false_label + "]");
            currinstr = code_buffer.emit("br label %" + exp->label);
            string jump_label = code_buffer.genLabel();
            code_buffer.bpatch(CodeBuffer::makelist({first, FIRST}), jump_label);
        }
        exp_list.insert(exp_list.end(), expList->exp_list.begin(), expList->exp_list.end());
        FUNC_OUT
    }

    Exp::Exp(Id *id) : Typeable(TN_VOID) {
        FUNC_IN
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
        this->value = id->name; //symbol_data_opt->getName(); TODO maybe change the value
        this->reg = loadRegister(symbol_data_opt->getOffset(), symbol_data_opt->getTypes().back());
        if(this->type == TN_BOOL && this->value != "true" && this->value != "false" && this->value != "0" && this->value != "1"){
            currinstr = code_buffer.emit("br i1 %" + this->reg + ", label @, label @");
            this->true_list = CodeBuffer::merge(this->true_list, CodeBuffer::makelist({currinstr, FIRST}));
            this->false_list = CodeBuffer::merge(this->false_list, CodeBuffer::makelist({currinstr, SECOND}));
            this->label = code_buffer.genLabel();
        }
        FUNC_OUT
    }

    Exp::Exp(Call *call) : Typeable(call->type) {
        if(call->type != TN_VOID){
            reg = generate_register->nextRegister();
            CodeBuffer::instance().emit("%" + reg + " = " + call->calling_line);
        }
        else{
            CodeBuffer::instance().emit(call->calling_line);
        }
        value = call->calling_line;
    }

    Exp::Exp(Num *num) : Typeable(TN_INT), value(to_string(num->value)) {
        FUNC_IN
        //std::cout << "type is " << type << std::endl;
        this->reg = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + this->reg + " = add i32 0, " + value);
        FUNC_OUT
    }

    Exp::Exp(Num *num, B *b) : Typeable(TN_BYTE), value(to_string(num->value)) {
        FUNC_IN
        if (num->value > 255) {
            output::errorByteTooLarge(yylineno, to_string(num->value));
            FUNC_OUT
            exit(0);
        }
        this->reg = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + this->reg + " = add i8 0, " + value);
        FUNC_OUT
    }

    Exp::Exp(String *str) : Typeable(TN_STR), value(str->value) {
        FUNC_IN
        this->reg = generate_register->nextRegister();
        code_buffer.emitGlobal("@" + reg + "= constant [" + to_string(value.size() - 2) + " x i8] c\"" + value + '\"');
        currinstr = code_buffer.emit(
                "%" + reg + "= getelementptr [" + to_string(value.size() - 2) + " x i8], [" +
                to_string(value.size() - 2) +
                " x i8]* @" + reg + ", i8 0, i8 0");
        FUNC_OUT
    }

    Exp::Exp(Boolean *boolean) : Typeable(TN_BOOL), value(to_string(boolean->value)) {
        FUNC_IN
        this->reg = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + this->reg + " = add i1 0," + value);
        /**
        currinstr = code_buffer.emit("br label @");
        auto list = CodeBuffer::makelist({currinstr,FIRST});
        this->false_list ={};
        this->true_list = {};
        if(boolean->value){
            this->true_list = list;
            ////<< "true\n";
        } else {
            this->false_list = list;
        }**/
        FUNC_OUT
    }

    Exp::Exp(Not *_not, Exp *exp) : Typeable(TN_BOOL), label(exp->label) {
        FUNC_IN
        TypeAssert(exp, TN_BOOL);
        if (exp->value == "0") {
            this->value = "1";
        } else {
            this->value = "0";
        }
        this->reg = generate_register->nextRegister();
        currinstr = code_buffer.emit("%" + this->reg + " = add i1 1, %" + exp->reg);
        this->false_list = exp->true_list;
        this->true_list = exp->false_list;
        FUNC_OUT
    }

    Exp::Exp(Exp *exp1, And *_and, Exp *exp2) : Typeable(TN_BOOL) {
        FUNC_IN
        TypeAssert(exp1, TN_BOOL);
        TypeAssert(exp2, TN_BOOL);

        code_buffer.bpatch(exp1->true_list, exp1->label);

        if (exp2->value == "true" || exp2->value == "false" || exp2->value == "1" || exp2->value == "0") {
            int loc = code_buffer.emit("br i1 %" + exp2->reg + ", label @ , label @");
            exp2->true_list = code_buffer.merge(exp2->true_list, code_buffer.makelist({loc, FIRST}));
            exp2->false_list = code_buffer.merge(exp2->false_list, code_buffer.makelist({loc, SECOND}));
            exp2->label = code_buffer.genLabel();
        }

        //int label_location = code_buffer.emit("br i1 %" + exp1->reg + ", label @, label @");
        //this->reg = generate_register->nextRegister();
        //TODO why
        this->true_list = exp2->true_list;
        this->false_list = CodeBuffer::merge(exp1->false_list, exp2->false_list);
        this->label = exp2->label;
        FUNC_OUT

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

    Exp::Exp(Exp *exp1, Or *_or, Exp *exp2) : Typeable(TN_BOOL) {
        FUNC_IN
        TypeAssert(exp1, TN_BOOL);
        TypeAssert(exp2, TN_BOOL);

        code_buffer.bpatch(exp1->false_list, exp1->label);
        if (exp2->value == "true" || exp2->value == "false" || exp2->value == "1" || exp2->value == "0") {
            int loc = code_buffer.emit("br i1 %" + exp2->reg + ", label @ , label @");
            exp2->true_list = code_buffer.merge(exp2->true_list, code_buffer.makelist({loc, FIRST}));
            exp2->false_list = code_buffer.merge(exp2->false_list, code_buffer.makelist({loc, SECOND}));
            exp2->label = code_buffer.genLabel();
        }
        this->false_list = exp2->false_list;
        this->true_list = CodeBuffer::merge(exp1->true_list, exp2->true_list);
        this->label = exp2->label;
        FUNC_OUT
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

    string convert_to_llvm_relop(Relop::RelopValue val) {
        switch (val) {
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
        FUNC_IN
        if (!isNumeric(exp1->type) || !isNumeric(exp2->type)) {
            //////<< "error in relop" << "\n";
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->reg = generate_register->nextRegister();
        //code_buffer.emit("relop value: " + to_string((relop->value)));
        if(exp1->type != TN_INT){
            auto oldreg = exp1->reg;
            exp1->reg = generate_register->nextRegister();
            code_buffer.emit("%"+exp1->reg + " = zext " + convert_type_llvm(exp1->type) + " %" + oldreg + " to i32");
            exp1->type = TN_INT;
        }

        if(exp2->type != TN_INT){
            auto oldreg = exp2->reg;
            exp2->reg = generate_register->nextRegister();
            code_buffer.emit("%"+exp2->reg + " = zext " + convert_type_llvm(exp2->type) + " %" + oldreg + " to i32");
            exp2->type = TN_INT;
        }

        currinstr = code_buffer.emit(
                "%" + this->reg + " = icmp " + convert_to_llvm_relop(relop->value) + " i32 %" + exp1->reg + ", %" +
                exp2->reg);
        currinstr = code_buffer.emit("br i1 %" + this->reg + ", label @, label @ ; relop");
        this->true_list = CodeBuffer::makelist({currinstr, FIRST});
        this->false_list = CodeBuffer::makelist({currinstr, SECOND});
        this->label = code_buffer.genLabel();
        //this->value = "(" + exp1->value + relop->value + exp2->value + ")";
        FUNC_OUT
    }

    Exp::Exp(Type *type, Exp *exp) : Typeable(type->type) {
        FUNC_IN
        if (!canExplicitlyAssign(exp->type, type->type)) {
            output::errorMismatch(yylineno);
            FUNC_OUT
            exit(0);
        }
        this->reg = generate_register->nextRegister();
        this->label = exp->label;
        FUNC_OUT
    }

    Exp::Exp(Exp *exp) : Typeable(exp->type), reg(exp->reg), false_list(exp->false_list), true_list(exp->true_list),
                         label(exp->label), value(exp->value) {
        FUNC_IN
        //code_buffer.emit("(exp)="+convert_type(exp->type));
        FUNC_OUT
    }


    Exp::Exp(Exp *exp1, If *_if, Exp *exp2, Else *_else, Exp *exp3) : Typeable(exp3->type) {
        FUNC_IN
        TypeAssert(exp2, TN_BOOL);
        if (exp1->type != exp3->type and !canExplicitlyAssign(exp1->type, exp3->type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if (exp1->type == TN_INT or exp3->type == TN_INT) {
            type = TN_INT;
        }
        FUNC_OUT
    }


    Exp::Exp(Exp *exp1, Binop *binop, Exp *exp2) : Typeable(
            exp1->type == TN_INT || exp2->type == TN_INT ? TN_INT : TN_BYTE) {
        FUNC_IN
        if (!isNumeric(exp1->type) || !isNumeric(exp2->type)) {
            output::errorMismatch(yylineno);
            //////<< "exp1 type: " << exp1->type << " exp2 type: " << exp2->type << "\n";
            exit(0);
        }
        //////<< "regs: " << exp1->reg << " " << exp2->reg << endl;
        //TODO: verify signed vs unsigned, implement void @error_division_by_zero()
        this->reg = generate_register->nextRegister();
        std::string reg_left = exp1->reg;
        std::string reg_right = exp2->reg;
        const bool is_any_int = exp1->type == TN_INT || exp2->type == TN_INT;
        const std::string exp_size = is_any_int ? "i32" : "i8";
        std::string op = [&]() {
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
        this->value = "(" + exp1->value + " " + op + " " + exp2->value + ")";
        PRINT_PARAM(value);
        if (binop->value == Binop::DIV) {
            // * division by zero
            const auto &reg1 = generate_register->nextRegister("div_by_zero");
            currinstr = code_buffer.emit("%" + reg1 + " = icmp eq " + convert_type_llvm(exp2->type) + " %" + exp2->reg + ", 0");
            const int need_back_patch = currinstr = code_buffer.emit("br i1 %" + reg1 + ", label @, label @");
            const std::string &yes_div_zero = code_buffer.genLabel();
            currinstr = code_buffer.emit("call void @error_division_by_zero()");
            int loc = code_buffer.emit("br label @");
            const std::string &no_div_zero = code_buffer.genLabel();
            code_buffer.bpatch(code_buffer.makelist({loc, FIRST}), no_div_zero);
            code_buffer.bpatch(std::vector{make_pair(need_back_patch, FIRST)}, yes_div_zero);
            code_buffer.bpatch(std::vector{make_pair(need_back_patch, SECOND)}, no_div_zero);
            if(exp_size == "i8"){
                op = "udiv";
            }
        }
        currinstr = code_buffer.emit(
                "%" + this->reg + " = " + op + " " + exp_size + " %" + reg_left + ", %" + reg_right);
        FUNC_OUT
    }

    Exp::Exp(const string &x, Exp *exp) : Typeable(TN_BOOL), reg(exp->reg), false_list(exp->false_list),
                                          true_list(exp->true_list) {
        FUNC_IN
        TypeAssert(exp, TN_BOOL);
        if (exp->value == "true" || exp->value == "false" || exp->value == "0" || exp->value == "1" || exp->value == "null") {
            currinstr = code_buffer.emit("br i1 %" + exp->reg + ", label @, label @");
            exp->true_list = code_buffer.merge(exp->true_list, code_buffer.makelist({currinstr, FIRST}));
            exp->false_list = code_buffer.merge(exp->false_list, code_buffer.makelist({currinstr, SECOND}));
            exp->label = code_buffer.genLabel();
        }
        code_buffer.bpatch(exp->true_list, exp->label);
        this->false_list = exp->false_list;
        this->true_list = exp->true_list;
        this->label = exp->label;
        this->reg = exp->reg;
        FUNC_OUT
    }

    void end_function(RetType *ret) {
        FUNC_IN
        current_function = "";
        closeScope();
        closeFunction(ret);
        FUNC_OUT
    }
}
