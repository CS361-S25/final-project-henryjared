#ifndef WORLD_H
#define WORLD_H

#include "emp/Evolve/World.hpp"
#include "emp/math/random_utils.hpp"
#include "emp/math/Random.hpp"
#include "emp/data/DataFile.hpp"
#include <limits>

/**
 * The Daisyworld system, which updates the amount of white and black daisies
 * based on temperature. There are no agents in the world, rather, it inherits from
 * Empirical's world to have access to data files.
 */
class World : emp::World<float> {

    /**
     * Holds the amount of white, black, and gray daisies on the ground
     */
    struct GroundCover {
        /**
         * The proportion of ground that is covered by the different kinds of daisies
         */
        float proportionWhite;
        float proportionBlack;
        float proportionGray;

        GroundCover(float _proportionWhite = 0.33, float _proportionBlack = 0.33, float _proportionGray = 0.0) : proportionWhite(_proportionWhite), proportionBlack(_proportionBlack), proportionGray(_proportionGray) {}

        /**
         * @returns the proportion of the planet that is not covered by daisies
         */
        float GetProportionGround() {
            // equation (2) of Daisyworld paper
            return 1 - proportionWhite - proportionBlack - proportionGray;
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
        float GetTotalAlbedo() {
            return (proportionWhite * whiteAlbedo) + (proportionBlack * blackAlbedo) + (GetProportionGround() * groundAlbedo) + (proportionGray * grayAlbedo);
        }
    };
    
    /**
     * The proportion of ground covered over the entire flat planet
     */
    GroundCover ground;

    /**
     * Whether the world is round. Flat worlds have a single homogenous population of daisies. Round worlds have
     * different populations of daisies at different latitudes. This determines while ground or groundAtLatitudes is used.
     */
    bool roundWorld = false;
    
    // dimensionless scaling factor for solar luminosity
    float solarLuminosity = 1.0;

    // whether black/white daisies are allowed to exist
    bool blackDaisiesEnabled;
    bool whiteDaisiesEnabled;
    bool grayDaisiesEnabled;

    // whether daisies can grow or die
    bool daisiesCanGrowAndDie;

    emp::DataMonitor<float>* temperatureMonitor;

    // the albedos of the different colored flowers
    static constexpr float whiteAlbedo = 0.75;
    static constexpr float blackAlbedo = 0.25;
    static constexpr float grayAlbedo = 0.5;
    static constexpr float groundAlbedo = 0.5;
    
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

    // the number of latitudes the round planet is subdivided into
    static constexpr int numberOfLatitudes = 90;

    // the number of latitudes that are visible on the display
    static constexpr int numberOfDisplayedLatitudes = 10;

    /**
     * The proportion of ground covered by the different daisies at different latitudes.
     */
    GroundCover groundAtLatitudes[numberOfLatitudes] = {};

    // how luminosity changes over different latitudes on a round planet
    const float minLuminosityMultiplier = 0.6;
    const float maxLuminosityMultiplier = 1.5;

    public:

    /**
     * Initializes a starting solar luminosity and flower populations.
     * @param _roundWorld Whether to compute different temperatures at different latitudes of the planet
     */
    World(float _proportionWhite, float _proportionBlack, float _solarLuminosity, float _proportionGray = 0.0f, bool _roundWorld = false)
        : ground(_proportionWhite, _proportionBlack, _proportionGray), solarLuminosity(_solarLuminosity), roundWorld(_roundWorld) {
        whiteDaisiesEnabled = true;
        blackDaisiesEnabled = true;
        grayDaisiesEnabled = false;
        daisiesCanGrowAndDie = true;
        update = 0;
    }

    /** 
     * 
     */
    void SetRoundWorld(bool _roundworld) {
        roundWorld = _roundworld;
    }

    /**
     * What proportion of the sun's aggregate luminosity translates into sunlight shining on this latitude.
     * @param latitude The latitude on the planet. Ranges from 0 to 9, where 0 is polar and 9 is equatorial.
     * @returns a number from minLuminosityMultiplier to maxLuminosityMultiplier, linearly interpolated.
     * This function times solarLuminosity times fluxConstant is the light flux reaching this latitude.
     */
    float GetLuminosityMultiplierAtLatitude(int latitude) {
        return minLuminosityMultiplier + (maxLuminosityMultiplier - minLuminosityMultiplier) / (numberOfLatitudes - 1) * latitude;
    }

    /**
     * @returns the averaged total albedo over the entire planet (how much sunlight is reflected in aggregate). If the world is round
     */
    float GetTotalAlbedo() {
        if (roundWorld) {
            return GetAverageAlbedoOnRoundPlanet();
        } else {
            return ground.GetTotalAlbedo();
        }
    }

    /**
     * @returns The amount of sunlight that is reflected overall on a round planet, where absorbsions on higher latitudes
     * with less sunlight are weighted less
     */
    float GetAverageAlbedoOnRoundPlanet() {
        float totalGlobalAbsorbsion = 0.0;
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            GroundCover groundAtLatitude = groundAtLatitudes[latitude];
            float AlbedoAtLatitude = groundAtLatitude.GetTotalAlbedo();
            float AbsorbsionAtLatitude = 1 - AlbedoAtLatitude;
            totalGlobalAbsorbsion += GetLuminosityMultiplierAtLatitude(latitude) * AbsorbsionAtLatitude / numberOfLatitudes;
        }
        return 1 - totalGlobalAbsorbsion;
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
     * Gets the local temperature of the flowers depending on global temperatue, their albedo, and the latitude of this patch of flowers
     * @param albedo The albedo of the local flowers
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 9 (equitorial)
     * @param globalTemperatue The temperature of the planet before this update
     * @param globalAlbedo The aggregate global albedo of the planet before this update
     * @returns the local temperature over areas with flowers of that albedo
     */
    float GetTemperatureOfAlbedoAtLatitude(float localAlbedo, int latitude, float globalTemperature, float globalAlbedo) {
        // based on equation (7) of Daisyworld, adapted to a planet with multiple latitudes and thus multiple solar luminosities
        float globalAbsorbtivity = 1 - globalAlbedo;
        float localAbsorbtivity = 1 - localAlbedo;
        float scaledLocalAbsorbtivity = localAbsorbtivity * GetLuminosityMultiplierAtLatitude(latitude);
        return conductivityConstant * (scaledLocalAbsorbtivity - globalAbsorbtivity) + globalTemperature;
    }

    /**
     * Gets the average temperature at one latitude on the planet
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 9 (equitorial)
     */
    float GetTemperatureOfLatitude(int latitude) {
        // based on equation (4) of Daisyworld
        float latitudalAbsorbtivity = 1 - groundAtLatitudes[latitude].GetTotalAlbedo();
        float scaledLatitudalAbsorbtivity = latitudalAbsorbtivity * GetLuminosityMultiplierAtLatitude(latitude);
        return std::pow((fluxConstant * solarLuminosity * scaledLatitudalAbsorbtivity) / stefansConstant, 0.25) - celsiusToKelvin;
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
     * @returns the proportion of the world that is covered by white daisies, from 0 to 1. On a round world,
     * averages the white areas of each latitude.
     */
    float GetProportionWhite() {
        if (roundWorld) {
            float totalWhiteProportion = 0.0;
            for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                totalWhiteProportion += groundAtLatitudes[latitude].proportionWhite / numberOfLatitudes;
            }
            return totalWhiteProportion;
        }
        return ground.proportionWhite;
    }

    /**
     * @returns the proportion of the world that is covered by black daisies, from 0 to 1. On a round world,
     * averages the black areas of each latitude.
     */
    float GetProportionBlack() {
        if (roundWorld) {
            float totalBlackProportion = 0.0;
            for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                totalBlackProportion += groundAtLatitudes[latitude].proportionBlack / numberOfLatitudes;
            }
            return totalBlackProportion;
        }
        return ground.proportionBlack;
    }

    /**
     * @returns the proportion of the world that is covered by gray daisies, from 0 to 1. On a round world,
     * averages the gray areas of each latitude.
     */
    float GetProportionGray() {
        if (roundWorld) {
            float totalGrayProportion = 0.0;
            for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                totalGrayProportion += groundAtLatitudes[latitude].proportionGray / numberOfLatitudes;
            }
            return totalGrayProportion;
        }
        return ground.proportionGray;
    }

    /**
     * @returns the proportion of the world that is not covered by daisies, from 0 to 1. On a round world,
     * averages the ground areas of each latitude.
     */
    float GetProportionGround() {
        if (roundWorld) {
            float totalGroundProportion = 0.0;
            for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                totalGroundProportion += groundAtLatitudes[latitude].GetProportionGround() / numberOfLatitudes;
            }
            return totalGroundProportion;
        }
        return ground.GetProportionGround();
    }

    /**
     * On a round world, how much ground is covered by white daisies at this latitude.
     * @param latitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionWhiteAtLatitude(int latitude) {
        int displayBandWidth = numberOfLatitudes / numberOfDisplayedLatitudes;
        int totalDaisiesInBand = 0.0;
        for (int internalLatitude = numberOfLatitudes - displayBandWidth * latitude - displayBandWidth; internalLatitude < numberOfLatitudes - displayBandWidth * latitude; internalLatitude++) {
            totalDaisiesInBand += groundAtLatitudes[internalLatitude].proportionWhite / displayBandWidth;
        }
        return totalDaisiesInBand;
    }

    /**
     * On a round world, how much ground is covered by black daisies at this latitude.
     * @param latitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionBlackAtLatitude(int latitude) {
        int displayBandWidth = numberOfLatitudes / numberOfDisplayedLatitudes;
        int totalDaisiesInBand = 0.0;
        for (int internalLatitude = numberOfLatitudes - displayBandWidth * latitude - displayBandWidth; internalLatitude < numberOfLatitudes - displayBandWidth * latitude; internalLatitude++) {
            totalDaisiesInBand += groundAtLatitudes[internalLatitude].proportionBlack / displayBandWidth;
        }
        return totalDaisiesInBand;
    }

    /**
     * On a round world, how much ground is covered by gray daisies at this latitude.
     * @param latitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionGrayAtLatitude(int latitude) {
        int displayBandWidth = numberOfLatitudes / numberOfDisplayedLatitudes;
        int totalDaisiesInBand = 0.0;
        for (int internalLatitude = numberOfLatitudes - displayBandWidth * latitude - displayBandWidth; internalLatitude < numberOfLatitudes - displayBandWidth * latitude; internalLatitude++) {
            totalDaisiesInBand += groundAtLatitudes[internalLatitude].proportionGray / displayBandWidth;
        }
        return totalDaisiesInBand;
    }

    /**
     * On a round world, how much ground is covered by bare ground (no daisies) at this latitude.
     * @param latitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionGroundAtLatitude(int latitude) {
        int displayBandWidth = numberOfLatitudes / numberOfDisplayedLatitudes;
        int totalGroundInBand = 0.0;
        for (int internalLatitude = numberOfLatitudes - displayBandWidth * latitude - displayBandWidth; internalLatitude < numberOfLatitudes - displayBandWidth * latitude; internalLatitude++) {
            totalGroundInBand += groundAtLatitudes[internalLatitude].GetProportionGround() / displayBandWidth;
        }
        return totalGroundInBand;
    }

    /**
     * Enabled or disables black daisies. If disabled, sets their population to 0
     */
    void SetBlackEnabled(bool _blackEnabled) {
        blackDaisiesEnabled = _blackEnabled;
        if (!_blackEnabled) {
            ground.proportionBlack = 0.0;
            for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                groundAtLatitudes[latitude].proportionBlack = 0.0;
            }
        }
    }

    /**
     * Enabled or disables white daisies. If disabled, sets their population to 0
     */
    void SetWhiteEnabled(bool _whiteEnabled) {
        whiteDaisiesEnabled = _whiteEnabled;
        if (!_whiteEnabled) {
            ground.proportionWhite = 0.0;
            for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                groundAtLatitudes[latitude].proportionWhite = 0.0;
            }
        }
    }

    /**
     * Enabled or disables gray daisies. If disabled, sets their population to 0
     */
    void SetGrayEnabled(bool _grayEnabled) {
        grayDaisiesEnabled = _grayEnabled;
        if (!_grayEnabled) {
            ground.proportionGray = 0.0;
            for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                groundAtLatitudes[latitude].proportionGray = 0.0;
            }
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
     * @returns the rate of change of the proportion of white daisies per unit time on a flat planet
     */
    float WhiteGrowthRate() {
        return GrowthRateOfColor(GetProportionWhite(), GetTemperatureOfAlbedo(whiteAlbedo));
    }

    /**
     * @returns the rate of change of the proportion of black daisies per unit time on a flat planet
     */
    float BlackGrowthRate() {
        return GrowthRateOfColor(GetProportionBlack(), GetTemperatureOfAlbedo(blackAlbedo));
    }

    /**
     * @returns the rate of change of the proportion of gray daisies per unit time on a flat planet
     */
    float GrayGrowthRate() {
        return GrowthRateOfColor(GetProportionGray(), GetTemperatureOfAlbedo(grayAlbedo));
    }

    /**
     * Growth rate function for daisies of a certain color of amount of daisies of that color and local temperature of those daisies
     */
    float GrowthRateOfColor(float proportionOfColor, float localTemperature) {
        // equation (1) from Daisyworld paper
        return proportionOfColor * (GrowthRateFunction(localTemperature) * GetProportionGround() - deathRate);
    }

    /**
     * Calculates the rate of change of the proportion of white daisies per unit time at a specific latitude of a round planet
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 9 (equitorial)
     * @param globalTemperature The global temperature of the planet before this update
     * @param globalAlbedo The aggregate albedo of the planet before this update
     * @returns the growth rate of white daisies per unit time
     */
    float WhiteGrowthRateAtLatitude(int latitude, float globalTemperature, float globalAlbedo) {
        return GrowthRateOfColorAtLatitude(groundAtLatitudes[latitude].proportionWhite, GetTemperatureOfAlbedoAtLatitude(whiteAlbedo, latitude, globalTemperature, globalAlbedo), latitude);
    }

    /**
     * Calculates the rate of change of the proportion of black daisies per unit time at a specific latitude of a round planet
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 9 (equitorial)
     * @param globalTemperature The global temperature of the planet before this update
     * @param globalAlbedo The aggregate albedo of the planet before this update
     * @returns the growth rate of black daisies per unit time
     */
    float BlackGrowthRateAtLatitude(int latitude, float globalTemperature, float globalAlbedo) {
        return GrowthRateOfColorAtLatitude(groundAtLatitudes[latitude].proportionBlack, GetTemperatureOfAlbedoAtLatitude(blackAlbedo, latitude, globalTemperature, globalAlbedo), latitude);
    }

    /**
     * Calculates the rate of change of the proportion of gray daisies per unit time at a specific latitude of a round planet
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 9 (equitorial)
     * @param globalTemperature The global temperature of the planet before this update
     * @param globalAlbedo The aggregate albedo of the planet before this update
     * @returns the growth rate of gray daisies per unit time
     */
    float GrayGrowthRateAtLatitude(int latitude, float globalTemperature, float globalAlbedo) {
        return GrowthRateOfColorAtLatitude(groundAtLatitudes[latitude].proportionGray, GetTemperatureOfAlbedoAtLatitude(grayAlbedo, latitude, globalTemperature, globalAlbedo), latitude);
    }

    /**
     * Growth rate function for daisies of a certain color on round planet of amount of daisies of that color and local temperature of those daisies
     */
    float GrowthRateOfColorAtLatitude(float proportionOfColor, float localTemperature, int latitude) {
        // equation (1) from Daisyworld paper
        return proportionOfColor * (GrowthRateFunction(localTemperature) * groundAtLatitudes[latitude].GetProportionGround() - deathRate);
    }

    /**
     * Performs one time step, allowing the daisies to grow and die according to temperature as long as growth and
     * death are not disabled
     */
    void Update() {
        emp::World<float>::Update();
        if (daisiesCanGrowAndDie) {
            if (roundWorld) {
                float globalTemperature = GetGlobalTemperature();
                float globalAlbedo = GetTotalAlbedo();
                for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                    UpdateDaisyAmountsAtLatitude(latitude, globalTemperature, globalAlbedo);
                }
            } else {
                UpdateDaisyAmountsOnFlatPlanet();
            }
        }
    }

    /**
     * Does one time step, letting daisies grow and die according to the local temperature
     */
    void UpdateDaisyAmountsOnFlatPlanet() {
        // the amount that each type of daisy grows this update
        float whiteGrowthAmount = WhiteGrowthRate() * timePerUpdate;
        float blackGrowthAmount = BlackGrowthRate() * timePerUpdate;
        float grayGrowthAmount = GrayGrowthRate() * timePerUpdate;
        // update the amounts of each type of daisy if they are enabled
        if (whiteDaisiesEnabled) ground.IncrementWhiteAmount(whiteGrowthAmount);
        if (blackDaisiesEnabled) ground.IncrementBlackAmount(blackGrowthAmount);
        if (grayDaisiesEnabled) ground.IncrementGrayAmount(grayGrowthAmount);
    }

    /**
     * Does one time step on this latitude of the planet, letting daisies grow and die according to local temperature
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 9 (equitorial)
     * @param globalTemperature The global temperature of the planet before this update
     * @param globalAlbedo The aggregate albedo of the planet before this update
     */
    void UpdateDaisyAmountsAtLatitude(int latitude, float globalTemperature, float globalAlbedo) {
        // the amount that each type of daisy grows this update
        float whiteGrowthAmount = WhiteGrowthRateAtLatitude(latitude, globalTemperature, globalAlbedo) * timePerUpdate;
        float blackGrowthAmount = BlackGrowthRateAtLatitude(latitude, globalTemperature, globalAlbedo) * timePerUpdate;
        float grayGrowthAmount = GrayGrowthRateAtLatitude(latitude, globalTemperature, globalAlbedo) * timePerUpdate;
        // update the amounts of each type of daisy if they are enabled
        if (whiteDaisiesEnabled) groundAtLatitudes[latitude].IncrementWhiteAmount(whiteGrowthAmount);
        if (blackDaisiesEnabled) groundAtLatitudes[latitude].IncrementBlackAmount(blackGrowthAmount);
        if (grayDaisiesEnabled) groundAtLatitudes[latitude].IncrementGrayAmount(grayGrowthAmount);
    }

    /**
     * @returns The average latitude of the habitat of white daisies
     */
    float GetAverageLatitudeOfWhite() {    
        if (GetProportionWhite() < 0.0001) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        float totalLatitudeProportion = 0.0;
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            totalLatitudeProportion += latitude * groundAtLatitudes[latitude].proportionWhite;
        }
        return totalLatitudeProportion / GetProportionWhite() / numberOfLatitudes;
    }

    /**
     * @returns The average latitude of the habitat of black daisies
     */
    float GetAverageLatitudeOfBlack() {    
        if (GetProportionBlack() < 0.0001) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        float totalLatitudeProportion = 0.0;
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            totalLatitudeProportion += latitude * groundAtLatitudes[latitude].proportionBlack;
        }
        return totalLatitudeProportion / GetProportionBlack() / numberOfLatitudes;
    }

    /**
     * @returns The average latitude of the habitat of gray daisies
     */
    float GetAverageLatitudeOfGray() {    
        if (GetProportionGray() < 0.0001) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        float totalLatitudeProportion = 0.0;
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            totalLatitudeProportion += latitude * groundAtLatitudes[latitude].proportionGray;
        }
        return totalLatitudeProportion / GetProportionGray() / numberOfLatitudes;
    }

    /**
     * The maximum latitude at which white daisies exist on a round planet
     * @returns -1 if no white daisies exist
     */
    int GetMaxLatitudeOfWhite() {
        for (int latitude = numberOfLatitudes - 1; latitude >= 0; latitude--) {
            if (groundAtLatitudes[latitude].proportionWhite > 0.0) {
                return latitude;
            }
        }
        return -1;
    }

    /**
     * The maximum latitude at which black daisies exist on a round planet
     * @returns -1 if no black daisies exist
     */
    int GetMaxLatitudeOfBlack() {
        for (int latitude = numberOfLatitudes - 1; latitude >= 0; latitude--) {
            if (groundAtLatitudes[latitude].proportionBlack > 0.0) {
                return latitude;
            }
        }
        return -1;
    }

    /**
     * The maximum latitude at which gray daisies exist on a round planet
     * @returns -1 if no gray daisies exist
     */
    int GetMaxLatitudeOfGray() {
        for (int latitude = numberOfLatitudes - 1; latitude >= 0; latitude--) {
            if (groundAtLatitudes[latitude].proportionGray > 0.0) {
                return latitude;
            }
        }
        return -1;
    }

    /**
     * The minimum latitude at which white daisies exist on a round planet
     * @returns numberOfLatitudes if no white daisies exist
     */
    int GetMinLatitudeOfWhite() {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            if (groundAtLatitudes[latitude].proportionWhite > 0.0) {
                return latitude;
            }
        }
        return numberOfLatitudes;
    }

    /**
     * The minimum latitude at which black daisies exist on a round planet
     * @returns numberOfLatitudes if no black daisies exist
     */
    int GetMinLatitudeOfBlack() {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            if (groundAtLatitudes[latitude].proportionBlack > 0.0) {
                return latitude;
            }
        }
        return numberOfLatitudes;
    }

    /**
     * The minimum latitude at which gray daisies exist on a round planet
     * @returns numberOfLatitudes if no gray daisies exist
     */
    int GetMinLatitudeOfGray() {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            if (groundAtLatitudes[latitude].proportionGray > 0.0) {
                return latitude;
            }
        }
        return numberOfLatitudes;
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
        // on a round world, add the average latitudes of each type of daisy
        if (roundWorld) {
            file.AddFun<std::string>([this]() { float min = GetMinLatitudeOfWhite(); return min == numberOfLatitudes ? "" : std::to_string(min); }, "min_lat_w", "Minimum latitude of white daisies");
            file.AddFun<std::string>([this]() { float mean = GetAverageLatitudeOfWhite(); return std::isnan(mean) ? "" : std::to_string(mean); }, "mean_lat_w", "Average latitude of white daisies");
            file.AddFun<std::string>([this]() { float max = GetMaxLatitudeOfWhite(); return max < 0 ? "" : std::to_string(max); }, "max_lat_w", "Minimum latitude of white daisies");
            file.AddFun<std::string>([this]() { float min = GetMinLatitudeOfBlack(); return min == numberOfLatitudes ? "" : std::to_string(min); }, "min_lat_b", "Minimum latitude of black daisies");
            file.AddFun<std::string>([this]() { float mean = GetAverageLatitudeOfBlack(); return std::isnan(mean) ? "" : std::to_string(mean); }, "mean_lat_b", "Average latitude of black daisies");
            file.AddFun<std::string>([this]() { float max = GetMaxLatitudeOfBlack(); return max < 0 ? "" : std::to_string(max); }, "max_lat_b", "Minimum latitude of black daisies");
            if (grayDaisiesEnabled) {
                file.AddFun<std::string>([this]() { float min = GetMinLatitudeOfGray(); return min == numberOfLatitudes ? "" : std::to_string(min); }, "min_lat_g", "Minimum latitude of gray daisies");
                file.AddFun<std::string>([this]() { float mean = GetAverageLatitudeOfGray(); return std::isnan(mean) ? "" : std::to_string(mean); }, "mean_lat_g", "Average latitude of gray daisies");
                file.AddFun<std::string>([this]() { float max = GetMaxLatitudeOfGray(); return max < 0 ? "" : std::to_string(max); }, "max_lat_g", "Minimum latitude of gray daisies");
            }
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
     * @param The minimum amounts of each type of daisy
     */
    void BoostDaisiesIfExtinct(float whiteBoost = 0.01, float blackBoost = 0.01, float grayBoost = 0.01) {
        if (roundWorld) {
            BoostDaisiesIfExtinctOnRoundWorld();
            return;
        }
        if (whiteDaisiesEnabled && GetProportionWhite() < whiteBoost) {
            ground.proportionWhite = whiteBoost;
        }
        if (blackDaisiesEnabled && GetProportionBlack() < blackBoost) {
            ground.proportionBlack = blackBoost;
        }
        if (grayDaisiesEnabled && GetProportionGray() < grayBoost) {
            ground.proportionGray = grayBoost;
        }
    }

    /**
     * If the black/white/gray daisies have gone extinct, set their proportion at each latitude to some small value so they
     * may get started again.
     * @param The minimum amounts of each type of daisy
     */
    void BoostDaisiesIfExtinctOnRoundWorld(float whiteBoost = 0.001, float blackBoost = 0.001, float grayBoost = 0.001) {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            if (whiteDaisiesEnabled && groundAtLatitudes[latitude].proportionWhite < whiteBoost) {
                groundAtLatitudes[latitude].proportionWhite = whiteBoost;
            }
            if (blackDaisiesEnabled && groundAtLatitudes[latitude].proportionBlack < blackBoost) {
                groundAtLatitudes[latitude].proportionBlack = blackBoost;
            }
            if (grayDaisiesEnabled && groundAtLatitudes[latitude].proportionGray < grayBoost) {
                groundAtLatitudes[latitude].proportionGray = grayBoost;
            }
        }
    }
};

#endif
