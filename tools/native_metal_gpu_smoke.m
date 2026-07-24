#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdio.h>

int main(void) {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            fprintf(stderr, "Metal device unavailable\n");
            return 2;
        }

        NSString *source = @""
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "kernel void add_i32(device const int* a [[buffer(0)]], "
            "device const int* b [[buffer(1)]], device int* out [[buffer(2)]], "
            "uint id [[thread_position_in_grid]]) { out[id] = a[id] + b[id]; }\n";
        NSError *error = nil;
        id<MTLLibrary> library = [device newLibraryWithSource:source options:nil error:&error];
        if (!library) {
            fprintf(stderr, "Metal shader compile failed: %s\n", error.localizedDescription.UTF8String);
            return 3;
        }
        id<MTLFunction> function = [library newFunctionWithName:@"add_i32"];
        id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
        if (!pipeline) {
            fprintf(stderr, "Metal pipeline creation failed: %s\n", error.localizedDescription.UTF8String);
            return 4;
        }

        int a[4] = {1, 2, 3, 4};
        int b[4] = {10, 20, 30, 40};
        int expected[4] = {11, 22, 33, 44};
        id<MTLBuffer> ba = [device newBufferWithBytes:a length:sizeof(a) options:MTLResourceStorageModeShared];
        id<MTLBuffer> bb = [device newBufferWithBytes:b length:sizeof(b) options:MTLResourceStorageModeShared];
        id<MTLBuffer> bout = [device newBufferWithLength:sizeof(a) options:MTLResourceStorageModeShared];
        id<MTLCommandQueue> queue = [device newCommandQueue];
        id<MTLCommandBuffer> command = [queue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [command computeCommandEncoder];
        [encoder setComputePipelineState:pipeline];
        [encoder setBuffer:ba offset:0 atIndex:0];
        [encoder setBuffer:bb offset:0 atIndex:1];
        [encoder setBuffer:bout offset:0 atIndex:2];
        [encoder dispatchThreads:MTLSizeMake(4, 1, 1) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];
        [encoder endEncoding];
        [command commit];
        [command waitUntilCompleted];

        int *out = (int *)bout.contents;
        for (int i = 0; i < 4; i++) {
            if (out[i] != expected[i]) {
                fprintf(stderr, "Metal result mismatch at %d: %d\n", i, out[i]);
                return 5;
            }
        }
        printf("[OK] Metal GPU compute: %s\n", device.name.UTF8String);
        return 0;
    }
}
