
#ifndef _GENERATE_REGISTER_HPP_
#define _GENERATE_REGISTER_HPP_
#include <string>
#include <iostream>

# pragma once

using namespace std;

class GenerateRegister {
    private:
    int counter;

    public:
    GenerateRegister() : counter(0) {}
    string nextRegister(const string& prefix = ""){
        //std::cout << "create register " << counter << std::endl;
        return prefix + string{prefix.empty() ? "" : "_"} + "t" + to_string(counter++);
    }
};

#endif //_GENERATE_REGISTER_HPP_