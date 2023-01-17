
#ifndef _GENERATE_REGISTER_HPP_
#define _GENERATE_REGISTER_HPP_
#include <string>
#include <iostream>

using namespace std;

class GenerateRegister {
    private:
    int counter;

    public:
    GenerateRegister():counter(0){}
    string nextRegister(){
        //std::cout << "create register " << counter << std::endl;
        return "t" + to_string(counter++);
    }
};

#endif //_GENERATE_REGISTER_HPP_