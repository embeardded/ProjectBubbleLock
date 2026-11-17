# Hexagonal firmware core with Vertical Slices, shared adapters and a Registry

The firmware uses a hexagonal architecture rather than the usual layered structure of drivers, then a hardware abstraction layer, then the application. The core holds the use cases, and each Vertical Slice is one of them. All hardware sits behind ports, implemented by adapters that every slice shares. Slices share data only through the Registry and never call each other.

Ports speak domain language, and the chip-specific work sits inside the adapters, not in the core:
- **Sensor fusion** (e.g. a Kalman filter) is inside the IMU adapter, so the IMU port reports the Payload's orientation.
- **The chip-specific half of calibration** is also in the IMU adapter. The calibration *procedure* is a core use case.
- **Voltage-mode FOC, encoder reading and the per-Joint position loop** are inside each Joint adapter, so a Joint's port is just "go to this angle" and "here is your angle".

We chose this so a core use case can be built and tested without knowing which chips are fitted. It also means an IMU that does its own fusion on the chip could replace the current one with no change to the core.

## Consequences

- The fast motor-control loop (20 kHz) never enters the core. The core only sees Joint angles.
- A sensor fusion filter is tested through its adapter's tests, not through core unit tests. Moving it into the core would be a deliberate reversal of this decision, not a cleanup.

## Amendment: slices of one Feature may share internal code

Vertical Slices are grouped into Features, such as Hold Level, made of the slices Hold Level: pitch and Hold Level: roll. Slices of the same Feature may share internal code, such as a control law that both use. Slices of different Features never share code; they share only through the Registry and the shared adapters. Slices still never call each other.

- Shared internal code is tested only through the slices that use it, through their ports (ADR-0003).
- Emulated Tests are built as one image per Feature, so a Feature's shared code is built once with its slices' tests.
