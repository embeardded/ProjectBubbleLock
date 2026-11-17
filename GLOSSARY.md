# ProjectBubbleLock

A project for learning how to build an embedded AI Software Factory. Its devices are demonstrated in person to win attention for the factory.

## Language

### Project

**ProjectBubbleLock**:
The whole project: the Software Factory plus the devices it produces.
_Avoid_: BubbleLock

**Software Factory**:
The setup that has AI agents write embedded firmware and gives them automated, hardware-free feedback on it. It is the main thing this project exists to learn, and it is not tied to any one device.
_Avoid_: AI factory, pipeline, harness

**Showcase**:
An in-person demonstration of one of the project's physical devices, used to open a conversation about the Software Factory.
_Avoid_: Demo video

### Firmware architecture

**Firmware**:
The program that runs on a device's MCU, made of its Vertical Slices, adapters and Registry. It is what ships on the device.
_Avoid_: Application, app, binary, image

**Feature**:
A named behaviour of a device, such as Hold Level or Yaw Follow, delivered by one or more Vertical Slices.
_Avoid_: Module, component, behaviour (as a term)

**Vertical Slice**:
One use case of the Firmware, such as Hold Level: pitch, built and delivered end to end and accepted by an automated test. Every slice belongs to exactly one Feature. Slices share data only through the Registry.
_Avoid_: Layer, module, milestone

**Registry**:
The firmware's mechanism for sharing both state and events between Vertical Slices. It is the only way slices share data.
_Avoid_: Blackboard, global state, data bus

**Telemetry**:
A device's live stream of internal values to a PC, for plotting and for comparing real runs with simulated ones.
_Avoid_: Logging, debug output

### Testing

**Host Test**:
A deep test of one Vertical Slice through its ports, with the adapters replaced by test doubles, built for and run on the development PC. It never tests a slice's internals.
_Avoid_: Unit test, native test, PC test

**Emulated Test**:
The Host Tests, built for the MCU and run on the emulated CPU.
_Avoid_: Target test, on-target test

**Adapter Test**:
A deep test of one adapter through its port, running on the emulated CPU against emulated hardware: the MCU's own peripherals, any external chip, and simulated physics when the adapter drives something physical. It is driven and checked from outside the firmware and never tests the adapter's internals.
_Avoid_: Driver test, peripheral test

**World Test**:
An executable specification run end to end against emulated chips and simulated physics.
_Avoid_: Simulation test, integration test, end-to-end test

**Handle Motion**:
A named, repeatable movement of the Handle used to disturb the Stabilizer in a World Test, such as "walking" or "sharp twist".
_Avoid_: Disturbance, trajectory, motion profile

**Run Report**:
The recorded trace of a World Test and, when it fails, a short plain-language summary of what went wrong, written for the agent.
_Avoid_: Test log, test output

### Levelling

**Payload**:
The set of Bubble Indicators that a device keeps level. Nothing else is part of it.
_Avoid_: Camera, camera tray, platform

**Bubble Indicator**:
One of the two physical tube vials that make up the Payload, one per Tilt direction. It is the visible proof, and the spec, that the Payload is level.
_Avoid_: Spirit level, bubble level

**Tilt**:
The Payload's rotation away from level. It is a pair: pitch Tilt (forward/back) and roll Tilt (sideways), each judged by its own Bubble Indicator. It is the only motion the Bubble Indicators show.
_Avoid_: Angle, attitude

**Tilt Limit**:
The largest Tilt allowed in each direction: the point where that Bubble Indicator's bubble reaches its line, set by the vial's rated sensitivity.
_Avoid_: Tolerance, threshold

**Joint**:
A motorized axis of a device: a motor and its encoder, which go to a commanded angle and report their actual angle.
_Avoid_: Axis (when you mean the hardware), motor

**Payload Mount**:
The part of a device that carries the Payload and the device's IMU, positioned by the device's last Joint.
_Avoid_: Platform, tray, end effector

### Devices

**Stabilizer**:
The first device: a handheld device with three Joints that keeps its Payload so level that the bubbles of its Bubble Indicators never cross their lines. Its chain is Handle → Yaw Joint → Roll Joint → Pitch Joint → Payload Mount.
_Avoid_: BubbleLock, gimbal, stabilizer rig

**Yaw Joint**:
The Stabilizer's Joint nearest the Handle. It turns everything after it about the Handle's vertical axis.
_Avoid_: Pan motor

**Roll Joint**:
The Stabilizer's middle Joint, between the Yaw Joint and the Pitch Joint. It corrects sideways Tilt.
_Avoid_: Roll motor

**Pitch Joint**:
The Stabilizer's last Joint. It carries the Payload Mount and corrects forward/back Tilt.
_Avoid_: Tilt motor, pitch motor

**Handle**:
The part of the Stabilizer the user holds and moves. The Stabilizer cancels its motion.
_Avoid_: Grip, base

**Hold Level**:
The Stabilizer's main behaviour: keeping both Tilts within their Tilt Limits while the Handle moves.
_Avoid_: Horizon lock, stabilization

**Yaw Follow**:
The Stabilizer's behaviour for turning about the vertical axis: the Payload turns with the Handle, but smoothly.
_Avoid_: Pan mode, heading lock

**Robotic Arm**:
The second device, planned for later and separate from the Stabilizer. It moves a Payload through 3D space and keeps it level with its own Joints, without the bubbles crossing their lines.
_Avoid_: Arm extension, arm mode
