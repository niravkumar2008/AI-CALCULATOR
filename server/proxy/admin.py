"""Admin tool for the proxy's database. Run on the server, next to app.py.

  python admin.py new-device "Nirav's board #1"   -> prints the device token ONCE (type it into the calculator: token)
  python admin.py devices                          -> every device, linked account, revoked?
  python admin.py revoke dev_1a2b3c                -> the token stops working at once
  python admin.py paid someone@example.com 2026-12-31   -> subscription paid until that date (manual / no Stripe)
  python admin.py tier someone@example.com pro      -> put an account on the pro (Opus) or base (Sonnet) tier
  python admin.py tiers                            -> the tiers: model, fallback, monthly cap, burst limits
  python admin.py account someone@example.com      -> subscription state, tier, solves this month
  python admin.py prompt                           -> the system prompt version the proxy sends
  python admin.py usage                            -> solves per account / device this month
  python admin.py firmware <firmware.bin> <version>  -> publish a firmware image for over-the-air updates, one per
                                                      board family (the version's part before "-"):
                                                      stage14-... = v14 e-paper (firmware-prototype/.pio/build/prototype/firmware.bin)
                                                      v15lcd-...  = v15 LCD (firmware-v15-lcd/.pio/build/v15lcd/firmware.bin)
                                                      version = kFirmwareVersion in that project's claude_client.h
  python admin.py firmware                         -> what is published now, per board family
"""
import sys
import time
from datetime import datetime, timezone

import config
import db
import firmware


def main(argv):
    db.init()
    if len(argv) < 2:
        print(__doc__)
        return 1
    cmd = argv[1]
    if cmd == "new-device":
        device_id, token = db.create_device(argv[2] if len(argv) > 2 else "")
        print(f"device id: {device_id}\ntoken:     {token}\n"
              "Type  token  in the calculator's Serial Monitor and paste it. It is not shown again.")
    elif cmd == "devices":
        for d in db.list_devices():
            print(f"{d['id']}  {d['label'] or '-':20s}  hw={d['hw_id'] or '-':18s}  account={d['email'] or '-'}"
                  f"{'  REVOKED' if d['revoked'] else ''}")
    elif cmd == "revoke" and len(argv) > 2:
        print("revoked" if db.revoke_device(argv[2]) else "no such device")
    elif cmd == "paid" and len(argv) > 3:
        until = datetime.strptime(argv[3], "%Y-%m-%d").replace(tzinfo=timezone.utc)
        n = db.set_paid_until(argv[2], until.timestamp() + 86399)
        print("updated" if n else "no such account (it is created when a calculator is linked)")
    elif cmd == "tier" and len(argv) > 3:
        try:
            n = db.set_tier(argv[2], argv[3])
        except ValueError as e:
            print(e)
            return 1
        print("updated" if n else "no such account (it is created when a calculator is linked)")
    elif cmd == "tiers":
        import policy
        import solve
        for tier in db.TIERS:
            model = policy.model_for(tier)
            chain = " -> ".join(solve.model_chain(model))
            print(f"{tier:5s} model {chain:40s} cap {policy.cap_for(tier)}/month")
        print(f"burst: {config.SOLVE_CONCURRENCY} at a time, {config.SOLVES_PER_MINUTE}/minute, "
              f"{config.SOLVES_PER_HOUR}/hour per calculator; trial {config.TRIAL_DAYS} days; "
              f"past-due grace {config.PAST_DUE_GRACE_DAYS} days; prompt cache {'on' if config.PROMPT_CACHE else 'off'}")
    elif cmd == "account" and len(argv) > 2:
        a = db.account_by_email(argv[2])
        if a is None:
            print("no such account")
            return 1
        fmt = lambda ts: time.strftime("%Y-%m-%d", time.gmtime(ts)) if ts else "-"  # noqa: E731
        print(f"{a['email']}: {db.subscription_state(a)} (billing status {a['sub_status'] or '-'}), tier {db.account_tier(a)}, "
              f"trial ends {fmt(a['trial_ends_at'])}, paid until {fmt(a['paid_until'])}, "
              f"solves this month {db.used('acct:' + str(a['id']), db.month())}")
    elif cmd == "prompt":
        import solve
        system, _, version = solve.server_prompt()
        print(f"{version}: {config.SYSTEM_PROMPT_FILE} ({len(system)} characters), source={config.PROMPT_SOURCE}")
    elif cmd == "firmware" and len(argv) > 3:
        try:
            m = firmware.publish(argv[2], argv[3])
        except (OSError, ValueError) as e:
            print(f"not published: {e}")
            return 1
        fam = firmware.family(m["version"])
        print(f"published {m['version']} for board family {fam} "
              f"({firmware.KNOWN_FAMILIES.get(fam, 'not a known board: no calculator will match it')}): "
              f"{m['size']} bytes, sha256 {m['sha256']}\n"
              f"in {config.FIRMWARE_DIR}. Only {fam} calculators are offered it; the other family's image is unchanged.\n"
              f"Calculators switched off on a cable install it on their next check.")
    elif cmd == "firmware":
        found = firmware.published()
        if not found:
            print("nothing published")
        for fam, m in found.items():
            print(f"{fam:8s} {m['version']}: {m['size']} bytes, sha256 {m['sha256']} ({m['file']})")
    elif cmd == "usage":
        with db.conn() as c:
            for r in c.execute("SELECT * FROM usage WHERE period=? ORDER BY count DESC", (db.month(),)):
                print(f"{r['subject']:20s} {r['count']}")
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
