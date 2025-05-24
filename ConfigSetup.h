#ifndef CONSETUP_H
#define CONSETUP_H

#include "emp/config/ArgManager.hpp"

EMP_BUILD_CONFIG(MyConfigType,
    VALUE(LUMINOSITY, float, 0, "What value should the luminosity be?"),
    VALUE(ENABLE_GRAY, bool, false, "Whether to allow gray daisies to grow")
)

#endif