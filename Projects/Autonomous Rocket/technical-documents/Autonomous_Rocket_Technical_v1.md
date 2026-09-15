# Autonomous Rocket Technical v1

These are my own notes on what I'm building and where it actually stands.

Date: September 2026

---

## 1. What I'm actually trying to build

The obvious way to describe this project is "build a rocket that lands itself." But that's not really a useful description, because it skips over what "lands itself" actually requires. If I just start trying to build that directly, I don't know what I'm building.

So instead I broke it into a loop: sensors tell me something, I use that to figure out what state the rocket is actually in, I compare that to where I want it to go, I decide what the rocket should do, I turn that into actual actuator commands, the rocket moves, the sensors read the new state, and it repeats. Every piece of the project is really just one link in that loop. I'm giving myself about 3.5 years for this because I'm doing it around school and because, realistically, each one of those links is its own hard problem.

The landing part is the part that sounds cool, but it's not what I'm working on right now, and I don't think it's even the part that teaches me the most. The actual hard problem is the middle of that loop. That's figuring out my real state from sensors that are noisy and biased, then doing something useful with an estimate that's never quite right. Getting a microcontroller to read an IMU is not hard. Making the rest of the system work when that IMU reading is slightly wrong is hard.

## 2. Why I can't throttle my way to a landing

The obvious way an autonomous rocket lands is by throttling the engine down so vertical velocity hits zero right as it touches the ground. That's basically how Falcon 9 does it.

I can't do that with my current motors. They're solid motors that only ignite once, and once lit they burn exactly the way the thrust curve says they'll burn. I can't make them push harder or softer, I can't shut them off early, and I can't restart them.

*Note: there is a way to throttle solid motors, but that requires more research into high temperature withstanding ceramics and how to implement that into the whole system. That could possibly be an extension later on, but I'm deeming it unnecessary for right now.*

That leaves me with two things I can actually change:

* When I light the landing burn
* Which direction the fixed thrust is pointing (TVC), plus how the fins are angled

That's it. No "throttle down a little." Because of that, the timing of the landing burn is probably the riskiest decision in the whole flight. If I light it too early or too late, there's no way to fix a bad vertical velocity mismatch once the motor's already burning. I don't have an answer for this yet. It needs an actual sensitivity analysis later, basically running a bunch of simulated flights with slightly wrong timing and seeing how bad the landing gets, not a guess.

## 3. Fins vs. TVC

Early on I considered having the fins double as landing legs, since that seemed efficient: one part doing two jobs instead of building a separate structure. But there's a problem with that. If the same part is both a control surface and something the rocket's weight rests on at touchdown, then any failure in one function risks taking out the other too, and I'd be designing it around two very different sets of loads. So I split that up. Fins stay aerodynamic only, and the landing structure (legs or a skid, not decided yet) is a separate mechanical problem.

I also went back and forth on whether fins or TVC should be the main way I steer. I ended up deciding TVC is primary during powered flight, because it can actually generate meaningful control moments regardless of how fast the rocket is moving, while fins only work when there's enough air moving over them. Fins end up being the backup. They help stabilize the rocket, and during any unpowered phase (coast, unpowered descent) they're the only control I have at all, since there's no thrust to redirect.

I also had an earlier idea of doing a belly flop maneuver like Starship does, with a flip right before landing, and I dropped it. It looked cool, but it needs way more aerodynamic control authority and way more structural strength than a small, fixed thrust, early stage vehicle can realistically handle. A mostly vertical descent with smaller trajectory corrections is a much more reasonable target for where I actually am right now.

## 4. Where things actually stand

Not making this sound more finished than it is.

**Simulator (Python, rocket_sim package, repo MTMAG11/rocket-simulator):**

I started with the simplest possible model. Just thrust, changing mass, gravity, and basic time integration to get velocity and altitude. For the motor, I used real thrust curve data from a Klima A6 .eng file instead of inventing a curve, because I wanted at least one part of the simulation based on something real rather than a number I made up.

I'm tracking rocket dry mass (150 g) and motor mass (14.5 g total, 3.5 g of that is propellant) separately instead of lumping them into one number, because the mass actually changes during the burn and I need that reflected correctly in the acceleration calculation.

Right now the model outputs about 10.4 N peak thrust, 54 m/s² peak acceleration, 11 m/s max velocity, and 8.5 m max altitude. I don't think these numbers mean much as real predictions yet, since there's no drag, no 3D motion, and no wind in the model. They're mostly just confirmation that the basic time integration logic works.

While testing it, I noticed the simulation keeps calculating an acceleration of negative 9.81 m/s² even after the rocket has already landed. That's not really a bug in the math. Gravity actually is still acting in the equation. It's a bug in the structure of the simulation, because right now it's just one continuous free fall calculation instead of something that tracks actual flight phases, like before launch, powered ascent, coast, descent, and ground contact. So the next thing I need to do is add real flight states so the simulation stops producing telemetry once the rocket's on the ground.

There's no 3DOF, no 6DOF, no aerodynamics, no sensor model, no estimator, no control, and no guidance built yet. All of that is still just the plan, not code.

**Avionics hardware:**

The ESP32 (XH-32S board) is my prototyping platform, not something I'm assuming will be the final flight computer.

The BMP388 barometer has been bench tested and gives stable altitude readings once it's calibrated. That one basically just works.

The ICM42688 IMU is detected correctly on the I2C bus (the WHO_AM_I check passes), but I haven't finished building the actual read and calibration pipeline for it yet.

The SD card logging was broken for a while, and figuring out why took some actual debugging. I originally had the SD card's CS pin on GPIO15 using the simple SD.begin(15) call, and it kept failing to initialize reliably. I eventually traced it to a bad wiring connection between D19 and DO, which was a hardware wiring issue, not a code issue. Once I switched to an explicit SPI configuration with CS on GPIO5 and a 1 MHz clock speed, logging started working reliably. I still haven't run it for an extended period, so I'm not ready to fully trust it yet. It needs to survive a long run before I'd use it for anything that actually matters.

The IMU and barometer have only been tested separately so far, not read together on a shared timestamp, so there's no fused sensor loop yet either.

**Everything else is still just an idea:** state estimation, TVC hardware, fin hardware, control, guidance, a custom flight computer, CAD, and structural design. No code and no hardware exists for any of it yet.

That's genuinely where things are. This is month zero of a multi year project, so it's supposed to look this early. The thing I actually want to avoid is pretending I'm further along than this.

## 5. How I'm planning to get from here to a working system

I laid this out in order on purpose, because each stage only means something once the stage before it actually works. Jumping ahead to a more interesting sounding stage before the earlier one is solid just means I'd be building on top of something I can't trust.

This is only for the simulator:
1. Fix the flight state bug in the simulator and add real telemetry (burnout time, altitude, velocity, time to apogee, max velocity, max acceleration, landing time) as one structured results object instead of scattered numbers.
2. Move to a 3DOF simulation: real 3D position and velocity, gravity, basic drag, and wind. This is the first point where the rocket can actually move through space instead of just up and down.
3. Move to 6DOF: attitude (using quaternions instead of Euler angles, mainly to avoid gimbal lock), angular velocity, moments of inertia, and aerodynamic torques. This is where control actually starts to matter, because now the rocket can tumble and something has to stop it from tumbling.
4. Add sensor simulation: fake IMU, baro, and eventually GPS data with realistic noise, bias, drift, and latency layered on top of the true simulated state. The important rule here is that the flight computer code never gets to see the true state directly, only the noisy sensor output. If I let it cheat and use the true state, then none of the later testing actually proves anything.
5. Build a state estimator. I'll probably start with something simpler, like a complementary filter, before trying a full EKF, since I want to actually understand why the filter works before I'm relying on one I can't debug.
6. Build closed loop control: PID, on a single axis first, entirely in simulation. I want to prove I can stabilize one axis before trying to control all three at once.
7. Build guidance, starting with something simple, like a proportional correction toward a landing point, before anything like real trajectory optimization.
8. Run uncertainty analysis using Monte Carlo simulations with randomized thrust, wind, and sensor errors to see how much the landing accuracy actually varies once things aren't perfect.
9. Hardware in the loop testing: the real flight computer running against simulated sensors and dynamics in real time.
10. Real hardware and real flight tests, each one designed to answer a specific question rather than just "see if it lands."

I'm deliberately not using machine learning anywhere in this early plan. It's tempting because it can look like progress even when the underlying system isn't something I actually understand. I'd rather get a physics based system working and validated first, and only look at ML later if there's an actual measurable reason to.

## 6. The problems I don't have answers for yet

* **Landing burn ignition timing.** Since there's no throttle, there's no way to correct a bad ignition timing guess once the burn has started. I need to actually run the sensitivity numbers on this, meaning how much landing error a given timing error produces, instead of just hoping the timing will be close enough.
* **Static margin vs. controllability.** A rocket that's more aerodynamically stable is also harder to steer, because the same force keeping it stable also resists whatever the controller is trying to do. Too unstable, and the whole thing depends on the control system never making a mistake. I can't actually analyze this yet because I haven't picked a vehicle configuration.
* **How much TVC deflection I actually need.** Since thrust magnitude is fixed, the only way to get more control authority out of TVC is a bigger gimbal angle. I don't know yet how much disturbance torque I actually need to be able to fight, so I can't size this properly.
* **Single motor vs. multiple motors.** The idea of two ascent motors plus one separate landing motor sounds appealing, but it also adds real problems, like asymmetric thrust if the two motors don't burn identically, and the CG shifting in a weird way as different motors burn out at different times. This needs an actual trade study, not just picking whichever version sounds cooler.

## 7. Math and programming I need before the next stages make sense

Calculus is basically done. Differential equations is next, since the actual equations of motion are ODEs and I can't really do rigid body dynamics without them. After that comes linear algebra, which I need for rotations and for literally every step of a Kalman filter, and probability and statistics, which I need to model sensor noise and run Monte Carlo analysis correctly. Mechanics, rigid body dynamics, aerodynamics, and control theory come after that, mostly in parallel, learned roughly when the simulator stage actually needs them instead of all at once up front.

Python is what I'm using for the simulator and for prototyping algorithms, since it's faster to iterate in and easier to debug when something's wrong. C++ is for the actual embedded flight computer, and it's the harder language transition right now. My plan is to prototype every algorithm in Python first, confirm it actually behaves the way I expect, and only then port it to C++, instead of trying to write new logic directly in C++ while I'm still building basic fluency in it.

## 8. One rule I won't break

Autonomy doesn't automatically mean safe. If the system is confidently wrong about its own state, it'll still confidently do the wrong thing unless I specifically design it not to. So the rule is that if the state estimate looks untrustworthy, or a sensor faults, or an actuator faults, or the software hits an error, the rocket fails to a safe, passive state instead of continuing to try to fly itself on bad information. I haven't fully worked out what that safe state actually looks like yet. It could mean cutting thrust if that's even physically possible, commanding actuators to neutral, or maybe keeping a parachute purely as a backup. But the principle behind it isn't something I'm willing to compromise on later just because it's inconvenient.

## 9. How I'm keeping this organized

I use ChatGPT for day to day idea capture and quick brainstorming, and Claude for the more careful technical documentation. There's no automatic sync between the two. If I want to bring something from one into the other, I copy it over myself and say so explicitly. Either way, my own decisions are what actually count. Anything either one suggests is just a suggestion until I actually decide to go with it.

I'm keeping the more detailed tracking stuff, like decision history, the risk list, and open questions, in a separate file so this document doesn't turn back into an unreadable spreadsheet. This one is meant to stay the readable version of what the project actually is and where it actually stands.

---

*I expect this to go out of date fairly fast, and that's fine. I'll update the relevant section whenever something real actually changes, not on a fixed schedule.*
