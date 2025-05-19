#ifndef WORLD_H
#define WORLD_H

#include "emp/Evolve/World.hpp"
#include "emp/math/random_utils.hpp"
#include "emp/math/Random.hpp"

class World {
    
    // the proportion of ground that is covered by the different types of daisies, from 0 to 1
    float proportionWhite;
    float proportionBlack;
    
    // dimensionless scaling factor for solar luminosity
    float solarLuminosity;

    // whether black/white daisies are allowed to exist
    bool blackDaisiesEnabled;
    bool whiteDaisiesEnabled;

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

    // the death rate of daisies per time
    const float deathRate = 0.3;

    public:

    /**
     * Initializes a starting solar luminosity and flower populations
     */
    World(float _percentWhite, float _percentBlack, float _solarLuminosity) : proportionWhite(_percentWhite), proportionBlack(_percentBlack), solarLuminosity(_solarLuminosity) {}

    /**
     * @returns the proportion of the planet that is not covered by daisies
     */
    float GetProportionGround() {
        return 1 - proportionWhite - proportionBlack;
    }

    /**
     * @returns a weighted average of albedos of the different flowers
     */
    float GetTotalAlbedo() {
        return (proportionWhite * whiteAlbedo) + (proportionBlack * blackAlbedo) + (GetProportionGround() * groundAlbedo);
    }
    
    /**
     * @returns the average global temperature of the planet in Celsius, based on average albedo and solar luminosity
     */
    float GetGlobalTemperature() {
        float globalAlbedo = GetTotalAlbedo();
        float globalAbsorbsion = 1 - globalAlbedo;
        
        // calculate the global temperature using the Stefan-Boltzman equation
        // equation (4) of Daisyworld
        return std::pow((fluxConstant * solarLuminosity * globalAbsorbsion) / stefansConstant, 0.25) - celsiusToKelvin;
    }

    /**
     * @returns the local temperature over areas with black flowers, based on black flowers' albedo and conduction from global temperature
     */
    float GetTemperatureOfBlackFlowers() {
        // equation (7) of Daisyworld
        return conductivityConstant * (GetTotalAlbedo() - blackAlbedo) + GetGlobalTemperature();
    }

    /**
     * @returns the local temperature over areas with white flowers, based on white flowers' albedo and conduction from global temperature
     */
    float GetTemperatureOfWhiteFlowers() {
        // equation (7) of Daisyworld
        return conductivityConstant * (GetTotalAlbedo() - whiteAlbedo) + GetGlobalTemperature();
    }

    /**
     * Sets the dimensionless solar luminosity of the world
     */
    void SetSolarLuminosity(float _solarLuminosity) {
        solarLuminosity = _solarLuminosity;
    }

    /**
     * @returns the dimensionless solar luminosity, with values typically around 1
     */
    float GetSolarLuminosity() {
        return solarLuminosity;
    }

    /**
     * @returns the proportion of the world that is covered by white daisies, from 0 to 1
     */
    float GetProportionWhite() {
        return proportionWhite;
    }

    /**
     * @returns the proportion of the world that is covered by black daisies, from 0 to 1
     */
    float GetProportionBlack() {
        return proportionBlack;
    }

    /**
     * Enabled or disables black daisies. If disabled, sets their population to 0
     */
    void SetBlackEnabled(bool _blackEnabled) {
        blackDaisiesEnabled = _blackEnabled;
        if (!_blackEnabled) {
            proportionBlack = 0.0;
        }
    }

    /**
     * Enabled or disables white daisies. If disabled, sets their population to 0
     */
    void SetWhiteEnabled(bool _whiteEnabled) {
        whiteDaisiesEnabled = _whiteEnabled;
        if (!_whiteEnabled) {
            proportionWhite = 0.0;
        }
    }

    /**
     * @param localTemperature The local temperature over this type of flower
     * @returns the growth rate on bare ground of this type of daisy
     */
    float GrowthRateFunction(float localTemperature) {
        // equation (3) from Daisyworld paper
        return 1 - 0.003265 * (22.5 - localTemperature) * (22.5 - localTemperature);
    }

    /**
     * @returns the rate of change of the proportion of white daisies per unit time
     */
    float WhiteGrowthRate() {
        // equation (1) from Daisyworld paper
        return proportionWhite * (GrowthRateFunction(GetTemperatureOfWhiteFlowers()) * GetProportionGround() - deathRate);
    }

    /**
     * @returns the rate of change of the proportion of black daisies per unit time
     */
    float BlackGrowthRate() {
        // equation (1) from Daisyworld paper
        return proportionBlack * (GrowthRateFunction(GetTemperatureOfBlackFlowers()) * GetProportionGround() - deathRate);
    }
};

#endif