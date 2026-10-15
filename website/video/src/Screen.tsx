import type React from "react";
import { colors, fonts } from "./theme";

// The calculator display: a dark bezel around a grey-green screen.
export const Screen: React.FC<{
  readonly children: React.ReactNode;
  readonly status?: string;
  readonly style?: React.CSSProperties;
}> = ({ children, status = "AI", style }) => {
  return (
    <div
      style={{
        width: 1120,
        padding: 36,
        borderRadius: 44,
        background: colors.bezel,
        boxShadow:
          "0 40px 90px rgba(0,0,0,.55), inset 0 2px 0 rgba(255,255,255,.06)",
        ...style,
      }}
    >
      <div
        style={{
          position: "relative",
          height: 520,
          borderRadius: 14,
          background: colors.screen,
          color: colors.screenInk,
          fontFamily: fonts.mono,
          overflow: "hidden",
          boxShadow: "inset 0 6px 18px rgba(0,0,0,.25)",
        }}
      >
        <div
          style={{
            display: "flex",
            justifyContent: "flex-end",
            padding: "14px 26px",
            fontSize: 30,
            borderBottom: "3px solid rgba(29,35,31,.75)",
          }}
        >
          {status}
        </div>
        <div style={{ position: "absolute", inset: "70px 0 0 0" }}>
          {children}
        </div>
      </div>
    </div>
  );
};
