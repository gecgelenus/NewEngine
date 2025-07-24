#pragma once

#include "util.hpp"
#include "vulkan_context.hpp"
#include <iostream>

#include "vma.h"


class ResourceManager{

public:
    ResourceManager(vk_ctx& p_ctx);
    ~ResourceManager();

    void allocateMemoryPrimitives(std::vector<ObjectPrimitive>& primitives);
    void addPrimitives(std::vector<ObjectPrimitive>& primitives);
    void updateDrawBuffer();
    void updateInstanceBuffer();



    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;
    uint32_t vertexLast = 0;

    VkBuffer indexBuffer;
    VmaAllocation indexBufferAllocation;
    uint32_t indexLast = 0;


    VkBuffer instanceBuffer;
    VmaAllocation instanceBufferAllocation;

    VkBuffer drawBuffer;
    VmaAllocation drawBufferAllocation;

    vk_ctx& ctx;

    std::vector<ObjectPrimitive> primitiveList;
    std::vector<VkDrawIndexedIndirectCommand> drawCommands;
    std::vector<InstanceInfo> instanceInfos;

    size_t allocateIndexMemory(size_t size);
    size_t allocateVertexMemory(size_t stride, size_t size);


};