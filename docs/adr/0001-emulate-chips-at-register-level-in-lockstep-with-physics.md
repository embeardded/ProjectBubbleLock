# Emulate the MCU and its external chips at register level, in lockstep with a physics simulation

The Software Factory needs to give AI agents fast, automated feedback that doesn't depend on physical hardware. We decided to run the unmodified Firmware on an emulated MCU, with every external chip (IMU, encoders) modelled at the bus and register level. That emulator runs in lockstep with a physics simulation of the Handle, motors, Joints and Payload. Most teams stop at host-side unit tests or mock the hardware abstraction layer instead, because writing peripheral and chip models by hand is tedious. In this project, AI agents write those models, which makes the cost small. In return, drivers, interrupt timing and control code are all tested on the same instruction set and registers they will meet on the real chip.

## Considered Options

- **QEMU:** rejected. It emulates the CPU core but has almost no STM32 peripheral models. It also runs on the host's wall clock, so a loop coupled to a physics sim would be non-deterministic and flaky.
- **Renode:** chosen. It has STM32F4 peripheral models and deterministic virtual time, and custom chip models can be written in C# and loaded at runtime. Its External Control API lets an outside process step time and read or write peripherals, which is what lockstep needs.
- **Webots:** rejected as the physics side. It wants to own the main loop, and its 3D visuals add nothing because the Showcase is the physical device.
- **MuJoCo:** chosen as the physics side. It is deterministic, fast, and can be stepped from a script.

## Consequences

- Hardware choices are constrained by what Renode can model. In particular, Renode's STM32 timers do not model complementary outputs or dead-time, so only motor drivers with one PWM input per phase are allowed.
- Renode cannot model timer-triggered or injected ADC sampling, so current sensing cannot be emulated. Motor control therefore starts voltage-mode.
- No ready-made Renode↔physics bridge exists, so the project owns that bridge.
