# Low earth orbit visualizer

Inspirations:
- https://orbital-watch-pink.vercel.app/
- https://www.jack-huston.com/TLE-Satellite-Plotter/

TODO:

- [ ] Zoom the visualization out to view satellites at further distance orbits
    - Scale the visualization to render all the Celestrak satellites

- [ ] Color the circles based off of the satellite type:
  (payload, rocket body, debris, unknown)

- [ ] Trace satellite trajectories using curve line around the globe

- [ ] Show satellite info in basic ImGUI UI when clicking on a circle:
  (name, catalogue number, norad id, type, inclination, perigee,
  apogee, velocity, lattitude, longittude, eccentricity, mean motion, epoch)
    - https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp

    - [ ] Highlight the satellite circle when clicked

    - [ ] List the number of each type of satellites next to a checkbox that can be used to toggle their filtering

    - [ ] Search for a specific satellite by name and move camera towards it if found

- [ ] Write a shell script to pull the latest Celestrack csv satellite data

- [ ] Reduce the app's startup time

- [ ] Continue to drag the camera after the mouse is released

- [ ] Improve startup time

- [ ] Port to WASM, release project, finish the project by **May 13**

---

```bash
# Install tools to speed up compilation
sudo apt install mold ccache

# Development build setup
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_LINKER_TYPE=MOLD
```
