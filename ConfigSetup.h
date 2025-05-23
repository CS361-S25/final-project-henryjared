#ifndef CONSETUP_H
#define CONSETUP_H

#include "emp/config/ArgManager.hpp"

EMP_BUILD_CONFIG(MyConfigType,
    VALUE(LUMINOSITY, int, 0, "What value should the luminosity be?")
)

#endif