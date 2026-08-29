/*
 * nc_runtime_full.c — komplett NcVal-runtime for native ELF
 * 
 * Kompiler: clang -target x86_64-linux-gnu -O1 -fno-stack-protector
 *           -ffreestanding -nostdlib -fno-builtin -o nc_rt.o -c nc_runtime_full.c
 *
 * NcVal: 16 bytes = { uint64_t type; uint64_t val }
 * type: 0=nil, 1=int, 2=str, 3=list, 4=map, 5=bool
 * val:  int value / ptr to {len,bytes} / ptr to list / ptr to map / 0/1
 *
 * Minnekart (same som native_codegen.no):
 *   HEAP_VA = 0x600000
 *   HEAP_VA[0..7] = heap_next (ptr til neste ledig byte)
 *   heap veks opp frå HEAP_VA+8
 */

typedef unsigned long  u64;
typedef unsigned int   u32;
typedef unsigned char  u8;

#define HEAP_VA  0x600000UL
#define HEAP_SZ  (64*1024*1024UL)  // 64MB

/* Type-konstantar */
#define NC_NIL   0UL
#define NC_INT   1UL
#define NC_STR   2UL
#define NC_LIST  3UL
#define NC_MAP   4UL
#define NC_BOOL  5UL

typedef struct { u64 type; u64 val; } NcVal;  // 16 bytes

/* Bump-allocator: HEAP_VA[0..7] = heap_next pointer */
static u8* heap_alloc(u64 sz) {
    u64* hp = (u64*)HEAP_VA;
    u8*  ptr = (u8*)(*hp);
    *hp = (u64)(ptr + sz);
    return ptr;
}

static NcVal* nc_alloc(void) {
    return (NcVal*)heap_alloc(sizeof(NcVal));
}

/* ── Konstruktørar ────────────────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* nc_nil(void) {
    NcVal* v = nc_alloc(); v->type = NC_NIL; v->val = 0; return v;
}
__attribute__((noinline)) NcVal* nc_int(u64 i) {
    NcVal* v = nc_alloc(); v->type = NC_INT; v->val = i; return v;
}
__attribute__((noinline)) NcVal* nc_bool(u64 b) {
    NcVal* v = nc_alloc(); v->type = NC_BOOL; v->val = (b ? 1 : 0); return v;
}

/* Streng: { u64 len; u8 bytes[len]; } — NUL-avslutta */
__attribute__((noinline)) NcVal* nc_str_raw(const u8* src, u64 len) {
    u8* buf = heap_alloc(8 + len + 1);
    *(u64*)buf = len;
    u64 i = 0;
    while (i < len) { buf[8 + i] = src[i]; i++; }
    buf[8 + len] = 0;
    NcVal* v = nc_alloc();
    v->type = NC_STR; v->val = (u64)buf;
    return v;
}
__attribute__((noinline)) NcVal* nc_str_cstr(const u8* s) {
    u64 len = 0;
    while (s[len]) len++;
    return nc_str_raw(s, len);
}

/* ── Streng-operasjonar ────────────────────────────────────────────────────── */
static u64   str_len(NcVal* s)  { return *(u64*)s->val; }
static u8*   str_ptr(NcVal* s)  { return (u8*)(s->val + 8); }

__attribute__((noinline)) NcVal* rt_concat(NcVal* a, NcVal* b) {
    u64 la = str_len(a), lb = str_len(b);
    u64 tot = la + lb;
    u8* buf = heap_alloc(8 + tot + 1);
    *(u64*)buf = tot;
    u64 i = 0;
    u8* pa = str_ptr(a); while (i < la) { buf[8+i] = pa[i]; i++; }
    u8* pb = str_ptr(b); u64 j = 0;
    while (j < lb) { buf[8+la+j] = pb[j]; j++; }
    buf[8+tot] = 0;
    NcVal* v = nc_alloc(); v->type = NC_STR; v->val = (u64)buf;
    return v;
}

__attribute__((noinline)) NcVal* rt_slice(NcVal* s, NcVal* from_v, NcVal* to_v) {
    u64 len = str_len(s);
    u8* ptr = str_ptr(s);
    u64 f = (u64)from_v->val, t = (u64)to_v->val;
    if (f > len) f = len;
    if (t > len) t = len;
    if (t < f)   t = f;
    return nc_str_raw(ptr + f, t - f);
}

__attribute__((noinline)) NcVal* rt_index_of_str(NcVal* haystack, NcVal* needle) {
    u64 hl = str_len(haystack), nl = str_len(needle);
    u8* hp = str_ptr(haystack); u8* np = str_ptr(needle);
    if (nl == 0) return nc_int(0);
    if (nl > hl) return nc_int((u64)-1LL);
    u64 i = 0;
    while (i <= hl - nl) {
        u64 j = 0;
        while (j < nl && hp[i+j] == np[j]) j++;
        if (j == nl) return nc_int(i);
        i++;
    }
    return nc_int((u64)-1LL);
}

__attribute__((noinline)) NcVal* rt_contains(NcVal* haystack, NcVal* needle) {
    NcVal* pos = rt_index_of_str(haystack, needle);
    return nc_bool(pos->val != (u64)-1LL);
}

__attribute__((noinline)) NcVal* rt_starts_with(NcVal* s, NcVal* prefix) {
    u64 sl = str_len(s), pl = str_len(prefix);
    if (pl > sl) return nc_bool(0);
    u8* sp = str_ptr(s); u8* pp = str_ptr(prefix);
    u64 i = 0;
    while (i < pl) { if (sp[i] != pp[i]) return nc_bool(0); i++; }
    return nc_bool(1);
}

__attribute__((noinline)) NcVal* rt_ends_with(NcVal* s, NcVal* suffix) {
    u64 sl = str_len(s), xl = str_len(suffix);
    if (xl > sl) return nc_bool(0);
    u8* sp = str_ptr(s); u8* xp = str_ptr(suffix);
    u64 off = sl - xl;
    u64 i = 0;
    while (i < xl) { if (sp[off+i] != xp[i]) return nc_bool(0); i++; }
    return nc_bool(1);
}

__attribute__((noinline)) NcVal* rt_trim(NcVal* s) {
    u8* p = str_ptr(s); u64 len = str_len(s);
    u64 start = 0;
    while (start < len && (p[start]==' '||p[start]=='\t'||p[start]=='\n'||p[start]=='\r')) start++;
    u64 end = len;
    while (end > start && (p[end-1]==' '||p[end-1]=='\t'||p[end-1]=='\n'||p[end-1]=='\r')) end--;
    return nc_str_raw(p + start, end - start);
}

__attribute__((noinline)) NcVal* rt_replace(NcVal* s, NcVal* from, NcVal* to) {
    /* Enkel implementasjon: scan gjennom s, erstatt kvar førekomst av from med to */
    u64 sl = str_len(s), fl = str_len(from), tl = str_len(to);
    u8* sp = str_ptr(s); u8* fp = str_ptr(from); u8* tp = str_ptr(to);
    if (fl == 0) return s;
    /* Telle førekomstar */
    u64 count = 0, i = 0;
    while (i + fl <= sl) {
        u64 j = 0;
        while (j < fl && sp[i+j] == fp[j]) j++;
        if (j == fl) { count++; i += fl; } else i++;
    }
    u64 new_len = sl + count * tl - count * fl;
    u8* buf = heap_alloc(8 + new_len + 1);
    *(u64*)buf = new_len;
    u64 src = 0, dst = 0;
    while (src <= sl) {
        if (src + fl <= sl) {
            u64 j = 0;
            while (j < fl && sp[src+j] == fp[j]) j++;
            if (j == fl) {
                u64 k = 0;
                while (k < tl) { buf[8+dst] = tp[k]; dst++; k++; }
                src += fl; continue;
            }
        }
        if (src < sl) { buf[8+dst] = sp[src]; dst++; }
        src++;
    }
    buf[8+new_len] = 0;
    NcVal* v = nc_alloc(); v->type = NC_STR; v->val = (u64)buf;
    return v;
}

/* Bruk heile strengen delt inn av separator */
__attribute__((noinline)) NcVal* rt_split(NcVal* s, NcVal* sep);  /* forward */

/* ── heltall↔tekst ────────────────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_int_to_str(NcVal* n) {
    u64 v = n->val;
    int neg = 0;
    if ((long long)v < 0) { neg = 1; v = (u64)(-(long long)v); }
    u8 tmp[24]; int pos = 23; tmp[pos] = 0;
    if (v == 0) { tmp[--pos] = '0'; }
    else { while (v) { tmp[--pos] = '0' + (v % 10); v /= 10; } }
    if (neg) tmp[--pos] = '-';
    return nc_str_cstr(tmp + pos);
}

__attribute__((noinline)) NcVal* rt_str_to_int(NcVal* s) {
    u8* p = str_ptr(s); u64 len = str_len(s);
    long long v = 0; int neg = 0; u64 i = 0;
    if (i < len && p[i] == '-') { neg = 1; i++; }
    while (i < len && p[i] >= '0' && p[i] <= '9') { v = v*10 + p[i]-'0'; i++; }
    return nc_int((u64)(neg ? -v : v));
}

__attribute__((noinline)) NcVal* rt_char_code(NcVal* s) {
    if (str_len(s) == 0) return nc_int(0);
    return nc_int(str_ptr(s)[0]);
}

__attribute__((noinline)) NcVal* rt_chr(NcVal* n) {
    u8 c = (u8)(n->val & 0xFF);
    return nc_str_raw(&c, 1);
}

__attribute__((noinline)) NcVal* rt_lower(NcVal* s) {
    u64 len = str_len(s); u8* p = str_ptr(s);
    u8* buf = heap_alloc(8 + len + 1);
    *(u64*)buf = len;
    u64 i = 0;
    while (i < len) {
        u8 c = p[i];
        buf[8+i] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
        i++;
    }
    buf[8+len] = 0;
    NcVal* v = nc_alloc(); v->type = NC_STR; v->val = (u64)buf;
    return v;
}

__attribute__((noinline)) NcVal* rt_upper(NcVal* s) {
    u64 len = str_len(s); u8* p = str_ptr(s);
    u8* buf = heap_alloc(8 + len + 1);
    *(u64*)buf = len;
    u64 i = 0;
    while (i < len) {
        u8 c = p[i];
        buf[8+i] = (c >= 'a' && c <= 'z') ? c - 32 : c;
        i++;
    }
    buf[8+len] = 0;
    NcVal* v = nc_alloc(); v->type = NC_STR; v->val = (u64)buf;
    return v;
}

__attribute__((noinline)) NcVal* rt_type_of(NcVal* v) {
    static const u8* names[] = {
        (u8*)"null", (u8*)"heltall", (u8*)"tekst",
        (u8*)"liste", (u8*)"ordbok", (u8*)"boolsk"
    };
    u64 t = v->type;
    if (t > 5) t = 0;
    return nc_str_cstr(names[t]);
}

/* ── Liste ────────────────────────────────────────────────────────────────── */
/* List struct: { u64 len; u64 cap; NcVal** items } */
#define LIST_LEN(lv) (*(u64*)(lv)->val)
#define LIST_CAP(lv) (*((u64*)(lv)->val + 1))
#define LIST_ITEMS(lv) ((NcVal**)*((u64*)(lv)->val + 2))

__attribute__((noinline)) NcVal* rt_list_new(void) {
    u64 cap = 8;
    u8* header = heap_alloc(24);  /* len, cap, items_ptr */
    NcVal** items = (NcVal**)heap_alloc(cap * 8);
    *(u64*)header = 0;
    *((u64*)header + 1) = cap;
    *((u64*)header + 2) = (u64)items;
    NcVal* v = nc_alloc(); v->type = NC_LIST; v->val = (u64)header;
    return v;
}

__attribute__((noinline)) void rt_list_app(NcVal* lst, NcVal* item) {
    u64* hp = (u64*)lst->val;
    u64 len = hp[0], cap = hp[1];
    NcVal** items = (NcVal**)hp[2];
    if (len >= cap) {
        u64 new_cap = cap * 2;
        NcVal** new_items = (NcVal**)heap_alloc(new_cap * 8);
        u64 i = 0; while (i < len) { new_items[i] = items[i]; i++; }
        hp[1] = new_cap; hp[2] = (u64)new_items; items = new_items;
    }
    items[len] = item; hp[0] = len + 1;
}

__attribute__((noinline)) NcVal* rt_list_get(NcVal* lst, NcVal* idx_v) {
    u64* hp = (u64*)lst->val;
    u64 len = hp[0]; NcVal** items = (NcVal**)hp[2];
    u64 idx = idx_v->val;
    if (idx >= len) return nc_nil();
    return items[idx];
}

__attribute__((noinline)) void rt_list_set(NcVal* lst, NcVal* idx_v, NcVal* val) {
    u64* hp = (u64*)lst->val;
    u64 len = hp[0]; NcVal** items = (NcVal**)hp[2];
    u64 idx = idx_v->val;
    if (idx >= len) return;
    items[idx] = val;
}

__attribute__((noinline)) NcVal* rt_list_len(NcVal* lst) {
    return nc_int(*(u64*)lst->val);
}

__attribute__((noinline)) NcVal* rt_list_pop(NcVal* lst) {
    u64* hp = (u64*)lst->val;
    if (hp[0] == 0) return nc_nil();
    hp[0]--;
    return ((NcVal**)hp[2])[hp[0]];
}

__attribute__((noinline)) NcVal* rt_list_slice(NcVal* lst, NcVal* fv, NcVal* tv) {
    u64* hp = (u64*)lst->val;
    u64 len = hp[0]; NcVal** items = (NcVal**)hp[2];
    u64 f = fv->val, t = tv->val;
    if (f > len) f = len;
    if (t > len) t = len;
    NcVal* out = rt_list_new();
    u64 i = f;
    while (i < t) { rt_list_app(out, items[i]); i++; }
    return out;
}

__attribute__((noinline)) NcVal* rt_list_remove(NcVal* lst, NcVal* idx_v) {
    u64* hp = (u64*)lst->val;
    u64 len = hp[0]; NcVal** items = (NcVal**)hp[2];
    u64 idx = idx_v->val;
    if (idx >= len) return nc_nil();
    NcVal* removed = items[idx];
    u64 i = idx;
    while (i + 1 < len) { items[i] = items[i+1]; i++; }
    hp[0]--;
    return removed;
}

/* split: dele streng på sep → liste */
__attribute__((noinline)) NcVal* rt_split(NcVal* s, NcVal* sep) {
    NcVal* result = rt_list_new();
    u64 sl = str_len(s), sepl = str_len(sep);
    u8* sp = str_ptr(s); u8* sepp = str_ptr(sep);
    if (sepl == 0) {
        u64 i = 0;
        while (i < sl) { rt_list_app(result, nc_str_raw(sp+i, 1)); i++; }
        return result;
    }
    u64 start = 0, i = 0;
    while (i <= sl) {
        int match = 0;
        if (i + sepl <= sl) {
            u64 j = 0;
            while (j < sepl && sp[i+j] == sepp[j]) j++;
            match = (j == sepl);
        }
        if (match || i == sl) {
            rt_list_app(result, nc_str_raw(sp + start, i - start));
            start = i + sepl; i += sepl;
        } else i++;
    }
    return result;
}

__attribute__((noinline)) NcVal* rt_join(NcVal* lst, NcVal* sep) {
    u64* hp = (u64*)lst->val;
    u64 len = hp[0]; NcVal** items = (NcVal**)hp[2];
    if (len == 0) return nc_str_cstr((u8*)"");
    NcVal* acc = items[0];
    u64 i = 1;
    while (i < len) { acc = rt_concat(rt_concat(acc, sep), items[i]); i++; }
    return acc;
}

/* ── Map ─────────────────────────────────────────────────────────────────── */
/* Map struct: { u64 len; u64 cap; u64 keys_ptr; u64 vals_ptr } */
__attribute__((noinline)) NcVal* rt_map_new(void) {
    u64 cap = 8;
    u8* header = heap_alloc(32);  /* len, cap, keys_ptr, vals_ptr */
    NcVal** keys = (NcVal**)heap_alloc(cap * 8);
    NcVal** vals = (NcVal**)heap_alloc(cap * 8);
    *(u64*)header       = 0;
    *((u64*)header + 1) = cap;
    *((u64*)header + 2) = (u64)keys;
    *((u64*)header + 3) = (u64)vals;
    NcVal* v = nc_alloc(); v->type = NC_MAP; v->val = (u64)header;
    return v;
}

static int str_eq(NcVal* a, NcVal* b) {
    u64 la = str_len(a), lb = str_len(b);
    if (la != lb) return 0;
    u8* pa = str_ptr(a); u8* pb = str_ptr(b);
    u64 i = 0;
    while (i < la) { if (pa[i] != pb[i]) return 0; i++; }
    return 1;
}

static int val_eq_key(NcVal* a, NcVal* b) {
    if (a->type != b->type) return 0;
    if (a->type == NC_STR) return str_eq(a, b);
    return a->val == b->val;
}

__attribute__((noinline)) NcVal* rt_map_get(NcVal* m, NcVal* key) {
    u64* hp = (u64*)m->val;
    u64 len = hp[0];
    NcVal** keys = (NcVal**)hp[2];
    NcVal** vals = (NcVal**)hp[3];
    u64 i = 0;
    while (i < len) {
        if (val_eq_key(keys[i], key)) return vals[i];
        i++;
    }
    return nc_nil();
}

__attribute__((noinline)) void rt_map_set(NcVal* m, NcVal* key, NcVal* val) {
    u64* hp = (u64*)m->val;
    u64 len = hp[0], cap = hp[1];
    NcVal** keys = (NcVal**)hp[2];
    NcVal** vals = (NcVal**)hp[3];
    u64 i = 0;
    while (i < len) {
        if (val_eq_key(keys[i], key)) { vals[i] = val; return; }
        i++;
    }
    if (len >= cap) {
        u64 nc2 = cap * 2;
        NcVal** nk = (NcVal**)heap_alloc(nc2 * 8);
        NcVal** nv = (NcVal**)heap_alloc(nc2 * 8);
        u64 j = 0; while (j < len) { nk[j] = keys[j]; nv[j] = vals[j]; j++; }
        hp[1] = nc2; hp[2] = (u64)nk; hp[3] = (u64)nv;
        keys = nk; vals = nv;
    }
    keys[len] = key; vals[len] = val; hp[0] = len + 1;
}

__attribute__((noinline)) NcVal* rt_map_has(NcVal* m, NcVal* key) {
    u64* hp = (u64*)m->val;
    u64 len = hp[0]; NcVal** keys = (NcVal**)hp[2];
    u64 i = 0;
    while (i < len) { if (val_eq_key(keys[i], key)) return nc_bool(1); i++; }
    return nc_bool(0);
}

__attribute__((noinline)) NcVal* rt_map_keys(NcVal* m) {
    u64* hp = (u64*)m->val;
    u64 len = hp[0]; NcVal** keys = (NcVal**)hp[2];
    NcVal* lst = rt_list_new();
    u64 i = 0;
    while (i < len) { rt_list_app(lst, keys[i]); i++; }
    return lst;
}

__attribute__((noinline)) NcVal* rt_map_vals(NcVal* m) {
    u64* hp = (u64*)m->val;
    u64 len = hp[0]; NcVal** vals = (NcVal**)hp[3];
    NcVal* lst = rt_list_new();
    u64 i = 0;
    while (i < len) { rt_list_app(lst, vals[i]); i++; }
    return lst;
}

__attribute__((noinline)) void rt_map_del(NcVal* m, NcVal* key) {
    u64* hp = (u64*)m->val;
    u64 len = hp[0]; NcVal** keys = (NcVal**)hp[2]; NcVal** vals = (NcVal**)hp[3];
    u64 i = 0;
    while (i < len) {
        if (val_eq_key(keys[i], key)) {
            u64 j = i;
            while (j + 1 < len) { keys[j] = keys[j+1]; vals[j] = vals[j+1]; j++; }
            hp[0]--; return;
        }
        i++;
    }
}

/* ── INDEX_GET / INDEX_SET dispatch ──────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_index_get(NcVal* obj, NcVal* key) {
    if (obj->type == NC_LIST) return rt_list_get(obj, key);
    if (obj->type == NC_MAP)  return rt_map_get(obj, key);
    if (obj->type == NC_STR)  {
        /* Teikn-aksess: obj[i] → streng av lengde 1 */
        u64 len = str_len(obj); u64 idx = key->val;
        if (idx >= len) return nc_str_cstr((u8*)"");
        return nc_str_raw(str_ptr(obj) + idx, 1);
    }
    return nc_nil();
}

__attribute__((noinline)) NcVal* rt_index_set(NcVal* obj, NcVal* key, NcVal* val) {
    if (obj->type == NC_LIST) { rt_list_set(obj, key, val); return obj; }
    if (obj->type == NC_MAP)  { rt_map_set(obj, key, val); return obj; }
    return obj;
}

/* ── Fil-I/O (Linux syscalls) ────────────────────────────────────────────── */
static long syscall1(long n, long a1) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return r;
}
static long syscall2(long n, long a1, long a2) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return r;
}
static long syscall3(long n, long a1, long a2, long a3) {
    long r;
    register long r10 __asm__("r10") = a3;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    (void)r10; return r;
}
static long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    long r;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    register long r9  __asm__("r9")  = a6;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return r;
}

#define SYS_read   0
#define SYS_write  1
#define SYS_open   2
#define SYS_close  3
#define SYS_lseek  8
#define SYS_mmap   9
#define SYS_exit  60

#define O_RDONLY  0
#define O_WRONLY  1
#define O_CREAT   64
#define O_TRUNC   512
#define PROT_RW   3
#define MAP_ANON_PRIV 0x22

__attribute__((noinline)) NcVal* rt_skriv(NcVal* s) {
    u64 len = str_len(s); u8* ptr = str_ptr(s);
    syscall3(SYS_write, 1, (long)ptr, (long)len);
    return nc_nil();
}

__attribute__((noinline)) NcVal* rt_fil_les(NcVal* path) {
    u8 path_buf[4096]; u64 pl = str_len(path); u8* pp = str_ptr(path);
    if (pl >= 4096) pl = 4095;
    u64 i = 0; while (i < pl) { path_buf[i] = pp[i]; i++; } path_buf[pl] = 0;
    long fd = syscall3(SYS_open, (long)path_buf, O_RDONLY, 0);
    if (fd < 0) return nc_str_cstr((u8*)"");
    /* Finn filstorleik via lseek */
    long sz = syscall3(SYS_lseek, fd, 0, 2);
    syscall3(SYS_lseek, fd, 0, 0);
    if (sz <= 0) { syscall1(SYS_close, fd); return nc_str_cstr((u8*)""); }
    u8* buf = heap_alloc(8 + (u64)sz + 1);
    *(u64*)buf = (u64)sz;
    long got = 0;
    while (got < sz) {
        long r = syscall3(SYS_read, fd, (long)(buf + 8 + got), sz - got);
        if (r <= 0) break;
        got += r;
    }
    buf[8 + sz] = 0;
    *(u64*)buf = (u64)got;
    syscall1(SYS_close, fd);
    NcVal* v = nc_alloc(); v->type = NC_STR; v->val = (u64)buf;
    return v;
}

__attribute__((noinline)) NcVal* rt_fil_skriv(NcVal* path, NcVal* content) {
    u8 path_buf[4096]; u64 pl = str_len(path); u8* pp = str_ptr(path);
    if (pl >= 4096) pl = 4095;
    u64 i = 0; while (i < pl) { path_buf[i] = pp[i]; i++; } path_buf[pl] = 0;
    long fd = syscall3(SYS_open, (long)path_buf, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd < 0) return nc_nil();
    u64 len = str_len(content); u8* ptr = str_ptr(content);
    syscall3(SYS_write, fd, (long)ptr, (long)len);
    syscall1(SYS_close, fd);
    return nc_nil();
}

/* fil_skriv_binær: content er ei liste av bytes (heltall) */
__attribute__((noinline)) NcVal* rt_fil_skriv_binar(NcVal* path, NcVal* lst) {
    u8 path_buf[4096]; u64 pl = str_len(path); u8* pp = str_ptr(path);
    if (pl >= 4096) pl = 4095;
    u64 i = 0; while (i < pl) { path_buf[i] = pp[i]; i++; } path_buf[pl] = 0;
    long fd = syscall3(SYS_open, (long)path_buf, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd < 0) return nc_nil();
    u64* hp = (u64*)lst->val;
    u64 len = hp[0]; NcVal** items = (NcVal**)hp[2];
    /* Skriv i bulk-buffer */
    u8* wbuf = heap_alloc(len + 64);
    i = 0; while (i < len) { wbuf[i] = (u8)(items[i]->val & 0xFF); i++; }
    syscall3(SYS_write, fd, (long)wbuf, (long)len);
    syscall1(SYS_close, fd);
    return nc_nil();
}

/* ── Miljøvariablar (getenv via initial stack envp) ──────────────────────── */
/* envp er lagra av _start i HEAP_VA+HEAP_SZ-8 */
#define ENVP_STORE  (HEAP_VA + HEAP_SZ - 8)

__attribute__((noinline)) NcVal* rt_miljo_hent(NcVal* name) {
    u8** envp = *(u8***)ENVP_STORE;
    if (!envp) return nc_str_cstr((u8*)"");
    u8* np = str_ptr(name); u64 nl = str_len(name);
    u64 i = 0;
    while (envp[i]) {
        u8* e = envp[i];
        /* Samanlikn namn (fram til '=') */
        u64 j = 0;
        while (j < nl && e[j] == np[j]) j++;
        if (j == nl && e[j] == '=') {
            return nc_str_cstr(e + j + 1);
        }
        i++;
    }
    return nc_str_cstr((u8*)"");
}

__attribute__((noinline)) NcVal* rt_miljo_finnes(NcVal* name) {
    NcVal* v = rt_miljo_hent(name);
    return nc_bool(str_len(v) > 0);
}

/* ── fil_finnes ───────────────────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_fil_finnes(NcVal* path) {
    u8 path_buf[4096]; u64 pl = str_len(path); u8* pp = str_ptr(path);
    if (pl >= 4096) pl = 4095;
    u64 i = 0; while (i < pl) { path_buf[i] = pp[i]; i++; } path_buf[pl] = 0;
    long fd = syscall3(SYS_open, (long)path_buf, O_RDONLY, 0);
    if (fd < 0) return nc_bool(0);
    syscall1(SYS_close, fd);
    return nc_bool(1);
}

/* ── Samanlikning (type-aware) ────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_eq(NcVal* a, NcVal* b) {
    if (a->type != b->type) return nc_bool(0);
    if (a->type == NC_STR)  return nc_bool(str_eq(a, b));
    if (a->type == NC_NIL)  return nc_bool(1);
    return nc_bool(a->val == b->val);
}

/* ── BINARY_ADD: type-dispatch ────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_add(NcVal* a, NcVal* b) {
    if (a->type == NC_INT && b->type == NC_INT)
        return nc_int(a->val + b->val);
    /* Konverter til streng */
    NcVal* sa = (a->type == NC_STR) ? a : rt_int_to_str(a);
    NcVal* sb = (b->type == NC_STR) ? b : rt_int_to_str(b);
    return rt_concat(sa, sb);
}

/* ── JSON serialisering ────────────────────────────────────────────────────── */
/* Enkel rekursiv JSON-serialisering */
__attribute__((noinline)) NcVal* rt_json_str(NcVal* v);  /* forward */

static NcVal* json_str_escape(NcVal* s) {
    /* Minimal escape: " og \ og \n */
    u64 len = str_len(s); u8* p = str_ptr(s);
    /* Worst case: kvar teikn → 6 bytes (\\uXXXX) */
    u8* buf = heap_alloc(8 + len * 6 + 3);
    u64 dst = 0;
    buf[8 + dst++] = '"';
    u64 i = 0;
    while (i < len) {
        u8 c = p[i];
        if (c == '"')       { buf[8+dst++]='\\'; buf[8+dst++]='"'; }
        else if (c == '\\') { buf[8+dst++]='\\'; buf[8+dst++]='\\'; }
        else if (c == '\n') { buf[8+dst++]='\\'; buf[8+dst++]='n'; }
        else if (c == '\t') { buf[8+dst++]='\\'; buf[8+dst++]='t'; }
        else if (c == '\r') { buf[8+dst++]='\\'; buf[8+dst++]='r'; }
        else                { buf[8+dst++] = c; }
        i++;
    }
    buf[8+dst++] = '"';
    *(u64*)buf = dst;
    NcVal* v = nc_alloc(); v->type = NC_STR; v->val = (u64)buf;
    return v;
}

static NcVal* json_null_str(void) { return nc_str_cstr((u8*)"null"); }
static NcVal* json_comma(void)    { return nc_str_cstr((u8*)","); }
static NcVal* json_colon(void)    { return nc_str_cstr((u8*)":"); }

__attribute__((noinline)) NcVal* rt_json_str(NcVal* v) {
    if (!v || v->type == NC_NIL)  return json_null_str();
    if (v->type == NC_BOOL)       return v->val ? nc_str_cstr((u8*)"true") : nc_str_cstr((u8*)"false");
    if (v->type == NC_INT) {
        NcVal* s = rt_int_to_str(v); return s;
    }
    if (v->type == NC_STR) return json_str_escape(v);
    if (v->type == NC_LIST) {
        u64* hp = (u64*)v->val; u64 len = hp[0]; NcVal** items = (NcVal**)hp[2];
        NcVal* out = nc_str_cstr((u8*)"[");
        u64 i = 0;
        while (i < len) {
            if (i > 0) out = rt_concat(out, json_comma());
            out = rt_concat(out, rt_json_str(items[i]));
            i++;
        }
        return rt_concat(out, nc_str_cstr((u8*)"]"));
    }
    if (v->type == NC_MAP) {
        u64* hp = (u64*)v->val; u64 len = hp[0];
        NcVal** keys = (NcVal**)hp[2]; NcVal** vals = (NcVal**)hp[3];
        NcVal* out = nc_str_cstr((u8*)"{");
        u64 i = 0;
        while (i < len) {
            if (i > 0) out = rt_concat(out, json_comma());
            NcVal* k = (keys[i]->type == NC_STR) ? json_str_escape(keys[i]) : rt_json_str(keys[i]);
            out = rt_concat(out, rt_concat(k, json_colon()));
            out = rt_concat(out, rt_json_str(vals[i]));
            i++;
        }
        return rt_concat(out, nc_str_cstr((u8*)"}"));
    }
    return json_null_str();
}

/* ── JSON parsing ──────────────────────────────────────────────────────────── */
/* Mini JSON-parser: parser-state er ein index (u64) inn i strengen */
typedef struct { u8* src; u64 pos; u64 len; } JP;

static void jp_ws(JP* p) {
    while (p->pos < p->len) {
        u8 c = p->src[p->pos];
        if (c==' '||c=='\t'||c=='\n'||c=='\r') p->pos++; else break;
    }
}

static NcVal* jp_str(JP* p);
static NcVal* jp_val(JP* p);

static NcVal* jp_str(JP* p) {
    p->pos++;  /* skip " */
    u64 start = p->pos;
    /* Finn slutt-anførsels (med escape-handling) */
    u8* tmp = heap_alloc(str_len((NcVal*)0) + 65536 + 8);  /* uhm, problematisk */
    /* Enklare: tell lengde først, allokér, fyll */
    u64 est_pos = p->pos;
    u64 est_len = 0;
    while (est_pos < p->len && p->src[est_pos] != '"') {
        if (p->src[est_pos] == '\\') est_pos++;
        est_pos++; est_len++;
    }
    u8* buf = heap_alloc(8 + est_len + 1);
    u64 dst = 0;
    while (p->pos < p->len && p->src[p->pos] != '"') {
        u8 c = p->src[p->pos++];
        if (c == '\\') {
            c = p->src[p->pos++];
            if      (c == 'n') buf[8+dst++] = '\n';
            else if (c == 't') buf[8+dst++] = '\t';
            else if (c == 'r') buf[8+dst++] = '\r';
            else if (c == '\\') buf[8+dst++] = '\\';
            else if (c == '"') buf[8+dst++] = '"';
            else if (c == '/') buf[8+dst++] = '/';
            else { buf[8+dst++] = '\\'; buf[8+dst++] = c; }
        } else buf[8+dst++] = c;
    }
    if (p->pos < p->len) p->pos++;  /* skip closing " */
    *(u64*)buf = dst; buf[8+dst] = 0;
    NcVal* v = nc_alloc(); v->type = NC_STR; v->val = (u64)buf;
    return v;
}

static NcVal* jp_val(JP* p) {
    jp_ws(p);
    if (p->pos >= p->len) return nc_nil();
    u8 c = p->src[p->pos];
    if (c == '"') return jp_str(p);
    if (c == '{') {
        p->pos++;
        NcVal* m = rt_map_new();
        jp_ws(p);
        if (p->pos < p->len && p->src[p->pos] == '}') { p->pos++; return m; }
        while (p->pos < p->len) {
            jp_ws(p);
            if (p->src[p->pos] != '"') break;
            NcVal* k = jp_str(p);
            jp_ws(p);
            if (p->pos < p->len && p->src[p->pos] == ':') p->pos++;
            NcVal* v = jp_val(p);
            rt_map_set(m, k, v);
            jp_ws(p);
            if (p->pos < p->len && p->src[p->pos] == ',') p->pos++;
            else break;
        }
        jp_ws(p);
        if (p->pos < p->len && p->src[p->pos] == '}') p->pos++;
        return m;
    }
    if (c == '[') {
        p->pos++;
        NcVal* lst = rt_list_new();
        jp_ws(p);
        if (p->pos < p->len && p->src[p->pos] == ']') { p->pos++; return lst; }
        while (p->pos < p->len) {
            jp_ws(p);
            rt_list_app(lst, jp_val(p));
            jp_ws(p);
            if (p->pos < p->len && p->src[p->pos] == ',') p->pos++;
            else break;
        }
        jp_ws(p);
        if (p->pos < p->len && p->src[p->pos] == ']') p->pos++;
        return lst;
    }
    if (c == 't' && p->pos+4 <= p->len) { p->pos += 4; return nc_bool(1); }
    if (c == 'f' && p->pos+5 <= p->len) { p->pos += 5; return nc_bool(0); }
    if (c == 'n' && p->pos+4 <= p->len) { p->pos += 4; return nc_nil(); }
    if (c == '-' || (c >= '0' && c <= '9')) {
        long long v = 0; int neg = (c == '-');
        if (neg) p->pos++;
        while (p->pos < p->len && p->src[p->pos] >= '0' && p->src[p->pos] <= '9')
            v = v*10 + p->src[p->pos++] - '0';
        /* Skip decimal/exp */
        if (p->pos < p->len && p->src[p->pos] == '.') {
            p->pos++;
            while (p->pos < p->len && p->src[p->pos] >= '0' && p->src[p->pos] <= '9') p->pos++;
        }
        if (p->pos < p->len && (p->src[p->pos]=='e'||p->src[p->pos]=='E')) {
            p->pos++;
            if (p->pos < p->len && (p->src[p->pos]=='+'||p->src[p->pos]=='-')) p->pos++;
            while (p->pos < p->len && p->src[p->pos] >= '0' && p->src[p->pos] <= '9') p->pos++;
        }
        return nc_int((u64)(neg ? -v : v));
    }
    return nc_nil();
}

__attribute__((noinline)) NcVal* rt_json_parse(NcVal* s) {
    JP p;
    p.src = str_ptr(s); p.pos = 0; p.len = str_len(s);
    return jp_val(&p);
}

/* ── Konvertering ─────────────────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_to_tekst(NcVal* v) {
    if (v->type == NC_STR)  return v;
    if (v->type == NC_INT)  return rt_int_to_str(v);
    if (v->type == NC_BOOL) return v->val ? nc_str_cstr((u8*)"sann") : nc_str_cstr((u8*)"usann");
    if (v->type == NC_NIL)  return nc_str_cstr((u8*)"ingenting");
    return rt_json_str(v);
}

__attribute__((noinline)) NcVal* rt_to_int(NcVal* v) {
    if (v->type == NC_INT)  return v;
    if (v->type == NC_STR)  return rt_str_to_int(v);
    if (v->type == NC_BOOL) return nc_int(v->val);
    return nc_int(0);
}

__attribute__((noinline)) NcVal* rt_feil(NcVal* msg) {
    NcVal* s = rt_to_tekst(msg);
    rt_skriv(nc_str_cstr((u8*)"feil: "));
    rt_skriv(s);
    rt_skriv(nc_str_cstr((u8*)"\n"));
    syscall1(SYS_exit, 1);
    return nc_nil();
}

/* ── Lengde: type-dispatch ────────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_lengde(NcVal* v) {
    if (v->type == NC_LIST) return rt_list_len(v);
    if (v->type == NC_STR)  return nc_int(str_len(v));
    if (v->type == NC_MAP)  return nc_int(*(u64*)v->val);
    return nc_int(0);
}

/* ── kast (throw) ─────────────────────────────────────────────────────────── */
__attribute__((noinline)) void rt_kast(NcVal* msg) {
    NcVal* s = rt_to_tekst(msg);
    rt_skriv(nc_str_cstr((u8*)"norscode: "));
    rt_skriv(s); rt_skriv(nc_str_cstr((u8*)"\n"));
    syscall1(SYS_exit, 1);
}

/* ── Heap-initialisering (kall frå _start) ────────────────────────────────── */
__attribute__((noinline)) void rt_init_heap(void) {
    /* mmap 64MB heap */
    long addr = syscall6(SYS_mmap, HEAP_VA, HEAP_SZ, PROT_RW, MAP_ANON_PRIV, -1, 0);
    /* heap_next = HEAP_VA + 8 */
    *(u64*)HEAP_VA = HEAP_VA + 8;
}

/* ── Sannhetsverdi (truthiness) ───────────────────────────────────────────── */
__attribute__((noinline)) long rt_truthy(NcVal* v) {
    if (!v || v->type == NC_NIL)  return 0;
    if (v->type == NC_BOOL)       return v->val ? 1 : 0;
    if (v->type == NC_INT)        return v->val != 0 ? 1 : 0;
    if (v->type == NC_STR)        return str_len(v) > 0 ? 1 : 0;
    return 1;  /* lister og maps er alltid sanne */
}

__attribute__((noinline)) NcVal* rt_not(NcVal* v) {
    return nc_bool(rt_truthy(v) ? 0 : 1);
}

__attribute__((noinline)) NcVal* rt_ne(NcVal* a, NcVal* b) {
    return nc_bool(rt_truthy(rt_eq(a, b)) ? 0 : 1);
}

__attribute__((noinline)) NcVal* rt_lt(NcVal* a, NcVal* b) {
    if (a->type == NC_INT && b->type == NC_INT)
        return nc_bool((long long)a->val < (long long)b->val ? 1 : 0);
    /* Streng-samanlikning */
    if (a->type == NC_STR && b->type == NC_STR) {
        u64 la = str_len(a), lb = str_len(b);
        u8* pa = str_ptr(a); u8* pb = str_ptr(b);
        u64 n = la < lb ? la : lb;
        u64 i = 0;
        while (i < n) { if (pa[i] != pb[i]) return nc_bool(pa[i] < pb[i] ? 1 : 0); i++; }
        return nc_bool(la < lb ? 1 : 0);
    }
    return nc_bool(0);
}

__attribute__((noinline)) NcVal* rt_gt(NcVal* a, NcVal* b)  { return rt_lt(b, a); }

__attribute__((noinline)) NcVal* rt_le(NcVal* a, NcVal* b) {
    return nc_bool(rt_truthy(rt_lt(a, b)) || rt_truthy(rt_eq(a, b)) ? 1 : 0);
}

__attribute__((noinline)) NcVal* rt_ge(NcVal* a, NcVal* b) {
    return nc_bool(rt_truthy(rt_gt(a, b)) || rt_truthy(rt_eq(a, b)) ? 1 : 0);
}

__attribute__((noinline)) NcVal* rt_sub(NcVal* a, NcVal* b) {
    return nc_int((u64)((long long)a->val - (long long)b->val));
}
__attribute__((noinline)) NcVal* rt_mul(NcVal* a, NcVal* b) {
    return nc_int((u64)((long long)a->val * (long long)b->val));
}
__attribute__((noinline)) NcVal* rt_div(NcVal* a, NcVal* b) {
    if (b->val == 0) return nc_int(0);
    return nc_int((u64)((long long)a->val / (long long)b->val));
}
__attribute__((noinline)) NcVal* rt_mod(NcVal* a, NcVal* b) {
    if (b->val == 0) return nc_int(0);
    return nc_int((u64)((long long)a->val % (long long)b->val));
}
__attribute__((noinline)) NcVal* rt_neg(NcVal* a) {
    return nc_int((u64)(-(long long)a->val));
}

/* ── Hjelpefunksjonar for BUILD_MAP ──────────────────────────────────────── */
/* build_map_n: pop 2n (key/val pairs alternating) frå stack og bygg map */
/* — denne er kompleks å kalle direkte, men vi kan kalle rt_map_new + rt_map_set */

/* ── Streng til boolsk tekst ──────────────────────────────────────────────── */
__attribute__((noinline)) NcVal* rt_bool_to_str(NcVal* v) {
    return v->val ? nc_str_cstr((u8*)"sann") : nc_str_cstr((u8*)"usann");
}

/* ── Hjelpar for BUILD_LIST: tar reversed array frå stack ────────────────── */
/* items[0] = item[n-1] (sist pusha), items[n-1] = item[0] (fyrst pusha)   */
__attribute__((noinline)) NcVal* rt_build_list_rev(NcVal** items_rev, u64 n) {
    NcVal* lst = rt_list_new();
    /* Append i omvendt rekkjefølge for å få riktig liste-rekkjefølge */
    long long i = (long long)n - 1;
    while (i >= 0) {
        rt_list_app(lst, items_rev[i]);
        i--;
    }
    return lst;
}

/* BUILD_MAP: items_rev har alternerende val, key (sist pusha = val for siste par) */
/* For BUILD_MAP n: stack har [key0, val0, key1, val1, ..., key(n-1), val(n-1)] */
/* top = val(n-1), neste = key(n-1), ..., bottom = key0                         */
__attribute__((noinline)) NcVal* rt_build_map_rev(NcVal** items_rev, u64 n) {
    /* items_rev[0] = val(n-1), items_rev[1] = key(n-1), ... */
    NcVal* m = rt_map_new();
    u64 i = 0;
    while (i < n) {
        NcVal* val = items_rev[i * 2];
        NcVal* key = items_rev[i * 2 + 1];
        rt_map_set(m, key, val);
        i++;
    }
    return m;
}

/* ── Throw-handler: skriv til stderr og exit ─────────────────────────────── */
__attribute__((noinline)) void rt_throw(NcVal* msg) {
    NcVal* s = rt_to_tekst(msg);
    /* Skriv til stderr (fd=2) */
    u64 len = str_len(s); u8* ptr = str_ptr(s);
    syscall3(SYS_write, 2, (long)ptr, (long)len);
    syscall3(SYS_write, 2, (long)(u8*)"\n", 1);
    syscall1(SYS_exit, 1);
}
