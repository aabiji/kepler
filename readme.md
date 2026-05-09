# Low earth orbit visualizer

Inspirations:
- https://orbital-watch-pink.vercel.app/
- https://www.jack-huston.com/TLE-Satellite-Plotter/

TODO:

- Introduce a new struct to share Instances and Satellites between the 2 main and simulation threads

- Increase zoom out distance to view satellites at further distance orbits

- Stop hardcoding asset paths
    - Add a resource manager that'll verify the necessary files exist, map resource enum to actual paths, etc

- Remove the color field from the satellite data, they should all have the same color, add new info fields to the `Satellite` struct

- Use cpp-httplb to load TLE data from "https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle"

    - Switch from parsing CSV to parsing 3 line element data

    - How many satellites will we have? Will we have to do optimizations?

- Search for satellite by name or by norad ID. If found, treat the satellite as if it were selected

- Render the predicted orbit of a satellite at a time

- [ ] Port to WASM, release project, finish the project by **May 13**

---

```bash
# Install tools to speed up compilation
sudo apt install mold ccache

# Development build setup
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_LINKER_TYPE=MOLD
```
