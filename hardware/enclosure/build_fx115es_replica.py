"""Unbranded CAD replica of the Casio fx-115ES calculator shell, built in Fusion 360.

    python build_fx115es_replica.py                 offline: compute + check the 2D geometry only
    python build_fx115es_replica.py all             every Fusion stage in order (new document)
    python build_fx115es_replica.py front back      just those stages
    python build_fx115es_replica.py shots views=front,back

Fusion must be open with the FusionMCPBridge add-in running (same as fusion_run.py).
Stages: new front back keys parts looks check export shots explode_shots  (= "all")
Optional, after "all": fitcheck fitcheck_shots fitcheck_cuts  (imports hardware/fab/ai_calc_board.step; re-run when it changes)
Progress goes to replica/build.log.

Frame (same as build_case.py): millimetres, front view. X right, Y up (towards the display),
Z = 0 at the outside of the back cover (rubber feet stick out below), front face at Z = T_TOP (screen section) / T_KEY (keypad section).
KiCad points convert with X = 150 - x_kicad, Y = 138.94 - y_kicad.

Nothing here is Casio branded: no logo, no model name, no key legends.
Every number below is a named parameter; the README in replica/ lists where each came from.
"""
import json, math, os, sys, time

REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
ENC = os.path.join(REPO, "hardware", "enclosure")
OUT = os.path.join(ENC, "replica")
CX, CY = 150.0, 138.94

# â”€â”€ Overall size â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
L_FRONT = 159.9      # C1  front shell outer length (= L_BACK - FRONT_BOTTOM_INSET, checked offline)
L_BACK = 161.0       # C3  back cover length; the traced silhouette is the back cover (photo 88e03c27)
W = 79.3             # C2 (front shell, screen section) = C3 (back cover) outer width
# The trace bulges ~1 mm in the keypad section and is ~1 mm narrow at the screen section; C2 and C3 are
# both 79.3, so the long sides are straightened to W between these Y (the corner shapes are kept).
Y_STRAIGHT = (-55.0, 77.0)   # top end: C9/C11/C15 put the corner posts 4.2/6.1 from the walls -> ~5 mm corner
# The back cover is a tray (photos 84e63d95, 4e287737, e59eff71): in the keypad section its side walls come
# up round the front shell, which is a little smaller there; at the screen section the two are flush.
TRAY_WALL = 0.8      # C5  "wall at keypad section" = the tray wall
TRAY_GAP = 0.15      # guess
FRONT_SIDE_INSET = TRAY_WALL + TRAY_GAP   # front shell side inset below the screen section
FRONT_BOTTOM_INSET = L_BACK - L_FRONT     # 1.1: front shell is shorter at the bottom end only (photo 4e287737)
# Rev C (2026-10-05, photos 7c6e8116 / 76dd27d2 / 478afcb6): the silver shell's side skin steps in (a long thin
# ledge inside the keypad-section walls) from just under the SHIFT/ALPHA row; front views 4e287737 / e59eff71
# show navy round the silver from about Y 0 down. Rev A/B had the step 30 mm too low (Y -12 .. -40).
Y_INSET0, Y_INSET1 = 18.0, -2.0           # the inset grows from 0 to full between these Y
# The navy side walls do not start at full height: photos 2f54d6c7 / 84e63d95 / 050df9b7 (back cover inside)
# show them low (= the 1.5 rim) in the screen section and rising from the small ring (~51 mm below the top,
# Y ~31) to full height by the big ring (Y ~8). Modelled as a straight ramp of the split line on the
# outside of each long side; the silver skin is cut back there so both outer surfaces stay flush.
SIDE_RAMP_Y = (31.0, 8.0)                 # photo (+-3 mm); ramp shape (straight) = guess
# Rev C2 (Nirav 2026-10-05: "more like a faceplate and back cover"; photos 76dd27d2 / 2a97120d / 7c6e8116): below
# the ramp (keypad section and bottom end) the silver part is only a faceplate with a short skirt that drops
# into the navy back cover; the navy wall (C5 0.8) is the side wall there. In the screen section the silver
# part keeps its deep walls (stubs, clips, wire channel, top wall: photos 2a97120d, 66bb7a5f; D1 5.5 rim -> board).
Z_SKIRT = 8.3                             # guess: bottom of the faceplate's skirt in the keypad section (1.5 inside the navy wall)
SKIRT_CUT_D = 2.8                         # strip removed from the silver below Z_SKIRT: whole skin (side 0.95 + 1.4, bottom 1.1 + 1.6) + 0.1
# Slide-case rail groove along both long sides (one groove serves both case positions: the case lips sit
# GROOVE_ZC from the case floor's inside face either way). NOT seen in any photo (no side view): guess.
GROOVE_ON = True
GROOVE_ZC, GROOVE_H, GROOVE_D = 6.4, 1.0, 0.35

# â”€â”€ Thickness / Z stack (Z = 0 at the outside of the back cover) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
# Rev B2 (Nirav's answers, 2026-10-03 late):
#   D4 = 6.0 from the board's back (F.Cu) face to the INSIDE of the back cover floor;
#   D1 = 5.5 from the front shell's RIM (parting line) down to the board's back (F.Cu) face.
# Stack, top part: floor 1.0 (D5) + 6.0 (D4) + board 0.8 + 4.0 front side = 11.8 (D10); keypad: 3.5 front
# side -> 11.3 (D10). Parting line = 7.0 - 5.5 = Z 1.5 (the back cover is a shallow tray; its side walls
# rise round the front shell in the keypad section only). Checks: D3 (board line -> outside at the top
# edge, "about 5") = 11.8 - 7.0 = 4.8; photo 871567b6: the LCD shield (D8 3.75 under a 1.2 plate) ends at
# Z 6.8, level with the Casio board's back face (7.0).
T_TOP = 11.8         # D10 top part (screen section) body thickness, feet not included
T_KEY = 11.3         # D10 "11.3 overall": keypad-section face
Y_RAMP = (22.0, 30.0)   # guess: face rises from T_KEY to T_TOP between these Y (between REPLAY and the bezel)
T_BODY = T_TOP
FOOT_PROUD = 0.3     # guess: feet stick out this much below the back cover

# â”€â”€ Walls and plates â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
WALL_KEY = 1.4       # from C11 (corner post 4.2 from the inside side wall, C9 68.1 c-c); C5's 0.8 = tray wall
WALL_SCREEN = 4.7    # C4  side wall at the screen section = skin + wire channel + inner rib (photos)
INNER_RIB_T = 1.0    # photo 25490985: thin inner rib beside the screen
INNER_RIB_Y = (24.0, 66.0)   # photo: rib runs beside the LCD
INNER_RIB_FULL = True   # rev F: the inner rib crosses the board plane; rev G: from INNER_RIB_Z0 (4.5 below the rim) to the plate.
                        # C4 (4.7 / 5.5) was read with the calipers on the rim, so the rib is there at rim height;
                        # photo 871567b6 shows its top edge level with the Casio board's back face. Until the depth
                        # rod says otherwise (pre-grind checklist, verification/07), the model keeps the conservative case.
                        # rev G (verification/12): depth rod read 4.5-5.0 for the rib, 5.1-6.1 for the pins, 5.5 for the hooks:
                        # all of them cross the board plane (INNER_RIB_Z0 / PIN_Z0 / HOOK_Z0 below).
WALL_TOP = 1.2       # from C11 + C15 (corner post 6.1 from the inside top wall, 8.05 from the outside)
WALL_BOTTOM = 1.6    # guess: bottom end wall
Y_SCREEN_SEC = 24.0  # guess: Y where the side wall steps from thin (keypad) to thick (screen)
FACE_T = 1.2         # guess; caps (dome top 9.6 + 0.4 flange) must fit under the keypad plate (10.1)
D1_RIM = 5.5         # D1: front shell rim (parting line) -> board back face
SEAM_Z = 7.0 - D1_RIM   # 1.5: parting line (board back face at 7.0 = D5 + D4, set below)
Z_TRAY_TOP = 9.8     # guess (no side photo): top of the tray side walls in the keypad section
BACK_PLATE = 1.0     # D5  back cover floor
LIP_H, LIP_T, LIP_GAP = 1.5, 0.8, 0.1   # guess: locating lip on the back cover
EDGE_R_FRONT = 2.0   # guess (photos): round-over of the front face edge
EDGE_R_BACK = 1.2    # guess: round-over of the back edge
Z_PLATE_TOP = T_TOP - FACE_T     # 10.5, underside of the front plate in the screen section
Z_PLATE = T_KEY - FACE_T         # 10.0, underside of the front plate in the keypad section

# â”€â”€ Display, solar cell â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
WIN = (60.65, 24.3)  # C12 display window
WIN_R = 1.0          # guess: window corner radius
WIN_TOP_FROM_EDGE = 24.0   # C13 case top edge -> window top edge
BEZEL_MARGIN = 2.5   # guess: lens overlaps the window by this all round
BEZEL_DEPTH = 1.0    # guess: lens recess depth
LENS_T = 0.8         # guess: clear window thickness (top sits 0.2 below the face)
SOLAR_C = (10.85, 67.34)   # photo (stage 10), solar-cell window centre
SOLAR = (34.0, 11.0)       # photo, solar-cell window size
SOLAR_DEPTH = 0.8          # guess
SOLAR_FRAME = (0.8, 2.5)   # photo 2a97120d: rib frame round the solar window inside (thickness, height: guess)
EPD_P3 = (59.0, 29.2, 1.0) # P3 e-paper panel (thickness from the Waveshare drawing); fit check only

# â”€â”€ Keys â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
# Opening sizes measured on the fx-115ES shell (build_case.py KEY_OPEN), keyed by KiCad pad width class
KEY_OPEN = {"oval": (8.3, 5.6), 6.0: (8.1, 6.0), 7.0: (8.9, 5.9), 9.0: (11.8, 8.2)}
# photo aa09fe8a: tab-shaped openings, small top corners and big bottom corners (rt, rb)
KEY_R = {"oval": None, 6.0: (0.8, 2.0), 7.0: (0.8, 2.0), 9.0: (1.0, 3.0)}
KEY_BEVEL = (0.4, 30.0)  # photo: thin bevel round each opening (depth, degrees)
COLLAR = (0.25, 0.75, 0.8)  # photos 76dd27d2 / e82c982d: raised rim round each opening underneath (from, to, height);
                            # rev D: 0.8 tall (was 0.5); the 0.25 step inside is where the cap flanges sit
ROW_RIB_T = 0.8      # photo: full-width ribs between key rows underneath (same height as the collars)
OVAL_KEYS = ("SHIFT", "ALPHA", "MODE", "ON")
REPLAY_D = 15.4      # 4-way pad opening
CAP_CLR = 0.25       # guess: cap to opening clearance per side
CAP_FLANGE = 0.4     # guess: retaining skirt at the bottom of each cap, per side
FLANGE_T = 0.4       # guess
KEY_PROUD = 1.5      # guess (photos): key tops above the face
REPLAY_PROUD = 1.2   # guess
CROWN_TAPER = 20.0   # degrees, bevel on the part above the face
DIRS = ("UP", "DOWN", "LEFT", "RIGHT")

# â”€â”€ Board and keymat â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
BOARD = (65.05, 97.9)        # C6 Casio board
BOARD_T = 0.8                # board 0.8 (also our PCB)
D4_IN = 6.0                  # D4 (rev B2): board back face -> inside of the back floor
Z_BOARD_B = 1.0 + D4_IN      # 7.0 = D5 floor + D4
Z_BOARD_F = Z_BOARD_B + BOARD_T              # 7.8; key side 4.0 / 3.5 below the outside of the face (top / keys)
assert abs(Z_BOARD_B - D1_RIM - SEAM_Z) < 1e-9
MAT_T = 0.8                  # guess: rubber keymat sheet, lying on the board (photo 28bb9c4b)
DOME_GAP = 0.3               # guess: black contact above the board pad (key travel)
D9 = 1.5                     # D9: top of the button rubber -> black contact
Z_DOME_TOP = Z_BOARD_F + DOME_GAP + D9       # 8.1, caps rest here
Z_CAP_BOT = Z_DOME_TOP + 0.02
Z_MEET = 5.5                                 # guess: screw posts (front) meet bosses (back) here
Z_LOC_END = Z_BOARD_B - 0.3                  # locating posts go through the board

# â”€â”€ Posts (front shell) and bosses (back cover) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
SCREW_POST_D = 3.85  # C7
LOC_POST_D = 2.9     # C7
PILOT_D = 1.7        # guess: self-tapping screw pilot (the "hole" in a screw post)
LOC_BORE_D = 1.2     # guess: bore in each locating post (photo 2a97120d: the white posts are tubes)
LOC_BORE_DEPTH = 3.0 # guess
BOSS_D = 5.5         # guess: back-cover boss
SCREW_D, CBORE_D, CBORE_H = 2.2, 4.4, 1.2    # guess: screw clearance + counterbore from outside
CORNER_PITCH = 68.1  # C9  top-corner screw posts centre to centre
CORNER_FROM_SIDE, CORNER_FROM_TOP = 4.2, 6.1  # C11 (kept as a check: depends on the guessed skin)
BOTTOM_PITCH = 39.0  # C10 bottom screw posts centre to centre (39-40; board notches 38.94)
MID_PITCH, MID_XC = 46.5, -0.8   # C8 row-2 screw pair spacing; centre x from the mat photo (stage 10)
H8H10_PITCH = 42.3   # C8 second pair (KiCad H8/H10, the row just under the screws)
# C15: case top edge -> far edge of each post's hole ("to the outside of the holes"). With the hole
# radius taken off, the rows agree with the photo-fitted posts to <= 1.05 mm (centres: <= 1.65).
# Row 3 (H8/H10, ~80 mm) was not read; it stays at the photo position.
C15 = {"corner": 8.9, "mid": 69.95, "H4": 89.6, "H2/H9": 120.45, "H6": 130.2, "H13/H14": 142.4, "bottom": 154.6}
LOC_POSTS = ["H2", "H4", "H6", "H8", "H9", "H10", "H13", "H14"]   # x from the board snapshot (photo fit)
LOC_ROW = {"H4": "H4", "H2": "H2/H9", "H9": "H2/H9", "H6": "H6", "H13": "H13/H14", "H14": "H13/H14"}

# â”€â”€ Side-wall stubs and wire holders (screen section) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
# C14: stubs stick in 5.45 (from the outside, i.e. 0.75 past the 4.7 wall of C4) and are 5 high;
# C4: 5.5 with the wire holder. Y positions from photo 2a97120d (3 stubs per side, holder near the
# window's bottom corners). Stub width along Y is a guess.
STUB_Y, STUB_IN, STUB_W = (37.3, 47.5, 56.0), 5.45, 1.5
STUB_Z0 = 7.8 + 0.2         # 8.0 = 6.5 below the rim: the level the zone-6 relief files the rib down to (was "the stubs
                            # stop above the board plane" until rev F; superseded by the rev G depth-rod readings below).
# rev G (2026-10-06, verification/12): depth-rod readings below the rim, BOTH sides symmetric (Nirav, ruler across the rim):
#   thin inner rib top edge 4.5-5.0 (worst case 4.5: the rib crosses the board, Z 7.0-7.8, by 1.0-1.8 mm),
#   round pins: outer two 5.1-5.5 (top at/just past the board's component face), middle 6.0-6.1 (inside the board thickness),
#   U-shaped wire hooks (and the C4 holder) 5.5 (top level with the board's component face).
INNER_RIB_Z0 = SEAM_Z + 4.5                                   # 6.0
PIN_Z0 = {STUB_Y[0]: SEAM_Z + 5.1, STUB_Y[1]: SEAM_Z + 6.0, STUB_Y[2]: SEAM_Z + 5.1}   # 6.6 / 7.5 / 6.6 (outer, middle, outer)
HOOK_Z0 = SEAM_Z + 5.5                                        # 7.0 (hooks and the C4 holder arm)
HOLDER_Y, HOLDER_IN, HOLDER_W = 26.0, 5.5, 2.0
COIN_CUP = (12.4, 0.8, 5.5)  # LR44 cup in the front shell (photos 66bb7a5f, 2a97120d): inside dia, wall, D7 height

# ── Rev D (2026-10-05 evening): faceplate details from photos e82c982d / 2a97120d / 2f54d6c7 ─────────────
# LR44 holder (stays: stage 14, battery on the back floor): the ring is open left and right (spring-clip slots),
# with an I-bar either side joined to the ring ends by short lugs, and a low contact rib across the cup floor.
COIN_GAP = 4.0               # photo: gap in the ring on each side (along Y)
COIN_BAR_X = 9.0             # I-bar centre, from the cup centre (X); photo: the holder box is ~21 wide
COIN_BAR = (1.0, 14.0, 2.6, 0.8)   # bar thickness (X), length (Y), end-flange length (X), flange thickness (Y)
COIN_LUG_T = 0.8             # lugs from the ring ends out to the bars
COIN_CONTACT = (0.8, 1.2, 1.5)     # contact rib across the cup: width, height from the plate, Y offset from the centre
# Wire hooks on the inner ribs beside the window: photo 2a97120d shows 3 hooks + 3 pins per side. Y of each hook
# and the direction of its return leg (+1 = towards the top). The C4 holder (HOLDER_Y) is the lowest one.
HOOKS = ((63.5, +1), (58.1, +1), (HOLDER_Y, -1))
HOOK_W, HOOK_LEG = 0.8, (1.6, 0.5)   # arm width along Y (A, B; the C4 holder keeps HOLDER_W), return leg length, thickness
# Comb snap tabs mid top / mid bottom (photo e82c982d "III"; 2a97120d): (end, X centre, width), photo +-2 mm.
SNAP_TABS = (("top", 5.0, 9.0), ("bottom", 3.5, 7.0))
SNAP_TEETH, SNAP_GAP = 3, 1.0
SNAP_TOP = (1.6, SEAM_Z - 0.4)       # top teeth: depth in from the top wall's inside face, bottom Z (0.4 below the rim)
SNAP_BOT = (1.1, 2.5, 2.4)           # bottom teeth: from / to (in from the back outline, inside the navy wall), bottom Z
SNAP_CATCH = (0.7, 1.9, 2.2, 1.0)    # navy catch at the bottom (photo 2f54d6c7): from / to (in from the outline), top Z, extra width
SNAP_POCKET = (0.2, 0.95, 3.1)       # navy notch for the top teeth: clearance all round, bottom Z, top Z (through the lip)
# Keypad underside (photos e82c982d / 76dd27d2): beads on the row ribs between columns, a row rib under the last row,
# crush ribs round the locating posts.
BEAD_D = 1.4
CRUSH = (4, 0.5, 0.6, 0.9)           # ribs per locating post, width, reach past the post, height under the plate

# â”€â”€ Back cover â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
RIB_T = 1.0          # guess
RING_T = 1.0         # guess
UPPER_RIB_H = 1.0    # D13 rib grid (screen section)
RING_H = 4.0         # D13 small ring / big ring
LOWER_RIB_H = 1.0    # D13 lower ribs
MID_RIB_H = {"mid rib vert": RING_H, "mid rib (stepped)": UPPER_RIB_H}   # guess (the vertical one runs through the small ring)
SOLAR_BOX = (-4.0, 29.0, 62.9, 74.9)   # stage 10 photo: ribbed box behind the solar cell (X0, X1, Y0, Y1)
SOLAR_BOX_H = 5.5    # D6 frame 5-6
SOLAR_GRID_H = 5.0   # D13 solar panel grid; photo 050df9b7: 4 x 2 cells
DOOR_C = (-18.3, 69.94)   # stage 10 photo: battery opening centre
DOOR = (16.0, 15.0)  # guess: shallow rectangular recess round the battery hole (photo e82c982d)
DOOR_R, DOOR_CLR = 2.0, 0.15
DOOR_DEPTH = 0.6     # guess: recess depth = lid thickness
BATT_HOLE_D = 12.0   # photo: round LR44 hole (with a key notch)
CENTRE_HOLE_D = 2.0  # photo e82c982d: 7th hole over the small ring (purpose unknown)
TICK = (2.5, 0.8)    # photo: short ribs on the inside of the long walls (length, thickness)
TICK_Y = (62.0, 44.0, 26.0, 8.0, -10.0, -28.0, -46.0, -62.0)   # guess spacing, in pairs 2 mm apart
FEET = [(27.0, 52.0), (-27.0, 52.0), (27.0, -62.0), (-27.0, -62.0)]   # guess
FOOT_D, FOOT_POCKET = 7.0, 0.4

LCD, LCD_T = (68.0, 28.0), 3.75   # D8 thickness; width from photo fd923128 (fits between the stubs), height guess

# â”€â”€ Slide-on hard case (navy, separate part) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
# Casio's fx-115ES PLUS guide: "slide its hard case downwards to remove it, and then affix the hard case
# to the back of the calculator" -> open end at the TOP (display end), closed end at the BOTTOM, in both
# positions. Stored: over the front; in use: flipped about the long axis and slid onto the back from below.
# No photo or measurement of the case yet: every size here is a guess (see QUESTIONS_AND_ISSUES.md).
CASE_CLR = 0.3        # guess: side / end clearance to the calculator outline
CASE_WALL = 1.0       # guess: side and end wall thickness
CASE_FLOOR = 1.2      # guess: floor (the face that covers the keys / the back)
CASE_KEY_CLR = 0.3    # guess: floor inside face above the key tops when stored
CASE_LIP_CLR = 0.1    # lip to groove-bottom clearance (lip reaches GROOVE_D - this past the outline)
CASE_Y_STRAIGHT = -55.0   # side walls straight above this Y (the slide direction); closed end follows the bottom outline below
CASE_SLIDE = 0.0      # how far the case is pulled off (towards -Y) from fully seated; 0 = fully on
CASE_COLOR = "navy"

SPLINE_STEP = 2.5    # outer surface = smooth spline through the trace every 2.5 mm

# â”€â”€ Fit check (stage fitcheck): our PCB in the replica â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
FIT_STEP = os.path.join(REPO, "hardware", "fab", "ai_calc_board.step")
FIT_PCB = os.path.join(REPO, "hardware", "kicad", "ai_calc.kicad_pcb")      # read only
FIT_TARGETS = ("Front shell", "Back cover", "Keymat", "Battery lid", "Solar cell (dummy)")
# footprints whose 3D model is missing from the STEP get a box of this height (pattern in the footprint name)
FIT_PROXY_H = [("ESP32-S3-MINI", 2.55), ("QFN", 0.9)]
# KiCad KEEPOUTS (User.3) rectangles turned into parts: (size key, label, box size, height, side of the board)
FIT_KEEPOUT_PARTS = [((12.0, 12.0), "camera module", (8.5, 8.5), 5.4, "F"),     # D4 note: needs 5.4 (+0.3)
                     ((31.0, 11.5), "LiPo battery", (31.0, 11.5), 3.8, "F"),
                     ((59.2, 29.2), "e-paper panel (P3)", EPD_P3[:2], EPD_P3[2], "B")]

def F(x, y):
    """KiCad point -> front-view point."""
    return (CX - x, CY - y)

# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
# 2D geometry (pure Python, runs offline and inside Fusion)
# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
def _unit(v):
    l = math.hypot(*v)
    return (v[0] / l, v[1] / l)

def area(P):
    return sum(P[i - 1][0] * P[i][1] - P[i][0] * P[i - 1][1] for i in range(len(P))) / 2

def trace_outline(length):
    """Traced fx-115ES silhouette scaled to W x length, CCW, centred on the trace's mid-height."""
    d = json.load(open(os.path.join(ENC, "fx115es_outline.json")))   # one closed loop (re-chained 2026-10-03)
    xs = [p[0] for p in d["outline"]]; ys = [p[1] for p in d["outline"]]
    sx, sy = W / (max(xs) - min(xs)), length / (max(ys) - min(ys))
    yc = (max(ys) + min(ys)) / 2                       # same placement as build_case.fx115_outline
    V = [(-(x - CX) * sx, CY - yc - (y - yc) * sy) for x, y in d["outline"]]
    # straighten the long sides to W (C2 = C3 = 79.3): per-Y x scale, held constant beyond Y_STRAIGHT
    y0, y1 = Y_STRAIGHT
    hw = lambda y: (x_at(V, y, +1) - x_at(V, y, -1)) / 2
    s0, s1 = W / 2 / hw(y0), W / 2 / hw(y1)
    V = [(x * (W / 2 / hw(y) if y0 < y < y1 else (s0 if y <= y0 else s1)), y) for x, y in V]
    V = [p for i, p in enumerate(V) if math.dist(p, V[i - 1]) > 0.3]
    changed = True
    while changed:
        changed = False
        for i in range(len(V)):
            a, p, c = V[i - 1], V[i], V[(i + 1) % len(V)]
            cr = (p[0] - a[0]) * (c[1] - p[1]) - (p[1] - a[1]) * (c[0] - p[0])
            if abs(cr) < 0.02 * math.dist(a, p) * math.dist(p, c):
                V.pop(i); changed = True; break
    return V if area(V) > 0 else V[::-1]

def offset_var(V, dfun):
    """Offset a CCW polygon inwards; dfun(outward normal) gives the distance for each edge (miter joins)."""
    n = len(V)
    lines = []
    for i in range(n):
        a, b = V[i], V[(i + 1) % n]
        u = _unit((b[0] - a[0], b[1] - a[1]))
        d = dfun((u[1], -u[0]), ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2))
        lines.append(((a[0] - u[1] * d, a[1] + u[0] * d), u, d))
    out = []
    for i in range(n):
        (p, u, d1), (q, w, d2) = lines[i - 1], lines[i]
        den = u[0] * w[1] - u[1] * w[0]
        own = q                                       # this edge's offset start
        if abs(den) < 1e-6:
            out.append(own); continue
        t = ((q[0] - p[0]) * w[1] - (q[1] - p[1]) * w[0]) / den
        x = (p[0] + u[0] * t, p[1] + u[1] * t)
        pend = (V[i][0] - u[1] * d1, V[i][1] + u[0] * d1)    # previous edge's offset end
        ok = math.dist(x, V[i]) < 1.5 * max(d1, d2, 0.1) + 0.3
        out.append(x if ok else ((pend[0] + own[0]) / 2, (pend[1] + own[1]) / 2))
    return untangle(out)

def untangle(Pl, span=8):
    """Remove the small loops an inward offset makes at tight spots: when edge i crosses edge j
    (j a few edges further on), drop the vertices between them and keep the crossing point."""
    Pl = list(Pl)
    changed = True
    while changed:
        changed = False
        n = len(Pl)
        for i in range(n):
            a, b = Pl[i], Pl[(i + 1) % n]
            for k in range(2, min(span, n - 2)):
                j = (i + k) % n
                c, d = Pl[j], Pl[(j + 1) % n]
                den = (b[0] - a[0]) * (d[1] - c[1]) - (b[1] - a[1]) * (d[0] - c[0])
                if abs(den) < 1e-12:
                    continue
                t = ((c[0] - a[0]) * (d[1] - c[1]) - (c[1] - a[1]) * (d[0] - c[0])) / den
                u = ((c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0])) / den
                if 0 < t < 1 and 0 < u < 1:
                    xp = (a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1]))
                    idx = [(i + m) % n for m in range(1, k + 1)]      # vertices i+1 .. j
                    keep = [Pl[m] for m in range(n) if m not in idx]
                    pos = keep.index(a) + 1
                    Pl = keep[:pos] + [xp] + keep[pos:]
                    changed = True
                    break
            if changed:
                break
    out = []
    for p in Pl:                                      # drop near-duplicate points (Fusion merges them badly)
        if not out or math.dist(p, out[-1]) > 0.05:
            out.append(p)
    while len(out) > 3 and math.dist(out[0], out[-1]) <= 0.05:
        out.pop()
    return out

def offset_const(V, d):
    return offset_var(V, lambda n, m=None: d)

def wall_fun(side, top, bottom):
    def f(n, m=None):
        w = min(1.0, max(0.0, (abs(n[1]) - 0.5) / 0.4))
        return side * (1 - w) + (top if n[1] > 0 else bottom) * w
    return f

def front_fun(n, m):
    """Inset of the front shell from the back-cover outline: 0 at the top and the screen section,
    FRONT_SIDE_INSET along the sides below it (smooth ramp), FRONT_BOTTOM_INSET at the bottom end."""
    w = min(1.0, max(0.0, (abs(n[1]) - 0.5) / 0.4))
    t = min(1.0, max(0.0, (Y_INSET0 - m[1]) / (Y_INSET0 - Y_INSET1)))
    t = t * t * (3 - 2 * t)
    return FRONT_SIDE_INSET * t * (1 - w) + (FRONT_BOTTOM_INSET if n[1] < 0 else 0.0) * w

def crossings(P, y0):
    out = []
    for i in range(len(P)):
        a, b = P[i], P[(i + 1) % len(P)]
        if (a[1] - y0) * (b[1] - y0) < 0:
            t = (y0 - a[1]) / (b[1] - a[1])
            out.append((i, (a[0] + t * (b[0] - a[0]), y0), b[1] > a[1]))
    assert len(out) == 2, "expected 2 crossings at y=%.2f" % y0
    up = [c for c in out if c[2]][0]; down = [c for c in out if not c[2]][0]
    return up, down                                   # right side goes up, left side goes down (CCW)

def chain(P, i0, i1):
    """Vertices i0+1 .. i1 (mod n)."""
    n, out, i = len(P), [], i0
    while i != i1:
        i = (i + 1) % n
        out.append(P[i])
    return out

def cavity(V):
    """Inside of the front shell: thin side walls below Y_SCREEN_SEC, thick ones above."""
    K = offset_var(V, wall_fun(WALL_KEY, WALL_TOP, WALL_BOTTOM))
    S = offset_var(V, wall_fun(WALL_SCREEN, WALL_TOP, WALL_BOTTOM))
    (kr, kr_p, _), (kl, kl_p, _) = crossings(K, Y_SCREEN_SEC)
    (sr, sr_p, _), (sl, sl_p, _) = crossings(S, Y_SCREEN_SEC)
    C = [kl_p] + chain(K, kl, kr) + [kr_p, sr_p] + chain(S, sr, sl) + [sl_p]
    C = [p for i, p in enumerate(C) if math.dist(p, C[i - 1]) > 0.05]
    assert area(C) > 0
    return C, K, S

def resample(P, step):
    """Points every ~step mm along a closed polygon (spline fit points for the outer surface)."""
    per = sum(math.dist(P[i - 1], P[i]) for i in range(len(P)))
    n = int(per / step)
    out, i, acc = [], 0, 0.0
    segs = [(P[i], P[(i + 1) % len(P)]) for i in range(len(P))]
    k, pos = 0, 0.0
    for j in range(n):
        target = j * per / n
        while pos + math.dist(*segs[k]) < target:
            pos += math.dist(*segs[k]); k += 1
        a, b = segs[k]
        t = (target - pos) / max(1e-9, math.dist(a, b))
        out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    # light smoothing (the trace has ~0.1 mm pixel noise)
    m = len(out)
    return [((out[j - 1][0] + 2 * out[j][0] + out[(j + 1) % m][0]) / 4,
             (out[j - 1][1] + 2 * out[j][1] + out[(j + 1) % m][1]) / 4) for j in range(m)]

def clip_below(P, y0):
    """Part of polygon P with y <= y0 (Sutherland-Hodgman, one edge)."""
    out = []
    for i in range(len(P)):
        a, b = P[i - 1], P[i]
        ina, inb = a[1] <= y0, b[1] <= y0
        if ina != inb:
            t = (y0 - a[1]) / (b[1] - a[1])
            out.append((a[0] + t * (b[0] - a[0]), y0))
        if inb:
            out.append(b)
    return out

def clip_above(P, y0):
    """Part of polygon P with y >= y0."""
    return [(x, -y) for x, y in clip_below([(x, -y) for x, y in P], -y0)][::-1]

def x_at(P, y, side):
    xs = []
    for i in range(len(P)):
        (x1, y1), (x2, y2) = P[i - 1], P[i]
        if (y1 - y) * (y2 - y) <= 0 and y1 != y2:
            xs.append(x1 + (y - y1) * (x2 - x1) / (y2 - y1))
    return max(xs) if side > 0 else min(xs)

def y_at(P, x, side):
    ys = []
    for i in range(len(P)):
        (x1, y1), (x2, y2) = P[i - 1], P[i]
        if (x1 - x) * (x2 - x) <= 0 and x1 != x2:
            ys.append(y1 + (x - x1) * (y2 - y1) / (x2 - x1))
    return max(ys) if side > 0 else min(ys)

def side_band(P, xmin, xout, right):
    """Polygon between the part of P beyond |x| > xmin on one side and a far line at |x| = xout."""
    s = 1 if right else -1
    # rotate so the run is contiguous (P is CCW, the run never wraps more than once)
    idx = [k for k, p in enumerate(P) if p[0] * s > xmin]
    runs, cur = [], [idx[0]]
    for a, b in zip(idx, idx[1:]):
        if b == a + 1:
            cur.append(b)
        else:
            runs.append(cur); cur = [b]
    runs.append(cur)
    if len(runs) > 1 and runs[0][0] == 0 and runs[-1][-1] == len(P) - 1:
        runs = [runs[-1] + runs[0]] + runs[1:-1]
    run = max(runs, key=len)
    pts = [P[k] for k in run]
    y_first, y_last = pts[0][1], pts[-1][1]
    return pts + [(s * xout, y_last), (s * xout, y_first)]

def _rect(c, w, h):
    x, y = c
    return [(x - w / 2, y - h / 2), (x + w / 2, y - h / 2), (x + w / 2, y + h / 2), (x - w / 2, y + h / 2)]

def rrect_pts(c, w, h, r, n=6):
    """Rounded rectangle as a polygon (for 2D checks)."""
    x, y = c
    r = min(r, w / 2, h / 2)
    pts = []
    for cx_, cy_, a0 in ((x + w / 2 - r, y + h / 2 - r, 0), (x - w / 2 + r, y + h / 2 - r, 90),
                         (x - w / 2 + r, y - h / 2 + r, 180), (x + w / 2 - r, y - h / 2 + r, 270)):
        for k in range(n + 1):
            a = math.radians(a0 + 90 * k / n)
            pts.append((cx_ + r * math.cos(a), cy_ + r * math.sin(a)))
    return pts

def seg_dist(p, a, b):
    ab = (b[0] - a[0], b[1] - a[1]); l2 = ab[0] ** 2 + ab[1] ** 2
    t = 0 if l2 == 0 else max(0, min(1, ((p[0] - a[0]) * ab[0] + (p[1] - a[1]) * ab[1]) / l2))
    return math.dist(p, (a[0] + ab[0] * t, a[1] + ab[1] * t))

def poly_dist(p, P):
    return min(seg_dist(p, P[i - 1], P[i]) for i in range(len(P)))

def key_open(name, w):
    k = "oval" if name in OVAL_KEYS else round(w)
    ow, oh = KEY_OPEN.get(k, KEY_OPEN[7.0])
    r = None if k == "oval" else KEY_R.get(k, (0.8, 2.0))
    return ow, oh, r

def densify(P, step):
    """Split long edges so no edge is longer than step (keeps the original corners)."""
    out = []
    for i in range(len(P)):
        a, b = P[i], P[(i + 1) % len(P)]
        k = max(1, int(math.ceil(math.dist(a, b) / step)))
        out += [(a[0] + (b[0] - a[0]) * j / k, a[1] + (b[1] - a[1]) * j / k) for j in range(k)]
    return out

def tray_band(Vb):
    """Closed polygon of the back cover's tray wall: from the outline inwards by (front inset - gap),
    only where that is positive (keypad section sides and the bottom end)."""
    n = len(Vb)
    outer, inner, keep = [], [], []
    for i in range(n):
        a, p, b = Vb[i - 1], Vb[i], Vb[(i + 1) % n]
        u = _unit((b[0] - a[0], b[1] - a[1]))
        nrm = (u[1], -u[0])                            # outward
        d = front_fun(nrm, p) - TRAY_GAP
        keep.append(d > 0.05)
        outer.append((p[0] + nrm[0] * 0.3, p[1] + nrm[1] * 0.3)); inner.append((p[0] - nrm[0] * d, p[1] - nrm[1] * d))
    start = next(i for i in range(n) if keep[i] and not keep[i - 1])
    run = []
    i = start
    while keep[i]:
        run.append(i); i = (i + 1) % n
    return [outer[k] for k in run] + [inner[k] for k in reversed(run)]

def side_strip(Vb, d, ylo, yhi):
    """Two closed polygons (one per long side): from 0.3 outside the outline to d inside it, ylo <= Y <= yhi."""
    n = len(Vb)
    out = []
    for sg in (+1, -1):
        idx = [i for i in range(n) if ylo <= Vb[i][1] <= yhi and Vb[i][0] * sg > 0]
        idx.sort(key=lambda i: Vb[i][1])
        outer, inner = [], []
        for i in idx:
            a, p, b = Vb[i - 1], Vb[i], Vb[(i + 1) % n]
            u = _unit((b[0] - a[0], b[1] - a[1]))
            nrm = (u[1], -u[0])
            outer.append((p[0] + nrm[0] * 0.3, p[1] + nrm[1] * 0.3)); inner.append((p[0] - nrm[0] * d, p[1] - nrm[1] * d))
        out.append(outer + inner[::-1])
    return out

def perimeter_strip(Vb, d, ymax):
    """One closed U-shaped polygon: the part of the outline below ymax (both long sides + the bottom end),
    from 0.3 outside to d inside, following the outline order."""
    n = len(Vb)
    keep = [Vb[i][1] <= ymax for i in range(n)]
    start = next(i for i in range(n) if keep[i] and not keep[i - 1])
    run, i = [], start
    while keep[i]:
        run.append(i); i = (i + 1) % n
    outer, inner = [], []
    for i in run:
        a, p, b = Vb[i - 1], Vb[i], Vb[(i + 1) % n]
        u = _unit((b[0] - a[0], b[1] - a[1]))
        nrm = (u[1], -u[0])
        outer.append((p[0] + nrm[0] * 0.3, p[1] + nrm[1] * 0.3)); inner.append((p[0] - nrm[0] * d, p[1] - nrm[1] * d))
    return untangle(outer + inner[::-1])

def case_plan(Vb, off, ytop):
    """Slide-case plan outline offset `off` outside the calculator: straight long sides (x = +-(W/2 + off))
    from CASE_Y_STRAIGHT up to ytop (open end), closed end following the bottom outline."""
    a = W / 2 + off
    V = offset_const(Vb, -off)
    B = clip_below(V, CASE_Y_STRAIGHT)
    B = [(max(-a, min(a, x)), y) for x, y in B]
    # the clipped polygon closes with an edge along y = CASE_Y_STRAIGHT from the right crossing to the left one
    k = max(range(len(B)), key=lambda i: (abs(B[i][1] - CASE_Y_STRAIGHT) < 1e-6, B[i][0]))
    P_ = B[:k + 1] + [(a, CASE_Y_STRAIGHT), (a, ytop), (-a, ytop), (-a, CASE_Y_STRAIGHT)] + B[k + 1:]
    P_ = [p for i, p in enumerate(P_) if math.dist(p, P_[i - 1]) > 0.05]
    return P_ if area(P_) > 0 else P_[::-1]

def geometry():
    """Everything the Fusion stages draw, in one dict (also written to replica/geometry.json)."""
    snap = json.load(open(os.path.join(OUT, "board_snapshot.json")))
    Vb = trace_outline(L_BACK)
    Vd = densify(Vb, 1.0)                              # the inset varies along Y: needs short edges
    Vf = offset_var(Vd, front_fun)
    K = offset_var(Vf, wall_fun(WALL_KEY, WALL_TOP, WALL_BOTTOM))       # inside of the front shell skin
    C = K
    top = max(y for x, y in Vf)
    win_c = (0.0, top - WIN_TOP_FROM_EDGE - WIN[1] / 2)

    # screw posts: x from C9 / C8 / C10, y from C15 (top edge -> far edge of the pilot hole)
    yrow = lambda key, hole_d: top - (C15[key] - hole_d / 2)
    cy_ = yrow("corner", PILOT_D)
    my_ = yrow("mid", PILOT_D)
    by_ = yrow("bottom", PILOT_D)
    screws = [("corner R", (CORNER_PITCH / 2, cy_)), ("corner L", (-CORNER_PITCH / 2, cy_)),
              ("mid R", (MID_XC + MID_PITCH / 2, my_)), ("mid L", (MID_XC - MID_PITCH / 2, my_)),
              ("bottom R", (BOTTOM_PITCH / 2, by_)), ("bottom L", (-BOTTOM_PITCH / 2, by_))]
    # locating posts: x from the photo fit (board snapshot), y from C15 where read; H8/H10 spacing from C8
    holes = {h[0]: tuple(h[1]) for h in snap["holes"]}
    locs = []
    for n in LOC_POSTS:
        x, y = holes[n]
        if n in LOC_ROW:
            y = yrow(LOC_ROW[n], LOC_BORE_D)
        if n in ("H8", "H10"):
            xc = (holes["H8"][0] + holes["H10"][0]) / 2
            x = xc + (H8H10_PITCH / 2 if n == "H8" else -H8H10_PITCH / 2)
        locs.append((n, (x, y)))
    photo_posts = {n: holes[n] for n in LOC_POSTS}
    photo_posts.update({"corner R": (32.66, 73.15), "mid R": (22.4, 12.74), "mid L": (-24.0, 12.74),
                        "bottom R": (19.5, -71.1)})      # rev A (photo / C11) positions, for the check

    keys = [(n, tuple(c), w, h) for n, c, w, h in snap["keys"]]
    replay = tuple(snap["replay"])
    openings = []
    for n, c, w, h in keys:
        if n in DIRS:
            continue
        ow, oh, r = key_open(n, w)
        openings.append(dict(name=n, c=c, w=ow, h=oh, r=r, oval=n in OVAL_KEYS, number=round(w) == 9))

    # inner rib beside the screen (wire channel between it and the skin) + 3 pins per side
    inner_ribs, pins = [], []
    y0, y1 = INNER_RIB_Y
    for sg in (+1, -1):
        xi = lambda y: x_at(Vf, y, sg) - sg * WALL_SCREEN
        inner_ribs.append([(xi(y0), y0), (xi(y0) + sg * INNER_RIB_T, y0), (xi(y1) + sg * INNER_RIB_T, y1), (xi(y1), y1)])
        for y, depth, w in [(y, STUB_IN, STUB_W) for y in STUB_Y] + [(HOLDER_Y, HOLDER_IN, HOLDER_W)]:
            x, xt = xi(y), x_at(Vf, y, sg) - sg * depth      # from the inner rib face to C14 / C4 depth
            pins.append([(x + sg * 0.1, y - w / 2), (x + sg * 0.1, y + w / 2), (xt, y + w / 2), (xt, y - w / 2)])

    # full-width ribs under the plate between key rows (photo 76dd27d2), and short ones beside REPLAY
    rows = sorted({(round(o["c"][1], 2), o["h"]) for o in openings if not o["oval"]}, reverse=True)
    row_ribs = []
    for (ya, ha), (yb, hb) in zip(rows, rows[1:]):
        ym = ((ya - ha / 2) + (yb + hb / 2)) / 2
        row_ribs.append([(x_at(K, ym, -1) - 0.3, ym), (x_at(K, ym, +1) + 0.3, ym)])
    ym = (rows[0][0] + rows[0][1] / 2 + min(o["c"][1] - o["h"] / 2 for o in openings if o["oval"])) / 2
    for sg in (+1, -1):
        row_ribs.append([(sg * (REPLAY_D / 2 + COLLAR[1] - 0.2), ym), (x_at(K, ym, sg) + sg * 0.3, ym)])
    # rev D: a rib under the last row too (photo e82c982d), beads on the full-width ribs between the columns
    yl, hl = rows[-1]
    by = min(c[1] for n, c in screws)                   # photo: it runs just above the bottom screw bosses
    ym = (yl - hl / 2 + by) / 2
    row_ribs.append([(x_at(K, ym, -1) - 0.3, ym), (x_at(K, ym, +1) + 0.3, ym)])
    beads = []
    for k, (ya, ha) in enumerate(rows):
        xs = sorted(o["c"][0] + s * o["w"] / 2 for o in openings if not o["oval"] and abs(o["c"][1] - ya) < 0.02 for s in (-1, 1))
        yrs = [r[0][1] for r in row_ribs[:len(rows) - 1]] + [ym]
        yr = yrs[k]                                    # the rib just below row k
        beads += [((xs[i] + xs[i + 1]) / 2, yr) for i in range(1, len(xs) - 1, 2)
                  if all(math.dist(((xs[i] + xs[i + 1]) / 2, yr), c) > SCREW_POST_D / 2 + 1.0 for n, c in screws + locs)]

    # rev D: wire hooks (J shape) on the inner ribs, LR44 holder, comb snap tabs, crush ribs
    hooks = []
    for sg in (+1, -1):
        xi = lambda y: x_at(Vf, y, sg) - sg * WALL_SCREEN
        for y, d in HOOKS:
            held = abs(y - HOLDER_Y) < 1e-6
            w = HOLDER_W if held else HOOK_W
            xt = x_at(Vf, y, sg) - sg * HOLDER_IN                 # tip at the C4 depth (5.5 from the outside)
            if not held:                                          # arm (the C4 holder's arm is already in "pins")
                hooks.append([(xi(y) + sg * 0.1, y - w / 2), (xi(y) + sg * 0.1, y + w / 2), (xt, y + w / 2), (xt, y - w / 2)])
            ye = y + d * w / 2
            hooks.append([(xt, ye - d * 0.01), (xt + sg * HOOK_LEG[1], ye - d * 0.01), (xt + sg * HOOK_LEG[1], ye + d * HOOK_LEG[0]),
                          (xt, ye + d * HOOK_LEG[0])])
    cx_, cy_ = DOOR_C
    ro = COIN_CUP[0] / 2 + COIN_CUP[1]
    coin = []
    for sg in (+1, -1):
        xb = cx_ + sg * COIN_BAR_X
        t, L, fl, ft = COIN_BAR
        coin.append(_rect((xb, cy_), t, L))
        for sy in (+1, -1):
            coin.append(_rect((xb, cy_ + sy * (L / 2 - ft / 2)), fl, ft))
            yl_ = cy_ + sy * (COIN_GAP / 2 + COIN_LUG_T / 2)
            x0_ = cx_ + sg * (COIN_CUP[0] / 2 + 0.2)
            coin.append(_rect(((x0_ + xb) / 2, yl_), abs(xb - x0_), COIN_LUG_T))
    snap = dict(teeth=[], pocket=[], catch=[])
    for end, xc, wt in SNAP_TABS:
        tw = (wt - (SNAP_TEETH - 1) * SNAP_GAP) / SNAP_TEETH
        for i in range(SNAP_TEETH):
            x = xc - wt / 2 + tw / 2 + i * (tw + SNAP_GAP)
            if end == "top":
                yc = y_at(C, x, +1)
                snap["teeth"].append(("top", [(x - tw / 2, yc + 0.3), (x + tw / 2, yc + 0.3), (x + tw / 2, yc - SNAP_TOP[0]),
                                              (x - tw / 2, yc - SNAP_TOP[0])]))
            else:
                yb = y_at(Vb, x, -1)
                snap["teeth"].append(("bottom", [(x - tw / 2, yb + SNAP_BOT[0]), (x + tw / 2, yb + SNAP_BOT[0]),
                                                 (x + tw / 2, yb + SNAP_BOT[1]), (x - tw / 2, yb + SNAP_BOT[1])]))
        if end == "top":
            yc = min(y_at(C, x, +1) for x in (xc - wt / 2, xc, xc + wt / 2))
            c_ = SNAP_POCKET[0]
            snap["pocket"] = [(xc - wt / 2 - c_, yc - SNAP_TOP[0] - c_), (xc + wt / 2 + c_, yc - SNAP_TOP[0] - c_),
                              (xc + wt / 2 + c_, yc + 0.4), (xc - wt / 2 - c_, yc + 0.4)]
        else:
            ys = [y_at(Vb, x, -1) for x in (xc - wt / 2 - SNAP_CATCH[3] / 2, xc, xc + wt / 2 + SNAP_CATCH[3] / 2)]
            snap["catch"] = [(xc - (wt + SNAP_CATCH[3]) / 2, max(ys) + SNAP_CATCH[0]), (xc + (wt + SNAP_CATCH[3]) / 2, max(ys) + SNAP_CATCH[0]),
                             (xc + (wt + SNAP_CATCH[3]) / 2, min(ys) + SNAP_CATCH[1]), (xc - (wt + SNAP_CATCH[3]) / 2, min(ys) + SNAP_CATCH[1])]
    crush = []
    for n, (x, y) in locs:
        for k in range(CRUSH[0]):
            a = math.pi / 4 + k * 2 * math.pi / CRUSH[0]
            r0, r1 = LOC_POST_D / 2 - 0.2, LOC_POST_D / 2 + CRUSH[2]
            u, v = (math.cos(a), math.sin(a)), (-math.sin(a) * CRUSH[1] / 2, math.cos(a) * CRUSH[1] / 2)
            crush.append([(x + u[0] * r0 + v[0], y + u[1] * r0 + v[1]), (x + u[0] * r1 + v[0], y + u[1] * r1 + v[1]),
                          (x + u[0] * r1 - v[0], y + u[1] * r1 - v[1]), (x + u[0] * r0 - v[0], y + u[1] * r0 - v[1])])

    rb = json.load(open(os.path.join(REPO, "hardware", "fitcheck", "backcover_fx115es.json")))
    ribs, rings = [], []
    for name, v in rb.items():
        if name.startswith("_"):
            continue
        if v[0] == "ring":
            rings.append((name, F(*v[1]), v[2]))
        else:
            (x1, y1), (x2, y2) = F(*v[1]), F(*v[2])
            lim = lambda x, y: max(x_at(C, y, -1) + 1.5, min(x_at(C, y, +1) - 1.5, x))
            ylim = lambda y: min(y, y_at(C, (x1 + x2) / 2, +1) - 1.5)
            h = MID_RIB_H.get(name, UPPER_RIB_H if name.startswith("upper") else LOWER_RIB_H)   # D13
            ribs.append((name, (lim(x1, y1), ylim(y1)), (lim(x2, y2), ylim(y2)), h))
    x0, x1, y0, y1 = SOLAR_BOX
    box = [((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))]   # frame (D6)
    grid = [((x0 + k * (x1 - x0) / 4, y0), (x0 + k * (x1 - x0) / 4, y1)) for k in (1, 2, 3)] + \
           [((x0, (y0 + y1) / 2), (x1, (y0 + y1) / 2))]                                  # 4 x 2 cells (D13)

    lip_o, lip_i = offset_const(C, LIP_GAP), offset_const(C, LIP_GAP + LIP_T)
    ticks = []
    for y in TICK_Y:
        for dy in (-1.0, 1.0):
            for sg in (+1, -1):
                x = x_at(lip_i, y + dy, sg)
                ticks.append([(x + sg * 0.2, y + dy - TICK[1] / 2), (x + sg * 0.2, y + dy + TICK[1] / 2),
                              (x - sg * TICK[0], y + dy + TICK[1] / 2), (x - sg * TICK[0], y + dy - TICK[1] / 2)])

    bottom_inner = y_at(C, 0.0, -1)
    board_c = (0.0, bottom_inner + 0.4 + BOARD[1] / 2)
    ytop_case = max(y for x, y in Vb)                 # open end flush with the calculator's top
    case_in = case_plan(Vb, CASE_CLR, ytop_case)
    case_out = case_plan(Vb, CASE_CLR + CASE_WALL, ytop_case)
    ylo = Y_INSET1 - 6.0
    return dict(ramp_navy=side_strip(Vd, TRAY_WALL, ylo, SIDE_RAMP_Y[0] + 0.5),
                ramp_cut=side_strip(Vd, TRAY_WALL + TRAY_GAP, ylo, SIDE_RAMP_Y[0] + 0.5),
                case_in=case_in, case_out=case_out, case_ytop=ytop_case,
                skirt_cut=perimeter_strip(Vd, SKIRT_CUT_D, SIDE_RAMP_Y[1]),
Vf=Vf, Vb=Vb, Vf_s=resample(Vf, SPLINE_STEP), Vb_s=resample(Vb, SPLINE_STEP), C=C, K=K,
                C_top=clip_above(C, Y_RAMP[1]), photo_posts=photo_posts,
                top=top, win_c=win_c, screws=screws, locs=locs, keys=keys, replay=replay, openings=openings,
                inner_ribs=inner_ribs, pins=pins, row_ribs=row_ribs, ribs=ribs, rings=rings, box=box, grid=grid,
                ticks=ticks, board_c=board_c, board_clip=offset_const(C, 0.3), lip_o=lip_o, lip_i=lip_i,
                tray_band=tray_band(Vd), beads=beads, hooks=hooks, coin=coin, snap=snap, crush=crush)

def check(g):
    """2D sanity checks and caliper cross-checks; returns a list of text lines."""
    out = []
    Vf, C, K = g["Vf"], g["C"], g["K"]
    xs = [p[0] for p in Vf]; ys = [p[1] for p in Vf]
    out.append("front outline %.2f x %.2f mm (C2 79.3 / C1 159.9), back %.2f x %.2f (C3 79.3 x 161)" % (
        max(xs) - min(xs), max(ys) - min(ys), max(p[0] for p in g["Vb"]) - min(p[0] for p in g["Vb"]),
        max(p[1] for p in g["Vb"]) - min(p[1] for p in g["Vb"])))
    out.append("front shell width at Y=45 (screen) %.2f, at Y=-50 (keypad) %.2f" % (
        x_at(Vf, 45, 1) - x_at(Vf, 45, -1), x_at(Vf, -50, 1) - x_at(Vf, -50, -1)))
    out.append("Z stack: back floor %.1f | board %.1f-%.1f | plate underside %.1f (keys) / %.1f (screen) | face %.1f / %.1f"
               % (BACK_PLATE, Z_BOARD_B, Z_BOARD_F, Z_PLATE, Z_PLATE_TOP, T_KEY, T_TOP))
    out.append("  D1 rim -> board back %.2f (5.5) | D4 board back -> inside floor %.2f (6.0) | board front -> face %.2f top / %.2f keys"
               " | sum %.2f / %.2f (D10 11.8 / 11.3)" % (Z_BOARD_B - SEAM_Z, Z_BOARD_B - BACK_PLATE, T_TOP - Z_BOARD_F,
               T_KEY - Z_BOARD_F, BACK_PLATE + D4_IN + BOARD_T + T_TOP - Z_BOARD_F, BACK_PLATE + D4_IN + BOARD_T + T_KEY - Z_BOARD_F))
    out.append("  D3 at the top edge: board back -> outside face %.2f (about 5); board front -> plate underside %.2f; stubs end at %.2f"
               % (T_TOP - Z_BOARD_B, Z_PLATE_TOP - Z_BOARD_F, STUB_Z0))
    out.append("  rings top %.1f (%.1f under the board) | solar grid top %.1f, frame top %.1f (board back %.1f)"
               % (BACK_PLATE + RING_H, Z_BOARD_B - BACK_PLATE - RING_H, BACK_PLATE + SOLAR_GRID_H,
                  BACK_PLATE + SOLAR_BOX_H, Z_BOARD_B))
    out.append("  keys: dome top %.2f, cap bottom %.2f, face %.1f, cap top %.1f; LCD %.2f-%.2f"
               % (Z_DOME_TOP, Z_CAP_BOT, T_KEY, T_KEY + KEY_PROUD, Z_PLATE_TOP - 0.05 - LCD_T, Z_PLATE_TOP - 0.05))
    top = g["top"]
    pp = g["photo_posts"]
    for n, c in g["screws"]:
        ref = pp.get(n)
        out.append("screw post %-9s (%6.2f, %6.2f)  %.2f from top  side wall gap %.2f%s" % (
            n, c[0], c[1], top - c[1], poly_dist(c, C) - SCREW_POST_D / 2,
            "  (rev A %.2f, %.2f)" % ref if ref else ""))
    for n, c in g["locs"]:
        ref = pp[n]
        out.append("locating   %-9s (%6.2f, %6.2f)  %.2f from top  moved %.2f from the photo/board position"
                   % (n, c[0], c[1], top - c[1], math.dist(c, ref)))
    s = dict(g["screws"])
    cr = s["corner R"]
    out.append("C9  corner posts c-c %.2f (68.1)" % (2 * cr[0]))
    out.append("C11 corner post %.2f from inside side wall (4.2), %.2f from inside top wall (6.1) with the guessed skin"
               % (x_at(K, cr[1], +1) - cr[0], y_at(K, cr[0], +1) - cr[1]))
    out.append("C8  mid screws c-c %.2f (46.5), H8-H10 c-c %.2f (42.3)" % (
        s["mid R"][0] - s["mid L"][0], dict(g["locs"])["H8"][0] - dict(g["locs"])["H10"][0]))
    out.append("C10 bottom posts c-c %.2f (39-40), %.2f from inside side wall" % (
        2 * s["bottom R"][0], x_at(C, s["bottom R"][1], +1) - s["bottom R"][0]))
    out.append("C14 stub tips %.2f from the outside (5.45); holder %.2f (5.5); inside width between stub tips %.2f"
               % (STUB_IN, HOLDER_IN, 2 * (x_at(Vf, STUB_Y[1], 1) - STUB_IN)))
    out.append("window centre (%.2f, %.2f), top edge %.2f below case top" % (g["win_c"] + (g["top"] - g["win_c"][1] - WIN[1] / 2,)))
    posts = [(n, c, SCREW_POST_D) for n, c in g["screws"]] + [(n, c, LOC_POST_D) for n, c in g["locs"]]
    worst = []
    for o in g["openings"]:
        P = rrect_pts(o["c"], o["w"], o["h"], o["h"] / 2 if o["oval"] else o["r"][0])
        for n, c, d in posts:
            gap = poly_dist(c, P) - d / 2
            from_inside = abs(c[0] - o["c"][0]) < o["w"] / 2 and abs(c[1] - o["c"][1]) < o["h"] / 2
            if from_inside:
                gap = -gap
            if gap < 0.6:
                worst.append("  post %-8s vs opening %-6s gap %.2f" % (n, o["name"], gap))
    for n, c, d in posts:
        gap = math.dist(c, g["replay"]) - REPLAY_D / 2 - d / 2
        if gap < 0.6:
            worst.append("  post %-8s vs REPLAY gap %.2f" % (n, gap))
    out.append("posts closer than 0.6 mm to a key opening: %d (negative = the opening nicks the post, as on the real shell)" % len(worst))
    out += worst
    door = rrect_pts(DOOR_C, DOOR[0], DOOR[1], DOOR_R)
    for n, c in g["screws"]:
        if poly_dist(c, door) < BOSS_D / 2 + 1.0 or (abs(c[0] - DOOR_C[0]) < DOOR[0] / 2 and abs(c[1] - DOOR_C[1]) < DOOR[1] / 2):
            out.append("  WARNING boss %s %.2f from the battery door" % (n, poly_dist(c, door) - BOSS_D / 2))
    for f in FEET:
        for n, c in g["screws"]:
            if math.dist(f, c) < FOOT_D / 2 + CBORE_D / 2 + 1:
                out.append("  WARNING foot %s near screw %s" % (f, n))
    return out

# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
# Fusion side (helpers copied from build_case.py)
# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
def cm(v):
    return v / 10.0

def P(x, y, z=0.0):
    import adsk.core
    return adsk.core.Point3D.create(cm(x), cm(y), cm(z))

def log(msg):
    with open(os.path.join(OUT, "build.log"), "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

ROOT_TAG = "Replica - Front shell"

def find_design():
    """The replica document (activate it if another one is in front)."""
    import adsk.fusion
    for d in app.documents:
        p = adsk.fusion.Design.cast(d.products.itemByProductType("DesignProductType"))
        if p and any(o.component.name == ROOT_TAG for o in p.rootComponent.occurrences):
            if not d.isActive:
                d.activate()
            return p
    raise RuntimeError("replica document not open: run stage new first")

def comp_named(name, create=False):
    import adsk.core
    root = design.rootComponent
    for occ in root.occurrences:
        if occ.component.name == "Replica - " + name:
            return occ.component
    if create:
        occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        occ.component.name = "Replica - " + name
        return occ.component
    raise KeyError(name)

def body_named(comp, name):
    for b in comp.bRepBodies:
        if b.name == name:
            return b
    raise KeyError(name)

def new_sketch(comp, name):
    sk = comp.sketches.add(comp.xYConstructionPlane)
    sk.name = name
    sk.isComputeDeferred = True
    return sk

def poly(sk, pts):
    L = sk.sketchCurves.sketchLines
    n = len(pts)
    first = prev = None
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        ln = L.addByTwoPoints(prev.endSketchPoint if prev else P(*a), P(*b) if i < n - 1 else first.startSketchPoint)
        first = first or ln
        prev = ln

def spline(sk, pts):
    """Closed outline as two fitted splines (left/right halves) joined tangentially.
    A single closed spline gives a periodic edge that Fusion refuses to fillet."""
    import adsk.core
    n = len(pts)
    iR = max(range(n), key=lambda i: pts[i][0] - abs(pts[i][1]) * 0.01)
    iL = min(range(n), key=lambda i: pts[i][0] + abs(pts[i][1]) * 0.01)
    def run_(a, b):
        out, i = [pts[a]], a
        while i != b:
            i = (i + 1) % n
            out.append(pts[i])
        return out
    def open_spline(ps, start=None, end=None):
        col = adsk.core.ObjectCollection.create()
        col.add(start or P(*ps[0]))
        for p in ps[1:-1]:
            col.add(P(*p))
        col.add(end or P(*ps[-1]))
        return sk.sketchCurves.sketchFittedSplines.add(col)
    s1 = open_spline(run_(iR, iL))
    s2 = open_spline(run_(iL, iR), s1.endSketchPoint, s1.startSketchPoint)
    sk.geometricConstraints.addTangent(s1, s2)
    return s1, s2

def rrect(sk, c, w, h, r, rb=None):
    """Rounded rectangle from lines and arcs; rb (if given) is the radius of the two bottom corners."""
    x, y = c
    rt = r
    rb = r if rb is None else rb
    lim = lambda v: max(0.0, min(v, w / 2 - 0.005, h / 2 - 0.005))
    rt, rb = lim(rt), lim(rb)
    if rt + rb > h - 0.01:
        rb = h - 0.01 - rt
    L, A = sk.sketchCurves.sketchLines, sk.sketchCurves.sketchArcs
    if rt <= 0 and rb <= 0:
        poly(sk, [(x + w / 2, y - h / 2), (x + w / 2, y + h / 2), (x - w / 2, y + h / 2), (x - w / 2, y - h / 2)])
        return
    k = 1 - 1 / math.sqrt(2)
    corners = [  # (start, mid, end, radius) going CCW from the top-right corner
        ((x + w / 2, y + h / 2 - rt), (x + w / 2 - rt * k, y + h / 2 - rt * k), (x + w / 2 - rt, y + h / 2)),
        ((x - w / 2 + rt, y + h / 2), (x - w / 2 + rt * k, y + h / 2 - rt * k), (x - w / 2, y + h / 2 - rt)),
        ((x - w / 2, y - h / 2 + rb), (x - w / 2 + rb * k, y - h / 2 + rb * k), (x - w / 2 + rb, y - h / 2)),
        ((x + w / 2 - rb, y - h / 2), (x + w / 2 - rb * k, y - h / 2 + rb * k), (x + w / 2, y - h / 2 + rb))]
    for i, (s0, m, e) in enumerate(corners):
        if math.dist(s0, e) > 1e-4:
            A.addByThreePoints(P(*s0), P(*m), P(*e))
        nxt = corners[(i + 1) % 4][0]
        if math.dist(e, nxt) > 1e-4:
            L.addByTwoPoints(P(*e), P(*nxt))

def ellipse(sk, c, a, b):
    sk.sketchCurves.sketchEllipses.add(P(*c), P(c[0] + a, c[1]), P(c[0], c[1] + b))

def key_shape(sk, o, grow):
    """A key opening grown (or shrunk) by grow mm all round."""
    if o["oval"]:
        ellipse(sk, o["c"], o["w"] / 2 + grow, o["h"] / 2 + grow)
    else:
        rt, rb = o["r"]
        rrect(sk, o["c"], o["w"] + 2 * grow, o["h"] + 2 * grow, max(0.05, rt + grow), max(0.05, rb + grow))

def shape_pts(o, grow):
    """Polygon of key_shape (for centroid tests)."""
    if o["oval"]:
        a, b = o["w"] / 2 + grow, o["h"] / 2 + grow
        return [(o["c"][0] + a * math.cos(t * math.pi / 24), o["c"][1] + b * math.sin(t * math.pi / 24)) for t in range(48)]
    return rrect_pts(o["c"], o["w"] + 2 * grow, o["h"] + 2 * grow, max(0.05, sum(o["r"]) / 2 + grow))

def circle_pts(c, d):
    return [(c[0] + d / 2 * math.cos(t * math.pi / 24), c[1] + d / 2 * math.sin(t * math.pi / 24)) for t in range(48)]

def circle(sk, c, d):
    sk.sketchCurves.sketchCircles.addByCenterRadius(P(*c), cm(d / 2))

def profiles(sk, loops=None, test=None):
    import adsk.core
    sk.isComputeDeferred = False
    col = adsk.core.ObjectCollection.create()
    for p in sk.profiles:
        if loops is not None and p.profileLoops.count != loops:
            continue
        if test:
            c = p.areaProperties().centroid
            if not test((c.x * 10, c.y * 10)):
                continue
        col.add(p)
    if col.count == 0:
        raise RuntimeError("sketch %s formed no profiles" % sk.name)
    return col

def smallest_profiles(sk, n):
    import adsk.core
    sk.isComputeDeferred = False
    ps = sorted(sk.profiles, key=lambda p: p.areaProperties().area)[:n]
    col = adsk.core.ObjectCollection.create()
    for p in ps:
        col.add(p)
    return col

def extrude(comp, prof, z0, h, op="new", bodies=None, name=None, taper=0.0):
    import adsk.core, adsk.fusion
    log("extrude %s %s z0=%.2f h=%.2f" % (comp.name, op, z0, h))
    ops = {"new": adsk.fusion.FeatureOperations.NewBodyFeatureOperation,
           "cut": adsk.fusion.FeatureOperations.CutFeatureOperation,
           "join": adsk.fusion.FeatureOperations.JoinFeatureOperation}
    ex = comp.features.extrudeFeatures
    inp = ex.createInput(prof, ops[op])
    inp.startExtent = adsk.fusion.OffsetStartDefinition.create(adsk.core.ValueInput.createByReal(cm(z0)))
    if taper:
        inp.setOneSideExtent(adsk.fusion.DistanceExtentDefinition.create(adsk.core.ValueInput.createByReal(cm(h))),
                             adsk.fusion.ExtentDirections.PositiveExtentDirection,
                             adsk.core.ValueInput.createByString("%g deg" % taper))
    else:
        inp.setDistanceExtent(False, adsk.core.ValueInput.createByReal(cm(h)))
    if bodies:
        inp.participantBodies = list(bodies)
    f = ex.add(inp)
    if name and op == "new":
        for i in range(f.bodies.count):
            f.bodies.item(i).name = name if f.bodies.count == 1 else "%s %d" % (name, i + 1)
    return f

def edge_loop_at(body, z, outer=True):
    import adsk.core
    col = adsk.core.ObjectCollection.create()
    for f in body.faces:
        g = f.geometry
        if isinstance(g, adsk.core.Plane) and abs(abs(g.normal.z) - 1) < 1e-6 \
                and abs(f.pointOnFace.z - cm(z)) < 1e-5:
            for lp in f.loops:
                if lp.isOuter == outer:
                    for e in lp.edges:
                        col.add(e)
    return col

def fillet(comp, edges, r):
    import adsk.core
    log("fillet %s %d edges r=%.2f" % (comp.name, edges.count, r))
    fi = comp.features.filletFeatures.createInput()
    v = adsk.core.ValueInput.createByReal(cm(r))
    try:
        fi.edgeSetInputs.addConstantRadiusEdgeSet(edges, v, True)
    except AttributeError:
        fi.addConstantRadiusEdgeSet(edges, v, True)
    return comp.features.filletFeatures.add(fi)

def appearance(body, name):
    try:
        import adsk.core
        lib = app.materialLibraries.itemByName("Fusion Appearance Library")
        if name in CUSTOM_LOOKS:
            base, rgb, props = CUSTOM_LOOKS[name]
            local = design.appearances.itemByName(name)
            if not local:
                local = design.appearances.addByCopy(lib.appearances.itemByName(base), name)
                for pid in props:
                    p = local.appearanceProperties.itemById(pid)
                    if p:
                        p.value = adsk.core.Color.create(rgb[0], rgb[1], rgb[2], 255)
            body.appearance = local
            return name
        a = lib.appearances.itemByName(name)
        local = design.appearances.itemByName(name) or design.appearances.addByCopy(a, name)
        body.appearance = local
        return name
    except Exception as e:
        return "failed %s" % e

def inside(p, Pl):
    x, y, c = p[0], p[1], False
    for i in range(len(Pl)):
        (x1, y1), (x2, y2) = Pl[i - 1], Pl[i]
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            c = not c
    return c

def rib_rect(a, b, t):
    u = _unit((b[0] - a[0], b[1] - a[1])); n = (-u[1] * t / 2, u[0] * t / 2)
    return [(a[0] + n[0], a[1] + n[1]), (b[0] + n[0], b[1] + n[1]), (b[0] - n[0], b[1] - n[1]), (a[0] - n[0], a[1] - n[1])]

def ramp_tool(comp, strips, d_name):
    """New body: the side strips from SEAM_Z up to the ramped split line (SIDE_RAMP_Y), i.e. the part of the
    long sides that is navy. Returns the body (caller combines it)."""
    import adsk.core
    sk = new_sketch(comp, d_name)
    for q in strips:
        poly(sk, q)
    f = extrude(comp, profiles(sk), SEAM_Z - 0.01, Z_TRAY_TOP - SEAM_Z + 0.01, name=d_name)
    bodies = [b for b in comp.bRepBodies if b.name.startswith(d_name)]   # also a caller's body named d_name + "..."
    sk = comp.sketches.add(comp.yZConstructionPlane)
    sk.name = d_name + " ramp"
    m2s = lambda y, z: sk.modelToSketchSpace(P(0, y, z))
    y0, y1 = SIDE_RAMP_Y
    pts = [m2s(y0 + 20, SEAM_Z - 0.5), m2s(y0, SEAM_Z - 0.5), m2s(y1, Z_TRAY_TOP), m2s(-120, Z_TRAY_TOP),
           m2s(-120, Z_TRAY_TOP + 5), m2s(y0 + 20, Z_TRAY_TOP + 5)]
    for i in range(len(pts)):
        sk.sketchCurves.sketchLines.addByTwoPoints(pts[i], pts[(i + 1) % len(pts)])
    extrude(comp, profiles(sk), -60, 120, "cut", bodies)
    return [b for b in comp.bRepBodies if b.name.startswith(d_name)]

def combine(comp, target, tools, op):
    import adsk.core, adsk.fusion
    col = adsk.core.ObjectCollection.create()
    for t in tools:
        col.add(t)
    ci = comp.features.combineFeatures.createInput(target, col)
    ci.operation = {"cut": adsk.fusion.FeatureOperations.CutFeatureOperation,
                    "join": adsk.fusion.FeatureOperations.JoinFeatureOperation}[op]
    ci.isKeepToolBodies = False
    return comp.features.combineFeatures.add(ci)

def cut_groove(comp, body):
    """Slide-case rail groove along both long sides (GROOVE_D deep from the outline, GROOVE_H tall)."""
    if not GROOVE_ON:
        return
    sk = new_sketch(comp, "case rail groove")
    xg = W / 2 - GROOVE_D
    poly(sk, [(xg, -100), (60, -100), (60, 100), (xg, 100)])
    poly(sk, [(-60, -100), (-xg, -100), (-xg, 100), (-60, 100)])
    extrude(comp, profiles(sk), GROOVE_ZC - GROOVE_H / 2, GROOVE_H, "cut", [body])

def build_slide_case(comp, g, pose="stored", slide=None, name="Slide case"):
    """The navy slide-on hard case as one body. pose 'stored' = over the front, 'in_use' = on the back
    (flipped about the long axis). slide = mm pulled off towards -Y (default CASE_SLIDE)."""
    import adsk.core
    slide = CASE_SLIDE if slide is None else slide
    z_in = max(T_TOP, T_KEY + KEY_PROUD) + CASE_KEY_CLR          # floor inside face, stored pose
    z_lip0, z_lip1 = GROOVE_ZC - GROOVE_H / 2 + 0.05, GROOVE_ZC + GROOVE_H / 2 - 0.05
    sk = new_sketch(comp, "case block")
    poly(sk, g["case_out"])
    extrude(comp, profiles(sk), z_lip0, z_in + CASE_FLOOR - z_lip0, name=name)
    cb = body_named(comp, name)
    try:
        fillet(comp, edge_loop_at(cb, z_in + CASE_FLOOR), 1.0)
    except Exception as e:
        log("case fillet skipped: %s" % str(e)[:80])
    sk = new_sketch(comp, "case pocket")                 # inside, open at the top end
    yt = g["case_ytop"]
    poly(sk, [(x, y + 1.0 if y >= yt - 1e-6 else y) for x, y in g["case_in"]])
    extrude(comp, profiles(sk), z_lip0 - 0.1, z_in - z_lip0 + 0.1, "cut", [cb])
    sk = new_sketch(comp, "case lips")
    a, xl, y0, y1 = W / 2 + CASE_CLR + 0.01, W / 2 - GROOVE_D + CASE_LIP_CLR, CASE_Y_STRAIGHT, g["case_ytop"]
    poly(sk, [(xl, y0), (a, y0), (a, y1), (xl, y1)]); poly(sk, [(-a, y0), (-xl, y0), (-xl, y1), (-a, y1)])
    extrude(comp, profiles(sk), z_lip0, z_lip1 - z_lip0, "join", [cb])
    m = adsk.core.Matrix3D.create()
    if pose == "in_use":                                   # 180 deg about the long axis through Z = zm
        zm = (z_in + (-FOOT_PROUD)) / 2                    # floor inside face lands on the feet
        m.setWithArray([-1, 0, 0, 0, 0, 1, 0, cm(-slide), 0, 0, -1, cm(2 * zm), 0, 0, 0, 1])
    else:
        m.translation = adsk.core.Vector3D.create(0, cm(-slide), 0)
    if pose == "in_use" or slide:
        col = adsk.core.ObjectCollection.create(); col.add(cb)
        mi = comp.features.moveFeatures.createInput2(col)
        mi.defineAsFreeMove(m)
        comp.features.moveFeatures.add(mi)
    return cb

# â”€â”€ Stages â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
def stage_new(g):
    import adsk.core, adsk.fusion
    global design
    for d in list(app.documents):                     # close earlier replica builds only
        p = adsk.fusion.Design.cast(d.products.itemByProductType("DesignProductType"))
        if p and any(o.component.name == ROOT_TAG for o in p.rootComponent.occurrences):
            d.close(False)
    app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    design = adsk.fusion.Design.cast(app.activeProduct)
    design.designType = adsk.fusion.DesignTypes.ParametricDesignType
    comp_named("Front shell", True)
    print("new document")

def stage_front(g):
    fr = comp_named("Front shell")
    sk = new_sketch(fr, "outline")
    spline(sk, g["Vf_s"])
    extrude(fr, profiles(sk), SEAM_Z, T_TOP - SEAM_Z, name="Front shell")
    fb = body_named(fr, "Front shell")
    fillet(fr, edge_loop_at(fb, T_TOP), EDGE_R_FRONT)
    # D10: the keypad section is 0.5 lower than the top part; a ramp between REPLAY and the bezel (guess)
    sk = fr.sketches.add(fr.yZConstructionPlane)
    sk.name = "keypad step"
    m2s = lambda y, z: sk.modelToSketchSpace(P(0, y, z))
    pts = [m2s(-100, T_KEY), m2s(Y_RAMP[0], T_KEY), m2s(Y_RAMP[1], T_TOP), m2s(Y_RAMP[1], T_TOP + 5), m2s(-100, T_TOP + 5)]
    for i in range(len(pts)):
        sk.sketchCurves.sketchLines.addByTwoPoints(pts[i], pts[(i + 1) % len(pts)])
    extrude(fr, profiles(sk), -60, 120, "cut", [fb])
    import adsk.core
    # the cut leaves a crease where it meets the round-over: soften it (cosmetic, skipped if Fusion refuses)
    cut_faces = [f for f in fb.faces if isinstance(f.geometry, adsk.core.Plane) and 0.9 < f.geometry.normal.z
                 and f.pointOnFace.z < cm(T_TOP - 0.01)]
    edges = adsk.core.ObjectCollection.create()
    for f in cut_faces:
        for e in f.edges:
            if not all(any(o == t for t in cut_faces) for o in e.faces):
                edges.add(e)
    try:
        fillet(fr, edges, 0.6)
    except Exception as ex:
        log("crease fillet skipped: %s" % str(ex)[:80])
    sk = new_sketch(fr, "cavity")
    poly(sk, g["C"])
    extrude(fr, profiles(sk), SEAM_Z - 0.1, Z_PLATE - SEAM_Z + 0.1, "cut", [fb])
    sk = new_sketch(fr, "cavity screen section")       # the plate is the same thickness, so it steps up too
    poly(sk, g["C_top"])
    extrude(fr, profiles(sk), Z_PLATE - 0.01, Z_PLATE_TOP - Z_PLATE + 0.01, "cut", [fb])

    # row ribs, beads, crush ribs and collars under the plate. Rev D: built BEFORE the posts (the collars' inside cut
    # then never nicks a post) and the collars as "outer shape joined, then the inner shape cut": the old ring
    # profiles were filtered out by their centroid (it lies in the hole), so only the merged webs had been built.
    zc0 = Z_PLATE - COLLAR[2]
    sk = new_sketch(fr, "row ribs")
    rects = [rib_rect(a, b, ROW_RIB_T) for a, b in g["row_ribs"]]
    for r in rects:
        poly(sk, r)
    extrude(fr, profiles(sk, test=lambda p: any(inside(p, r) for r in rects)), zc0, COLLAR[2] + 0.01, "join", [fb])
    sk = new_sketch(fr, "row rib beads")
    for c in g["beads"]:
        circle(sk, c, BEAD_D)
    extrude(fr, profiles(sk), zc0, COLLAR[2] + 0.01, "join", [fb])
    sk = new_sketch(fr, "crush ribs")                   # star ribs round the locating posts
    for q in g["crush"]:
        poly(sk, q)
    extrude(fr, profiles(sk, test=lambda p: any(inside(p, q) for q in g["crush"])), Z_PLATE - CRUSH[3], CRUSH[3] + 0.01, "join", [fb])
    sk = new_sketch(fr, "collars")
    for o in g["openings"]:
        key_shape(sk, o, COLLAR[1])
    extrude(fr, profiles(sk), zc0, COLLAR[2] + 0.01, "join", [fb])
    sk = new_sketch(fr, "replay collar")                # lower: the pad's 4 direction domes sit under its edge
    circle(sk, g["replay"], REPLAY_D + 2 * COLLAR[1])
    hr = Z_PLATE - Z_DOME_TOP - 0.05
    extrude(fr, profiles(sk), Z_PLATE - hr, hr + 0.01, "join", [fb])
    sk = new_sketch(fr, "collars inside")               # the cap flanges sit in this step under the plate
    for o in g["openings"]:
        key_shape(sk, o, COLLAR[0])
    circle(sk, g["replay"], REPLAY_D + 2 * COLLAR[0])
    extrude(fr, profiles(sk), zc0 - 0.05, COLLAR[2] + 0.05, "cut", [fb])

    # posts first, so the key openings nick them where the real ones sit in the webs
    sk = new_sketch(fr, "screw posts")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_POST_D)
    extrude(fr, profiles(sk), Z_MEET, Z_PLATE_TOP - Z_MEET + 0.01, "join", [fb])
    sk = new_sketch(fr, "pilot holes")
    for n, c in g["screws"]:
        circle(sk, c, PILOT_D)
    extrude(fr, smallest_profiles(sk, len(g["screws"])), Z_MEET, Z_PLATE - Z_MEET - 0.5, "cut", [fb])
    sk = new_sketch(fr, "locating posts")
    for n, c in g["locs"]:
        circle(sk, c, LOC_POST_D)
    extrude(fr, profiles(sk), Z_LOC_END, Z_PLATE_TOP - Z_LOC_END + 0.01, "join", [fb])
    sk = new_sketch(fr, "locating post bores")
    for n, c in g["locs"]:
        circle(sk, c, LOC_BORE_D)
    extrude(fr, smallest_profiles(sk, len(g["locs"])), Z_LOC_END - 0.1, LOC_BORE_DEPTH + 0.1, "cut", [fb])

    # screen section: inner rib (wire channel behind it), 3 stubs + 1 wire holder per side (C14 / C4)
    sk = new_sketch(fr, "inner ribs")
    for q in g["inner_ribs"]:
        poly(sk, q)
    z0 = INNER_RIB_Z0 if INNER_RIB_FULL else STUB_Z0  # rev G: rib top edge 4.5 below the rim (rev F had it from the rim)
    extrude(fr, profiles(sk), z0, Z_PLATE_TOP - z0 + 0.01, "join", [fb])
    # rev G: each pin starts at its measured depth (outer 5.1, middle 6.0 below the rim), hooks + C4 holder at 5.5
    groups = {}
    for q in g["pins"]:
        yc = sum(p[1] for p in q) / len(q)
        zq = HOOK_Z0 if abs(yc - HOLDER_Y) < 1.0 else PIN_Z0[min(STUB_Y, key=lambda y: abs(y - yc))]
        groups.setdefault(round(zq, 2), []).append(q)
    for zq, qs in sorted(groups.items()):
        sk = new_sketch(fr, "wall stubs Z%.1f" % zq)
        for q in qs:
            poly(sk, q)
        extrude(fr, profiles(sk), zq, Z_PLATE_TOP - zq + 0.01, "join", [fb])
    z0 = HOOK_Z0
    sk = new_sketch(fr, "wire hooks")                  # rev D: J-hooks (3 per side incl. the C4 holder)
    for q in g["hooks"]:
        poly(sk, q)
    extrude(fr, profiles(sk), z0, Z_PLATE_TOP - z0 + 0.01, "join", [fb])
    # LR44 cup (D7 rib height) and the rib frame round the solar window
    sk = new_sketch(fr, "coin cup")
    circle(sk, DOOR_C, COIN_CUP[0]); circle(sk, DOOR_C, COIN_CUP[0] + 2 * COIN_CUP[1])
    extrude(fr, profiles(sk, 2), Z_PLATE_TOP - COIN_CUP[2], COIN_CUP[2] + 0.01, "join", [fb])
    # rev D: the ring is open left and right (clip slots), I-bars + lugs either side, contact rib across the floor
    z_cup = Z_PLATE_TOP - COIN_CUP[2]
    sk = new_sketch(fr, "coin cup side gaps")
    rr = COIN_CUP[0] / 2 + COIN_CUP[1] + 0.2
    poly(sk, _rect((DOOR_C[0] - rr + 1.0, DOOR_C[1]), 2.0, COIN_GAP)); poly(sk, _rect((DOOR_C[0] + rr - 1.0, DOOR_C[1]), 2.0, COIN_GAP))
    extrude(fr, profiles(sk), z_cup - 0.1, COIN_CUP[2] - 0.01, "cut", [fb])
    sk = new_sketch(fr, "coin holder bars")
    for q in g["coin"]:
        poly(sk, q)
    extrude(fr, profiles(sk, test=lambda p: any(inside(p, q) for q in g["coin"])), z_cup, COIN_CUP[2] + 0.01, "join", [fb])
    sk = new_sketch(fr, "coin contact rib")
    poly(sk, _rect((DOOR_C[0], DOOR_C[1] + COIN_CONTACT[2]), COIN_CUP[0] + 0.4, COIN_CONTACT[0]))
    extrude(fr, profiles(sk), Z_PLATE_TOP - COIN_CONTACT[1], COIN_CONTACT[1] + 0.01, "join", [fb])
    sk = new_sketch(fr, "solar frame")
    rrect(sk, SOLAR_C, SOLAR[0] + 0.4, SOLAR[1] + 0.4, 0.8)
    rrect(sk, SOLAR_C, SOLAR[0] + 0.4 + 2 * SOLAR_FRAME[0], SOLAR[1] + 0.4 + 2 * SOLAR_FRAME[0], 0.8 + SOLAR_FRAME[0])
    extrude(fr, profiles(sk, 2), Z_PLATE_TOP - SOLAR_FRAME[1], SOLAR_FRAME[1] + 0.01, "join", [fb])


    # display window in a bevelled recess (lens sits in it), solar-cell window
    sk = new_sketch(fr, "lens recess")
    rrect(sk, g["win_c"], WIN[0] + 2 * BEZEL_MARGIN, WIN[1] + 2 * BEZEL_MARGIN, WIN_R + BEZEL_MARGIN)
    extrude(fr, profiles(sk), T_BODY - BEZEL_DEPTH, BEZEL_DEPTH + 0.01, "cut", [fb], taper=45)
    sk = new_sketch(fr, "window")
    rrect(sk, g["win_c"], WIN[0], WIN[1], WIN_R)
    extrude(fr, profiles(sk), Z_PLATE - 1, FACE_T + 2, "cut", [fb])
    sk = new_sketch(fr, "solar window")               # through-opening, cell behind it (photo 921b4503)
    rrect(sk, SOLAR_C, SOLAR[0], SOLAR[1], 0.8)
    extrude(fr, profiles(sk), Z_PLATE_TOP - 0.5, FACE_T + 1, "cut", [fb])

    # key openings with a thin bevel on top
    sk = new_sketch(fr, "key openings")
    for o in g["openings"]:
        key_shape(sk, o, 0.0)
    circle(sk, g["replay"], REPLAY_D)
    prof = profiles(sk)
    extrude(fr, prof, Z_PLATE - 1, FACE_T + 2, "cut", [fb])
    extrude(fr, profiles(sk), T_KEY - KEY_BEVEL[0], KEY_BEVEL[0] + 0.01, "cut", [fb], taper=KEY_BEVEL[1])
    # rev C: where the navy side wall ramps up, the silver skin is cut back (wall + gap) below the split line
    combine(fr, fb, ramp_tool(fr, g["ramp_cut"], "side ramp cut"), "cut")
    # rev C2: keypad section + bottom end = faceplate only: the silver skin stops at Z_SKIRT inside the navy wall
    sk = new_sketch(fr, "faceplate skirt cut")
    poly(sk, g["skirt_cut"])
    extrude(fr, profiles(sk), SEAM_Z - 0.1, Z_SKIRT - SEAM_Z + 0.1, "cut", [fb])
    # rev D: comb snap tabs, top (into a notch in the back cover's lip) and bottom (inside the navy end wall)
    for end, z0_, z1_ in (("top", SNAP_TOP[1], Z_PLATE_TOP + 0.01), ("bottom", SNAP_BOT[2], Z_SKIRT + 0.3)):
        sk = new_sketch(fr, "snap tab %s" % end)
        for e, q in g["snap"]["teeth"]:
            if e == end:
                poly(sk, q)
        extrude(fr, profiles(sk), z0_, z1_ - z0_, "join", [fb])
    cut_groove(fr, fb)
    print("front ok, faces", fb.faces.count)

def stage_back(g):
    import adsk.core
    bk = comp_named("Back cover", True)
    sk = new_sketch(bk, "outline")
    spline(sk, g["Vb_s"])
    extrude(bk, profiles(sk), 0, SEAM_Z, name="Back cover")
    bb = body_named(bk, "Back cover")
    fillet(bk, edge_loop_at(bb, 0), EDGE_R_BACK)
    sk = new_sketch(bk, "cavity")                     # inside of the lip, so the lip grows out of the rim
    poly(sk, g["lip_i"])
    extrude(bk, profiles(sk), BACK_PLATE, SEAM_Z, "cut", [bb])
    sk = new_sketch(bk, "lip")
    poly(sk, g["lip_o"]); poly(sk, g["lip_i"])
    extrude(bk, profiles(sk, 2), SEAM_Z - 0.01, LIP_H + 0.01, "join", [bb])
    # tray walls: where the front shell is smaller than the back (keypad section, bottom end)
    sk = new_sketch(bk, "tray walls")
    poly(sk, g["tray_band"])                           # drawn 0.3 proud, then trimmed back to the outline spline
    extrude(bk, profiles(sk), SEAM_Z - 0.01, Z_TRAY_TOP - SEAM_Z + 0.01, name="side ramp walls (tray)")
    # rev C: the side walls rise along a ramp (SIDE_RAMP_Y) instead of starting at full height (tray band too)
    combine(bk, bb, ramp_tool(bk, g["ramp_navy"], "side ramp walls"), "join")
    sk = new_sketch(bk, "tray trim")
    spline(sk, g["Vb_s"]); poly(sk, [(-60, -100), (60, -100), (60, 100), (-60, 100)])
    extrude(bk, profiles(sk, 2), SEAM_Z - 0.005, Z_TRAY_TOP - SEAM_Z + 1, "cut", [bb])
    sk = new_sketch(bk, "wall ticks")                 # pairs of short ribs on the inside of the long walls
    for q in g["ticks"]:
        poly(sk, q)
    extrude(bk, profiles(sk), BACK_PLATE - 0.01, SEAM_Z + LIP_H - BACK_PLATE + 0.01, "join", [bb])

    sk = new_sketch(bk, "bosses")
    for n, c in g["screws"]:
        circle(sk, c, BOSS_D)
    extrude(bk, profiles(sk), BACK_PLATE - 0.01, Z_MEET - 0.1 - BACK_PLATE + 0.01, "join", [bb])

    # ribs and rings (positions photo 2f54d6c7, heights D13: grid 1 / rings 4 / lower ribs 1)
    for h in sorted({r[3] for r in g["ribs"]}):
        sk = new_sketch(bk, "ribs %.1f high" % h)
        rects = [rib_rect(a, b, RIB_T) for n, a, b, hh in g["ribs"] if hh == h]
        for r in rects:
            poly(sk, r)
        extrude(bk, profiles(sk, test=lambda p: any(inside(p, r) for r in rects)), BACK_PLATE - 0.01,
                h + 0.01, "join", [bb])
    for n, c, r in g["rings"]:
        sk = new_sketch(bk, n)
        circle(sk, c, 2 * r); circle(sk, c, 2 * r - 2 * RING_T)
        extrude(bk, profiles(sk, 2), BACK_PLATE - 0.01, RING_H + 0.01, "join", [bb])
    for nm, segs, h in (("solar box grid", g["grid"], SOLAR_GRID_H), ("solar box frame", g["box"], SOLAR_BOX_H)):
        sk = new_sketch(bk, nm)                       # D13 grid 5, D6 frame 5-6; photo 050df9b7: 4 x 2 cells
        rects = [rib_rect(a, b, RIB_T) for a, b in segs]
        for r in rects:
            poly(sk, r)
        extrude(bk, profiles(sk, test=lambda p: any(inside(p, r) for r in rects)), BACK_PLATE - 0.01,
                h + 0.01, "join", [bb])

    # screw holes with counterbores from outside
    sk = new_sketch(bk, "screw holes")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_D)
    extrude(bk, smallest_profiles(sk, 6), -1, Z_MEET + 1, "cut", [bb])
    sk = new_sketch(bk, "counterbores")
    for n, c in g["screws"]:
        circle(sk, c, CBORE_D)
    extrude(bk, smallest_profiles(sk, 6), -1, CBORE_H + 1, "cut", [bb])

    # battery: shallow recess for the lid + round LR44 hole with a key notch; centre hole over the small ring
    sk = new_sketch(bk, "battery recess")
    rrect(sk, DOOR_C, DOOR[0], DOOR[1], DOOR_R)
    extrude(bk, profiles(sk), -1, DOOR_DEPTH + 1, "cut", [bb])
    sk = new_sketch(bk, "battery hole")
    circle(sk, DOOR_C, BATT_HOLE_D)
    rrect(sk, (DOOR_C[0], DOOR_C[1] + BATT_HOLE_D / 2), 3.0, 2.4, 0.4)
    extrude(bk, profiles(sk), -1, BACK_PLATE + 1.01, "cut", [bb])
    small = [c for n, c, r in g["rings"] if n.startswith("small")][0]
    sk = new_sketch(bk, "centre hole")
    circle(sk, small, CENTRE_HOLE_D)
    extrude(bk, profiles(sk), -1, BACK_PLATE + 1.01, "cut", [bb])
    sk = new_sketch(bk, "feet pockets")
    for c in FEET:
        circle(sk, c, FOOT_D + 0.2)
    extrude(bk, smallest_profiles(sk, 4), -1, FOOT_POCKET + 1, "cut", [bb])
    # rev D: notch in the lip for the top snap teeth, catch block for the bottom ones (photo 2f54d6c7)
    sk = new_sketch(bk, "snap notch top")
    poly(sk, g["snap"]["pocket"])
    extrude(bk, profiles(sk), SNAP_POCKET[1], SNAP_POCKET[2] - SNAP_POCKET[1], "cut", [bb])
    sk = new_sketch(bk, "snap catch bottom")
    poly(sk, g["snap"]["catch"])
    extrude(bk, profiles(sk), BACK_PLATE - 0.01, SNAP_CATCH[2] - BACK_PLATE + 0.01, "join", [bb])
    cut_groove(bk, bb)
    print("back ok, faces", bb.faces.count)

def stage_keys(g):
    km = comp_named("Keymat", True)
    sk = new_sketch(km, "sheet")                       # board-sized rectangle (photos 28bb9c4b, dc7abe75)
    rrect(sk, g["board_c"], BOARD[0] - 0.4, BOARD[1] - 0.4, 0.5)
    extrude(km, profiles(sk), Z_BOARD_F + 0.02, MAT_T, name="Keymat (rubber)")   # sheet lies on the board
    kb = body_named(km, "Keymat (rubber)")
    sk = new_sketch(km, "clip")
    poly(sk, g["board_clip"]); poly(sk, [(-60, -100), (60, -100), (60, 100), (-60, 100)])
    extrude(km, profiles(sk, 2), Z_BOARD_F - 0.1, MAT_T + 0.3, "cut", [kb])
    sk = new_sketch(km, "domes")                       # one dome per key, rubber top D9 = 1.5 above the contact
    for n, c, w, h in g["keys"]:
        circle(sk, c, min(w, h) - 0.8)
    extrude(km, profiles(sk), Z_BOARD_F + 0.02 + MAT_T - 0.01, Z_DOME_TOP - (Z_BOARD_F + 0.02 + MAT_T - 0.01), "join", [kb])
    sk = new_sketch(km, "post holes")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_POST_D + 0.6)
    for n, c in g["locs"]:
        circle(sk, c, LOC_POST_D + 0.6)
    extrude(km, profiles(sk, test=lambda p: any(math.dist(p, c) < 2.5 for n, c in g["screws"] + g["locs"])),
            Z_BOARD_F, Z_DOME_TOP - Z_BOARD_F + 0.1, "cut", [kb])

    kc = comp_named("Keycaps", True)
    caps = [(o["name"], o["c"]) for o in g["openings"]]
    rd = REPLAY_D - 2 * CAP_CLR
    sk = new_sketch(kc, "flanges")
    for o in g["openings"]:
        key_shape(sk, o, CAP_FLANGE - CAP_CLR)
    circle(sk, g["replay"], rd + 2 * CAP_FLANGE)
    f = extrude(kc, profiles(sk), Z_CAP_BOT, FLANGE_T, name="cap")
    bodies = [f.bodies.item(i) for i in range(f.bodies.count)]
    sk = new_sketch(kc, "shafts")
    for o in g["openings"]:
        key_shape(sk, o, -CAP_CLR)
    circle(sk, g["replay"], rd)
    prof = profiles(sk)
    extrude(kc, prof, Z_CAP_BOT + FLANGE_T - 0.01, T_KEY - Z_CAP_BOT - FLANGE_T + 0.01, "join", bodies)
    # crowns: tapered tops (a taper is much cheaper than filleting 47 loops)
    sk2 = new_sketch(kc, "crowns")
    for o in g["openings"]:
        key_shape(sk2, o, -CAP_CLR)
    extrude(kc, profiles(sk2), T_KEY, KEY_PROUD, "join", list(kc.bRepBodies), taper=-CROWN_TAPER)
    # REPLAY sits a little lower than the keys: trim its crown
    sk3 = new_sketch(kc, "replay trim")
    circle(sk3, g["replay"], rd + 1)
    extrude(kc, profiles(sk3), T_KEY + REPLAY_PROUD, 3, "cut", list(kc.bRepBodies))
    # flanges clear the posts that sit in the webs between keys
    sk4 = new_sketch(kc, "post clearance")
    for n, c in g["screws"]:
        circle(sk4, c, SCREW_POST_D + 0.6)
    for n, c in g["locs"]:
        circle(sk4, c, LOC_POST_D + 0.6)
    extrude(kc, profiles(sk4, test=lambda p: any(math.dist(p, c) < 2.5 for n, c in g["screws"] + g["locs"])),
            Z_CAP_BOT - 0.1, FLANGE_T + 0.2, "cut", list(kc.bRepBodies))
    # name each cap after its key (nearest centre)
    names = caps + [("REPLAY", g["replay"])]
    for b in kc.bRepBodies:
        bb = b.boundingBox
        mid = ((bb.minPoint.x + bb.maxPoint.x) * 5, (bb.minPoint.y + bb.maxPoint.y) * 5)
        b.name = "Key " + min(names, key=lambda t: math.dist(t[1], mid))[0]
    print("keys ok: mat + %d caps" % kc.bRepBodies.count)

def stage_parts(g):
    ln = comp_named("Window lens", True)
    sk = new_sketch(ln, "lens")
    rrect(sk, g["win_c"], WIN[0] + 2 * BEZEL_MARGIN - 0.3, WIN[1] + 2 * BEZEL_MARGIN - 0.3, WIN_R + BEZEL_MARGIN - 0.15)
    extrude(ln, profiles(sk), T_BODY - BEZEL_DEPTH + 0.02, LENS_T, name="Window lens (clear)")

    so = comp_named("Solar cell (dummy)", True)
    sk = new_sketch(so, "cell")
    rrect(sk, SOLAR_C, SOLAR[0] + 0.2, SOLAR[1] + 0.2, 0.8)     # behind the through-window, inside its frame
    extrude(so, profiles(sk), Z_PLATE_TOP - 0.6, 0.55, name="Solar cell (dummy)")

    dr = comp_named("Battery lid", True)              # guess: thin lid in the recess over the LR44 hole
    sk = new_sketch(dr, "lid")
    rrect(sk, DOOR_C, DOOR[0] - 2 * DOOR_CLR, DOOR[1] - 2 * DOOR_CLR, DOOR_R - DOOR_CLR)
    extrude(dr, profiles(sk), 0.0, DOOR_DEPTH - 0.02, name="Battery lid")
    db = body_named(dr, "Battery lid")
    sk = new_sketch(dr, "grip")
    for k in range(-2, 3):
        rrect(sk, (DOOR_C[0] + k * 1.8, DOOR_C[1] - 3.5), 0.8, 4.0, 0.3)
    extrude(dr, profiles(sk), -0.1, 0.35, "cut", [db])

    ft = comp_named("Rubber feet", True)
    sk = new_sketch(ft, "feet")
    for c in FEET:
        circle(sk, c, FOOT_D)
    extrude(ft, profiles(sk), -FOOT_PROUD, FOOT_PROUD + FOOT_POCKET - 0.02, name="Foot")

    rf = comp_named("Reference - Casio board (C6)", True)
    sk = new_sketch(rf, "board")
    rrect(sk, g["board_c"], BOARD[0], BOARD[1], 0.5)
    extrude(rf, profiles(sk), Z_BOARD_B, BOARD_T, name="Casio board (reference)")
    pb = body_named(rf, "Casio board (reference)")
    sk = new_sketch(rf, "corner clip")                 # real board corners follow the rounded case ends
    poly(sk, g["board_clip"]); poly(sk, [(-60, -100), (60, -100), (60, 100), (-60, 100)])
    extrude(rf, profiles(sk, 2), Z_BOARD_B - 0.1, BOARD_T + 0.2, "cut", [pb])
    sk = new_sketch(rf, "holes")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_POST_D + 0.4)
    for n, c in g["locs"]:
        circle(sk, c, LOC_POST_D + 0.4)
    extrude(rf, profiles(sk, test=lambda p: any(math.dist(p, c) < 2.4 for n, c in g["screws"] + g["locs"])),
            Z_BOARD_B - 0.1, BOARD_T + 0.2, "cut", [pb])

    lc = comp_named("Reference - LCD (dummy)", True)  # makes the window read right; size is a guess
    sk = new_sketch(lc, "lcd")
    rrect(sk, g["win_c"], LCD[0], LCD[1], 0.5)
    extrude(lc, profiles(sk), Z_PLATE_TOP - 0.05 - LCD_T, LCD_T, name="LCD (dummy)")

    cv = comp_named("Slide case (stored)", True)        # the separate navy hard case, over the front
    build_slide_case(cv, g, "stored", name="Slide case")
    stage_looks(g)
    print("parts ok")

# rev C: custom colours matched to the photos (the library "Paint - Metallic (Silver)" rendered cream)
SILVER, NAVY = "AIC Silver (front shell)", "AIC Navy (back shell, case)"
CUSTOM_LOOKS = {SILVER: ("Paint - Metallic (Silver)", (178, 184, 192), ("layered_diffuse", "surface_albedo", "layered_bottom_f0")),
                NAVY: ("Plastic - Matte (Blue)", (34, 40, 78), ("opaque_albedo",))}
LOOKS = {"Front shell": SILVER, "Back cover": NAVY,
         "Keymat": "Rubber - Soft", "Window lens": "Glass (Grey)", "Solar cell (dummy)": "Paint - Metallic (Dark Grey)",
         "Battery lid": NAVY, "Rubber feet": "Rubber - Hard",
         "Reference - Casio board (C6)": "Plastic - Matte (Green)",
         "Reference - LCD (dummy)": "Glass - Heavy Color", "Slide case (stored)": NAVY, "Slide case (in use)": NAVY}

def stage_looks(g):
    for cname, look in LOOKS.items():
        try:
            for b in comp_named(cname).bRepBodies:
                appearance(b, look)
        except KeyError:
            pass
    try:
        for b in comp_named("Keycaps").bRepBodies:
            n = b.name[4:]
            appearance(b, "Plastic - Matte (Gray)" if n == "REPLAY" else "Paint - Metallic (Dark Grey)")   # photo 4e287737
    except KeyError:
        pass

def stage_check(g):
    import adsk.core
    col = adsk.core.ObjectCollection.create()
    out = []
    for occ in design.rootComponent.occurrences:
        vol = 0
        for body in occ.bRepBodies:
            col.add(body)
            vol += body.physicalProperties.volume
        bb = occ.boundingBox
        out.append("%-36s %3d bodies %7.2f cm3  %5.1f x %5.1f x %5.1f mm" % (
            occ.component.name, occ.bRepBodies.count, vol, (bb.maxPoint.x - bb.minPoint.x) * 10,
            (bb.maxPoint.y - bb.minPoint.y) * 10, (bb.maxPoint.z - bb.minPoint.z) * 10))
    ii = design.createInterferenceInput(col)
    ii.areCoincidentFacesIncluded = False
    res = design.analyzeInterference(ii)
    out.append("bodies checked: %d" % col.count)
    out.append("interferences: %s" % (res.count if res is not None else "0 (Fusion returned no result set)"))
    for i in range(res.count if res is not None else 0):
        r = res.item(i)
        out.append("  %s  <->  %s : %.4f cm3" % (r.entityOne.name, r.entityTwo.name, r.interferenceBody.volume))
    # second opinion: pairwise boolean intersection of every pair whose boxes overlap
    tb = adsk.fusion.TemporaryBRepManager.get()
    bodies = [col.item(i) for i in range(col.count)]
    hits, undet, pairs = [], [], 0
    for i in range(len(bodies)):
        for j in range(i + 1, len(bodies)):
            a, b = bodies[i], bodies[j]
            if not a.boundingBox.intersects(b.boundingBox):
                continue
            pairs += 1
            v = None
            for dz in (0.0, 0.002, -0.002):           # faces that just touch confuse the kernel: nudge 0.02 mm
                try:
                    ta, tbb = tb.copy(a), tb.copy(b)
                    if dz:
                        m = adsk.core.Matrix3D.create(); m.translation = adsk.core.Vector3D.create(0, 0, dz)
                        tb.transform(tbb, m)
                    tb.booleanOperation(ta, tbb, adsk.fusion.BooleanTypes.IntersectionBooleanType)
                    vv = sum(ta.lumps.item(k).volume for k in range(ta.lumps.count)) if ta and ta.lumps.count else 0
                    v = vv if v is None else min(v, vv)
                    if v < 1e-6:
                        break
                except Exception:
                    pass
            if v is None:
                undet.append("  (undetermined: %s <-> %s)" % (a.name, b.name))
                continue
            if v > 1e-6:
                hits.append("  %s / %s  <->  %s / %s : %.4f cm3" % (a.parentComponent.name, a.name,
                            b.parentComponent.name, b.name, v))
    out.append("pairwise boolean check: %d pairs with overlapping boxes, %d real overlaps" % (pairs, len(hits)))
    out += hits + undet
    with open(os.path.join(OUT, "interference.txt"), "w", encoding="utf8") as f:
        f.write("\n".join(out) + "\n")
    for line in out:
        log("check | " + line)
    print("\n".join(out[-12:]))

def stage_export(g):
    import adsk.fusion
    root = design.rootComponent
    for occ in list(root.occurrences):                 # exports are the replica only: drop the fit-check parts
        if occ.component.name.startswith(FIT_PREFIX):
            nm = occ.name
            occ.deleteMe()
            log("export: removed %s (re-run stage fitcheck to bring it back)" % nm)
    em = design.exportManager
    d = OUT
    em.execute(em.createFusionArchiveExportOptions(os.path.join(d, "fx115es_replica.f3d"), root))
    em.execute(em.createSTEPExportOptions(os.path.join(d, "fx115es_replica.step"), root))
    sd = os.path.join(d, "stl")
    os.makedirs(sd, exist_ok=True)
    n = 0
    for occ in root.occurrences:
        cname = occ.component.name.replace("Replica - ", "")
        if cname.startswith("Reference"):
            continue
        if cname == "Keycaps":                         # 47 caps: one file for all + one per cap
            o = em.createSTLExportOptions(occ, os.path.join(sd, "keycaps_all.stl"))
            em.execute(o); n += 1
        for k, b in enumerate(occ.bRepBodies):
            safe = "".join(ch if ch.isalnum() else "_" for ch in (b.name if cname == "Keycaps" else cname + "_" + b.name))
            if cname == "Keycaps":
                safe = "keycaps/%02d_%s" % (k, safe)
                os.makedirs(os.path.join(sd, "keycaps"), exist_ok=True)
            o = em.createSTLExportOptions(b, os.path.join(sd, safe.strip("_") + ".stl"))
            o.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementMedium
            em.execute(o); n += 1
    print("exported f3d, step and %d stl" % n)

def set_visible(names, vis):
    for occ in design.rootComponent.occurrences:
        if occ.component.name.replace("Replica - ", "") in names:
            occ.isLightBulbOn = vis

def stage_shots(g):
    import adsk.core
    d = os.path.join(OUT, "renders")
    os.makedirs(d, exist_ok=True)
    vp = app.activeViewport
    views = ARGS.get("views", "front,back,side,top_end,iso_front,iso_back").split(",")
    V = adsk.core.ViewOrientations
    named = {"front": V.TopViewOrientation, "back": V.BottomViewOrientation,
             "iso_front": V.IsoTopRightViewOrientation, "iso_back": V.IsoBottomLeftViewOrientation,
             "side": V.RightViewOrientation, "top_end": V.BackViewOrientation, "iso_left": V.IsoTopLeftViewOrientation}
    for comp in design.allComponents:
        for skt in comp.sketches:
            skt.isLightBulbOn = False
    hide = [s for s in ARGS.get("hide", "Slide case (stored);Reference - Casio board (C6)").split(";") if s]
    set_visible(hide, False)
    try:
        for v in views:
            cam = vp.camera
            if v.startswith("top@"):                 # top@x;y;extent
                x, y, ext = map(float, v.split("@")[1].split(";"))
                cam.isFitView = False
                cam.target = P(x, y, T_BODY); cam.eye = P(x, y, T_BODY + 300)
                cam.upVector = adsk.core.Vector3D.create(0, 1, 0)
                cam.viewExtents = cm(ext)
            elif v.startswith("eye@"):               # eye@ex;ey;ez;tx;ty;tz;extent
                ex_, ey, ez, tx, ty, tz, ext = map(float, v.split("@")[1].split(";"))
                cam.isFitView = False
                cam.target = P(tx, ty, tz); cam.eye = P(ex_, ey, ez)
                cam.upVector = adsk.core.Vector3D.create(0, 0, 1)
                cam.viewExtents = cm(ext)
            else:
                cam.viewOrientation = named[v]
                cam.isFitView = True
            vp.camera = cam
            adsk.doEvents(); vp.refresh()
            fname = ARGS.get("prefix", "") + (v.split("@")[0] + ("_%d" % views.index(v) if "@" in v else ""))
            vp.saveAsImageFile(os.path.join(d, fname + ".png"), 1600, 1200)
    finally:
        set_visible(hide, True)
    print("shots:", views)

def stage_explode(g):
    import adsk.core
    gap = float(ARGS.get("gap", 14))
    order = {"Rubber feet": -2, "Battery lid": -1, "Back cover": 0, "Reference - Casio board (C6)": 1,
             "Reference - LCD (dummy)": 1,
             "Keymat": 2, "Keycaps": 3, "Front shell": 4, "Solar cell (dummy)": 5, "Window lens": 5, "Slide case (stored)": 6}
    for occ in design.rootComponent.occurrences:
        m = adsk.core.Matrix3D.create()
        m.translation = adsk.core.Vector3D.create(0, 0, cm(gap * order.get(occ.component.name.replace("Replica - ", ""), 0)))
        occ.transform = m
    print("explode", gap)

def stage_section(g):
    """Cut every body at Y = ARGS y (or X = ARGS x), take side shots with prefix section_, then undo."""
    y = ARGS.get("y"); x = ARGS.get("x")
    made = []
    for occ in design.rootComponent.occurrences:
        comp = occ.component
        if comp.bRepBodies.count == 0:
            continue
        sk = new_sketch(comp, "section")
        if y is not None:
            poly(sk, [(-100, float(y)), (100, float(y)), (100, float(y) + 200), (-100, float(y) + 200)])
        else:
            poly(sk, [(float(x), -100), (float(x) + 200, -100), (float(x) + 200, 100), (float(x), 100)])
        made += [extrude(comp, profiles(sk), -5, 30, "cut", list(comp.bRepBodies)), sk]
    ARGS["prefix"] = ARGS.get("prefix", "section_")
    try:
        stage_shots(g)
    finally:
        for f in reversed(made):
            f.deleteMe()

def stage_explode_shots(g):
    ARGS["gap"] = ARGS.get("gap", "14"); stage_explode(g)
    ARGS["views"] = "iso_front,iso_back"; ARGS["prefix"] = "exploded_"
    ARGS["hide"] = "Reference - Casio board (C6);" + FIT_PCB_COMP + ";" + FIT_PROXY_COMP
    stage_shots(g)
    ARGS["gap"] = "0"; stage_explode(g)

# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
# Fit check: our PCB (hardware/fab/ai_calc_board.step) in the replica
#   python build_fx115es_replica.py fitcheck          import + interference check + report
#   python build_fx115es_replica.py fitcheck_shots    renders with the board in place
# Re-run both whenever the PCB agent regenerates the STEP; nothing else needs rebuilding.
# The KiCad file is only read (footprint positions, holes, keep-outs, Edge.Cuts).
# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
FIT_PREFIX = "Fit-check - "
FIT_PCB_COMP = FIT_PREFIX + "PCB (ai_calc_board.step)"
FIT_PROXY_COMP = FIT_PREFIX + "missing parts (proxies)"

def _sexp_blocks(s, tag):
    """Every balanced '(tag ...)' block of a KiCad s-expression file (strings skipped)."""
    import re
    for m in re.finditer(r"\(" + tag + r"\s", s):
        i, d, j = m.start(), 0, m.start()
        while True:
            ch = s[j]
            if ch == '"':
                j += 1
                while s[j] != '"':
                    j += 2 if s[j] == "\\" else 1
            elif ch == "(":
                d += 1
            elif ch == ")":
                d -= 1
                if d == 0:
                    break
            j += 1
        yield s[i:j + 1]

def board_info():
    """What the fit check needs from the KiCad board and the STEP (pure Python, read only)."""
    import re
    global FIT_STEP, FIT_PCB
    if "ARGS" in globals():                           # step=... pcb=... to check an older board (e.g. from git)
        FIT_STEP, FIT_PCB = ARGS.get("step", FIT_STEP), ARGS.get("pcb", FIT_PCB)
    s = open(FIT_PCB, encoding="utf8").read()
    lay = lambda b: (re.search(r'\(layer "([^"]+)"', b) or [None, None])[1]
    num = lambda pat, b: tuple(map(float, re.search(pat, b).group(1).split()))
    # Edge.Cuts sampled every ~0.5 mm (outline, slot, notches)
    edge, segs = [], []
    for tag in ("gr_line", "gr_arc"):
        for b in _sexp_blocks(s, tag):
            if lay(b) != "Edge.Cuts":
                continue
            a, e = num(r"\(start ([-\d. ]+)\)", b), num(r"\(end ([-\d. ]+)\)", b)
            pts = [a, num(r"\(mid ([-\d. ]+)\)", b), e] if tag == "gr_arc" else [a, e]
            for p, q in zip(pts, pts[1:]):
                segs.append((p, q))
                k = max(1, int(math.dist(p, q) / 0.5))
                edge += [(p[0] + (q[0] - p[0]) * t / k, p[1] + (q[1] - p[1]) * t / k) for t in range(k + 1)]
    keepouts = []
    for b in _sexp_blocks(s, "gr_rect"):
        if lay(b) == "User.3":
            (x1, y1), (x2, y2) = num(r"\(start ([-\d. ]+)\)", b), num(r"\(end ([-\d. ]+)\)", b)
            keepouts.append((abs(x2 - x1), abs(y2 - y1), ((x1 + x2) / 2, (y1 + y2) / 2)))
    fps = []
    for b in _sexp_blocks(s, "footprint"):
        name = re.search(r'\(footprint "([^"]+)"', b).group(1)
        at = re.search(r'\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)', b).groups()
        x, y, rot = float(at[0]), float(at[1]), float(at[2] or 0)
        ref = (re.search(r'\(property "Reference" "([^"]+)"', b) or re.search(r'\(fp_text reference "([^"]+)"', b))
        ref = ref.group(1) if ref else "?"
        side = "B" if lay(b) == "B.Cu" else "F"
        models = [m.split("/")[-1].split("\\")[-1] for m in re.findall(r'\(model "([^"]+)"', b)]
        drills = [float(d) for d in re.findall(r'\(drill ([\d.]+)', b)]
        loc = []                                          # local outline points: Fab, else courtyard
        for want in ("Fab", "CrtYd"):
            for tag in ("fp_line", "fp_rect", "fp_poly", "fp_circle", "fp_arc"):
                for g in _sexp_blocks(b, tag):
                    if (lay(g) or "").endswith(want):
                        loc += [tuple(map(float, q)) for q in re.findall(r'\((?:start|end|xy|center|mid) ([-\d.]+) ([-\d.]+)\)', g)]
            if loc:
                break
        th = math.radians(rot)
        pts = [(x + px * math.cos(th) + py * math.sin(th), y - px * math.sin(th) + py * math.cos(th)) for px, py in loc]
        fps.append(dict(ref=ref, name=name.split(":")[-1], x=x, y=y, rot=rot, side=side, models=models,
                        drill=max(drills) if drills else 0.0, pts=pts))
    st = open(FIT_STEP, encoding="latin-1").read()
    step_refs = sorted(set(r for r in re.findall(r"NEXT_ASSEMBLY_USAGE_OCCURRENCE\('[^']*','([^']*)'", st)
                           if not r.startswith("=>")))
    xs = [p[0] for p in edge]; ys = [p[1] for p in edge]
    return dict(edge=edge, segs=segs, bbox=(min(xs), max(xs), min(ys), max(ys)), fps=fps, keepouts=keepouts,
                step_refs=step_refs, step_mtime=os.path.getmtime(FIT_STEP), pcb_mtime=os.path.getmtime(FIT_PCB))

def fit_proxies(info, z_back=Z_BOARD_B):
    """Boxes for parts the STEP does not contain: [(label, front-view polygon, z0, h)]."""
    out, notes = [], []
    zF = lambda h, side: (z_back - h, h) if side == "F" else (Z_BOARD_F, h)
    for fp in info["fps"]:
        if not fp["models"] or fp["ref"] in info["step_refs"]:
            continue
        h = next((hh for pat, hh in FIT_PROXY_H if pat in fp["name"]), None)
        if h is None or not fp["pts"]:
            notes.append("%s (%s): 3D model not in the STEP and no proxy height known" % (fp["ref"], fp["name"]))
            continue
        xs = [p[0] for p in fp["pts"]]; ys = [p[1] for p in fp["pts"]]
        poly_ = [F(xs_, ys_) for xs_, ys_ in ((min(xs), min(ys)), (max(xs), min(ys)), (max(xs), max(ys)), (min(xs), max(ys)))]
        out.append(("%s %s" % (fp["ref"], fp["name"]), poly_) + zF(h, fp["side"]))
    for key, label, size, h, side in FIT_KEEPOUT_PARTS:
        hit = [k for k in info["keepouts"] if abs(k[0] - key[0]) < 0.3 and abs(k[1] - key[1]) < 0.3]
        if not hit:
            notes.append("%s: no %.1f x %.1f keep-out on User.3, not placed" % (label, key[0], key[1]))
            continue
        c = F(*hit[0][2])
        poly_ = [(c[0] + sx * size[0] / 2, c[1] + sy * size[1] / 2) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        out.append((label, poly_) + zF(h, side))
    return out, notes

def fit_features(G):
    """Named shell features for labelling collisions: (target, label, kind, data)."""
    f = []
    for n, c in G["screws"]:
        f += [("Front shell", "screw post " + n, "circle", (c, SCREW_POST_D / 2)),
              ("Back cover", "screw boss " + n, "circle", (c, BOSS_D / 2))]
    for n, c in G["locs"]:
        f.append(("Front shell", "locating post " + n, "circle", (c, LOC_POST_D / 2)))
    for q in G["inner_ribs"]:
        f.append(("Front shell", "inner rib of the 4.7 screen-section wall (C4)", "poly", q))
    for q in G["pins"]:
        y = sum(p[1] for p in q) / 4
        f.append(("Front shell", ("wire holder" if abs(y - HOLDER_Y) < 1 else "side-wall stub (C14)") + " y=%.1f" % y, "poly", q))
    f.append(("Front shell", "LR44 cup (D7)", "circle", (DOOR_C, COIN_CUP[0] / 2 + COIN_CUP[1])))
    f.append(("Front shell", "solar-window frame", "poly", rrect_pts(SOLAR_C, SOLAR[0] + 2, SOLAR[1] + 2, 1)))
    for a, b in G["row_ribs"]:
        f.append(("Front shell", "key-row rib", "seg", (a, b, ROW_RIB_T)))
    for n, c, r in G["rings"]:
        f.append(("Back cover", n + " (D13, %.0f high)" % RING_H, "ring", (c, r - RING_T / 2, RING_T)))
    for n, a, b, h in G["ribs"]:
        f.append(("Back cover", "%s (%.0f high)" % (n, h), "seg", (a, b, RIB_T)))
    for a, b in G["box"]:
        f.append(("Back cover", "solar box frame (D6, %.1f high)" % SOLAR_BOX_H, "seg", (a, b, RIB_T)))
    for a, b in G["grid"]:
        f.append(("Back cover", "solar box grid (D13, %.0f high)" % SOLAR_GRID_H, "seg", (a, b, RIB_T)))
    for q in G["ticks"]:
        f.append(("Back cover", "wall tick", "poly", q))
    return f

def fit_label(target, X, Y, zlo, zhi, feats, G):
    def d(kind, data):
        if kind == "circle":
            return max(0.0, math.dist((X, Y), data[0]) - data[1])
        if kind == "ring":
            return max(0.0, abs(math.dist((X, Y), data[0]) - data[1]) - data[2] / 2)
        if kind == "seg":
            return max(0.0, seg_dist((X, Y), data[0], data[1]) - data[2] / 2)
        return 0.0 if inside((X, Y), data) else poly_dist((X, Y), data)
    best = min(((d(k, dd), lab) for t, lab, k, dd in feats if t == target), default=(99, ""))
    if best[0] < 1.5:
        return best[1]
    if target == "Back cover":
        return "back floor" if zhi <= BACK_PLATE + 0.3 else ("lip / rim" if not inside((X, Y), G["lip_i"]) else "back cover")
    if target == "Front shell":
        return "front plate underside" if zlo >= Z_PLATE - 0.3 else ("outer skin / wall" if not inside((X, Y), G["C"]) else "front shell")
    return target

def stage_fitcheck(g):
    import adsk.core, adsk.fusion
    info = board_info()
    root = design.rootComponent
    for occ in list(root.occurrences):
        if occ.component.name.startswith(FIT_PREFIX):
            occ.deleteMe()
        else:
            occ.transform = adsk.core.Matrix3D.create()          # undo any explode
    occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
    occ.component.name = FIT_PCB_COMP
    t0 = time.time()
    im = app.importManager
    im.importToTarget(im.createSTEPImportOptions(FIT_STEP), occ.component)
    log("fitcheck: STEP imported in %.0f s" % (time.time() - t0))

    def bodies_under(o):
        out = list(o.bRepBodies)
        for c in o.childOccurrences:
            out += bodies_under(c)
        return out
    pcb = bodies_under(occ)
    span = lambda b: (b.boundingBox.maxPoint.x - b.boundingBox.minPoint.x) * (b.boundingBox.maxPoint.y - b.boundingBox.minPoint.y)
    board = max(pcb, key=span)
    bb = board.boundingBox
    xs0, xs1, ys0, ys1 = bb.minPoint.x * 10, bb.maxPoint.x * 10, bb.minPoint.y * 10, bb.maxPoint.y * 10
    zs0, zs1 = bb.minPoint.z * 10, bb.maxPoint.z * 10
    kx0, kx1, ky0, ky1 = info["bbox"]
    ox, oy = xs0 - kx0, ys1 + ky0                  # STEP = (x_k + ox, -y_k + oy, z), board B.Cu face at zs0
    size_err = max(abs((xs1 - xs0) - (kx1 - kx0)), abs((ys1 - ys0) - (ky1 - ky0)))
    if size_err > 0.05:                            # STEP and KiCad file are from different revisions:
        ox, oy = 0.0, 0.0                          # KiCad's export keeps its own coordinates, trust that
        log("fitcheck: WARNING STEP board size differs from Edge.Cuts by %.2f: offset forced to 0" % size_err)
    # front view: X = 150 - x_k, Y = 138.94 - y_k; F.Cu (STEP +z) faces the back cover, B.Cu (keys) sits at Z_BOARD_F
    m = adsk.core.Matrix3D.create()
    m.setWithArray([-1, 0, 0, cm(CX + ox), 0, 1, 0, cm(CY - oy), 0, 0, -1, cm(Z_BOARD_F + zs0), 0, 0, 0, 1])
    occ.transform2 = m
    try:
        design.snapshots.add()                        # parametric design: capture the new position
    except Exception:
        pass
    adsk.doEvents()
    pcb = bodies_under(occ)
    # name every STEP part after its KiCad reference: same 3D model, nearest footprint origin
    stem = lambda f: os.path.splitext(f)[0].lower()
    names, res, pairs_ = {}, [], []
    okey = lambda b: b.assemblyContext.fullPathName if b.assemblyContext else b.name
    occs = {}
    for b in pcb:
        if b != board:
            occs.setdefault(okey(b), b.assemblyContext)
    for key_, ctx in occs.items():
        comp = (ctx.component.name if ctx else key_).split(" (")[0].lower()
        names[key_] = comp
        c = ctx.boundingBox
        X, Y = (c.minPoint.x + c.maxPoint.x) * 5, (c.minPoint.y + c.maxPoint.y) * 5
        for fp in info["fps"]:
            if any(stem(mm) == comp for mm in fp["models"]):
                pairs_.append((math.dist((X, Y), F(fp["x"], fp["y"])), key_, fp["ref"], comp))
    done_b, done_r = set(), set()
    for dd, tok, ref, comp in sorted(pairs_):          # global nearest-first matching
        if tok in done_b or ref in done_r:
            continue
        done_b.add(tok); done_r.add(ref)
        names[tok] = "%s (%s)" % (ref, comp)
        res.append((dd, ref))
    for key_, ctx in occs.items():                     # STEP part named differently from the model file
        if key_ in done_b:
            continue
        c = ctx.boundingBox
        X, Y = (c.minPoint.x + c.maxPoint.x) * 5, (c.minPoint.y + c.maxPoint.y) * 5
        cand = [fp for fp in info["fps"] if fp["models"] and fp["ref"] not in done_r]
        if cand:
            fp = min(cand, key=lambda fp: math.dist((X, Y), F(fp["x"], fp["y"])))
            if math.dist((X, Y), F(fp["x"], fp["y"])) < 3.0:
                done_r.add(fp["ref"]); names[key_] = "%s (%s)" % (fp["ref"], names[key_])
    res.sort()
    log("fitcheck: placement residual median %.2f max %.2f (%d parts)" % (
        res[len(res) // 2][0] if res else -1, res[-1][0] if res else -1, len(res)))

    # boxes for parts that have no model in the STEP
    proxies, notes = fit_proxies(info, Z_BOARD_F - (zs1 - zs0))
    pc = root.occurrences.addNewComponent(adsk.core.Matrix3D.create()).component
    pc.name = FIT_PROXY_COMP
    for label, q, z0, h in proxies:
        sk = new_sketch(pc, label)
        poly(sk, q)
        extrude(pc, profiles(sk), z0, h, name="proxy " + label)
    for b in pc.bRepBodies:
        appearance(b, "Plastic - Matte (Yellow)")
    prox = list(pc.bRepBodies)

    # interference: every PCB / proxy body against the shell parts
    tbm = adsk.fusion.TemporaryBRepManager.get()
    def tcopy(b):
        t = tbm.copy(b)
        tb_, bb_ = t.boundingBox, b.boundingBox
        if abs(tb_.minPoint.x - bb_.minPoint.x) > 1e-4 or abs(tb_.minPoint.z - bb_.minPoint.z) > 1e-4:
            tbm.transform(t, b.assemblyContext.transform2)
        return t
    targets = []
    for name in FIT_TARGETS:
        try:
            for b in comp_named(name).bRepBodies:
                targets.append((name, b, tcopy(b)))
        except KeyError:
            pass
    hits, errs, pairs = [], [], 0
    t0 = time.time()
    for b in pcb + prox:
        part = ("proxy: " + b.name[6:]) if b in prox else names.get(okey(b), b.name)
        pz = (b.assemblyContext.boundingBox if (b not in prox and b.assemblyContext) else b.boundingBox).minPoint.z * 10
        if b == board:
            part = "PCB (board)"
        for tname, tb_body, tcp in targets:
            if not b.boundingBox.intersects(tb_body.boundingBox):
                continue
            pairs += 1
            try:
                a = tcopy(b) if b not in prox else tbm.copy(b)
                tbm.booleanOperation(a, tbm.copy(tcp), adsk.fusion.BooleanTypes.IntersectionBooleanType)
            except Exception as e:
                errs.append("%s vs %s: %s" % (part, tname, str(e)[:60]))
                continue
            for k in range(a.lumps.count if a else 0):
                L = a.lumps.item(k)
                v = L.volume * 1000
                lb = L.boundingBox
                if v < 0.01 or min(lb.maxPoint.x - lb.minPoint.x, lb.maxPoint.y - lb.minPoint.y,
                                   lb.maxPoint.z - lb.minPoint.z) * 10 < 0.02:   # touching faces
                    continue
                lo = [lb.minPoint.x * 10, lb.minPoint.y * 10, lb.minPoint.z * 10]
                hi = [lb.maxPoint.x * 10, lb.maxPoint.y * 10, lb.maxPoint.z * 10]
                try:                                  # lump boxes can be loose: tighten on the edge points
                    pts_ = []
                    for si in range(L.shells.count):
                        for e in L.shells.item(si).edges:
                            ev = e.evaluator
                            ok, p0, p1 = ev.getParameterExtents()
                            for t in range(9):
                                ok2, q = ev.getPointAtParameter(p0 + (p1 - p0) * t / 8)
                                if ok2:
                                    pts_.append((q.x * 10, q.y * 10, q.z * 10))
                    if pts_:
                        lo = [max(lo[i], min(q[i] for q in pts_)) for i in range(3)]
                        hi = [min(hi[i], max(q[i] for q in pts_)) for i in range(3)]
                except Exception:
                    pass
                hits.append(dict(part=part, target=tname, lo=lo, hi=hi, vol=v, pz=pz))
    log("fitcheck: %d pairs, %d overlaps, %d errors, %.0f s" % (pairs, len(hits), len(errs), time.time() - t0))
    for h in hits:
        X, Y = (h["lo"][0] + h["hi"][0]) / 2, (h["lo"][1] + h["hi"][1]) / 2
        h["feature"] = fit_label(h["target"], X, Y, h["lo"][2], h["hi"][2], fit_features(g), g)
    occ.isLightBulbOn = True
    write_fit_report(info, g, hits, errs, notes, proxies, dict(ox=ox, oy=oy, zs0=zs0, t=zs1 - zs0, size_err=size_err,
                     resid=res, n_bodies=len(pcb), pairs=pairs))
    print("fitcheck: %d bodies, %d overlaps -> replica/fitcheck_report.md" % (len(pcb) + len(prox), len(hits)))

def fit_2d(info, g):
    """Exact 2D clearances at board height: KiCad holes vs posts, board edge vs walls/stubs/posts."""
    rows = []
    posts = [(n, c, SCREW_POST_D, "screw post") for n, c in g["screws"]] + [(n, c, LOC_POST_D, "locating post") for n, c in g["locs"]]
    for fp in info["fps"]:
        if not fp["drill"] or not fp["ref"].startswith("H"):
            continue
        hc = F(fp["x"], fp["y"])
        n, c, d, kind = min(posts, key=lambda p: math.dist(p[1], hc))
        dist = math.dist(c, hc)
        rows.append(("hole %s Ã˜%.1f" % (fp["ref"], fp["drill"]), "%s %s Ã˜%.2f" % (kind, n, d),
                     fp["drill"] / 2 - d / 2 - dist, "hole centre %.2f from the post" % dist))
    E = [F(*p) for p in info["edge"]]
    S = [(F(*a), F(*b)) for a, b in info["segs"]]
    def in_board(p):                                   # ray casting over the Edge.Cuts segments (slot counts as outside)
        c = False
        for (x1, y1), (x2, y2) in S:
            if (y1 > p[1]) != (y2 > p[1]) and p[0] < x1 + (p[1] - y1) * (x2 - x1) / (y2 - y1):
                c = not c
        return c
    def clearance(pts):
        """> 0: gap between the feature and the board edge; < 0: the feature reaches this far into the board."""
        ins = [q for q in pts if in_board(q)]
        if ins:
            d, q = max((min(math.dist(q, e) for e in E), q) for q in ins)
            return -d, q
        return min((math.dist(q, e), e) for q in pts for e in E)
    feats = [("front-shell inner skin (outside it = wall)", densify(offset_const(g["C"], -0.01), 0.5)[::1], True)]
    if INNER_RIB_FULL or STUB_Z0 <= Z_BOARD_F:        # the rib reaches the board plane (rev F) -> a board-edge clearance
        for q in g["inner_ribs"]:
            feats.append(("inner rib (C4 4.7 wall)", densify(q, 0.2), False))
    if STUB_Z0 <= Z_BOARD_F:                           # only when they reach down to the board
        for q in g["pins"]:
            y = sum(p[1] for p in q) / 4
            feats.append((("wire holder" if abs(y - HOLDER_Y) < 1 else "side-wall stub (C14)") + " y=%.1f" % y, densify(q, 0.2), False))
    holes = [F(fp["x"], fp["y"]) for fp in info["fps"] if fp["drill"] and fp["ref"].startswith("H")]
    for n, c, d, kind in posts:
        if any(math.dist(c, h) < 3.0 for h in holes):
            continue                                   # goes through a board hole: see the hole rows
        feats.append(("%s %s" % (kind, n), circle_pts(c, d), False))
    for label, pts, is_wall in feats:
        if is_wall:                                    # board must stay inside the skin: edge points outside it
            out_ = [e for e in E if not inside(e, g["C"])]
            if out_:
                dd, q = max((poly_dist(e, g["C"]), e) for e in out_)
                rows.append(("board edge near (%.1f, %.1f)" % q, label, -dd, "KiCad (%.2f, %.2f)" % (CX - q[0], CY - q[1])))
            else:
                dd, q = min((poly_dist(e, g["C"]), e) for e in E)
                rows.append(("board edge near (%.1f, %.1f)" % q, label, dd, "KiCad (%.2f, %.2f)" % (CX - q[0], CY - q[1])))
            continue
        dmin, q = clearance(pts)
        if dmin < 1.0:
            rows.append(("board near (%.1f, %.1f)" % q, label, dmin, "KiCad (%.2f, %.2f)" % (CX - q[0], CY - q[1])))
    return rows

def fit_vs_clearance_table(hits, free):
    """Compare with the PCB agent's hardware/fitcheck/clearance_table.md (read only), if it exists."""
    import re
    path = os.path.join(REPO, "hardware", "fitcheck", "clearance_table.md")
    if not os.path.exists(path):
        return []
    theirs = {}
    for line in open(path, encoding="utf8"):
        c = [x.strip().strip("*") for x in line.strip().strip("|").split("|")]
        if len(c) >= 8 and re.match(r"^[A-Z]+\d+$", c[0]):
            try:
                theirs[c[0]] = (float(c[1]), c[2], float(c[4]), float(c[7]))
            except ValueError:
                pass
    mine = {}
    for h in hits:
        ref = h["part"].split(" ")[0]
        d = min(h["hi"][i] - h["lo"][i] for i in range(3))
        if ref not in mine or d > mine[ref][0]:
            mine[ref] = (d, h["target"] + ": " + h["feature"], h["pz"] - BACK_PLATE)
    L = ["", "## Compared with `hardware/fitcheck/clearance_table.md` (PCB agent)", "",
         "That table assumes %s free height (\"design\" column 4.5) and adds a rib's height to the part height whenever "
         "a rib is within 1.5 mm. This report uses exact 3D overlap with the photo rib positions and %.2f free height "
         "(D4 = 6.0 to the inside of the floor). Rows where the two disagree, or that they flag:" % ("a 4.0 / 4.5 / 5.5", free), "",
         "| Part | Their height | Their margin @4.5 | Their margin after grind | Here: overlap | Here: part bottom above the floor | Agree? |",
         "| --- | --- | --- | --- | --- | --- | --- |"]
    for ref, (ht, feat, m45, mg) in sorted(theirs.items(), key=lambda t: t[1][2]):
        if m45 >= 0.3 and ref not in mine:
            continue
        me = mine.get(ref)
        floor_margin = free - ht
        agree = "yes" if (me is not None) == (m45 < 0.3) else "no"
        L.append("| %s | %.2f | %.2f | %.2f | %s | %.2f | %s |" % (
            ref, ht, m45, mg, ("%.2f (%s)" % (me[0], me[1])) if me else "none", me[2] if me else floor_margin, agree))
    for ref in sorted(set(mine) - set(theirs)):
        if ref.startswith("proxy") or ref.startswith("PCB"):
            continue
        L.append("| %s | - | (not in their table) | - | %.2f (%s) | %.2f | no |" % (ref, mine[ref][0], mine[ref][1], mine[ref][2]))
    cam = [h for h in hits if "camera" in h["part"]]
    L += ["", "- Camera: not in their table (it is a separate module). Here: %s." % (
        ("%.2f overlap with %s, bottom %.2f above the floor" % (min(cam[0]["hi"][i] - cam[0]["lo"][i] for i in range(3)),
                                                                cam[0]["feature"], cam[0]["pz"] - BACK_PLATE)) if cam else "no collision"),
          "- Free height: their design value 4.5 came from reading D4 to the outside of the back; Nirav has since said D4 = 6.0 is to the inside face, so their \"optimistic 5.5\" column is the closest, and with 6.0 every part that only clashes with the floor clears it (J4 bottom %.2f, J3 bottom %.2f above the floor, from the STEP models)." % (
              mine.get("J4", (0, 0, free - 5.5))[2], mine.get("J3", (0, 0, free - 5.0))[2])]
    return L

def write_fit_report(info, g, hits, errs, notes, proxies, t):
    import datetime
    ts = lambda s: datetime.datetime.fromtimestamp(s).strftime("%Y-%m-%d %H:%M:%S")
    L = ["# Fit check: our PCB in the fx-115ES replica", "",
         "Generated by `python build_fx115es_replica.py fitcheck` on %s. Re-run it after the PCB STEP is regenerated." % ts(time.time()), "",
         "| Input | File | Timestamp |", "| --- | --- | --- |",
         "| PCB STEP | `%s` | **%s** |" % (ARGS.get("step_label", "hardware/fab/ai_calc_board.step"), ts(info["step_mtime"])),
         "| KiCad board (read only: positions, holes, keep-outs) | `%s` | %s |" % (ARGS.get("pcb_label", "hardware/kicad/ai_calc.kicad_pcb"), ts(info["pcb_mtime"])), "",
         "## Placement", "",
         "- Front-view frame: X = 150 âˆ’ x_kicad, Y = 138.94 âˆ’ y_kicad, Z from the outside of the back cover. "
         "The board is turned over (180Â° about Y): F.Cu parts face the back cover, the key pads (B.Cu) face the keys.",
         "- Board key side (B.Cu) at Z = %.2f, back side (F.Cu) at Z = %.2f; free height behind the board to the back floor %.2f (D4 = 6.0 to the inside of the floor, rev B2). See \"Stack-up used\" for how the thickness readings were reconciled." % (Z_BOARD_F, Z_BOARD_F - t["t"], Z_BOARD_F - t["t"] - BACK_PLATE),
         "- STEP frame: x_step = x_kicad %+.2f, y_step = âˆ’y_kicad %+.2f, board %.2f mm thick (KiCad 0.8: our parts sit %.2f mm closer to the floor in reality); STEP board size matches Edge.Cuts to %.2f mm%s." % (
             t["ox"], t["oy"], t["t"], 0.8 - t["t"], t["size_err"], "" if t["size_err"] <= 0.05 else " (**mismatch: STEP and KiCad file are different revisions**)"),
         "- Check: %d STEP parts land %.2f mm (median) / %.2f mm (max, model offsets) from their KiCad footprint origins." % (
             len(t["resid"]), t["resid"][len(t["resid"]) // 2][0] if t["resid"] else -1, t["resid"][-1][0] if t["resid"] else -1),
         "- Bodies checked: %d from the STEP + %d proxies, against %s. %d body pairs had overlapping boxes." % (
             t["n_bodies"], len(proxies), ", ".join(FIT_TARGETS), t["pairs"]), "",
         "### Parts not in the STEP", ""]
    L += ["- Proxy box: **%s**, Z %.2f to %.2f" % (lab, z0, z0 + h) for lab, q, z0, h in proxies]
    L += ["- %s" % n for n in notes]
    L += ["- Not modelled: the magnetic connector (P5 not in hand), wires, solder fillets.", ""]
    L += ["## Collisions (3D)", ""]
    if not hits:
        L.append("None.")
    else:
        L += ["Depth = the smallest size of the overlap box (how far the parts would have to move apart). "
              "Location is the middle of the overlap: front-view X/Y and KiCad x/y. Sorted by depth.", "",
              "| # | PCB part | Hits | X, Y (front view) | x, y (KiCad) | Z range | Overlap box (mm) | Depth | Volume mmÂ³ | Note |",
              "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
        hits.sort(key=lambda h: -min(h["hi"][i] - h["lo"][i] for i in range(3)))
        for k, h in enumerate(hits, 1):
            X, Y = (h["lo"][0] + h["hi"][0]) / 2, (h["lo"][1] + h["hi"][1]) / 2
            dims = [h["hi"][i] - h["lo"][i] for i in range(3)]
            note = ""
            if h["target"] == "Back cover" and h["part"] != "PCB (board)":
                room = h["pz"] - BACK_PLATE
                note = ("part bottom Z %.2f: %.2f into the floor itself (grinding ribs is not enough)" % (h["pz"], -room)
                        if room < 0 else "part bottom Z %.2f, %.2f above the floor: grind the rib/box" % (h["pz"], room))
            elif h["part"] == "PCB (board)":
                note = "the board itself"
            L.append("| %d | %s | %s: %s | %.1f, %.1f | %.1f, %.1f | %.2fâ€“%.2f | %.2f Ã— %.2f Ã— %.2f | **%.2f** (%s) | %.2f | %s |" % (
                k, h["part"], h["target"], h["feature"], X, Y, CX - X, CY - Y, h["lo"][2], h["hi"][2],
                dims[0], dims[1], dims[2], min(dims), "XYZ"[dims.index(min(dims))], h["vol"], note))
    if errs:
        L += ["", "Boolean errors (not checked): " + "; ".join(errs)]
    L += ["", "## Clearances at board height (2D, exact)", "",
          "Negative = overlap. Holes: radial clearance between the KiCad hole and the replica post "
          "(post positions from calipers C8/C9/C10/C15, the rest from photos).", "",
          "| Board feature | Shell feature | Clearance mm | Note |", "| --- | --- | --- | --- |"]
    for a, b, c, n in sorted(fit_2d(info, g), key=lambda r: r[2]):
        L.append("| %s | %s | %s%.2f%s | %s |" % (a, b, "**" if c < 0 else "", c, "**" if c < 0 else "", n))
    win_c = g["win_c"]
    epd = [k for k in info["keepouts"] if abs(k[0] - 59.2) < 0.3 and abs(k[1] - 29.2) < 0.3]
    if epd:
        ec = F(*epd[0][2])
        L += ["", "## Display", "",
              "- E-paper glass (P3 %.1f Ã— %.1f) centre %.2f, %.2f vs window (C12 %.2f Ã— %.2f) centre %.2f, %.2f: offset %.2f, %.2f." % (
                  EPD_P3[0], EPD_P3[1], ec[0], ec[1], WIN[0], WIN[1], win_c[0], win_c[1], ec[0] - win_c[0], ec[1] - win_c[1]),
              "- The window is %.2f mm wider than the glass: %.2f mm of board shows at each end unless masked." % (
                  WIN[0] - EPD_P3[0], (WIN[0] - EPD_P3[0]) / 2)]
    if STUB_Z0 > Z_BOARD_F:
        L += ["", "Side-wall stubs, wire holders and the inner rib end at Z %.2f, %.2f above the board's key side "
              "(Nirav: they stop above the board plane), so they are not board-edge clearances any more; "
              "parts on the key side under them are in the 3D table." % (STUB_Z0, STUB_Z0 - Z_BOARD_F)]
    L += fit_vs_clearance_table(hits, Z_BOARD_F - t["t"] - BACK_PLATE)
    L += ["", "## Stack-up used", "",
          "**How the thickness readings fit together (rev B2, Nirav's answers).** D4 = 6.0 is from the board's back "
          "(component, F.Cu) face to the *inside* of the back cover; D1 = 5.5 is from the front shell's *rim* (parting line) "
          "down to that same face. Stack at the top part: floor 1.0 (D5) + 6.0 (D4) + board 0.8 + %.1f front side = %.1f "
          "(D10 11.8); at the keys %.1f front side gives %.1f (D10 11.3). Parting line at Z %.1f. Cross-checks: D3 (board line "
          "to the outside at the top edge, \"about 5\") = %.1f; photo 871567b6: the LCD shield (D8 3.75 under a 1.2 plate) "
          "ends at Z %.2f, level with the Casio board's back face (Z %.1f)." % (
              T_TOP - Z_BOARD_F, BACK_PLATE + D4_IN + BOARD_T + T_TOP - Z_BOARD_F, T_KEY - Z_BOARD_F,
              BACK_PLATE + D4_IN + BOARD_T + T_KEY - Z_BOARD_F, SEAM_Z, T_TOP - Z_BOARD_B, Z_PLATE_TOP - 0.05 - LCD_T, Z_BOARD_B), "",
          "| Z (mm) | What |", "| --- | --- |",
          "| 0 â€“ %.1f | back cover floor (D5) |" % BACK_PLATE,
          "| %.1f â€“ %.1f | rings (D13: 4 high); rib grid and lower ribs 1 high; solar box grid to %.1f, frame to %.1f (D13, D6) |" % (
              BACK_PLATE, BACK_PLATE + RING_H, BACK_PLATE + SOLAR_GRID_H, BACK_PLATE + SOLAR_BOX_H),
          "| %.1f â€“ %.1f | PCB (F.Cu parts below, towards the back) |" % (Z_BOARD_B, Z_BOARD_F),
          "| %.1f â€“ %.1f | keymat sheet + domes (D9) |" % (Z_BOARD_F, Z_DOME_TOP),
          "| %.1f / %.1f | front plate underside, keypad / screen section |" % (Z_PLATE, Z_PLATE_TOP),
          "| %.1f / %.1f | face, keypad / screen section (D10) |" % (T_KEY, T_TOP), ""]
    rp = os.path.join(OUT, ARGS.get("report", "fitcheck_report.md"))
    open(rp, "w", encoding="utf8").write("\n".join(L) + "\n")
    prev = ARGS.get("prev", os.path.join(OUT, "fitcheck_report_stage11.md"))   # earlier run to compare with
    if prev != "none" and os.path.exists(prev) and os.path.abspath(prev) != os.path.abspath(rp):
        open(rp, "a", encoding="utf8").write("\n" + fit_compare(prev, rp))

def _parse_report(path):
    """Collisions and 2D clearances from a fitcheck_report.md: ({(part, feature): (depth, where)}, {item: (mm, where)}, stamp)."""
    import re
    out, two, stamp = {}, {}, "?"
    if not os.path.exists(path):
        return out, two, stamp
    section = ""
    for line in open(path, encoding="utf8"):
        if line.startswith("## "):
            section = line
        m = re.match(r"\| PCB STEP \| .* \| \*\*(.+?)\*\* \|", line)
        if m:
            stamp = m.group(1)
        if "Before / after" in section:
            break
        c = [x.strip() for x in line.strip().strip("|").split("|")]
        if section.startswith("## Collisions") and len(c) >= 9 and c[0].isdigit():
            part = c[1] if c[1].startswith("proxy") else re.sub(r" \(.*\)$", "", c[1])
            tgt = re.sub(r" \((D\d+[^)]*|\d+(\.\d+)? high)\)", "", c[2])
            if tgt in ("Back cover: back cover", "Back cover: solar box"):
                tgt = "Back cover: solar box + floor"
            d = float(re.search(r"\*\*([\d.]+)\*\*", c[7]).group(1))
            k = (part, tgt)
            if k not in out or d > out[k][0]:
                out[k] = (d, c[3])
        elif section.startswith("## Clearances") and len(c) == 4 and (c[0].startswith("hole") or c[0].startswith("board")):
            try:
                v = float(c[2].strip("*"))
            except ValueError:
                continue
            k = c[1] if c[0].startswith("board") else c[0].split(" Ã˜")[0]
            k = re.sub(r" y=[\d.]+", "", k) + ("" if c[0].startswith("hole") else (" (right)" if float(c[0].split("(")[1].split(",")[0]) > 0 else " (left)"))
            if k not in two or v < two[k][0]:
                two[k] = (v, c[0])
    return out, two, stamp

def fit_compare(prev_path, cur_path):
    """Before/after section text (previous report -> current report)."""
    a, a2, sa = _parse_report(prev_path)
    b, b2, sb = _parse_report(cur_path)
    L = ["## Before / after: `%s` vs this run" % os.path.basename(prev_path), "",
         "STEP %s (before) vs %s (now). Depth = worst overlap per part and shell feature (mm)." % (sa, sb), "",
         "| PCB part | Shell feature | Before | Now | Status |", "| --- | --- | --- | --- | --- |"]
    for k in sorted(set(a) | set(b), key=lambda k: -max(a.get(k, (0,))[0], b.get(k, (0,))[0])):
        da, db = a.get(k, (None,))[0], b.get(k, (None,))[0]
        st = "**gone**" if db is None else ("new" if da is None else ("same" if abs(da - db) < 0.02 else ("better" if db < da else "worse")))
        L.append("| %s | %s | %s | %s | %s |" % (k[0], k[1], "-" if da is None else "%.2f" % da,
                                                   "-" if db is None else "**%.2f**" % db, st))
    L += ["", "2D clearances at board height (negative = overlap; worst value per item, only items below 0.3 mm):", "",
          "| Item | Before | Now | Status |", "| --- | --- | --- | --- |"]
    for k in sorted(set(a2) | set(b2), key=lambda k: min(a2.get(k, (9,))[0], b2.get(k, (9,))[0])):
        va, vb = a2.get(k, (None,))[0], b2.get(k, (None,))[0]
        if (va is None or va >= 0.3) and (vb is None or vb >= 0.3):
            continue
        if vb is None or vb >= 0:
            st = "**ok now**" if (va is not None and va < 0) else "ok"
        else:
            st = "still overlaps" if (va is not None and va < 0) else "new overlap"
        L.append("| %s | %s | %s | %s |" % (k, "-" if va is None else "%.2f" % va, "-" if vb is None else "%.2f" % vb, st))
    return "\n".join(L) + "\n"

def stage_fitcheck_shots(g):
    """Renders with the PCB in place: iso views with one shell half hidden, and a section across the screen section."""
    shell_front = "Front shell;Keycaps;Keymat;Window lens;Solar cell (dummy);Reference - LCD (dummy);Reference - Casio board (C6);Slide case (stored)"
    shell_back = "Back cover;Battery lid;Rubber feet;Reference - LCD (dummy);Reference - Casio board (C6);Slide case (stored)"
    for views, hide, prefix in (("iso_front,front", shell_front, "fitcheck_"), ("iso_back,back", shell_back, "fitcheck_open_back_")):
        ARGS.update(views=views, hide=hide, prefix=prefix)
        stage_shots(g)
    # section across the screen section (through the window centre by default): a Fusion section
    # analysis, so the imported board and its parts are cut too
    import adsk.core
    y = float(ARGS.get("y", g["win_c"][1]))
    sa = design.analyses.sectionAnalyses
    inp = sa.createInput(design.rootComponent.xZConstructionPlane, cm(y))
    inp.flip = ARGS.get("flip", "0") == "1"
    inp.isHatchShown = True
    sec = sa.add(inp)
    try:
        ARGS.update(views=ARGS.get("sec_views", "top_end,eye@0;160;6;0;%g;6;95,eye@25;150;6;25;%g;6;40" % (y, y)),
                    hide="Slide case (stored);Reference - Casio board (C6);Reference - LCD (dummy)", prefix="section_screen_")
        stage_shots(g)
    finally:
        sec.deleteMe()

def stage_fitcheck_cuts(g):
    """Section renders through named PCB parts (default: the camera and J4), each from the top end
    (whole width) and a close-up. Part positions come from the KiCad file, so they follow PCB edits."""
    import adsk.core
    info = board_info()
    fpd = {fp["ref"]: fp for fp in info["fps"]}
    cam = [k for k in info["keepouts"] if abs(k[0] - 12.0) < 0.3 and abs(k[1] - 12.0) < 0.3]
    cuts = []
    for name in ARGS.get("cuts", "camera,J4").split(","):
        if name == "camera" and cam:
            cuts.append(("camera", F(*cam[0][2])))
        elif name in fpd:
            cuts.append((name, F(fpd[name]["x"], fpd[name]["y"])))
    sa = design.analyses.sectionAnalyses
    for name, (x, y) in cuts:
        inp = sa.createInput(design.rootComponent.xZConstructionPlane, cm(y))
        inp.isHatchShown = True
        sec = sa.add(inp)
        try:
            ARGS.update(views="top_end,eye@%g;%g;6;%g;%g;5;24" % (x, y + 150, x, y),
                        hide="Slide case (stored);Reference - Casio board (C6);Reference - LCD (dummy)",
                        prefix="section_%s_" % name.lower())
            stage_shots(g)
        finally:
            sec.deleteMe()
    print("cuts:", [(n, round(c[0], 2), round(c[1], 2)) for n, c in cuts])

# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
STAGES = ["new", "front", "back", "keys", "parts", "check", "export", "shots", "explode_shots"]

def to_json(g):
    return json.loads(json.dumps(g))

def send(stage, extra):
    """Run one stage inside Fusion through the FusionMCPBridge add-in; wait on build.log if it runs long."""
    import urllib.request, urllib.error
    args = dict(extra, stage=stage)
    script = "ARGS = %r\n" % args + open(os.path.abspath(__file__), encoding="utf8").read()
    secret = open(os.path.join(os.path.expanduser("~"), ".fusion-mcp-secret")).read().strip()
    req = urllib.request.Request("http://127.0.0.1:7654/execute", data=json.dumps({"script": script}).encode(),
                                 headers={"Authorization": "Bearer " + secret, "Content-Type": "application/json"})
    logf = os.path.join(OUT, "build.log")
    start = os.path.getsize(logf) if os.path.exists(logf) else 0
    try:
        r = json.load(urllib.request.urlopen(req, timeout=60))
    except urllib.error.HTTPError as e:
        r = json.load(e)
    except Exception as e:
        r = {"error": "timeout: %s" % e}
    if "timeout" in str(r.get("error", "")).lower():
        tail = lambda: open(logf, "rb").read()[start:].decode("utf8", "replace") if os.path.exists(logf) else ""
        t0 = time.time()
        while "stage %s done" % stage not in tail() and "FAILED" not in tail():
            if time.time() - t0 > 900:
                r = {"error": "gave up waiting"}; break
            time.sleep(3)
        else:
            r = {"result": "(finished after the bridge timed out)\n" + tail()[-800:]}
            if "FAILED" in tail():
                r = {"result": "", "traceback": tail()[-2500:]}
    print(r.get("result") or r.get("error") or r)
    if r.get("traceback") or r.get("error"):
        print(r.get("traceback", ""))
        sys.stdout.flush()
        os._exit(1)

if "ARGS" in globals():
    import adsk.core, adsk.fusion, traceback
    log("stage " + ARGS["stage"])
    try:
        G = json.load(open(os.path.join(OUT, "geometry.json")))
        G["screws"] = [(n, tuple(c)) for n, c in G["screws"]]
        G["locs"] = [(n, tuple(c)) for n, c in G["locs"]]
        if ARGS["stage"] != "new":
            design = find_design()
        globals()["stage_" + ARGS["stage"]](G)
    except Exception:
        log("stage %s FAILED %s" % (ARGS["stage"], traceback.format_exc()))
        raise
    log("stage %s done" % ARGS["stage"])
elif __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    g = geometry()
    json.dump(to_json(g), open(os.path.join(OUT, "geometry.json"), "w"))
    lines = check(g)
    open(os.path.join(OUT, "check_2d.txt"), "w", encoding="utf8").write("\n".join(lines) + "\n")
    print("\n".join(lines))
    stages = [a for a in sys.argv[1:] if "=" not in a]
    extra = dict(a.split("=", 1) for a in sys.argv[1:] if "=" in a)
    if stages == ["all"]:
        stages = STAGES
    for st in stages:
        t = time.time()
        print("==", st)
        send(st, extra)
        print("   %.0f s" % (time.time() - t))
