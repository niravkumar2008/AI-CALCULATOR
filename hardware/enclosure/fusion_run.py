"""Send a Python file to the FusionMCPBridge add-in and print what it printed.

Usage: python fusion_run.py script.py [key=value ...]   (the pairs become the ARGS dict)
       python fusion_run.py build_case.py all             (every build stage in order)
Needs Fusion open with the add-in running. The bridge gives up after 30 s but Fusion
keeps going; for build_case stages we then wait for "stage X done" in build.log.
"""
import json, pathlib, sys, time, urllib.request
sys.stdout.reconfigure(encoding="utf-8")

HERE = pathlib.Path(__file__).parent
ALL = ["shells", "front", "back", "keymat", "legends", "faceplate", "parts", "check", "export"]

def run(script_path, args):
    script = "ARGS = %r\n" % args + pathlib.Path(script_path).read_text(encoding="utf8")
    secret = (pathlib.Path.home() / ".fusion-mcp-secret").read_text().strip()
    req = urllib.request.Request(
        "http://127.0.0.1:7654/execute", data=json.dumps({"script": script}).encode(),
        headers={"Authorization": f"Bearer {secret}", "Content-Type": "application/json"})
    log = HERE / "build.log"
    start = log.stat().st_size if log.exists() else 0
    try:
        r = json.load(urllib.request.urlopen(req, timeout=60))
    except urllib.error.HTTPError as e:
        r = json.load(e)
    except TimeoutError:
        r = {"error": "Fusion did not respond within timeout"}
    if "timeout" in str(r.get("error", "")) and "stage" in args:
        tail = lambda: log.read_bytes()[start:].decode("utf8", "replace") if log.exists() else ""
        t0 = time.time()
        while "stage %s done" % args["stage"] not in tail() and "FAILED" not in tail():
            started = any(l.endswith(" stage " + args["stage"]) for l in tail().splitlines())
            if not started and time.time() - t0 > 120:
                return run(script_path, args)       # the request never reached Fusion: send it again
            time.sleep(3)
        r = {"result": tail()[-1500:] if "FAILED" in tail() else "(finished after the bridge timed out)"}
        if "FAILED" in tail():
            r["traceback"] = "see build.log"
    print(r.get("result") or r.get("error") or r)
    if r.get("traceback"):
        print(r["traceback"])
        sys.exit(1)

if __name__ == "__main__":
    if sys.argv[2:] == ["all"]:
        for st in ALL:
            t = time.time()
            print("==", st)
            run(sys.argv[1], {"stage": st})
            print("   %.0f s" % (time.time() - t))
    else:
        run(sys.argv[1], dict(a.split("=", 1) for a in sys.argv[2:]))
