# Brand name ideas

**Chosen 2026-10-11: Proofline** (company name: Proofline Corporation). The site now says Proofline;
the domain and contact email are still the placeholders `proofline.example` / `hello@proofline.example`.
Still to do: a trademark search, and registering the corporation before using "Corporation" for real.

The list below is the original shortlist (2026-10-10), kept for the record. Until 2026-10-11 the site
used `[Brand]` as a placeholder. These were first ideas, **not cleared**:
I only ran quick web searches on 2026-10-10 to rule out Casio names and obvious existing
brands. A real trademark search (USPTO, plus the countries you'll sell in) is still for your lawyer.

Rules I followed: no Casio name or mark (Casio, ClassWiz, V.P.A.M., Natural Textbook Display,
anything "fx-"), nothing that sounds like cheating or stealth, works for a 16-year-old and for
the parent paying, short enough for the calculator's screen and the shell.

| # | Name | Why it works | What the quick check found | Risk |
|---|---|---|---|---|
| 1 | **Proofline** | "Proof" as in showing your working; a line of steps. Says *honest tutor* without saying "AI". | No product called Proofline found; similar "Proof…" names exist in print-proofing software (ProofHQ, ProofStuff). | Low to medium |
| 2 | **Kitestep** | Not about calculators at all: a kite climbs one step at a time. Friendly, distinctive, easy to spell. | Nothing found under that name. | Low |
| 3 | **Orrin** | A short coined name, like a person or a place. Works as a logo word and fits on the screen. | No app, calculator or edtech brand found. It is a given name, so check for small businesses. | Low |
| 4 | **Ninefold** | Abstract, confident, a little mathematical (nine, fold) without being nerdy. | No current brand found; an Australian cloud company used the name until about 2016 (from memory, please check). | Medium |
| 5 | **Wrenly** | Small, quick, clever bird; soft and warm for parents. | No "Wrenly" found, but "Wren" is crowded (a climate app, a UK kitchen brand). | Medium |

Ruled out during the check: **Steplight** (a Ring smart-light product), **Cairn** (an existing
learning platform and Cairn.info), **Wayfinder** (an existing K-12 edtech company), **Tessel**
(tessel.io, and an older hardware board of that name), **Sumwise** (financial-modelling software).

My pick: **Kitestep** for warmth and distinctiveness, **Proofline** if you want the name itself
to carry the "shows the steps" promise.

## When you choose

**Update 2026-10-11:** steps 1 and 2 are done (the site and `make_embed.py` say Proofline; `make_embed.py` is
retired for the site anyway). Step 3 is **not done**: the video still says "AI·CALC". Step 5: the placeholder is
now `proofline.example`, so replace that (not `aicalc.example`) once a domain is bought.

1. Search and replace `[Brand]` in `web/site/` (HTML, `site.webmanifest`) and in the legal pages.
2. Set `BRAND` in `sim/web/make_embed.py`, then run `python sim/web/make_page.py && python sim/web/make_embed.py`.
3. Change `brand="AI·CALC"` in `website/video/src/AiSolveVideo.tsx` and re-render the video (see `website/README.md`).
4. Re-make `web/site/assets/og-image.jpg` if you want the name on it (it has no name today).
5. Buy the domain and replace `aicalc.example` (canonical, Open Graph, sitemap, robots, contact email).
