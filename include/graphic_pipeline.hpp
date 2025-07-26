#pragma once
#include "util.hpp"
#include "vulkan_context.hpp"
#include "spirv_reflect.h"
#include <string>
#include "colorlog.h"

#define GRAPHICS_PIPELINE_CTX "Graphics Pipeline"


struct PushConstant{
    uint64_t modelBufferAddress;
    uint64_t materialBufferAddress;

};


class GraphicPipeline{

public:
    GraphicPipeline();
    GraphicPipeline(const vk_ctx& context, const std::string& p_vertexShader, const std::string& p_fragmentShader, const vk_instance_params& p_instance_params);
    ~GraphicPipeline();


    
    PushConstant pushConstant{};

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkShaderModule vertexShader;
    VkShaderModule fragmentShader;
    SpvReflectShaderModule vertexShaderReflect;
    SpvReflectShaderModule fragmentShaderReflect;

    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferAllocation;
    VkBuffer drawBuffer;
    VmaAllocation drawBufferAllocation;

    std::vector<VkBuffer> VPMatrixBuffer;
    std::vector<VmaAllocation> VPMatrixBufferAllocation;
    std::vector<void*> VPMatrixBufferMapped;

    std::vector<VkBuffer> ModelMatrixBuffer;
    std::vector<VmaAllocation> ModelMatrixBufferAllocation;
    std::vector<void*> ModelMatrixBufferMapped;

    std::vector<VkDrawIndexedIndirectCommand> drawCommands;

    std::vector<SpvReflectInterfaceVariable*> interfaceVariables;
    uint32_t strideSize = 0;
    uint32_t strideSizeInstance = 0;


    vk_ctx ctx;

    bool destroyed = false;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSetsVP;
    std::vector<VkDescriptorSet> descriptorSetsModel;
    std::vector<VkDescriptorSet> descriptorSetsTexture;

    std::vector<std::string> vertexAttributeList = {
        "pos",
        "normal",
        "UV",
        "color"
    };




    void createGraphicPipeline(const vk_ctx&, const vk_instance_params&);
    VkShaderModule createShaderModule(const vk_ctx& context, const std::vector<char>& code);
    VmaAllocationInfo createBuffer(VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& allocation);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset);
    void createDescriptorPool();
    void allocateDescriptorSets();
    void createUniformBuffers();
    bool isContaints(std::vector<std::string>& array, std::string element);

};