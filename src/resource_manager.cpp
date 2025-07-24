#include "resource_manager.hpp"

ResourceManager::ResourceManager(vk_ctx &p_ctx)
    :ctx(p_ctx)
{
 
    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_GB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    vertexBuffer, vertexBufferAllocation);

    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB*100, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		indexBuffer, indexBufferAllocation);

    CTX::AUX::createBuffer(ctx, (VkDeviceSize)(sizeof(VkDrawIndexedIndirectCommand) * 10000), VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		drawBuffer, drawBufferAllocation);
	
	CTX::AUX::createBuffer(ctx, (VkDeviceSize)(sizeof(InstanceInfo) * 10000), VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		instanceBuffer, instanceBufferAllocation);
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::allocateMemoryPrimitives(std::vector<ObjectPrimitive> &primitives)
{
    for(int i = 0; i < primitives.size(); i++){
        size_t vertexStart = allocateVertexMemory(primitives[i].stride, primitives[i].vertexCount);
        size_t indexStart = allocateIndexMemory(primitives[i].indices.size());

        primitives[i].startOffsetVertex = vertexStart;
        primitives[i].startOffsetIndex = indexStart;
        
        CTX::AUX::uploadData(ctx, primitives[i].dataBuffer, vertexBuffer, primitives[i].dataSize, primitives[i].startOffsetVertex*primitives[i].stride);
        CTX::AUX::uploadData(ctx, primitives[i].indices.data(), indexBuffer, primitives[i].indices.size(), primitives[i].indices.size()*sizeof(uint32_t));

    }
}

void ResourceManager::addPrimitives(std::vector<ObjectPrimitive> &primitives)
{

    allocateMemoryPrimitives(primitives);

    for(int i = 0; i < primitives.size(); i++){
        primitiveList.push_back(primitives[i]);
    }
}

void ResourceManager::updateDrawBuffer()
{
    drawCommands.clear();
    drawCommands.resize(primitiveList.size());

    for(int i = 0; i < drawCommands.size();i++){
        VkDrawIndexedIndirectCommand tmpCmd{};
        tmpCmd.firstIndex = primitiveList[i].startOffsetIndex;
        tmpCmd.firstInstance = 0;
        tmpCmd.indexCount = primitiveList[i].indices.size();
        tmpCmd.instanceCount = 1;
        tmpCmd.vertexOffset = primitiveList[i].startOffsetVertex;
        drawCommands[i] = tmpCmd;
    }

    CTX::AUX::uploadData(ctx, drawCommands.data(), drawBuffer, drawCommands.size() * sizeof(VkDrawIndexedIndirectCommand), 0);

}

void ResourceManager::updateInstanceBuffer()
{
    instanceInfos.clear();
    instanceInfos.resize(primitiveList.size());

    for(int i = 0; i < ctx.instanceInfos.size();i++){
        instanceInfos[i].materialIndex = primitiveList[i].materialIndex;
        instanceInfos[i].modelIndex = primitiveList[i].modelIndex;
    }   

    CTX::AUX::uploadData(ctx, instanceInfos.data(), instanceBuffer, instanceInfos.size() * sizeof(InstanceInfo), 0);
}

size_t ResourceManager::allocateIndexMemory(size_t size)
{

    ctx.indexLast += size;
    std::cout << "Index allocation --> start: " << ctx.indexLast - size << " -- size: " << size << std::endl;
    return ctx.indexLast - size;
}

size_t ResourceManager::allocateVertexMemory(size_t stride, size_t size)
{
    size_t start = 0;
    if(ctx.vertexLast % stride != 0){
        start = ctx.vertexLast / stride + stride;
    }else{
        start = ctx.vertexLast / stride;
    }
    ctx.vertexLast = start * stride;
    ctx.vertexLast += size * stride;
    std::cout << "Vertex allocation --> start: " << start*stride << " -- size: " << size*stride << " -- stride: " << stride << std::endl;

    return start;
}
