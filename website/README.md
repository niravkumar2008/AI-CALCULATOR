# Website assets

Everything the launch site will use. The site itself isn't built yet (see `../WEBSITE_PROMPT.md`).

| Folder / file | What it is | Status |
|---|---|---|
| `simulator/index.html` | The real calculator engine, calculator only, for an `<iframe>` "Try it" section. AI Solve (`MODE 4 =`) cycles through 3 demo answers; no API key field and no network calls. | Done |
| `assets/video/ai-solve.mp4` | 12.8 s, 1080p, silent: snap a problem → press = → steps appear → "Learn every step." | Done (brand is the placeholder "AI·CALC") |
| `assets/video/ai-solve-poster.jpg` | The poster frame (steps on screen) shown before the video plays. | Done |
| `video/` | The Remotion project that makes the video. | Done |
| `renders/studio_renders.py` | Studio shots from the Fusion final assembly: hero front, hero back, straight-on front, low angle, exploded. | Script ready; needs Fusion open |
| `PHOTO_SHOT_LIST.md` | How to photograph the first prototype with a phone. | For when boards arrive |

## Rebuild

- **Simulator embed** (after any simulator change): `python sim/web/make_page.py` then `python sim/web/make_embed.py`.
- **Video:** `cd website/video`, `npm run dev` to edit in Remotion Studio, then
  `npx remotion render AiSolve out/ai-solve.mp4 --codec=h264 --crf=20` and copy it to `assets/video/`.
  To change the brand name, edit `brand="AI·CALC"` in `src/AiSolveVideo.tsx` (or in Studio).
- **Studio renders:** open `hardware/enclosure/final_assembly/ai_calculator_final_assembly.f3d` in Fusion, run the FusionMCPBridge add-in, then
  `python hardware/enclosure/fusion_run.py website/renders/studio_renders.py` (ray-traced, minutes per shot)
  or add `mode=viewport` for quick transparent-background images. Output: `website/renders/out/`.

Remotion is free for individuals and companies of up to 3 people; check its license if the business grows past that.
