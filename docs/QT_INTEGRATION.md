# Qt Integration

The backend has no Qt dependency. A graphics item only needs to keep the
`ComponentId` of its model. Labels, positions, pins, and simulation state stay
in `Circuit`.

## Creating components

Create a model when the user places a part on the scene:

```cpp
auto& gate = circuit.emplaceComponent<LogicGate>(
    "U1", GateKind::And, 2, 5e-9, LogicLevels{}, Point{x, y});
```

## Pins and hit testing

`pinPosition` returns scene coordinates after rotation and mirroring:

```cpp
Point scenePosition = model.pinPosition(pinIndex);
bool highlighted = model.pin(pinIndex).isMouseOver(mousePoint, scenePosition);
```

## Wires

Connect two selected pins and convert the returned route to a `QPainterPath`:

```cpp
Wire& wire = circuit.connect(firstPinRef, secondPinRef);
for (const Point& point : wire.route()) {
    QPointF scenePoint{point.x, point.y};
}
```

Crossing wires remain separate until the user creates a junction:

```cpp
circuit.addJunction(clickPoint, {firstWireId, secondWireId});
```

## Moving and transforming parts

```cpp
circuit.moveComponent(componentId, {newX, newY});
circuit.component(componentId).rotateClockwise();
circuit.component(componentId).mirrorHorizontal();
circuit.refreshWireRoutes(componentId);
```

Reload the connected wire routes after each operation.

## Simulation controls

Keep one engine per open project:

```cpp
SimulationEngine engine(circuit);
engine.run();
engine.pause();
engine.stepToNextEvent();
engine.stop();
```

Call `advance` from a `QTimer` while the engine is running:

```cpp
engine.advance(deltaTimeSeconds);
```

Wire colors are returned as hexadecimal strings that can be passed to
`QColor`:

```cpp
QString color = QString::fromStdString(engine.wireColor(wireId));
```

Switches and buttons can be changed directly from mouse events:

```cpp
switchModel.toggle();
buttonModel.press();
buttonModel.release();
```

Display `engine.messages()` in the simulation log. Each message includes a
severity, simulation time, code, and readable description.
