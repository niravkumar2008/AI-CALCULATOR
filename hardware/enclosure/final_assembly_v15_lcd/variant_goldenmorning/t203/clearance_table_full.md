# Full clearance table: every part vs every shell feature (closed case, grinds applied)

Generated 2026-10-09 00:12 by `build_final_assembly_v15.py fulltable`. Board STEP 2026-10-08 21:46:13. Minimum distance per body pair (Fusion measureMinimumDistance; pairs whose boxes are more than 3 mm apart are not listed). FAIL = gap under 0.2 mm or an overlap, unless the contact is intended. Positions are front-view X, Y (mm from the board centre, display up) and Z from the outside of the back cover; KiCad x = 150 - X, y = 138.94 - Y.

| Part | Body | Shell part | Shell body | Gap (mm) | At (front X, Y, Z) | KiCad x, y | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- |
| LiPo battery | LiPo pouch (Adafruit #1317) | Back cover | Back cover | 0.00 | -17.2, 77.1, 1.0 | 167.2, 61.8 | PASS (intended contact: rests on the floor (0.1 mm tape under it, not modelled)) |
| PCB | board | Front shell | Front shell | 0.10 | 19.4, -46.0, 7.7 | 130.6, 184.9 | FAIL (< 0.2 mm) |
| PCB | board | Keymat | Keymat (rubber) | 0.11 | 20.9, -46.5, 7.7 | 129.1, 185.5 | PASS (intended contact: mat lies on the key side of the board (0.11 in the model)) |
| Magnet connector | Magnet face flange | Front shell | Front shell | 0.25 | 29.5, 80.7, 9.2 | 120.5, 58.3 | PASS (intended contact: face glued in the U-notch (0.25 per side, glue fills it)) |
| LiPo battery | LiPo pouch (Adafruit #1317) | Front shell | Front shell | 0.30 | -12.4, 67.9, 4.8 | 162.4, 71.0 | PASS |
| PCB | J4 | Back cover | Back cover | 0.41 | 8.7, 63.5, 1.4 | 141.3, 75.4 | PASS |
| LiPo battery | LiPo pouch (Adafruit #1317) | Battery lid | Battery lid | 0.42 | -12.3, 62.6, 1.0 | 162.3, 76.3 | PASS |
| Ribbons and wires | Camera FPC | Back cover | Back cover | 0.47 | 0.4, 20.2, 5.5 | 149.6, 118.7 | PASS |
| Ribbons and wires | LCD FPC | Back cover | Back cover | 0.55 | -29.8, 44.2, 2.5 | 179.8, 94.7 | PASS |
| Magnet connector | Magnet plate | Front shell | Front shell | 0.55 | 29.3, 80.7, 8.9 | 120.7, 58.3 | PASS |
| PCB | D2 | Back cover | Back cover | 0.66 | -6.8, -14.4, 5.7 | 156.8, 153.4 | PASS |
| LCD panel | LCD top polarizer | Front shell | Front shell | 0.67 | 22.7, 33.7, 9.9 | 127.3, 105.2 | PASS |
| Camera module | Camera lens barrel | Back cover | Back cover | 0.71 | 3.0, 43.8, 1.5 | 147.0, 95.1 | PASS |
| Magnet connector | Magnet face flange | Back cover | Back cover | 0.71 | 15.5, 80.7, 2.2 | 134.5, 58.3 | PASS |
| Ribbons and wires | LiPo lead black | Back cover | Back cover | 0.79 | -2.0, 67.1, 1.8 | 152.0, 71.8 | PASS |
| LCD panel | LCD CF glass | Front shell | Front shell | 0.80 | 23.0, 58.2, 9.8 | 127.0, 80.7 | PASS |
| LiPo battery | JST-PH plug (in J4) | Back cover | Back cover | 0.80 | 11.4, 58.7, 2.0 | 138.6, 80.3 | PASS |
| Ribbons and wires | LCD FPC | Front shell | Front shell | 0.83 | -30.2, 37.6, 9.8 | 180.2, 101.3 | PASS |
| LCD panel | LCD top polarizer | Window mask | Window mask (black vinyl, inside the lens) | 0.88 | 22.7, 57.9, 9.9 | 127.3, 81.0 | PASS |
| PCB | R10 | Keymat | Keymat (rubber) | 0.91 | 15.9, -24.7, 6.9 | 134.1, 163.7 | PASS |
| PCB | R19 | Keymat | Keymat (rubber) | 0.91 | 8.9, -26.7, 6.9 | 141.1, 165.7 | PASS |
| PCB | C18 | Keymat | Keymat (rubber) | 0.91 | 12.6, -26.8, 6.9 | 137.4, 165.7 | PASS |
| PCB | C13 | Keymat | Keymat (rubber) | 0.91 | 9.8, -16.0, 6.9 | 140.2, 154.9 | PASS |
| PCB | C17 | Keymat | Keymat (rubber) | 0.91 | -7.5, -17.5, 6.9 | 157.5, 156.4 | PASS |
| PCB | R17 | Keymat | Keymat (rubber) | 0.91 | 26.5, 21.1, 6.9 | 123.5, 117.8 | PASS |
| PCB | C16 | Keymat | Keymat (rubber) | 0.91 | -7.5, -13.1, 6.9 | 157.5, 152.0 | PASS |
| PCB | C31 | Keymat | Keymat (rubber) | 0.91 | 20.4, -5.8, 6.9 | 129.6, 144.7 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -5.8, -22.2, 6.9 | 155.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -5.3, -22.2, 6.9 | 155.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -4.8, -22.2, 6.9 | 154.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -4.3, -22.2, 6.9 | 154.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -3.8, -22.2, 6.9 | 153.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -3.3, -22.2, 6.9 | 153.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -2.8, -22.2, 6.9 | 152.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -2.3, -22.2, 6.9 | 152.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -1.8, -22.2, 6.9 | 151.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -1.3, -22.2, 6.9 | 151.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -0.8, -22.2, 6.9 | 150.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | -0.3, -22.2, 6.9 | 150.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 0.2, -22.2, 6.9 | 149.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 0.7, -22.2, 6.9 | 149.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 1.2, -22.2, 6.9 | 148.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 1.7, -22.2, 6.9 | 148.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 2.2, -22.2, 6.9 | 147.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 2.7, -22.2, 6.9 | 147.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 3.2, -22.2, 6.9 | 146.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 3.7, -22.2, 6.9 | 146.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 4.2, -22.2, 6.9 | 145.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 4.7, -22.2, 6.9 | 145.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 5.2, -22.2, 6.9 | 144.8, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 5.7, -22.2, 6.9 | 144.3, 161.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.91 | 6.4, -21.3, 6.9 | 143.6, 160.2 | PASS |
| PCB | R16 | Keymat | Keymat (rubber) | 0.91 | 23.8, -2.5, 6.9 | 126.2, 141.5 | PASS |
| PCB | FB1 | Keymat | Keymat (rubber) | 0.91 | 13.4, -15.8, 6.9 | 136.6, 154.8 | PASS |
| PCB | U5 | Keymat | Keymat (rubber) | 0.91 | 13.9, -22.9, 6.9 | 136.1, 161.8 | PASS |
| PCB | C10 | Keymat | Keymat (rubber) | 0.91 | 15.2, -18.6, 6.9 | 134.8, 157.5 | PASS |
| PCB | C12 | Keymat | Keymat (rubber) | 0.91 | 8.6, -18.6, 6.9 | 141.4, 157.5 | PASS |
| PCB | R15 | Keymat | Keymat (rubber) | 0.91 | 24.9, -2.5, 6.9 | 125.0, 141.5 | PASS |
| PCB | C15 | Keymat | Keymat (rubber) | 0.91 | 9.8, -22.2, 6.9 | 140.2, 161.1 | PASS |
| PCB | C11 | Keymat | Keymat (rubber) | 0.91 | 9.8, -18.6, 6.9 | 140.2, 157.5 | PASS |
| PCB | R14 | Keymat | Keymat (rubber) | 0.91 | 26.1, -2.5, 6.9 | 123.8, 141.5 | PASS |
| PCB | C14 | Keymat | Keymat (rubber) | 0.91 | 15.2, -22.2, 6.9 | 134.8, 161.1 | PASS |
| PCB | R13 | Keymat | Keymat (rubber) | 0.91 | 27.4, -2.5, 6.9 | 122.7, 141.5 | PASS |
| PCB | U4 | Keymat | Keymat (rubber) | 0.91 | 13.9, -19.3, 6.9 | 136.1, 158.2 | PASS |
| PCB | D2 | Keymat | Keymat (rubber) | 0.91 | -6.2, -15.4, 6.9 | 156.2, 154.3 | PASS |
| PCB | R9 | Keymat | Keymat (rubber) | 0.91 | 11.3, -26.7, 6.9 | 138.7, 165.7 | PASS |
| PCB | R18 | Keymat | Keymat (rubber) | 0.91 | 10.2, -26.7, 6.9 | 139.8, 165.7 | PASS |
| PCB | U6 | Keymat | Keymat (rubber) | 0.91 | 24.9, -8.6, 6.9 | 125.2, 147.6 | PASS |
| PCB | R8 | Keymat | Keymat (rubber) | 0.91 | 13.8, -26.7, 6.9 | 136.2, 165.7 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.94 | -7.5, -18.5, 6.9 | 157.5, 157.4 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 0.94 | 7.5, -18.5, 6.9 | 142.5, 157.4 | PASS |
| PCB | J1 | Back cover | Back cover | 0.95 | -0.5, -17.1, 4.9 | 150.5, 156.1 | PASS |
| LCD panel | LCD active area (marker) | Front shell | Front shell | 0.95 | -21.4, 34.5, 9.9 | 171.4, 104.4 | PASS |
| LCD panel | LCD active area (marker) | Window lens | Window lens (clear) | 0.96 | 21.4, 57.2, 9.9 | 128.6, 81.8 | PASS |
| LCD panel | LCD top polarizer | Window lens | Window lens (clear) | 0.98 | 22.7, 57.9, 9.9 | 127.3, 81.0 | PASS |
| LCD panel | LCD active area (marker) | Window mask | Window mask (black vinyl, inside the lens) | 0.99 | 21.4, 34.5, 9.9 | 128.6, 104.4 | PASS |
| LCD panel | LCD CF glass | Window mask | Window mask (black vinyl, inside the lens) | 1.01 | 23.0, 58.2, 9.8 | 127.0, 80.7 | PASS |
| LCD panel | LCD driver IC (COG) + sealant | Window mask | Window mask (black vinyl, inside the lens) | 1.01 | -23.1, 53.8, 9.8 | 173.1, 85.1 | PASS |
| Ribbons and wires | LCD FPC | Window mask | Window mask (black vinyl, inside the lens) | 1.01 | -24.3, 53.1, 9.8 | 174.3, 85.8 | PASS |
| Magnet connector | Magnet plate | Back cover | Back cover | 1.01 | 15.7, 80.0, 2.5 | 134.3, 59.0 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 1.05 | -6.6, -18.1, 6.8 | 156.6, 157.0 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 1.05 | 6.6, -18.1, 6.8 | 143.4, 157.0 | PASS |
| PCB | J1 | Back cover | Back cover | 1.08 | -0.7, -17.2, 5.2 | 150.7, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.08 | -0.3, -17.2, 5.2 | 150.3, 156.2 | PASS |
| PCB | U1 | Front shell | Front shell | 1.08 | 35.0, 48.2, 6.9 | 115.0, 90.7 | PASS |
| PCB | J1 | Back cover | Back cover | 1.10 | -1.2, -17.2, 5.2 | 151.2, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.10 | 0.2, -17.2, 5.2 | 149.8, 156.2 | PASS |
| LCD panel | LCD TFT glass (with driver ledge) | Front shell | Front shell | 1.10 | 23.0, 58.2, 9.5 | 127.0, 80.7 | PASS |
| LCD panel | LCD CF glass | Window lens | Window lens (clear) | 1.11 | 23.0, 58.2, 9.8 | 127.0, 80.7 | PASS |
| LCD panel | LCD driver IC (COG) + sealant | Window lens | Window lens (clear) | 1.11 | -23.1, 53.8, 9.8 | 173.1, 85.1 | PASS |
| Ribbons and wires | LCD FPC | Window lens | Window lens (clear) | 1.11 | -24.3, 53.1, 9.8 | 174.3, 85.8 | PASS |
| PCB | J1 | Back cover | Back cover | 1.13 | -1.7, -17.2, 5.2 | 151.7, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.13 | 0.7, -17.2, 5.2 | 149.3, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.18 | -2.2, -17.2, 5.2 | 152.2, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.18 | 1.2, -17.2, 5.2 | 148.8, 156.2 | PASS |
| PCB | C6 | Front shell | Front shell | 1.19 | 10.0, 73.4, 6.9 | 140.0, 65.5 | PASS |
| PCB | J3 | Front shell | Front shell | 1.19 | 27.8, 72.8, 6.9 | 122.2, 66.1 | PASS |
| PCB | U3 | Front shell | Front shell | 1.19 | 26.7, 61.4, 6.9 | 123.3, 77.5 | PASS |
| PCB | R3 | Front shell | Front shell | 1.19 | 16.5, 60.8, 6.9 | 133.5, 78.1 | PASS |
| PCB | D7 | Front shell | Front shell | 1.19 | 28.9, 65.0, 6.9 | 121.1, 73.9 | PASS |
| PCB | D6 | Front shell | Front shell | 1.20 | 21.5, 60.7, 6.9 | 128.5, 78.3 | PASS |
| PCB | Q1 | Front shell | Front shell | 1.21 | 15.0, 60.6, 6.9 | 135.0, 78.3 | PASS |
| PCB | U2 | Front shell | Front shell | 1.25 | 15.4, 72.6, 6.9 | 134.6, 66.3 | PASS |
| PCB | J1 | Back cover | Back cover | 1.25 | -2.7, -17.2, 5.2 | 152.7, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.25 | 1.7, -17.2, 5.2 | 148.3, 156.2 | PASS |
| PCB | Q2 | Front shell | Front shell | 1.27 | 12.9, 62.1, 6.9 | 137.1, 76.8 | PASS |
| LCD panel | LCD TFT glass (with driver ledge) | Window mask | Window mask (black vinyl, inside the lens) | 1.31 | 23.0, 58.2, 9.5 | 127.0, 80.7 | PASS |
| PCB | J1 | Back cover | Back cover | 1.34 | -3.2, -17.2, 5.3 | 153.2, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.34 | 2.2, -17.2, 5.3 | 147.8, 156.2 | PASS |
| PCB | C16 | Back cover | Back cover | 1.42 | -7.5, -12.8, 6.4 | 157.5, 151.7 | PASS |
| LCD panel | LCD TFT glass (with driver ledge) | Window lens | Window lens (clear) | 1.42 | 23.0, 58.2, 9.5 | 127.0, 80.7 | PASS |
| PCB | C8 | Front shell | Front shell | 1.44 | 25.0, 60.0, 6.9 | 125.0, 78.9 | PASS |
| PCB | J1 | Back cover | Back cover | 1.44 | -3.7, -17.2, 5.3 | 153.7, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.44 | 2.7, -17.2, 5.3 | 147.3, 156.2 | PASS |
| LCD panel | LCD backlight (frame + LED light guide) | Front shell | Front shell | 1.45 | 23.6, 58.7, 9.2 | 126.4, 80.2 | PASS |
| Camera module | Camera AF motor (VCM) | Back cover | Back cover | 1.50 | -3.5, 43.8, 2.5 | 153.5, 95.1 | PASS |
| PCB | J1 | Back cover | Back cover | 1.57 | -4.2, -17.2, 5.3 | 154.2, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.57 | 3.2, -17.2, 5.3 | 146.8, 156.2 | PASS |
| PCB | J4 | Front shell | Front shell | 1.58 | 6.7, 62.7, 6.9 | 143.3, 76.2 | PASS |
| PCB | board | Back cover | Back cover | 1.60 | -24.9, 15.7, 7.0 | 174.9, 123.2 | PASS |
| LCD panel | LCD backlight (frame + LED light guide) | Window mask | Window mask (black vinyl, inside the lens) | 1.66 | 23.6, 58.7, 9.2 | 126.4, 80.2 | PASS |
| LiPo battery | JST-PH plug (in J4) | Front shell | Front shell | 1.69 | 11.4, 61.6, 6.4 | 138.6, 77.3 | PASS |
| PCB | J1 | Back cover | Back cover | 1.70 | -4.7, -17.2, 5.3 | 154.7, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.70 | 3.7, -17.2, 5.3 | 146.3, 156.2 | PASS |
| PCB | J1 | Keymat | Keymat (rubber) | 1.71 | -6.3, -20.7, 6.1 | 156.3, 159.7 | PASS |
| Ribbons and wires | LiPo lead red | Back cover | Back cover | 1.72 | 9.4, 58.1, 3.7 | 140.6, 80.8 | PASS |
| LCD panel | LCD backlight (frame + LED light guide) | Window lens | Window lens (clear) | 1.76 | 23.6, 58.7, 9.2 | 126.4, 80.2 | PASS |
| PCB | R22 | Front shell | Front shell | 1.82 | -6.4, 59.6, 6.9 | 156.4, 79.3 | PASS |
| Magnet connector | Magnet rear block | Back cover | Back cover | 1.82 | 30.0, 78.7, 3.2 | 120.0, 60.3 | PASS |
| PCB | J4 | Front shell | Front shell | 1.83 | 11.7, 62.9, 6.8 | 138.3, 76.0 | PASS |
| PCB | J4 | Front shell | Front shell | 1.83 | 5.5, 62.9, 6.8 | 144.6, 76.0 | PASS |
| Ribbons and wires | Camera FPC | Keymat | Keymat (rubber) | 1.84 | -4.0, -18.9, 6.0 | 154.0, 157.8 | PASS |
| PCB | J3 | Front shell | Front shell | 1.85 | 26.6, 68.5, 6.9 | 123.4, 70.4 | PASS |
| PCB | J1 | Back cover | Back cover | 1.86 | -5.2, -17.2, 5.3 | 155.2, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 1.86 | 4.2, -17.2, 5.3 | 145.8, 156.2 | PASS |
| PCB | D1 | Front shell | Front shell | 1.88 | 19.6, 63.0, 6.8 | 130.4, 75.9 | PASS |
| PCB | board | Keycaps | Key Abs | 1.91 | -24.6, 3.3, 7.7 | 174.6, 135.6 | PASS |
| PCB | board | Keycaps | Key RCL | 1.91 | -24.2, -24.4, 7.7 | 174.2, 163.4 | PASS |
| PCB | board | Keycaps | Key a/b | 1.91 | -24.2, -5.5, 7.7 | 174.2, 144.4 | PASS |
| PCB | board | Keycaps | Key 1 | 1.91 | -22.8, -58.3, 7.7 | 172.8, 197.2 | PASS |
| PCB | board | Keycaps | Key 7 | 1.91 | -22.8, -36.0, 7.7 | 172.8, 175.0 | PASS |
| PCB | board | Keycaps | Key ALPHA | 1.91 | -20.3, 18.1, 7.7 | 170.3, 120.8 | PASS |
| PCB | board | Keycaps | Key o'' | 1.91 | -13.6, -14.9, 7.7 | 163.6, 153.8 | PASS |
| PCB | board | Keycaps | Key . | 1.91 | -18.9, -62.1, 7.7 | 168.9, 201.0 | PASS |
| PCB | board | Keycaps | Key 5 | 1.91 | -18.9, -39.8, 7.7 | 168.9, 178.8 | PASS |
| PCB | board | Keycaps | Key ( | 1.91 | -2.9, -24.4, 7.7 | 152.8, 163.4 | PASS |
| PCB | board | Keycaps | Key x^2 | 1.91 | -2.9, -5.5, 7.7 | 152.8, 144.4 | PASS |
| PCB | board | Keycaps | Key AC | 1.91 | 28.6, -36.0, 7.7 | 121.4, 175.0 | PASS |
| PCB | board | Keycaps | Key 6 | 1.91 | 2.9, -47.2, 7.7 | 147.1, 186.1 | PASS |
| PCB | board | Keycaps | Key ) | 1.91 | 7.8, -24.4, 7.7 | 142.2, 163.4 | PASS |
| PCB | board | Keycaps | Key x^n | 1.91 | 7.8, -5.5, 7.7 | 142.2, 144.4 | PASS |
| PCB | board | Keycaps | Key + | 1.91 | 15.8, -58.3, 7.7 | 134.2, 197.2 | PASS |
| PCB | board | Keycaps | Key DEL | 1.91 | 15.8, -36.0, 7.7 | 134.2, 175.0 | PASS |
| PCB | board | Keycaps | Key MODE | 1.91 | 11.7, 18.1, 7.7 | 138.3, 120.8 | PASS |
| PCB | board | Keycaps | Key cos | 1.91 | 18.5, -14.9, 7.7 | 131.5, 153.8 | PASS |
| PCB | board | Keycaps | Key log_a | 1.91 | 28.7, 3.3, 7.7 | 121.3, 135.6 | PASS |
| PCB | board | Keycaps | Key M+ | 1.91 | 29.1, -24.4, 7.7 | 120.9, 163.4 | PASS |
| PCB | board | Keycaps | Key ln | 1.91 | 29.1, -5.5, 7.7 | 120.9, 144.4 | PASS |
| PCB | board | Keycaps | Key - | 1.91 | 28.6, -58.3, 7.7 | 121.4, 197.2 | PASS |
| PCB | board | Keycaps | Key / | 1.91 | 20.8, -46.5, 7.7 | 129.2, 185.4 | PASS |
| PCB | board | Keycaps | Key = | 1.91 | 19.6, -62.1, 7.7 | 130.4, 201.0 | PASS |
| PCB | board | Keycaps | Key tan | 1.91 | 29.1, -14.9, 7.7 | 120.9, 153.8 | PASS |
| PCB | board | Keycaps | Key ON | 1.91 | 22.3, 18.1, 7.7 | 127.7, 120.8 | PASS |
| PCB | board | Keycaps | Key log | 1.91 | 20.6, -0.2, 7.7 | 129.4, 139.2 | PASS |
| PCB | board | Keycaps | Key S<>D | 1.91 | 18.5, -24.4, 7.7 | 131.5, 163.4 | PASS |
| PCB | board | Keycaps | Key x^-1 | 1.91 | 18.1, 3.3, 7.7 | 131.9, 135.6 | PASS |
| PCB | board | Keycaps | Key x | 1.91 | 8.0, -38.7, 7.7 | 142.1, 177.6 | PASS |
| PCB | board | Keycaps | Key Ans | 1.91 | 15.8, -69.4, 7.7 | 134.2, 208.4 | PASS |
| PCB | board | Keycaps | Key sin | 1.91 | 7.8, -14.9, 7.7 | 142.2, 153.8 | PASS |
| PCB | board | Keycaps | Key 9 | 1.91 | 2.9, -36.0, 7.7 | 147.1, 175.0 | PASS |
| PCB | board | Keycaps | Key 3 | 1.91 | 2.9, -58.3, 7.7 | 147.1, 197.2 | PASS |
| PCB | board | Keycaps | Key x10^x | 1.91 | 2.9, -69.4, 7.7 | 147.1, 208.4 | PASS |
| PCB | board | Keycaps | Key hyp | 1.91 | -2.9, -14.9, 7.7 | 152.8, 153.8 | PASS |
| PCB | board | Keycaps | Key 8 | 1.91 | -9.9, -36.0, 7.7 | 159.9, 175.0 | PASS |
| PCB | board | Keycaps | Key 2 | 1.91 | -9.9, -58.3, 7.7 | 159.9, 197.2 | PASS |
| PCB | board | Keycaps | Key sqrt | 1.91 | -13.6, -5.5, 7.7 | 163.6, 144.4 | PASS |
| PCB | board | Keycaps | Key ENG | 1.91 | -13.6, -24.4, 7.7 | 163.6, 163.4 | PASS |
| PCB | board | Keycaps | Key x^3 | 1.91 | -14.0, 3.3, 7.7 | 164.0, 135.6 | PASS |
| PCB | board | Keycaps | Key 4 | 1.91 | -19.6, -39.8, 7.7 | 169.6, 178.8 | PASS |
| PCB | board | Keycaps | Key 0 | 1.91 | -19.6, -66.3, 7.7 | 169.6, 205.2 | PASS |
| PCB | board | Keycaps | Key (-) | 1.91 | -24.2, -14.9, 7.7 | 174.2, 153.8 | PASS |
| PCB | board | Keycaps | Key SHIFT | 1.91 | -30.9, 18.1, 7.7 | 180.9, 120.8 | PASS |
| PCB | board | Keycaps | Key REPLAY | 1.91 | -7.8, 13.4, 7.7 | 157.8, 125.6 | PASS |
| Magnet connector | Magnet rear block | Front shell | Front shell | 1.98 | 30.0, 78.7, 8.2 | 120.0, 60.3 | PASS |
| Magnet connector | Magnet neck | Front shell | Front shell | 2.01 | 30.0, 80.0, 7.5 | 120.0, 59.0 | PASS |
| PCB | J1 | Back cover | Back cover | 2.03 | -5.7, -17.2, 5.3 | 155.7, 156.2 | PASS |
| PCB | J1 | Back cover | Back cover | 2.03 | 4.7, -17.2, 5.3 | 145.3, 156.2 | PASS |
| PCB | board | Screws | Screw mid R | 2.03 | 21.6, 15.7, 7.7 | 128.4, 123.2 | PASS |
| PCB | board | Screws | Screw mid L | 2.03 | -24.9, 15.7, 7.7 | 174.9, 123.2 | PASS |
| Magnet connector | Magnet pins (straightened) | Front shell | Front shell | 2.10 | 23.8, 73.8, 6.0 | 126.2, 65.1 | PASS |
| PCB | J1 | Back cover | Back cover | 2.11 | -6.5, -16.9, 5.4 | 156.5, 155.8 | PASS |
| PCB | C7 | Front shell | Front shell | 2.12 | 30.6, 62.6, 6.9 | 119.4, 76.3 | PASS |
| Camera module | Camera lens glass | Back cover | Back cover | 2.16 | 1.4, 43.8, 1.5 | 148.6, 95.1 | PASS |
| Ribbons and wires | LCD FPC | Rubber feet | Foot 2 | 2.17 | -29.8, 49.9, 2.5 | 179.8, 89.0 | PASS |
| PCB | J1 | Back cover | Back cover | 2.21 | 5.2, -17.2, 5.3 | 144.8, 156.2 | PASS |
| PCB | board | Solar cell (dummy) | Solar cell (dummy) | 2.29 | 2.5, 62.5, 7.7 | 147.5, 76.5 | PASS |
| PCB | R4 | Front shell | Front shell | 2.34 | 24.9, 63.6, 6.9 | 125.2, 75.3 | PASS |
| PCB | R10 | Front shell | Front shell | 2.38 | 15.9, -24.5, 6.9 | 134.1, 163.5 | PASS |
| PCB | R19 | Front shell | Front shell | 2.38 | 8.9, -25.9, 6.9 | 141.1, 164.9 | PASS |
| PCB | C18 | Front shell | Front shell | 2.38 | 12.6, -26.0, 6.9 | 137.4, 164.9 | PASS |
| PCB | C13 | Front shell | Front shell | 2.38 | 10.2, -16.0, 6.9 | 139.8, 154.9 | PASS |
| PCB | R17 | Front shell | Front shell | 2.38 | 26.5, 21.2, 6.9 | 123.5, 117.8 | PASS |
| PCB | C31 | Front shell | Front shell | 2.38 | 20.4, -5.0, 6.9 | 129.6, 143.9 | PASS |
| PCB | J1 | Front shell | Front shell | 2.38 | -0.3, -22.2, 6.9 | 150.3, 161.2 | PASS |
| PCB | J1 | Front shell | Front shell | 2.38 | 0.2, -22.2, 6.9 | 149.8, 161.2 | PASS |
| PCB | J1 | Front shell | Front shell | 2.38 | -6.4, -18.1, 6.9 | 156.4, 157.1 | PASS |
| PCB | FB1 | Front shell | Front shell | 2.38 | 13.4, -15.5, 6.9 | 136.6, 154.4 | PASS |
| PCB | U5 | Front shell | Front shell | 2.38 | 11.3, -20.5, 6.9 | 138.7, 159.4 | PASS |
| PCB | C10 | Front shell | Front shell | 2.38 | 15.2, -17.6, 6.9 | 134.8, 156.6 | PASS |
| PCB | C12 | Front shell | Front shell | 2.38 | 8.6, -17.6, 6.9 | 141.4, 156.6 | PASS |
| PCB | C15 | Front shell | Front shell | 2.38 | 10.0, -22.2, 6.9 | 140.0, 161.1 | PASS |
| PCB | C11 | Front shell | Front shell | 2.38 | 9.8, -18.6, 6.9 | 140.2, 157.5 | PASS |
| PCB | U4 | Front shell | Front shell | 2.38 | 11.3, -19.2, 6.9 | 138.7, 158.1 | PASS |
| PCB | D2 | Front shell | Front shell | 2.38 | -6.1, -15.0, 6.9 | 156.1, 153.9 | PASS |
| PCB | R9 | Front shell | Front shell | 2.38 | 11.3, -25.9, 6.9 | 138.7, 164.9 | PASS |
| PCB | R18 | Front shell | Front shell | 2.38 | 10.7, -26.7, 6.9 | 139.3, 165.6 | PASS |
| PCB | U6 | Front shell | Front shell | 2.38 | 25.1, -6.7, 6.9 | 125.0, 145.6 | PASS |
| PCB | R8 | Front shell | Front shell | 2.38 | 13.8, -25.9, 6.9 | 136.2, 164.9 | PASS |
| PCB | C17 | Front shell | Front shell | 2.39 | -7.5, -17.0, 6.9 | 157.5, 156.0 | PASS |
| PCB | J1 | Front shell | Front shell | 2.39 | -0.7, -22.2, 6.9 | 150.7, 161.2 | PASS |
| PCB | J1 | Front shell | Front shell | 2.39 | 0.7, -22.2, 6.9 | 149.3, 161.2 | PASS |
| PCB | J1 | Front shell | Front shell | 2.42 | -7.5, -18.1, 6.9 | 157.5, 157.1 | PASS |
| PCB | J1 | Back cover | Back cover | 2.42 | 5.7, -17.2, 5.3 | 144.3, 156.2 | PASS |
| PCB | J1 | Front shell | Front shell | 2.42 | 7.5, -18.1, 6.9 | 142.5, 157.1 | PASS |
| Magnet connector | Magnet neck | Back cover | Back cover | 2.44 | 14.0, 80.0, 4.0 | 136.0, 59.0 | PASS |
| PCB | J1 | Front shell | Front shell | 2.45 | -1.2, -22.2, 6.9 | 151.2, 161.2 | PASS |
| PCB | J1 | Front shell | Front shell | 2.45 | 1.2, -22.2, 6.9 | 148.8, 161.2 | PASS |
| PCB | U1 | Back cover | Back cover | 2.50 | 16.7, 44.2, 4.5 | 133.3, 94.8 | PASS |
| PCB | J1 | Front shell | Front shell | 2.54 | -6.6, -17.6, 6.8 | 156.6, 156.6 | PASS |
| PCB | J1 | Front shell | Front shell | 2.54 | 6.6, -17.6, 6.8 | 143.4, 156.6 | PASS |
| PCB | R20 | Front shell | Front shell | 2.54 | 20.2, 58.6, 6.9 | 129.8, 80.3 | PASS |
| PCB | J1 | Back cover | Back cover | 2.54 | 6.5, -16.9, 5.4 | 143.5, 155.8 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -5.7, -18.1, 6.7 | 155.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -5.2, -18.1, 6.7 | 155.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -4.7, -18.1, 6.7 | 154.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -4.2, -18.1, 6.7 | 154.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -3.7, -18.1, 6.7 | 153.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -3.2, -18.1, 6.7 | 153.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -2.7, -18.1, 6.7 | 152.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -2.2, -18.1, 6.7 | 152.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | -1.7, -18.1, 6.7 | 151.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 1.8, -18.1, 6.7 | 148.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 2.3, -18.1, 6.7 | 147.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 2.8, -18.1, 6.7 | 147.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 3.3, -18.1, 6.7 | 146.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 3.8, -18.1, 6.7 | 146.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 4.3, -18.1, 6.7 | 145.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 4.8, -18.1, 6.7 | 145.2, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 5.3, -18.1, 6.7 | 144.7, 157.1 | PASS |
| PCB | J1 | Front shell | Front shell | 2.58 | 5.8, -18.1, 6.7 | 144.2, 157.1 | PASS |
| Ribbons and wires | LiPo lead red | Front shell | Front shell | 2.69 | -2.0, 61.6, 5.4 | 152.0, 77.3 | PASS |
| PCB | R10 | Keycaps | Key S<>D | 2.71 | 15.9, -24.4, 6.9 | 134.1, 163.4 | PASS |
| PCB | R17 | Keycaps | Key ON | 2.71 | 26.3, 21.1, 6.9 | 123.7, 117.8 | PASS |
| PCB | C16 | Keycaps | Key hyp | 2.71 | -7.5, -13.1, 6.9 | 157.5, 152.0 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -5.8, -22.2, 6.9 | 155.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -5.3, -22.2, 6.9 | 155.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -4.8, -22.2, 6.9 | 154.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -4.3, -22.2, 6.9 | 154.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -3.8, -22.2, 6.9 | 153.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -3.3, -22.2, 6.9 | 153.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -2.8, -22.2, 6.9 | 152.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -2.3, -22.2, 6.9 | 152.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -1.8, -22.2, 6.9 | 151.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -1.3, -22.2, 6.9 | 151.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -0.7, -22.2, 6.9 | 150.7, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 0.7, -22.2, 6.9 | 149.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 1.2, -22.2, 6.9 | 148.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 1.7, -22.2, 6.9 | 148.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 2.2, -22.2, 6.9 | 147.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 2.7, -22.2, 6.9 | 147.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 3.2, -22.2, 6.9 | 146.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 3.7, -22.2, 6.9 | 146.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 4.2, -22.2, 6.9 | 145.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 4.7, -22.2, 6.9 | 145.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 5.2, -22.2, 6.9 | 144.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 5.7, -22.2, 6.9 | 144.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.71 | -6.4, -18.2, 6.9 | 156.4, 157.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.71 | 6.4, -18.2, 6.9 | 143.6, 157.2 | PASS |
| PCB | R16 | Keycaps | Key ln | 2.71 | 23.8, -2.5, 6.9 | 126.2, 141.5 | PASS |
| PCB | U5 | Keycaps | Key S<>D | 2.71 | 13.9, -22.9, 6.9 | 136.1, 161.8 | PASS |
| PCB | C10 | Keycaps | Key S<>D | 2.71 | 15.2, -18.6, 6.9 | 134.8, 157.5 | PASS |
| PCB | C12 | Keycaps | Key ) | 2.71 | 8.6, -18.6, 6.9 | 141.4, 157.5 | PASS |
| PCB | R15 | Keycaps | Key ln | 2.71 | 24.9, -2.5, 6.9 | 125.0, 141.5 | PASS |
| PCB | C15 | Keycaps | Key ) | 2.71 | 9.9, -22.2, 6.9 | 140.1, 161.1 | PASS |
| PCB | R14 | Keycaps | Key ln | 2.71 | 26.1, -2.5, 6.9 | 123.8, 141.5 | PASS |
| PCB | C14 | Keycaps | Key S<>D | 2.71 | 15.2, -22.2, 6.9 | 134.8, 161.1 | PASS |
| PCB | R13 | Keycaps | Key ln | 2.71 | 27.4, -2.5, 6.9 | 122.7, 141.5 | PASS |
| PCB | U4 | Keycaps | Key S<>D | 2.71 | 13.9, -19.3, 6.9 | 136.1, 158.2 | PASS |
| PCB | D2 | Keycaps | Key hyp | 2.71 | -6.2, -14.7, 6.9 | 156.2, 153.7 | PASS |
| PCB | U6 | Keycaps | Key tan | 2.71 | 26.0, -8.9, 6.9 | 124.0, 147.9 | PASS |
| PCB | C11 | Keycaps | Key ) | 2.71 | 9.8, -18.6, 6.9 | 140.2, 157.5 | PASS |
| PCB | U6 | Keycaps | Key ln | 2.71 | 25.1, -5.6, 6.9 | 124.9, 144.5 | PASS |
| PCB | FB1 | Keycaps | Key cos | 2.71 | 13.4, -15.1, 6.9 | 136.6, 154.0 | PASS |
| PCB | C31 | Keycaps | Key log | 2.71 | 20.4, -4.8, 6.9 | 129.6, 143.7 | PASS |
| PCB | R2 | Front shell | Front shell | 2.72 | 18.0, 64.1, 6.9 | 132.0, 74.8 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.73 | -0.3, -22.2, 6.9 | 150.3, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.73 | 0.3, -22.2, 6.9 | 149.7, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.73 | -7.5, -18.2, 6.9 | 157.5, 157.2 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.73 | 7.5, -18.2, 6.9 | 142.5, 157.2 | PASS |
| PCB | C16 | Front shell | Front shell | 2.77 | -8.5, -13.1, 6.9 | 158.5, 152.0 | PASS |
| PCB | C17 | Keycaps | Key ( | 2.80 | -7.5, -17.5, 6.9 | 157.5, 156.4 | PASS |
| PCB | C18 | Keycaps | Key DEL | 2.81 | 12.6, -26.8, 6.9 | 137.4, 165.7 | PASS |
| PCB | R19 | Keycaps | Key DEL | 2.82 | 8.9, -26.7, 6.9 | 141.1, 165.7 | PASS |
| PCB | R9 | Keycaps | Key DEL | 2.82 | 11.3, -26.7, 6.9 | 138.7, 165.7 | PASS |
| PCB | R18 | Keycaps | Key DEL | 2.82 | 10.2, -26.7, 6.9 | 139.8, 165.7 | PASS |
| PCB | R8 | Keycaps | Key DEL | 2.82 | 13.8, -26.7, 6.9 | 136.2, 165.7 | PASS |
| PCB | C13 | Keycaps | Key sin | 2.84 | 9.8, -15.0, 6.9 | 140.2, 153.9 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.84 | -0.2, -22.2, 6.9 | 150.2, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.84 | 0.2, -22.2, 6.9 | 149.8, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 2.86 | -6.5, -18.1, 6.8 | 156.5, 157.0 | PASS |
| PCB | J1 | Keycaps | Key ) | 2.86 | 6.5, -18.1, 6.8 | 143.5, 157.0 | PASS |
| PCB | R16 | Front shell | Front shell | 2.89 | 23.8, -1.6, 6.9 | 126.2, 140.5 | PASS |
| PCB | R5 | Front shell | Front shell | 2.90 | 24.4, 58.2, 6.9 | 125.6, 80.8 | PASS |
| PCB | J1 | Back cover | Back cover | 2.92 | -0.3, -21.5, 4.9 | 150.3, 160.4 | PASS |
| PCB | C32 | Front shell | Front shell | 2.94 | 32.2, 56.9, 6.9 | 117.8, 82.0 | PASS |
| PCB | C17 | Back cover | Back cover | 2.95 | -7.5, -17.0, 6.4 | 157.5, 155.9 | PASS |
| PCB | C15 | Keycaps | Key S<>D | 2.96 | 10.2, -22.2, 6.9 | 139.8, 161.1 | PASS |
| PCB | C11 | Keycaps | Key S<>D | 2.99 | 10.2, -18.6, 6.9 | 139.8, 157.5 | PASS |
| Magnet connector | Magnet contact pads | Front shell | Front shell | 3.00 | 23.8, 81.6, 6.5 | 126.2, 57.3 | PASS |
| PCB | U5 | Keycaps | Key ) | 3.00 | 11.2, -20.5, 6.9 | 138.8, 159.4 | PASS |
| PCB | U4 | Keycaps | Key ) | 3.00 | 11.2, -19.3, 6.9 | 138.8, 158.2 | PASS |
| PCB | R8 | Keycaps | Key S<>D | 3.03 | 13.8, -25.8, 6.9 | 136.2, 164.7 | PASS |
| PCB | J1 | Keycaps | Key ) | 3.03 | -0.7, -22.2, 6.9 | 150.7, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 3.03 | 0.7, -22.2, 6.9 | 149.3, 161.2 | PASS |
| PCB | C18 | Keycaps | Key S<>D | 3.04 | 13.0, -25.8, 6.9 | 137.0, 164.7 | PASS |
| PCB | U6 | Keycaps | Key cos | 3.06 | 22.0, -9.6, 6.9 | 128.0, 148.5 | PASS |
| PCB | C31 | Keycaps | Key ln | 3.08 | 20.8, -4.8, 6.9 | 129.2, 143.7 | PASS |
| PCB | board | Screws | Screw bottom L | 3.09 | -23.4, -71.8, 7.7 | 173.4, 210.7 | PASS |
| LCD panel | LCD backlight (frame + LED light guide) | Solar cell (dummy) | Solar cell (dummy) | 3.12 | 23.6, 58.7, 9.2 | 126.4, 80.2 | PASS |
| PCB | R19 | Keycaps | Key ) | 3.12 | 8.9, -25.8, 6.9 | 141.1, 164.7 | PASS |
| PCB | C13 | Back cover | Back cover | 3.13 | 9.8, -15.0, 6.4 | 140.2, 153.9 | PASS |
| PCB | board | Screws | Screw bottom R | 3.13 | 23.4, -71.8, 7.7 | 126.6, 210.7 | PASS |
| PCB | J1 | Front shell | Front shell | 3.19 | -0.6, -21.5, 6.1 | 150.6, 160.4 | PASS |
| PCB | Q5 | Front shell | Front shell | 3.22 | -6.2, 57.9, 6.9 | 156.2, 81.0 | PASS |
| PCB | R9 | Keycaps | Key S<>D | 3.23 | 11.8, -25.8, 6.9 | 138.2, 164.7 | PASS |
| PCB | C13 | Keycaps | Key cos | 3.28 | 10.2, -15.0, 6.9 | 139.8, 153.9 | PASS |
| PCB | J1 | Keycaps | Key ) | 3.29 | -1.2, -22.2, 6.9 | 151.2, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 3.29 | 1.2, -22.2, 6.9 | 148.8, 161.2 | PASS |
| Ribbons and wires | Camera FPC | Front shell | Front shell | 3.33 | -2.5, -15.5, 6.0 | 152.5, 154.4 | PASS |
| PCB | U4 | Keycaps | Key cos | 3.35 | 13.3, -16.7, 6.8 | 136.7, 155.6 | PASS |
| PCB | C14 | Front shell | Front shell | 3.36 | 15.2, -22.2, 6.9 | 134.8, 161.1 | PASS |
| PCB | U7 | Back cover | Back cover | 3.37 | 17.1, 57.0, 5.4 | 132.9, 81.9 | PASS |
| PCB | J4 | Back cover | Back cover | 3.37 | 11.7, 62.9, 4.4 | 138.3, 76.0 | PASS |
| PCB | J4 | Back cover | Back cover | 3.37 | 5.2, 63.7, 4.4 | 144.8, 75.2 | PASS |
| PCB | U5 | Back cover | Back cover | 3.37 | 11.9, -21.6, 5.4 | 138.1, 160.5 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.38 | 6.4, -16.9, 6.9 | 143.6, 155.8 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.38 | -6.4, -16.9, 6.9 | 156.4, 155.8 | PASS |
| PCB | R15 | Front shell | Front shell | 3.38 | 24.9, -1.6, 6.9 | 125.0, 140.5 | PASS |
| PCB | R14 | Front shell | Front shell | 3.38 | 26.1, -1.6, 6.9 | 123.8, 140.5 | PASS |
| PCB | R13 | Front shell | Front shell | 3.38 | 27.4, -1.6, 6.9 | 122.7, 140.5 | PASS |
| PCB | C1 | Front shell | Front shell | 3.38 | 28.1, 31.1, 6.9 | 121.9, 107.8 | PASS |
| PCB | D2 | Keycaps | Key o'' | 3.38 | -9.9, -14.7, 6.9 | 159.9, 153.7 | PASS |
| PCB | U6 | Keycaps | Key log | 3.40 | 22.0, -5.6, 6.9 | 128.0, 144.5 | PASS |
| PCB | J3 | Back cover | Back cover | 3.42 | 27.8, 68.5, 4.4 | 122.2, 70.4 | PASS |
| PCB | R18 | Keycaps | Key ) | 3.43 | 10.2, -25.8, 6.9 | 139.8, 164.7 | PASS |
| PCB | C17 | Keycaps | Key hyp | 3.45 | -7.5, -17.0, 6.9 | 157.5, 156.0 | PASS |
| Magnet connector | Magnet contact pads | Back cover | Back cover | 3.46 | 23.8, 81.6, 5.0 | 126.2, 57.3 | PASS |
| Magnet connector | Magnet rear block | Screws | Screw corner R | 3.47 | 31.0, 77.7, 3.2 | 119.0, 61.3 | PASS |
| PCB | C9 | Front shell | Front shell | 3.50 | 10.8, 31.4, 6.9 | 139.2, 107.5 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.50 | -6.7, -16.9, 6.8 | 156.7, 155.8 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.50 | 6.6, -16.9, 6.8 | 143.4, 155.8 | PASS |
| PCB | R6 | Front shell | Front shell | 3.51 | 13.2, 31.5, 6.9 | 136.8, 107.5 | PASS |
| PCB | R7 | Front shell | Front shell | 3.51 | 11.9, 31.5, 6.9 | 138.1, 107.5 | PASS |
| PCB | C3 | Front shell | Front shell | 3.54 | 24.6, 31.5, 6.9 | 125.4, 107.4 | PASS |
| PCB | C13 | Keycaps | Key ) | 3.60 | 9.8, -16.0, 6.9 | 140.2, 154.9 | PASS |
| PCB | J1 | Back cover | Back cover | 3.60 | -6.5, -18.1, 6.8 | 156.5, 157.0 | PASS |
| PCB | J1 | Keycaps | Key ) | 3.60 | -1.7, -22.2, 6.9 | 151.7, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 3.60 | 1.7, -22.2, 6.9 | 148.3, 161.2 | PASS |
| PCB | FB1 | Keycaps | Key S<>D | 3.60 | 13.4, -15.8, 6.9 | 136.6, 154.8 | PASS |
| PCB | R18 | Keycaps | Key S<>D | 3.63 | 10.7, -25.8, 6.9 | 139.3, 164.7 | PASS |
| PCB | C12 | Keycaps | Key S<>D | 3.65 | 9.0, -18.6, 6.9 | 141.0, 157.5 | PASS |
| PCB | C32 | Back cover | Back cover | 3.67 | 31.0, 57.3, 5.7 | 119.0, 81.6 | PASS |
| PCB | C1 | Back cover | Back cover | 3.67 | 28.1, 31.5, 5.7 | 121.9, 107.4 | PASS |
| PCB | Q4 | Front shell | Front shell | 3.69 | -10.7, 33.8, 6.9 | 160.7, 105.2 | PASS |
| PCB | R23 | Front shell | Front shell | 3.69 | -11.8, 58.7, 6.9 | 161.8, 80.2 | PASS |
| PCB | R21 | Front shell | Front shell | 3.69 | -10.0, 32.9, 6.9 | 160.0, 106.0 | PASS |
| PCB | C2 | Front shell | Front shell | 3.69 | 26.5, 32.3, 6.9 | 123.5, 106.7 | PASS |
| PCB | FB1 | Keycaps | Key sin | 3.71 | 11.8, -15.1, 6.9 | 138.2, 154.0 | PASS |
| PCB | D2 | Keycaps | Key ( | 3.71 | -6.8, -15.8, 6.8 | 156.8, 154.7 | PASS |
| PCB | Q4 | Back cover | Back cover | 3.71 | -10.9, 34.4, 5.7 | 160.9, 104.5 | PASS |
| PCB | Q5 | Back cover | Back cover | 3.71 | -5.9, 57.2, 5.7 | 155.9, 81.7 | PASS |
| PCB | Q1 | Back cover | Back cover | 3.72 | 14.5, 58.0, 5.7 | 135.5, 80.9 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.73 | -5.8, -17.2, 6.7 | 155.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.73 | -5.3, -17.2, 6.7 | 155.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.73 | -4.8, -17.2, 6.7 | 154.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.73 | -4.3, -17.2, 6.7 | 154.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.73 | -3.8, -17.2, 6.7 | 153.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.73 | -3.3, -17.2, 6.7 | 153.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.73 | 3.2, -17.2, 6.7 | 146.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.73 | 3.7, -17.2, 6.7 | 146.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.73 | 4.2, -17.2, 6.7 | 145.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.73 | 4.7, -17.2, 6.7 | 145.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.73 | 5.2, -17.2, 6.7 | 144.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.73 | 5.7, -17.2, 6.7 | 144.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.73 | -2.8, -17.2, 6.7 | 152.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.73 | 2.8, -17.2, 6.7 | 147.2, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.75 | -2.3, -17.2, 6.7 | 152.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.75 | 2.3, -17.2, 6.7 | 147.7, 156.2 | PASS |
| PCB | C19 | Front shell | Front shell | 3.76 | -5.2, 34.5, 6.9 | 155.2, 104.4 | PASS |
| PCB | C10 | Keycaps | Key cos | 3.80 | 15.2, -17.6, 6.9 | 134.8, 156.5 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.81 | -1.8, -17.2, 6.7 | 151.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.81 | 1.8, -17.2, 6.7 | 148.2, 156.2 | PASS |
| PCB | C33 | Front shell | Front shell | 3.83 | 29.3, 54.8, 6.9 | 120.7, 84.2 | PASS |
| PCB | C12 | Keycaps | Key sin | 3.85 | 8.6, -17.6, 6.9 | 141.4, 156.5 | PASS |
| PCB | U7 | Front shell | Front shell | 3.86 | 17.7, 56.9, 6.9 | 132.3, 82.0 | PASS |
| PCB | J1 | Keycaps | Key hyp | 3.89 | -1.3, -17.2, 6.7 | 151.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 3.89 | 1.3, -17.2, 6.7 | 148.7, 156.2 | PASS |
| PCB | R10 | Keycaps | Key DEL | 3.91 | 15.9, -24.7, 6.9 | 134.1, 163.7 | PASS |
| PCB | J5 | Back cover | Back cover | 3.92 | -7.0, 43.4, 5.9 | 157.0, 95.6 | PASS |
| PCB | J5 | Back cover | Back cover | 3.92 | -10.5, 53.6, 5.9 | 160.5, 85.3 | PASS |
| PCB | J1 | Keycaps | Key ) | 3.95 | -2.2, -22.2, 6.9 | 152.2, 161.2 | PASS |
| PCB | J1 | Keycaps | Key ( | 3.95 | 2.2, -22.2, 6.9 | 147.8, 161.2 | PASS |
| PCB | R9 | Keycaps | Key ) | 3.95 | 11.3, -25.8, 6.9 | 138.7, 164.7 | PASS |
| PCB | J1 | Back cover | Back cover | 3.96 | 6.5, -18.1, 6.8 | 143.5, 157.0 | PASS |
| PCB | C13 | Keycaps | Key S<>D | 3.98 | 10.2, -16.0, 6.9 | 139.8, 154.9 | PASS |
| PCB | J5 | Back cover | Back cover | 3.98 | -9.9, 54.1, 6.0 | 159.9, 84.8 | PASS |
| PCB | C16 | Keycaps | Key o'' | 3.98 | -8.5, -12.6, 6.9 | 158.5, 151.6 | PASS |
| PCB | U4 | Back cover | Back cover | 4.00 | 11.9, -19.4, 5.4 | 138.1, 158.3 | PASS |
| PCB | J5 | Back cover | Back cover | 4.02 | -10.0, 37.7, 6.0 | 160.0, 101.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 4.02 | -0.8, -17.2, 6.7 | 150.8, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 4.02 | 0.8, -17.2, 6.7 | 149.2, 156.2 | PASS |
| PCB | C31 | Keycaps | Key cos | 4.03 | 20.4, -5.8, 6.9 | 129.6, 144.7 | PASS |
| PCB | D6 | Back cover | Back cover | 4.04 | 20.9, 59.6, 5.7 | 129.1, 79.3 | PASS |
| Ribbons and wires | LiPo lead black | Front shell | Front shell | 4.09 | -2.0, 61.6, 4.0 | 152.0, 77.3 | PASS |
| PCB | C11 | Keycaps | Key sin | 4.10 | 9.8, -17.6, 6.9 | 140.2, 156.5 | PASS |
| PCB | C3 | Back cover | Back cover | 4.12 | 24.6, 31.5, 6.1 | 125.4, 107.4 | PASS |
| LCD panel | LCD driver IC (COG) + sealant | Front shell | Front shell | 4.12 | -24.0, 37.8, 9.8 | 174.0, 101.1 | PASS |
| PCB | C36 | Front shell | Front shell | 4.12 | -7.1, 35.6, 6.9 | 157.1, 103.3 | PASS |
| PCB | J3 | Front shell | Front shell | 4.13 | 24.1, 68.5, 6.9 | 125.9, 70.4 | PASS |
| PCB | board | Screws | Screw corner R | 4.17 | 29.4, 75.7, 7.7 | 120.7, 63.3 | PASS |
| PCB | J1 | Keycaps | Key hyp | 4.18 | -0.3, -17.2, 6.7 | 150.3, 156.2 | PASS |
| PCB | J1 | Keycaps | Key sin | 4.18 | 0.3, -17.2, 6.7 | 149.7, 156.2 | PASS |
| PCB | D7 | Back cover | Back cover | 4.19 | 32.1, 64.7, 5.9 | 117.9, 74.2 | PASS |
| PCB | U4 | Keycaps | Key sin | 4.21 | 11.2, -16.9, 6.9 | 138.8, 155.8 | PASS |
| PCB | R19 | Keycaps | Key S<>D | 4.24 | 9.4, -25.8, 6.9 | 140.6, 164.7 | PASS |
| PCB | C17 | Keycaps | Key ENG | 4.25 | -8.5, -17.5, 6.9 | 158.5, 156.4 | PASS |
| PCB | J3 | Front shell | Front shell | 4.27 | 21.6, 65.7, 6.9 | 128.4, 73.2 | PASS |
| PCB | J3 | Front shell | Front shell | 4.27 | 19.0, 65.7, 6.9 | 131.0, 73.2 | PASS |
| PCB | C33 | Back cover | Back cover | 4.28 | 27.3, 54.8, 5.7 | 122.7, 84.1 | PASS |
| Magnet connector | Magnet pins (straightened) | Back cover | Back cover | 4.29 | 26.2, 77.7, 5.4 | 123.8, 61.3 | PASS |
| PCB | R19 | Keycaps | Key 9 | 4.30 | 8.9, -26.7, 6.9 | 141.1, 165.7 | PASS |
| PCB | C8 | Back cover | Back cover | 4.31 | 25.0, 59.3, 6.1 | 125.0, 79.7 | PASS |
| PCB | C12 | Back cover | Back cover | 4.34 | 8.6, -17.6, 6.4 | 141.4, 156.5 | PASS |
| PCB | U3 | Back cover | Back cover | 4.37 | 28.7, 60.8, 5.4 | 121.3, 78.1 | PASS |
| PCB | U2 | Back cover | Back cover | 4.37 | 14.8, 70.1, 5.4 | 135.2, 68.8 | PASS |
| PCB | C34 | Back cover | Back cover | 4.37 | 24.9, 54.6, 5.7 | 125.1, 84.3 | PASS |
| PCB | J1 | Keycaps | Key sin | 4.37 | -0.2, -17.2, 6.7 | 150.2, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 4.37 | 0.2, -17.2, 6.7 | 149.8, 156.2 | PASS |
| PCB | FB1 | Keycaps | Key ) | 4.37 | 11.8, -15.8, 6.9 | 138.2, 154.8 | PASS |
| PCB | C31 | Keycaps | Key tan | 4.41 | 20.8, -5.8, 6.9 | 129.2, 144.7 | PASS |
| PCB | C9 | Back cover | Back cover | 4.42 | 10.8, 31.4, 6.4 | 139.2, 107.5 | PASS |
| PCB | C2 | Back cover | Back cover | 4.42 | 26.5, 32.3, 6.4 | 123.5, 106.7 | PASS |
| PCB | C15 | Back cover | Back cover | 4.42 | 9.8, -22.2, 6.4 | 140.2, 161.1 | PASS |
| PCB | FB1 | Back cover | Back cover | 4.45 | 11.8, -15.1, 6.1 | 138.2, 154.0 | PASS |
| PCB | R23 | Back cover | Back cover | 4.47 | -10.2, 57.9, 6.5 | 160.2, 81.0 | PASS |
| PCB | D2 | Keycaps | Key ENG | 4.49 | -9.9, -15.4, 6.9 | 159.9, 154.3 | PASS |
| Magnet connector | Magnet neck | Screws | Screw corner R | 4.56 | 31.0, 78.7, 4.0 | 119.0, 60.3 | PASS |
| PCB | R6 | Back cover | Back cover | 4.57 | 13.2, 31.5, 6.6 | 136.8, 107.5 | PASS |
| PCB | R5 | Back cover | Back cover | 4.57 | 24.4, 57.7, 6.6 | 125.6, 81.2 | PASS |
| PCB | R7 | Back cover | Back cover | 4.57 | 11.9, 31.5, 6.6 | 138.1, 107.5 | PASS |
| PCB | R21 | Back cover | Back cover | 4.57 | -9.2, 32.4, 6.6 | 159.2, 106.5 | PASS |
| PCB | R20 | Back cover | Back cover | 4.57 | 20.2, 58.1, 6.6 | 129.8, 80.8 | PASS |
| PCB | J1 | Keycaps | Key sin | 4.59 | -0.7, -17.2, 6.7 | 150.7, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 4.59 | 0.7, -17.2, 6.7 | 149.3, 156.2 | PASS |
| PCB | C11 | Keycaps | Key cos | 4.60 | 10.2, -17.6, 6.9 | 139.8, 156.5 | PASS |
| PCB | C5 | Front shell | Front shell | 4.65 | 13.7, 66.1, 6.9 | 136.3, 72.8 | PASS |
| PCB | D1 | Back cover | Back cover | 4.66 | 22.0, 63.1, 5.7 | 128.0, 75.8 | PASS |
| PCB | C18 | Keycaps | Key ) | 4.66 | 12.6, -25.8, 6.9 | 137.4, 164.7 | PASS |
| PCB | C7 | Back cover | Back cover | 4.66 | 31.4, 61.3, 6.1 | 118.6, 77.6 | PASS |
| PCB | C19 | Back cover | Back cover | 4.67 | -5.2, 34.5, 6.1 | 155.2, 104.4 | PASS |
| PCB | Q2 | Back cover | Back cover | 4.71 | 12.7, 62.7, 5.7 | 137.3, 76.2 | PASS |
| PCB | R17 | Back cover | Back cover | 4.76 | 26.5, 21.1, 6.6 | 123.5, 117.8 | PASS |
| PCB | R22 | Back cover | Back cover | 4.76 | -6.4, 59.1, 6.6 | 156.4, 79.8 | PASS |
| Camera module | Camera sensor board | Back cover | Back cover | 4.77 | 4.2, 44.4, 5.9 | 145.8, 94.5 | PASS |
| PCB | J1 | Keycaps | Key sin | 4.85 | -1.2, -17.2, 6.7 | 151.2, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 4.85 | 1.2, -17.2, 6.7 | 148.8, 156.2 | PASS |
| PCB | U6 | Back cover | Back cover | 4.89 | 26.0, -9.6, 5.9 | 124.0, 148.5 | PASS |
| PCB | R3 | Back cover | Back cover | 4.97 | 17.0, 59.9, 6.6 | 133.0, 79.1 | PASS |
| PCB | C11 | Back cover | Back cover | 5.00 | 9.8, -17.6, 6.4 | 140.2, 156.5 | PASS |
| PCB | C34 | Front shell | Front shell | 5.09 | 26.0, 54.6, 6.9 | 124.0, 84.3 | PASS |
| PCB | C14 | Back cover | Back cover | 5.11 | 15.2, -22.0, 6.4 | 134.8, 160.9 | PASS |
| PCB | J5 | Front shell | Front shell | 5.11 | -7.6, 37.3, 6.9 | 157.6, 101.6 | PASS |
| PCB | C6 | Back cover | Back cover | 5.12 | 10.0, 72.1, 6.1 | 140.0, 66.8 | PASS |
| PCB | C5 | Back cover | Back cover | 5.12 | 13.7, 66.1, 6.1 | 136.3, 72.8 | PASS |
| PCB | J1 | Keycaps | Key sin | 5.13 | -1.7, -17.2, 6.7 | 151.7, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 5.13 | 1.7, -17.2, 6.7 | 148.3, 156.2 | PASS |
| PCB | C12 | Keycaps | Key cos | 5.22 | 9.0, -17.6, 6.9 | 141.0, 156.5 | PASS |
| PCB | C17 | Keycaps | Key o'' | 5.24 | -8.5, -17.0, 6.9 | 158.5, 156.0 | PASS |
| PCB | J5 | Front shell | Front shell | 5.26 | -9.8, 37.6, 6.9 | 159.8, 101.4 | PASS |
| PCB | R10 | Back cover | Back cover | 5.38 | 14.9, -24.2, 6.6 | 135.1, 163.2 | PASS |
| PCB | C36 | Back cover | Back cover | 5.39 | -7.5, 36.6, 6.4 | 157.5, 102.3 | PASS |
| PCB | C18 | Back cover | Back cover | 5.42 | 12.6, -26.8, 6.4 | 137.4, 165.7 | PASS |
| PCB | C31 | Back cover | Back cover | 5.42 | 20.4, -5.8, 6.4 | 129.6, 144.7 | PASS |
| PCB | C10 | Back cover | Back cover | 5.42 | 15.2, -18.6, 6.4 | 134.8, 157.5 | PASS |
| PCB | C4 | Back cover | Back cover | 5.42 | 24.8, 50.8, 6.4 | 125.2, 88.1 | PASS |
| PCB | J5 | Front shell | Front shell | 5.42 | -9.8, 54.1, 6.9 | 159.8, 84.8 | PASS |
| PCB | J1 | Keycaps | Key sin | 5.43 | -2.2, -17.2, 6.7 | 152.2, 156.2 | PASS |
| PCB | J1 | Keycaps | Key hyp | 5.43 | 2.2, -17.2, 6.7 | 147.8, 156.2 | PASS |
| PCB | R19 | Back cover | Back cover | 5.57 | 8.9, -26.7, 6.6 | 141.1, 165.7 | PASS |
| PCB | R2 | Back cover | Back cover | 5.57 | 18.0, 64.1, 6.6 | 132.0, 74.8 | PASS |
| PCB | R16 | Back cover | Back cover | 5.57 | 23.8, -2.5, 6.6 | 126.2, 141.5 | PASS |
| PCB | R15 | Back cover | Back cover | 5.57 | 24.9, -2.5, 6.6 | 125.0, 141.5 | PASS |
| PCB | R1 | Back cover | Back cover | 5.57 | 22.9, 50.8, 6.6 | 127.1, 88.2 | PASS |
| PCB | R14 | Back cover | Back cover | 5.57 | 26.1, -2.5, 6.6 | 123.8, 141.5 | PASS |
| PCB | R13 | Back cover | Back cover | 5.57 | 27.4, -2.5, 6.6 | 122.7, 141.5 | PASS |
| PCB | R4 | Back cover | Back cover | 5.57 | 24.9, 63.7, 6.6 | 125.2, 75.3 | PASS |
| PCB | R9 | Back cover | Back cover | 5.57 | 11.3, -26.7, 6.6 | 138.7, 165.7 | PASS |
| PCB | R18 | Back cover | Back cover | 5.57 | 10.2, -26.7, 6.6 | 139.8, 165.7 | PASS |
| PCB | R8 | Back cover | Back cover | 5.57 | 13.8, -26.7, 6.6 | 136.2, 165.7 | PASS |
| PCB | J3 | Back cover | Back cover | 5.62 | 26.6, 65.7, 6.6 | 123.4, 73.2 | PASS |
| PCB | J3 | Back cover | Back cover | 5.62 | 24.1, 65.7, 6.6 | 125.9, 73.2 | PASS |
| PCB | J3 | Back cover | Back cover | 5.62 | 21.6, 65.7, 6.6 | 128.4, 73.2 | PASS |
| PCB | J3 | Back cover | Back cover | 5.62 | 19.0, 65.7, 6.6 | 131.0, 73.2 | PASS |
| PCB | J5 | Front shell | Front shell | 5.75 | -6.9, 37.9, 6.6 | 156.9, 101.0 | PASS |
| LCD panel | LCD backlight (frame + LED light guide) | Back cover | Back cover | 5.90 | 7.0, 44.4, 7.9 | 143.0, 94.5 | PASS |
| PCB | C4 | Front shell | Front shell | 6.64 | 24.8, 50.8, 6.9 | 125.2, 88.1 | PASS |
| Camera module | Camera sensor board | Front shell | Front shell | 6.87 | -4.2, 39.6, 6.9 | 154.2, 99.3 | PASS |
| LCD panel | LCD TFT glass (with driver ledge) | Back cover | Back cover | 7.15 | 7.0, 44.4, 9.2 | 143.0, 94.5 | PASS |
| LCD panel | LCD CF glass | Back cover | Back cover | 7.50 | 7.0, 44.4, 9.5 | 143.0, 94.5 | PASS |
| LCD panel | LCD driver IC (COG) + sealant | Back cover | Back cover | 7.50 | -23.1, 44.3, 9.5 | 173.1, 94.7 | PASS |
| Camera module | Camera AF motor (VCM) | Front shell | Front shell | 7.58 | -4.1, 39.7, 5.9 | 154.1, 99.2 | PASS |
| PCB | R1 | Front shell | Front shell | 7.74 | 22.9, 51.3, 6.9 | 127.1, 87.7 | PASS |
| LCD panel | LCD top polarizer | Back cover | Back cover | 7.80 | 7.0, 44.4, 9.8 | 143.0, 94.5 | PASS |
| LCD panel | LCD active area (marker) | Back cover | Back cover | 7.93 | 21.4, 57.2, 9.9 | 128.6, 81.8 | PASS |
| Camera module | Camera lens barrel | Front shell | Front shell | 10.73 | 0.0, 40.8, 2.5 | 150.0, 98.1 | PASS |
| Camera module | Camera lens glass | Front shell | Front shell | 12.33 | 0.0, 42.4, 1.8 | 150.0, 96.5 | PASS |
| PCB | board | Screws | Screw corner L | 12.59 | -33.6, 60.7, 7.7 | 183.7, 78.3 | PASS |

**527 pairs checked, 1 FAIL, 526 PASS.**
