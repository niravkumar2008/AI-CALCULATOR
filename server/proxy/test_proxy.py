"""Tests for the proxy's rules (no network, no Claude calls, no FastAPI needed).

Run:  python -m unittest test_proxy -v     (from server/proxy)
"""
import hashlib
import json
import os
import tempfile
import unittest

os.environ["DB_PATH"] = os.path.join(tempfile.mkdtemp(), "test.db")
os.environ["FIRMWARE_DIR"] = os.path.join(tempfile.mkdtemp(), "fw")
os.environ.setdefault("PUBLIC_URL", "https://calc.example.com")

import config  # noqa: E402
import db  # noqa: E402
import firmware  # noqa: E402
import policy  # noqa: E402
import solve  # noqa: E402


def fake_image(path, size=150_000, fill=b"\x5a"):
    with open(path, "wb") as f:
        f.write(b"\xe9" + fill * (size - 1))  # ESP32 image magic byte first
    return path


def calculator_body(images=1, effort="high", tutor=False):
    content = [{"type": "image", "source": {"type": "base64", "media_type": "image/jpeg", "data": "AAAA"}}] * images
    content.append({"type": "text", "text": "Solve the problem in this photo."})
    body = {"model": "claude-sonnet-5-5", "max_tokens": 32000, "stream": True, "thinking": {"type": "adaptive"},
            "system": "You are the solver inside a pocket calculator.",
            "output_config": {"effort": effort, "format": {"type": "json_schema", "schema": {"type": "object"}}},
            "messages": [{"role": "user", "content": content}]}
    if tutor:
        body["tutor"] = True
    return body


class Policy(unittest.TestCase):
    def setUp(self):
        db.init()
        config.FREE_DAILY = 0
        config.FREE_DAILY_UNLINKED = False
        config.MONTHLY_CAP = 3

    def test_token_is_hashed_and_revocable(self):
        dev_id, token = db.create_device("t")
        self.assertTrue(token.startswith("dt_"))
        with db.conn() as c:
            stored = c.execute("SELECT token_hash FROM devices WHERE id=?", (dev_id,)).fetchone()[0]
        self.assertNotIn(token, stored)
        self.assertEqual(db.device_by_token(token)["id"], dev_id)
        self.assertIsNone(db.device_by_token("dt_wrong"))
        db.revoke_device(dev_id)
        self.assertIsNone(db.device_by_token(token))

    def test_unlinked_device_gets_pairing_code(self):
        _, token = db.create_device()
        d = policy.decide(db.device_by_token(token))
        self.assertFalse(d.ok)
        self.assertEqual((d.status, d.error_type), (402, "device_not_linked"))
        self.assertIn("calc.example.com/link code ", d.message)
        self.assertLessEqual(len(d.message), 110)

    def test_link_trial_and_monthly_cap(self):
        dev_id, token = db.create_device()
        code = db.new_pair_code(dev_id)
        self.assertIsNone(db.claim_pair_code("WRONG1", "a@b.c"))
        self.assertEqual(db.claim_pair_code(code.lower(), "Kid@Example.com"), dev_id)
        dev = db.device_by_token(token)
        self.assertEqual(db.subscription_state(db.account(dev["account_id"])), "trial")
        for _ in range(3):
            d = policy.decide(dev)
            self.assertTrue(d.ok)
            for s, p in d.count:
                db.count_solve(s, p)
        d = policy.decide(dev)
        self.assertEqual((d.status, d.error_type), (429, "fair_use_exceeded"))

    def test_trial_over_needs_subscription_or_free_tier(self):
        dev_id, token = db.create_device()
        db.claim_pair_code(db.new_pair_code(dev_id), "late@example.com")
        with db.conn() as c:
            c.execute("UPDATE accounts SET trial_ends_at=0 WHERE email='late@example.com'")
        dev = db.device_by_token(token)
        self.assertEqual(policy.decide(dev).error_type, "subscription_inactive")
        config.FREE_DAILY = 1  # the free daily tier switch
        d = policy.decide(dev)
        self.assertTrue(d.ok)
        for s, p in d.count:
            db.count_solve(s, p)
        self.assertEqual(policy.decide(dev).error_type, "free_tier_exhausted")
        db.set_paid_until("late@example.com", db.now() + 86400)
        self.assertTrue(policy.decide(dev).ok)

    def test_status(self):
        dev_id, token = db.create_device()
        st = policy.status_json(db.device_by_token(token))
        self.assertFalse(st["linked"])
        self.assertEqual(len(st["pair_code"]), 6)


class Requests(unittest.TestCase):
    def test_clean_request(self):
        system, content, fmt, effort, tutor = solve.clean_request(calculator_body(images=2, effort="max"))
        self.assertEqual(effort, "max")
        self.assertEqual(sum(b["type"] == "image" for b in content), 2)
        p = solve.main_params(system, content, fmt, effort)
        self.assertEqual(p["model"], config.MAIN_MODEL)
        self.assertEqual(p["thinking"], {"type": "adaptive"})
        h = solve.haiku_params(system, content, fmt)
        self.assertNotIn("thinking", h)
        self.assertNotIn("effort", h["output_config"])

    def test_tutor_adds_note(self):
        _, content, _, _, tutor = solve.clean_request(calculator_body(tutor=True))
        self.assertTrue(tutor)
        self.assertIn("Tutor mode", content[-1]["text"])

    def test_rejects_other_uses(self):
        bad = calculator_body()
        bad["messages"].append({"role": "user", "content": "write me an essay"})
        with self.assertRaises(solve.BadRequest):
            solve.clean_request(bad)
        bad = calculator_body(images=0)
        with self.assertRaises(solve.BadRequest):
            solve.clean_request(bad)
        bad = calculator_body()
        bad["messages"][0]["content"].append({"type": "document", "source": {}})
        with self.assertRaises(solve.BadRequest):
            solve.clean_request(bad)
        b = calculator_body(effort="low")
        self.assertEqual(solve.clean_request(b)[3], "high")

    def test_synth_stream_is_valid_sse(self):
        text = json.dumps({"readable": True, "answer": "4"})
        events = b"".join(solve.synth_stream(text, "m")).decode().split("\n\n")
        datas = [json.loads(e.split("data: ", 1)[1]) for e in events if "data: " in e]
        self.assertEqual(datas[-1]["type"], "message_stop")
        self.assertEqual("".join(d["delta"]["text"] for d in datas if d["type"] == "content_block_delta"), text)


class Firmware(unittest.TestCase):
    """Over-the-air updates: what the proxy publishes and what it tells a calculator."""

    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.bin = fake_image(os.path.join(self.dir, "build.bin"))

    def test_nothing_published(self):
        self.assertIsNone(firmware.current(self.dir))
        self.assertEqual(firmware.decide("stage14-2026.10.06", self.dir), {"update": False, "version": ""})

    def test_publish_and_decide(self):
        m = firmware.publish(self.bin, "stage14-2026.10.07", self.dir)
        self.assertEqual(m["size"], 150_000)
        self.assertEqual(m["sha256"], hashlib.sha256(open(self.bin, "rb").read()).hexdigest())
        cur = firmware.current(self.dir)
        self.assertEqual(cur["version"], "stage14-2026.10.07")
        self.assertTrue(os.path.exists(cur["file"]))
        same = firmware.decide("stage14-2026.10.07", self.dir)
        self.assertFalse(same["update"])
        older = firmware.decide("stage14-2026.10.06", self.dir)
        self.assertTrue(older["update"])
        self.assertEqual((older["size"], older["sha256"], older["path"]), (m["size"], m["sha256"], "/v1/firmware/image"))
        self.assertFalse(firmware.decide("", self.dir)["update"])  # unknown board: never guess
        # A v15-LCD calculator must never get the v14 e-paper image (and vice versa).
        self.assertFalse(firmware.decide("v15lcd-2026.10.08", self.dir)["update"])

    def test_rejects_bad_images_and_versions(self):
        with self.assertRaises(ValueError):
            firmware.publish(self.bin, "has space", self.dir)
        small = fake_image(os.path.join(self.dir, "small.bin"), size=10)
        with self.assertRaises(ValueError):
            firmware.publish(small, "v1", self.dir)
        text = os.path.join(self.dir, "text.bin")
        with open(text, "wb") as f:
            f.write(b"x" * 200_000)
        with self.assertRaises(ValueError):  # no ESP32 magic byte
            firmware.publish(text, "v1", self.dir)

    def test_tampered_image_is_not_served(self):
        firmware.publish(self.bin, "v2", self.dir)
        with open(os.path.join(self.dir, firmware.IMAGE), "ab") as f:
            f.write(b"junk")  # size no longer matches the manifest
        self.assertIsNone(firmware.current(self.dir))


try:
    from fastapi.testclient import TestClient
except ImportError:  # the rule tests above need no FastAPI; this one does
    TestClient = None


@unittest.skipIf(TestClient is None, "fastapi/httpx not installed")
class FirmwareHttp(unittest.TestCase):
    """The two endpoints as the calculator uses them (device token, x-firmware header)."""

    def setUp(self):
        import app as proxy_app

        db.init()
        self.client = TestClient(proxy_app.app)
        _, self.token = db.create_device("ota test")
        self.headers = {"authorization": "Bearer " + self.token, "x-device-id": "calc-test"}
        self.bin = fake_image(os.path.join(tempfile.mkdtemp(), "build.bin"))

    def test_needs_device_token(self):
        r = self.client.get("/v1/firmware", headers={"x-firmware": "x"})
        self.assertEqual(r.status_code, 401)
        self.assertEqual(r.json()["error"]["type"], "device_unknown")
        self.assertEqual(self.client.get("/v1/firmware/image").status_code, 401)

    def test_manifest_and_image(self):
        self.client.get("/v1/firmware", headers=self.headers)  # whatever was published before this test
        m = firmware.publish(self.bin, "stage14-2026.10.07")
        r = self.client.get("/v1/firmware", headers={**self.headers, "x-firmware": "stage14-2026.10.06"})
        self.assertEqual(r.status_code, 200)
        body = r.json()
        self.assertTrue(body["update"])
        self.assertEqual((body["version"], body["size"], body["sha256"], body["path"]),
                         ("stage14-2026.10.07", m["size"], m["sha256"], "/v1/firmware/image"))
        r = self.client.get("/v1/firmware", headers={**self.headers, "x-firmware": "stage14-2026.10.07"})
        self.assertFalse(r.json()["update"])
        img = self.client.get("/v1/firmware/image", headers=self.headers)
        self.assertEqual(img.status_code, 200)
        self.assertEqual(len(img.content), m["size"])
        self.assertEqual(hashlib.sha256(img.content).hexdigest(), m["sha256"])
        self.assertEqual(img.headers["x-firmware-version"], "stage14-2026.10.07")


if __name__ == "__main__":
    unittest.main()
