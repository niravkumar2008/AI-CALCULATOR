import type React from "react";
import {
  AbsoluteFill,
  Easing,
  Interactive,
  interpolate,
  useCurrentFrame,
  useVideoConfig,
  type InteractivitySchema,
} from "remotion";
import { Caption } from "../Caption";
import { Screen } from "../Screen";
import { colors, demo, fonts } from "../theme";

const ease = Easing.bezier(0.16, 1, 0.3, 1);

// The answer, then each step appears in turn, as the calculator shows them.
const SolveSceneInner: React.FC<{
  readonly caption: string;
  readonly style?: React.CSSProperties;
}> = ({ caption, style }) => {
  const frame = useCurrentFrame();
  const { fps } = useVideoConfig();

  const reveal = (start: number): React.CSSProperties => ({
    opacity: interpolate(frame, [start, start + 10], [0, 1], {
      extrapolateLeft: "clamp",
      extrapolateRight: "clamp",
    }),
    translate: interpolate(
      frame,
      [start, start + 14],
      ["0px 18px", "0px 0px"],
      {
        extrapolateLeft: "clamp",
        extrapolateRight: "clamp",
        easing: ease,
      },
    ),
  });

  return (
    <AbsoluteFill
      style={{ backgroundColor: colors.bg, fontFamily: fonts.body, ...style }}
    >
      <Caption text={caption} />
      <AbsoluteFill
        style={{
          justifyContent: "center",
          alignItems: "center",
          paddingTop: 150,
        }}
      >
        <Screen status="AI SOLVE">
          <div style={{ padding: "18px 40px", display: "grid", gap: 18 }}>
            <div
              style={{
                fontSize: 30,
                letterSpacing: ".08em",
                ...reveal(0.2 * fps),
              }}
            >
              ANSWER
            </div>
            <div
              style={{ fontSize: 68, fontWeight: 500, ...reveal(0.4 * fps) }}
            >
              {demo.answer}
            </div>
            <div
              style={{
                height: 3,
                background: "rgba(29,35,31,.5)",
                ...reveal(1 * fps),
              }}
            />
            <div style={{ fontSize: 44, ...reveal(1.3 * fps) }}>
              1. {demo.steps[0]}
            </div>
            <div style={{ fontSize: 44, ...reveal(2.3 * fps) }}>
              2. {demo.steps[1]}
            </div>
            <div style={{ fontSize: 44, ...reveal(3.3 * fps) }}>
              3. {demo.steps[2]}
            </div>
          </div>
        </Screen>
      </AbsoluteFill>
    </AbsoluteFill>
  );
};

const schema = {
  caption: {
    type: "text-content",
    default: "See every step.",
    description: "Headline",
  },
} as const satisfies InteractivitySchema;

export const SolveScene = Interactive.withSchema({
  Component: SolveSceneInner,
  componentName: "<SolveScene>",
  schema,
  wrapInSequence: true,
});
