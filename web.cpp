#define UIT_VENDORIZE_EMP
#define UIT_SUPPRESS_MACRO_INSEEP_WARNINGS

#include "emp/math/Random.hpp"
#include "emp/web/Animate.hpp"
#include "emp/web/web.hpp"
#include "World.h"

emp::web::Document doc{"target"};
emp::web::Document buttons("buttons");

class Animator : public emp::web::Animate {

    // Constants for the grid size and rectangle dimensions
    // These are used to define the size of the canvas and the rectangles
    // that represent the daisies in the world
    const int num_h_boxes = 10;
    const int num_w_boxes = 10;
    const double RECT_SIDE = 30;
    const double width{num_w_boxes * RECT_SIDE};
    const double height{num_h_boxes * RECT_SIDE};

    emp::web::Canvas canvas{width, height, "canvas"};
    emp::Random random{444};

    World world{0.5, 0.5, 2};

    // 2D grid to store the color of each cell
    std::vector<std::vector<std::string>> grid;

public:
    
    Animator() {

        doc << canvas;
        buttons << GetToggleButton("Toggle");
        buttons << GetStepButton("Step");
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

        // Calculate the number of each color
        int total_cells = num_h_boxes * num_w_boxes;
        int num_black = (total_cells * world.GetProportionBlack());
        int num_white = (total_cells * world.GetProportionWhite());
        int num_green = total_cells - num_black - num_white;

        // Create a flat vector to represent all cells
        std::vector<std::string> cells;
        cells.insert(cells.end(), num_black, "black");
        cells.insert(cells.end(), num_white, "white");
        cells.insert(cells.end(), num_green, "green");

        // Fill the grid
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
                std::string fill;

                // Determine the fill color based on the cell's value
                if (color == "black") fill = "black";
                else if (color == "white") fill = "white";
                else fill = "green"; // Default to green for any other value

                // Draw the rectangle at the correct position with the chosen color
                canvas.Rect(x * RECT_SIDE, y * RECT_SIDE, RECT_SIDE, RECT_SIDE, fill, "black");
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
        float min_temp = 0;
        float max_temp = 70;

        float percent = (temp - min_temp) / (max_temp - min_temp);
        percent = std::max(0.0f, std::min(1.0f, percent)); // Clamp between 0 and 1

        int bar_height = 150;
        int fill_height = static_cast<int>(bar_height * percent);

        std::stringstream ss;
        ss << "<div style='width:100%; text-align:center; font-size:1em; margin-bottom:4px; margin-left:25px;'>";
        ss << std::setprecision(1) << std::fixed << temp << "°C</div>";
        ss << "<div style='width:40px; height:" << bar_height << "px; border:1px solid #333; background:#eee; position:relative; margin-left:40px;'>";
        ss << "<div style='position:absolute; bottom:0; width:100%; height:" << fill_height << "px; background:#f55;'></div>";
        ss << "</div>";

        emp::web::Document doc_thermo("thermometer");
        doc_thermo.Clear();
        doc_thermo << ss.str();
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
        float min_lum = 0;
        float max_lum = 2;
        float percent = (lum - min_lum) / (max_lum - min_lum);
        percent = std::max(0.0f, std::min(1.0f, percent));

        // Sun size
        int radius = 50;

        // Color: from yellow (255,255,0) to white (255,255,255)
        int color_val = static_cast<int>(percent * 255); // 0-255 for blue component
        std::stringstream color;
        color << "rgb(255,255," << color_val << ")";

        std::stringstream ss;
        ss << "<svg width='150' height='150'>";
        ss << "<circle cx='60' cy='75' r='" << radius << "' fill='" << color.str() << "' stroke='#aaa'/>";
        ss << "<text x='60' y='80' text-anchor='middle' font-size='20' fill='#333'>" 
        << std::setprecision(1) << std::fixed << lum << "</text>";
        ss << "</svg>";

        emp::web::Document doc_sun("sun");
        doc_sun.Clear();
        doc_sun << ss.str();
    }


    void DoFrame() override {

        canvas.Clear();
        world.Update();

        UpdateGrid();
        Draw();
        UpdateThermometer();
        UpdateSun();
    }
};

Animator animator;

int main() {
    animator.Step();
}