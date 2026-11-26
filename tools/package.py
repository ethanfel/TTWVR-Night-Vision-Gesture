# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import sys
from zipfile import ZIP_DEFLATED, ZipFile

version = sys.argv[1]
root = Path(__file__).resolve().parents[1]
build = root / "build"
source = f"https://github.com/ethanfel/TTWVR-Night-Vision-Gesture/tree/v{version}"
with ZipFile(build / f"TTWVR-Night-Vision-Gesture-{version}.zip", "w", ZIP_DEFLATED) as archive:
    archive.write(build / "TTWVRNightVision.dll", "NVSE/Plugins/TTWVRNightVision.dll")
    archive.write(root / "LICENSE", "LICENSE")
    archive.writestr("NOTICE.txt", f"TTWVR Night Vision Gesture {version}\nGPL-3.0-only\n\nSource and build instructions\n{source}\n")
