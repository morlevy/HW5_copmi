//
// Created by alonb on 08/12/2022.
//

#ifndef HW3_MACROS_H
#define HW3_MACROS_H


#define HW3_DEBUG 1

//Macro's flags
#define LEXEME_IN_TOKEN  ((HW3_DEBUG) && 1)
#define EXIT_PRINT_FLAG  ((HW3_DEBUG) && 1)
#define PRINT_PARAM_FLAG ((HW3_DEBUG) && 1)
#define DO_DEBUG_FLAG    ((HW3_DEBUG) && 1)
#define FUNC_IN_OUT_FLAG ((HW3_DEBUG) && 1)



#if EXIT_PRINT_FLAG
#include <iostream>
#endif

#if EXIT_PRINT_FLAG
#define EXIT_PRINT(exit_value) \
    do {std::cout << __PRETTY_FUNCTION__ << '\n'; exit(exit_value);} while(0);
#else
#define EXIT_PRINT(exit_value)
#endif

#if PRINT_PARAM_FLAG
#define PRINT_PARAM(parameter) \
    do {std::cout << "PRINT_PARAM:\t" #parameter " = " << parameter << '\n';} while(0);
#else
#define PRINT_PARAM(parameter)
#endif

#if DO_DEBUG_FLAG
#define DO_DEBUG(code_to_exe) \
    do {code_to_exe} while(0);
#else
#define DO_DEBUG(parameter)
#endif

#if FUNC_IN_OUT_FLAG
#define FUNC_IN \
do {std::cout << __PRETTY_FUNCTION__ << "--> in -->" << '\n';} while(0);
#define FUNC_OUT \
do {std::cout << __PRETTY_FUNCTION__ << "<-- out <--" << '\n';} while(0);
#else
#define FUNC_IN
#define FUNC_OUT
#endif

#endif //HW3_MACROS_H
