# Daisyworld Simulation ![White Daisy](images/title_flower.png)

This project is a reimplementation and extension of the classic Daisyworld model by Watson & Lovelock (1983), built as an interactive web simulation.

## What is Daisyworld?

Daisyworld is a simple artificial life and climate model that demonstrates how living organisms (daisies) can regulate a planet’s temperature through feedback with their environment. Black daisies warm the planet by absorbing sunlight, while white daisies cool it by reflecting sunlight. The model shows how life and environment can co-evolve to maintain habitable conditions.

## Features

- **Interactive Web Simulation:** Adjust solar luminosity and daisy types in real time.
- **Multiple Daisy Types:** Includes black, white, and gray (neutral) daisies.
- **Flat and Round Planet Modes:** Simulate a world with or without latitude-based temperature gradients.
- **Visualization:** See daisy populations, temperature, and solar luminosity as the simulation runs.
- **Configurable Parameters:** Change simulation settings via a user-friendly panel with tooltips.

## How to Use

1. **Build the Project:**  
   Compile with Emscripten, making sure to include all images and resources.

2. **Run `compile-run-web.sh`:**  
   Launch the simulation in your browser.

3. **Interact:**  
   - Use the config panel to enable/disable daisy types, adjust solar luminosity, and toggle latitude simulation.
   - Watch the grid, thermometer, sun, and population bars update in real time.
   - Hover over config options for helpful tooltips.

## Scientific Background

- **Original Model:**  
  Watson & Lovelock’s Daisyworld demonstrates planetary temperature regulation by life. The model uses simple equations for temperature, daisy growth, and feedback.
- **Extensions:**  
  This version adds gray daisies (neutral albedo) and a round world with latitude-dependent sunlight, allowing for more realistic and varied ecological dynamics.

## Results

- **Replication:**  
  The simulation reproduces the results of the original Daisyworld paper, showing stable planetary temperatures over a wide range of solar luminosities.
- **Extensions:**  
  Adding gray daisies and latitude effects reveals new dynamics, such as competition and coexistence across different regions of the planet.

## Limitations

- The model is not agent-based; it tracks daisy populations as continuous variables.
- Some results (e.g., gray daisy population curves) may differ slightly from the original paper due to longer simulation times for steady state.

## References

- Watson, A. J., & Lovelock, J. E. (1983). Biological homeostasis of the global environment: the parable of Daisyworld. *Tellus B: Chemical and Physical Meteorology*, 35(4), 284-289.

---

**Explore Daisyworld and see how life can shape a planet!**