#define UIT_VENDORIZE_EMP
#define UIT_SUPPRESS_MACRO_INSEEP_WARNINGS

#include "emp/math/Random.hpp"
#include "emp/web/Animate.hpp"
#include "emp/web/web.hpp"
#include "World.h"

emp::web::Document doc{"target"};

class Animator : public emp::web::Animate {

    // Constants for the grid size and rectangle dimensions
    // These are used to define the size of the canvas and the rectangles
    // that represent the daisies in the world
    const int num_h_boxes = 10;
    const int num_w_boxes = 10;
    const double RECT_SIDE = 25;
    const double width{num_w_boxes * RECT_SIDE};
    const double height{num_h_boxes * RECT_SIDE};

    emp::web::Canvas canvas{width, height, "canvas"};
    emp::Random random{444};

    World world{0.5, 0.5, 1.0};

    // 2D grid to store the color of each cell
    std::vector<std::vector<std::string>> grid;

public:

    Animator() {

        doc << canvas;
        doc << GetToggleButton("Toggle");
        doc << GetStepButton("Step");

        UpdateGrid();
    }

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

    void Draw() {

        for (int y = 0; y < num_h_boxes; ++y) {
            for (int x = 0; x < num_w_boxes; ++x) {
                std::string color = grid[y][x];
                std::string fill;
                if (color == "black") fill = "black";
                else if (color == "white") fill = "white";
                else fill = "green";
                canvas.Rect(x * RECT_SIDE, y * RECT_SIDE, RECT_SIDE, RECT_SIDE, fill, "black");
            }
        }
    }

    void UpdateThermometer() {

        float temp = world.GetGlobalTemperature();
        float min_temp = 0; // adjust as needed
        float max_temp = 70;  // adjust as needed
        float percent = (temp - min_temp) / (max_temp - min_temp);
        percent = std::max(0.0f, std::min(1.0f, percent)); // clamp between 0 and 1

        int bar_height = 200;
        int fill_height = static_cast<int>(bar_height * percent);

        std::stringstream ss;
        ss << "<div style='width:40px; height:" << bar_height << "px; border:1px solid #333; background:#eee; position:relative;'>";
        ss << "<div style='position:absolute; bottom:0; width:100%; height:" << fill_height << "px; background:#f55;'></div>";
        ss << "<div style='position:absolute; top:0; width:100%; text-align:center; font-size:1em;'>";
        ss << std::setprecision(1) << std::fixed << temp << "°C</div>";
        ss << "</div>";

        emp::web::Document doc_thermo("thermometer");
        doc_thermo.Clear();
        doc_thermo << ss.str();
    }


    void DoFrame() override {

        canvas.Clear();
        world.Update();

        UpdateGrid();
        Draw();
        UpdateThermometer();
    }
};

Animator animator;

int main() {
    animator.Step();
}