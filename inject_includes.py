#  inject_includes.py  -  PlatformIO pre-build hook
#
#  M5HAL (pulled in by M5UnitUnified / M5Unit-NFC) does `#include <Wire.h>` and
#  `#include <SPI.h>` but does not declare those built-in libraries in its
#  manifest. PlatformIO compiles each library in isolation, so those headers are
#  not on M5HAL's include path and the build fails with "Wire.h: No such file".
#
#  This runs before the build, resolves the Arduino-ESP32 framework directory,
#  and appends the Wire/SPI source folders to the GLOBAL include path (CPPPATH),
#  which every library build inherits. No hard-coded user paths.
import os
Import("env")

framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")

include_dirs = []
for lib in ("Wire", "SPI"):
    path = os.path.join(framework_dir, "libraries", lib, "src")
    if os.path.isdir(path):
        include_dirs.append(path)

if include_dirs:
    env.Append(CPPPATH=include_dirs)
    print("inject_includes: added to CPPPATH -> %s" % include_dirs)
else:
    print("inject_includes: WARNING - Wire/SPI source dirs not found under %s"
          % framework_dir)
