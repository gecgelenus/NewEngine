#pragma once
#include "util.hpp"
#include "vulkan_context.hpp"
#include "spirv_reflect.h"
#include <string>
#include "colorlog.h"

#define GRAPHICS_PIPELINE_CTX "Graphics Pipeline"





class GraphicPipeline{

public:
    GraphicPipeline();
    GraphicPipeline(const vk_ctx& context, const std::string& p_vertexShader, const std::string& p_fragmentShader, const vk_instance_params& p_instance_params);
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


    vk_ctx ctx;



    std::vector<std::string> vertexAttributeList = {
        "pos",
        "normal",
        "UV",
        "color"
    };




    void createGraphicPipeline(const vk_ctx&, const vk_instance_params&);
    VkShaderModule createShaderModule(const vk_ctx& context, const std::vector<char>& code);
    bool isContaints(std::vector<std::string>& array, std::string element);

};