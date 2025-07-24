#include "graphic_pipeline.hpp"
#include <glm/glm.hpp>
#include <iostream>

GraphicPipeline::GraphicPipeline()
{
}

GraphicPipeline::GraphicPipeline(const vk_ctx &context, const std::string &p_vertexShader, const std::string &p_fragmentShader, const vk_instance_params &p_instance_params)
{
    ctx = context;
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
	createDescriptorPool();
	allocateDescriptorSets();
	createUniformBuffers();
    createGraphicPipeline(context, p_instance_params);
}

GraphicPipeline::~GraphicPipeline()
{

    spvReflectDestroyShaderModule(&vertexShaderReflect);
    spvReflectDestroyShaderModule(&fragmentShaderReflect);

    vkDestroyShaderModule(ctx.device, vertexShader, nullptr);
    vkDestroyShaderModule(ctx.device, fragmentShader, nullptr);

    vkDestroyPipeline(ctx.device, pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);


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

	VkDescriptorSetLayout layouts[] = { ctx.setCameraLayout , ctx.setModelMatLayout, ctx.setTextureLayout};


	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Accessible by VS and FS
	pushConstantRange.offset = 0; // Start from the beginning of the push constant block
	pushConstantRange.size = sizeof(PushConstant); // Size of your data

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
	pipelineLayoutInfo.pSetLayouts = layouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
    
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
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
    pipelineInfo.pNext = &pipeline_create;

	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        ALERT(GRAPHICS_PIPELINE_CTX, "Failed to create graphics pipeline.");
	}else{
        INFO(GRAPHICS_PIPELINE_CTX, "Graphics pipeline created");
    }

}

VmaAllocationInfo GraphicPipeline::createBuffer(VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& allocation)
{
	VmaAllocationInfo info;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = memoryType;
	vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &info);

	return info;
}

void GraphicPipeline::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = ctx.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset; // Optional
	copyRegion.dstOffset = dstOffset; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx.graphicsQueue); // TODO: Implement async transfer

	vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

void GraphicPipeline::createDescriptorPool()
{
	VkDescriptorPoolSize VPSize{};
	VPSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VPSize.descriptorCount = static_cast<uint32_t>(ctx.swapchainImageViews.size());

	VkDescriptorPoolSize ModelSize{};
	ModelSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ModelSize.descriptorCount = static_cast<uint32_t>(ctx.swapchainImageViews.size()) * 64;

	VkDescriptorPoolSize TextureSize{};
	TextureSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	TextureSize.descriptorCount = static_cast<uint32_t>(ctx.swapchainImageViews.size()) * 64;

	VkDescriptorPoolSize sizes[] = { VPSize, ModelSize, TextureSize};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 3;
	poolInfo.pPoolSizes = sizes;
	poolInfo.maxSets = static_cast<uint32_t>(ctx.swapchainImageViews.size()*3);

	if (vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool");
	}


}

void GraphicPipeline::allocateDescriptorSets()
{

	std::vector<VkDescriptorSetLayout> VPlayouts(ctx.swapchainImageViews.size(), ctx.setCameraLayout);


	VkDescriptorSetAllocateInfo VPallocInfo{};
	VPallocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	VPallocInfo.descriptorPool = descriptorPool;
	VPallocInfo.descriptorSetCount = static_cast<uint32_t>(ctx.swapchainImageViews.size());
	VPallocInfo.pSetLayouts = VPlayouts.data();

	descriptorSetsVP.resize(ctx.swapchainImageViews.size());
	if (vkAllocateDescriptorSets(ctx.device, &VPallocInfo, descriptorSetsVP.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	std::vector<VkDescriptorSetLayout> Modellayouts(ctx.swapchainImageViews.size(), ctx.setModelMatLayout);


	VkDescriptorSetAllocateInfo ModelallocInfo{};
	ModelallocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ModelallocInfo.descriptorPool = descriptorPool;
	ModelallocInfo.descriptorSetCount = static_cast<uint32_t>(ctx.swapchainImageViews.size());
	ModelallocInfo.pSetLayouts = Modellayouts.data();

	descriptorSetsModel.resize(ctx.swapchainImageViews.size());
	if (vkAllocateDescriptorSets(ctx.device, &ModelallocInfo, descriptorSetsModel.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}


	std::vector<VkDescriptorSetLayout> Texturelayouts(ctx.swapchainImageViews.size(), ctx.setTextureLayout);


	VkDescriptorSetAllocateInfo TextureAllocInfo{};
	TextureAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	TextureAllocInfo.descriptorPool = descriptorPool;
	TextureAllocInfo.descriptorSetCount = static_cast<uint32_t>(ctx.swapchainImageViews.size());
	TextureAllocInfo.pSetLayouts = Texturelayouts.data();

	descriptorSetsTexture.resize(ctx.swapchainImageViews.size());
	if (vkAllocateDescriptorSets(ctx.device, &TextureAllocInfo, descriptorSetsTexture.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}



}

void GraphicPipeline::createUniformBuffers()
{

	VkDeviceSize ModelBufferSize = sizeof(glm::mat4) * 64;

	for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++) {

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = ctx.camera.buffers[i]; // <--- Your actual camera uniform buffer
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(glm::mat4); // The size of the data in your buffer

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSetsVP[i]; // The descriptor set to update
		descriptorWrite.dstBinding = 0;                       // Matches layout(binding = 0) in shader
		descriptorWrite.dstArrayElement = 0;                  // If it's not an array, this is 0
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Matches layout in shader
		descriptorWrite.descriptorCount = 1;                  // Matches descriptorCount in layout
		descriptorWrite.pBufferInfo = &bufferInfo;            // Link to your buffer info
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);
	}



	ModelMatrixBuffer.resize(ctx.swapchainImageViews.size());
	ModelMatrixBufferAllocation.resize(ctx.swapchainImageViews.size());
	ModelMatrixBufferMapped.resize(ctx.swapchainImageViews.size());

	for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++) {
		

		VmaAllocationInfo info =  createBuffer(ModelBufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ModelMatrixBuffer[i], ModelMatrixBufferAllocation[i]);

			
		std::vector<VkDescriptorBufferInfo> bufferInfos(64);

		for (uint32_t j = 0; j < 64; j++) {
			bufferInfos[j].buffer = ModelMatrixBuffer[i];
			bufferInfos[j].offset = j * sizeof(glm::mat4);                        
			bufferInfos[j].range = sizeof(glm::mat4);          
		}

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSetsModel[i]; // The VkDescriptorSet for Set 1 (from previous discussion)
		descriptorWrite.dstBinding = 0;                         // Matches layout(binding = 0) in your shader
		descriptorWrite.dstArrayElement = 0;                    // <--- Start updating from the first element (index 0) of the array
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Matches type in shader
		descriptorWrite.descriptorCount = 64;                   // <--- The total number of elements in the array you are updating
		descriptorWrite.pBufferInfo = bufferInfos.data();       // <--- Pointer to the array of VkDescriptorBufferInfo
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);

		ModelMatrixBufferMapped[i] = info.pMappedData;
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