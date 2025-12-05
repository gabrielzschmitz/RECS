# RECS

<img align="right" width="192px" src="./resources/icon.svg" alt="RECS Logo">

<a href="./LICENSE"><img src="https://img.shields.io/badge/license-MIT-green" alt="License"></a>
<a href="https://www.buymeacoffee.com/gabrielzschmitz" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: 20px !important;width: 87px;" ></a>
<a href="https://github.com/gabrielzschmitz/RECS"><img src="https://img.shields.io/github/stars/gabrielzschmitz/RECS?style=social" alt="Give me a Star"></a>

**RECS** is a minimal and efficient Entity Component System (ECS) library
designed specifically to integrate smoothly with
**[raylib](https://www.raylib.com)**. It provides a simple foundation to build
games or interactive applications using the ECS architecture.

This repository includes both the **RECS library** itself and a minimal example
demonstrating sprite rendering, animations, and basic player movement using
RECS and raylib.

---

## Quick Start

### 1. Clone the repository

```sh
git clone https://github.com/gabrielzschmitz/RECS.git
cd Game
```

### 2. Build and run

Follow the platform-specific instructions in [INSTALL.md](INSTALL.md) to build
and run the example.

### 3. Explore the example

* **`src/engine`** — The core ECS library (RECS) components, systems, and
utilities.
* **`src/entities`** — Example game entities using RECS components (e.g.,
player with animations).
* **`resources`** — Assets such as sprites and icons used in the example.

---

## Features

* Minimal, easy-to-understand ECS design tailored for raylib
* Components for transforms, sprites, animations, directions, and bounding
boxes
* Systems to render sprites, animate frames, handle player input, and draw
bounding boxes
* Example of animated player character with keyboard controls

---

## Usage

Use **RECS** as a lightweight ECS foundation for your own raylib projects.
Extend components and systems to fit your game’s needs, or build entirely new
gameplay on top of it.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file
for full details.
