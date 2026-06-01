# Optical Flicker Generator — Interface & Protocol Reference

## Overview

The device exposes three interfaces:


| Interface   | Transport         | Port   | Format             |
| ----------- | ----------------- | ------ | ------------------ |
| ASCII shell | USB Serial (CDC)  | —      | Line-oriented text |
| ASCII shell | Ethernet / Telnet | TCP 23 | Same as Serial     |
| HTTP API    | Ethernet          | TCP 80 | JSON               |


All LED control goes through `FlickerController`, so frequency calibration applies uniformly regardless of which interface is used. The output is two hardware channels (MKR Zero D2 and D3) driven in lockstep with the same PWM parameters; there is no per-channel control.

---

## ASCII Shell (Serial + Telnet)

### Framing

- One command per line, terminated by `\n` or `\r\n`.
- Command verb is **case-insensitive**; arguments are case-sensitive where noted.
- Response is a single line terminated by `\r\n` (Arduino `println`).
- Max line length: 128 bytes (excess bytes are discarded; the line is dropped).
- Partial lines with no new data for 5 s are silently discarded.

### Response format

```
OK                     # success, no payload
OK <payload>           # success with data
ERROR <reason>         # failure; reason is human-readable
```

Errors always begin with the literal string `ERROR`  followed by a short description. Scripts should match on the prefix.

---

### Commands

#### `identify`

Query device identity.

```
→ identify
← OK FLICKER_DEVICE 1.0 A0B1C2D3E4F56789A0B1C2D3E4F56789
```

Payload: `<device_type> <firmware_version> <id>`  
`<id>` is the SAMD21 128-bit hardware serial number encoded as 32 uppercase hex chars.

---

#### `mode [value]`

Query or set the operating mode.

```
→ mode
← OK flicker

→ mode flicker
← OK
```

Valid modes: `off`, `constant`, `flicker`, `sinus`  
Changing mode keeps existing frequency/duty/intensity values; only the mode changes.


| Mode       | Description                                                 |
| ---------- | ----------------------------------------------------------- |
| `off`      | LED output disabled                                         |
| `constant` | Continuous illumination at the configured intensity         |
| `flicker`  | Square-wave envelope at configured frequency and duty cycle |
| `sinus`    | Sinusoidal envelope at configured frequency                 |


---

#### `frequency` / `freq [value]`

Query or set the flicker/sinus frequency in Hz.

```
→ frequency
← OK 50

→ freq 10
← OK

→ freq 0
← ERROR out of range (1-500 Hz)
```

Range: 1–500 Hz  
The value is the **desired** physical Hz; `FlickerController` applies the calibration table before commanding the hardware. Setting frequency does not change mode.

---

#### `dutycycle` / `duty [value]`

Query or set the duty cycle percentage (flicker mode only; stored for other modes too).

```
→ duty
← OK 50

→ duty 30
← OK

→ duty 5
← ERROR out of range (10-90 %)
```

Range: 10–90 %

---

#### `intensity` / `int [value]`

Query or set the LED intensity percentage.

```
→ intensity
← OK 100

→ int 75
← OK
```

Range: 0–100 %

---

#### `carrier [value]`

Query or set the PWM carrier frequency in Hz. The carrier drives intensity modulation in `constant` and `sinus` modes; it has no effect in `flicker` mode. The value is persisted to EEPROM immediately.

```
→ carrier
← OK 20000

→ carrier 10000
← OK

→ carrier 500
← ERROR out of range (1000-50000 Hz)
```

Range: 1 000–50 000 Hz

---

#### `calibration`

Print the stored frequency calibration table.

```
→ calibration
← OK CAL 0
```

*(no calibration stored)*

```
→ calibration
← OK CAL 8 1:1;5:5;10:10;25:26;50:51;100:103;200:206;500:515
```

*(8 points stored)*

Payload format: `OK CAL <count> [cmd1:meas1;cmd2:meas2;…]`  

- `count` — number of stored calibration points.  
- Each pair is `<commanded_hz>:<measured_hz>` separated by `;`.

---

#### `calibrate`

Start the interactive frequency calibration wizard.

The wizard drives the LED at each of 8 nominal frequencies (1, 5, 10, 25, 50, 100, 200, 500 Hz) in sequence. At each step, the device replies with a prompt; the user measures the actual output frequency and sends it as a plain integer.

**Wizard session:**

```
→ calibrate
← OK WIZARD 1/8 1        # step/total, target Hz

→ 1                       # measured Hz (integer)
← OK WIZARD 2/8 5

→ 5
← OK WIZARD 3/8 10

→ 11
← OK WIZARD 4/8 25
…
→ 519
← OK WIZARD DONE          # table saved, prior mode restored
```

**Wizard prompt format:** `OK WIZARD <step>/<total> <target_hz>`

While the wizard is active, the shell is in measurement mode — any line that is not a valid unsigned integer receives `ERROR expected measured Hz as integer` and the step is re-prompted. No other commands are accepted until the wizard completes.

The wizard forces `flicker` mode during measurement (for a stable square-wave envelope) while preserving the user's configured duty cycle and intensity. Calibration bypass is enabled so raw Hz values drive the hardware. On completion, the prior mode and frequency are restored and calibration is applied immediately.

**Errors:**

```
→ calibrate
← ERROR wizard already active   # sent while wizard is in progress
```

---

### Error reference


| Message                                           | Cause                                     |
| ------------------------------------------------- | ----------------------------------------- |
| `ERROR empty command`                             | Line contained no token                   |
| `ERROR unknown command`                           | Verb not recognised                       |
| `ERROR invalid argument`                          | Argument not parseable                    |
| `ERROR unknown mode (off constant flicker sinus)` | Unrecognised mode name                    |
| `ERROR out of range (1-500 Hz)`                   | Frequency outside 1–500 Hz                |
| `ERROR out of range (10-90 %)`                    | Duty cycle outside 10–90 %                |
| `ERROR out of range (0-100 %)`                    | Intensity outside 0–100 %                 |
| `ERROR out of range (1000-50000 Hz)`              | Carrier frequency outside 1 000–50 000 Hz |
| `ERROR wizard already active`                     | `calibrate` sent while wizard running     |
| `ERROR expected measured Hz as integer`           | Non-integer sent during wizard            |


---

## HTTP / JSON API

Base URL: `http://<device-ip>/`

Responses use `Content-Type: application/json` and `Connection: close`.

### `GET /api/identify`

```json
{
  "device": "FLICKER_DEVICE",
  "firmware": "1.0",
  "id": "A0B1C2D3E4F56789A0B1C2D3E4F56789"
}
```

---

### `GET /api/state`

Returns the current mode and all parameters.

```json
{
  "mode": "flicker",
  "frequency": 50,
  "dutycycle": 50,
  "intensity": 100
}
```

`frequency` reflects the user's **desired** Hz value, not the calibrated command value sent to hardware.

---

### `POST /api/off`

Turns the LED off. No request body required.

```json
{ "ok": true }
```

---

### `POST /api/constant`

Sets constant mode. Optional JSON body applies parameters before switching.

```json
{ "intensity": 75 }
→
{ "ok": true }
```

---

### `POST /api/flicker`

Sets flicker mode. Optional JSON body applies parameters before switching.

```json
{ "frequency": 10, "dutycycle": 30, "intensity": 80 }
→
{ "ok": true }
```

JSON keys: `frequency` (Hz, 1–500), `dutycycle` (%, 10–90), `intensity` (%, 0–100). All are optional; omit any key to keep the current value.

---

### `POST /api/sinus`

Sets sinus mode. Optional JSON body.

```json
{ "frequency": 25, "intensity": 100 }
→
{ "ok": true }
```

---

### `GET /api/frequency`

```json
{ "frequency": 50 }
```

### `POST /api/frequency`

```json
{ "value": 25 }
→
{ "ok": true }
```

### `GET /api/dutycycle` / `POST /api/dutycycle`

Same pattern. POST body: `{ "value": 40 }`.

### `GET /api/intensity` / `POST /api/intensity`

Same pattern. POST body: `{ "value": 80 }`.

---

### HTTP error responses

```json
{ "error": "Not found" }      // 404 — unknown path
{ "error": "Bad request" }    // 400 — unknown API endpoint
```

Note: the mode-set POST endpoints (`/api/flicker`, `/api/sinus`, etc.) do **not** validate individual parameter ranges — out-of-range values are silently clamped by `FlickerController`. Use the ASCII shell if you need explicit range errors.

---

### Web UI

`GET /` — HTML control page for interactive use.  
`GET /config` — HTML network configuration page (DHCP, static IP/gateway/subnet, PWM carrier frequency).  
`POST /config` — Submit configuration form. Changes take effect on next boot for network settings.

---

## Frequency Calibration

### Concept

Hardware timer resolution means the actual output frequency deviates slightly from the commanded value, especially at the low and high ends of the range. The calibration table stores (commanded Hz, measured Hz) pairs from a one-time wizard run. When a desired frequency `D` is requested, `FlickerController` computes the command value `C` such that the hardware actually outputs `D` Hz, using piecewise-linear inverse interpolation over the stored pairs.

### Math

Points are stored sorted by commanded Hz (the wizard steps are monotonically increasing). For a desired frequency `D`:

1. If the table has fewer than 2 points → identity (`C = D`).
2. If `D ≤ meas[0]` → clamp to `cmd[0]`.
3. If `D ≥ meas[n-1]` → clamp to `cmd[n-1]`.
4. Otherwise find `i` where `meas[i] ≤ D ≤ meas[i+1]` and interpolate:

```
C = cmd[i] + (D - meas[i]) × (cmd[i+1] - cmd[i]) / (meas[i+1] - meas[i])
```

Integer arithmetic; uses 64-bit intermediate to avoid overflow.

### Persistence

The calibration table is stored in EEPROM (SAMD21 flash emulation via `FlashStorage_SAMD`) as part of `StoredConfig` (magic `0x464C4353`). It is treated as factory data and is **not** cleared by a factory reset. Re-running `calibrate` replaces the entire table.

---

## Network configuration

Configured via the `/config` web page or directly in EEPROM. Stored fields:


| Field          | Default |
| -------------- | ------- |
| DHCP           | enabled |
| Static IP      | 0.0.0.0 |
| Gateway        | 0.0.0.0 |
| Subnet         | 0.0.0.0 |
| PWM carrier Hz | 20000   |


The PWM carrier frequency (1 000–50 000 Hz) controls the intensity modulation rate in `constant` and `sinus` modes. It has no effect in `flicker` mode.

Network changes take effect after a power cycle.

---

## mDNS discovery

The device advertises itself on the LAN via **mDNS/DNS-SD** (UDP port 5353).

| Item | Value |
| ---- | ----- |
| Hostname | `flicker-` + last 6 hex digits of device serial (e.g. `flicker-a1b2c3.local`) |
| Service | `_http._tcp` on port 80 |

After the device has an IP address, open `http://flicker-<suffix>.local/` in a browser, or ping the hostname:

```
ping flicker-a1b2c3.local
```

**Linux:** `avahi-browse -a | grep -i flicker`

**macOS:** mDNS is built in; Safari Bonjour bookmarks also list `_http._tcp` services.

**Windows:** Built-in mDNS support varies by version. Install [Bonjour Print Services](https://support.apple.com/kb/DL999) if `.local` names do not resolve.

mDNS is always enabled when Ethernet has a valid IP. No configuration required.

---

## Set button

Short press on set button wakes the display from screensaver.

Hold set button for **10 s** to perform a factory reset: network settings and PWM carrier Hz are cleared to defaults. The frequency calibration table is **preserved**.