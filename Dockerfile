# syntax=docker/dockerfile:1
#
# Rein Norscode-runtime — ingen Python, ingen SQLite.
# std.db er NorsDB (rein Norscode); den valfrie SQLite-adapteren er dlopen-basert
# og aldri hard-lenka, så libsqlite3 trengst ikkje i normalbiletet.
#
# Byggjesteget materialiserer den plattform-native binæren frå den committede
# Linux x86_64-seeden via rein .no-kode (shellfri) — ingen gcc/clang/zig.
#
# ATTSTÅANDE (jf. docs/BYGG_ALT_SELVSTENDIG_PLAN.md Blokk B): den COMMITTEDE
# Linux x86_64-seeden er sjølv dynamisk lenka mot libssl.so.3/libcrypto.so.3
# (bygd via OpenSSL-kandidat-lana, ikkje den reine stien enno). Difor må
# ca-certificates (som dreg inn libssl3) vere med. Openssl-avhengnaden fell
# vekk automatisk når den reine full-host Linux-binæren blir promotert til seed.

# ── Byggjesteg: materialiser native runtime ─────────────────────────────────
FROM debian:bookworm-slim AS build

WORKDIR /opt/norscode

# ca-certificates: web-PKI for utgåande TLS. glibc følgjer med debian-slim
# (seed-ELF-en er dynamisk lenka mot /lib64/ld-linux-x86-64.so.2).
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY . .

# Gjer ./bin/nc køyrbar i containeren: seed frå den committede Linux x86_64-ELF-en,
# så materialiser den plattform-native binæren via rein Norscode.
RUN cp bootstrap/stage0/norscode-linux-x86_64 dist/norscode_native \
    && chmod +x dist/norscode_native \
    && ./bin/nc run tools/build-bootstrap-binary.no

# ── Køyresteg: slankt bilete med berre den native binæren ────────────────────
FROM debian:bookworm-slim AS runtime

WORKDIR /opt/norscode

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /opt/norscode /opt/norscode

VOLUME ["/work"]
EXPOSE 8000

# MERK: den argv-baserte forma under føreset ein full-CLI Linux-binær. Den
# committede seeden er enno env-driven (NORSCODE_CMD/NORSCODE_FILE) og køyrer
# berre selftest på rå argv — dvs. denne CMD blir først funksjonell når den reine
# full-host-binæren er promotert (Blokk B / Omgang 15 «nc run → AOT»). Fram til
# då må serving drivast via env, t.d.:
#   docker run -e NORSCODE_CMD=serve -e NORSCODE_FILE=/work/app.no ... <img>
ENTRYPOINT ["/opt/norscode/bin/nc"]
CMD ["serve", "/work/examples/web_routes.no", "--host", "0.0.0.0", "--port", "8000", "--production"]
