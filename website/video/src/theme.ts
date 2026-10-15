import { loadFont as loadBarlow } from "@remotion/google-fonts/Barlow";
import { loadFont as loadCondensed } from "@remotion/google-fonts/BarlowSemiCondensed";
import { loadFont as loadMono } from "@remotion/google-fonts/IBMPlexMono";
import { loadFont as loadHand } from "@remotion/google-fonts/Caveat";

// Same type family as the simulator, so the video and the "Try it" section match.
export const fonts = {
  body: loadBarlow("normal", { weights: ["500", "600"], subsets: ["latin"] })
    .fontFamily,
  display: loadCondensed("normal", {
    weights: ["600", "700"],
    subsets: ["latin"],
  }).fontFamily,
  mono: loadMono("normal", { weights: ["500"], subsets: ["latin"] }).fontFamily,
  hand: loadHand("normal", { weights: ["600"], subsets: ["latin"] }).fontFamily,
};

export const colors = {
  bg: "#10141c", // the navy of the back cover, darkened
  ink: "#f2f3ef",
  muted: "#9aa3b2",
  accent: "#e07a4f", // the orange DEL / AC keys
  bezel: "#1b1d21",
  screen: "#c9d1c3", // grey-green display
  screenInk: "#1d231f",
  paper: "#f7f4ea",
  paperLine: "#c7d3e6",
};

// The arithmetic demo reply from the simulator (sim/web/page.html, DEMOS[0]).
export const demo = {
  problem: "3/4 + 2⁵ × sin 30°",
  answer: "16.75 (= 67/4)",
  steps: ["2⁵ = 32 and sin 30° = 0.5", "32 × 0.5 = 16", "3/4 + 16 = 16.75"],
};
