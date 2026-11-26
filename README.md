# TTWVR Night Vision Gesture

Bring a controller beside the temple on the same side and tap its button. Tap again to switch that effect off.

| Gesture | Effect |
| --- | --- |
| Left temple + **X** | Toggle the Pip-Boy light. Works with or without goggles. |
| Right temple + **A** | Toggle night vision. Carry **Night Vision Goggles** or **Reinforced Night Vision Goggles** in your inventory. |

Both gestures work on any WristBoy page during gameplay. You can use both effects at once. A holster slot is optional. If you carry both pairs of goggles, the gesture removes whichever pair you’re wearing or equips the reinforced pair if neither is worn.

## Install

Requires Tale of Two Wastelands, **TTWVR 1.1.8** (1.1.7 is also supported), xNVSE and JIP LN NVSE.

1. [Download the addon ZIP](https://github.com/ethanfel/TTWVR-Night-Vision-Gesture/releases/download/v1.1.8-r1/TTWVR-Night-Vision-Gesture-1.1.8-r1.zip).
2. Install and enable the ZIP in your VR mod profile, or copy its `NVSE` folder into `Fallout New Vegas/Data`.
3. Restart the game and load your save.

Release names follow the TTWVR version. **1.1.8-r1** is the first addon revision for TTWVR 1.1.8.

If the gesture doesn’t respond, check `Data/NVSE/Plugins/TTWVRNightVision.log` or the same path under your mod manager’s overwrite folder.

## Build

Run `make` with i686 MinGW-w64 to build the DLL, and `make test` with a native C++17 compiler to run the gesture tests. `make package` creates the installable ZIP in `build/`.

## License

This addon is licensed under the [GNU General Public License, version 3 only](LICENSE).
