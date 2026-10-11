"""pytest setup: a throw-away database and firmware folder, and no real secrets or network.
Runs before any test module imports config."""
import os
import tempfile

_tmp = tempfile.mkdtemp(prefix="calc-proxy-test-")
os.environ["DB_PATH"] = os.path.join(_tmp, "test.db")
os.environ["FIRMWARE_DIR"] = os.path.join(_tmp, "fw")
os.environ["PUBLIC_URL"] = "https://calc.example.com"
os.environ["ANTHROPIC_API_KEY"] = ""          # tests never reach the real API
os.environ["LINK_SECRET"] = "test-link-secret"
os.environ["STRIPE_WEBHOOK_SECRET"] = "whsec_test"
os.environ["MAIL_BACKEND"] = "console"
