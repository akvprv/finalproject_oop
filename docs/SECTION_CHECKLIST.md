# Assignment Coverage

## Project infrastructure

- [x] Separate core, component, wiring, and simulation modules
- [x] Backend independent of Qt
- [x] Abstract `Component` model
- [x] `Pin` direction, position, hit radius, net, and voltage
- [x] Stable identifiers for components, wires, junctions, and pin references
- [x] CMake and Makefile builds
- [x] Tests without external libraries

## Wiring and connections - 75 points

- [x] Automatic pin detection - 10
- [x] Orthogonal wire routing - 10
- [x] Explicit junctions; crossing wires remain separate - 15
- [x] Dynamic wire updates while a component moves - 20
- [x] Wire removal with junction and net updates - 20

## Basic component library - 120 points

- [x] Ground, DC source, battery, and clock generator - 20
- [x] Resistor, capacitor, and inductor - 30
- [x] Switch, push button, LED, and seven-segment display - 30
- [x] AND, OR, NOT, XOR, NAND, and D flip-flop - 40
- [x] Configurable gate input count
- [x] Voltage-based low, high, and undefined states
- [x] Propagation delay
- [x] Rising-edge D flip-flop

## Simulation controls - 40 points

- [x] Run, pause, and stop - 15
- [x] Live high, low, and undefined wire colors - 10
- [x] Live switch and push-button interaction - 8
- [x] Step to the next event or a fixed time - 7

## Frontend integration

- [x] Component creation at scene coordinates
- [x] Pin coordinates after rotation and mirroring
- [x] Routes ready for `QPainterPath`
- [x] Hex wire colors ready for `QColor`
- [x] Structured messages for `QPlainTextEdit`

## Tests

- [x] Pin transforms
- [x] Orthogonal routing
- [x] Net connectivity and wire dragging
- [x] Crossings with and without junctions
- [x] Gate states and undefined propagation
- [x] Rising-edge D flip-flop
- [x] Run, pause, stop, and step
- [x] Live switch updates
- [x] Wire colors
- [x] Floating inputs
- [x] Conflicting output sources

The R, C, and L classes include pins and editable values. The current engine is
a digital logic simulator and does not include a full analog MNA solver.
