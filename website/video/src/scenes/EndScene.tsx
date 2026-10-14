import type React from "react";
import {
  AbsoluteFill,
  Easing,
  Interactive,
  interpolate,
  useCurrentFrame,
  type InteractivitySchema,
} from "remotion";
import { colors, fonts } from "../theme";

const ease = Easing.bezier(0.16, 1, 0.3, 1);

// End card: the promise and the brand. The brand is Proofline.
const EndSceneInner: React.FC<{
  readonly headline: string;
  readonly brand: string;
  readonly style?: React.CSSProperties;
}> = ({ headline, brand, style }) => {
  const frame = useCurrentFrame();
  const fade = (start: number) => ({
    opacity: interpolate(frame, [start, start + 12], [0, 1], {
      extrapolateLeft: "clamp",
      extrapolateRight: "clamp",
    }),
    translate: interpolate(
      frame,
      [start, start + 18],
      ["0px 20px", "0px 0px"],
      {
        extrapolateLeft: "clamp",
        extrapolateRight: "clamp",
        easing: ease,
      },
    ),
  });

  return (
    <AbsoluteFill
      style={{
        backgroundColor: colors.bg,
        justifyContent: "center",
        alignItems: "center",
        gap: 36,
        fontFamily: fonts.display,
        color: colors.ink,
        ...style,
      }}
    >
      <div
        style={{
          fontSize: 130,
          fontWeight: 700,
          letterSpacing: "-0.015em",
          ...fade(0),
        }}
      >
        {headline}
      </div>
      <div
        style={{
          fontSize: 52,
          fontWeight: 600,
          letterSpacing: ".2em",
          color: colors.accent,
          ...fade(14),
        }}
      >
        {brand}
      </div>
    </AbsoluteFill>
  );
};

const schema = {
  headline: {
    type: "text-content",
    default: "Learn every step.",
    description: "Headline",
  },
  brand: {
    type: "text-content",
    default: "Proofline",
    description: "Brand name",
  },
} as const satisfies InteractivitySchema;

export const EndScene = Interactive.withSchema({
  Component: EndSceneInner,
  componentName: "<EndScene>",
  schema,
  wrapInSequence: true,
});
