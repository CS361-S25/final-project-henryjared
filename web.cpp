#define UIT_VENDORIZE_EMP
#define UIT_SUPPRESS_MACRO_INSEEP_WARNINGS

#include "emp/math/Random.hpp"
#include "emp/web/Animate.hpp"
#include "emp/web/web.hpp"
#include "emp/web/Document.hpp"
#include "emp/config/ArgManager.hpp"
#include "emp/prefab/ConfigPanel.hpp"
#include "emp/web/UrlParams.hpp"

#include "ConfigSetup.h"
#include "World.h"

emp::web::Document doc{"target"};
emp::web::Document buttons("buttons");
MyConfigType config;

class Animator : public emp::web::Animate {

    // Constants for the grid size and rectangle dimensions
    // These are used to define the size of the canvas and the rectangles
    // that represent the daisies in the world
    const int num_h_boxes = 10;
    const int num_w_boxes = 10;
    const double RECT_SIDE = 30;
    const double width{num_w_boxes * RECT_SIDE};
    const double height{num_h_boxes * RECT_SIDE};
    
    // these constants determine how the world slowly changes in luminosity over time
    const float min_luminosity = 0.5;
    const float max_luminosity = 1.7;
    const float luminosity_change_per_frame = 0.001;
    const float world_time_per_frame = 0.5;

    // the current luminosity of the world
    float luminosity = 1.0;

    // whether the luminosity is currently on its part of the cycle where it is increasing
    bool increasing_luminosity = true;

    emp::web::Canvas canvas{width, height, "canvas"};

    World world{0.5, 0.5, 1};

    // 2D grid to store the color of each cell
    std::vector<std::vector<std::string>> grid;

    bool grayEnabled;

public:
    
    Animator() {

        // apply configuration query params and config files to config
        auto specs = emp::ArgManager::make_builtin_specs(&config);
        emp::ArgManager am(emp::web::GetUrlParams(), specs);
        am.UseCallbacks();
        if (am.HasUnused()) std::exit(EXIT_FAILURE);

        // setup configuration panel
        emp::prefab::ConfigPanel config_panel(config);
        config_panel.SetRange("LUMINOSITY", "0.5", "1.7");

        luminosity = config.LUMINOSITY();
        grayEnabled = config.ENABLE_GRAY();
        world.SetSolarLuminosity(luminosity);
        world.SetGrayEnabled(grayEnabled);

        doc << canvas;
        buttons << GetToggleButton("Toggle");
        buttons << GetStepButton("Step");
        buttons << config_panel;
        UpdateGrid();
    }

    /**
     * @brief Updates the grid with a new distribution of cell colors.
     *
     * This function recalculates the number of black, white, and green cells based on the current
     * proportions provided by the world object and the total number of cells in the grid. It then
     * fills the grid with the appropriate number of each color, ensuring the total matches the grid size.
     */
    void UpdateGrid() {

        emp::Random random{444};

        // Calculate the number of each color
        int total_cells = num_h_boxes * num_w_boxes;
        int num_black = (total_cells * world.GetProportionBlack());
        int num_white = (total_cells * world.GetProportionWhite());
        int num_gray = (total_cells * world.GetProportionGray());
        std::cout << std::to_string(num_gray) << std::endl;
        int num_green = total_cells - num_black - num_white - num_gray;

        // Create a flat vector to represent all cells
        std::vector<std::string> cells;
        cells.insert(cells.end(), num_black, "black");
        cells.insert(cells.end(), num_green, "green");
        cells.insert(cells.end(), num_gray, "gray");
        cells.insert(cells.end(), num_white, "white");
        

        // Shuffle the cells for random placement
        for (int i = cells.size() - 1; i > 0; --i) {
            int j = random.GetUInt(i + 1);
            std::swap(cells[i], cells[j]);
        }

        // Fill the grid with shuffled cells
        grid.resize(num_h_boxes, std::vector<std::string>(num_w_boxes));
        int idx = 0;
        for (int y = 0; y < num_h_boxes; ++y) {
            for (int x = 0; x < num_w_boxes; ++x) {
                grid[y][x] = cells[idx++];
            }
        }
    }

    /**
     * @brief Draws the current grid state onto the canvas.
     *
     * Iterates through each cell in the grid and draws a rectangle
     * with the appropriate color (black, white, or green) at the corresponding
     * position on the canvas.
     */
    void Draw() {

        for (int y = 0; y < num_h_boxes; ++y) {
            for (int x = 0; x < num_w_boxes; ++x) {
                // Get the color for the current cell
                std::string color = grid[y][x];

                std::string src;

                if (color == "black") {src = "images/black_daisy.png";}
                else if (color == "white") {src = "images/white_daisy.png";}
                else if (color == "gray") {src = "images/gray_daisy.png";}
                else {src = "images/grass.png";}
                
                // Draw the rectangle at the correct position with the chosen color
                canvas.Image(src, x * RECT_SIDE, y * RECT_SIDE, RECT_SIDE, RECT_SIDE);
            }
        }
    }

    /**
     * @brief Updates the thermometer display in the web interface to reflect the current global temperature.
     *
     * This function retrieves the current global temperature from the world object,
     * calculates its percentage within a defined range (min_temp to max_temp),
     * and visually represents this value as a filled bar (thermometer).
     */
    void UpdateThermometer() {

        // Get the current global temperature from the world
        float temp = world.GetGlobalTemperature();

        // Define the minimum and maximum temperature for the thermometer scale
        float min_temp = -20;
        float max_temp = 70;

        float percent = (temp - min_temp) / (max_temp - min_temp);
        percent = std::max(0.0f, std::min(1.0f, percent)); // Clamp between 0 and 1

        int bar_height = 150;
        int fill_height = static_cast<int>(bar_height * percent);

        std::stringstream ss;
        ss << "<div style='width:100%; text-align:center; font-size:1em; margin-bottom:4px;'>";
        ss << std::setprecision(1) << std::fixed << temp << "°C</div>";
        ss << "<div style='width:40px; height:" << bar_height << "px; border:1px solid #333; background:#eee; position:relative; margin: 0 auto;'>";
        ss << "<div style='position:absolute; bottom:0; width:100%; height:" << fill_height << "px; background:#f55;'></div>";
        ss << "</div>";

        emp::web::Document doc_thermo("thermometer");
        doc_thermo.Clear();
        doc_thermo << ss.str();
    }

    /**
     * Changes the luminosity a tiny amount each frame in a triangle wave
     */
    void UpdateLuminosity() {
        if (increasing_luminosity) {
            luminosity += luminosity_change_per_frame;
            // turn around when reach top
            if (luminosity >= max_luminosity) increasing_luminosity = false;
        } else {
            luminosity -= luminosity_change_per_frame;
            // turn around when reach top
            if (luminosity <= min_luminosity) increasing_luminosity = true;
        }
        world.SetSolarLuminosity(luminosity);
        world.BoostDaisiesIfExtinct();
    }

    /**
     * @brief Updates the sun visualization in the web interface based on the current solar luminosity.
     *
     * This function retrieves the current solar luminosity from the world object, clamps and scales it
     * to a displayable percentage, and then generates an SVG representation of the sun.
     */
    void UpdateSun() {

        float lum = world.GetSolarLuminosity();

        // Clamp and scale for display
        float percent = (lum - min_luminosity) / (max_luminosity - min_luminosity);
        percent = std::max(0.0f, std::min(1.0f, percent));

        // Sun size
        int radius = 60;

        // Color: from yellow (255,255,0) to white (255,255,255)
        int color_val = static_cast<int>(percent * 255); // 0-255 for blue component
        std::stringstream color;
        color << "rgb(255,255," << color_val << ")";

        std::stringstream ss;
        ss << "<svg width='150' height='150'>";
        ss << "<circle cx='60' cy='75' r='" << radius << "' fill='" << color.str() << "' stroke='#aaa'/>";
        ss << "<text x='60' y='80' text-anchor='middle' font-size='20' fill='#333'>" 
        << std::setprecision(2) << std::fixed << lum << "</text>";
        ss << "</svg>";

        emp::web::Document doc_sun("sun");
        doc_sun.Clear();
        doc_sun << ss.str();
    }


    void DoFrame() override {

        canvas.Clear();
        int number_of_updates = world.GetUpdatesPerTimeUnit() * world_time_per_frame;
        for (int update = 0; update < number_of_updates; update++) {
            world.Update();
        }

        UpdateGrid();
        Draw();
        UpdateThermometer();
        UpdateSun();
        UpdateLuminosity();
        std::cout << std::to_string(world.GetProportionGray()) << std::endl;
    }
};

Animator animator;

int main() {
    animator.Step();
}