# Low earth orbit visualizer

Inspirations:
- https://orbital-watch-pink.vercel.app/
- https://www.jack-huston.com/TLE-Satellite-Plotter/

https://en.wikipedia.org/wiki/Halo_(optical_phenomenon)

- Interpolate propagated satellite positions
- The earth should be tilted on its axis
- Initially rotate the earth so that it's rotation matches the actual earth's rotation
- Don't allow zooming through the planet
- Render a small halo around the earth
- Render clouds on the globe
- Decide whether to use the daymap or the nightmap based off of the simulated time (current user time?)
- Click on a satellite to show info on it
- Trace satellite trajectories using a curved line
- Basic UI using imgui:
  - List of all the satellites (name, type, description)
  - Filter satellites by type, by producer, etc
- Issue HTTP requests to pull in new satellite csvs every few hours (maybe during app load to get fresh data?)
- Zoom in on your GPS position and highlight the swarm of satellites that are overhead
- Port to WASM, release project

---

```bash
# Install tools to speed up compilation
sudo apt install mold ccache

# Development build setup
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_LINKER_TYPE=MOLD
```

---

- Low earth orbit: Orbit altitude of <= 2000 km.
- Drag: An opposing force exterted by the Earth's atmosphere on objects orbiting in LEO.
- SGP4: Simplified general pertubations 4
- TLE: Two-line element set. Although it's the de facto standard, it's very outdated (Y2K problems, running out of satellite identifiers, etc), so instead we'll be parsing CSV
- RAAN: The angle between the vernal equinox and the ascending node.
- The eccentricty of an ellipse is the distance between the two foci [-sqrt(a^2 - b^2), -sqrt(a^2 - b^2)], divided by the semi major axis.

- Periapsis: Point in an orbit closest to the center of mass of a system.
- Apoapsis: Point in an orbit furthest from the center of mass of a system.
  - Perigee and apogee are terms used specifically for Earth.

- Orbital node: When an orbit intersects a plane of reference to which it's inclined.
  - Ascending node: When the orbit intersects the plane of reference moving north.
  - Descending node: When the orbit intersects the plane of reference moving south.

---

- [Billboards](https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/)
- [Normal mapping](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/)
- [Coordinate reference frames](https://github.com/horizonanalytic/astrora/blob/main/src/coordinates/frames.rs)
- [Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)
- [Shader storage buffer objects](https://ktstephano.github.io/rendering/opengl/ssbos)
- [Creating an icosphere mesh in code](http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html)
- [Sphere.cpp](https://gist.github.com/Pikachuxxxx/5c4c490a7d7679824e0e18af42918efc)
- [Planetary Orbits](https://science.nasa.gov/learn/basics-of-space-flight/chapter5-1/)
- [Supplemental GP Element Sets](https://celestrak.org/NORAD/elements/supplemental/index.php?FORMAT=csv)
- [Satellite drag](https://www.swpc.noaa.gov/impacts/satellite-drag)
- [SPACETRACK REPORT NO. 3](https://celestrak.org/NORAD/documentation/spacetrk.pdf)
- [Revisiting Spacetrack Report #3: Rev 2](https://celestrak.org/publications/AIAA/2006-6753/AIAA-2006-6753-Rev2.pdf)
- [Two-line element set](https://en.wikipedia.org/wiki/Two-line_element_sete)
- [A New Way to Obtain GP Data (aka TLEs)](https://celestrak.org/NORAD/documentation/gp-data-formats.php)
- [Classical/Keplerian Orbital Elements](https://www.youtube.com/watch?v=AReKBoiph6g)
- [Introduction to spherical coordinates, Multivariable Calculus](https://www.youtube.com/watch?v=8x_UjFUySRg)
- [Solar Textures](https://www.solarsystemscope.com/textures/)
- [Deep star maps 2020](https://svs.gsfc.nasa.gov/4851)
