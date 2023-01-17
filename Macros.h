//
// Created by alonb on 08/12/2022.
//

#ifndef HW3_MACROS_H
#define HW3_MACROS_H


#define HW3_DEBUG 1

//Macro's flags
#define LEXEME_IN_TOKEN ((HW3_DEBUG) && 1)
#define EXIT_PRINT_FLAG ((HW3_DEBUG) && 1)



#if EXIT_PRINT_FLAG
#include <iostream>
#endif

#if EXIT_PRINT_FLAG
#define EXIT_PRINT(exit_value) \
    do {std::cout << __PRETTY_FUNCTION__ << '\n'; exit(exit_value);} while(0)
#else
#define EXIT_PRINT(exit_value)
#endif

#endif //HW3_MACROS_H
