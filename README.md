# ProjectBubbleLock

**Kinematic devices that lock the spirit-level bubble between the lines.**

ProjectBubbleLock stabilizes a spirit level. A handheld rig holds the level platform at world level while the user moves the handle, so the bubble stays between the vial lines. The same joint chain, encoders, and motor control later drive a robotic arm that moves across its envelope while the bubble stays inside the lines.

## How it works

A spirit-level vial is an inclinometer. The bubble drifts as soon as the platform tilts. ProjectBubbleLock measures that tilt and cancels it with motors.

An IMU on the level reports the platform orientation in world space. The kinematics solver inverts the measured handle rotation across the serial chain (yaw, roll, pitch), so a movement on one axis is corrected on the other two. Field-oriented control holds each motor at its target angle without backlash. The orientation loop is planned at 200 Hz and the motor loop at 20 kHz.

A rig passes when the bubble stays between the lines across a defined motion envelope. Reaching exact level is not required.

## Two devices, one core

| | Handheld rig | Robotic arm |
| --- | --- | --- |
| Input | Measured handle motion | Commanded end-effector path |
| Kinematics | Inverse of the sensed handle rotation | Inverse kinematics to a target pose |
| Pass condition | Bubble stays between the lines | Bubble stays between the lines while the arm moves |
| Shared | Serial-chain math, joint encoders, FOC motor layer | The same code |

## Roadmap

1. Stabilize one axis in simulation and measure the error.
2. Add sensor fusion so vibration in the measured angle does not reach the control loop.
3. Stabilize three axes together, with the platform staying inside vial tolerance under compound motion.
4. Build the handheld rig and run it on real hardware.
5. Run the same kinematics and motor code in robotic-arm mode.
