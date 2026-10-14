import type React from "react";
import { Easing, interpolate, useCurrentFrame } from "remotion";
import { colors, fonts } from "./theme";

// The headline each scene leads with, top-left inside the safe area.
export const Caption: React.FC<{ readonly text: string }> = ({ text }) => {
  const frame = useCurrentFrame();
  return (
    <div
      style={{
        position: "absolute",
        left: 140,
        top: 110,
        fontFamily: fonts.display,
        fontWeight: 700,
        fontSize: 96,
        letterSpacing: "-0.01em",
        color: colors.ink,
        opacity: interpolate(frame, [0, 14], [0, 1], {
          extrapolateLeft: "clamp",
          extrapolateRight: "clamp",
          easing: Easing.bezier(0.16, 1, 0.3, 1),
        }),
        translate: interpolate(frame, [0, 18], ["0px 24px", "0px 0px"], {
          extrapolateLeft: "clamp",
          extrapolateRight: "clamp",
          easing: Easing.bezier(0.16, 1, 0.3, 1),
        }),
      }}
    >
      {text}
    </div>
  );
};
