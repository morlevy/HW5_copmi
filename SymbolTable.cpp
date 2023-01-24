//
// Created by alonb on 08/12/2022.
//

#include "SymbolTable.h"
#include "hw3_output.hpp"
#include "Macros.h"
#include <algorithm>
#include <utility>


SymbolTable &SymbolTable::getInstance() noexcept {
    static SymbolTable symbolTable{};
    return symbolTable;
}

void SymbolTable::createNewScope(int statement_num) noexcept {
    //scopes.emplace_back(statement_num);

}

void SymbolTable::closeCurrentScope(int statement_num) noexcept {
    //TODO: output
    scopes.pop_back();
}

void SymbolTable::addNewSymbol(int statement_num, const std::string &name, Typename type) noexcept {
    if (isSymbolExists(name)) {
        output::errorDef(0, name); //TODO
    }
    Scope &scope = scopes.empty() ? global_scope : *scopes.rend();
    auto symbol = new SymbolData(statement_num, name, vector<Typename>{type},"");
    scopes.back().symbols.push_back(symbol);
}

const std::string& SymbolTable::SymbolData::getRegister() const{
    return reg;
}

void SymbolTable::SymbolData::setRegister(std::string reg1){
    this->reg = std::move(reg1);
}

SymbolTable::SymbolData* SymbolTable::getSymbol(const std::string &name) const noexcept {
    for ( auto scope: scopes) {
        if ( auto optional = scope.search(name)) {
            return optional;
        }
    }
    if ( auto optional = global_scope.search(name)) {
        return optional;
    }
    return nullptr;
}

bool SymbolTable::isSymbolExists(const std::string &name) const noexcept {
    for (const auto &scope: scopes) {
        if (scope.search(name)) {
            return true;
        }
    }
    if (global_scope.search(name)) {
        return true;
    }
    return false;
}



SymbolTable::SymbolTable() noexcept {
    scopes.emplace_back();
    scopes.back().symbols.push_back(new SymbolData(0, "print", std::vector<Typename>{ TN_STR , TN_VOID},""));
    scopes.back().symbols.push_back(new SymbolData(0, "printi", std::vector<Typename>{ TN_INT , TN_VOID},""));
    global_functions.emplace_back(-1, "print", std::vector<Typename>{ TN_STR , TN_VOID});
    global_functions.emplace_back(-1, "printi", std::vector<Typename>{ TN_INT , TN_VOID});
    offset_stack.emplace_back(0);
}



const SymbolTable::FunctionSymbolData* SymbolTable::getFunctionSymbol(const string &name) const noexcept {
    const auto& gf = global_functions;
    const auto &it = std::find_if(global_functions.begin(), global_functions.end(), [&](const SymbolTable::FunctionSymbolData &data) noexcept -> bool {return data.getName() == name;});
    if (it != global_functions.end()) {
        return it.base();
    }
    return nullptr;
}


SymbolTable::SymbolData::SymbolData(int offset, std::string name, std::vector<Typename> types, std::string reg, std::string value) :
offset(offset), name(std::move(name)), types(std::move(types)) , reg(reg), value(value){

}

int SymbolTable::SymbolData::getOffset() const {
    return offset;
}

const string &SymbolTable::SymbolData::getName() const {
    return name;
}

vector<Typename> SymbolTable::SymbolData::getTypes() const {
    return types;
}

SymbolTable::SymbolData::SymbolData(int offset, std::string name, Typename type, std::string reg, std::string value): offset(offset),name(name),types(std::vector<Typename>{type}),reg(reg), value(value) {

}

void SymbolTable::SymbolData::setValue(const string &value) {
    SymbolData::value = value;
}

const string &SymbolTable::SymbolData::getValue() const {
    return value;
}


SymbolTable::SymbolData* SymbolTable::Scope::search(const string &name) const noexcept {
    const auto &is_str_eq = [&](const SymbolData* data) noexcept -> bool { return data->getName() == name; };
    const auto iterator = std::find_if(symbols.begin(), symbols.end(), is_str_eq);

    if (iterator != symbols.end()) {
        return *iterator;
    }
    return nullptr;
}

vector<SymbolTable::SymbolData*> SymbolTable::Scope::getSymbols() {
    return symbols;
}

SymbolTable::Scope::Scope() :symbols{}{
}

SymbolTable::FunctionSymbolData::FunctionSymbolData(int line_number, std::string name,
                                                    std::vector<Typename> types) :
        line_number(line_number), name(std::move(name)), types(std::move(types)) {}

int SymbolTable::FunctionSymbolData::getLineNumber() const {
    return line_number;
}

const std::string &SymbolTable::FunctionSymbolData::getName() const {
    return name;
}

Typename SymbolTable::FunctionSymbolData::getType() const {
    return types.back();
}

const std::vector<Typename> &SymbolTable::FunctionSymbolData::getTypes() const {
    return types;
}

