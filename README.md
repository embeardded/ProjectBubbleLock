# ProjectBubbleLock

**Kinematic devices that lock the spirit-level bubble between the lines — handheld rig and future robotic arm.**

ProjectBubbleLock stabilizes a spirit level. A handheld rig holds the level platform at world level while the user moves the handle, so the bubble stays between the vial lines. The same joint chain, encoders, and motor control later drive a robotic arm. The arm moves anywhere in its envelope and the bubble still stays inside the lines.

## How it works

A spirit level is an inclinometer. Its bubble moves as soon as the platform tilts. ProjectBubbleLock measures that tilt and cancels it with motors.

An IMU on the level reports the platform orientation in world space. The kinematics solver inverts the measured handle rotation over the serial chain (yaw, roll, pitch), so a movement on one axis is corrected on the other two. Field-oriented control holds each motor at its target angle without backlash. The orientation loop is planned at 200 Hz and the motor loop at 20 kHz.

A rig passes when the bubble stays between the lines across a defined motion envelope. Level sits in the middle of that band. The band is the pass condition.

## Status

The project is in design and simulation planning. No firmware or hardware is in the repository yet, and the layout will change once the first working slice exists.

## Roadmap

1. Stabilize one axis in simulation and measure the error.
2. Add sensor fusion so vibration in the measured angle does not reach the control loop.
3. Stabilize all three axes together and stay inside vial tolerance under compound motion.
4. Build the handheld rig and run it on real hardware.
5. Reuse the kinematics and motor code in robotic-arm mode.
