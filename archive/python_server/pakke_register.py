#!/usr/bin/env python3
"""
pakke_register.py — Lokal Norscode-pakkeregister

Implementerer register-APIet som std/pakke.no forventar:
  GET  /søk?q=<ord>               — søk etter pakkar
  GET  /pakke/<namn>              — pakkeinfo + versjonsliste
  GET  /pakke/<namn>/<versjon>.tar.gz  — last ned pakke
  POST /publiser                  — last opp ny pakke (treng Bearer-token)
  GET  /helse                     — helsesjekk

Pakkar ligg i nc_pakkar/<namn>/<versjon>/
Metadata i nc_pakkar/<namn>/pakke.json

Bruk:
    python3 tools/pakke_register.py [--port 5173] [--mappe nc_pakkar]

Eller via nc:
    nc pakke register [--port 5173]
"""

import asyncio
import json
import os
import sys
import tarfile
import tempfile
import time
import io
from urllib.parse import urlparse, parse_qs

DEFAULT_PORT  = int(os.environ.get("NC_REGISTER_PORT", "5173"))
DEFAULT_MAPPE = os.environ.get("NC_PAKKAR_MAPPE", "nc_pakkar")


# ─── Pakkelager ───────────────────────────────────────────────────────────────

class Pakkelager:
    def __init__(self, rot: str):
        self.rot = os.path.abspath(rot)
        os.makedirs(self.rot, exist_ok=True)

    def _pakke_rot(self, namn: str) -> str:
        safe = namn.replace("/", "_").replace("..", "_")
        return os.path.join(self.rot, safe)

    def les_meta(self, namn: str) -> dict:
        p = os.path.join(self._pakke_rot(namn), "pakke.json")
        if not os.path.isfile(p):
            return {}
        with open(p, encoding="utf-8") as f:
            return json.load(f)

    def skriv_meta(self, namn: str, meta: dict):
        mappe = self._pakke_rot(namn)
        os.makedirs(mappe, exist_ok=True)
        with open(os.path.join(mappe, "pakke.json"), "w", encoding="utf-8") as f:
            json.dump(meta, f, indent=2, ensure_ascii=False)

    def versjonar(self, namn: str) -> list:
        rot = self._pakke_rot(namn)
        if not os.path.isdir(rot):
            return []
        return sorted(
            d for d in os.listdir(rot)
            if os.path.isdir(os.path.join(rot, d)) and d[0].isdigit()
        )

    def siste_versjon(self, namn: str) -> str:
        vs = self.versjonar(namn)
        return vs[-1] if vs else ""

    def alle_pakkar(self) -> list:
        if not os.path.isdir(self.rot):
            return []
        return [
            d for d in os.listdir(self.rot)
            if os.path.isdir(os.path.join(self.rot, d))
        ]

    def pakke_arkiv_sti(self, namn: str, versjon: str) -> str:
        return os.path.join(self._pakke_rot(namn), versjon, f"{namn}-{versjon}.tar.gz")

    def har_arkiv(self, namn: str, versjon: str) -> bool:
        return os.path.isfile(self.pakke_arkiv_sti(namn, versjon))

    def les_arkiv(self, namn: str, versjon: str) -> bytes:
        sti = self.pakke_arkiv_sti(namn, versjon)
        with open(sti, "rb") as f:
            return f.read()

    def lagre_arkiv(self, namn: str, versjon: str, data: bytes):
        mappe = os.path.join(self._pakke_rot(namn), versjon)
        os.makedirs(mappe, exist_ok=True)
        sti = os.path.join(mappe, f"{namn}-{versjon}.tar.gz")
        with open(sti, "wb") as f:
            f.write(data)

    def søk(self, spørjing: str) -> list:
        q   = spørjing.lower()
        res = []
        for namn in self.alle_pakkar():
            meta = self.les_meta(namn)
            besk = (meta.get("beskriving") or "").lower()
            if q in namn.lower() or q in besk:
                res.append({
                    "namn":       namn,
                    "versjon":    self.siste_versjon(namn),
                    "beskriving": meta.get("beskriving", ""),
                    "forfattar":  meta.get("forfattar", ""),
                })
        return res


# ─── HTTP-ruting ──────────────────────────────────────────────────────────────

def json_resp(data, status=200) -> tuple:
    body = json.dumps(data, ensure_ascii=False, indent=2).encode("utf-8")
    return status, {"content-type": "application/json; charset=utf-8"}, body


def err(melding: str, status=400) -> tuple:
    return json_resp({"feil": melding}, status)


def route(lager: Pakkelager, api_token: str, method: str,
          path: str, query: dict, body: bytes) -> tuple:
    parts = [p for p in path.split("/") if p]

    # GET /helse
    if method == "GET" and path in ("/helse", "/health"):
        return json_resp({"ok": True, "pakkar": len(lager.alle_pakkar())})

    # GET /søk?q=...
    if method == "GET" and parts and parts[0] in ("søk", "sok", "search"):
        q = query.get("q", [""])[0]
        return json_resp(lager.søk(q))

    # GET /pakkar — liste alle
    if method == "GET" and parts and parts[0] in ("pakkar", "packages"):
        alle = []
        for namn in lager.alle_pakkar():
            meta = lager.les_meta(namn)
            alle.append({
                "namn":       namn,
                "versjon":    lager.siste_versjon(namn),
                "beskriving": meta.get("beskriving", ""),
            })
        return json_resp(alle)

    # /pakke/<namn>[/<versjon>.tar.gz]
    if parts and parts[0] == "pakke" and len(parts) >= 2:
        namn = parts[1]

        # GET /pakke/<namn>/<versjon>.tar.gz — last ned
        if method == "GET" and len(parts) == 3 and parts[2].endswith(".tar.gz"):
            versjon = parts[2][:-7]  # strip .tar.gz
            if not lager.har_arkiv(namn, versjon):
                return err(f"Pakke {namn}@{versjon} ikkje funne", 404)
            data = lager.les_arkiv(namn, versjon)
            return 200, {"content-type": "application/gzip",
                         "content-disposition": f'attachment; filename="{namn}-{versjon}.tar.gz"'}, data

        # GET /pakke/<namn> — pakkeinfo
        if method == "GET" and len(parts) == 2:
            meta = lager.les_meta(namn)
            if not meta and not lager.versjonar(namn):
                return err(f"Pakke {namn} ikkje funne", 404)
            meta["siste_versjon"] = lager.siste_versjon(namn)
            meta["alle_versjonar"] = lager.versjonar(namn)
            return json_resp(meta)

    # POST /publiser — last opp pakke
    if method == "POST" and parts and parts[0] == "publiser":
        auth = query.get("token", [""])[0] or ""
        if api_token and auth != api_token:
            return err("Ugyldig API-token", 401)

        # Pakk ut metadata frå tar.gz
        try:
            tf = tarfile.open(fileobj=io.BytesIO(body))
            meta = {}
            for member in tf.getmembers():
                if member.name.endswith("pakke.json"):
                    f = tf.extractfile(member)
                    if f:
                        meta = json.load(f)
                    break
            tf.close()
        except Exception as e:
            return err(f"Ugyldig arkiv: {e}")

        namn    = meta.get("namn", "")
        versjon = meta.get("versjon", "")
        if not namn or not versjon:
            return err("pakke.json manglar namn eller versjon")

        lager.lagre_arkiv(namn, versjon, body)
        meta["siste_versjon"] = versjon
        lager.skriv_meta(namn, meta)

        print(f"  [publiser] {namn}@{versjon}")
        return json_resp({"ok": True, "pakke": namn, "versjon": versjon}, 201)

    return err(f"Ikkje funne: {method} {path}", 404)


# ─── Async-server ─────────────────────────────────────────────────────────────

class RegisterHandler(asyncio.Protocol):
    def __init__(self, lager, api_token, access_log=True):
        self.lager      = lager
        self.api_token  = api_token
        self.access_log = access_log
        self._buf       = b""
        self._transport = None

    def connection_made(self, transport):
        self._transport = transport
        self._peer = transport.get_extra_info("peername", ("?", 0))

    def data_received(self, data):
        self._buf += data
        if b"\r\n\r\n" not in self._buf:
            return
        asyncio.ensure_future(self._process())

    async def _process(self):
        raw  = self._buf
        self._buf = b""
        try:
            sep         = raw.index(b"\r\n\r\n")
            header_part = raw[:sep].decode("utf-8", errors="replace")
            body        = raw[sep + 4:]
            lines       = header_part.split("\r\n")
            req_parts   = lines[0].split(" ")
            method      = req_parts[0] if req_parts else "GET"
            full_path   = req_parts[1] if len(req_parts) > 1 else "/"
            parsed      = urlparse(full_path)
            path        = parsed.path
            query       = parse_qs(parsed.query)

            req_hdrs = {}
            for line in lines[1:]:
                if ":" in line:
                    k, v = line.split(":", 1)
                    req_hdrs[k.strip().lower()] = v.strip()

            content_len = int(req_hdrs.get("content-length", 0))
            body_data   = body[:content_len]

            # Token kan kome frå Authorization-header òg
            auth_hdr = req_hdrs.get("authorization", "")
            if auth_hdr.startswith("Bearer "):
                query.setdefault("token", [auth_hdr[7:]])

            status, hdrs, resp_body = route(
                self.lager, self.api_token, method, path, query, body_data
            )
        except Exception as e:
            status    = 500
            hdrs      = {"content-type": "application/json"}
            resp_body = json.dumps({"feil": str(e)}).encode()

        if self.access_log:
            print(f"  {self._peer[0]} — {method} {path} → {status}")

        merged = {"content-length": str(len(resp_body)), "connection": "close",
                  "access-control-allow-origin": "*"}
        merged.update(hdrs)
        head_lines = [f"HTTP/1.1 {status} OK"]
        head_lines += [f"{k}: {v}" for k, v in merged.items()]
        head_lines += ["", ""]
        self._transport.write("\r\n".join(head_lines).encode())
        self._transport.write(resp_body)
        self._transport.close()

    def connection_lost(self, exc): pass
    def eof_received(self): return False


async def _serve(mappe, host, port, api_token, access_log):
    lager = Pakkelager(mappe)
    loop  = asyncio.get_event_loop()

    def make_handler():
        return RegisterHandler(lager, api_token, access_log)

    server = await loop.create_server(make_handler, host, port, reuse_port=True)

    print(f"\n  Norscode pakkeregister")
    print(f"  Mappe: {os.path.abspath(mappe)}")
    print(f"  URL:   http://localhost:{port}/")
    print(f"  Pakkar: {len(lager.alle_pakkar())}")
    if api_token:
        print(f"  Auth:  Bearer-token aktiv")
    print(f"  Trykk Ctrl+C for å stoppe\n")

    stop = asyncio.Event()

    import signal as _sig
    def _shutdown():
        stop.set()

    for sig in (_sig.SIGTERM, _sig.SIGINT):
        try:
            loop.add_signal_handler(sig, _shutdown)
        except NotImplementedError:
            pass

    async with server:
        await stop.wait()
    print("Register stoppa.")


def run_register(mappe=DEFAULT_MAPPE, host="0.0.0.0", port=DEFAULT_PORT,
                 api_token="", access_log=True):
    asyncio.run(_serve(mappe, host, port, api_token, access_log))


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Norscode pakkeregister")
    parser.add_argument("--port",  type=int, default=DEFAULT_PORT)
    parser.add_argument("--host",            default="0.0.0.0")
    parser.add_argument("--mappe",           default=DEFAULT_MAPPE)
    parser.add_argument("--token",           default=os.environ.get("NC_API_NØKKEL", ""))
    parser.add_argument("--no-log",          action="store_true")
    args = parser.parse_args()
    run_register(args.mappe, args.host, args.port, args.token, not args.no_log)
