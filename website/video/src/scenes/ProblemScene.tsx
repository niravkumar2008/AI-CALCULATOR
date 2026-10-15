import type React from "react";
import {
  AbsoluteFill,
  Easing,
  Interactive,
  interpolate,
  useCurrentFrame,
  type InteractivitySchema,
} from "remotion";
import { Caption } from "../Caption";
import { colors, demo, fonts } from "../theme";

const ease = Easing.bezier(0.16, 1, 0.3, 1);

// A homework problem on lined paper; viewfinder brackets close in on it.
const ProblemSceneInner: React.FC<{
  readonly caption: string;
  readonly style?: React.CSSProperties;
}> = ({ caption, style }) => {
  const frame = useCurrentFrame();
  const bracket = interpolate(frame, [24, 54], [90, 0], {
    extrapolateLeft: "clamp",
    extrapolateRight: "clamp",
    easing: ease,
  });
  const corner = (pos: React.CSSProperties, sides: string) => (
    <div
      style={{
        position: "absolute",
        width: 70,
        height: 70,
        borderColor: colors.accent,
        borderStyle: "solid",
        borderWidth: sides,
        opacity: interpolate(frame, [20, 34], [0, 1], {
          extrapolateLeft: "clamp",
          extrapolateRight: "clamp",
        }),
        ...pos,
      }}
    />
  );

  return (
    <AbsoluteFill
      style={{ backgroundColor: colors.bg, fontFamily: fonts.body, ...style }}
    >
      <Caption text={caption} />
      <AbsoluteFill
        style={{
          justifyContent: "center",
          alignItems: "center",
          paddingTop: 120,
        }}
      >
        <div
          style={{
            position: "relative",
            width: 1000,
            height: 420,
            borderRadius: 10,
            backgroundColor: colors.paper,
            backgroundImage: `repeating-linear-gradient(transparent 0 58px, ${colors.paperLine} 58px 60px)`,
            boxShadow: "0 30px 80px rgba(0,0,0,.45)",
            rotate: "-2deg",
            translate: interpolate(frame, [0, 22], ["0px 60px", "0px 0px"], {
              extrapolateLeft: "clamp",
              extrapolateRight: "clamp",
              easing: ease,
            }),
            opacity: interpolate(frame, [0, 12], [0, 1], {
              extrapolateLeft: "clamp",
              extrapolateRight: "clamp",
            }),
          }}
        >
          <div
            style={{
              position: "absolute",
              left: 70,
              top: 40,
              fontFamily: fonts.hand,
              fontSize: 44,
              color: "#4b5563",
            }}
          >
            Q7. Evaluate
          </div>
          <div
            style={{
              position: "absolute",
              inset: 0,
              display: "flex",
              justifyContent: "center",
              alignItems: "center",
              paddingTop: 40,
              fontFamily: fonts.hand,
              fontSize: 110,
              color: "#1f2937",
            }}
          >
            {demo.problem}
          </div>
          {corner({ left: -30 - bracket, top: -30 - bracket }, "8px 0 0 8px")}
          {corner({ right: -30 - bracket, top: -30 - bracket }, "8px 8px 0 0")}
          {corner(
            { left: -30 - bracket, bottom: -30 - bracket },
            "0 0 8px 8px",
          )}
          {corner(
            { right: -30 - bracket, bottom: -30 - bracket },
            "0 8px 8px 0",
          )}
        </div>
      </AbsoluteFill>
    </AbsoluteFill>
  );
};

const schema = {
  caption: {
    type: "text-content",
    default: "Snap a problem.",
    description: "Headline",
  },
} as const satisfies InteractivitySchema;

export const ProblemScene = Interactive.withSchema({
  Component: ProblemSceneInner,
  componentName: "<ProblemScene>",
  schema,
  wrapInSequence: true,
});
