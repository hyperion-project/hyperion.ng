#!/usr/bin/env python3
import platform
import subprocess
import sys
import argparse
import multiprocessing
import shutil
import os

# Argument parser
parser = argparse.ArgumentParser(description="Qt CMake Preset Runner")
parser.add_argument("--qt5", action="store_true", help="Use Qt5 (default is Qt6)")
parser.add_argument("-r", action="store_true", help="Use release build (default is debug)")
parser.add_argument("variant", nargs="?", default="all", choices=["all", "light", "minimum"], help="Build variant (all, light, minimum)")
parser.add_argument("--preset", action="store_true", help="Run cmake with --preset")
parser.add_argument("--build", action="store_true", help="Run cmake --build build --parallel <n>")
parser.add_argument("--full", action="store_true", help="Run cmake preset + build (requires --clean)")
parser.add_argument("--clean", action="store_true", help="Delete build directory before configuration")
parser.add_argument("-n", type=int, help="Number of CPUs for parallel build")

args = parser.parse_args()

# Require at least one action
if not (args.preset or args.build or args.full):
    print("❌ You must specify at least one of: --preset, --build, or --full")
    parser.print_help()
    sys.exit(1)

# Automatically enable --clean if --full or --preset is passed without it
if (args.full or args.preset) and not args.clean:
    print("ℹ️  --full or --preset implies --clean. Cleaning build directory...")
    args.clean = True

# Build path and parallelism
build_dir = "build"
parallel = args.n if args.n else multiprocessing.cpu_count()

# If only --build is given
if args.build and not args.full:
    print(f"▶ Running cmake --build {build_dir} --parallel {parallel}")
    try:
        subprocess.run(["cmake", "--build", build_dir, "--parallel", str(parallel)], check=True)
    except subprocess.CalledProcessError as e:
        print(f"❌ Build failed: {e}")
        sys.exit(e.returncode)
    sys.exit(0)

# Configure preset name
qt_version = "qt5" if args.qt5 else "qt6"
build_type = "release" if args.r else "debug"
variant = args.variant.lower()

system = platform.system()
if system == "Windows":
    suffix = "-windows"
elif system == "Linux":
    suffix = "-linux"
else:
    print(f"❌ Unsupported platform: {system}")
    sys.exit(1)

preset_name = f"{build_type}-{qt_version}-{variant}{suffix}"

# Handle --clean (used by --preset or --full)
if args.clean:
    if os.path.exists(build_dir):
        print(f"🧹 Deleting build directory: {build_dir}")
        shutil.rmtree(build_dir)

# Run cmake --preset (with --fresh if cleaned)
if args.preset or args.full:
    cmake_cmd = ["cmake", "--preset", preset_name]
    if args.clean:
        cmake_cmd.append("--fresh")

    print(f"▶ Running {' '.join(cmake_cmd)}")
    try:
        subprocess.run(cmake_cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"❌ CMake configure failed: {e}")
        sys.exit(e.returncode)

# Follow-up build if --full
if args.full:
    print(f"▶ Running cmake --build {build_dir} --parallel {parallel}")
    try:
        subprocess.run(["cmake", "--build", build_dir, "--parallel", str(parallel)], check=True)
    except subprocess.CalledProcessError as e:
        print(f"❌ Build failed: {e}")
        sys.exit(e.returncode)

