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
    world.SetupDataFile("data/constant_luminosity_black.csv").SetTimingRepeat(world.GetUpdatesPerTimeUnit());

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
    world.SetupDataFile("data/constant_luminosity_black_and_white.csv").SetTimingRepeat(world.GetUpdatesPerTimeUnit());
    
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
        // boost the daisies halfway through to allow them to respond to other types of daisies growing
        if (update == updates / 2) world.BoostDaisiesIfExtinct();
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
 * @param grayEnabled whether to allow gray daisies to grow
 * @param roundWorld whether to have different daisy populations and sunlight at different latitudes of the world
 */
void TestRaisingAndLoweringLuminosity(bool whiteEnabled, bool blackEnabled, std::string outputFile, float minLuminosity = 0.5, float maxLuminosity = 1.7, float luminosityStep = 0.01, int timePerLuminosity = 500, bool grayEnabled = false, bool roundWorld = false) {
    // setup world with the first luminosity value
    // when all 3 are enabled, each starts with 0.33, otherwise, each starts with 0.5 as long as it's enabled
    World world(whiteEnabled ? 0.33 : 0.0, blackEnabled ? 0.33 : 0.0, minLuminosity, grayEnabled ? 0.33 : 0.0, roundWorld);
    world.SetWhiteEnabled(whiteEnabled);
    world.SetBlackEnabled(blackEnabled);
    world.SetGrayEnabled(grayEnabled);
    // std::cout << "Conducting new test. Starting white = " << std::to_string(world.GetProportionWhite()) << ", starting black = " << std::to_string(world.GetProportionBlack()) << ", starting gray = " << std::to_string(world.GetProportionGray()) << std::endl;
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
    std::cout << "Test 1" << std::endl;
    // Test 1: make sure that the world can correctly calculate temperature based on the amount of daisies in it
    TestTemperatureCalculations();

    std::cout << "Test 2" << std::endl;
    // Test 2: see how the population of black daisies changes over time in a constant-luminosity environment
    TestConstantLuminosityOnlyBlack();

    std::cout << "Test 3" << std::endl;
    // Test 3: see how the population of black and white daisies co-change over time in a constant-luminosity environment
    TestConstantLuminosityBlackAndWhite();

    std::cout << "Test 4" << std::endl;
    // Test 4: get the temperature of the planet at each luminosity without daisies, corresponding to graph (a) in Daisyworld paper
    // Expected output: temperature is very negative (off graph) when luminosity is 0.5, is about 70 Celsius when luminosity is 1.7,
    // and increases monotonically and concave-down between those.
    TestRaisingAndLoweringLuminosity(false, false, "data/no_daisies.csv");

    std::cout << "Test 5" << std::endl;
    // Test 5: test how the world responds to different luminosities while there are only black daisies, corresponding to graph (b) in Daisyworld paper
    // Expected output: from luminosities 0.7 to 1.1, black daises are able to grow and make the global temperature about 30 Celsius. The Daisyworld
    // paper did not investigate falling luminosities in this scenario.
    TestRaisingAndLoweringLuminosity(false, true, "data/black.csv");

    std::cout << "Test 6" << std::endl;
    // Test 6: test how the world responds to different luminosities when there are only white daisies, corresponding to graph (c) in Daisyworld paper
    // Expected output: white daisies start growing at luminosity about 0.8 and survive until luminosity 1.6, when they abruptly go extinct.
    // For falling luminosities, white daisies don't start thriving until about luminosity 1.2, when they return to the previous curve.
    // While daisies survive, they keep the planet at about 15 to 25 Celsius.
    TestRaisingAndLoweringLuminosity(true, false, "data/white.csv");

    std::cout << "Test 7" << std::endl;
    // Test 7: how does the world respond to different luminosities when stabilized by both white and black daisies, corresponding to graph (d) of Daisyworld paper
    // Expected output: some daisies survive from around luminosities 0.7 to 1.55. Black daisies dominate at the lower end, and white daisies
    // dominate at the upper end. Between these luminosities, the daisies keep the planet around 22.5 Celcius (optimal growing temperature),
    // reaching a minimum at luminosity about 1.4. The Daisyworld paper did not investigate falling luminosities in this scenario.
    TestRaisingAndLoweringLuminosity(true, true, "data/black_and_white.csv");

    std::cout << "Test 8" << std::endl;
    // Test 8 (extension 1): how does the world react when there are only gray daisies, that are the same albedo as the ground, corresponding to graph (a) of Daisyworld paper
    // Expected output: same temperature as without any daisies. Gray daisies exist from luminosities 0.8 to 1.2
    // and peak around 1.0.
    TestRaisingAndLoweringLuminosity(false, false, "data/gray.csv", 0.5, 1.7, 0.01, 500, true);

    std::cout << "Test 9" << std::endl;
    // Test 9 (extension 1): how does the world react when there are white, gray, and black daisies?
    // Not tested in Daisyworld paper. Prediction: the gray daisies will take up room and reduce the ability for white and black daisies
    // to stabilize the environment.
    TestRaisingAndLoweringLuminosity(true, true, "data/white_black_and_gray.csv", 0.5, 1.7, 0.01, 500, true);

    std::cout << "Test 10" << std::endl;
    // Test 10 (extension 2): what if the world was round and different latitudes recieve different amounts of sunlight?
    // Control test: baseline average temperature of planet without any daisies.
    TestRaisingAndLoweringLuminosity(false, false, "data/no_daisies_round.csv", 0.5, 1.7, 0.01, 500, false, true);

    std::cout << "Test 11" << std::endl;
    // Test 11 (extension 2): A round world with only black daisies.
    // Not tested in Daisyworld paper. Prediction: the center of the population of black daisies will move towards the poles as luminosity
    // increases. Daisies will persist in the world for a wider range of luminosities.
    TestRaisingAndLoweringLuminosity(false, true, "data/black_round.csv", 0.5, 1.7, 0.01, 500, false, true);

    std::cout << "Test 12" << std::endl;
    // Test 12 (extension 2): A round world with only white daisies.
    // Not tested in Daisyworld paper. Prediction: the center of the population of white daisies will move towards the poles as luminosity
    // increases. White daisies will do better than black daisies did for higher luminosities. Daisies will persist in the world for a wider range of luminosities.
    TestRaisingAndLoweringLuminosity(true, false, "data/white_round.csv", 0.5, 1.7, 0.01, 500, false, true);

    std::cout << "Test 13" << std::endl;
    // Test 13 (extension 2): A round world with both black and white daisies.
    // Not tested in Daisyworld paper. Prediction: white daisies will thrive at lower latitudes while black daisies thrive at higher latitudes.
    // Daisies will persist on the world for a wider range of solar luminosities, which will stabilize the temperature for also a wider range of luminosities.
    TestRaisingAndLoweringLuminosity(true, true, "data/white_black_round.csv", 0.5, 1.7, 0.01, 500, false, true);

    std::cout << "Test 14" << std::endl;
    // Test 14 (extension 1+2): A round world with white, black, and gray daisies.
    TestRaisingAndLoweringLuminosity(true, true, "data/white_black_and_gray_round.csv", 0.5, 1.7, 0.01, 500, true, true);
};