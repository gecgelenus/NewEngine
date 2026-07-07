#include "graphic_pipeline.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <chrono>


GraphicPipeline::GraphicPipeline(vk_ctx &context, const std::string &p_vertexShader, const std::string &p_fragmentShader, const vk_instance_params &p_instance_params)
	: ctx(context)
{
    std::vector<char> vertexCode = util::readFile(p_vertexShader);
    std::vector<char> fragmentCode = util::readFile(p_fragmentShader);

    vertexShader = createShaderModule(context, vertexCode);
    fragmentShader = createShaderModule(context, fragmentCode);

    SpvReflectResult result = spvReflectCreateShaderModule(vertexCode.size() * sizeof(char), vertexCode.data(), &vertexShaderReflect);
    if(result == SPV_REFLECT_RESULT_SUCCESS){
        VERBOSE(GRAPHICS_PIPELINE_CTX, "Created vertex shader module (SPIRV REFLECT)");
    }else{
        ALERT(GRAPHICS_PIPELINE_CTX, "Failed to create vertex shader module (SPIRV REFLECT)");
    }

    result = spvReflectCreateShaderModule(fragmentCode.size() * sizeof(char), fragmentCode.data(), &fragmentShaderReflect);
    if(result == SPV_REFLECT_RESULT_SUCCESS){
        VERBOSE(GRAPHICS_PIPELINE_CTX, "Created fragment shader module (SPIRV REFLECT)");
    }else{
        ALERT(GRAPHICS_PIPELINE_CTX, "Failed to create fragment shader module (SPIRV REFLECT)");
    }
    createGraphicPipeline(context, p_instance_params);

	id = ctx.pipelineIDNext;
	ctx.pipelineIDNext++;
}

GraphicPipeline::~GraphicPipeline()
{
	//spvReflectDestroyShaderModule(&vertexShaderReflect);
	//spvReflectDestroyShaderModule(&fragmentShaderReflect);

	vkDestroyShaderModule(ctx.device, vertexShader, nullptr);
	vkDestroyShaderModule(ctx.device, fragmentShader, nullptr);

	vkDestroyPipeline(ctx.device, pipeline, nullptr);

    
}

VkShaderModule GraphicPipeline::createShaderModule(const vk_ctx& context, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        ALERT(GRAPHICS_PIPELINE_CTX, "Failed to create shader module");
	}else{
        SUCCESS(GRAPHICS_PIPELINE_CTX, "Created shader module");
    }

	return shaderModule;
}

void GraphicPipeline::createGraphicPipeline(const vk_ctx& context, const vk_instance_params& p_instance_params){


    SpvReflectResult result;

    uint32_t var_count = 0;
    result = spvReflectEnumerateInputVariables(&vertexShaderReflect, &var_count, NULL);
    
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    std::vector<SpvReflectInterfaceVariable*> input_vars(var_count);

    result = spvReflectEnumerateInputVariables(&vertexShaderReflect, &var_count, input_vars.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);


    for(int i = 0; i < var_count; i++){
        VERBOSE(GRAPHICS_PIPELINE_CTX, "Input var: %s", input_vars[i]->name);
        VERBOSE(GRAPHICS_PIPELINE_CTX, "Input loc: %u", input_vars[i]->location);
        VERBOSE(GRAPHICS_PIPELINE_CTX, "Input format: %u", input_vars[i]->format);

    }

    std::vector<VkVertexInputAttributeDescription> inputs;

    size_t totalSize = 0;
    size_t totalSizeInstance = 0;

	std::sort(input_vars.begin(), input_vars.end(),
    [](const SpvReflectInterfaceVariable* a, const SpvReflectInterfaceVariable* b) {
        return a->location < b->location;
    });
    
    interfaceVariables.resize(var_count);



    for(int i = 0; i < var_count; i++){
        interfaceVariables[i] = input_vars[i];
    }




    for(int i = 0; i < var_count;i++){
		if(interfaceVariables[i] == nullptr){
			continue;
		}
		
		if(isContaints(vertexAttributeList, interfaceVariables[i]->name)){
			VkVertexInputAttributeDescription tmpDesc{};
			tmpDesc.binding = 0;
			tmpDesc.location = interfaceVariables[i]->location;
			tmpDesc.format = (VkFormat)interfaceVariables[i]->format;
			tmpDesc.offset = totalSize;
			
			std::cout << "Vertex attribute: " << interfaceVariables[i]->name << " on location " << interfaceVariables[i]->location << std::endl;
			

			inputs.push_back(tmpDesc);

			switch (tmpDesc.format)
			{
			case VK_FORMAT_R32G32_SFLOAT:
				totalSize += sizeof(glm::vec2);
				break;
			case VK_FORMAT_R32G32B32_SFLOAT:
				totalSize += sizeof(glm::vec3);
				break;
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				totalSize += sizeof(glm::vec4);
				break;
			default:
				WARNING(GRAPHICS_PIPELINE_CTX, "Undefined data type encountered on shaders while calculating offsets of inputs. Data will likely missaligned.");
				break;
			}
		}else{
			VkVertexInputAttributeDescription tmpDesc{};
			tmpDesc.binding = 1;
			tmpDesc.location = interfaceVariables[i]->location;
			tmpDesc.format = (VkFormat)interfaceVariables[i]->format;
			tmpDesc.offset = totalSizeInstance;

			std::cout << "Instance attribute: " << interfaceVariables[i]->name << " on location " << interfaceVariables[i]->location << std::endl;
			

			inputs.push_back(tmpDesc);

			switch (tmpDesc.format)
			{
			case VK_FORMAT_R32G32_SFLOAT:
				totalSizeInstance += sizeof(glm::vec2);
				break;
			case VK_FORMAT_R32G32B32_SFLOAT:
				totalSizeInstance += sizeof(glm::vec3);
				break;
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				totalSizeInstance += sizeof(glm::vec4);
				break;
			case VK_FORMAT_R32_SINT:
				totalSizeInstance += sizeof(int32_t);
				break;
			default:
				WARNING(GRAPHICS_PIPELINE_CTX, "Undefined data type encountered on shaders while calculating offsets of inputs. Data will likely missaligned.");
				break;
			}
		}
        
    }

	strideSize = totalSize;
	strideSizeInstance = totalSizeInstance;


    //SHADER CREATION

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertexShader;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentShader;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// DYNAMIC STATES

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// VERTEX INPUT

	VkVertexInputBindingDescription bindingDescriptionVertexData{};
	bindingDescriptionVertexData.binding = 0;
	bindingDescriptionVertexData.stride = totalSize;
	bindingDescriptionVertexData.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputBindingDescription bindingDescriptionInstanceData{};
	bindingDescriptionInstanceData.binding = 1;
	bindingDescriptionInstanceData.stride = totalSizeInstance;
	bindingDescriptionInstanceData.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	
	VkVertexInputBindingDescription descriptions[] = {bindingDescriptionVertexData, bindingDescriptionInstanceData};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 2;
	vertexInputInfo.pVertexBindingDescriptions = descriptions; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = inputs.size();
	vertexInputInfo.pVertexAttributeDescriptions = inputs.data(); // Optional

	// INPUT ASSEMBLY

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// VIEWPORT AND SCISSOR

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)context.swapchainExtent.width;
	viewport.height = (float)context.swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = context.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// RASTERIZER

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// DEPTH AND STENCIL

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;


	// MULTISAMPLING
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.minSampleShading = .2f;
	multisampling.rasterizationSamples = p_instance_params.msaaSamples;
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// COLOR BLENDING

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// PIPELINE LAYOUT


    
    VkPipelineRenderingCreateInfoKHR pipeline_create{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    pipeline_create.pNext                   = VK_NULL_HANDLE;
    pipeline_create.colorAttachmentCount    = 1;
    pipeline_create.pColorAttachmentFormats = &(context.swapchainImageFormat);
    pipeline_create.depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT_S8_UINT;
    pipeline_create.stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;


   
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = ctx.globalPipelineLayout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
    pipelineInfo.pNext = &pipeline_create;

	auto pipelineCreateStart = std::chrono::high_resolution_clock::now();

	VkResult pipelineCreateResult = vkCreateGraphicsPipelines(context.device, context.pipelineCache, 1, &pipelineInfo, nullptr, &pipeline);

	auto pipelineCreateEnd = std::chrono::high_resolution_clock::now();
	double pipelineCreateMs = std::chrono::duration<double, std::milli>(pipelineCreateEnd - pipelineCreateStart).count();
	std::cout << "Pipeline creation took: " << pipelineCreateMs << " ms" << std::endl;

	if (pipelineCreateResult != VK_SUCCESS) {
        ALERT(GRAPHICS_PIPELINE_CTX, "Failed to create graphics pipeline.");
	}else{
        INFO(GRAPHICS_PIPELINE_CTX, "Graphics pipeline created");
    }

}





bool GraphicPipeline::isContaints(std::vector<std::string> &array, std::string element)
{

	for(std::string x: array){
		if(element == x){
			return true;
		}
	}


    return false;
}