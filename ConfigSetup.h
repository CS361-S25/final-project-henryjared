#ifndef CONSETUP_H
#define CONSETUP_H

#include "emp/config/ArgManager.hpp"

EMP_BUILD_CONFIG(MyConfigType,
    VALUE(LUMINOSITY, float, 0.5, "Change the value of the solar luminosity. See how Daisyworld looks at different luminosities!"),
    VALUE(ADD_BLACK_DAISIES, bool, true, "Enable black daisies on Daisyworld."),
    VALUE(ADD_GRAY_DAISIES, bool, false, "Add a gray daisy to Daisyworld. See how the temperature proportion of daisies changes!"),
    VALUE(ADD_WHITE_DAISIES, bool, true, "Enable white daisies on Daisyworld."),
    VALUE(LATITUDE_SIMULATION, bool, false, "Simulate a Daisyworld with different latitudes. See how the growth pattern of daisies changes!")
)

#endif