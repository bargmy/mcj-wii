# RubyDung (rd-132328) Nintendo Wii Port

This is a native port of the rd-132328 C codebase (originally derived from the `mcj-portc` codebase) to the Nintendo Wii. 

The game runs at a smooth **60 FPS** on real Wii hardware and the Dolphin Emulator, featuring custom optimized vertex array rendering and hardware clock timing adjustments.

---

## Features & Wii Optimizations
* **Smooth 60 FPS Performance**: Migrated all immediate mode rendering (`glBegin`/`glEnd`) to Client-Side Vertex Arrays, avoiding GPU FIFO queue desyncs under the OpenGX wrapper.
* **Xbox / Wii Controller Scheme**: Designed specifically for comfortable console controls:
  * **Nunchuk Analog Stick**: Controls camera look rotation (Pitch and Yaw) with smooth deadzones.
  * **Wii Remote D-Pad**: Controls player movement (Forward, Backward, Strafe Left/Right).
  * **Nunchuk C Button** (Xbox A Button): Jump.
  * **Nunchuk Z Button** / **Wii B Button** (Xbox B Button / Right Trigger): Break block.
  * **Wii 1 Button** / **Wii A Button** (Xbox Left Trigger / Left Bumper): Place block.
* **On-Screen Block Target Outline**: Restored the original flashing white face outline selection target overlay.
* **Clipped Render Distance**: Implemented distance-based chunk culling (4 chunks / 64 blocks radius) specifically for the Wii, preventing the GPU from choking on large vertex counts.
* **Embedded Texture Assets**: Packaged terrain and character texture sheets inside the binary as compressed C arrays, resolving Dolphin SD Card Virtual FS errors.
* **Wii Hardware Time Base (TB)**: Clock system utilizes the Wii's low-level Time Base registers to prevent physics tick skipping or frame choking.

---

## Build Prerequisites
To compile the project, you need the **devkitPro** toolchain equipped with **devkitPPC** and the Wii port libraries.

### Required Packages:
* **devkitPPC**: PowerPC cross-compiler toolchain.
* **libogc**: Core Wii system API wrapper.
* **freeglut**: OpenGL Utility Toolkit for Wii.
* **opengx**: OpenGL to GX hardware pipeline translation layer.
* **libfat**: SD/USB FAT storage library.
* **wiiuse**: Wiimote interface API.
* **zlib**: Compression library for level serialization.

---

## Compilation

### Option A: Local Build (devkitPro Installed)
If devkitPro is set up on your machine, simply run:
```bash
make -f Makefile.wii clean
make -f Makefile.wii
```
This generates `rubydung.dol` (executable) and `rubydung.elf` (debug target).

### Option B: Docker Build (Recommended)
You can compile the project instantly without local toolchain installation using the devkitPro Docker image:
```bash
docker run --rm -v $(pwd):/workspace -w /workspace devkitpro/devkitppc:latest bash -c "source /opt/devkitpro/wiivars.sh && make -f Makefile.wii clean && make -f Makefile.wii"
```

---

## Playing

### Via Homebrew Channel (SD/USB)
1. Format an SD Card or USB drive to FAT32.
2. Create a folder named `/apps/rubydung/` on the root of the card.
3. Copy `rubydung.dol` into `/apps/rubydung/` and rename it to `boot.dol`.
4. Load the Homebrew Channel on your Wii and launch the game.

### Via Dolphin Emulator
Simply drag and drop `rubydung.dol` into the Dolphin window to play instantly.
