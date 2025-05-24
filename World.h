#ifndef WORLD_H
#define WORLD_H

#include "emp/Evolve/World.hpp"
#include "emp/math/random_utils.hpp"
#include "emp/math/Random.hpp"
#include "emp/data/DataFile.hpp"

/**
 * The Daisyworld system, which updates the amount of white and black daisies
 * based on temperature. There are no agents in the world, rather, it inherits from
 * Empirical's world to have access to data files.
 */
class World : emp::World<float> {

    struct GroundCover {
        // the proportion of ground that is covered by the different types of daisies, from 0 to 1
        float proportionWhite;
        float proportionBlack;
        float proportionGray;

        public:

        GroundCover(float _proportionWhite, float _proportionBlack, float _proportionGray = 0.0) : proportionWhite(_proportionWhite), proportionBlack(_proportionBlack), proportionGray(_proportionGray) {}

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
         * @returns the proportion of the world that is covered by gray daisies, from 0 to 1
         */
        float GetProportionGray() {
            return proportionGray;
        }

        /**
         * @returns the proportion of the planet that is not covered by daisies
         */
        float GetProportionGround() {
            // equation (2) of Daisyworld paper
            return 1 - proportionWhite - proportionBlack - proportionGray;
        }

        /**
         * sets the proprotion of white daisies
         */
        void SetProportionWhite(float _proportionWhite) {
            proportionWhite = _proportionWhite;
        }

        /**
         * sets the proprotion of black daisies
         */
        void SetProportionBlack(float _proportionBlack) {
            proportionBlack = _proportionBlack;
        }

        /**
         * sets the proprotion of white daisies
         */
        void SetProportionGray(float _proportionGray) {
            proportionGray = _proportionGray;
        }

        /**
         * Increments the white proportion by delta, clamping between 0 and 1
         */
        void IncrementWhiteAmount(float delta) {
            proportionWhite += delta;
            // clamp values below at 0, don't allow tiny amounts of daisies
            if (proportionWhite < 0.001) proportionWhite = 0.0;
        }

        /**
         * Increments the black proportion by delta, clamping between 0 and 1
         */
        void IncrementBlackAmount(float delta) {
            proportionBlack += delta;
            // clamp values below at 0, don't allow tiny amounts of daisies
            if (proportionBlack < 0.001) proportionBlack = 0.0;
        }

        /**
         * Increments the gray proportion by delta, clamping between 0 and 1
         */
        void IncrementGrayAmount(float delta) {
            proportionGray += delta;
            // clamp values below at 0, don't allow tiny amounts of daisies
            if (proportionGray < 0.001) proportionGray = 0.0;
        }

        /**
         * @returns a weighted average of the albedos of the different types of flowers
         */
        float GetTotalAlbedo(float whiteAlbedo, float blackAlbedo, float groundAlbedo, float grayAlbedo) {
            return (proportionWhite * whiteAlbedo) + (proportionBlack * blackAlbedo) + (GetProportionGround() * groundAlbedo) + (proportionGray * grayAlbedo);
        }
    };
    
    GroundCover ground;
    
    // dimensionless scaling factor for solar luminosity
    float solarLuminosity;

    // whether black/white daisies are allowed to exist
    bool blackDaisiesEnabled;
    bool whiteDaisiesEnabled;
    bool grayDaisiesEnabled;

    // whether daisies can grow or die
    bool daisiesCanGrowAndDie;

    emp::DataMonitor<float>* temperatureMonitor;

    // the albedos of the different colored flowers
    const float whiteAlbedo = 0.75;
    const float blackAlbedo = 0.25;
    const float grayAlbedo = 0.5;
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

    // how much time is incremented each time Update is called
    const float timePerUpdate = 0.01;

    public:

    /**
     * Initializes a starting solar luminosity and flower populations
     */
    World(float _proportionWhite, float _proportionBlack, float _solarLuminosity, float _proportionGray = 0.0) : ground(_proportionWhite, _proportionBlack, _proportionGray), solarLuminosity(_solarLuminosity) {
        whiteDaisiesEnabled = true;
        blackDaisiesEnabled = true;
        grayDaisiesEnabled = false;
        daisiesCanGrowAndDie = true;
        update = 0;
    }

    /**
     * @returns the averaged total albedo over the entire planet (how much sunlight is reflected in aggregate)
     */
    float GetTotalAlbedo() {
        return ground.GetTotalAlbedo(whiteAlbedo, blackAlbedo, groundAlbedo, grayAlbedo);
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
     * Gets the local temperature of the flowers depending on global temperature and their albedo
     * @param albedo The albedo of the local flowers
     * @returns the local temperature over areas with flowers of that albedo, based on global temperature
     */
    float GetTemperatureOfAlbedo(float localAlbedo) {
        // equation (7) of Daisyworld
        return conductivityConstant * (GetTotalAlbedo() - localAlbedo) + GetGlobalTemperature();
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
        return ground.GetProportionWhite();
    }

    /**
     * @returns the proportion of the world that is covered by black daisies, from 0 to 1
     */
    float GetProportionBlack() {
        return ground.GetProportionBlack();
    }

    /**
     * @returns the proportion of the world that is covered by gray daisies, from 0 to 1
     */
    float GetProportionGray() {
        return ground.GetProportionGray();
    }

    /**
     * @returns the proportion of the world that is not covered by daisies, from 0 to 1
     */
    float GetProportionGround() {
        return ground.GetProportionGround();
    }

    /**
     * Enabled or disables black daisies. If disabled, sets their population to 0
     */
    void SetBlackEnabled(bool _blackEnabled) {
        blackDaisiesEnabled = _blackEnabled;
        if (!_blackEnabled) {
            ground.SetProportionBlack(0.0);
        }
    }

    /**
     * Enabled or disables white daisies. If disabled, sets their population to 0
     */
    void SetWhiteEnabled(bool _whiteEnabled) {
        whiteDaisiesEnabled = _whiteEnabled;
        if (!_whiteEnabled) {
            ground.SetProportionWhite(0.0);
        }
    }

    /**
     * Enabled or disables gray daisies. If disabled, sets their population to 0
     */
    void SetGrayEnabled(bool _grayEnabled) {
        grayDaisiesEnabled = _grayEnabled;
        if (!_grayEnabled) {
            ground.SetProportionGray(0.0);
        }
    }

    /**
     * Enables or disables changes in the amounts of daisies
     */
    void SetDaisyGrowthAndDeath(bool _daisiesCanGrowAndDie) {
        daisiesCanGrowAndDie = _daisiesCanGrowAndDie;
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
        return GetProportionWhite() * (GrowthRateFunction(GetTemperatureOfAlbedo(whiteAlbedo)) * GetProportionGround() - deathRate);
    }

    /**
     * @returns the rate of change of the proportion of black daisies per unit time
     */
    float BlackGrowthRate() {
        // equation (1) from Daisyworld paper
        return GetProportionBlack() * (GrowthRateFunction(GetTemperatureOfAlbedo(blackAlbedo)) * GetProportionGround() - deathRate);
    }

    /**
     * @returns the rate of change of the proportion of gray daisies per unit time
     */
    float GrayGrowthRate() {
        // equation (1) from Daisyworld paper
        return GetProportionGray() * (GrowthRateFunction(GetTemperatureOfAlbedo(grayAlbedo)) * GetProportionGround() - deathRate);
    }

    /**
     * Performs one time step, allowing the daisies to grow and die according to temperature as long as growth and
     * death are not disabled
     */
    void Update() {
        emp::World<float>::Update();
        if (daisiesCanGrowAndDie) {
            // the amount that each type of daisy grows this update
            float whiteGrowthAmount = WhiteGrowthRate() * timePerUpdate;
            float blackGrowthAmount = BlackGrowthRate() * timePerUpdate;
            float grayGrowthAmount = GrayGrowthRate() * timePerUpdate;
            // update the amounts of each type of daisy if they are enabled
            if (whiteDaisiesEnabled) ground.IncrementWhiteAmount(whiteGrowthAmount);
            if (blackDaisiesEnabled) ground.IncrementBlackAmount(blackGrowthAmount);
            if (grayDaisiesEnabled) ground.IncrementGrayAmount(grayGrowthAmount);
        }
    }

    /**
     * Sets up a data file tracking the time, solar luminosity, amounts of daisies, and global temperature of Daisyworld
     * @returns the data file
     */
    emp::DataFile& SetupDataFile(const std::string& fileName) {
        emp::DataFile& file = SetupFile(fileName);
        // add variables to the data file
        file.AddVar(update, "t", "update");
        file.AddVar(solarLuminosity, "L", "Solar luminosity");
        file.AddFun<float>([this]() { return GetProportionWhite(); }, "a_w", "Proportion of white daisies");
        file.AddFun<float>([this]() { return GetProportionBlack(); }, "a_b", "Proportion of black daisies");
        if (grayDaisiesEnabled) {
            file.AddFun<float>([this]() { return GetProportionGray(); }, "a_g", "Proportion of gray daisies");
        }
        // calculate the temperature each time the data file is written
        file.AddFun<float>([this]() { return GetGlobalTemperature(); }, "temp", "Global temperature");
        // finish setting up the file
        file.PrintHeaderKeys();
        return file;
    }

    /**
     * How many updates must be run to simulate one time unit in this world
     */
    float GetUpdatesPerTimeUnit() {
        return 1.0 / timePerUpdate;
    }

    /**
     * If the black/white daisies have gone extinct, set their proportion to some small value so they may get started again
     */
    void BoostDaisiesIfExtinct(float whiteBoost = 0.01, float blackBoost = 0.01, float grayBoost = 0.01) {
        if (whiteDaisiesEnabled && GetProportionWhite() < whiteBoost) {
            ground.SetProportionWhite(whiteBoost);;
        }
        if (blackDaisiesEnabled && GetProportionBlack() < blackBoost) {
            ground.SetProportionBlack(blackBoost);
        }
        if (grayDaisiesEnabled && GetProportionGray() < grayBoost) {
            ground.SetProportionGray(grayBoost);
        }
    }
};

#endif
