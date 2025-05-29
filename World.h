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
         * proportion[0] = white, proportion[1] = black, proportion[2] = gray
         */
        float proportion[3];

        GroundCover(float _proportionWhite = 0.33, float _proportionBlack = 0.33, float _proportionGray = 0.0) {
            proportion[WHITE] = _proportionWhite;
            proportion[BLACK] = _proportionBlack;
            proportion[GRAY] = _proportionGray;
        }

        /**
         * @returns the proportion of the planet that is not covered by daisies
         */
        float GetProportionGround() {
            // equation (2) of Daisyworld paper
            float total = 1.0;
            for (int i=0; i<COLORS; i++) {
                total -= proportion[i];
            }
            return total;
        }

        /**
         * Gets the proportion of the number of daisies of this existent color, otherwise gets bare ground coverage
         */
        float Proportion(int color) {
            return (color < 0 || color >= COLORS) ? GetProportionGround() : proportion[color];
        }

        /**
         * Increments the color by delta, keeping it clamped below at 0
         */
        void IncrementColor(int color, float delta) {
            proportion[color] += delta;
            // clamp values below at 0, don't allow tiny amounts of daisies
            if (proportion[color] < 0.001) proportion[color] = 0.0;
        }

        /**
         * @returns a weighted average of the albedos of the different types of flowers
         */
        float GetTotalAlbedo() {
            float total = GetProportionGround() * groundAlbedo;
            for (int i=0; i<COLORS; i++) {
                total += proportion[i] * flowerAlbedos[i];
            }
            return total;
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

    // whether each type of daisy is allowed to exist
    bool enabledColors[3] = {true, true, false};

    // whether daisies can grow or die
    bool daisiesCanGrowAndDie = true;

    emp::DataMonitor<float>* temperatureMonitor;

    // the global temperature and albedo, cached until the proportion of daisies or luminosity changes
    // if no applicable value, set to nan
    float cachedGlobalTemperature = std::numeric_limits<float>::quiet_NaN();
    float cachedGlobalAlbedo = std::numeric_limits<float>::quiet_NaN();

    // the albedos of the different colored flowers
    static constexpr float flowerAlbedos[3] = {0.75, 0.25, 0.5};
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

    public:

    /**
     * When variables and functions take color index, white is 0
     */
    static constexpr int WHITE = 0;

    /**
     * When variables and functions take color index, black is 1
     */
    static constexpr int BLACK = 1;

    /**
     * When variables and functions take color index, gray is 2
     */
    static constexpr int GRAY = 2;

    /**
     * The number of different colored daisies that the simulation can run
     */
    static constexpr int COLORS = 3;

    /**
     * Initializes a starting solar luminosity and flower populations.
     * @param _roundWorld Whether to compute different temperatures at different latitudes of the planet
     */
    World(float _proportionWhite, float _proportionBlack, float _solarLuminosity, float _proportionGray = 0.0f, bool _roundWorld = false)
        : ground(_proportionWhite, _proportionBlack, _proportionGray), solarLuminosity(_solarLuminosity), roundWorld(_roundWorld) {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            groundAtLatitudes[latitude] = GroundCover(_proportionWhite, _proportionBlack, _proportionGray);
        }
        daisiesCanGrowAndDie = true;
        update = 0;
    }

    private:

    /**
     * The proportion of ground covered by the different daisies at different latitudes.
     */
    GroundCover groundAtLatitudes[numberOfLatitudes] = {};

    // how luminosity changes over different latitudes on a round planet
    const float minLuminosityMultiplier = 0.6;
    const float maxLuminosityMultiplier = 1.5;

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
     * Gets the amount of either a color of daisy or bare ground, either over the entire world or at a specific latitude
     * @param color The color of daisy, or -1 to choose ground
     * @param aggregateLatitude -1 if getting the proportion over entire world. Otherwise, the average number of this color
     * in this band of latitudes.
     */
    float Proportion(int color, int aggregateLatitude) {
        if (roundWorld) {
            float totalProportion = 0.0;
            if (aggregateLatitude < 0) {
                // aggregate over entire planet
                for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                    totalProportion += groundAtLatitudes[latitude].Proportion(color) / numberOfLatitudes;
                }
            } else {
                // aggregate over a certain band of latitudes of the planet
                int displayBandWidth = numberOfLatitudes / numberOfDisplayedLatitudes;
                for (int internalLatitude = numberOfLatitudes - displayBandWidth * aggregateLatitude - displayBandWidth; internalLatitude < numberOfLatitudes - displayBandWidth * aggregateLatitude; internalLatitude++) {
                    totalProportion += groundAtLatitudes[internalLatitude].Proportion(color) / displayBandWidth;
                }
            }
            return totalProportion;
            
        }
        return color < 0 ? ground.GetProportionGround() : ground.proportion[color];
    }

    /**
     * Enables or disables the specified color of daisies. Disabled colors cannot grow and are kept at
     * 0 proportion
     */
    void SetColorEnabled(int color, bool enabled) {
        enabledColors[color] = enabled;
        if (!enabled) {
            ground.proportion[color] = 0.0;
            if (roundWorld) {
                for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                    groundAtLatitudes[latitude].proportion[color] = 0.0;
                }
            }
            ClearCachedValues();
        }
    }

    /**
     * @param localTemperature The local temperature over this type of flower
     * @returns the growth rate per unit time on bare ground of this type of daisy
     */
    float GrowthRateFunction(float localTemperature) {
        // equation (3) from Daisyworld paper
        return 1 - 0.003265 * (22.5 - localTemperature) * (22.5 - localTemperature);
    }

    /**
     * Calculates the rate of change of amount of daisies of a color on a flat planet.
     * @param color The color of these daisies
     */
    float GrowthRate(int color) {
        // equation (1) from Daisyworld paper
        float proportionOfColor = ground.proportion[color];
        float localTemperature = LocalTemperature(color);
        return proportionOfColor * (GrowthRateFunction(localTemperature) * GetProportionGround() - deathRate);
    }

    /**
     * Calculates the rate of change of the proportion of a color of daisies per unit time at a certain latitude on a round planet
     * @param color The color of these daisies
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 99 (equitorial)
     * @returns the growth rate of daisies of this color per unit time
     */
    float GrowthRateAtLatitude(int color, int latitude) {
        // equation (1) from Daisyworld paper
        float proportionOfColor = groundAtLatitudes[latitude].proportion[color];
        float localTemperature = LocalTemperatureAtLatitude(color, latitude);
        return proportionOfColor * (GrowthRateFunction(localTemperature) * groundAtLatitudes[latitude].GetProportionGround() - deathRate);
    }

    /**
     * Gets the local temperature of the flowers of a color
     * @param color The color of the flowers
     * @returns the local temperature over areas with flowers of that color, based on global temperature
     */
    float LocalTemperature(int color) {
        // equation (7) of Daisyworld
        float localAlbedo = flowerAlbedos[color];
        return conductivityConstant * (GetTotalAlbedo() - localAlbedo) + GetGlobalTemperature();
    }

    /**
     * Calculates the local temperature of the flowers depending on global temperatue, their albedo, and the latitude of this patch of flowers
     * @param color The color of the local flowers
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 99 (equitorial)
     * @param latitudinalConduction Of the temperature influence conducting from elsewhere on the planet, what proportion comes
     * from the latitudinal temperature?
     * @returns the local temperature over areas with flowers of that color
     */
    float LocalTemperatureAtLatitude(int color, int latitude, float latitudinalConduction = 0.0) {
        // based on equation (7) of Daisyworld, adapted to a planet with multiple latitudes and thus multiple solar luminosities
        float globalAlbedo = GetTotalAlbedo();
        float globalTemperature = GetGlobalTemperature();
        float globalAbsorbtivity = 1 - globalAlbedo;
        float localAlbedo = flowerAlbedos[color];
        float localAbsorbtivity = 1 - localAlbedo;
        float scaledLocalAbsorbtivity = localAbsorbtivity * GetLuminosityMultiplierAtLatitude(latitude);
        float conductingTemperature = latitudinalConduction == 0.0 ? globalTemperature : latitudinalConduction * TemperatureOfLatitude(latitude) + (1 - latitudinalConduction) * globalTemperature;
        return conductivityConstant * (scaledLocalAbsorbtivity - globalAbsorbtivity) + conductingTemperature;
    }

    /**
     * Resets the cached values of global temperature and global albedo when the luminosity or proportions change
     */
    void ClearCachedValues() {
        cachedGlobalTemperature = std::numeric_limits<float>::quiet_NaN();
        cachedGlobalAlbedo = std::numeric_limits<float>::quiet_NaN();
    }

    /**
     * Does one time step, letting daisies grow and die according to the local temperature
     */
    void UpdateDaisyAmountsOnFlatPlanet() {
        // the amount that each type of daisy grows this update
        float growthAmounts[COLORS];
        for (int i=0; i<COLORS; i++) {
            growthAmounts[i] = GrowthRate(i) * timePerUpdate;
        }
        // update the amounts of each type of daisy if they are enabled
        for (int i=0; i<COLORS; i++) {
            if (enabledColors[i]) ground.IncrementColor(i, growthAmounts[i]);
        }
        ClearCachedValues();
    }

    /**
     * Does one time step on a round planet, letting daisies grow and die according to their local temperature
     */
    void UpdateDaisyAmountsOnRoundPlanet() {
        float growthAmounts[COLORS][numberOfLatitudes];
        CalculateGrowthAmountsOnRoundPlanet(growthAmounts);
        DoDaisyGrowthOnRoundPlanet(growthAmounts);
        ClearCachedValues();
    }

    /**
     * stores the amount that each type of daisy grows at this latitude into a growth array
     */
    void CalculateGrowthAmountsOnRoundPlanet(float (&growthAmounts)[COLORS][numberOfLatitudes]) {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            for (int i=0; i<COLORS; i++) {
                if (enabledColors[i]) growthAmounts[i][latitude] = GrowthRateAtLatitude(i, latitude) * timePerUpdate;
            }
        }
    }

    /**
     * Given an array of how much each type of daisy should grow or die this update at this latitude, increments
     * or decrements the daisy amounts
     */
    void DoDaisyGrowthOnRoundPlanet(float (&growthAmounts)[COLORS][numberOfLatitudes]) {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            for (int i=0; i<COLORS; i++) {
                if (enabledColors[i]) groundAtLatitudes[latitude].IncrementColor(i, growthAmounts[i][latitude]);
            }
        }
    }

    /**
     * Gets the average latitude of the habitat of this color of daisy
     * @param color The color of daisy
     */
    float AverageLatitude(int color) {
        float totalLatitudeProportion = 0.0;
        float totalProportion = 0.0;
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            totalProportion += groundAtLatitudes[latitude].proportion[color];
            totalLatitudeProportion += latitude * groundAtLatitudes[latitude].proportion[color];
        }
        if (totalProportion < 0.0001) {
            // there aren't enough daisies of this color to get a meaningful average
            return std::numeric_limits<float>::quiet_NaN();
        }
        return totalLatitudeProportion / totalProportion;
    }

    /**
     * The maximum latitude (most equatorial) at which daisies of this color exist
     * @param color The color of daisy
     * @returns The maximal latitude (most equatorial) of that habitat, or -1 if no daisies of this color exist
     */
    int MaxLatitude(int color) {
        for (int latitude = numberOfLatitudes - 1; latitude >= 0; latitude--) {
            if (groundAtLatitudes[latitude].proportion[color] > 0.0) {
                return latitude;
            }
        }
        return -1;
    }

    /**
     * The minimum latitude (most polar) at which daisies of this color exist
     * @param color The color of daisy
     * @returns The minimum latitude (most polar) of that habitat, or numberOfLatitudes if no daisies of this color exist
     */
    int MinLatitude(int color) {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            if (groundAtLatitudes[latitude].proportion[color] > 0.0) {
                return latitude;
            }
        }
        return numberOfLatitudes;
    }

    /**
     * If the black/white/gray daisies have gone extinct, set their proportion at each latitude to some small value so they
     * may get started again.
     * @param The minimum amounts of each type of daisy
     */
    void BoostDaisiesIfExtinctOnRoundWorld(float whiteBoost = 0.001, float blackBoost = 0.001, float grayBoost = 0.001) {
        for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
            if (enabledColors[WHITE] && groundAtLatitudes[latitude].proportion[WHITE] < whiteBoost) groundAtLatitudes[latitude].proportion[WHITE] = whiteBoost;
            if (enabledColors[BLACK] && groundAtLatitudes[latitude].proportion[BLACK] < blackBoost) groundAtLatitudes[latitude].proportion[BLACK] = blackBoost;
            if (enabledColors[GRAY] && groundAtLatitudes[latitude].proportion[GRAY] < grayBoost) groundAtLatitudes[latitude].proportion[GRAY] = grayBoost;
        }
    }

    /**
     * Adds proportions for each type of daisy to a data file
     */
    void AddDaisyProportionsToDataFile(emp::DataFile& file) {
        file.AddFun<float>([this]() { return Proportion(WHITE, -1); }, "a_w", "Proportion of white daisies");
        file.AddFun<float>([this]() { return Proportion(BLACK, -1); }, "a_b", "Proportion of black daisies");
        if (enabledColors[GRAY]) {
            file.AddFun<float>([this]() { return Proportion(GRAY, -1); }, "a_g", "Proportion of gray daisies");
        }
    }

    /**
     * Adds statistics for the min, mean, and max latitudes of each type of daisy to a Empirical data file
     */
    void AddLatitudeStatisticsToDataFile(emp::DataFile& file) {
        file.AddFun<std::string>([this]() { return FilterLatitudeData(MinLatitude(WHITE)); }, "min_lat_w", "Minimum latitude of white daisies");
        file.AddFun<std::string>([this]() { return FilterLatitudeData(AverageLatitude(WHITE)); }, "mean_lat_w", "Average latitude of white daisies");
        file.AddFun<std::string>([this]() { return FilterLatitudeData(MaxLatitude(WHITE)); }, "max_lat_w", "Minimum latitude of white daisies");
        file.AddFun<std::string>([this]() { return FilterLatitudeData(MinLatitude(BLACK)); }, "min_lat_b", "Minimum latitude of black daisies");
        file.AddFun<std::string>([this]() { return FilterLatitudeData(AverageLatitude(BLACK)); }, "mean_lat_b", "Average latitude of black daisies");
        file.AddFun<std::string>([this]() { return FilterLatitudeData(MaxLatitude(BLACK)); }, "max_lat_b", "Minimum latitude of black daisies");
        if (enabledColors[GRAY]) {
            file.AddFun<std::string>([this]() { return FilterLatitudeData(MinLatitude(GRAY)); }, "min_lat_g", "Minimum latitude of gray daisies");
            file.AddFun<std::string>([this]() { return FilterLatitudeData(AverageLatitude(GRAY)); }, "mean_lat_g", "Average latitude of gray daisies");
            file.AddFun<std::string>([this]() { return FilterLatitudeData(MaxLatitude(GRAY)); }, "max_lat_g", "Minimum latitude of gray daisies");
        }
    }

    /**
     * Converts a float latitude statistic into a string, filtering out data that doesn't fall in the possible latitude ranges
     * @param latitude A latitude statistic coming from a function
     * @return A string to be inserted into a data table
     */
    static std::string FilterLatitudeData(float latitude) {
        if (std::isnan(latitude)) return "";
        return latitude < 0 || latitude > numberOfLatitudes - 1 ? "" : std::to_string(latitude);
    }

    /**
     * Converts a int latitude statistic into a string, filtering out data that doesn't fall in the possible latitude ranges
     * @param latitude A latitude statistic coming from a function
     * @return A string to be inserted into a data table
     */
    static std::string FilterLatitudeData(int latitude) {
        return latitude < 0 || latitude > numberOfLatitudes - 1 ? "" : std::to_string(latitude);
    }
    
    public:

    /**
     * @returns the averaged total albedo over the entire planet (how much sunlight is reflected in aggregate). If the world is round
     */
    float GetTotalAlbedo() {
        if (std::isnan(cachedGlobalAlbedo)) {
            cachedGlobalAlbedo = roundWorld ? GetAverageAlbedoOnRoundPlanet() : ground.GetTotalAlbedo();
        }
        return cachedGlobalAlbedo;
    }
    
    /**
     * @returns the average global temperature of the planet in Celsius, based on average albedo and solar luminosity
     */
    float GetGlobalTemperature() {
        if (std::isnan(cachedGlobalTemperature)) {
            float globalAlbedo = GetTotalAlbedo();
            float globalAbsorbsion = 1 - globalAlbedo;
            // calculate the global temperature using the Stefan-Boltzman equation
            // equation (4) of Daisyworld
            cachedGlobalTemperature = std::pow((fluxConstant * solarLuminosity * globalAbsorbsion) / stefansConstant, 0.25) - celsiusToKelvin;
        }
        return cachedGlobalTemperature;
    }

    /**
     * Gets the average temperature at one latitude on the planet
     * @param latitude The latitude on the planet, ranging from 0 (polar) to 9 (equitorial)
     */
    float TemperatureOfLatitude(int latitude) {
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
        ClearCachedValues();
    }

    /**
     * @returns the dimensionless solar luminosity, with values typically around 1
     */
    float GetSolarLuminosity() {
        return solarLuminosity;
    }
  
    /** 
     * Sets whether the world is round (has different latitudes) or not. When changing world types, moves the current daisy proportions over.
     */
    void SetRoundWorld(bool _roundWorld) {
        if (roundWorld != _roundWorld) {
            // we are changing the roundness
            if (_roundWorld) {
                // going from flat to round world, distribute flowers homogeneously
                for (int latitude = 0; latitude < numberOfLatitudes; latitude++) {
                    for (int color = 0; color < COLORS; color++) {
                        groundAtLatitudes[latitude].proportion[color] = ground.proportion[color];
                    }
                }
            } else {
                // going from round to flat world, aggregate values from all latitudes
                for (int color = 0; color < COLORS; color++) {
                    ground.proportion[color] = Proportion(color, -1);
                }
            }
            roundWorld = _roundWorld;
        }
    }

    /**
     * @returns Whether the world is round
     */
    bool IsWorldRound() {
        return roundWorld;
    }

    /**
     * @returns the proportion of the world that is covered by white daisies, from 0 to 1. On a round world,
     * averages the white areas of each latitude.
     */
    float GetProportionWhite() {
        return Proportion(WHITE, -1);
    }

    /**
     * @returns the proportion of the world that is covered by black daisies, from 0 to 1. On a round world,
     * averages the black areas of each latitude.
     */
    float GetProportionBlack() {
        return Proportion(BLACK, -1);
    }

    /**
     * @returns the proportion of the world that is covered by gray daisies, from 0 to 1. On a round world,
     * averages the gray areas of each latitude.
     */
    float GetProportionGray() {
        return Proportion(GRAY, -1);
    }

    /**
     * @returns the proportion of the world that is not covered by daisies, from 0 to 1. On a round world,
     * averages the ground areas of each latitude.
     */
    float GetProportionGround() {
        return Proportion(-1, -1);
    }

    /**
     * On a round world, how much ground is covered by white daisies at this latitude.
     * @param displayLatitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionWhiteAtLatitude(int displayLatitude) {
        return Proportion(WHITE, displayLatitude);
    }

    /**
     * On a round world, how much ground is covered by black daisies at this latitude.
     * @param displayLatitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionBlackAtLatitude(int displayLatitude) {
        return Proportion(BLACK, displayLatitude);
    }

    /**
     * On a round world, how much ground is covered by gray daisies at this latitude.
     * @param displayLatitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionGrayAtLatitude(int displayLatitude) {
        return Proportion(GRAY, displayLatitude);
    }

    /**
     * On a round world, how much ground is covered by bare ground (no daisies) at this latitude.
     * @param displayLatitude The displayed latitude of the planet, which may differ from the internal subdivision.
     * By default, there are 10 latitude classes, from 0 (equatorial) to 9 (polar)
     */
    float GetProportionGroundAtLatitude(int displayLatitude) {
        return Proportion(-1, displayLatitude);
    }

    /**
     * Enabled or disables white daisies. If disabled, sets their population to 0
     */
    void SetWhiteEnabled(bool _whiteEnabled) {
        SetColorEnabled(WHITE, _whiteEnabled);
    }

    /**
     * Enabled or disables black daisies. If disabled, sets their population to 0
     */
    void SetBlackEnabled(bool _blackEnabled) {
        SetColorEnabled(BLACK, _blackEnabled);
    }

    /**
     * Enabled or disables gray daisies. If disabled, sets their population to 0
     */
    void SetGrayEnabled(bool _grayEnabled) {
        SetColorEnabled(GRAY, _grayEnabled);
    }

    /**
     * Enables or disables changes in the amounts of daisies
     */
    void SetDaisyGrowthAndDeath(bool _daisiesCanGrowAndDie) {
        daisiesCanGrowAndDie = _daisiesCanGrowAndDie;
    }

    /**
     * Performs one time step, allowing the daisies to grow and die according to temperature as long as growth and
     * death are not disabled
     */
    void Update() {
        emp::World<float>::Update();
        if (daisiesCanGrowAndDie) {
            if (roundWorld) {
                UpdateDaisyAmountsOnRoundPlanet();
            } else {
                UpdateDaisyAmountsOnFlatPlanet();
            }
        }
    }

    /**
     * @returns The average latitude of the habitat of white daisies
     */
    float AverageLatitudeOfWhite() {    
        return AverageLatitude(WHITE);
    }

    /**
     * @returns The average latitude of the habitat of black daisies
     */
    float AverageLatitudeOfBlack() {    
        return AverageLatitude(BLACK);
    }

    /**
     * @returns The average latitude of the habitat of gray daisies
     */
    float AverageLatitudeOfGray() {    
        return AverageLatitude(GRAY);
    }

    /**
     * The maximum latitude at which white daisies exist on a round planet
     * @returns -1 if no white daisies exist
     */
    int MaxLatitudeOfWhite() {
        return MaxLatitude(WHITE);
    }

    /**
     * The maximum latitude at which black daisies exist on a round planet
     * @returns -1 if no black daisies exist
     */
    int MaxLatitudeOfBlack() {
        return MaxLatitude(BLACK);
    }

    /**
     * The maximum latitude at which gray daisies exist on a round planet
     * @returns -1 if no gray daisies exist
     */
    int MaxLatitudeOfGray() {
        return MaxLatitude(GRAY);
    }

    /**
     * The minimum latitude at which white daisies exist on a round planet
     * @returns numberOfLatitudes if no white daisies exist
     */
    int MinLatitudeOfWhite() {
        return MinLatitude(WHITE);
    }

    /**
     * The minimum latitude at which black daisies exist on a round planet
     * @returns numberOfLatitudes if no black daisies exist
     */
    int MinLatitudeOfBlack() {
        return MinLatitude(BLACK);
    }

    /**
     * The minimum latitude at which gray daisies exist on a round planet
     * @returns numberOfLatitudes if no gray daisies exist
     */
    int MinLatitudeOfGray() {
        return MinLatitude(GRAY);
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
        AddDaisyProportionsToDataFile(file);
        // on a round world, add the average latitudes of each type of daisy
        if (roundWorld) {
            AddLatitudeStatisticsToDataFile(file);
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
        ClearCachedValues();
        if (roundWorld) {
            BoostDaisiesIfExtinctOnRoundWorld();
            return;
        }
        if (enabledColors[WHITE] && GetProportionWhite() < whiteBoost) ground.proportion[WHITE] = whiteBoost;
        if (enabledColors[BLACK] && GetProportionBlack() < blackBoost) ground.proportion[BLACK] = blackBoost;
        if (enabledColors[GRAY] && GetProportionGray() < grayBoost) ground.proportion[GRAY] = grayBoost;
    }
};

#endif
