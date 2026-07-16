/* Optional Metal tensor backend. The C ABI stays portable; non-Apple builds
 * report unavailable and continue through the scalar/SIMD tensor path. */

#if defined(__APPLE__)
#include <dlfcn.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stddef.h>
#include <stdint.h>

typedef void *NcMetalId;
typedef struct { unsigned long width; unsigned long height; unsigned long depth; } NcMetalSize;
typedef NcMetalId (*NcMetalMsg0)(NcMetalId, SEL);
typedef NcMetalId (*NcMetalMsg1)(NcMetalId, SEL, NcMetalId);
typedef NcMetalId (*NcMetalMsg2)(NcMetalId, SEL, NcMetalId, NcMetalId);
typedef NcMetalId (*NcMetalMsg3)(NcMetalId, SEL, NcMetalId, NcMetalId, NcMetalId);
typedef NcMetalId (*NcMetalMsgSource)(NcMetalId, SEL, NcMetalId, NcMetalId, NcMetalId);
typedef NcMetalId (*NcMetalMsgSize)(NcMetalId, SEL, NcMetalSize, NcMetalSize);
typedef NcMetalId (*NcMetalCreateDevice)(void);

static NcMetalId nc_metal_msg0(NcMetalId object, const char *name) {
    return ((NcMetalMsg0)objc_msgSend)(object, sel_registerName(name));
}

static NcMetalId nc_metal_msg1(NcMetalId object, const char *name, NcMetalId a) {
    return ((NcMetalMsg1)objc_msgSend)(object, sel_registerName(name), a);
}

static NcMetalId nc_metal_msg2(NcMetalId object, const char *name, NcMetalId a, NcMetalId b) {
    return ((NcMetalMsg2)objc_msgSend)(object, sel_registerName(name), a, b);
}

static NcMetalId nc_metal_msg3(NcMetalId object, const char *name,
                               NcMetalId a, NcMetalId b, NcMetalId c) {
    return ((NcMetalMsg3)objc_msgSend)(object, sel_registerName(name), a, b, c);
}

static NcMetalId nc_metal_msg_source(NcMetalId object, const char *name,
                                     NcMetalId source, NcMetalId options, NcMetalId error) {
    return ((NcMetalMsgSource)objc_msgSend)(object, sel_registerName(name), source, options, error);
}

static NcMetalId nc_metal_msg_size(NcMetalId object, const char *name,
                                   NcMetalSize grid, NcMetalSize group) {
    return ((NcMetalMsgSize)objc_msgSend)(object, sel_registerName(name), grid, group);
}

static void nc_metal_release(NcMetalId object) {
    if (object) (void)nc_metal_msg0(object, "release");
}

int nc_metal_available(void) {
    void *framework = dlopen("/System/Library/Frameworks/Metal.framework/Metal", RTLD_LAZY | RTLD_LOCAL);
    if (!framework) return 0;
    NcMetalCreateDevice create_device = (NcMetalCreateDevice)dlsym(framework, "MTLCreateSystemDefaultDevice");
    return create_device && create_device() != NULL;
}

int nc_metal_matmul_i32(const int32_t *left, const int32_t *right,
                        int32_t *output, int rows, int inner, int columns) {
    if (!left || !right || !output || rows <= 0 || inner <= 0 || columns <= 0) return -1;
    void *framework = dlopen("/System/Library/Frameworks/Metal.framework/Metal", RTLD_LAZY | RTLD_LOCAL);
    if (!framework) return -2;
    NcMetalCreateDevice create_device = (NcMetalCreateDevice)dlsym(framework, "MTLCreateSystemDefaultDevice");
    if (!create_device) return -3;
    NcMetalId device = create_device();
    if (!device) return -4;

    const char *kernel_source =
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "kernel void nc_matmul(device const int* a [[buffer(0)]], "
        "device const int* b [[buffer(1)]], device int* c [[buffer(2)]], "
        "constant uint& inner [[buffer(3)]], constant uint& columns [[buffer(4)]], "
        "uint gid [[thread_position_in_grid]]) { "
        "uint row = gid / columns; uint col = gid % columns; int sum = 0; "
        "for (uint k = 0; k < inner; k++) sum += a[row * inner + k] * b[k * columns + col]; "
        "c[gid] = sum; }\n";
    NcMetalId string_class = (NcMetalId)objc_getClass("NSString");
    NcMetalId source_string = ((NcMetalId(*)(NcMetalId, SEL, const char *))objc_msgSend)(
        string_class, sel_registerName("stringWithUTF8String:"), kernel_source);
    NcMetalId library = nc_metal_msg_source(device, "newLibraryWithSource:options:error:", source_string, NULL, NULL);
    if (!library) return -5;
    NcMetalId function_name = ((NcMetalId(*)(NcMetalId, SEL, const char *))objc_msgSend)(
        string_class, sel_registerName("stringWithUTF8String:"), "nc_matmul");
    NcMetalId function = nc_metal_msg1(library, "newFunctionWithName:", function_name);
    if (!function) { nc_metal_release(library); return -6; }
    NcMetalId pipeline = nc_metal_msg_source(device, "newComputePipelineStateWithFunction:error:", function, NULL, NULL);
    if (!pipeline) { nc_metal_release(function); nc_metal_release(library); return -7; }
    NcMetalId queue = nc_metal_msg0(device, "newCommandQueue");
    NcMetalId command = nc_metal_msg0(queue, "commandBuffer");
    NcMetalId encoder = nc_metal_msg0(command, "computeCommandEncoder");
    if (!queue || !command || !encoder) {
        nc_metal_release(pipeline); nc_metal_release(function); nc_metal_release(library); return -8;
    }

    size_t left_bytes = (size_t)rows * (size_t)inner * sizeof(int32_t);
    size_t right_bytes = (size_t)inner * (size_t)columns * sizeof(int32_t);
    size_t output_bytes = (size_t)rows * (size_t)columns * sizeof(int32_t);
    NcMetalId options = NULL; /* MTLResourceStorageModeShared */
    NcMetalId left_buffer = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)left, (NcMetalId)(uintptr_t)left_bytes, options);
    NcMetalId right_buffer = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)right, (NcMetalId)(uintptr_t)right_bytes, options);
    NcMetalId output_buffer = nc_metal_msg2(device, "newBufferWithLength:options:",
        (NcMetalId)(uintptr_t)output_bytes, options);
    if (!left_buffer || !right_buffer || !output_buffer) return -9;
    uint32_t inner_value = (uint32_t)inner, columns_value = (uint32_t)columns;
    NcMetalId inner_buffer = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)&inner_value, (NcMetalId)(uintptr_t)sizeof(inner_value), options);
    NcMetalId columns_buffer = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)&columns_value, (NcMetalId)(uintptr_t)sizeof(columns_value), options);
    nc_metal_msg1(encoder, "setComputePipelineState:", pipeline);
    nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", left_buffer, NULL, NULL);
    nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", right_buffer, NULL, (NcMetalId)(uintptr_t)1);
    nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", output_buffer, NULL, (NcMetalId)(uintptr_t)2);
    nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", inner_buffer, NULL, (NcMetalId)(uintptr_t)3);
    nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", columns_buffer, NULL, (NcMetalId)(uintptr_t)4);
    NcMetalSize grid = {(unsigned long)(rows * columns), 1, 1};
    NcMetalSize group = {1, 1, 1};
    nc_metal_msg_size(encoder, "dispatchThreads:threadsPerThreadgroup:", grid, group);
    nc_metal_msg0(encoder, "endEncoding");
    nc_metal_msg0(command, "commit");
    nc_metal_msg0(command, "waitUntilCompleted");
    NcMetalId contents = nc_metal_msg0(output_buffer, "contents");
    if (!contents) return -10;
    for (int i = 0; i < rows * columns; i++) output[i] = ((int32_t *)contents)[i];
    return 0;
}

/* Run the bounded diffusion step on the same Metal device as tensor matmul.
 * The caller owns the prompt hash and output buffer; this function only owns
 * transient shared Metal buffers and never changes the public media format. */
int nc_metal_diffusion_rgb(const int32_t *initial, int32_t *output,
                           int width, int height, int steps, int seed) {
    if (!initial || !output || width <= 0 || height <= 0 || steps <= 0) return -1;
    if ((long long)width * (long long)height > 4194304LL || steps > 100) return -2;
    void *framework = dlopen("/System/Library/Frameworks/Metal.framework/Metal", RTLD_LAZY | RTLD_LOCAL);
    if (!framework) return -3;
    NcMetalCreateDevice create_device = (NcMetalCreateDevice)dlsym(framework, "MTLCreateSystemDefaultDevice");
    if (!create_device) return -4;
    NcMetalId device = create_device();
    if (!device) return -5;

    const char *kernel_source =
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "kernel void nc_diffusion_step(device const int* src [[buffer(0)]], "
        "device int* dst [[buffer(1)]], constant uint& width [[buffer(2)]], "
        "constant uint& height [[buffer(3)]], constant uint& round [[buffer(4)]], "
        "constant int& seed [[buffer(5)]], uint gid [[thread_position_in_grid]]) { "
        "uint pixels = width * height; if (gid >= pixels * 3) return; "
        "uint pixel = gid / 3; uint channel = gid % 3; "
        "uint x = pixel % width; uint y = pixel / width; "
        "int sum = src[gid] * 2; int count = 2; "
        "if (x > 0) { sum += src[((pixel - 1) * 3) + channel]; count++; } "
        "if (x + 1 < width) { sum += src[((pixel + 1) * 3) + channel]; count++; } "
        "if (y > 0) { sum += src[((pixel - width) * 3) + channel]; count++; } "
        "if (y + 1 < height) { sum += src[((pixel + width) * 3) + channel]; count++; } "
        "int n = seed; n = (n + int(x) * 374761393) % 2147483647; "
        "n = (n + int(y) * 668265263) % 2147483647; "
        "n = (n + int(channel) * 214013) % 2147483647; "
        "n = (n + int(round) * 122949829) % 2147483647; "
        "int noise = (n % 511) - 255; int steering = 128 + noise / 8; "
        "int value = (sum + steering) / (count + 1); "
        "dst[gid] = clamp(value, 0, 255); }\n";
    NcMetalId string_class = (NcMetalId)objc_getClass("NSString");
    NcMetalId source_string = ((NcMetalId(*)(NcMetalId, SEL, const char *))objc_msgSend)(
        string_class, sel_registerName("stringWithUTF8String:"), kernel_source);
    NcMetalId library = nc_metal_msg_source(device, "newLibraryWithSource:options:error:", source_string, NULL, NULL);
    if (!library) return -6;
    NcMetalId function_name = ((NcMetalId(*)(NcMetalId, SEL, const char *))objc_msgSend)(
        string_class, sel_registerName("stringWithUTF8String:"), "nc_diffusion_step");
    NcMetalId function = nc_metal_msg1(library, "newFunctionWithName:", function_name);
    if (!function) { nc_metal_release(library); return -7; }
    NcMetalId pipeline = nc_metal_msg_source(device, "newComputePipelineStateWithFunction:error:", function, NULL, NULL);
    if (!pipeline) { nc_metal_release(function); nc_metal_release(library); return -8; }
    NcMetalId queue = nc_metal_msg0(device, "newCommandQueue");
    if (!queue) { nc_metal_release(pipeline); nc_metal_release(function); nc_metal_release(library); return -9; }

    size_t value_count = (size_t)width * (size_t)height * 3u;
    size_t value_bytes = value_count * sizeof(int32_t);
    NcMetalId current = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)initial, (NcMetalId)(uintptr_t)value_bytes, NULL);
    NcMetalId next = nc_metal_msg2(device, "newBufferWithLength:options:",
        (NcMetalId)(uintptr_t)value_bytes, NULL);
    if (!current || !next) {
        nc_metal_release(next); nc_metal_release(current); nc_metal_release(queue);
        nc_metal_release(pipeline); nc_metal_release(function); nc_metal_release(library);
        return -10;
    }
    uint32_t width_value = (uint32_t)width, height_value = (uint32_t)height;
    int32_t seed_value = seed;
    NcMetalId width_buffer = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)&width_value, (NcMetalId)(uintptr_t)sizeof(width_value), NULL);
    NcMetalId height_buffer = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)&height_value, (NcMetalId)(uintptr_t)sizeof(height_value), NULL);
    NcMetalId round_buffer = nc_metal_msg2(device, "newBufferWithLength:options:",
        (NcMetalId)(uintptr_t)sizeof(uint32_t), NULL);
    NcMetalId seed_buffer = nc_metal_msg3(device, "newBufferWithBytes:length:options:",
        (NcMetalId)&seed_value, (NcMetalId)(uintptr_t)sizeof(seed_value), NULL);
    if (!width_buffer || !height_buffer || !round_buffer || !seed_buffer) {
        nc_metal_release(seed_buffer); nc_metal_release(round_buffer);
        nc_metal_release(height_buffer); nc_metal_release(width_buffer);
        nc_metal_release(next); nc_metal_release(current); nc_metal_release(queue);
        nc_metal_release(pipeline); nc_metal_release(function); nc_metal_release(library);
        return -11;
    }
    for (int step = 0; step < steps; step++) {
        uint32_t round_value = (uint32_t)step;
        NcMetalId round_contents = nc_metal_msg0(round_buffer, "contents");
        if (!round_contents) return -12;
        *(uint32_t *)round_contents = round_value;
        NcMetalId command = nc_metal_msg0(queue, "commandBuffer");
        NcMetalId encoder = command ? nc_metal_msg0(command, "computeCommandEncoder") : NULL;
        if (!command || !encoder) return -13;
        nc_metal_msg1(encoder, "setComputePipelineState:", pipeline);
        nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", current, NULL, NULL);
        nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", next, NULL, (NcMetalId)(uintptr_t)1);
        nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", width_buffer, NULL, (NcMetalId)(uintptr_t)2);
        nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", height_buffer, NULL, (NcMetalId)(uintptr_t)3);
        nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", round_buffer, NULL, (NcMetalId)(uintptr_t)4);
        nc_metal_msg3(encoder, "setBuffer:offset:atIndex:", seed_buffer, NULL, (NcMetalId)(uintptr_t)5);
        NcMetalSize grid = {(unsigned long)value_count, 1, 1};
        NcMetalSize group = {1, 1, 1};
        nc_metal_msg_size(encoder, "dispatchThreads:threadsPerThreadgroup:", grid, group);
        nc_metal_msg0(encoder, "endEncoding");
        nc_metal_msg0(command, "commit");
        nc_metal_msg0(command, "waitUntilCompleted");
        NcMetalId swap = current; current = next; next = swap;
    }
    NcMetalId contents = nc_metal_msg0(current, "contents");
    if (!contents) return -14;
    memcpy(output, contents, value_bytes);
    nc_metal_release(seed_buffer); nc_metal_release(round_buffer);
    nc_metal_release(height_buffer); nc_metal_release(width_buffer);
    nc_metal_release(next); nc_metal_release(current); nc_metal_release(queue);
    nc_metal_release(pipeline); nc_metal_release(function); nc_metal_release(library);
    return 0;
}
#else
int nc_metal_available(void) { return 0; }
int nc_metal_matmul_i32(const int32_t *left, const int32_t *right,
                        int32_t *output, int rows, int inner, int columns) {
    (void)left; (void)right; (void)output; (void)rows; (void)inner; (void)columns;
    return -1;
}
int nc_metal_diffusion_rgb(const int32_t *initial, int32_t *output,
                           int width, int height, int steps, int seed) {
    (void)initial; (void)output; (void)width; (void)height; (void)steps; (void)seed;
    return -1;
}
#endif
