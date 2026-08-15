#pragma once
#include "util.hpp"
#include "vulkan_context.hpp"
#include "spirv_reflect.h"
#include <string>
#include "colorlog.h"

#define GRAPHICS_PIPELINE_CTX "Graphics Pipeline"

#define GRAPHICS_DEPTH 0
#define GRAPHICS_COLOR 1




class GraphicPipeline{

public:
    GraphicPipeline(vk_ctx& context,const std::string& p_vertexShader, const std::string& p_fragmentShader, const vk_instance_params& p_instance_params);
    ~GraphicPipeline();


    
    PushConstant pushConstant{};

    VkPipeline pipeline;
    VkShaderModule vertexShader;
    VkShaderModule fragmentShader;
    SpvReflectShaderModule vertexShaderReflect;
    SpvReflectShaderModule fragmentShaderReflect;


    std::vector<SpvReflectInterfaceVariable*> interfaceVariables;
    uint32_t strideSize = 0;
    uint32_t strideSizeInstance = 0;


    vk_ctx& ctx;
    uint32_t id;



    std::vector<std::string> vertexAttributeList = {
        "pos",
        "normal",
        "UV",
        "color"
    };


    VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineMultisampleStateCreateInfo multisampling;

	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendAttachmentState IDAttachment;

    VkPipelineColorBlendAttachmentState colorAttachments[10];
    VkFormat attachmentFormats[10];


	VkPipelineColorBlendStateCreateInfo colorBlending;

    VkPipelineRenderingCreateInfoKHR pipeline_create;
	VkGraphicsPipelineCreateInfo pipelineInfo;



    void createGraphicPipeline(const vk_ctx&, const vk_instance_params&);
    void createGraphicPipelineShadowMap(const vk_ctx&, const vk_instance_params&);

    void setDefaultPipeline(const vk_ctx&, const vk_instance_params&);
    void enableIDAttachment();


    VkShaderModule createShaderModule(const vk_ctx& context, const std::vector<char>& code);
    bool isContaints(std::vector<std::string>& array, std::string element);

};