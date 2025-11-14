# new-huxley

Embedded Raspberry Pi 4 Client-Server application

## Overview

new-huxley is a lightweight client-server application written in C++ targeting the Raspberry Pi 4. It provides a foundation for building embedded networked systems where a Raspberry Pi can act as either the server or the client. The repository contains source code, build scripts (Makefiles and CMake), and documentation to build, run, and extend the system.

## Features

- C++17 codebase optimized for Raspberry Pi 4
- Simple client / server architecture (TCP-based by default)
- Build support via Makefile and CMake
- Designed for easy extension to add sensors, actuators or services

> Note: The README makes conservative assumptions about network/protocol details. If your project uses UDP, custom protocols, or additional services (MQTT, TLS), update the sections below accordingly.

## Repository layout

- src/ - C++ source files
- include/ - public headers (if present)
- build/ - recommended out-of-source build directory
- tests/ - unit or integration tests (if present)
- Makefile, CMakeLists.txt - top-level build files

## Requirements

- Raspberry Pi 4 (or a compatible ARM64/ARMv7 device)
- GNU toolchain (glibc with C++ support)
- cmake (optional, for CMake-based builds)
- make (for the Makefile flow)
- Optional: cross-compilation toolchain if building from a non-ARM machine

## Building on the Raspberry Pi (native build)

1. Update and install build dependencies:

   sudo apt update && sudo apt install -y build-essential cmake git

2. Clone the repository:

   git clone https://github.com/jssilvaa/new-huxley.git
   cd new-huxley

3. Build with Makefile:

   make

   or using CMake (out-of-source recommended):

   mkdir -p build && cd build
   cmake ..
   cmake --build . --config Release

The produced binaries will appear in the build/ or bin/ directory depending on project CMake or Makefile conventions.

## Cross-compiling (from x86_64 Linux)

If you want to build on an x86_64 machine for the Pi, set up a cross toolchain and point CMake at it. The exact setup depends on your toolchain (e.g. gcc-arm-linux-gnueabihf or aarch64 toolchain for 64-bit OS). A simple flow:

1. Install a cross compiler (example for 32-bit armhf):

   sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

2. Configure a CMake toolchain file (toolchain.cmake) and run:

   cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..

Refer to CMake cross-compilation documentation or your CI configuration for exact details.

## Running

### Cross-compiled target build

```bash
make                # builds huxley using the Buildroot aarch64 toolchain
scp huxley ...      # or `make deploy`
```

### Host simulation + CLI harness

You can exercise the entire stack on your development machine before deploying:

```bash
make host           # native g++ build (outputs huxley_host)
make host-run       # start the host binary on port 8080 (--no-block)
make cli-run        # open the interactive Python CLI (client/client.py)
make test-sim       # automated harness: starts host server + scripted clients
```

- `make test-sim` launches `huxley_host` on port 9090, runs `tests/cli_harness.py` to
   register two ephemeral users, log them in, attempt authorized/unauthorized sends,
   and logs the transcript to `/tmp/huxley_sim.log`.
- Open additional terminals and run `python client/client.py localhost 8080` to run
   interactive slash commands; see `client/README.md` for details.

Command-line flags for the C++ server can be listed with `./huxley_host --help`.

## Configuration

- Default port, host, and logging settings are typically configurable via command-line flags or a config file. Check source or binary --help.
- If your application communicates over the network, ensure firewalls permit the configured port.

## Network and Security

- This project does not enable encryption or authentication by default. For production or exposed networks, add TLS (e.g., using OpenSSL or mbedTLS) and authentication.
- Consider using systemd to run server components as services and configure automatic restart on failure.

## Debugging and Logging

- Use -v/--verbose or logging configuration if the binaries expose it.
- Run under gdb or use core dumps for low-level debugging.

## Testing

- If tests exist under tests/, run them after building (e.g., ctest or make test).
- Add unit tests for networking, serialization, and platform-specific behavior.

## Troubleshooting

- Build failures: ensure you have the correct toolchain and dependencies installed
- Runtime networking issues: verify IP addresses, port availability, and firewall settings

## License

## Contact

Maintainer: jssilvaa (https://github.com/jssilvaa)
            vascoolify (https://github.com/vascoolify)
