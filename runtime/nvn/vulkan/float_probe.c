// LARECOMP_VK_FLOAT_PROBE=1 (issue #8 diagnostics): at start, one tiny compute shader checks how this driver treats
// the float edge cases the translated shaders lean on (NaN in isnan / min / max / clamp, infinities from rsqrt / log2 /
// divides, denormals, half conversions, whether math after unpackHalf2x16 runs in fp16) and logs each result next to
// the IEEE answer NVIDIA and current drivers give. Inputs come from the buffer, so the compiler can't fold them.
// Compute, not fragment: a driver's fragment compiler may still differ, but the arithmetic rules are shared.
#include <stdlib.h>
#include <string.h>

#include "../../rt.h"
#include "shaders.h"

static const char probe_src[] =
    "#version 460\n"
    "layout(local_size_x = 1) in;\n"
    "layout(std430, set = 0, binding = 0) buffer B { uint i[16]; uint o[48]; } b;\n"
    "float saturate_(float x) { return isnan(x) ? 0.0f : clamp(x, 0.0f, 1.0f); }\n"
    "float fmin_(float a, float c) { return isnan(a) ? c : (isnan(c) ? a : min(a, c)); }\n"
    "float h2f_bits(uint h) { uint s = (h & 0x8000u) << 16, e = (h >> 10) & 0x1Fu, m = h & 0x3FFu;"
    " if (e == 0u) { float d = float(m) * 5.9604644775390625e-8; return s != 0u ? -d : d; }"
    " if (e == 31u) return uintBitsToFloat(s | 0x7F800000u | (m << 13));"
    " return uintBitsToFloat(s | ((e + 112u) << 23) | (m << 13)); }\n"
    "void main() {\n"
    "  float zero = uintBitsToFloat(b.i[0]), one = uintBitsToFloat(b.i[1]), nan = uintBitsToFloat(b.i[2]);\n"
    "  float inf = uintBitsToFloat(b.i[3]), neg = uintBitsToFloat(b.i[4]), den = uintBitsToFloat(b.i[5]);\n"
    "  float big = uintBitsToFloat(b.i[9]), k4096 = uintBitsToFloat(b.i[10]);\n"
    "  b.o[0] = isnan(nan) ? 1u : 0u;\n"
    "  b.o[1] = (floatBitsToUint(nan) & 0x7FFFFFFFu) > 0x7F800000u ? 1u : 0u;\n"
    "  b.o[2] = floatBitsToUint(min(nan, one));\n"
    "  b.o[3] = floatBitsToUint(max(nan, one));\n"
    "  b.o[4] = floatBitsToUint(clamp(nan, zero, one));\n"
    "  b.o[5] = floatBitsToUint(saturate_(nan));\n"
    "  b.o[6] = floatBitsToUint(fmin_(nan, one));\n"
    "  b.o[7] = (nan < one || isnan(nan) || isnan(one)) ? 1u : 0u;\n"
    "  b.o[8] = (nan != nan) ? 1u : 0u;\n"
    "  b.o[9] = floatBitsToUint(inversesqrt(zero));\n"
    "  b.o[10] = floatBitsToUint(log2(zero));\n"
    "  b.o[11] = floatBitsToUint(one / zero);\n"
    "  b.o[12] = floatBitsToUint(zero * inf);\n"
    "  b.o[13] = floatBitsToUint(exp2(-inf));\n"
    "  b.o[14] = floatBitsToUint(sqrt(neg));\n"
    "  b.o[15] = floatBitsToUint(den * one);\n"
    "  b.o[16] = floatBitsToUint(unpackHalf2x16(b.i[6]).x);\n"
    "  b.o[17] = floatBitsToUint(unpackHalf2x16(b.i[7]).x);\n"
    "  b.o[18] = floatBitsToUint(unpackHalf2x16(b.i[8]).x);\n"
    "  b.o[19] = packHalf2x16(vec2(big, zero));\n"
    "  b.o[20] = floatBitsToUint(unpackHalf2x16(b.i[11]).x * k4096 * k4096);\n"
    "  b.o[21] = floatBitsToUint(h2f_bits(b.i[11]) * k4096 * k4096);\n"
    "  b.o[22] = floatBitsToUint(normalize(vec3(zero)).x);\n"
    "  b.o[23] = floatBitsToUint(one / inf);\n"
    "}\n";

static uint32_t mem_type(uint32_t bits, VkMemoryPropertyFlags want) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(g_vk.phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want) return i;
    return UINT32_MAX;
}

void vk_float_probe(void) {
    const char *e = getenv("LARECOMP_VK_FLOAT_PROBE");
    if (!e || !atoi(e)) return;
    size_t nw = 0;
    uint32_t *spv = vk_glsl_to_spirv(probe_src, STAGE_COMPUTE, "float_probe", &nw);
    if (!spv) { LOG("[probe] float probe: compile failed\n"); return; }
    VkShaderModule mod = vk_module(spv, nw);
    free(spv);
    if (!mod) { LOG("[probe] float probe: no shader module\n"); return; }
    VkDevice dev = g_vk.dev;
    VkBuffer buf = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VkPipelineLayout pl = VK_NULL_HANDLE;
    VkPipeline pipe = VK_NULL_HANDLE;
    VkDescriptorPool dp = VK_NULL_HANDLE;
    VkCommandPool cp = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    uint32_t *map = NULL;
    const VkDeviceSize size = (16 + 48) * 4;
    VkBufferCreateInfo bi = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT};
    if (vkCreateBuffer(dev, &bi, NULL, &buf) != VK_SUCCESS) goto out;
    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(dev, buf, &mr);
    uint32_t mt = mem_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (mt == UINT32_MAX) goto out;
    VkMemoryAllocateInfo ma = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = mr.size, .memoryTypeIndex = mt};
    if (vkAllocateMemory(dev, &ma, NULL, &mem) != VK_SUCCESS || vkBindBufferMemory(dev, buf, mem, 0) != VK_SUCCESS) goto out;
    if (vkMapMemory(dev, mem, 0, size, 0, (void **)&map) != VK_SUCCESS) { map = NULL; goto out; }
    memset(map, 0xEE, size);
    static const uint32_t in[16] = {
        0x00000000u, 0x3F800000u, 0x7FC00000u, 0x7F800000u,  // 0, 1, NaN, +inf
        0xBF800000u, 0x00000001u,                            // -1, the smallest fp32 denormal
        0x7C00u, 0x0001u, 0x7E00u,                           // halves: +inf, the smallest denormal, NaN
        0x4788B800u, 0x45800000u,                            // 70000 (over the half range), 4096
        0x3C00u,                                             // half 1.0
    };
    memcpy(map, in, sizeof in);
    VkDescriptorSetLayoutBinding bd = {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL};
    VkDescriptorSetLayoutCreateInfo li = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &bd};
    if (vkCreateDescriptorSetLayout(dev, &li, NULL, &dsl) != VK_SUCCESS) goto out;
    VkPipelineLayoutCreateInfo pli = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &dsl};
    if (vkCreatePipelineLayout(dev, &pli, NULL, &pl) != VK_SUCCESS) goto out;
    VkComputePipelineCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                                      .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = mod, .pName = "main"},
                                      .layout = pl};
    if (vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &ci, NULL, &pipe) != VK_SUCCESS) goto out;
    VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
    VkDescriptorPoolCreateInfo dpi = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &ps};
    if (vkCreateDescriptorPool(dev, &dpi, NULL, &dp) != VK_SUCCESS) goto out;
    VkDescriptorSet set;
    VkDescriptorSetAllocateInfo dai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = dp,
                                       .descriptorSetCount = 1, .pSetLayouts = &dsl};
    if (vkAllocateDescriptorSets(dev, &dai, &set) != VK_SUCCESS) goto out;
    VkDescriptorBufferInfo dbi = {buf, 0, size};
    VkWriteDescriptorSet wr = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = 0, .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi};
    vkUpdateDescriptorSets(dev, 1, &wr, 0, NULL);
    VkCommandPoolCreateInfo cpi = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = g_vk.qfam};
    if (vkCreateCommandPool(dev, &cpi, NULL, &cp) != VK_SUCCESS) goto out;
    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo cai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = cp,
                                       .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
    if (vkAllocateCommandBuffers(dev, &cai, &cb) != VK_SUCCESS) goto out;
    VkCommandBufferBeginInfo cbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkBeginCommandBuffer(cb, &cbi);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pl, 0, 1, &set, 0, NULL);
    vkCmdDispatch(cb, 1, 1, 1);
    VkMemoryBarrier mb = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                          .dstAccessMask = VK_ACCESS_HOST_READ_BIT};
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &mb, 0, NULL, 0, NULL);
    vkEndCommandBuffer(cb);
    VkFenceCreateInfo fi = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateFence(dev, &fi, NULL, &fence) != VK_SUCCESS) goto out;
    VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb};
    if (vkQueueSubmit(g_vk.queue, 1, &si, fence) != VK_SUCCESS || vkWaitForFences(dev, 1, &fence, VK_TRUE, 5000000000ull) != VK_SUCCESS) {
        LOG("[probe] float probe: submission failed\n");
        goto out;
    }
    {
        // expected: IEEE results (what NVIDIA and current drivers give); "any" = undefined in GLSL, shown for comparison
        static const struct { const char *what, *expect; } row[] = {
            {"isnan(NaN)", "1"}, {"NaN by bits", "1"}, {"min(NaN, 1)", "any (3f800000 usual)"},
            {"max(NaN, 1)", "any (3f800000 usual)"}, {"clamp(NaN, 0, 1)", "any"}, {"saturate(NaN) (ours)", "0"},
            {"fmin(NaN, 1) (ours)", "3f800000"}, {"NaN < 1 || isnan", "1"}, {"NaN != NaN", "1"},
            {"inversesqrt(0)", "7f800000"}, {"log2(0)", "ff800000"}, {"1 / 0", "7f800000"}, {"0 * inf", "NaN (7fc00000 or ffc00000)"},
            {"exp2(-inf)", "0"}, {"sqrt(-1)", "NaN"}, {"fp32 denormal * 1", "1 (kept) or 0 (flushed)"},
            {"half +inf -> float", "7f800000"}, {"half denormal -> float", "33800000"}, {"half NaN -> float", "7fc00000"},
            {"packHalf(70000)", "7c00 (or 7bff)"}, {"half 1 -> float * 4096 * 4096", "4b800000 (7f800000: math ran in fp16)"},
            {"half 1 by bits * 4096 * 4096", "4b800000"}, {"normalize(0).x", "any (NaN usual)"}, {"1 / inf", "0"},
        };
        for (size_t i = 0; i < sizeof row / sizeof row[0]; i++)
            LOG("[probe] %-30s %08x   expected %s\n", row[i].what, map[16 + i], row[i].expect);
    }
out:
    if (fence) vkDestroyFence(dev, fence, NULL);
    if (cp) vkDestroyCommandPool(dev, cp, NULL);
    if (dp) vkDestroyDescriptorPool(dev, dp, NULL);
    if (pipe) vkDestroyPipeline(dev, pipe, NULL);
    if (pl) vkDestroyPipelineLayout(dev, pl, NULL);
    if (dsl) vkDestroyDescriptorSetLayout(dev, dsl, NULL);
    if (map) vkUnmapMemory(dev, mem);
    if (buf) vkDestroyBuffer(dev, buf, NULL);
    if (mem) vkFreeMemory(dev, mem, NULL);
    vkDestroyShaderModule(dev, mod, NULL);
}
