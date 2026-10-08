# AI Calculator proxy

A small web server that sits between the calculators and Claude. **It holds the Claude API key; the
calculators never do.** Each calculator has its own device token instead.

```
calculator --(HTTPS, device token)--> proxy --(HTTPS, API key)--> Claude API
```

What it does for every AI solve:
1. Checks the device token (stored only as a hash; revocable).
2. Checks the calculator is linked to an account, and the account's subscription: first month free
   from linking, then paid ($15/month, set by the billing webhook). Optional free tier: `FREE_DAILY`.
3. Enforces fair use: `MONTHLY_CAP` (500) solves per account per calendar month. Only finished answers count.
4. Checks the request is a photo solve (1-2 photos, the calculator's JSON format), sets the model and limits,
   and forwards it. A stolen token can't be used for anything else.
5. Streams Claude's reply straight back, so the calculator reads exactly what the API sends.

Optional **Haiku-first routing** (`ROUTING=haiku-first`): tries `claude-haiku-4-5-20251001` first; if Haiku
reads the photo with confidence >= `HAIKU_MIN_CONFIDENCE` and nothing unclear, that answer is sent; otherwise
the main model (`claude-sonnet-5-5`) solves it. Cheaper for easy problems, a little slower for hard ones.

`REFUSAL_FALLBACK=1` (default) turns on the Claude API's server-side refusal fallback for the main model
(beta `server-side-fallback-2026-07-01`, `fallbacks: "default"`): if Sonnet 5.5's safety filter declines a
harmless homework photo, another model answers instead of the student seeing an error. Set it to 0 if your
account rejects the beta.

## Files

| File | What |
| --- | --- |
| `app.py` | the web server (FastAPI): `/v1/solve`, `/v1/pair/start`, `/v1/device/status`, `/v1/firmware`, `/link`, billing webhook |
| `policy.py` | who may solve: link, subscription, monthly cap, free tier (error messages shown on the calculator) |
| `solve.py` | checks the calculator's request, calls Claude (main model / Haiku first) |
| `db.py` | SQLite: devices, accounts, pairing codes, usage |
| `admin.py` | command line: new device tokens, revoke, mark paid, usage, publish firmware |
| `firmware.py` | over-the-air updates: the published image + manifest, `GET /v1/firmware` and `/v1/firmware/image` |
| `config.py` + `.env.example` | every setting |
| `test_proxy.py` | tests: `python -m unittest test_proxy -v` (no network, no key needed; the two HTTP tests need `fastapi` + `httpx`) |

## Run it on your PC (to try it)

```
cd server/proxy
python -m venv .venv
.venv\Scripts\activate            (Windows)      or   source .venv/bin/activate
pip install -r requirements.txt
copy .env.example .env             then edit .env: paste your API key after ANTHROPIC_API_KEY=
python admin.py new-device "board 1"
uvicorn app:app --port 8000
```
The calculator needs **HTTPS**, so for a real test use one of the hosted options below (or a tunnel such as
`cloudflared tunnel --url http://localhost:8000`, which gives you an https:// address).

## Deploy (pick one)

Any host that runs a Python web app with HTTPS works. Two easy ones:

- **Render / Railway / Fly.io:** create a new web service from this folder (the `Dockerfile` is ready).
  Add a persistent disk/volume mounted at `/data` (the database). Set the environment variables from
  `.env.example` in the host's dashboard (the API key goes there, not in git). Their https:// address is your
  `PUBLIC_URL`.
- **A small VPS:** `pip install -r requirements.txt`, run `uvicorn app:app --port 8000` as a service, put
  Caddy in front (`caddy reverse-proxy --from calc.yourdomain.com --to localhost:8000`, automatic HTTPS).

The calculator trusts Let's Encrypt, Google Trust Services, DigiCert and Amazon certificates (the same roots as
for api.anthropic.com), which covers all of the hosts above.

## Set up a calculator

1. On the server: `python admin.py new-device "Nirav board 1"` -> copy the token (shown once). On a hosted
   service run it in the host's shell (Render: the service's **Shell** tab; Fly.io: `fly ssh console`;
   Railway: `railway run` / its shell), so it writes to the database on the `/data` volume. Or, with
   `ADMIN_TOKEN` set: `curl -X POST "https://your-proxy-address/v1/admin/devices?label=board%201" -H "x-admin-token: <ADMIN_TOKEN>"`.
2. Calculator on USB, Serial Monitor: `proxy` -> paste `https://your-proxy-address`;
   `token` -> paste the token; `wifi` -> hotspot name and password. Or without a PC: SHIFT MODE (SETUP),
   6, = on the calculator (serial: `setup`) opens the phone setup page (`firmware-prototype/README.md` §2).
   This is step 10 of the Power-Up guide (`hardware/bringup_guide.html`).
3. Link it: `pair` prints a 6-letter code (an unlinked calculator also shows the code when you try an AI
   solve). Open `https://your-proxy-address/link`, type the code and your e-mail. The free month starts.
4. `account` on the calculator shows the subscription and the solves used this month.

## Firmware updates over the air

The calculators update themselves from this server (firmware `src/ota.cpp`): switched off on a cable, a
calculator asks `GET /v1/firmware` (with its device token and its version in `x-firmware`), and when the
answer is `"update": true` it downloads `/v1/firmware/image`, checks the SHA-256 from the manifest and
installs it into its second app slot (rollback if the new one crashes).

```
python admin.py firmware ../../firmware-prototype/.pio/build/prototype/firmware.bin stage14-2026.10.07
python admin.py firmware        (what is published now)
```
The image and `firmware.json` live in `FIRMWARE_DIR` (default `./firmware`, git-ignored; `/data/firmware` in
the Docker image, on the same volume as the database). Only one image is published at a time; a different
version string (newer or older) is offered to every calculator that reports another one. The endpoints
need a device token like every other calculator call, and the manifest's size and hash are what the
calculator trusts (the transport is TLS to the roots pinned in the firmware).

## Subscriptions

Until a payment provider is wired up: `python admin.py paid someone@example.com 2026-12-31`.
With Stripe (or similar): point its webhook at a tiny handler that calls
`POST /v1/billing/webhook` with header `x-billing-secret: <BILLING_SECRET>` and body
`{"email": "...", "paid_until": <unix time>}`.

The price is not final, so the knobs are settings: `TRIAL_DAYS`, `MONTHLY_CAP`, `FREE_DAILY`
(N free solves a day without a subscription; `FREE_DAILY_UNLINKED=1` also before linking).

## Security notes

- The API key exists only in the server's environment. `api_key.txt` in the repo root is for the PC
  simulator and is git-ignored; don't copy it anywhere.
- Device tokens are random 40-character strings, stored as SHA-256 hashes. `python admin.py revoke dev_...`
  stops a lost calculator at once.
- The `/link` page is deliberately minimal (code + e-mail). Before selling, put it behind a real sign-in
  (e-mail magic link or the shop's account) so nobody can link a calculator to someone else's e-mail.
- Keep a spend limit on the Anthropic Console as a last line of defence.
