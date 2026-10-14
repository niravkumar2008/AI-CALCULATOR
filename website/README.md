# Website assets

Source assets for the launch site.

**Update 2026-10-11:** the site is built (29 pages, `web/site/`; see `web/SITE_README.md`), branded
**Proofline** on the placeholder domain `proofline.example`, and not deployed. (`WEBSITE_PROMPT.md`, the
original brief, is not in this worktree; the brief the pages follow now is `web/site-src/PAGE_BRIEF.md`.)

| Folder / file | What it is | Status |
|---|---|---|
| `simulator/index.html` | The e-paper calculator engine, calculator only, for an `<iframe>` "Try it" section. AI Solve (`MODE 4 =`) cycles through 3 demo answers; no API key field and no network calls. | Retired for the site: `/try/` now uses the v15 firmware's own simulator (see `web/SITE_README.md`) |
| `assets/video/ai-solve.mp4` | 12.8 s, 1080p, silent: snap a problem → press = → steps appear → "Learn every step." | Done; re-rendered 2026-10-11 with the end card brand **Proofline** (also copied to `web/site/assets/video/`). The calculator screen in the video is still the grey-green e-paper-era style, not the v15 colour screen. |
| `assets/video/ai-solve-poster.jpg` | The poster frame (steps on screen, video frame 254) shown before the video plays. The site's 960/1600 jpg/webp posters are made from the same frame. | Done (regenerated 2026-10-11) |
| `video/` | The Remotion project that makes the video. | Done |
| `renders/studio_renders.py` | Studio shots from the Fusion final assembly: hero front, hero back, straight-on front, low angle, exploded. | Script ready; needs Fusion open |
| `PHOTO_SHOT_LIST.md` | How to photograph the first prototype with a phone. | For when boards arrive |

## Rebuild

- **Simulator embed** (after any simulator change): `python sim/web/make_page.py` then `python sim/web/make_embed.py`.
- **Video:** `cd website/video`, `npm run dev` to edit in Remotion Studio, then
  `npx remotion render AiSolve out/ai-solve.mp4 --codec=h264 --crf=20` and copy it to `assets/video/`.
  To change the brand name, edit `brand="Proofline"` in `src/AiSolveVideo.tsx` (and the defaults in `src/Root.tsx`
  and `src/scenes/EndScene.tsx`), or change it in Studio. Posters: `npx remotion still AiSolve poster.png --frame=254`,
  then save 960×540 and 1600×900 jpg/webp copies with Pillow.
- **Studio renders:** open `hardware/enclosure/final_assembly/ai_calculator_final_assembly.f3d` in Fusion, run the FusionMCPBridge add-in, then
  `python hardware/enclosure/fusion_run.py website/renders/studio_renders.py` (ray-traced, minutes per shot)
  or add `mode=viewport` for quick transparent-background images. Output: `website/renders/out/`.

Remotion is free for individuals and companies of up to 3 people; check its license if the business grows past that.
