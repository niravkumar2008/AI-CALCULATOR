import { linearTiming, TransitionSeries } from "@remotion/transitions";
import { fade } from "@remotion/transitions/fade";
import { useVideoConfig } from "remotion";
import { EndScene } from "./scenes/EndScene";
import { ProblemScene } from "./scenes/ProblemScene";
import { ShootScene } from "./scenes/ShootScene";
import { SolveScene } from "./scenes/SolveScene";

// The AI Solve flow for the website: point, press =, read the steps.
// Scenes 90 + 75 + 165 + 90 = 420 frames, minus three 12-frame fades = 384.
export const AiSolveVideo = () => {
  const { fps } = useVideoConfig();

  return (
    <TransitionSeries>
      <TransitionSeries.Sequence
        name="Problem"
        durationInFrames={90}
        premountFor={fps}
      >
        <ProblemScene caption="Snap a problem." />
      </TransitionSeries.Sequence>
      <TransitionSeries.Transition
        presentation={fade()}
        timing={linearTiming({ durationInFrames: 12 })}
      />
      <TransitionSeries.Sequence
        name="Shoot"
        durationInFrames={75}
        premountFor={fps}
      >
        <ShootScene caption="Press = to shoot." />
      </TransitionSeries.Sequence>
      <TransitionSeries.Transition
        presentation={fade()}
        timing={linearTiming({ durationInFrames: 12 })}
      />
      <TransitionSeries.Sequence
        name="Solve"
        durationInFrames={165}
        premountFor={fps}
      >
        <SolveScene caption="See every step." />
      </TransitionSeries.Sequence>
      <TransitionSeries.Transition
        presentation={fade()}
        timing={linearTiming({ durationInFrames: 12 })}
      />
      <TransitionSeries.Sequence
        name="End"
        durationInFrames={90}
        premountFor={fps}
      >
        <EndScene headline="Learn every step." brand="Proofline" />
      </TransitionSeries.Sequence>
    </TransitionSeries>
  );
};
