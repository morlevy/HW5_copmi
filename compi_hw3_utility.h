//
// Created by alonb on 08/12/2022.
//

#ifndef HW3_COMPI_HW3_UTILITY_H
#define HW3_COMPI_HW3_UTILITY_H

//#include "Grammar.h"

enum Typename {TN_INT, TN_BYTE, TN_BOOL, TN_STR, TN_VOID};

struct Typeable {
    Typeable() = delete;
    explicit Typeable(Typename type);
    Typename type;
};

bool canImplicitlyAssign(Typename src, Typename dst);
bool canExplicitlyAssign(Typename src, Typename dst);

bool canImplicitlyAssign(const Typeable& src, const Typeable& dst);

void TypeAssert(const Typeable* typeable, Typename tn) noexcept;

bool isNumeric(Typename tn);

//bool is_all_params_type_match(const )


#endif //HW3_COMPI_HW3_UTILITY_H
