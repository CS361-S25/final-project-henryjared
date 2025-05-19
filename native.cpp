#include "World.h"

int main(int argc, char* argv[]) {

    // create a world with a 50-50 mix of black and white daisies
    World world(0.5, 0.5, 1);
    world.SetDaisyGrowthAndDeath(false);

    // expected output: 0.5
    std::cout << "Global Albedo: " << world.GetTotalAlbedo() << std::endl;
    // expected output: 26
    std::cout << "Global Temperature: " << world.GetGlobalTemperature() << std::endl;

    // expected output: around 31
    std::cout << "Temperature of Black Daisies: " << world.GetTemperatureOfBlackFlowers() << std::endl;
    
    // expected output: around 21
    std::cout << "Temperature of White Daisies: " << world.GetTemperatureOfWhiteFlowers() << std::endl;

    // perform a test where there are only black daisies and they grow and die over time
    // starting amounts: black = 0.5. Solar luminosity = 1
    // EXPECTED OUTPUT (based on Daisyworld paper graph (b)) stabilizing around a_b = 0.15, T_e = 35
    World world2(0, 0.5, 1);
    world2.SetWhiteEnabled(false);

    // output data every 100 updates (1 time unit)
    world2.SetupDataFile("black.data").SetTimingRepeat(100);

    // update the world 10000 times (100 time units)
    for (int i=0; i<10001; i++) {
        world2.Update();
        // output data to console every time unit
        if (i % 100 == 0) {
            std::cout << "Black test Update " << std::to_string(i) << ", temperature: " << std::to_string(world2.GetGlobalTemperature()) << std::endl;
        }
    }
    
    // perform another test, where daisies may grow and die over time and there are both black and white daisies
    // starting amounts = white 0.5, black 0.5. Solar luminosity = 1
    // EXPECTED OUTPUT (based on Daisyworld paper graph (d)) stabilizing around a_b = 0.3, a_w = 0.4, T_e = 22
    World world3(0.5, 0.5, 1);

    // output data every 100 updates (1 time unit)
    world3.SetupDataFile("black_and_white.data").SetTimingRepeat(100);
    
    // update the world 10000 times (100 time units)
    for (int i=0; i<10001; i++) {
        world3.Update();
        // output data to console every time unit
        if (i % 100 == 0) {
            std::cout << "Black+White test Update " << std::to_string(i) << ", temperature: " << std::to_string(world3.GetGlobalTemperature()) << std::endl;
        }
    }

};