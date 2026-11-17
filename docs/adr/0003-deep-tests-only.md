# Deep tests only: test through ports, never internals

Every test exercises a Vertical Slice or an adapter only through its ports. Test doubles may replace an adapter at a port, but never anything inside a slice or an adapter. This also applies to maths that would be easy to unit-test on its own, such as the sensor fusion filter (tested only through the IMU adapter's port in an Adapter Test) and voltage-mode FOC (tested only through a Joint adapter's port). We chose this so that agents and humans can rewrite a slice's or an adapter's internals freely without touching any test. Tests stay coupled only to behaviour that someone actually relies on.

## Consequences

- A failing test points at a whole slice or adapter, not at a specific function. Failure messages therefore have to say what behaviour was wrong, in measurable terms, not merely "failed".
- An adapter that drives something physical, such as a Joint, is tested with simulated physics in lockstep, because its behaviour can't be observed through its port otherwise.
- "Missing unit tests" for internal maths are deliberate. Adding them is a reversal of this decision, not a cleanup.
