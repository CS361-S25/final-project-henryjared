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
    // that represent the organisms in the world
    const int num_h_boxes = 10;
    const int num_w_boxes = 10;
    const double RECT_SIDE = 10;
    const double width{num_w_boxes * RECT_SIDE};
    const double height{num_h_boxes * RECT_SIDE};

    emp::web::Canvas canvas{width, height, "canvas"};
    World world{0.5, 0.5, 1.0};

public:

    /**
     * @brief Constructor for the Animator class.
     * Initializes the canvas, sets up the world, and starts the organisms.
     */
    Animator() {

        //Given a perecetnage of black daises, add that many to grid
        //Given a percetnage of white daises, add that many to grid

    }



    void DoFrame() override {

        canvas.Clear();
    }


};