# ProjectBubbleLock

**Learning to build an embedded AI Software Factory, demonstrated through devices that keep a spirit-level bubble between the lines.**

ProjectBubbleLock is a project for learning how to build an embedded AI **Software Factory**: a setup where AI agents write embedded firmware and get automated feedback on it, with no physical hardware needed.

Terms in bold are defined in [GLOSSARY.md](GLOSSARY.md). Architectural decisions are recorded in [docs/adr](docs/adr).

## The devices

The first device is the **Stabilizer**, a handheld device with three **Joints**. Its **Payload** is a pair of spirit-level vials, the **Bubble Indicators**: one for forward/back **Tilt** and one for sideways Tilt. However you move the **Handle**, the Stabilizer keeps the Payload so level that neither bubble crosses its lines.

The bubbles are the visible proof and the spec. Each vial's rated sensitivity sets the **Tilt Limit** that every test checks against. Turning about the vertical axis doesn't move the bubbles, so there the Stabilizer uses **Yaw Follow**: the Payload turns with the Handle, but smoothly.

The second device, planned for later, is the **Robotic Arm**. It moves a Payload through 3D space and keeps it level with its own Joints.

## How the factory works

An agent takes one GitHub issue at a time, which delivers or grows one **Vertical Slice**. It loops on its own until every test level passes, and then a human reviews and merges the result. The agent never needs hardware, because everything the firmware touches is emulated or simulated.

- **The MCU and its chips.** The unmodified firmware runs on an emulated MCU ([Renode](https://renode.io)), with the MCU's peripherals and the external chips modelled at register level. Agents write the chip models, which makes this level of fidelity affordable. See [ADR-0001](docs/adr/0001-emulate-chips-at-register-level-in-lockstep-with-physics.md).
- **The physical world.** The Handle, the Joints and the **Payload Mount** are simulated in [MuJoCo](https://mujoco.org), in lockstep with the emulator. Runs are exact and repeatable, never flaky, and faster than real time.
- **Testing.** Every test is a deep test through a port, never a test of internals ([ADR-0003](docs/adr/0003-deep-tests-only.md)). There are four levels:
  - **Host Test:** a slice tested on the PC.
  - **Emulated Test:** the same tests run on the emulated CPU.
  - **Adapter Test:** an adapter tested against its emulated hardware, including simulated physics where the adapter drives something physical.
  - **World Test:** executable specifications run end to end.
- **Feedback.** When a World Test fails, its **Run Report** tells the agent in plain language what went wrong, when, and by how much.
