#!/usr/bin/env python3
"""Check firmware-v15-lcd/src/root_ca.h (the calculator's trusted TLS roots).

Standard library only. For every PEM block in kRootCAs it checks that:
  - it parses as an X.509 certificate,
  - its SHA-256 fingerprint equals the "SHA-256" line in the comment above it,
  - it is a CA (basicConstraints cA = TRUE) and self-signed (subject == issuer),
  - it does not expire before 2030 (warning only for older roots kept on purpose).
With --live it also opens TLS connections to public Cloudflare hosts using ONLY the
bundle as the trust store (needs internet).

  python tools/tls_check/check_roots.py [--live] [host ...]
Exit code 0 = all checks passed.
"""
import hashlib
import os
import re
import socket
import ssl
import sys
import tempfile
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
HEADER = os.path.normpath(os.path.join(HERE, '..', '..', 'firmware-v15-lcd', 'src', 'root_ca.h'))
# workers.dev / cloudflare.com: Google Trust Services (WE1 <- GTS Root R4);
# one.one.one.one: Cloudflare's own SSL.com chain (SSL.com ECC R2 <- SSL.com ECC root).
DEFAULT_HOSTS = ['workers.dev', 'workers.cloudflare.com', 'developers.cloudflare.com',
                 'one.one.one.one', 'letsencrypt.org', 'api.anthropic.com']
MIN_EXPIRY_YEAR = 2030


def read_bundle(path):
    """Returns (pem_text, [(fingerprint_from_comment, pem_block), ...])."""
    lines = open(path, encoding='utf-8').read().splitlines()
    pem, certs, fp, cur = [], [], None, None
    for line in lines:
        m = re.match(r'\s*//\s*SHA-256\s+([0-9A-F:]+)', line)
        if m:
            fp = m.group(1).replace(':', '')
            continue
        m = re.match(r'\s*"(.*)\\n"\s*;?\s*$', line)
        if not m:
            continue
        text = m.group(1)
        pem.append(text)
        if text == '-----BEGIN CERTIFICATE-----':
            cur = [text]
        elif cur is not None:
            cur.append(text)
            if text == '-----END CERTIFICATE-----':
                certs.append((fp, '\n'.join(cur) + '\n'))
                cur, fp = None, None
    return '\n'.join(pem) + '\n', certs


def der_len(b, i):
    """Length of the DER element whose length byte is at b[i]; returns (len, next index)."""
    n = b[i]
    if n < 0x80:
        return n, i + 1
    k = n & 0x7F
    return int.from_bytes(b[i + 1:i + 1 + k], 'big'), i + 1 + k


def is_ca(der):
    """True if the basicConstraints extension (OID 2.5.29.19) says cA = TRUE."""
    oid = bytes([0x06, 0x03, 0x55, 0x1D, 0x13])
    i = der.find(oid)
    if i < 0:
        return False
    i += len(oid)
    if der[i] == 0x01:  # critical BOOLEAN
        i += 3
    if der[i] != 0x04:  # OCTET STRING holding the BasicConstraints SEQUENCE
        return False
    _, i = der_len(der, i + 1)
    if der[i] != 0x30:
        return False
    n, i = der_len(der, i + 1)
    return n >= 3 and der[i] == 0x01 and der[i + 2] != 0x00


def decode(pem):
    with tempfile.NamedTemporaryFile('w', suffix='.pem', delete=False) as f:
        f.write(pem)
        name = f.name
    try:
        return ssl._ssl._test_decode_cert(name)  # stdlib helper; parses with OpenSSL
    finally:
        os.unlink(name)


def name_of(rdns):
    return ', '.join(f'{k}={v}' for rdn in rdns for k, v in rdn)


def check_static(certs):
    ok = True
    for i, (want, pem) in enumerate(certs):
        der = ssl.PEM_cert_to_DER_cert(pem)
        fp = hashlib.sha256(der).hexdigest().upper()
        info = decode(pem)
        cn = dict(x for rdn in info['subject'] for x in rdn).get('commonName', '?')
        exp = datetime.strptime(info['notAfter'], '%b %d %H:%M:%S %Y %Z').replace(tzinfo=timezone.utc)
        problems = []
        if want is None:
            problems.append('no SHA-256 comment')
        elif want != fp:
            problems.append(f'fingerprint {fp} != comment {want}')
        if not is_ca(der):
            problems.append('not a CA')
        if info['subject'] != info['issuer']:
            problems.append('not self-signed (issuer ' + name_of(info['issuer']) + ')')
        if exp < datetime.now(timezone.utc):
            problems.append('EXPIRED')
        warn = ' (warning: expires before %d)' % MIN_EXPIRY_YEAR if exp.year < MIN_EXPIRY_YEAR else ''
        status = 'FAIL ' + '; '.join(problems) if problems else 'ok'
        ok &= not problems
        print(f'{i + 1:2}. {cn:48} {exp:%Y-%m-%d} {len(pem):5} B  {status}{warn}')
    return ok


def check_live(bundle, hosts):
    ctx = ssl.create_default_context(cadata=bundle)  # ONLY the bundle, not the OS store
    ok = True
    for h in hosts:
        try:
            with socket.create_connection((h, 443), timeout=15) as s:
                with ctx.wrap_socket(s, server_hostname=h) as t:
                    cert = t.getpeercert()
                    issuer = dict(x for rdn in cert['issuer'] for x in rdn)
                    print(f'  {h:28} ok  {t.version()}  {t.cipher()[0]}  leaf issuer: '
                          f'{issuer.get("organizationName", "")} / {issuer.get("commonName", "")}')
        except ssl.SSLCertVerificationError as e:
            ok = False
            print(f'  {h:28} UNTRUSTED  {e.verify_message}')
        except OSError as e:
            print(f'  {h:28} skipped (network: {e})')
    return ok


def main(argv):
    live = '--live' in argv
    hosts = [a for a in argv if not a.startswith('--')] or DEFAULT_HOSTS
    bundle, certs = read_bundle(HEADER)
    print(f'{HEADER}\n{len(certs)} roots, {len(bundle)} bytes of PEM (+1 NUL in flash)')
    ok = len(certs) > 0 and check_static(certs)
    if live:
        print('Live handshakes, trusting only this bundle:')
        ok &= check_live(bundle, hosts)
    print('ALL OK' if ok else 'PROBLEMS FOUND')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
