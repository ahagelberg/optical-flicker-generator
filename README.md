# Optical Flicker Generator

Optical Flicker Generator is a complete hardware + firmware device for controlled high-power optical stimulation.

Current hardware platform:

- Arduino MKR Zero
- Arduino Ethernet Shield
- Custom PCB with high-current LED driver stages

Current software provides:

- Web control page (`/`)
- Web configuration page (`/config`)
- HTTP JSON API
- ASCII shell over Serial and optional Telnet

## Project Scope

- Firmware source code is under `firmware/`.
- Hardware design files (schematics/PCB/manufacturing outputs) are under `hardware/`.

## Documentation

- User manual: `firmware/USER_MANUAL.md`
- Interface/protocol reference: `firmware/PROTOCOL.md`

## Getting Started

1. Flash the firmware to the device.
2. Connect power and Ethernet.
3. Open the control page in a browser using:
   - `http://flicker-<suffix>.local/`, or
   - the IP shown on the device display.

## Repository Layout

- `firmware/` - PlatformIO firmware source and documentation
- `hardware/` - Hardware design files
