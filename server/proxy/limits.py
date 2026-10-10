"""Per-device burst limits: solves in flight, solves per minute / hour, and a looser limit for
the small endpoints. Kept in memory, so the proxy must run as ONE process (one uvicorn
worker; see README). The monthly cap per tier is in policy.py (database).

A lease that is never released (a crashed stream) expires after LEASE_MAX_S, so a
calculator can never lock itself out for good.
"""
import threading
import time
from collections import deque

LEASE_MAX_S = 15 * 60


class Limited(Exception):
    def __init__(self, reason, retry_after):
        super().__init__(reason)
        self.reason = reason            # "concurrency" | "minute" | "hour"
        self.retry_after = max(1, int(retry_after + 0.999))


class Lease:
    def __init__(self, limiter, key, token):
        self._limiter, self._key, self._token = limiter, key, token
        self._done = False

    def release(self):
        if not self._done:
            self._done = True
            self._limiter._release(self._key, self._token)


class Limiter:
    def __init__(self, clock=time.monotonic):
        self._clock = clock
        self._lock = threading.Lock()
        self._events = {}   # key -> deque of timestamps (last hour)
        self._inflight = {}  # key -> {token: started}
        self._next = 0

    def acquire(self, key, concurrency, per_minute, per_hour=0):
        """A Lease for one request, or raises Limited. concurrency/per_* <= 0 means no limit."""
        t = self._clock()
        with self._lock:
            ev = self._events.setdefault(key, deque())
            while ev and t - ev[0] >= 3600:
                ev.popleft()
            live = self._inflight.setdefault(key, {})
            for tok, started in list(live.items()):
                if t - started > LEASE_MAX_S:
                    del live[tok]
            if concurrency > 0 and len(live) >= concurrency:
                raise Limited("concurrency", 5)
            if per_minute > 0:
                recent = [x for x in ev if t - x < 60]
                if len(recent) >= per_minute:
                    raise Limited("minute", 60 - (t - recent[0]))
            if per_hour > 0 and len(ev) >= per_hour:
                raise Limited("hour", 3600 - (t - ev[0]))
            ev.append(t)
            self._next += 1
            live[self._next] = t
            return Lease(self, key, self._next)

    def hit(self, key, per_minute):
        """Counts one small request (no lease). Raises Limited above per_minute."""
        self.acquire(key, 0, per_minute).release()

    def _release(self, key, token):
        with self._lock:
            self._inflight.get(key, {}).pop(token, None)

    def reset(self):
        with self._lock:
            self._events.clear()
            self._inflight.clear()
