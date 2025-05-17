#ifndef WORLD_H
#define WORLD_H

#include "emp/Evolve/World.hpp"
#include "emp/math/random_utils.hpp"
#include "emp/math/Random.hpp"

class World {
    
    // the proportion of ground that is covered by the different types of daisies
    float percentWhite;
    float percentBlack;
    
    // dimensionless scaling factor for solar luminosity
    float solarLuminosity;

    // the albedos of the different colored flowers
    const float whiteAlbedo = 0.75;
    const float blackAlbedo = 0.25;
    const float groundAlbedo = 0.5;
    
    // stefan's constant in units of ergs / (second * cm^2 * K^4)
    const float stefansConstant = 0.0000567;
    
    // base value of solar luminosity in ergs / (second * cm^2)
    const float fluxConstant = 917000;

    // add this to convert from Celsius to Kelvin
    const float celsiusToKelvin = 273;

    // the degree to which solar intensity is distributed between different surfaces
    const float conductivityConstant = 20;

    public:

    World(float _percentWhite, float _percentBlack, float _solarLuminosity) : percentWhite(_percentWhite), percentBlack(_percentBlack), solarLuminosity(_solarLuminosity) {}

    float GetPercentGround() {
        return 1 - percentWhite - percentBlack;
    }

    float GetTotalAlbedo() {
        return (percentWhite * whiteAlbedo) + (percentBlack * blackAlbedo) + (GetPercentGround() * groundAlbedo);
    }
    
    float GetGlobalTemperature() {
        float globalAlbedo = GetTotalAlbedo();
        float globalAbsorbsion = 1 - globalAlbedo;
        
        // calculate the global temperature using the Stefan-Boltzman equation
        // equation (4) of Daisyworld
        return std::pow((fluxConstant * solarLuminosity * globalAbsorbsion) / stefansConstant, 0.25) - celsiusToKelvin;
    }

    float GetTemperatureOfBlackFlowers() {
        // equation (7) of Daisyworld
        return conductivityConstant * (GetTotalAlbedo() - blackAlbedo) + GetGlobalTemperature();
    }

    float GetTemperatureOfWhiteFlowers() {
        // equation (7) of Daisyworld
        return conductivityConstant * (GetTotalAlbedo() - whiteAlbedo) + GetGlobalTemperature();
    }
};

#endif