# ProjectBubbleLock

**Kinematic devices that lock the spirit-level bubble between the lines.**

ProjectBubbleLock stabilizes a payload that is normally left to gravity: a spirit level. A handheld rig counter-rotates its platform so the bubble stays inside the vial lines while the operator's hand moves. The same serial-chain kinematics and motor-control core is then reused as a robotic arm that can move freely while the bubble never crosses a line.

## Idea

A spirit-level vial is a mechanical inclinometer: the bubble drifts as soon as the platform tilts. ProjectBubbleLock closes that loop actively.

- **Sense** the platform's orientation in world space, with an IMU mounted on the level itself.
- **Solve** the joint angles that return the platform to level, using forward and inverse kinematics over the serial motor chain (yaw → roll → pitch), so a twist on one axis is accounted for in the others.
- **Drive** zero-backlash brushless gimbal motors with field-oriented control at a high loop rate.
- **Accept** the result by the vial, not the radian: the bubble stays between the lines across the specified motion envelope.

The tolerance is the product. "Level" is only the middle of the band the bubble has to remain inside.

## Two devices, one core

| | Handheld rig | Robotic arm |
| --- | --- | --- |
| Input | Measured handle motion | Commanded end-effector path |
| Kinematics | Inverse of the sensed handle rotation | Inverse kinematics to a target pose |
| Goal | Bubble stays between the lines | Bubble stays between the lines while the arm moves |
| Shared | Serial-chain math, joint encoders, FOC motor layer | Same |

## Roadmap

1. Single-axis stabilization slice with a measured acceptance threshold.
2. Sensor fusion and filtering so frame noise never reaches the control loop.
3. Three-axis cross-coupled kinematics holding the platform inside vial tolerance under compound motion.
4. Handheld rig running on real hardware.
5. Robotic-arm mode on the same kinematic and motor core.
