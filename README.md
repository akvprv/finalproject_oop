# Proteus Backend

This repository contains the C++ backend for the Proteus-style circuit editor.
The model is independent of Qt, so the user interface can focus on drawing and
input handling while the backend owns circuit state and simulation behavior.

## Features

- Component and pin models with local and scene coordinates
- Rotation and horizontal or vertical mirroring
- Pin hit testing
- Circuit, wire, junction, and net management
- Orthogonal wire routing and live rerouting while components move
- Ground, DC source, battery, and clock source
- Resistor, capacitor, and inductor models
- Switch, push button, LED, and seven-segment display
- AND, OR, NOT, XOR, NAND, and rising-edge D flip-flop
- Configurable logic levels and propagation delay
- Run, pause, stop, and single-step simulation controls
- Live wire colors for high, low, and undefined signals
- Floating-input, output-conflict, and convergence diagnostics

## Reviewed corrections

- Removes stale junctions and rebuilds nets after live wire rerouting.
- Rejects duplicate junctions and invalid non-finite component values.
- Treats every error-level preflight diagnostic as a simulation blocker.
- Exposes complete editable properties for LEDs, displays, gates, and DFFs.
- Adds regression tests for junction lifecycle and numeric validation.

The submitted ZIP intentionally excludes the working `.git` directory. Import
the files into the team repository and create commits with the real contributor
identity after review.

## Build

Linux or MinGW:

```bash
make test
make demo
./build/proteus_demo
```

CMake, Qt Creator, or Visual Studio:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The project requires C++20 and CMake 3.20 or newer. Link the Qt application to
the `proteus_backend` target.

## Layout

- `include/proteus/core`: circuit, component, pin, wire, and junction models
- `include/proteus/components`: source, passive, interactive, and digital parts
- `include/proteus/simulation`: simulation engine and diagnostics
- `include/proteus/wiring`: orthogonal routing
- `tests/test_main.cpp`: standalone tests with no third-party dependency
- `docs/QT_INTEGRATION.md`: Qt integration notes
- `docs/SECTION_CHECKLIST.md`: assignment coverage
- `docs/GIT_WORKFLOW.md`: Git workflow

## Scope

The simulation engine focuses on digital logic and interactive components.
Resistors, capacitors, and inductors have complete component models and editable
values, but the project does not include a full analog MNA solver.
