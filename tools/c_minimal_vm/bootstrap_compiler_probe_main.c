/* Libc entry for dual-embed bootstrap compiler probe (macOS / dev). */
extern int norcode_compiler_main(void);

int main(void) {
    return norcode_compiler_main();
}
