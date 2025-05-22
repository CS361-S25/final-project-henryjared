#include "World.h"

/**
 * Test whether the world correctly calculates its global temperature based on the proportion of daisies
 */
void TestTemperatureCalculations() {
    // create a world with a 50-50 mix of black and white daisies
    World world(0.5, 0.5, 1);
    world.SetDaisyGrowthAndDeath(false);

    // expected output: 0.5
    std::cout << "Global Albedo: " << world.GetTotalAlbedo() << std::endl;
    // expected output: about 26
    std::cout << "Global Temperature: " << world.GetGlobalTemperature() << std::endl;

    // expected output: around 31
    std::cout << "Temperature of Black Daisies: " << world.GetTemperatureOfBlackFlowers() << std::endl;
    
    // expected output: around 21
    std::cout << "Temperature of White Daisies: " << world.GetTemperatureOfWhiteFlowers() << std::endl;
}

/**
 * Test when the sun has constant luminosity, there are only black daisies, and they are allowed to grow and die
 * Writes to output file black.csv
 */
void TestConstantLuminosityOnlyBlack() {
    // perform a test where there are only black daisies and they grow and die over time
    // starting amounts: black = 0.5. Solar luminosity = 1
    // EXPECTED OUTPUT (based on Daisyworld paper graph (b)) stabilizing around a_b = 0.15, T_e = 35
    World world(0, 0.5, 1);
    world.SetWhiteEnabled(false);

    // output data every 1 time unit
    world.SetupDataFile("constant_luminosity_black.csv").SetTimingRepeat(world.GetUpdatesPerTimeUnit());

    // update the world for 100 time units
    for (int i=0; i<world.GetUpdatesPerTimeUnit() * 100 + 1; i++) {
        world.Update();
    }

    std::cout << "Black test completed. Temperature = " << std::to_string(world.GetGlobalTemperature()) << "; black daisy proportion = " << std::to_string(world.GetProportionBlack()) << std::endl;
}

/**
 * Test when the sun has constant luminosity, there are only black daisies, and they are allowed to grow and die
 * Writes to output file black_and_white.csv
 */
void TestConstantLuminosityBlackAndWhite() {
    // perform another test, where daisies may grow and die over time and there are both black and white daisies
    // starting amounts = white 0.5, black 0.5. Solar luminosity = 1
    // EXPECTED OUTPUT (based on Daisyworld paper graph (d)) stabilizing around a_b = 0.3, a_w = 0.4, T_e = 22
    World world(0.5, 0.5, 1);

    // output data every 1 time unit
    world.SetupDataFile("constant_luminosity_black_and_white.csv").SetTimingRepeat(world.GetUpdatesPerTimeUnit());
    
    // update the world for 100 time units
    for (int i=0; i<world.GetUpdatesPerTimeUnit() * 100 + 1; i++) {
        world.Update();
    }

    std::cout << "Black and white test completed. Temperature = " << std::to_string(world.GetGlobalTemperature()) << "; black daisy proportion = " << std::to_string(world.GetProportionBlack()) << "; white daisy proportion = " << std::to_string(world.GetProportionWhite()) << std::endl;
}

/**
 * Run the Update method on a world updates times
 * @param world The world
 * @param updates How many times to call the update function
 */
void UpdateWorldTimes(World& world, int updates) {
    for (int update = 0; update < updates; update++) {
        world.Update();
    }
}

/**
 * Updates the world's luminosity, makes sure daisies are not extinct, and updates the world for updates steps
 * @param world The world
 * @param luminosity The dimensionless luminosity to test at
 * @param updates Number of updates to run at this luminosity
 */
void TestWorldAtLuminosity(World& world, float luminosity, int updates) {
    world.SetSolarLuminosity(luminosity);
    world.BoostDaisiesIfExtinct();
    UpdateWorldTimes(world, updates);
}

/**
 * Test as the solar luminosity rises and falls. Carresponds to graphs (b), (c), and (d) of Daisyworld paper.
 * Outputs what proportion of daisies and temperature the system stabilized at for each luminosity
 * @param whiteEnabled whether to allow white daisies to grow
 * @param blackEnabled whether to allow black daisies to grow
 * @param outputFile name of file to output data to
 * @param minLuminosity minimum dimensionless solar luminosity
 * @param maxLuminosity maximum dimensionless solar luminosity
 * @param luminosityStep how finely to change the luminosity
 * @param timePerLuminosity how long in time units to allow the world to stabilize after the luminosity has changed
 */
void TestRaisingAndLoweringLuminosity(bool whiteEnabled, bool blackEnabled, std::string outputFile, float minLuminosity = 0.5, float maxLuminosity = 1.7, float luminosityStep = 0.01, int timePerLuminosity = 50) {
    // setup world with the first luminosity value
    World world(whiteEnabled ? 0.5 : 0.0, blackEnabled ? 0.5 : 0.0, minLuminosity);
    world.SetWhiteEnabled(whiteEnabled);
    world.SetBlackEnabled(blackEnabled);
    // how many updates to do before switching the luminosity
    int updatesPerLuminosity = timePerLuminosity * world.GetUpdatesPerTimeUnit();
    // record data once per luminosity, at the last update where the world is that luminosity
    world.SetupDataFile(outputFile).SetTimingRepeat(updatesPerLuminosity);
    // give the world one update so that the data file records on the last update that the world is each luminosity
    world.Update();
    // raise the luminosity from minLuminosity to maxLuminosity
    int numberOfLuminosityTrials = std::round((maxLuminosity - minLuminosity) / luminosityStep);
    for (int trial = 0; trial < numberOfLuminosityTrials; trial++) {
        float luminosity = minLuminosity + luminosityStep * trial;
        TestWorldAtLuminosity(world, luminosity, updatesPerLuminosity);
    }
    // lower the luminosity from maxLuminosity to minLuminosity
    for (int trial = numberOfLuminosityTrials; trial >= 0; trial--) {
        float luminosity = minLuminosity + luminosityStep * trial;
        TestWorldAtLuminosity(world, luminosity, updatesPerLuminosity);
    }

    std::cout << "Raising and lowering luminosity test completed." << std::endl;
}

int main(int argc, char* argv[]) {
    // Test 1: make sure that the world can correctly calculate temperature based on the amount of daisies in it
    TestTemperatureCalculations();

    // Test 2: see how the population of black daisies changes over time in a constant-luminosity environment
    TestConstantLuminosityOnlyBlack();

    // Test 3: see how the population of black and white daisies co-change over time in a constant-luminosity environment
    TestConstantLuminosityBlackAndWhite();

    // Test 4: get the temperature of the planet at each luminosity without daisies, corresponding to graph (a) in Daisyworld paper
    // Expected output: temperature is very negative (off graph) when luminosity is 0.5, is about 70 Celsius when luminosity is 1.7,
    // and increases monotonically and concave-down between those.
    TestRaisingAndLoweringLuminosity(false, false, "no_daisies.csv");

    // Test 5: test how the world responds to different luminosities while there are only black daisies, corresponding to graph (b) in Daisyworld paper
    // Expected output: from luminosities 0.7 to 1.1, black daises are able to grow and make the global temperature about 30 Celsius. The Daisyworld
    // paper did not investigate falling luminosities in this scenario.
    TestRaisingAndLoweringLuminosity(false, true, "black.csv");

    // Test 6: test how the world responds to different luminosities when there are only white daisies, corresponding to graph (c) in Daisyworld paper
    // Expected output: white daisies start growing at luminosity about 0.8 and survive until luminosity 1.6, when they abruptly go extinct.
    // For falling luminosities, white daisies don't start thriving until about luminosity 1.2, when they return to the previous curve.
    // While daisies survive, they keep the planet at about 15 to 25 Celsius.
    TestRaisingAndLoweringLuminosity(true, false, "white.csv");

    // Test 7: how does the world respond to different luminosities when stabilized by both white and black daisies, corresponding to graph (d) of Daisyworld paper
    // Expected output: some daisies survive from around luminosities 0.7 to 1.55. Black daisies dominate at the lower end, and white daisies
    // dominate at the upper end. Between these luminosities, the daisies keep the planet around 22.5 Celcius (optimal growing temperature),
    // reaching a minimum at luminosity about 1.4. The Daisyworld paper did not investigate falling luminosities in this scenario.
    TestRaisingAndLoweringLuminosity(true, true, "black_and_white.csv");
};