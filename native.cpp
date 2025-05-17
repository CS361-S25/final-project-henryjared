#include "World.h"

int main(int argc, char* argv[]) {

    // create a world with a 50-50 mix of black and white daisies
    World world(0.5, 0.5, 1);

    // expected output: 0.5
    std::cout << "Global Albedo: " << world.GetTotalAlbedo() << std::endl;
    // expected output: 26
    std::cout << "Global Temperature: " << world.GetGlobalTemperature() << std::endl;

    // expected output: around 31
    std::cout << "Temperature of Black Daisies: " << world.GetTemperatureOfBlackFlowers() << std::endl;
    
    // expected output: around 21
    std::cout << "Temperature of White Daisies: " << world.GetTemperatureOfWhiteFlowers() << std::endl;

};