const std = @import("std");
const builtin = @import("builtin");

// C ABI for the Norscode native runtime. The caller owns `out`.
export fn norscode_argon2id(
    password_ptr: [*]const u8,
    password_len: usize,
    salt_ptr: [*]const u8,
    salt_len: usize,
    memory_kib: u32,
    iterations: u32,
    parallelism: u32,
    out_ptr: [*]u8,
    out_len: usize,
) c_int {
    if (!builtin.single_threaded) return -3;
    if (password_len > std.math.maxInt(u32) or salt_len < 8 or
        salt_len > std.math.maxInt(u32) or out_len < 4 or
        out_len > 4096 or memory_kib < 8 or memory_kib > 1048576 or
        iterations < 1 or iterations > 32 or parallelism < 1 or parallelism > 16) {
        return -2;
    }

    // Native builds use -fsingle-threaded; Argon2 still honours lane count,
    // but avoids requiring an uninitialised async Io implementation.
    const io: std.Io = undefined;
    const params = std.crypto.pwhash.argon2.Params{
        .t = iterations,
        .m = memory_kib,
        .p = @intCast(parallelism),
    };
    const password = password_ptr[0..password_len];
    const salt = salt_ptr[0..salt_len];
    const output = out_ptr[0..out_len];
    std.crypto.pwhash.argon2.kdf(
        std.heap.page_allocator,
        output,
        password,
        salt,
        params,
        .argon2id,
        io,
    ) catch return -1;
    return 0;
}

test "RFC 9106 Argon2id vector" {
    var output: [32]u8 = undefined;
    const rc = norscode_argon2id(
        "password".ptr,
        "password".len,
        "somesalt".ptr,
        "somesalt".len,
        65536,
        2,
        1,
        &output,
        output.len,
    );
    try std.testing.expectEqual(@as(c_int, 0), rc);
    const expected = "09316115d5cf24ed5a15a31a3ba326e5cf32edc24702987c02b6566f61913cf7";
    var actual: [64]u8 = undefined;
    for (output, 0..) |byte, i| {
        const digits = "0123456789abcdef";
        actual[i * 2] = digits[byte >> 4];
        actual[i * 2 + 1] = digits[byte & 15];
    }
    try std.testing.expectEqualStrings(expected, &actual);
}
