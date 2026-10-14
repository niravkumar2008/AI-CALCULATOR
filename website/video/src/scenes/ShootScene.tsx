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
import { Screen } from "../Screen";
import { colors, demo, fonts } from "../theme";

const PRESS = 36; // frame the = key goes down

// The live viewfinder on the calculator screen, then the = key takes the shot.
const ShootSceneInner: React.FC<{
  readonly caption: string;
  readonly style?: React.CSSProperties;
}> = ({ caption, style }) => {
  const frame = useCurrentFrame();
  const pressed = interpolate(
    frame,
    [PRESS - 4, PRESS, PRESS + 8],
    [1, 0.9, 1],
    {
      extrapolateLeft: "clamp",
      extrapolateRight: "clamp",
    },
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
          paddingTop: 150,
          gap: 44,
        }}
      >
        <Screen status="AI SOLVE  ·  VIEWFINDER" style={{ width: 1000 }}>
          {/* Dithered preview: the e-paper shows the camera image as dots. */}
          <div
            style={{
              position: "absolute",
              inset: 0,
              display: "flex",
              justifyContent: "center",
              alignItems: "center",
              fontFamily: fonts.hand,
              fontSize: 96,
              color: colors.screenInk,
              filter: "blur(1.2px)",
              rotate: "-2deg",
              translate: `${Math.sin(frame / 7) * 3}px ${Math.cos(frame / 9) * 2}px`,
              backgroundImage:
                "radial-gradient(rgba(29,35,31,.18) 1px, transparent 1.4px)",
              backgroundSize: "6px 6px",
            }}
          >
            {demo.problem}
          </div>
          {/* Crosshair brackets drawn by the firmware. */}
          <div
            style={{
              position: "absolute",
              inset: "40px 90px",
              border: "4px dashed rgba(29,35,31,.6)",
              borderRadius: 8,
            }}
          />
          <div
            style={{
              position: "absolute",
              inset: 0,
              background: "white",
              opacity: interpolate(
                frame,
                [PRESS, PRESS + 2, PRESS + 14],
                [0, 0.9, 0],
                {
                  extrapolateLeft: "clamp",
                  extrapolateRight: "clamp",
                  easing: Easing.out(Easing.quad),
                },
              ),
            }}
          />
        </Screen>
        <div
          style={{
            width: 200,
            height: 104,
            borderRadius: 22,
            background: "#4a4f58",
            boxShadow: `0 ${10 * pressed}px 0 #2b2e33`,
            translate: `0px ${(1 - pressed) * 60}px`,
            display: "flex",
            justifyContent: "center",
            alignItems: "center",
            fontFamily: fonts.display,
            fontWeight: 700,
            fontSize: 70,
            color: colors.ink,
          }}
        >
          =
        </div>
      </AbsoluteFill>
    </AbsoluteFill>
  );
};

const schema = {
  caption: {
    type: "text-content",
    default: "Press = to shoot.",
    description: "Headline",
  },
} as const satisfies InteractivitySchema;

export const ShootScene = Interactive.withSchema({
  Component: ShootSceneInner,
  componentName: "<ShootScene>",
  schema,
  wrapInSequence: true,
});
