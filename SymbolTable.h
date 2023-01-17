//
// Created by alonb on 08/12/2022.
//

#ifndef HW3_SYMBOL_TABLE_H
#define HW3_SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <map>
#include "Token.h"
#include "compi_hw3_utility.h"

//Singleton
class SymbolTable {

public:
    SymbolTable& operator=(const SymbolTable&) = delete;
    SymbolTable(const SymbolTable&) = delete;

    class SymbolData{
        const int offset;
        std::string name;
        std::vector<Typename> types;

    public:
        SymbolData() = delete;
        SymbolData(int offset, std::string name, std::vector<Typename> types);
        SymbolData(int offset, std::string name, Typename type);
        [[nodiscard]] int getOffset() const;
        [[nodiscard]] const std::string &getName() const;
        [[nodiscard]] std::vector<Typename> getTypes() const;
        void setType(Typename type_name);
    };

    struct Scope {
        Scope();
        std::vector<SymbolData*> symbols{};
        [[nodiscard]] SymbolData* search(const std::string& name) const noexcept;
        std::vector<SymbolData*> getSymbols();
    };

    class FunctionSymbolData {
        const int line_number;
        std::string name;
        std::vector<Typename> types;

    public:
        FunctionSymbolData() = delete;
        FunctionSymbolData(int line_number, std::string name, std::vector<Typename> types);
        [[nodiscard]] int getLineNumber() const;
        [[nodiscard]] const std::string &getName() const;
        [[nodiscard]] Typename getType() const;
        [[nodiscard]] const std::vector<Typename>& getTypes() const;

    };


    //params:
    Scope global_scope{}; // global variables
    std::vector<FunctionSymbolData> global_functions;
    std::vector<Scope> scopes{};
    std::vector<int> offset_stack;

    static SymbolTable& getInstance() noexcept; //Needed?
    void createNewScope(int statement_num) noexcept;
    void closeCurrentScope(int statement_num) noexcept;
    void addNewSymbol(int statement_num, const std::string& name, Typename data) noexcept;
    [[nodiscard]] bool isSymbolExists(const std::string& name) const noexcept;
    [[nodiscard]] SymbolData* getSymbol(const std::string& name) const noexcept;
    [[nodiscard]] const FunctionSymbolData* getFunctionSymbol(const std::string& name) const noexcept;

    SymbolTable() noexcept;
};


#endif //HW3_SYMBOL_TABLE_H
