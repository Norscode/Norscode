/*
 * nc_runtime.c — Norscode native runtime for lists, maps, strings, and I/O.
 *
 * All values are represented as int64_t:
 *   - heltall (integer):  raw int64_t
 *   - tekst   (string):   pointer to NUL-terminated char* (heap or .rodata)
 *   - liste   (list):     pointer to nc_list_t
 *   - ordbok  (map):      pointer to nc_map_t
 *   - boolsk  (bool):     0 or 1
 *   - ingenting (nil):    0
 *
 * Functions are exported with C linkage so AArch64 assembly can call them
 * via bl _nc_* (macOS) or bl nc_* (Linux).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ── Liste ─────────────────────────────────────────────────────────────────── */
typedef struct {
    int64_t  len;
    int64_t  cap;
    int64_t *data;  /* array of values */
} nc_list_t;

nc_list_t *nc_liste_ny(void) {
    nc_list_t *l = calloc(1, sizeof(nc_list_t));
    l->cap  = 8;
    l->data = calloc(l->cap, sizeof(int64_t));
    return l;
}

void nc_liste_legg_til(nc_list_t *l, int64_t val) {
    if (l->len >= l->cap) {
        l->cap *= 2;
        l->data = realloc(l->data, l->cap * sizeof(int64_t));
    }
    l->data[l->len++] = val;
}

int64_t nc_liste_les(nc_list_t *l, int64_t idx) {
    if (idx < 0) idx += l->len;
    if (idx < 0 || idx >= l->len) return 0;
    return l->data[idx];
}

void nc_liste_sett(nc_list_t *l, int64_t idx, int64_t val) {
    if (idx < 0) idx += l->len;
    if (idx < 0 || idx >= l->len) return;
    l->data[idx] = val;
}

int64_t nc_liste_fjern_siste(nc_list_t *l) {
    if (l->len <= 0) return 0;
    return l->data[--l->len];
}

int64_t nc_liste_lengde(nc_list_t *l) {
    return l ? l->len : 0;
}

/* ── Ordbok (map) ──────────────────────────────────────────────────────────── */
typedef struct {
    int64_t  len;
    int64_t  cap;
    char   **nøkler;
    int64_t *verdier;
} nc_map_t;

nc_map_t *nc_map_ny(void) {
    nc_map_t *m = calloc(1, sizeof(nc_map_t));
    m->cap    = 8;
    m->nøkler = calloc(m->cap, sizeof(char *));
    m->verdier = calloc(m->cap, sizeof(int64_t));
    return m;
}

int64_t nc_map_hent(nc_map_t *m, const char *k) {
    if (!m) return 0;
    for (int64_t i = 0; i < m->len; i++)
        if (!strcmp(m->nøkler[i], k)) return m->verdier[i];
    return 0;
}

void nc_map_sett(nc_map_t *m, const char *k, int64_t val) {
    for (int64_t i = 0; i < m->len; i++) {
        if (!strcmp(m->nøkler[i], k)) { m->verdier[i] = val; return; }
    }
    if (m->len >= m->cap) {
        m->cap *= 2;
        m->nøkler  = realloc(m->nøkler,  m->cap * sizeof(char *));
        m->verdier = realloc(m->verdier, m->cap * sizeof(int64_t));
    }
    m->nøkler [m->len] = strdup(k);
    m->verdier[m->len] = val;
    m->len++;
}

int64_t nc_map_har_nøkkel(nc_map_t *m, const char *k) {
    if (!m) return 0;
    for (int64_t i = 0; i < m->len; i++)
        if (!strcmp(m->nøkler[i], k)) return 1;
    return 0;
}

int64_t nc_map_lengde(nc_map_t *m) {
    return m ? m->len : 0;
}

/* ── Tekst (strenger) ──────────────────────────────────────────────────────── */
char *nc_tekst_sammenslå(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

char *nc_heltall_til_tekst(int64_t n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)n);
    return strdup(buf);
}

int64_t nc_tekst_til_heltall(const char *s) {
    return s ? (int64_t)atoll(s) : 0;
}

int64_t nc_tekst_lengde(const char *s) {
    return s ? (int64_t)strlen(s) : 0;
}

/* ── Utskrift (write() direkte — unngår stdio-buffering ved raw exit) ──────── */
#include <unistd.h>

void nc_skriv_tekst(const char *s) {
    if (s) { size_t l = strlen(s); if (l) write(1, s, l); }
    write(1, "\n", 1);
}

void nc_skriv_heltall(int64_t n) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lld\n", (long long)n);
    if (len > 0) write(1, buf, (size_t)len);
}

/* ── Listeoperasjoner for assembly (alle tar/returnerer int64_t) ─────────── */
/* nc_liste_ny_val:   returnerer peker som int64_t */
int64_t nc_liste_ny_val(void) {
    return (int64_t)(uintptr_t)nc_liste_ny();
}

void nc_liste_legg_til_val(int64_t liste_ptr, int64_t val) {
    nc_liste_legg_til((nc_list_t *)(uintptr_t)liste_ptr, val);
}

int64_t nc_liste_les_val(int64_t liste_ptr, int64_t idx) {
    return nc_liste_les((nc_list_t *)(uintptr_t)liste_ptr, idx);
}

void nc_liste_sett_val(int64_t liste_ptr, int64_t idx, int64_t val) {
    nc_liste_sett((nc_list_t *)(uintptr_t)liste_ptr, idx, val);
}

int64_t nc_liste_fjern_siste_val(int64_t liste_ptr) {
    return nc_liste_fjern_siste((nc_list_t *)(uintptr_t)liste_ptr);
}

int64_t nc_liste_lengde_val(int64_t liste_ptr) {
    return nc_liste_lengde((nc_list_t *)(uintptr_t)liste_ptr);
}

/* ── Mappeoperasjoner for assembly ─────────────────────────────────────────── */
int64_t nc_map_ny_val(void) {
    return (int64_t)(uintptr_t)nc_map_ny();
}

int64_t nc_map_hent_val(int64_t map_ptr, const char *k) {
    return nc_map_hent((nc_map_t *)(uintptr_t)map_ptr, k);
}

void nc_map_sett_val(int64_t map_ptr, const char *k, int64_t val) {
    nc_map_sett((nc_map_t *)(uintptr_t)map_ptr, k, val);
}

int64_t nc_map_har_nøkkel_val(int64_t map_ptr, const char *k) {
    return nc_map_har_nøkkel((nc_map_t *)(uintptr_t)map_ptr, k);
}

int64_t nc_map_lengde_val(int64_t map_ptr) {
    return nc_map_lengde((nc_map_t *)(uintptr_t)map_ptr);
}
