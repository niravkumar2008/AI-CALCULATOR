# Tester firmware (Stage 4)

> **This is the old breadboard tester (ESP32-S3-CAM dev board + buttons), not the board firmware.** The firmware for the custom board inside the Casio is **`../firmware-prototype/`**; never flash this one to that board.

Runs the full calculator (the same code as the browser and Windows simulators) on the real tester:
ESP32-S3-CAM (OV3660 camera) + Waveshare 2.13" e-paper HAT V4 + 4 buttons.
Wire it first with the hole-by-hole checklist in the Build Book.

## 1. Install (once)

1. Install [VS Code](https://code.visualstudio.com).
2. In VS Code: Extensions (left bar) → search **PlatformIO IDE** → Install. Restart VS Code when asked.

## 2. Upload

1. **File → Open Folder…** and pick `AI-CALCULATOR\firmware` (this folder, not the repo root).
2. Plug a USB-C **data** cable into the board's port marked **COM** (or UART).
3. Click the **→ Upload** arrow in the blue bar at the bottom. The first build downloads the ESP32 tools and takes 5–10 minutes; later builds take seconds.
4. When it says `SUCCESS`, press the board's **RST** button.

If upload fails with "Failed to connect" or "Wrong boot mode": hold **BOOT**, tap **RST**, let go of BOOT, and click Upload again. Press RST once it finishes.

If Windows doesn't show a COM port at all: try a different cable (many only charge), or install the **CH340** USB driver from the chip maker, WCH.

## 3. Set up Wi-Fi and your key (once)

1. Turn on your phone's hotspot. iPhone: Settings → Personal Hotspot → turn on **Maximize Compatibility** (the chip only uses 2.4 GHz).
2. In VS Code, click the **plug icon** (Serial Monitor) in the bottom bar.
3. Type `wifi` and press Enter, then the hotspot name, then the password.
4. Type `key`, press Enter, paste your Claude API key, press Enter.
5. Type `status` to check: camera found, Wi-Fi connected, key saved.

What you type isn't echoed back, so your key never appears on screen. Settings survive power-off and re-uploads. `forget` erases them.

| Command | Does |
| --- | --- |
| `wifi` | Set hotspot name and password |
| `key` | Set the Claude API key |
| `status` | Camera, Wi-Fi, key, free memory |
| `preview` | Live camera view + tuning + **Send to Claude** (scans what the camera sees and shows the answer on the page; the board steps onto the hotspot for a few seconds, then comes back) + a live **Focus** number (highest = sharpest distance), and the last scan photo and its close-up. The board makes its own network (name, password and link printed); join it from the laptop. `preview off` stops, 15 min limit |
| `scan` | List the Wi-Fi networks the board can see (checks the hotspot name) |
| `snap` | Take a test photo and report its size and time |
| `forget` | Erase hotspot and key |
| `keys ...` | Press calculator keys, e.g. `keys 1/3=` (list below) |
| `exam off` | End exam mode (the USB cable is the teacher's unlock) |

## How a scan photo is taken

The photo matters most, so a scan takes its time (about 10 s; the screen counts down):
1. **Careful scan** (on by default): tries exposure, gain and contrast on the real page and keeps
   the best for the light (see below).
2. **Hold still** wait (1.5 s by default; "Hold-still wait" on the preview page, 0.2-3 s).
3. **8 frames over 2 s**; keeps the one whose writing is sharpest.
4. **Zoom + enhance**: the writing is cut out at full resolution (or the whole frame when it fills
   it), the lighting evened out, contrast raised, enlarged to ~1.15 MP and sharpened. Claude gets
   the photo plus this enhanced copy.
With an **OV5640** (5 MP) the photo is 2560x1920, already as much detail as Claude reads, so steps 3-4
become 4 frames and no close-up: just the sharpest photo is sent.
Claude always gives its best guess; unsure answers show as "BEST GUESS (62% sure)" with what to
check. Press = to retake.

**Careful scan** (on by default; turn off on the preview page with "Careful scan" = 0, then Save for scans): before each photo it
tries 5 exposure levels, extra gain for dim rooms and 3 contrast levels on the real page, scores
each photo for clean ink edges, bright-but-not-washed-out paper and dark ink, and keeps the best.
About 5 s slower. "Tune for this light now" on the page does the same once, without scanning.

## 4. Use

The tester starts in **AI SOLVE** (MODE 4). It has 5 buttons:

| Key | Does |
| --- | --- |
| `=` | Take the photo / retake / retry; on the calculator screen: calculate |
| `AC` | Back or cancel. On an empty calculator screen: back to AI SOLVE (the tester has no MODE key) |
| ▲ ▼ | On the AI SOLVE screen: effort (Normal / Careful / Max: slower, better at tricky math). On an answer: scroll (hold to keep scrolling). On the calculator screen: history |
| BOOT (on the board) | ON / OFF |

After a scan of a calculation, `AC` takes you to the calculator screen with the scanned sum typed in: press `=` to check it.

**Every other key** can be pressed from the Serial Monitor with `keys`, one character per key:

| Type | Key | Type | Key | Type | Key |
| --- | --- | --- | --- | --- | --- |
| `0`–`9` `.` | digits | `+ - * /` | + − × ÷ | `^` | xⁿ |
| `(` `)` | brackets | `=` | = | `~` | (−) |
| `s` `c` `t` | sin cos tan | `l` `n` | log ln | `r` `q` | √ x² |
| `i` | x⁻¹ | `f` | fraction | `E` | ×10ˣ |
| `h` | hyp | `k` | RCL | `w` | S⇔D |
| `M` | M+ | `a` | Ans | `#` | DEL |
| `$` | AC | `m` | MODE | `o` | ON |
| `[` | SHIFT | `]` | ALPHA | `< > u d` | ◀ ▶ ▲ ▼ |
| `p` | π | `!` | x! | `%` `,` | % and comma |

Examples: `keys 1/3=` (1⌟3), `keys s30)=` (sin 30), `keys [m1` (SETUP → Deg), `keys m1` (back to COMP), `keys m4` (AI SOLVE), `keys [m8=` (exam mode on), `exam off`.

The Serial Monitor logs each key and each photo: size, time to the answer, time to done. That's the data for the camera distance test (15, 25, 35 cm).

Exam mode turns Wi-Fi off, is saved to flash every minute, and comes back on after a restart; only `exam off` over USB or 12 hours ends it.

## Troubleshooting

| You see | Try |
| --- | --- |
| E-paper stays blank | Check the 8 e-paper wires against the checklist; the Serial Monitor still works |
| `Camera: not found` | Re-seat the camera ribbon (flip the latch up, push the ribbon fully in, latch down) |
| NO CONNECTION | Hotspot on, Maximize Compatibility on, then `status` |
| "Secure connection failed" | Send me the exact text; the server certificate list may need a new entry |
| A button does nothing | Check its wire; pins are in `src/pins.h` |

## Files

```
platformio.ini        board, memory and library settings (N16R8: 16 MB flash, 8 MB PSRAM)
src/pins.h            every pin in one place (provisional until the board's labels are checked)
src/main.cpp          buttons, serial keys, Wi-Fi, exam radios, and the task that captures + sends each photo
src/camera.*          finds the camera wiring, takes a grayscale 1600x1200 JPEG
src/screen.*          draws the 125x61 calculator screen on the 250x122 e-paper (Plain or LCD dots)
src/claude_client.*   HTTPS to Claude with streaming, same request as the simulator
src/settings.*        hotspot, key and exam state in flash, serial commands
src/root_ca.h         certificates used to verify api.anthropic.com
../core/              shared calculator logic (the same code the simulator runs)
```
