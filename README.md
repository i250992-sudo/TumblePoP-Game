🎮 TumblePop – Arcade Edition (SFML 2.6.1)

A modern C++ reimplementation of the classic Tumble Pop arcade game, built using SFML 2.6.1.
This project focuses on polished gameplay mechanics, smooth animations, and faithful arcade-style enemy behavior.

🎥 Gameplay Demo

▶️ Watch the gameplay recording:
https://github.com/user-attachments/assets/ffaa6c64-83b4-4fe7-9783-720157d013c0

The video is stored directly in the repository and auto-plays on GitHub.

✨ Key Features

🎞 Smooth player & enemy animations with state-based FPS tuning

🕹 Consistent platforming mechanics

Jump triggers only from the ground

Ceiling collision detection with head-strike fallback

Seamless vertical row transitions when space is available

Forward momentum assist for accessible platform jumps

👾 Enemy AI

Linear movement patterns

Intelligent gap-jumping behavior

🌪 Vacuum & capture mechanics

Capture, store, and release enemies

Clean capture UI (Captured: X)

🎨 Asset-driven UI and effects

Font-based HUD

Dim-teleport effect for Invisible Man enemy

🕹 Controls
Action	Key
Move	Arrow Keys
Jump	Up Arrow
Vacuum (Hold)	Space
Aim Vacuum	W / A / S / D
Release One Enemy	Z
Release All Enemies	X
Quit Game	ESC
🚀 Quick Start (Windows – MinGW)

Compile and run using the following commands:

g++ -c tumblepop_final.cpp -std=c++11 -ISFML-2.6.1/SFML-2.6.1/include
g++ tumblepop_final.o -o tumblepop -LSFML-2.6.1/SFML-2.6.1/lib -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system
.\tumblepop.exe


Ensure SFML dependencies are available at runtime.

📁 Project Structure
TumblePoP-Game/
│
├── Data/
│   └── Assets, sprites, textures, fonts, audio
│   └── tumblepop game record.mp4
│
├── SFML-2.6.1/
│   └── Bundled SFML headers & libraries
│
├── tumblepop_final.cpp
├── cmds.txt
└── README.md

🛠 Technologies Used

Language: C++ (C++11)

Framework: SFML 2.6.1

Graphics

Audio

Window

System