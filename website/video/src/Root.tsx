import { Composition, Folder } from "remotion";
import { AiSolveVideo } from "./AiSolveVideo";
import { EndScene } from "./scenes/EndScene";
import { ProblemScene } from "./scenes/ProblemScene";
import { ShootScene } from "./scenes/ShootScene";
import { SolveScene } from "./scenes/SolveScene";

export const RemotionRoot: React.FC = () => {
  return (
    <>
      <Folder name="Scenes">
        <Composition
          id="Problem"
          component={ProblemScene}
          width={1920}
          height={1080}
          fps={30}
          durationInFrames={90}
          defaultProps={{ caption: "Snap a problem." }}
        />
        <Composition
          id="Shoot"
          component={ShootScene}
          width={1920}
          height={1080}
          fps={30}
          durationInFrames={75}
          defaultProps={{ caption: "Press = to shoot." }}
        />
        <Composition
          id="Solve"
          component={SolveScene}
          width={1920}
          height={1080}
          fps={30}
          durationInFrames={165}
          defaultProps={{ caption: "See every step." }}
        />
        <Composition
          id="End"
          component={EndScene}
          width={1920}
          height={1080}
          fps={30}
          durationInFrames={90}
          defaultProps={{ headline: "Learn every step.", brand: "Proofline" }}
        />
      </Folder>
      <Composition
        id="AiSolve"
        component={AiSolveVideo}
        width={1920}
        height={1080}
        fps={30}
        durationInFrames={384}
      />
    </>
  );
};
