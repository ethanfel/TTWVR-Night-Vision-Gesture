# TTWVR Night Vision Gesture

Toggle night vision by bringing your **left hand to your left temple and tapping X**, or your **right hand to your right temple and tapping A**. Tap again with either hand to switch it off.

Carry **Night Vision Goggles** or **Reinforced Night Vision Goggles** in your inventory. A holster slot is optional. If you carry both pairs, the gesture equips the reinforced pair and removes whichever pair you’re wearing.

## Install

Requires Tale of Two Wastelands, **TTWVR 1.1.7**, xNVSE and JIP LN NVSE.

1. [Download the addon ZIP](https://github.com/ethanfel/TTWVR-Night-Vision-Gesture/releases/download/v0.1/TTWVR-Night-Vision-Gesture-0.1.zip).
2. Install and enable the ZIP in your VR mod profile, or copy its `NVSE` folder into `Fallout New Vegas/Data`.
3. Restart the game and load your save.

If the gesture doesn’t respond, check `Data/NVSE/Plugins/TTWVRNightVision.log` or the same path under your mod manager’s overwrite folder.

## Build

Run `make` with i686 MinGW-w64 to build the DLL, and `make test` with a native C++17 compiler to run the gesture tests.
