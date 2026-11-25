# PlatformIO pre-build script (core2): force main.cpp to recompile every build, which
# refreshes its __DATE__/__TIME__ stamp. The device sets its clock from that stamp on the
# first boot of each new build, so a stale stamp would set it wrong.
#
# Touching the source is not enough: SCons decides what to rebuild from content checksums,
# not modification times. Deleting the compiled object leaves it no choice.
import os

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

obj = os.path.join(env.subst("$BUILD_DIR"), "src", "core2", "main.cpp.o")  # noqa: F821
if os.path.exists(obj):
    os.remove(obj)
