https://www.youtube.com/watch?v=N3TYbDLTiU0&ab_channel=MaxMarsland


**Key Features:**

IR Communication:

Utilized an IR receiver (TSOP4836) and emitter to send and receive binary-encoded signals.
Programmed the IR remote to support three functions: move left, move right, and shoot.
Integrated a TV remote as a substitute for the IR emitter to handle signal issues caused by hardware constraints.

Hardware Design:

Designed a custom PCB for the IR emitter, featuring an ATMEGA328P microcontroller, crystal oscillator, ISP header, and other essential components.
Integrated six 8x8 LED matrices driven by MAX7219 ICs for game visuals.

Game Development:

Implemented movement, shooting mechanics, and collision detection for player and alien entities.
Designed a timing-based system for animations, alien movements, and bullet trajectories.
Created a matrix-based coordinate system to handle positions and interactions efficiently.

Challenges & Solutions:

Resolved hardware timing issues affecting PWM by substituting the IR emitter with a TV remote.
Optimized game logic to handle real-time actions and minimize delays.
Adapted matrix orientations for accurate display and functionality.

Technologies & Tools:

Hardware: ATMEGA328P, IR LED, TSOP4836, MAX7219, LED Matrices, Arduino.
Software: C++ for microcontroller programming.
PCB Design: Developed custom PCB layouts for hardware components.

Achievements:

Successfully demonstrated a fully interactive Space Invaders game controlled via IR communication.
Built a modular system with reusable components, enabling future adaptability for other applications like TV or light control.
Gained hands-on experience in embedded systems, hardware troubleshooting, and real-time programming.
