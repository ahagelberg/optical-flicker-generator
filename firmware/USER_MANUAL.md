# Optical Flicker Generator - User Manual

## Purpose

This device generates controlled LED light output for optical flicker experiments.  
You can operate it from a browser on your local network, or via one of several APIs.

---

## Quick Start

1. Connect the hardware:
   - Connect Ethernet to your LAN.
   - Power the device.
2. Wait for startup to complete (display shows mode and settings).
3. Open the control page:
   - Use the IP shown on the device display, or
   - Try `http://flicker-xxxxxx.local/` (where `xxxxxx` is the device suffix)
4. Use the **Control Page** to set mode and output parameters.
5. If needed, open **Configuration** from the control page to change network settings.

---

## Accessing the Web Interface

- Main control page: `GET /`  
- Configuration page: `GET /config`

If `*.local` names do not resolve on your computer, use the device IP address shown on the display.

---

## Using the Control Page

The control page is for direct use and testing.

### Mode Selection

You can select one of four modes:

- **Off**: output disabled
- **Constant**: continuous light at selected intensity
- **Flicker**: square-wave modulation using frequency + duty cycle + intensity
- **Sinus**: sinusoidal modulation using frequency + intensity

### Parameters

- **Frequency (Hz):** 1-500
- **Duty cycle (%):** 10-90
- **Intensity (%):** 0-100

Click **Apply** to send the selected mode and parameter values to the device.

### Typical Workflow

1. Choose mode (`flicker`, `sinus`, `constant`, or `off`)
2. Set frequency and intensity (and duty for flicker)
3. Click **Apply**
4. Verify output on the device and experiment setup

---

## Using the Configuration Page

Open `/config` from the control page when you need to change network or hardware-related settings.

### Network Settings

- **Use DHCP** (checkbox)
  - Enabled: router assigns IP automatically
  - Disabled: device uses static values entered below
- **Static IP settings** (used when DHCP is disabled):
  - IP address
  - Gateway
  - Subnet mask

### Connectivity Settings

- **Enable Telnet access (port 23)**  
  Disabled by default. Enable only on trusted networks.

### Hardware/Display Settings

- **PWM carrier frequency (Hz):** 1,000-50,000
- **Display screensaver timeout (s):** 1-600

Click **Save** to store changes.

### When Changes Take Effect

- Network-related changes are applied after a power cycle.
- Other saved settings are applied immediately.

---

## Factory Reset (Set Button)

- **Short press:** wakes display from screensaver.
- **Press and hold for 10 seconds:** restores network and configuration defaults.

---

## Troubleshooting

### Cannot open the control page

- Check Ethernet cable and LAN connection.
- Confirm the device has an IP address on the display.
- Try opening the page by IP instead of `*.local` hostname.

### Wrong or no light output

- Confirm mode is not `off`.
- Verify parameter ranges on the control page.
- Click **Apply** after any change.

### Lost access after network changes

- Power-cycle the device.
- If static IP is misconfigured, perform factory reset and reconnect using DHCP defaults.

