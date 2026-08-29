# Autonomous Rocket Project — Technical Design Document

**Document ID:** ARP-TDD-001
**Version:** 1.0
**Status:** DRAFT — Living Document
**Classification:** Personal Engineering Program / Portfolio Project
**Program Horizon:** ~3.5 years from document date
**Date:** 2026-08-29

---

## 0. Document Control

### 0.0 Author's Note

> All of the ideas in this document are mine, as I get more ideas I store them in an LLM. I used ChatGPT for all of my project content and fed it to Claude. I want to be open and honest about this. This document was created entirely with Claude, but I created and set the parameters, and all of the ideas are mine.

This note is preserved verbatim (light typo correction only) because provenance matters in an engineering record. For clarity going forward: the program lead originated and owns every engineering idea, constraint, and decision in this document (the propulsion constraints, the landing architecture, the GNC loop concept, the phased roadmap philosophy, the current hardware status, etc.). ChatGPT was used as a scratchpad/idea-capture tool for the program lead's own thinking; Claude was used to organize that material into this structured document format, apply the status taxonomy (Section 0.2), and add standard aerospace-engineering framing (equations, verification hierarchy, requirement/risk tables) around the program lead's ideas. Claude did not originate the mission concept, the propulsion constraints, or the architectural decisions — it drafted the document, it did not design the rocket.

### 0.1 Purpose of This Document

This document is the engineering source of truth for an independent, long-horizon autonomous rocket development program. It exists to force explicit, written engineering reasoning at a stage where it would otherwise be easy to hand-wave — before hardware commitments, before code architecture calcifies, and before "it'll probably work" substitutes for analysis.

This is **Version 1.0**, the first version the program lead has reviewed and formally accepted as the baseline record (superseding the 0.1 internal draft). It describes a program that is almost entirely in the conceptual and foundational-learning stage. Treat every design statement here as provisional unless explicitly marked `DECIDED`. Most of the document is `PROPOSED`, `CONCEPTUAL`, or `OPEN` — version number reflects document maturity/ownership, not program/engineering maturity. See Section 36.1 for the actual state of the engineering work.

### 0.2 Status Taxonomy

Every non-trivial claim in this document is tagged with one of the following. This tagging is the single most important convention in the document — do not remove it in future revisions without replacing it with something equally explicit.

| Tag | Meaning |
|---|---|
| `REQUIREMENT` | A binding constraint the system must satisfy. Requirements can still be revised, but revision must be deliberate and logged. |
| `DECIDED` | A design decision that has been made and is currently load-bearing for other work. Still revisable, but changing it has ripple effects. |
| `PROPOSED` | The current best design idea. Not yet validated by analysis, simulation, or test. Default state for most of this document. |
| `EXPERIMENTAL` | An idea being explored specifically to learn whether it's viable. Failure is an acceptable and expected outcome. |
| `STRETCH` | A future/long-term goal, explicitly not required for program success. |
| `OPEN` | A question requiring research, analysis, or experiment before it can be resolved. |
| `ASSUMPTION` | A statement taken as true for planning purposes, not yet verified. |
| `RISK` | A specific way the program could fail, degrade, or be delayed. |
| `TBD` | A parameter or value that has not been determined. No number should be invented for these. |

### 0.3 Revision History

| Rev | Date | Change | Author |
|---|---|---|---|
| 0.1 | 2026-08-29 | Initial baseline document created from program-defining conversation | Program lead (user) + Claude (drafting assistant) |
| 1.0 | 2026-08-29 | Added author's note on provenance (Section 0.0: ideas are the program lead's, captured via LLM tools including ChatGPT, drafted into this document format by Claude); accepted as formal v1 baseline | Program lead (user) |

### 0.4 Companion Documents (Planned, Not Yet Created)

- ARP-REQ — Formal requirements register (this doc's Section 4 is the seed)
- ARP-RISK — Risk register (Section 31 is the seed)
- ARP-ICD-* — Interface control documents (sensor bus, actuator bus, power)
- ARP-TP-001 — Technical paper (Section 34)
- Flight test reports, one per test article/flight
- ADRs (Architecture Decision Records) for reversing any `DECIDED` item

---

## 1. Executive Summary

This document defines the initial engineering baseline for a personally-run autonomous rocket program. The long-term objective is to design, build, simulate, and eventually flight-test a rocket capable of estimating its own state in real time, predicting its landing point, and using thrust-vector control (TVC) and actuated aerodynamic fins to steer itself to a controlled powered landing at a predefined target — without a conventional parachute recovery architecture.

The program is explicitly structured as a multi-year, first-principles engineering effort, not a single build. It proceeds through mathematics and physics foundations, simulation software, avionics prototyping, state estimation, guidance, control, actuation, and only then to integrated and flight testing. The propulsion architecture is constrained to **single-ignition, fixed-thrust, non-throttleable, non-restartable motors** — a hard constraint that shapes every downstream GNC (Guidance, Navigation, and Control) decision, because the vehicle cannot modulate thrust magnitude and must do essentially everything through thrust *direction* (TVC) and aerodynamic control.

As of this revision, essentially nothing is flight-proven. Two sensors (BMP388 barometer, ICM42688P IMU) have been individually bench-tested on an ESP32 development board; microSD logging is unreliable; no simulator, estimator, guidance algorithm, controller, TVC mechanism, or fin mechanism yet exists. The mathematical foundation (differential equations, linear algebra, mechanics, controls) is largely ahead of the program lead rather than behind. This document should be read with that reality in front of it — it is a plan for building those things, not a description of things that exist.

The core engineering thesis of the program is captured in one loop, repeated throughout this document:

```
Sensors → State Estimation → Trajectory Prediction → Guidance → Control → Actuators (TVC + Fins) → Vehicle → Sensors (repeat)
```

Every subsystem in this document exists to serve one link in that loop.

---

## 2. Mission Definition

### 2.1 Mission Statement

Develop an autonomous flight vehicle that launches under solid/fixed-thrust rocket propulsion, estimates its own flight state from onboard sensors, predicts where it will land if no action is taken, compares that prediction to a predefined landing target, and uses closed-loop guidance and control (via TVC and actuated fins) to reduce landing error, ultimately achieving a controlled powered touchdown near the target.

### 2.2 Mission Phases (Conceptual, `PROPOSED`)

| Phase | Description | Primary Control Authority |
|---|---|---|
| Pad / Ignition | Motor ignition, rail/tower departure | None (open-loop stability only) |
| Powered Ascent | Boost under fixed thrust | TVC (attitude/stability), fins (secondary) |
| Coast / Apogee | Unpowered ballistic arc (if ascent motor(s) burn out before apogee) | Fins only (no thrust available) |
| Descent (Unpowered) | Vehicle falls, possibly decelerating aerodynamically | Fins (stabilization, drag shaping) |
| Terminal Powered Descent / Landing Burn | Landing motor ignites; vehicle steers toward target | TVC (primary), fins (secondary) |
| Touchdown | Final contact with ground | Passive structure (legs/skid — `TBD`, not yet designed) |

**`OPEN`:** With fixed, non-throttleable, single-ignition motors, the timing of landing-motor ignition relative to altitude/velocity is a hard, unforgiving problem — there is no "try again" if ignition timing is wrong. This is one of the central unsolved problems of the whole program and is revisited throughout (see Sections 7, 9, 20, 32).

### 2.3 Mission Success Criteria

No mission success criteria are finalized (`TBD`). Success will be defined incrementally per test/flight (Section 23, Section 29-equivalent roadmap phases), not as a single all-or-nothing "lands autonomously" criterion. Early flights succeed by answering specific engineering questions (Section 25.5), not by landing.

### 2.4 Explicitly Out of Scope (Current Baseline)

- Throttleable, restartable, or pulsed propulsion (`REQUIREMENT`, see Section 3)
- Belly-flop / high-AoA aerodynamic reentry maneuvers (`DECIDED` — discarded, see Section 2.5)
- Parachute-based primary recovery as the landing mechanism (though a parachute may still be retained as a **safety/abort** recovery method — `OPEN`, see Section 24)
- Fins functioning as landing legs (`DECIDED` — discarded)
- Machine learning as the foundation of the initial GNC system (`REQUIREMENT`, see Section 21)

### 2.5 Design History Note

An earlier concept considered a Starship-like belly-flop descent with a late flip maneuver. This has been **discarded** in favor of a mostly-vertical descent with trajectory correction, because a belly-flop architecture demands aerodynamic control authority, structural loading, and flip-maneuver GNC complexity far beyond what a fixed-thrust, small-scale, early-stage program can responsibly attempt. This is recorded here so future revisions don't silently re-litigate it without acknowledging why it was dropped.

---

## 3. Project Objectives

Objectives are grouped by horizon. Each has an informal ID for future cross-referencing.

| ID | Objective | Horizon | Type |
|---|---|---|---|
| OBJ-01 | Build first-principles understanding of the math/physics underlying rocket GNC | Year 1 | `REQUIREMENT` |
| OBJ-02 | Build a working custom rocket flight simulator with sensor and actuator models | Year 1 | `REQUIREMENT` |
| OBJ-03 | Build and validate individual avionics subsystems (IMU, baro, logging, power) | Year 1 | `REQUIREMENT` |
| OBJ-04 | Implement a working state estimator (EKF or equivalent) validated against simulated truth | Year 1-2 | `REQUIREMENT` |
| OBJ-05 | Implement basic guidance (landing-point prediction + error correction) | Year 2 | `REQUIREMENT` |
| OBJ-06 | Implement feedback control (PID baseline) for attitude/rate control | Year 2 | `REQUIREMENT` |
| OBJ-07 | Design, build, and characterize a TVC mechanism | Year 2 | `REQUIREMENT` |
| OBJ-08 | Design, build, and characterize actuated fins | Year 2 | `REQUIREMENT` |
| OBJ-09 | Validate the full GNC loop in simulation, including Monte Carlo robustness testing | Year 2-3 | `REQUIREMENT` |
| OBJ-10 | Conduct hardware-in-the-loop (HIL) testing of the flight computer | Year 2-3 | `REQUIREMENT` |
| OBJ-11 | Conduct progressively complex physical/flight tests | Year 3 | `REQUIREMENT` |
| OBJ-12 | Achieve an integrated autonomous flight demonstrating closed-loop GNC | Year 3-3.5 | `REQUIREMENT` (stretch on *landing accuracy* specifically, see OBJ-13) |
| OBJ-13 | Achieve a controlled powered landing at a predefined target within some accuracy bound (`TBD`) | End of program / beyond | `STRETCH` |
| OBJ-14 | Produce a formal technical paper documenting the program | Ongoing → final | `REQUIREMENT` (as a process), `STRETCH` (as a "complete" paper) |
| OBJ-15 | Explore hybrid physics+ML augmentation of the GNC system | Post Year-2 | `STRETCH` |

**Note on OBJ-13:** This document explicitly refuses to treat "lands autonomously on target" as a Year-1/Year-2 requirement. It is the long-run mission concept (Section 2), but making it a near-term *objective* would distort prioritization and encourage skipping the foundational work in Sections 25-30. The near-term objectives (OBJ-01 through OBJ-10) are the actual gating path.

---

## 4. Requirements

This is a seed requirements register, not a complete one. Requirements will be added/refined as design work matures. IDs use the prefix `REQ-<domain>-<number>`.

### 4.1 Propulsion Requirements

| ID | Requirement | Rationale | Status |
|---|---|---|---|
| REQ-PROP-01 | The vehicle SHALL use single-ignition, fixed-thrust motors only. | Program constraint — see Section 3 of source discussion; avoids scope creep into custom liquid/hybrid throttle systems the program cannot responsibly build early. | `REQUIREMENT`, `DECIDED` |
| REQ-PROP-02 | The vehicle SHALL NOT rely on motor restart capability. | Same as above. | `REQUIREMENT`, `DECIDED` |
| REQ-PROP-03 | The vehicle SHALL NOT rely on pulsed or variable thrust magnitude. | Same as above. | `REQUIREMENT`, `DECIDED` |
| REQ-PROP-04 | The vehicle SHALL NOT rely on multi-engine differential throttling for control. | Differential *throttle* is disallowed; differential *gimbal/TVC* across multiple fixed-thrust motors is a separate, still-open question (see REQ-PROP-05). | `REQUIREMENT`, `DECIDED` |
| REQ-PROP-05 | Whether the vehicle uses a single landing motor, multiple motors, or a dedicated ascent/landing motor split SHALL be determined by trade study before hardware procurement. | The "two outer ascent + one central landing motor" idea is a concept, not a decision. | `OPEN` |

### 4.2 GNC / Control Requirements

| ID | Requirement | Rationale | Status |
|---|---|---|---|
| REQ-GNC-01 | The flight computer SHALL maintain a continuously updated estimate of position, velocity, attitude, and angular rate during powered flight. | Core enabler for every downstream function. | `REQUIREMENT` |
| REQ-GNC-02 | The system SHALL predict a landing point from the current state estimate at a rate sufficient to close the guidance loop (rate `TBD`, likely 10-100 Hz class, pending control-bandwidth analysis). | Needed for guidance. | `REQUIREMENT`, rate `TBD` |
| REQ-GNC-03 | The system SHALL command TVC and fin actuators from a feedback controller, not open-loop schedules, once past the earliest test flights. | Defines "autonomous" for this program. | `REQUIREMENT` |
| REQ-GNC-04 | Guidance and control SHALL be architected as separate software/algorithmic layers (guidance produces trajectory/attitude *targets*; control produces actuator *commands*). | Explicit architectural separation requested; keeps the system debuggable. | `REQUIREMENT`, `DECIDED` |
| REQ-GNC-05 | The initial controller implementation SHALL be a PID-class controller before any more advanced method is attempted. | Testability, explainability, matches program's incremental-complexity philosophy. | `REQUIREMENT`, `DECIDED` |

### 4.3 Avionics Requirements

| ID | Requirement | Rationale | Status |
|---|---|---|---|
| REQ-AV-01 | The flight computer SHALL log raw sensor data, estimated state, and commanded actuator outputs at a rate sufficient for post-flight reconstruction (`TBD` Hz). | Section 22. | `REQUIREMENT` |
| REQ-AV-02 | The current ESP32-WROOM-32 based prototype platform is a learning/development platform ONLY and SHALL NOT be assumed as the final flight computer without an explicit trade study (compute, RAM, real-time behavior, redundancy). | Section 17. | `REQUIREMENT`, `DECIDED` (as a constraint on process, not on final hardware) |
| REQ-AV-03 | Data logging SHALL be demonstrated reliable (no dropped/corrupted sessions across N consecutive power cycles, N `TBD`) before it is trusted for any flight test. | Current SD logging has known intermittent init issues. | `REQUIREMENT` |

### 4.4 Safety Requirements

| ID | Requirement | Rationale | Status |
|---|---|---|---|
| REQ-SAFE-01 | Every flight test SHALL have a defined abort/failsafe behavior reachable from every flight phase. | Section 24. | `REQUIREMENT` |
| REQ-SAFE-02 | The vehicle SHALL fail to a passive, ballistic-safe, or recovery-deployed state on loss of state-estimate confidence, actuator fault, or software fault — not continue attempting closed-loop control on bad data. | Autonomy is not automatically safe (explicit program principle). | `REQUIREMENT` |
| REQ-SAFE-03 | Flight testing SHALL comply with applicable model/high-power rocketry regulations and launch-site rules for the jurisdiction in effect at test time (`TBD` — depends on rocket total impulse class, which is undetermined). | Legal/regulatory. | `REQUIREMENT`, details `TBD` |

### 4.5 Requirements Explicitly Not Yet Written

Structural requirements, mass/CG budget requirements, specific accuracy/precision requirements for landing, specific control-loop rate requirements, and specific safety-margin requirements all require analysis this document does not yet contain. Do not infer numeric requirements from examples used elsewhere in this document (e.g., "10-100 Hz") — those are illustrative ranges, not requirements, until an actual control-bandwidth analysis is done (Section 32).

---

## 5. System Architecture

### 5.1 Top-Level Control Loop

```
        ┌────────────────────────────────────────────────────────────┐
        │                                                              │
        ▼                                                              │
   [ SENSORS ]  ──▶  [ STATE ESTIMATOR ]  ──▶  [ TRAJECTORY PREDICTOR ] │
  IMU, Baro, GPS         (EKF-class)          (uses vehicle/env model) │
  Heading, (future)                                    │               │
                                                        ▼               │
                                              [ GUIDANCE ]              │
                                        (target vs. predicted state)   │
                                                        │               │
                                                        ▼               │
                                              [ CONTROLLER ]            │
                                           (PID → advanced later)      │
                                                        │               │
                                                        ▼               │
                                          [ ACTUATORS: TVC + FINS ]     │
                                                        │               │
                                                        ▼               │
                                              [ VEHICLE DYNAMICS ]  ────┘
                                          (responds; new true state)
```

This loop is identical in concept whether it is running in pure software simulation, in hardware-in-the-loop (real flight computer + simulated sensors/dynamics), or in real flight (real flight computer + real sensors + real vehicle). That equivalence is a `DECIDED` architectural principle (Section 12) — the GNC software should not be rewritten between stages.

### 5.2 Architectural Layers

| Layer | Responsibility | Answers the question |
|---|---|---|
| Sensing | Acquire raw physical measurements | "What did the sensors just read?" |
| Estimation | Fuse noisy/biased measurements into a best-estimate state | "What do I believe my state actually is?" |
| Prediction | Propagate the estimated state forward using a vehicle/environment model | "What will happen if I do nothing?" |
| Guidance | Compare predicted outcome to mission target, generate a desired trajectory/attitude | "What should happen instead?" |
| Control | Convert guidance targets into actuator commands | "What do I move, and how much?" |
| Actuation | Physically move TVC/fins | "Make it so." |
| Vehicle/Environment | Physical (or simulated) response | "What actually happened?" |

`REQUIREMENT` / `DECIDED`: Guidance and Control remain architecturally distinct modules with a defined interface (a "guidance target" struct: desired attitude, desired trajectory angle, desired lateral acceleration, etc. — exact contents `TBD`), even in the earliest implementations, so that later replacing either module doesn't require rewriting the other.

### 5.3 Data Flow Interfaces (`PROPOSED`, not yet formally specified)

| Interface | From → To | Contents (`PROPOSED`, non-final) |
|---|---|---|
| I-SENS | Sensors → Estimator | Raw/timestamped measurements, per-sensor validity flags |
| I-EST | Estimator → Predictor, Guidance | State estimate vector + covariance |
| I-PRED | Predictor → Guidance | Predicted landing point, predicted landing velocity, time-to-event estimates, uncertainty bounds |
| I-GUID | Guidance → Control | Desired attitude/trajectory angle, desired lateral acceleration, constraint flags |
| I-CTRL | Control → Actuators | TVC gimbal angle command(s), fin deflection command(s) |
| I-LOG | All layers → Data Logger | Timestamped copies of all of the above (Section 22) |

### 5.4 Architecture Risks

- **`RISK`**: Defining these interfaces too rigidly before any module exists could force premature design decisions. Mitigation: treat interface structs as versioned and expect churn through at least Phase 3-4 (Section 25).
- **`RISK`**: A single-threaded microcontroller loop conflating sensing, estimation, guidance, control, and logging without a real-time scheduling discipline is a common source of avionics software failure. This must be explicitly designed, not defaulted into (Section 17.4).

---

## 6. Vehicle Architecture

### 6.1 Current Status

No airframe, motor configuration, mass budget, or geometry has been finalized. This section defines the *shape* of decisions to be made, not the decisions themselves.

### 6.2 Key Vehicle Parameters (all `TBD` pending design)

| Parameter | Symbol | Status |
|---|---|---|
| Total mass (wet) | $m_0$ | `TBD` |
| Total mass (dry/landing) | $m_f$ | `TBD` |
| Length | $L$ | `TBD` |
| Diameter | $d$ | `TBD` |
| Center of mass (time-varying, propellant-dependent) | $x_{cg}(t)$ | `TBD`, `OPEN` — requires mass-properties model |
| Center of pressure | $x_{cp}$ | `TBD`, `OPEN` — requires aero modeling (e.g., Barrowman method or CFD/RocketPy) |
| Moment of inertia (roll, pitch, yaw) | $I_{xx}, I_{yy}, I_{zz}$ | `TBD` |
| Static margin | $x_{cp} - x_{cg}$, in calibers | `TBD`, `OPEN` — a key stability parameter that changes throughout flight as propellant burns |

### 6.3 Static Margin and Controllability Tension (`OPEN`, important)

A rocket with large positive static margin (CP well aft of CG) is passively stable but harder to actively steer — the same aerodynamic restoring force that makes it stable also fights the controller. A rocket with small or negative static margin is easier to actively steer but passively unstable, and depends entirely on the control system working correctly at all times. This is a classic aerospace tension, and it is **not resolved** in this document. It must be analyzed once a candidate airframe/motor configuration exists (Section 32, open question list).

### 6.4 Multi-Motor Configuration Concept (`CONCEPTUAL`)

A configuration with two outer ascent motors and one central landing motor has been discussed. This is recorded as a **concept requiring trade-study analysis**, not a decision. Concerns already identified:

- Asymmetric thrust if the two outer motors do not ignite/burn identically (`RISK`)
- CG shift as different motors burn out at different times (`RISK`)
- Structural/mounting complexity of three motor wells vs. one (`RISK`)
- Whether the "ascent motors burn out, coast, then landing motor ignites" sequence is even the right one, versus (e.g.) a single motor sized to do both with a computed ignition delay — this is unresolved (`OPEN`)

**`OPEN` — Alternative not yet fully explored:** a single fixed-thrust motor for the entire flight, with the landing maneuver executed using whatever thrust/deceleration is available near the end of burn, or via a second, smaller dedicated landing motor mounted separately rather than "two ascent + one landing." No trade study has been done comparing these options on mass, complexity, and control authority.

### 6.5 Recovery/Structure at Touchdown (`OPEN`)

Whether the vehicle lands on legs, a skid, or another structure is undecided. Fins are explicitly **not** intended to double as landing legs (Section 9). This is an open mechanical design problem for later phases.

---

## 7. Propulsion Architecture

### 7.1 Hard Constraints (repeated from Section 4.1 for locality)

`REQUIREMENT`, `DECIDED`: Single-ignition, fixed-thrust, non-throttleable, non-restartable, non-pulsed motors. No multi-engine differential throttling. These constraints are not to be silently redesigned around in any future revision of this document — if they change, that must be a logged, deliberate decision with rationale.

### 7.2 Why This Constraint Matters

Conventional powered-landing systems (e.g., orbital-class reusable boosters) rely heavily on deep throttling to null out vertical velocity precisely at touchdown. This program cannot do that. Instead, the vehicle must solve the landing problem using:

1. **Precise ignition timing** of a fixed-thrust burn, so that the burn's fixed impulse profile is consumed at approximately the right altitude/velocity combination, and
2. **Thrust *direction* control (TVC)** to manage horizontal position/velocity and attitude, and
3. **Aerodynamic control (fins)** for stabilization and secondary trajectory shaping.

This is a substantially harder guidance problem than a throttleable vehicle faces, because one major control lever (thrust magnitude vs. time) is simply unavailable. This should be treated as a defining engineering challenge of the whole program, not a minor footnote.

### 7.3 Propulsion Modeling Requirements for Simulation

To simulate propulsion faithfully (Section 11, 12), the following must eventually be characterized per candidate motor:

| Parameter | Notes |
|---|---|
| Thrust curve $F(t)$ | From manufacturer data or static test stand measurement |
| Total impulse | $\int F(t)\,dt$ |
| Burn time | |
| Propellant mass vs. time $m_p(t)$ | Needed for time-varying CG/inertia |
| Motor (casing) mass, empty and loaded | |
| Thrust misalignment / thrust vector offset tolerance | Manufacturing tolerance; affects disturbance torque model (Section 14 uncertainty) |
| Ignition delay / ignition reliability statistics | Affects ignition-timing risk analysis |

**`ASSUMPTION`:** Early simulation and analysis will likely use published commercial hobby/mid-power motor thrust-curve data (e.g., from a public thrust-curve database) as placeholders, not a custom-built motor, since motor development is out of scope for the near-term phases of this program (`OPEN` whether custom propulsion is ever in scope — not currently assumed).

### 7.4 Open Propulsion Questions

- REQ-PROP-05's trade study (Section 4.1) — single vs. multi-motor
- Ignition-timing-error sensitivity: how much does a small ignition-timing error affect landing accuracy, given no throttle to compensate? (`OPEN`, ties to Section 20 Monte Carlo work)
- What is an acceptable/available thrust-to-weight ratio for the landing burn, given the vehicle will be much lighter (propellant spent) than at liftoff? (`OPEN`)

---

## 8. Aerodynamic Control Architecture (Actuated Fins)

### 8.1 Role Definition (`DECIDED`)

Actuated fins are a **secondary/supplemental** control and stabilization mechanism. TVC is primary during powered flight (Section 9). Fins are the *only* control authority during any unpowered phase (coast, unpowered descent), which makes them important for stability even though they are secondary overall. Fins are explicitly **not** a landing-leg structure and **not** the primary landing-steering mechanism (both `DECIDED`, reversing earlier concepts).

### 8.2 Functional Roles

| Role | Description | Flight Phase |
|---|---|---|
| Passive-assist stabilization | Even un-actuated, fins contribute to static margin/aerodynamic stability | All powered/unpowered phases with airspeed |
| Active stabilization | Actuated deflection damps unwanted rotation | All phases with sufficient dynamic pressure |
| Attitude control | Commanded deflection to achieve a target attitude | Coast/descent, and as a TVC-assist during powered flight |
| Secondary trajectory correction | Small trim corrections to descent trajectory | Descent, landing approach |

### 8.3 Design Problems Requiring Future Analysis (all `OPEN`)

- Aerodynamic force/moment generation as a function of deflection angle, airspeed, and angle of attack
- Servo/actuator torque and speed requirements (must overcome hinge moment at max expected dynamic pressure)
- Mechanical loads and attachment structure sizing
- Response time / bandwidth of the fin-actuator system relative to required control bandwidth (Section 18.4)
- Flutter (aeroelastic instability) — a known failure mode for control surfaces at sufficient speed; must be checked once geometry/speed regime is known
- Low-speed effectiveness: fins produce very little force at low dynamic pressure (e.g., near apogee, or during a slow terminal descent) — this is a fundamental limitation to characterize, not assume away
- High-speed structural loads during ascent
- Failure modes: actuator jam, mechanical linkage failure, one fin failing asymmetrically vs. all fins failing

### 8.4 Interface to Control System

Fins are commanded by the Control layer (Section 18) as a deflection angle per fin (`TBD` number of fins — commonly 3 or 4 in amateur/experimental rocketry, `OPEN` which is used here). Fin geometry, count, and control allocation (how commanded roll/pitch/yaw moments map to individual fin deflections) are all `TBD`/`OPEN`.

---

## 9. TVC (Thrust Vector Control) Architecture

### 9.1 Role Definition (`DECIDED`)

TVC is the **primary control authority** for the powered-landing concept and a major control input during powered ascent. It works by deflecting the thrust vector's direction (via a gimbaled motor mount or equivalent mechanism), not by changing thrust magnitude — magnitude is fixed per Section 7.

### 9.2 Why TVC, Given Fixed Thrust

Because thrust magnitude cannot be commanded, TVC's entire value is in **redirecting a fixed force vector** to produce control moments and to steer the net translational acceleration direction. This is fundamentally a moment/torque problem (for attitude control) and a vector-steering problem (for trajectory shaping) rather than a magnitude problem.

### 9.3 Design Parameters Requiring Analysis (all `OPEN`/`TBD`)

| Parameter | Why It Matters |
|---|---|
| Gimbal geometry (single-axis, dual-axis/gimbal ring, or ball-and-socket) | Determines achievable deflection pattern and mechanical complexity |
| Maximum deflection angle | Directly bounds maximum control moment achievable |
| Actuator type (servo, linear actuator, other) | Determines torque/speed/mass tradeoffs |
| Actuator speed / slew rate | Must be fast enough relative to required control bandwidth |
| Reaction time (command → physical response) | Adds to total loop latency (Section 18.5) |
| Gimbal-to-CG moment arm | Determines control moment per unit deflection: $M \approx F_{thrust} \cdot \sin(\delta) \cdot \ell$ |
| Structural loads on gimbal mount | Must survive full thrust at max deflection plus dynamic loads |
| Control-loop dynamics (how gimbal angle maps through vehicle dynamics to attitude change) | Needed for controller design and stability analysis |

### 9.4 First-Order Control-Moment Relationship (illustrative, not final)

For small deflection angle $\delta$ of a gimbaled motor producing thrust $F$, located a distance $\ell$ from the vehicle CG:

$$M \approx F \cdot \ell \cdot \sin(\delta) \approx F \cdot \ell \cdot \delta \quad \text{(small-angle approx.)}$$

This is a first-order relationship for early analysis only. It ignores gimbal dynamics, thrust misalignment tolerances, structural compliance, and coupling with vehicle bending modes (which likely don't matter at this vehicle scale, but that assumption itself is `ASSUMPTION`, not verified).

### 9.5 Interaction With Fixed-Thrust Constraint (Critical Open Problem)

Because $F$ is fixed, the *only* ways to change control moment are changing $\delta$ (gimbal angle) or changing $\ell$ (which is fixed once built). This means TVC authority is bounded by the maximum mechanical deflection angle — there is no "add more thrust to the correction" option. Sizing the maximum deflection angle correctly (enough authority for worst-case disturbances, but not so much that it's mechanically impractical or destabilizing) is a first-class open design problem (Section 32).

### 9.6 Open TVC Questions

- Single-axis vs. dual-axis gimbal — trade study not done
- Maximum deflection angle sizing — requires disturbance/authority analysis not yet performed
- Actuator selection (specific servo/motor part) — `TBD`, no hardware selected
- How TVC and fin commands are allocated/blended when both are active (control allocation problem) — `OPEN`

---

## 10. Avionics Architecture

### 10.1 Current Development Platform (`DECIDED` as a *prototyping* platform only)

| Item | Current Status |
|---|---|
| MCU | ESP32-WROOM-32 development board |
| Barometer | BMP388 — bench-tested, produced relatively stable altitude readings after init/calibration |
| IMU | ICM42688P — detected successfully (`WHO_AM_I = 0x47`), sensor read pipeline still under development |
| Storage | 32 GB microSD via Adafruit breakout — **intermittent initialization issues, not yet reliable** |
| Bus wiring (development only) | SPI: CS=GPIO15, SCK=GPIO18, MISO=GPIO19, MOSI=GPIO23. I2C: SDA=GPIO21, SCL=GPIO22 |

`REQUIREMENT`: The pinout above is a **development pinout** and must not be silently treated as a flight pinout in later documents or code without an explicit note that it was re-validated for the flight configuration.

### 10.2 Avionics Functional Blocks (`PROPOSED`)

| Block | Function | Maturity |
|---|---|---|
| Sensor interface | Read IMU, baro, (future) GPS, heading | Early prototype |
| Timekeeping | Consistent timestamping across sensors for fusion | Not yet designed |
| State estimator | Runs onboard (or, in early phases, off the vehicle for analysis) | Not yet implemented |
| Guidance/control | Runs onboard in flight configuration | Not yet implemented |
| Actuator driver | Drives TVC and fin servos/actuators | Not yet implemented |
| Data logger | Writes flight data to non-volatile storage | Implemented but unreliable |
| Power management | Regulates/distributes power to MCU, sensors, actuators | Not yet designed |
| Telemetry (optional) | Downlink for ground monitoring | Not yet designed, `OPEN` whether needed for early tests |

### 10.3 Known Issues

- **microSD reliability (`RISK`)**: Intermittent SPI init failures on the SD breakout must be root-caused (wiring, power sequencing, card compatibility, SPI clock speed, or library issue) before any flight-critical logging depends on it. This is flagged as a near-term priority (Section 25.1) precisely because it's a small, boring problem that would be catastrophic to discover was never fixed after a flight.
- **No integrated sensor fusion yet**: BMP388 and ICM42688P have been validated individually, not together, and not with any timestamp-synchronized combined read loop.

### 10.4 Open Avionics Questions

- Sensor sampling rate requirements (`OPEN`, depends on control loop bandwidth — Section 18.4)
- Redundancy strategy — single-string vs. any sensor redundancy (`OPEN`, likely single-string for a long time given program scale)
- Power budget and battery selection (`TBD`, no analysis yet)

---

## 11. Sensor Architecture

### 11.1 Sensor Suite (Current + Prospective)

| Sensor | Measures | Status | Known Limitations |
|---|---|---|---|
| ICM42688P (IMU) | Specific force (accelerometer), angular rate (gyroscope) | Detected, integration in progress | Accelerometer measures specific force, not velocity directly (must integrate + subtract gravity model); gyro drifts and requires bias estimation |
| BMP388 (barometer) | Atmospheric pressure → altitude | Bench-tested, stable after calibration | Noisy, sensitive to dynamic pressure effects at speed (needs venting/placement consideration), affected by local weather-driven pressure baseline drift |
| GPS/GNSS | Position, velocity | Not yet integrated | Lower update rate than IMU (often 1-10 Hz vs. hundreds of Hz), susceptible to multipath/dropout, may lose lock under high dynamics or vehicle attitude blocking antenna view — `OPEN` whether a rocket-rated (high-g, high-velocity capable) GPS module is required |
| Heading/orientation aiding (e.g., magnetometer) | Absolute heading reference | Not yet selected | Magnetometers are sensitive to nearby motor/electronics magnetic interference — `OPEN` whether this is usable near a motor and avionics bay |
| Future/candidate sensors | e.g., additional IMU for redundancy, airspeed/pitot, TVC/fin position feedback | `STRETCH`/`OPEN` | Not yet scoped |

### 11.2 Sensor Modeling Requirements (for both real integration and simulation)

Each sensor needs an explicit noise/error model before it can be either (a) trusted in a real estimator or (b) simulated convincingly:

| Error Source | Applies To | Notes |
|---|---|---|
| White measurement noise | All | Modeled as Gaussian for most classical filters; real noise may not be perfectly Gaussian |
| Bias | Accelerometer, gyroscope | Slowly time-varying; often modeled as a random walk state in the estimator itself (Section 13) |
| Scale factor error | Accel, gyro | Calibration-dependent, `TBD` per actual sensor unit |
| Drift | Gyroscope (attitude), barometer (baseline pressure) | Accumulates over time without correction from an absolute reference |
| Latency | All, especially GPS | Must be accounted for in fusion timing, not ignored |
| Update rate | GPS especially | Fusion must handle asynchronous, multi-rate measurements |
| Dropout / outliers | GPS, potentially IMU under high vibration | Estimator needs outlier rejection logic (Section 13.9) |

### 11.3 Coordinate Frames (`OPEN`, must be formally defined before estimator work proceeds)

At minimum, the following frames will need explicit definitions, with clearly specified transformations between them:

- **Body frame**: fixed to the vehicle (e.g., X forward along thrust axis, Y/Z per right-hand rule)
- **Navigation/local frame**: e.g., a local NED (North-East-Down) or ENU (East-North-Up) frame centered near the launch site
- **Sensor frames**: each physical sensor may be mounted with an offset/rotation relative to the body frame (lever-arm and boresight misalignment corrections)
- **ECEF/geodetic**: only needed if GPS lat/lon/alt must be converted into a local Cartesian frame for guidance math

`ASSUMPTION`: For a vehicle of this scale and flight duration (seconds to tens of seconds of powered flight), a flat, non-rotating local-frame approximation is likely sufficient (Earth curvature/rotation effects are negligible). This has not been formally verified but is a standard and reasonable assumption at this scale.

---

## 12. Flight Computer

### 12.1 Current State

The ESP32-WROOM-32 is the current learning/prototyping platform. It is **not** assumed to be the final flight computer (REQ-AV-02).

### 12.2 Evaluation Criteria for Eventual Flight Computer Selection/Design (`OPEN`)

| Criterion | Why It Matters |
|---|---|
| CPU performance (clock speed, core count) | Must run sensor fusion + guidance + control within the required loop period with margin |
| Floating-point performance (hardware FPU vs. software emulation) | EKF-class estimators involve significant matrix math; software float emulation may be too slow |
| RAM | Must hold state/covariance matrices, logging buffers, and program state without overflow |
| Flash / non-volatile storage | Program storage + possibly onboard logging |
| Real-time behavior | Whether the platform/OS can guarantee loop timing (bare-metal, RTOS like FreeRTOS, or a full OS) — jitter in the control loop degrades stability |
| Sensor sampling bandwidth | I2C/SPI bus speed must support required sensor read rates |
| Data logging bandwidth | Must sustain writes at the logging rate (Section 22) without blocking the control loop |
| Power consumption | Affects battery sizing, especially relevant for longer ground-test campaigns |
| Reliability features | Watchdog timers, brown-out detection, fault handling — critical for a system that must fail safely (REQ-SAFE-02) |

### 12.3 Custom Flight Computer PCB (`STRETCH` / Phase 6, Section 25)

A future goal is a custom PCB (KiCad) integrating MCU, IMU, barometer, GPS interface, logging, power management, actuator interfaces, and communications, potentially with sensor redundancy. This is explicitly a **later-phase** goal — it should not be started before the sensor/estimator/algorithm work has matured enough to know what the board actually needs to support (a common failure mode is designing hardware before the software requirements are known).

### 12.4 Real-Time Architecture Considerations (`OPEN`)

A single-loop, unstructured "read sensors, do math, write actuators, log data" architecture on a microcontroller can silently develop timing problems (e.g., an SD card write blocking for milliseconds at an unpredictable moment, right when a control update was due). This needs deliberate design — likely involving either a real-time OS (e.g., FreeRTOS, already available on ESP32) with prioritized tasks, or a carefully hand-scheduled bare-metal loop with bounded worst-case execution time per section. No decision has been made (`OPEN`).

---

## 13. Software Architecture

### 13.1 Language Strategy (`DECIDED`)

| Language | Primary Use |
|---|---|
| Python | Simulation, data analysis, mathematical prototyping, visualization, early ML experimentation |
| C++ | Embedded flight software, real-time control, hardware interfaces, and (later) potentially performance-critical simulation components |

The program lead is currently learning both, with C++ being the newer and more difficult transition. The roadmap (Section 27) explicitly plans for algorithms to be prototyped in Python first and then ported to embedded-compatible C++, rather than being written in C++ from scratch while C++ fluency is still developing.

### 13.2 Modular Separation (`REQUIREMENT`, `DECIDED`)

The codebase SHALL maintain clean separation between:

- Simulation (physics, environment)
- GNC algorithms (estimation, guidance, control) — ideally written so the *same* algorithm code can run in simulation and (after porting/compilation for the target) on the real flight computer
- Hardware interfaces (sensor drivers, actuator drivers)
- Sensor models (simulated noise/bias injection)
- Actuator models (simulated TVC/fin response, including latency and rate limits)

This separation is what makes the Simulation → HIL → Flight progression (Section 12 of the source material / Section 15 here) possible without rewriting the GNC core at each stage.

### 13.3 Repository/Project Structure (`PROPOSED`, illustrative)

```
rocket-program/
├── sim/                  # Python: physics engine, RocketPy integration, sensor/actuator sim
├── gnc/                  # Estimation, guidance, control algorithms (Python prototypes)
├── gnc-embedded/         # C++ ports of validated gnc/ algorithms for the flight computer
├── firmware/             # ESP32 (or future flight computer) embedded project
├── hardware/             # KiCad projects, wiring diagrams, BOMs (future)
├── analysis/             # Post-flight / post-simulation data analysis notebooks
├── docs/                 # This document and companions (Section 0.4)
└── tests/                # Unit tests, Monte Carlo campaign configs
```

This is a proposal for discussion, not a committed structure — see Section 36 (Appendix) for the actual proposed doc/version-control plan requested for this document specifically.

### 13.4 Open Software Questions

- Testing framework/strategy for embedded C++ (`OPEN`)
- Whether any part of the simulator itself should eventually be ported to C++ for speed (e.g., large Monte Carlo batches) (`OPEN`, `STRETCH`)
- Build/CI strategy — currently none (`OPEN`)

---

## 14. Physics Model

### 14.1 Purpose

The physics model is the mathematical description of how the vehicle actually moves, used both for simulation (Section 15) and for the "vehicle/environment model" block inside trajectory prediction (Section 5.1, Section 16).

### 14.2 Environment Model Components

| Component | Notes |
|---|---|
| Gravity | Constant-$g$ flat-Earth approximation likely sufficient at this scale (`ASSUMPTION`, unverified) |
| Atmosphere (density, pressure, temperature vs. altitude) | Standard atmosphere model (e.g., ISA) as a first pass; real conditions will differ (Section 20) |
| Wind | Not modeled initially (`PROPOSED` starting point: zero wind); wind model to be added once basic dynamics work (`OPEN` when) |
| Turbulence | `STRETCH`, later-fidelity addition |

### 14.3 Vehicle Model Components

Mass properties, aerodynamics (drag, lift, moment coefficients as functions of Mach/AoA), and control-surface effects, all as described in Sections 6, 8, 9. None of these have real numerical values yet (`TBD` throughout) — the modeling *framework* is what's being defined here, not the numbers.

### 14.4 Rigid-Body Equations of Motion (`PROPOSED` mathematical foundation)

**Translational dynamics** (Newton's second law, in an inertial/local frame):

$$m\dot{\mathbf{v}} = \mathbf{F}_{thrust} + \mathbf{F}_{aero} + \mathbf{F}_{gravity}$$

where $m = m(t)$ is time-varying during powered flight due to propellant consumption (so $\dot m \neq 0$ must be handled correctly — this is the classic "variable mass system" subtlety of rocket dynamics, related to the Tsiolkovsky rocket equation for the 1-DOF case, extended here to full 3-DOF/6-DOF translational + rotational motion).

**Rotational dynamics** (Euler's rotation equations, in body frame):

$$\mathbf{I}\dot{\boldsymbol{\omega}} + \boldsymbol{\omega} \times (\mathbf{I}\boldsymbol{\omega}) = \mathbf{M}_{thrust} + \mathbf{M}_{aero}$$

where $\mathbf{I}$ is the (time-varying, due to propellant burn) inertia tensor, $\boldsymbol{\omega}$ is body-frame angular velocity, and $\mathbf{M}$ terms are moments from TVC-deflected thrust and aerodynamic forces (including fin contributions).

**Attitude kinematics** (`OPEN` which representation to use — quaternion is likely preferred to avoid gimbal lock, but Euler angles may be used for early intuition-building):

$$\dot{\mathbf{q}} = \tfrac{1}{2}\mathbf{q} \otimes \begin{bmatrix}0 \\ \boldsymbol{\omega}\end{bmatrix}$$

**`ASSUMPTION`**: 6-DOF (3 translational + 3 rotational) rigid-body dynamics are sufficient — i.e., the vehicle is assumed rigid (no significant structural flexing/bending modes). This is reasonable for a small vehicle but has not been formally checked and should be revisited once real vehicle dimensions/materials are chosen.

### 14.5 Mathematics Required

Vector calculus, ordinary differential equations, linear algebra (rotation matrices/quaternions), and classical mechanics (Newton-Euler formulation). This maps directly to the math learning roadmap (Section 26).

---

## 15. Simulation Architecture

### 15.1 Design Principle (`DECIDED`)

RocketPy is intended as a **physics/rocket-dynamics backend** where useful (it already implements much of the 6-DOF rocket flight mechanics, standard atmosphere models, and parachute-recovery simulation). The program's own GNC software (estimation, guidance, control) SHALL remain conceptually and architecturally independent of RocketPy, so that:

1. The GNC code is portable to a from-scratch simulator or to the real flight computer without RocketPy-specific dependencies, and
2. RocketPy's built-in assumptions (e.g., about recovery systems, or about not natively supporting active TVC/fin control loops) don't silently become load-bearing assumptions of the GNC design.

**`OPEN`**: The exact integration boundary — i.e., whether RocketPy is used purely for baseline validation/cross-checking, or is actually driven in a closed loop with an external controller (RocketPy does support custom control functions in recent versions, which would need to be evaluated) — is not yet decided.

### 15.2 Proposed Simulator Pipeline

```
Rocket Builder/CAD params → Motor Database → Atmosphere Model → Physics Engine
                                                                       │
                                                                       ▼
                                                            Sensor Simulation
                                                       (noisy IMU/GPS/baro, latency,
                                                        dropouts — Section 16)
                                                                       │
                                                                       ▼
                                                            State Estimator
                                                                       │
                                                                       ▼
                                                              Guidance
                                                                       │
                                                                       ▼
                                                              Controller
                                                                       │
                                                                       ▼
                                                          Actuator Model (TVC/fins)
                                                        (rate limits, latency, saturation)
                                                                       │
                                                                       ▼
                                                    Physics Engine (updated state) ──▶ repeat
```

This mirrors the real closed loop (Section 5.1) exactly, which is the point: the simulator should let the GNC software "believe" it's connected to a real rocket.

### 15.3 Fidelity Progression (`DECIDED` philosophy: start simple)

| Stage | Fidelity |
|---|---|
| 1 | 3-DOF point-mass, no wind, perfect sensors — validate basic trajectory math |
| 2 | 6-DOF rigid body, basic aero (drag only), perfect sensors — validate rotational dynamics |
| 3 | 6-DOF + noisy/biased sensors — validate estimator |
| 4 | + wind, more complete aero (lift, control-surface effects), actuator dynamics — validate guidance/control |
| 5 | + Monte Carlo parameter dispersion (Section 20) — validate robustness |
| 6 | Hardware-in-the-loop (real flight computer, simulated sensors/dynamics) |

Do not attempt to build Stage 4-6 fidelity before Stage 1-2 are validated. This progression directly informs the Phase 2-3 roadmap (Section 25).

### 15.4 Open Simulation Questions

- Numerical integration method (fixed-step RK4 is a reasonable default; adaptive-step methods are a possible refinement) — `OPEN`
- How to structure Monte Carlo parameter sweeps efficiently in Python (`OPEN`, likely a later concern)
- Whether/when to port performance-critical simulation loops to C++ for large Monte Carlo batches (`STRETCH`)

---

## 16. State Estimation

### 16.1 Purpose

State estimation fuses noisy, biased, asynchronous sensor measurements into a single best estimate of the vehicle's true state, with an associated uncertainty. Every downstream block (prediction, guidance, control) depends entirely on this estimate — a guidance system computing a perfect trajectory correction from a wrong state estimate is worse than useless.

### 16.2 Why Naive Approaches Fail

- Accelerometers measure **specific force** (thrust + aero acceleration minus gravity's contribution as sensed by an accelerometer at rest, i.e., $\mathbf{f} = \mathbf{a} - \mathbf{g}$ in the appropriate frame) — not velocity or position directly. Getting velocity/position requires integration, and integrating noisy/biased acceleration causes rapidly growing error ("integration drift").
- Gyroscopes measure angular rate, and integrating them to get attitude accumulates drift from any small, slowly-varying bias.
- Barometric altitude is noisy and subject to local pressure-baseline drift (weather-dependent).
- GPS provides absolute position/velocity but at low rate and with its own noise, and can drop out.

No single sensor is trustworthy alone; the estimator's job is to combine them so that each sensor's weaknesses are covered by another's strengths (e.g., GPS corrects long-term IMU drift; IMU fills in between low-rate GPS updates).

### 16.3 Candidate Approach (`PROPOSED`, not yet implemented)

An Extended Kalman Filter (EKF) is the current leading candidate, because rocket attitude/translational dynamics are nonlinear (particularly the attitude kinematics) and the EKF is the standard, well-understood starting point for this class of problem in aerospace navigation. Alternatives noted for future comparison (`OPEN`, not started): Unscented Kalman Filter (UKF, better nonlinearity handling, higher compute cost), complementary filters (simpler, common in low-cost drone attitude estimation, may be a reasonable *first* stepping stone before a full EKF), and particle filters (`STRETCH`, likely unnecessary complexity for this problem unless multi-modal uncertainty becomes an issue).

### 16.4 Candidate State Vector (`PROPOSED`, explicitly not final — see instruction not to treat this as locked)

$$\mathbf{x} = \begin{bmatrix} \mathbf{p} \\ \mathbf{v} \\ \mathbf{q} \\ \boldsymbol{\omega} \\ \mathbf{b}_a \\ \mathbf{b}_g \end{bmatrix}$$

where $\mathbf{p}$ = position (3), $\mathbf{v}$ = velocity (3), $\mathbf{q}$ = attitude quaternion (4, or a minimal 3-parameter error-state representation), $\boldsymbol{\omega}$ = angular velocity (3, sometimes taken directly from the gyro rather than estimated), $\mathbf{b}_a$ = accelerometer bias (3), $\mathbf{b}_g$ = gyroscope bias (3). Total dimension and exact contents are `OPEN` — for example, whether angular velocity is a filter state or a direct (bias-corrected) measurement is a design choice not yet made, and additional states (e.g., a barometer bias term) may be added later.

### 16.5 EKF Structure (standard form, for reference)

**Prediction step:**
$$\hat{\mathbf{x}}_{k|k-1} = f(\hat{\mathbf{x}}_{k-1|k-1}, \mathbf{u}_k)$$
$$\mathbf{P}_{k|k-1} = \mathbf{F}_k \mathbf{P}_{k-1|k-1} \mathbf{F}_k^T + \mathbf{Q}_k$$

**Measurement update step:**
$$\mathbf{y}_k = \mathbf{z}_k - h(\hat{\mathbf{x}}_{k|k-1})$$
$$\mathbf{S}_k = \mathbf{H}_k \mathbf{P}_{k|k-1} \mathbf{H}_k^T + \mathbf{R}_k$$
$$\mathbf{K}_k = \mathbf{P}_{k|k-1}\mathbf{H}_k^T \mathbf{S}_k^{-1}$$
$$\hat{\mathbf{x}}_{k|k} = \hat{\mathbf{x}}_{k|k-1} + \mathbf{K}_k \mathbf{y}_k$$
$$\mathbf{P}_{k|k} = (\mathbf{I} - \mathbf{K}_k \mathbf{H}_k)\mathbf{P}_{k|k-1}$$

where $f(\cdot)$ is the (nonlinear) process model, $\mathbf{F}_k$ its Jacobian, $h(\cdot)$ the measurement model, $\mathbf{H}_k$ its Jacobian, $\mathbf{Q}_k$ process noise covariance, $\mathbf{R}_k$ measurement noise covariance. Every one of $\mathbf{Q}_k$, $\mathbf{R}_k$, and the exact forms of $f$/$h$ is `TBD` and must come from sensor characterization (Section 11.2) and process-noise tuning, not guessed.

### 16.6 Open Estimation Questions

- Multi-rate sensor fusion (GPS at low rate, IMU at high rate) — architecture `OPEN` (e.g., predict-only on IMU steps, update on GPS arrival)
- Outlier/glitch rejection (e.g., chi-square innovation gating) — not yet designed
- Numerical stability of covariance propagation on embedded hardware with limited floating-point precision — `OPEN`
- Initialization strategy (how the filter gets a valid initial state/covariance at power-on/pad) — `OPEN`
- Observability: some states (e.g., certain biases) may not be observable during particular flight phases — needs formal analysis, not assumed

---

## 17. Guidance

### 17.1 Purpose and Boundary With Control (`REQUIREMENT`, `DECIDED` — repeated for emphasis because this separation is a named program requirement)

Guidance answers "what trajectory/attitude should the vehicle target?" Control (Section 18) answers "what actuator command achieves that target?" This document maintains that separation throughout; a future revision must not quietly merge them without logging why.

### 17.2 Conceptual Guidance Loop

At each guidance update, the system conceptually asks: current position/velocity/attitude (from the estimator) → predicted landing point if no further correction is applied (from the trajectory predictor, using the vehicle/environment model of Section 14) → error between predicted and target landing point → a desired trajectory/attitude correction to reduce that error, subject to the vehicle's actual control authority (Sections 8, 9) and to physical/safety constraints.

### 17.3 Progression of Guidance Sophistication (`PROPOSED` roadmap, matches Section 25 Phase 4)

| Stage | Approach | Maturity Target |
|---|---|---|
| 1 | Basic trajectory targeting: simple proportional correction toward target, using a simplified (e.g., constant-deceleration or ballistic) landing-point prediction | Year 2 |
| 2 | PID-based guidance on lateral position/velocity error | Year 2 |
| 3 | Numerical trajectory optimization / more accurate landing-point prediction using the full vehicle model | Year 2-3 |
| 4 | Model Predictive Control (MPC)-based guidance | `STRETCH`, Year 3+ |
| 5 | Probabilistic/Monte Carlo-informed, uncertainty-aware guidance | `STRETCH`, Year 3+ or beyond program horizon |

**`REQUIREMENT`**: Do not attempt Stage 3+ before Stage 1-2 are validated in simulation (Section 15.3) — this directly implements the program's stated "don't skip to the hardest problem" principle.

### 17.4 Guidance Under the Fixed-Thrust Constraint (Central Open Problem, restated)

Because thrust magnitude cannot be commanded, guidance cannot ask for "less deceleration, please" — it can only ask for "steer the fixed thrust vector differently" (via TVC) and "add drag/lift via fins." This fundamentally limits what trajectories are achievable and means the landing-point prediction must account for a largely pre-determined deceleration profile, with steering being the main adjustable variable. This is arguably the single hardest unresolved conceptual problem in the whole program and deserves dedicated study before Stage 3+ guidance is attempted (Section 32).

### 17.5 Guidance Robustness (preview of Section 20)

The guidance system should eventually explicitly account for the fact that its own inputs (state estimate, vehicle model, atmosphere model) are uncertain — e.g., asking "what happens if my state estimate is wrong?" or "what if the atmosphere differs from the model?" is a long-term goal (`STRETCH`), not a Year-1/Year-2 requirement. Early guidance will implicitly assume its inputs are correct, which is a known, accepted limitation at that stage.

---

## 18. Control System

### 18.1 Purpose

Convert guidance's desired trajectory/attitude targets into specific TVC gimbal angle and fin deflection commands, and execute those commands as a stable, well-behaved feedback loop.

### 18.2 Baseline Controller (`REQUIREMENT`, `DECIDED`)

The initial controller SHALL be PID-class (Proportional-Integral-Derivative) or a cascade of PID loops (e.g., an inner fast attitude/rate loop and an outer slower position/trajectory loop — a common aerospace control architecture). More advanced methods (Section 18.3) are explicitly deferred until PID-based control is implemented, tuned, and validated in simulation.

**Standard PID control law** (for reference, applied per controlled axis/quantity):

$$u(t) = K_p e(t) + K_i \int_0^t e(\tau)\,d\tau + K_d \frac{de(t)}{dt}$$

where $e(t)$ is the error between the guidance target and the current estimated value of the controlled quantity (e.g., pitch angle, angular rate, lateral position). $K_p, K_i, K_d$ are all `TBD` — they require system identification and tuning against an actual vehicle/simulation model, not guessed values.

### 18.3 Future Control Approaches (`STRETCH` / `OPEN`, explicitly not assumed)

Model-based control (e.g., using a linearized state-space vehicle model) and optimization-based control (e.g., MPC) are noted as areas for future investigation, contingent on PID-based control proving insufficient or on there being clear program bandwidth to pursue them. They are not assumed to be necessary.

### 18.4 Control-Loop Design Considerations

| Consideration | Description | Status |
|---|---|---|
| Control bandwidth | How fast the loop must run to control the vehicle's actual dynamics (fastest relevant mode, e.g., rotational dynamics, sets a lower bound on loop rate — commonly a rule of thumb is control rate ≥ 5-10x the bandwidth of the fastest mode to control) | `OPEN` — requires a vehicle dynamics model first |
| Sensor latency | Delay between physical event and available measurement | `TBD`, depends on sensor + fusion pipeline |
| Actuator latency | Delay between command and physical actuator response (TVC gimbal, fin servo) | `TBD`, depends on selected hardware |
| Saturation | Actuators have max deflection/rate; controller must handle gracefully (e.g., anti-windup for the integral term) | Not yet designed |
| Rate limits | Actuators can only move so fast; commanding faster changes than physically achievable causes tracking error | Not yet characterized |
| Stability | Closed-loop system must not oscillate/diverge; requires formal analysis (e.g., root locus, Bode/Nyquist, or simulation-based stability margins) once a linearized model exists | `OPEN` |
| Noise amplification | Derivative terms in particular amplify sensor noise; filtering may be required | Not yet addressed |
| Controller tuning | Manual tuning, classical methods (Ziegler-Nichols-style), or model-based tuning from system ID | `OPEN` |
| System identification | Determining actual vehicle response parameters (e.g., from ground tests or simulation) to inform tuning | Not yet started |

### 18.5 Total Loop Latency (Important Cross-Cutting Concern)

Total loop latency = sensor latency + estimation compute time + guidance compute time + control compute time + actuator latency. Every one of these adds phase lag, which directly threatens stability margins. This total must be estimated and budgeted once hardware is selected (Section 12) — it is currently entirely unaddressed (`OPEN`).

### 18.6 Control Allocation (TVC + Fins Together)

When both TVC and fins are active and commandable, a control allocation scheme is needed to decide how a desired net moment/force is split between the two actuator types (which have different bandwidth, authority, and effectiveness-vs-airspeed characteristics). This is unaddressed (`OPEN`) and likely a Year 2-3 problem once both actuator types individually exist and are characterized.

---

## 19. Landing System

### 19.1 Framing (`DECIDED`, stated explicitly per program direction)

The landing system is a **GNC + propulsion + control problem**, not a recovery-hardware problem. There is currently no parachute-based primary landing architecture in this design. (A parachute or other passive recovery mechanism as an **abort/safety fallback**, separate from the primary powered-landing concept, is an open question — Section 24 — not a contradiction of this framing.)

### 19.2 Landing Sequence (`CONCEPTUAL`, high-level, subject to complete revision)

1. Vehicle is in descent (unpowered, after ascent burnout and coast/apogee), being stabilized by fins.
2. State estimator and trajectory predictor continuously update the predicted landing point.
3. At a computed trigger condition (altitude/velocity/time — exact logic `OPEN` and safety-critical, see Section 19.3), the landing motor ignites.
4. During the landing burn, TVC (primary) and fins (secondary) steer the vehicle toward the target and control attitude, while the fixed-thrust burn provides the deceleration.
5. Touchdown occurs at burnout or near burnout, ideally with low vertical/lateral velocity.

### 19.3 The Ignition-Timing Problem (Flagged as Central and Unsolved)

Because the landing motor cannot be throttled, restarted, or extinguished early, the decision of **when** to ignite it is possibly the single highest-stakes, least-forgiving decision in the entire flight. Ignite too early or too late (relative to the fixed burn's impulse profile and the vehicle's actual descent state) and there is no way to compensate mid-burn other than steering — which does not fix a purely vertical velocity/altitude mismatch. This requires dedicated analysis (trigger-condition design, sensitivity to state-estimate error, sensitivity to motor performance variation) before it can be considered even a `PROPOSED` design, and is currently `OPEN`.

### 19.4 Landing Accuracy — No Requirement Set Yet

No numeric landing accuracy requirement (e.g., "within X meters of target") exists (`TBD`). Setting one prematurely, before any simulation-based sensitivity analysis exists, would be arbitrary. This will be derived from Monte Carlo simulation results (Section 20) once the vehicle/GNC design has matured enough to run them meaningfully.

### 19.5 Open Landing System Questions

- Ignition-timing trigger logic (Section 19.3) — `OPEN`, high priority for eventual analysis
- Touchdown structure (legs/skid) — `OPEN`, Section 6.5
- Abort/failsafe behavior if landing-burn conditions are not met at the planned trigger point (e.g., predicted trajectory has diverged too far from achievable landing) — `OPEN`, safety-critical (Section 24)

---

## 20. Uncertainty and Robustness

### 20.1 Motivation

Every model in this document (vehicle dynamics, propulsion, aerodynamics, sensors, actuators) is an approximation of a real system that will differ from the model. A GNC system designed and tested only against a single "nominal" simulation is not validated against reality — it is validated against its own assumptions. This section defines how the program intends to confront that gap, as a long-term goal.

### 20.2 Sources of Uncertainty (non-exhaustive)

| Source | Type |
|---|---|
| Motor thrust curve (actual vs. datasheet/nominal) | Propulsion |
| Vehicle mass and CG (manufacturing/assembly variation) | Vehicle |
| Aerodynamic coefficients (model vs. reality, especially without wind-tunnel or CFD validation) | Aerodynamics |
| Wind (magnitude, direction, gusts) | Environment |
| Atmospheric density/pressure (deviation from standard atmosphere) | Environment |
| Sensor noise and bias (unit-to-unit variation, temperature effects) | Avionics |
| Actuator response (backlash, friction, temperature effects on servo speed/torque) | Actuation |
| Structural properties (stiffness, potential unmodeled flexing) | Structure |
| Manufacturing tolerances generally | All |

### 20.3 Proposed Method: Monte Carlo Dispersion Analysis (`PROPOSED`, `STRETCH` in terms of timeline — this is Phase-4-level testing, Section 23)

Run many simulated flights (e.g., hundreds to thousands) with each uncertain parameter randomly perturbed according to an assumed distribution (e.g., ±X% on thrust, ±Y° on initial pointing error, wind drawn from a distribution), and characterize the resulting spread of outcomes:

- Landing-position distribution (dispersion ellipse)
- Landing-velocity distribution
- Failure/constraint-violation probability (e.g., "controller saturates," "vehicle exceeds structural limit," "predicted landing point never converges")
- Sensitivity analysis — which uncertain parameters matter most to landing accuracy (informs where real-world characterization effort should go)

**`ASSUMPTION`**: Distributions for each uncertain parameter (e.g., "thrust ±5%") are themselves currently unknown and would need to come from either manufacturer specifications, bench/static-fire testing, or conservative engineering judgment — not invented numbers.

### 20.4 Long-Term Goal: Uncertainty-Aware Guidance (`STRETCH`)

Beyond just *measuring* robustness via Monte Carlo, a further goal is for the guidance system to explicitly account for uncertainty when making decisions (e.g., biasing decisions conservatively when state-estimate confidence is low, or using robust/chance-constrained optimization methods). This is an advanced GNC research topic and is explicitly marked as beyond near-term scope.

---

## 21. Machine Learning Stretch Goal

### 21.1 Explicit Ordering Constraint (`REQUIREMENT`, `DECIDED`)

The initial GNC system SHALL be physics-based. Machine learning SHALL NOT be the foundation of the program's guidance, navigation, or control before a working physics-based system exists and has been validated in simulation. This is a deliberate anti-scope-creep constraint, since ML is an easy trap for a program like this: it can look like progress (a model that "learns") while producing something un-debuggable, non-analyzable, and disconnected from the physical understanding the program is explicitly trying to build (Section 41 design philosophy).

### 21.2 Proposed Progression (`STRETCH`, all steps beyond the first are speculative)

```
Physics-based model → working physics-based GNC → large simulation dataset
   → ML experiments (offline, on the dataset) → hybrid physics+ML model
```

### 21.3 Candidate ML Applications (`STRETCH`, `OPEN` — none scoped in detail)

- Learning corrections to aerodynamic coefficient models (residual learning on top of a physics baseline, not replacing it)
- Improving trajectory prediction accuracy using learned disturbance/model-error patterns
- Assisting automated system identification (extracting vehicle parameters from flight/ground-test data)
- Offline analysis/optimization of guidance parameters using simulated flight datasets

### 21.4 Guardrail

Any ML component considered SHALL augment, not replace, the physics-based model, and SHALL be explainable/analyzable enough to debug (e.g., avoid "black box learned end-to-end controller" as a design pattern for a system this safety-relevant, at least within this program's current scope).

---

## 22. Data Logging

### 22.1 Purpose

Sufficient data must be recorded during every ground test and flight to reconstruct what happened and evaluate the GNC system's actual performance against its intended performance. Without this, the program cannot learn from tests — this makes data logging arguably as important as any GNC algorithm.

### 22.2 Candidate Logged Channels (`PROPOSED`, non-final)

| Category | Channels |
|---|---|
| Timing | Timestamp (consistent clock reference across all channels) |
| Raw sensors | Raw accelerometer (3-axis), raw gyroscope (3-axis), raw barometric pressure, raw GPS (position, velocity, fix quality) |
| Derived sensor values | Barometric altitude |
| Estimator outputs | Estimated position, velocity, attitude, angular velocity, estimated biases, covariance/uncertainty metrics |
| Guidance outputs | Predicted landing position, target landing position, guidance targets (desired attitude/trajectory) |
| Control outputs | Controller error terms, commanded TVC angle(s), commanded fin deflection(s) |
| Actuator feedback (if instrumented) | Actual TVC position, actual fin position (if position sensing is added) |
| Motor/propulsion state | Ignition events, burn status flags |
| System health | Fault flags, sensor validity flags, watchdog events, battery voltage |
| Error metrics | Real-time computed error vs. target, where available |

### 22.3 Requirements

REQ-AV-01 and REQ-AV-03 (Section 4.3) govern this: adequate logging rate for reconstruction, and demonstrated reliability before flight-critical use. Exact logging rate is `TBD` pending control-loop rate determination (it should generally match or exceed the fastest control loop rate for meaningful reconstruction).

### 22.4 Post-Flight Analysis Tooling (`PROPOSED`)

The program should eventually have tooling to automatically parse, visualize, and analyze flight logs (e.g., plotting estimated vs. "truth" — in sim — or estimated vs. GPS-only trajectory in real flights, actuator command histories, error metrics over time). A separate "Black Box Analyzer" project in the user's broader engineering workspace may eventually support this (`OPEN` how tightly integrated it becomes with this program specifically).

---

## 23. Testing and Verification

### 23.1 Philosophy (`REQUIREMENT`, `DECIDED`)

The program SHALL NOT jump directly from software to a fully autonomous flight vehicle. Verification proceeds through progressively more realistic levels, and no level is skipped for a component that hasn't passed the level below it.

### 23.2 Verification Level Hierarchy

| Level | Name | Description | Exit Criteria (`TBD` specifics, but the *category* is fixed) |
|---|---|---|---|
| 1 | Mathematical verification | Verify equations independently (e.g., hand/symbolic derivation checks, known-solution comparisons) | Equations match known analytical/reference solutions where available |
| 2 | Software unit tests | Verify individual algorithm implementations in isolation (e.g., EKF update step against a known test vector) | Unit tests pass, edge cases covered |
| 3 | Simulation (closed loop) | Test the entire GNC loop in simulation (Section 15) | Loop runs stably across nominal scenarios |
| 4 | Monte Carlo simulation | Test uncertainty/robustness (Section 20) | Acceptable failure-mode statistics (thresholds `TBD`) |
| 5 | Hardware-in-the-loop (HIL) | Real flight computer connected to simulated sensors/vehicle dynamics | Flight computer produces correct commands against simulated scenarios in real time |
| 6 | Ground testing | Sensors, actuators, mechanisms, electronics tested physically (e.g., static TVC actuation tests, fin actuation tests, vibration/shock where feasible) | Physical components behave within modeled bounds |
| 7 | Controlled subscale experiments | Test individual control concepts physically at small scale/low risk (e.g., a tethered or simple test rig for attitude control, not a full flight) | Specific, narrow engineering questions answered |
| 8 | Flight testing | Full vehicle flight, incrementally more autonomous | Each flight answers pre-defined questions (Section 25.5) |

### 23.3 Current Verification Status

All components are currently pre-Level-1 or early Level-1/2 (individual sensor bench tests). No component has reached Level 3.

### 23.4 Test Planning Principle

Every test at every level SHALL have a small number of specific, written-down questions it is meant to answer, decided *before* the test — not a vague goal of "see if it works." This applies as much to a Level-1 equation check as to a Level-8 flight.

---

## 24. Safety

### 24.1 Principle (`REQUIREMENT`, `DECIDED`, stated explicitly per program direction)

Autonomy does not imply safety. An autonomous system with a bug or a bad sensor reading will confidently execute the wrong action unless explicitly designed not to. Safety is an engineering requirement from the start, not an afterthought layered on before the first flight.

### 24.2 Hazard Categories

| Category | Examples | Current Mitigation Status |
|---|---|---|
| Propulsion hazards | Motor malfunction, CATO (catastrophic failure), unexpected ignition | `OPEN` — standard hobby/high-power rocketry motor-handling safety practices apply once motor class is chosen; no program-specific mitigation designed yet |
| High-speed aerodynamic hazards | Structural failure at high dynamic pressure, flutter | `OPEN`, ties to Section 8.3 |
| Structural failure | Airframe, fin, or TVC mount failure under load | `OPEN`, no structural analysis done yet |
| Electronics failure | Power loss, brownout, connector failure, EMI | `OPEN` |
| Loss of control | Controller instability, actuator saturation without graceful degradation | `OPEN`, ties to Section 18.4 |
| Actuator failure | TVC or fin actuator jam, linkage failure, one-sided failure | `OPEN` |
| Sensor failure | Dropout, glitch, complete sensor loss | Partially addressed conceptually (outlier rejection, Section 16.6), not implemented |
| Software failure | Crash, infinite loop, incorrect logic, timing violation | `OPEN`, ties to Section 12.4 real-time design |
| Communications failure | Loss of any ground telemetry/command link, if one exists | `OPEN`, depends on whether telemetry is even used (Section 10.2) |
| Unexpected trajectory | Vehicle diverges from any expected flight envelope | `OPEN` — needs an explicit "flight envelope" definition and monitoring logic |

### 24.3 Failsafe Principle (`REQUIREMENT`)

REQ-SAFE-02 (Section 4.4): on loss of state-estimate confidence, actuator fault, or software fault, the vehicle SHALL fail to a defined passive/safe state rather than continuing closed-loop control on untrustworthy data. What that "safe state" is (e.g., commanding actuators to a neutral position, deploying a recovery device, cutting thrust if physically possible — noting REQ-PROP-02's no-restart-but-also-no-early-cutoff-capability constraint may limit this) is `OPEN` and needs real design work, likely including a decision on whether a passive recovery device (parachute) is retained purely as an abort mechanism separate from the primary powered-landing concept.

### 24.4 Launch-Site and Regulatory Considerations (`OPEN`, `TBD`)

Applicable regulations depend on total impulse class (which determines whether a flight falls under model rocketry or high-power rocketry rules in most jurisdictions), launch-site requirements (certified range, waivers, insurance where applicable), and any additional rules specific to actively-controlled/autonomous vehicles at a given launch site or club. None of this has been researched in this document and must be before any real flight test is planned — this is flagged as a near-term research item (Section 30), not a late-stage afterthought.

### 24.5 Testing Procedure Safety

Ground tests (Level 6-7, Section 23.2) involving live motors, pressurized systems, or powered actuators near people require their own written safety procedures (e.g., minimum standoff distances, kill switches, PPE) — not yet written (`OPEN`).

---

## 25. Development Roadmap

### 25.1 Roadmap Principles (`REQUIREMENT`, `DECIDED`)

The roadmap is dependency-ordered, not calendar-ordered for its own sake — a later phase is scheduled later *because* it depends on an earlier phase's output, not merely because "that's when it seems reasonable." The roadmap is intentionally light per week/month because the program lead is simultaneously in school with other commitments; sustainability is a design requirement of the roadmap itself, not just the rocket.

### 25.2 Near Term (Next Several Weeks–Months)

| Priority | Task | Depends On | Rationale |
|---|---|---|---|
| 1 | Root-cause and fix microSD logging reliability | Nothing (can start immediately) | Cheap to fix now; catastrophic to discover broken after a flight; also blocks any meaningful bench-test data collection |
| 2 | Build a combined, timestamp-synchronized sensor read loop (IMU + baro together) | Individual sensor bring-up (done) | First real step toward sensor fusion; currently sensors have only been validated separately |
| 3 | Begin differential equations coursework | Calculus (in progress/done for relevant topics) | Directly gates rigid-body dynamics and control theory (Section 26) |
| 4 | Start a minimal Python 3-DOF point-mass rocket simulator (Stage 1, Section 15.3) | Basic calculus/physics | Cheapest possible way to start exercising the "physics → code" muscle without needing RocketPy fluency yet |
| 5 | Research launch-site/regulatory requirements for the eventual test program | Nothing | Long lead time item; better to know constraints early than discover them late (Section 24.4) |

### 25.3 Year 1 — Foundations, Simulation, Avionics

Emphasis: mathematics (Section 26), basic simulation (Stages 1-3, Section 15.3), reliable avionics bring-up, and a first working (if crude) state estimator tested purely in simulation against known ground truth.

Dependencies enforced: a 3-DOF sim (near-term item) precedes a 6-DOF sim; 6-DOF sim with perfect sensors precedes 6-DOF sim with noisy sensors; noisy-sensor sim precedes estimator development (you need something to test the estimator against before writing it against real, unforgiving hardware).

### 25.4 Year 2 — Advanced GNC, Hardware, Integration

Emphasis: guidance Stage 1-2 (Section 17.3), PID control implementation and tuning in simulation (Section 18.2), TVC and fin mechanism design/characterization (Sections 8-9), and beginning hardware-in-the-loop testing (Level 5, Section 23.2).

Dependencies enforced: guidance/control work in simulation precedes any hardware actuation work being *trusted* for control (though mechanical prototyping of TVC/fins can and should proceed in parallel, since it's a separate engineering track with its own long lead time — see Section 28-29). A working simulated closed loop (estimator + guidance + PID control + simulated actuators) precedes HIL testing.

### 25.5 Year 3 — Physical Testing, Increasing Autonomy

Emphasis: Monte Carlo robustness testing (Section 20), ground testing of real TVC/fin mechanisms (Level 6), controlled subscale experiments (Level 7), and the first flight tests (Level 8) — each answering a specific, pre-defined question rather than attempting full autonomy immediately. Example first-flight-class questions (`PROPOSED`, illustrative, not commitments):

- Does the flight computer survive launch loads and continue operating?
- Are sensor measurements usable in real flight conditions (vibration, dynamic pressure effects on baro, etc.)?
- Does the state estimator's in-flight estimate track reasonably against an independent reference (e.g., GPS-only trajectory, ground-based tracking if available)?
- Does a PID attitude/rate controller remain stable using real (not simulated) actuators?
- Do TVC/fin actuators respond as characterized on the ground, under real flight loads?

### 25.6 Final Phase (Toward End of 3.5-Year Horizon and Beyond)

Emphasis: integrated GNC (estimator + guidance + control + both actuator types) on an actual flight vehicle, working toward — but not guaranteed to achieve within this horizon — an autonomous powered landing demonstration at a predefined target (OBJ-13, `STRETCH`). The program should be considered successful even if OBJ-13 is not fully achieved within 3.5 years, provided the foundational chain (Sections 25.2-25.5) has been genuinely built and validated — this is stated explicitly so that a full landing does not become a false measure of the program's actual engineering value.

---

## 26. Mathematics Learning Roadmap

Each subject is tied directly to the subsystem(s) it unlocks — this roadmap exists to prevent studying math disconnected from the project (per explicit program direction to avoid competition-math-style tangents).

| Subject | Status | Directly Needed For |
|---|---|---|
| Calculus (limits, derivatives, integrals, chain/product/quotient rules, trig derivatives, FTC, Riemann sums, antiderivatives) | Substantially complete per program lead's own account | Dynamics (integrating acceleration → velocity → position), optimization (guidance trajectory shaping), physics simulation, foundation for differential equations |
| Differential Equations | Upcoming, next major topic | Rocket dynamics (equations of motion are ODEs), control systems (transfer functions, system response), state-space models, simulation (numerical ODE integration, e.g., RK4) |
| Linear Algebra | Not yet started | Coordinate transformations, rotation matrices/quaternions (Section 14.4), state-space systems, the entire Kalman filter formalism (Section 16.5 is matrix algebra throughout) |
| Probability / Statistics | Not yet started | Sensor noise modeling (Section 11.2), the statistical foundation of the Kalman filter itself, uncertainty quantification, Monte Carlo analysis (Section 20) |
| Classical Mechanics | Not yet started (beyond general physics intuition) | Forces, momentum, Newton's laws underlying Section 14.4 translational dynamics |
| Rigid-Body Dynamics | Not yet started | Attitude dynamics, angular velocity, moments of inertia, Euler's rotation equations (Section 14.4) |
| Aerodynamics | Not yet started | Drag/lift modeling, static margin and stability (Section 6.3), fin control authority (Section 8) |
| Control Theory | Not yet started | PID design and tuning, stability analysis, TVC/fin control loop design (Section 18) |
| Numerical Methods | Not yet started | Simulation integration schemes, EKF numerical implementation, general "turning continuous math into working code" |

**`REQUIREMENT`**: The order above (calculus → differential equations → linear algebra roughly in parallel with or shortly after diff-eq → probability/statistics → mechanics/rigid-body/aero/controls, with the last four likely overlapping and reinforcing each other) is the current `PROPOSED` sequencing, chosen so that each subject unlocks the next subsystem in the software/engineering roadmap (Section 25) roughly when that subsystem is reached. This sequencing is not rigid — it should flex if a subsystem's needs pull a particular math topic forward.

---

## 27. Programming Learning Roadmap

| Track | Current Status | Roadmap |
|---|---|---|
| Python | Being learned/used | Continue using for simulation, data analysis, and algorithm prototyping (Section 13.1) — this is likely to advance fastest since it has the lowest friction for the math-heavy, iterative work of Sections 15-18 |
| C++ | Being learned, currently more tedious/difficult transition from Python | Explicitly planned as a *gradual porting* process: prototype an algorithm in Python first (e.g., a basic PID loop, a simplified EKF update), validate it against known behavior, then port the validated logic to embedded-compatible C++ — rather than attempting to write novel GNC logic directly in C++ while still building basic fluency |

**`ASSUMPTION`**: This Python-first, C++-port-second workflow will remain the right approach at least through Year 1-2; it should be revisited once C++ fluency has caught up enough that direct C++ prototyping becomes efficient rather than a bottleneck.

---

## 28. CAD / Mechanical Development Roadmap

| Milestone | Status |
|---|---|
| Learn CAD fundamentals (tool `TBD`, not yet selected — commonly Fusion 360, Onshape, or FreeCAD for hobbyist/education use, but no selection has been made here) | Not started |
| Model basic airframe geometry for simulation input (mass properties, geometry for aero estimation) | Not started; depends on vehicle configuration trade study (Section 6.4) |
| Design TVC gimbal mechanism | Not started; depends on Section 9.3 analysis |
| Design fin actuation mechanism | Not started; depends on Section 8.3 analysis |
| Design touchdown structure (legs/skid) | Not started, `OPEN` per Section 6.5 |
| Iterate mechanical designs against ground-test results (Level 6, Section 23.2) | Future |

This track runs largely in parallel with the software/GNC track rather than strictly after it, since mechanical design has its own long lead times (learning CAD, iterating physical prototypes, sourcing materials/actuators) — but early GNC analysis (required control authority, deflection angles, response times from Sections 8-9 and 18) should inform mechanical design targets before final mechanism designs are locked, to avoid building a mechanism that turns out to lack sufficient control authority.

---

## 29. Electronics Roadmap

| Milestone | Status |
|---|---|
| Reliable microSD logging | In progress — currently the top near-term priority (Section 25.2) |
| Combined IMU + baro read loop with consistent timestamping | Not started |
| GPS integration | Not started |
| Actuator driver circuitry (for TVC and fin servos/actuators) | Not started, depends on actuator selection (Sections 8-9, `TBD`) |
| Power system design (battery selection, regulation, budget) | Not started, `TBD` |
| Learn KiCad | Not started |
| Design custom flight-computer PCB (Section 12.3) | `STRETCH`, explicitly deferred until software/algorithm requirements are known |

---

## 30. Research Roadmap

Research items are questions requiring reading, analysis, or small experiments — distinct from the build-oriented roadmap above.

| Research Item | Why It's Needed | Priority |
|---|---|---|
| Launch-site and regulatory requirements for the eventual test program (Section 24.4) | Long lead time; shapes achievable vehicle scale (total impulse class) | High, near-term |
| Motor selection criteria and available thrust-curve data sources | Needed to put real numbers into the propulsion model (Section 7.3) | Medium, Year 1 |
| Barrowman-method (or equivalent) aerodynamic coefficient estimation for amateur rockets | Needed for a first-pass aero model without CFD/wind-tunnel access (Section 14.3, 6.2) | Medium, Year 1 |
| Survey of amateur/university rocketry TVC and fin-actuation mechanism designs (open literature, hobbyist project writeups) | Avoids reinventing solved sub-problems; informs Sections 8-9 mechanical design | Medium, Year 1-2 |
| EKF/UKF implementation references and worked examples for aerospace navigation | Directly informs Section 16 implementation | Medium, Year 1-2 |
| RocketPy's control-function/closed-loop capabilities (current version) | Resolves the `OPEN` item in Section 15.1 | Medium, Year 1-2 |
| Real-time embedded scheduling approaches on ESP32 (FreeRTOS task design) | Informs Section 12.4 | Medium, Year 1-2 |

---

## 31. Technical Risks

Risk = Likelihood × Impact, both rated qualitatively (L/M/H) at this stage since no quantitative failure-rate data exists yet.

| ID | Risk | Likelihood | Impact | Mitigation Direction |
|---|---|---|---|---|
| RISK-01 | Fixed-thrust ignition-timing error causes unrecoverable landing-burn mismatch (too high/low velocity at burnout) | H | H | Dedicated ignition-timing sensitivity analysis (Section 19.3); Monte Carlo characterization (Section 20) before any real landing-burn attempt |
| RISK-02 | Static margin / controllability trade (Section 6.3) is mis-sized, yielding a vehicle that is either uncontrollable-by-TVC-alone or dangerously unstable | M | H | Explicit stability analysis once candidate vehicle configuration exists; do not finalize airframe geometry without it |
| RISK-03 | microSD/data-logging unreliability persists into flight testing, losing critical flight data | M (currently known-broken) | H (loses the ability to learn from a flight) | Root-cause and fix as top near-term priority (Section 25.2); demonstrate reliability (REQ-AV-03) before trusting it for any real test |
| RISK-04 | Control-loop total latency (Section 18.5) is too high for required bandwidth, causing instability | M | H | Loop latency budgeting once hardware is selected; HIL testing (Level 5) before flight |
| RISK-05 | Multi-motor configuration (Section 6.4) introduces asymmetric thrust/CG issues not adequately modeled | M | M-H | Trade study (REQ-PROP-05) before committing to multi-motor hardware |
| RISK-06 | Fin/TVC control authority is insufficient at some flight regime (e.g., low dynamic pressure for fins, max deflection angle too small for TVC) | M | H | Explicit authority-vs-disturbance analysis (Sections 8.3, 9.5) before mechanism finalization |
| RISK-07 | Program scope/ambition exceeds sustainable pace given concurrent school/other commitments, leading to burnout or abandonment | M | H (to program continuity, not to any single flight) | Roadmap explicitly paced (Section 25.1); success redefined as incremental validated progress, not "does it land" |
| RISK-08 | Regulatory/launch-site constraints are discovered late and block planned flight tests | M | M | Research launch-site/regulatory requirements early (Section 24.4, Section 30) |
| RISK-09 | Estimator divergence or unhandled sensor outlier during flight leads to a confidently wrong state estimate driving control into a bad action | M | H | Outlier rejection design (Section 16.6); mandatory failsafe-to-safe-state behavior (REQ-SAFE-02) rather than trusting the estimator unconditionally |
| RISK-10 | Structural failure under flight or ground-test loads (airframe, fin mount, TVC mount) due to absent structural analysis | M | H | Structural analysis once geometry/materials are chosen; ground testing (Level 6) before flight loads are trusted |
| RISK-11 | Machine-learning stretch-goal work is pursued prematurely, displacing physics-based foundation work | L-M | M | Explicit ordering constraint (Section 21.1) |
| RISK-12 | Custom flight-computer PCB is designed before software/algorithm requirements are known, requiring a costly respin | L-M | M | Explicit deferral (Section 12.3) until requirements are known from working prototype software |

---

## 32. Open Engineering Questions

A consolidated list, drawn from every section above, of unresolved questions requiring research, analysis, or experiment. This list will grow — treat additions as healthy, not as scope creep, provided each new question is tied to a real decision that needs making.

| ID | Question | Related Section(s) |
|---|---|---|
| Q-01 | Single motor vs. multi-motor (two ascent + one landing, or other split) configuration — which is actually better, by what criteria? | 6.4, 7.4, REQ-PROP-05 |
| Q-02 | What static margin range gives an acceptable stability/controllability trade for this vehicle? | 6.3 |
| Q-03 | What is the ignition-timing trigger logic for the landing burn, and how sensitive is landing outcome to timing error? | 19.3 |
| Q-04 | What maximum TVC gimbal deflection angle is required, given worst-case disturbance torques? | 9.5, 9.6 |
| Q-05 | Single-axis or dual-axis TVC gimbal? | 9.6 |
| Q-06 | How many fins, what geometry, and what control-allocation scheme maps commanded moments to individual fin deflections? | 8.4 |
| Q-07 | How is control authority allocated/blended between TVC and fins when both are active? | 18.6 |
| Q-08 | What is the required control-loop update rate, given actual (not yet modeled) vehicle dynamics? | 18.4 |
| Q-09 | What is the total closed-loop latency budget (sensor + compute + actuator), and does any candidate flight computer meet it? | 12.2, 18.5 |
| Q-10 | Is an EKF the right estimator, or does a complementary filter or UKF serve better at this program's compute budget and complexity tolerance? | 16.3 |
| Q-11 | What exact state vector, process model, and measurement model should the estimator use? | 16.4, 16.5 |
| Q-12 | What is the root cause of microSD initialization reliability issues? | 10.3, 25.2 |
| Q-13 | What real-time scheduling architecture (RTOS vs. hand-scheduled bare metal) should the flight computer use? | 12.4 |
| Q-14 | Is the ESP32-class platform adequate for the final flight computer, or is a more capable MCU/SBC required? | 12.2 |
| Q-15 | What sensor sampling rates are actually required, once control bandwidth is known? | 11, 18.4 |
| Q-16 | What propellant/motor characterization data (thrust curve, total impulse, mass) will be used, and from what source (published data vs. static-fire testing)? | 7.3, 30 |
| Q-17 | What wind/atmosphere fidelity is needed before simulation results are trustworthy enough to inform real design decisions? | 14.2, 15.3 |
| Q-18 | What uncertainty distributions (thrust variation, aero coefficient uncertainty, sensor noise/bias magnitudes) should Monte Carlo analysis use, and where do those numbers come from? | 20.3 |
| Q-19 | What touchdown structure (legs, skid, or other) will the vehicle use, and how is it integrated without conflicting with fin/TVC placement? | 6.5, 28 |
| Q-20 | What is the vehicle's safe-state/failsafe behavior on loss of estimator confidence or actuator fault — and does it require retaining a passive recovery device (parachute) as an abort mechanism separate from the primary powered-landing concept? | 24.3 |

---

## 33. Future Work

Beyond the roadmap in Section 25, the following are recognized as legitimate future extensions once the core program has matured — none are commitments:

- Redundant sensor suites for fault tolerance (`STRETCH`)
- Telemetry/ground-station monitoring for real-time flight visibility (`OPEN` whether needed even for near-term flights)
- Higher-fidelity aerodynamic modeling (CFD or wind-tunnel testing) if amateur-method (e.g., Barrowman) estimates prove insufficient (`STRETCH`)
- Structural finite-element analysis of airframe/mounts (`STRETCH`, likely once real geometry/materials exist)
- Expanded Monte Carlo tooling with automated sensitivity analysis and visualization (`STRETCH`)
- Hybrid physics+ML models (Section 21) (`STRETCH`)
- A generalized "Black Box Analyzer" flight-data tool shared across the user's broader engineering projects (`OPEN` scope)
- Multi-vehicle or iterative vehicle-generation programs (build vehicle 2, 3, ... with lessons applied) (`STRETCH`, beyond current 3.5-year horizon framing)

---

## 34. Technical Paper Plan

### 34.1 Relationship to This Document (`DECIDED`)

This Technical Design Document (ARP-TDD-001) is the living engineering source of truth. A separate technical paper (ARP-TP-001) is planned to communicate the program's motivation, architecture, methodology, and results to an outside audience. The two documents serve different purposes and SHALL NOT be conflated: this document is allowed to be incomplete, provisional, and full of open questions; the paper should be written from actual results once they exist.

### 34.2 Planned Paper Structure (`PROPOSED`, to be written later, not now)

1. Motivation and problem statement
2. Related work / context (how this compares to existing amateur/professional powered-landing and GNC work)
3. System architecture (drawn from Sections 5-19 of this document, condensed)
4. Mathematical model (drawn from Sections 14, 16-18)
5. Simulation methodology and results (once real results exist)
6. Experimental/flight-test results (once real results exist)
7. Discussion of limitations and what didn't work
8. Conclusions and future work

### 34.3 Explicit Constraint (`REQUIREMENT`)

The paper SHALL be written from actual obtained results (simulation output, test data, flight data), not written aspirationally as though the system already works. This document (ARP-TDD-001) explicitly forbids inventing results (Section 41 of the source requirements, carried forward as a standing rule for all program documentation, including the eventual paper).

### 34.4 Timing

No paper drafting should begin in earnest before there is at least Level-3/4 simulation-based results (Section 23.2) worth reporting. Early paper work, if any, should be limited to motivation/background sections that don't depend on results.

---

## 35. Project Milestones

Milestones are checkpoints, not deadlines with hard dates (none are set, since the program is self-paced around school commitments). Each includes its exit criteria and explicit dependencies.

| ID | Milestone | Exit Criteria | Depends On |
|---|---|---|---|
| M-01 | Reliable data logging | N consecutive clean logging sessions with no init/write failures (N `TBD`) | Section 25.2 priority 1 |
| M-02 | Fused sensor read loop | IMU + baro read together on a consistent timestamp base, logged | M-01 |
| M-03 | First working point-mass simulator | 3-DOF sim reproduces expected ballistic trajectory for a simple test case | Basic calculus/physics |
| M-04 | 6-DOF simulator (perfect sensors) | Simulated vehicle exhibits physically reasonable rotational + translational behavior under thrust/aero | M-03, rigid-body dynamics math (Section 26) |
| M-05 | Sensor-realistic simulator | 6-DOF sim producing noisy/biased simulated IMU/baro/GPS output | M-04, Section 11.2 sensor models |
| M-06 | First working state estimator | EKF (or chosen alternative) tracks simulated truth state within a defined error bound in simulation | M-05, linear algebra + probability (Section 26) |
| M-07 | First working guidance (Stage 1) | Simple proportional landing-point-correction guidance demonstrated in simulation | M-06 |
| M-08 | First working PID controller (simulation) | Attitude/rate control loop stable in simulation against simulated actuator dynamics | M-06, control theory basics |
| M-09 | TVC mechanism prototype | Bench-characterized deflection range, speed, and torque | Section 9 analysis, CAD/electronics tracks |
| M-10 | Fin mechanism prototype | Bench-characterized deflection range, speed, and force/moment output | Section 8 analysis, CAD/electronics tracks |
| M-11 | Closed-loop simulation integration | Estimator + guidance + control + simulated actuators run together as one closed loop | M-06, M-07, M-08 |
| M-12 | Monte Carlo robustness campaign (first pass) | Distribution of outcomes computed for a defined uncertainty set | M-11, Section 20 |
| M-13 | Hardware-in-the-loop demonstration | Real flight computer running real GNC code against simulated sensors/dynamics in real time | M-11, flight computer selection (Section 12) |
| M-14 | Ground actuation test | TVC and fins actuate correctly under real (non-flight) conditions, matching bench characterization | M-09, M-10 |
| M-15 | First flight test | Answers a small set of pre-defined questions (Section 25.5); does not require any autonomy beyond basic stability | M-13, M-14, Section 24 safety/regulatory clearance |
| M-16 | First flight with closed-loop attitude control | Real PID controller stabilizes real vehicle attitude in flight using real TVC/fins | M-15 |
| M-17 | First flight with active guidance toward a target | Vehicle demonstrably alters trajectory toward a predefined target using guidance+control, landing accuracy not yet required | M-16 |
| M-18 | Autonomous powered landing demonstration (`STRETCH`) | Controlled powered touchdown near a predefined target | M-17, and everything above it |

---

## 36. Appendix A — Program Status, Priorities, and Closeout Summary

### 36.1 Current Project Status (as of this revision)

- **Mathematics**: Introductory calculus substantially complete (limits, derivatives incl. chain/product/quotient/trig rules, integrals, FTC, Riemann sums, antiderivatives). Differential equations is the next major topic. Linear algebra, probability/statistics, mechanics, rigid-body dynamics, aerodynamics, and control theory have not yet been started.
- **Embedded hardware**: ESP32-WROOM-32 in use. BMP388 barometer bench-tested with stable altitude readings post-calibration. ICM42688P IMU detected (WHO_AM_I confirmed) with integration in progress. microSD logging implemented but **unreliable** (intermittent init failures) — this is the single most concrete, fixable near-term item in the whole program.
- **Software**: Python and C++ both being learned; no simulator, estimator, guidance, or control code yet exists.
- **Simulation**: RocketPy identified as a candidate physics backend; no custom simulator yet built.
- **CAD/Electronics**: KiCad planned but not started; no PCB, no TVC/fin hardware exists.
- **Overall**: The program is genuinely at the very beginning. This is not a criticism — a 3.5-year first-principles program *should* look like this at month zero. The risk is not being early; the risk is pretending to be further along than this status shows, and this document exists partly to prevent that.

### 36.2 Top 10 Immediate Priorities

1. Fix microSD logging reliability (RISK-03) — small, boring, and blocking.
2. Build a combined, timestamped IMU+baro read loop.
3. Start differential equations.
4. Start a minimal 3-DOF Python point-mass simulator.
5. Research launch-site/regulatory requirements (long lead time item).
6. Define coordinate frames formally (Section 11.3) before any estimator code is written.
7. Write a first-pass propulsion data source plan — what motor data will actually be used for early simulation (Section 7.3, Q-16).
8. Decide (even provisionally) on a CAD tool and do a first basic model, to start the mechanical-design learning curve in parallel with software.
9. Draft the ignition-timing problem (Section 19.3) as a standalone research note — this is the program's hardest conceptual problem and deserves early, dedicated thought rather than being deferred until "later."
10. Set up the repository/version-control structure (Section 36.6) so that everything from here forward is tracked properly from day one.

### 36.3 Top 10 Engineering Risks

See Section 31 for full detail and mitigation directions. Ranked by combined likelihood × impact at this stage:

1. RISK-01 — Fixed-thrust ignition-timing error (unrecoverable landing-burn mismatch)
2. RISK-09 — Estimator divergence/unhandled outlier driving bad control action
3. RISK-02 — Static margin / controllability mis-sizing
4. RISK-06 — Insufficient fin/TVC control authority in some flight regime
5. RISK-04 — Excessive control-loop latency
6. RISK-10 — Structural failure from absent structural analysis
7. RISK-07 — Program pace unsustainable against school/other commitments
8. RISK-03 — Data-logging unreliability persisting into flight testing
9. RISK-05 — Multi-motor asymmetric thrust/CG issues
10. RISK-08 — Late-discovered regulatory/launch-site constraints

### 36.4 Top 20 Open Technical Questions

See Section 32 for the full table (Q-01 through Q-20) with related-section cross-references. Reproduced here as a flat list for quick reference:

1. Single vs. multi-motor configuration (Q-01)
2. Acceptable static margin range (Q-02)
3. Landing-burn ignition-timing trigger logic and sensitivity (Q-03)
4. Required maximum TVC deflection angle (Q-04)
5. Single- vs. dual-axis TVC gimbal (Q-05)
6. Fin count/geometry and control allocation (Q-06)
7. TVC/fin control allocation/blending scheme (Q-07)
8. Required control-loop update rate (Q-08)
9. Total closed-loop latency budget and flight-computer adequacy (Q-09)
10. EKF vs. complementary filter vs. UKF choice (Q-10)
11. Exact estimator state/process/measurement models (Q-11)
12. Root cause of microSD reliability issue (Q-12)
13. Real-time scheduling architecture for flight computer (Q-13)
14. Adequacy of ESP32-class hardware for final flight computer (Q-14)
15. Required sensor sampling rates (Q-15)
16. Motor/propellant characterization data source (Q-16)
17. Required wind/atmosphere simulation fidelity (Q-17)
18. Monte Carlo uncertainty distribution sourcing (Q-18)
19. Touchdown structure design (Q-19)
20. Safe-state/failsafe behavior and parachute-as-abort question (Q-20)

### 36.5 Full Multi-Year Roadmap (Summary)

See Section 25 for full detail and dependency rationale.

| Horizon | Focus |
|---|---|
| Near term (weeks-months) | Fix logging, combined sensor loop, start diff-eq, start 3-DOF sim, research regulations |
| Year 1 | Math foundations, simulation Stages 1-3, reliable avionics, first estimator tested in sim |
| Year 2 | Guidance Stage 1-2, PID control in sim, TVC/fin mechanism design+characterization, begin HIL |
| Year 3 | Monte Carlo testing, ground testing of real mechanisms, subscale experiments, first flight tests |
| Final phase (toward/beyond 3.5 yr) | Integrated flight GNC; autonomous powered landing at target remains a stretch goal, not a guarantee |

### 36.6 Recommended Next Milestone

**M-01 (reliable data logging)** combined with **M-03 (first working point-mass simulator)**, pursued in parallel, is the recommended immediate focus. Neither requires advanced math beyond what's already in hand, both produce fast, visible feedback (a fixed sensor logs cleanly; a simulator produces a trajectory plot), and both directly unblock everything downstream (M-02, M-04+ depend on them). Anything more ambitious right now (e.g., starting estimator work, or trying to design TVC hardware) would be getting ahead of the dependency chain this document just spent 35 sections establishing.

### 36.7 Proposed Document / Version-Control Structure

`PROPOSED`, for discussion — this is a starting point, not a mandate:

```
engineering-workspace/
└── rocketry/
    ├── docs/
    │   ├── ARP-TDD-001_technical_design_document.md   ← this document; versioned in git
    │   ├── ARP-REQ_requirements_register.md            ← split out from Section 4 once it grows large
    │   ├── ARP-RISK_risk_register.md                   ← split out from Section 31 once it grows large
    │   ├── adr/                                        ← Architecture Decision Records, one file per
    │   │                                                  reversed/changed `DECIDED` item, e.g.
    │   │                                                  ADR-001_single_vs_multi_motor.md
    │   ├── research-notes/                             ← one file per Section 30 research item
    │   └── ARP-TP-001_technical_paper_draft.md          ← started only per Section 34.4 timing rule
    ├── sim/            (code, per Section 13.3)
    ├── gnc/
    ├── gnc-embedded/
    ├── firmware/
    ├── hardware/
    ├── analysis/
    ├── flight-logs/    ← one dated subfolder per test/flight, raw + processed data + a short report
    └── tests/
```

**Version-control conventions (`PROPOSED`):**

- This document (ARP-TDD-001) is revised in place with an incremented version number and a new Revision History row (Section 0.3) for every material change — not rewritten silently.
- A `DECIDED` item that later changes gets an ADR explaining why, rather than a silent edit — this preserves the reasoning trail that makes the document trustworthy three years from now.
- Every flight or significant ground test gets a dated report in `flight-logs/`, even if the result was "it didn't work" — especially then, per Section 23.4's testing philosophy.
- Git tags at each major milestone (Section 35) give a clean way to look back at "what did the program know and believe at M-06" later.

---

*End of Version 1.0. This document is expected to be revised substantially and often. Treat its current length and apparent completeness as a description of the problem space, not evidence of progress against it — the actual progress is tracked in Section 36.1 and 36.2, and it is, honestly, very early.*

