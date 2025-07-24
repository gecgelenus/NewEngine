#include "render_batch.hpp"
#include <iostream>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

RenderBatch::RenderBatch()
{
}

RenderBatch::RenderBatch(const vk_ctx& p_ctx, const std::string & p_name)
	: ctx(p_ctx)
{
    name = p_name;
    __RenderBatch();
}

void RenderBatch::__RenderBatch()
{


	cpuVertexBuffer.push_back(1.0f);
	cpuVertexBuffer.clear();

	cpuIndexBuffer.shrink_to_fit();

    createBuffer((VkDeviceSize)100000000, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    vertexBuffer, vertexBufferAllocation);

    createBuffer((VkDeviceSize)10000000, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		indexBuffer, indexBufferAllocation);

    createBuffer((VkDeviceSize)(sizeof(VkDrawIndexedIndirectCommand) * 10000), VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		drawBuffer, drawBufferAllocation);
	
	createBuffer((VkDeviceSize)(sizeof(InstanceInfo) * 10000), VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		instanceBuffer, instanceBufferAllocation);

	
}

RenderBatch::~RenderBatch()
{
    vmaFreeMemory(ctx.allocator, vertexBufferAllocation);
    vmaFreeMemory(ctx.allocator, indexBufferAllocation);
    vmaFreeMemory(ctx.allocator, drawBufferAllocation);
    vmaFreeMemory(ctx.allocator, instanceBufferAllocation);

	

}

void RenderBatch::addObject(Object &p_object)
{
	

	p_object.vertexIndex = cpuVertexBuffer.size();
	cpuVertexBuffer.insert(cpuVertexBuffer.end(), p_object.vertexData.begin(), p_object.vertexData.end());
	p_object.indexIndex = cpuIndexBuffer.size();
	cpuIndexBuffer.insert(cpuIndexBuffer.end(), p_object.indexData.begin(), p_object.indexData.end());
	objects.push_back(p_object);
	uploadData(cpuVertexBuffer.data(), vertexBuffer, cpuVertexBuffer.size() * sizeof(float), 0);
	uploadData(cpuIndexBuffer.data(), indexBuffer, cpuIndexBuffer.size() * sizeof(uint32_t), 0);
	updateDrawCommands();

}

void RenderBatch::readGLTF(std::string &path)
{
	
}

void RenderBatch::processGltfScene(tinygltf::Model& model) {
    

}

void RenderBatch::processGltfFile(std::string& path) {
    
	tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;


    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);

	if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
    printf("Failed to parse glTF\n");
    }


    const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

    std::cout << "Processing scene: " << scene.name << std::endl;
	LoadModelTextures(model, path);
	updateTextureSets();


    // Iterate through the root nodes of the scene
    for (int nodeIdx : scene.nodes) {
        const tinygltf::Node& node = model.nodes[nodeIdx];
        
        // Recursively process nodes and their children
		ObjectTransformation t;
		t.translation = glm::vec3(1.0f);
		t.rotation = glm::quat();
		t.scale = glm::vec3(1.0f);

        processNode(model, path, node, t); // Start with identity matrix for root

    }

}

// Recursive helper function to traverse the node hierarchy
void RenderBatch::processNode(tinygltf::Model& model, std::string& path, const tinygltf::Node& node, ObjectTransformation parentTransform) {
    
	Object obj(ctx);
	obj.modelIndex = ctx.modelMatrixCheck.size();

	ctx.modelMatrixCheck.push_back(true);
	ctx.modelMatrixList.push_back(glm::mat4(1.0f));

	obj.name = node.name;
    std::cout << "  Node: " << node.name << std::endl;

	obj.parentTransformation = parentTransform;

	obj.transformation.translation = glm::vec3(0.0f, 0.0f, 0.0f); // Default to no translation
    obj.transformation.rotation = glm::identity<glm::quat>();    // Default to no rotation (identity quaternion)
    obj.transformation.scale = glm::vec3(1.0f, 1.0f, 1.0f);      // Default to no scaling (identity scale)

    // Check if translation is provided, otherwise use default
    if (node.translation.size() == 3) {
        obj.transformation.translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
    }
    // Check if rotation is provided, otherwise use default
    if (node.rotation.size() == 4) {
        // Correctly construct glm::quat from glTF (x, y, z, w) order
        obj.transformation.rotation = glm::quat(
            node.rotation[3], // w
            node.rotation[0], // x
            node.rotation[1], // y
            node.rotation[2]  // z
        );
    }
    // Check if scale is provided, otherwise use default
    if (node.scale.size() == 3) {
        obj.transformation.scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
    }
    // If this node has a mesh, process it
    if (node.mesh > -1) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];
        std::cout << "    Associated Mesh: " << mesh.name << " (Index: " << node.mesh << ")" << std::endl;

        // Iterate over primitives within the mesh
        for (const auto& primitive : mesh.primitives) {

			ObjectPrimitive tmpPrimitive;
			tmpPrimitive.mode = primitive.mode;
            std::cout << "      Primitive (Mode: " << primitive.mode << ")" << std::endl;
            
			if (primitive.attributes.count("POSITION")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                // Ensure data type is float (most common for positions)
                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    std::cerr << "Warning: POSITION attribute is not float type. Skipping." << std::endl;
                    continue;
                }
                // Ensure it's a VEC3 (most common for positions)
                if (accessor.type != TINYGLTF_TYPE_VEC3) {
                     std::cerr << "Warning: POSITION attribute is not VEC3 type. Skipping." << std::endl;
                    continue;
                }

                // Calculate the start of the data within the buffer
                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                // Determine stride (bytes between consecutive vertices)
                size_t stride = accessor.ByteStride(bufferView); // Use accessor.ByteStride for correct stride

                // Iterate through vertices
                for (size_t i = 0; i < accessor.count; ++i) {
                    const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                    tmpPrimitive.vertices.emplace_back(values[0], values[1], values[2]);
                }
            }

            // --- NORMAL data (similar logic) ---
            if (primitive.attributes.count("NORMAL")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC3) {
                    std::cerr << "Warning: NORMAL attribute is not float VEC3. Skipping." << std::endl;
                    continue;
                }

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t stride = accessor.ByteStride(bufferView);

                for (size_t i = 0; i < accessor.count; ++i) {
                    const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                    tmpPrimitive.normals.emplace_back(values[0], values[1], values[2]);
                }
            }

            // --- TEXCOORD_0 data (similar logic) ---
            if (primitive.attributes.count("TEXCOORD_0")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC2) {
                    std::cerr << "Warning: TEXCOORD_0 attribute is not float VEC2. Skipping." << std::endl;
                    continue;
                }

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t stride = accessor.ByteStride(bufferView);

                for (size_t i = 0; i < accessor.count; ++i) {
                    const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                    tmpPrimitive.UV.emplace_back(values[0], values[1]);
                }
            }
            
            // You can also access primitive.material here:
            if (primitive.material > -1) {
                const tinygltf::Material& material = model.materials[primitive.material];
				tmpPrimitive.materialName = material.name;
				std::string tmpPath = path;
				int texIndex = findTextureIndex(tmpPath.append(std::to_string(model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source)));
                std::cout << "Material: " << material.name  << " uses texture " << tmpPath << std::endl;
				
				
				if(texIndex < 0){
					tmpPrimitive.materialIndex = 0;
				}else{
					tmpPrimitive.materialIndex = texIndex;
				}
			}

			if (primitive.indices > -1) { // Check if indices are defined for this primitive
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

                // Determine the type of indices and copy them
                switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                        const uint8_t* indices = reinterpret_cast<const uint8_t*>(dataPtr);
                        for (size_t i = 0; i < accessor.count; ++i) {
                            tmpPrimitive.indices.push_back(static_cast<uint32_t>(indices[i]));
                        }
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        const uint16_t* indices = reinterpret_cast<const uint16_t*>(dataPtr);
                        for (size_t i = 0; i < accessor.count; ++i) {
                            tmpPrimitive.indices.push_back(static_cast<uint32_t>(indices[i]));
                        }
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                        const uint32_t* indices = reinterpret_cast<const uint32_t*>(dataPtr);
                        // Can copy directly if all your indices are uint32_t
                        tmpPrimitive.indices.assign(indices, indices + accessor.count);
                        break;
                    }
                    default:
                        std::cerr << "Warning: Unsupported index component type: " << accessor.componentType << std::endl;
                        break;
                }

            }

			tmpPrimitive.modelIndex = obj.modelIndex;
			obj.primitives.push_back(tmpPrimitive);

		}
    }
	
	objects.push_back(obj);

    // Recursively process children nodes
	if(node.children.size() > 0){
		for (int childIdx : node.children) {
    		const tinygltf::Node& childNode = model.nodes[childIdx];
        	processNode(model, path, childNode, obj.transformation); // Pass the accumulated transform
    	}
	}
    
}

void RenderBatch::updateDrawCommands()
{
	drawCommands.clear();

	uint32_t currentInstance = 0;

	for(int i = 0; i < objects.size();i++){
		for(int j = 0; j < objects[i].primitives.size();j++){
			VkDrawIndexedIndirectCommand tmpCmd{};
			tmpCmd.firstIndex = objects[i].primitives[j].indexOffset;
			tmpCmd.firstInstance = currentInstance;
			tmpCmd.indexCount = objects[i].primitives[j].indices.size();
			tmpCmd.instanceCount = 1;
			tmpCmd.vertexOffset = objects[i].primitives[j].vertexOffset;
			std::cout << "Draw command --> Index count: " << tmpCmd.indexCount << 
			 " vertex offset: " << tmpCmd.vertexOffset << std::endl;

			drawCommands.push_back(tmpCmd);
			objects[i].primitives[j].modelIndex = currentInstance;
			currentInstance++;
		}
	}

	uploadData(drawCommands.data(), drawBuffer, drawCommands.size() * sizeof(VkDrawIndexedIndirectCommand), 0);
}

void RenderBatch::updateTextureSets()
{
	for (size_t frameIndex = 0; frameIndex < ctx.swapchainImageViews.size(); frameIndex++) {

            std::vector<VkDescriptorImageInfo> descriptorInfos;

            // Populate all 64 descriptorInfos
            for (int textureIndex = 0; textureIndex < ctx.textureSet.size(); textureIndex++) {


                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                if(ctx.textureSet[textureIndex].enabled){
				imageInfo.imageView = ctx.textureSet[textureIndex].textureImageView;
                imageInfo.sampler = ctx.textureSet[textureIndex].textureSampler; // <--- ASSIGN THE SAMPLER HERE!
				}else{
									imageInfo.imageView = ctx.textureSet[0].textureImageView;
                	imageInfo.sampler = ctx.textureSet[0].textureSampler; // <--- ASSIGN THE SAMPLER HERE!

				}

                descriptorInfos.push_back(imageInfo);
            }

            VkWriteDescriptorSet descriptorWriteTex{};
            descriptorWriteTex.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWriteTex.dstSet = graphicPipeline.descriptorSetsTexture[frameIndex]; // Use outer loop variable
            descriptorWriteTex.dstBinding = 0;
            descriptorWriteTex.dstArrayElement = 0;
            descriptorWriteTex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // <--- CHANGE TYPE HERE!
            descriptorWriteTex.descriptorCount = descriptorInfos.size();
            descriptorWriteTex.pImageInfo = descriptorInfos.data();
            descriptorWriteTex.pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(ctx.device, 1, &descriptorWriteTex, 0, nullptr);
        }

}

void RenderBatch::reloadObjectData()
{
	void* dataBuffer = nullptr;
	std::vector<uint32_t> tmpIndexBuffer;

	std::vector<InstanceInfo> tmpInstanceBuffer;
	
	uint32_t dataSize = 0;
	uint32_t indexEntryCount = 0;
	uint32_t instanceCount = 0;

	for(int i = 0; i < objects.size(); i++){
		objects[i].formatData(graphicPipeline.interfaceVariables, graphicPipeline.strideSize);

		for(int j = 0; j < objects[i].primitives.size(); j++){
			dataSize += objects[i].primitives[j].dataSize;
			indexEntryCount += objects[i].primitives[j].indices.size();
			instanceCount++;
		}
	}

	
	dataBuffer = malloc(dataSize);
	tmpIndexBuffer.resize(indexEntryCount);
	tmpInstanceBuffer.resize(instanceCount);

	uint32_t tmpDataOffset = 0;
	uint32_t tmpVertexOffset = 0;
	uint32_t tmpIndexOffset = 0;
	uint32_t tmpDrawIndex = 0;


	for(int i = 0; i < objects.size(); i++){
		for(int j = 0; j < objects[i].primitives.size(); j++){
			memcpy(dataBuffer + tmpDataOffset, objects[i].primitives[j].dataBuffer,  objects[i].primitives[j].dataSize);
			objects[i].primitives[j].dataOffset = tmpDataOffset;
			tmpDataOffset +=  objects[i].primitives[j].dataSize;
			
			objects[i].primitives[j].vertexOffset = tmpVertexOffset;
			tmpVertexOffset += objects[i].primitives[j].vertices.size();


			memcpy(tmpIndexBuffer.data() + tmpIndexOffset, objects[i].primitives[j].indices.data(),  objects[i].primitives[j].indices.size() * sizeof(uint32_t));
			objects[i].primitives[j].indexOffset = tmpIndexOffset;
			tmpIndexOffset += objects[i].primitives[j].indices.size();


			objects[i].primitives[j].drawIndex = tmpDrawIndex;

			tmpInstanceBuffer[tmpDrawIndex].materialIndex = objects[i].primitives[j].materialIndex;
			tmpInstanceBuffer[tmpDrawIndex].modelIndex = objects[i].primitives[j].modelIndex;

			tmpDrawIndex++;


		}
	}

	uploadData(dataBuffer, vertexBuffer, dataSize, 0);
	uploadData(tmpIndexBuffer.data(), indexBuffer, indexEntryCount * sizeof(uint32_t), 0);
	uploadData(tmpInstanceBuffer.data(), instanceBuffer, instanceCount * sizeof(InstanceInfo), 0);

	updateDrawCommands();

	

	free(dataBuffer);
}



void RenderBatch::uploadData(void *p_data, VkBuffer p_dstBuffer, size_t p_size, uint64_t p_dstOffset)
{
    VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	createBuffer(p_size, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		stagingBuffer, stagingBufferAllocation);

	void* data;
	vmaMapMemory(ctx.allocator, stagingBufferAllocation, &data);
	memcpy(data, p_data, (size_t)p_size);
	vmaUnmapMemory(ctx.allocator, stagingBufferAllocation);


	copyBuffer(stagingBuffer, p_dstBuffer, p_size,0 ,p_dstOffset);

	vmaFreeMemory(ctx.allocator, stagingBufferAllocation);
}

VmaAllocationInfo RenderBatch::createBuffer(VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer &buffer, VmaAllocation &allocation)
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

void RenderBatch::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
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



void RenderBatch::DBGEnumerateObjects()
{
	std::cout << "------------- OBJECT LIST ON RENDER BATCH --------------" << name << std::endl;
	for(int i = 0; i < objects.size(); i++){
		std::cout << "Index: " << i << " --> " <<  objects[i].name << std::endl;
		
		std::cout << "Translation: (x=" << objects[i].transformation.translation.x << ", y=" << objects[i].transformation.translation.y << ", z=" << objects[i].transformation.translation.z << ")" << std::endl;
		std::cout << "Rotation: (x=" << objects[i].transformation.rotation.x << ", y=" << objects[i].transformation.rotation.y << ", z=" << objects[i].transformation.rotation.z << ", w=" << objects[i].transformation.rotation.w << ")" << std::endl;
		std::cout << "Scale: (x=" << objects[i].transformation.scale.x << ", y=" << objects[i].transformation.scale.y << ", z=" << objects[i].transformation.scale.z << ")" << std::endl;

		std::cout << "**** Primitives Start ****" << std::endl;
		std::cout << "Primitive count: " << objects[i].primitives.size() << std::endl;
		for(int j = 0; j < objects[i].primitives.size(); j++){
			
			std::cout << "Index: " << j << ", Mode: " << objects[i].primitives[j].mode << ", Vertices: " << objects[i].primitives[j].vertices.size() << 
			", Normals: " << objects[i].primitives[j].normals.size() << 
			", UV: " << objects[i].primitives[j].UV.size() << 
			", Indices: " << objects[i].primitives[j].indices.size() << 
			", Material name: " << objects[i].primitives[j].materialName << std::endl;
		}
		std::cout << "**** Primitives End ****" << std::endl;




	}

}

int RenderBatch::findTextureIndex(std::string & p_name)
{

	for(int i = 0; i < ctx.textureSet.size();i++){
		if(ctx.textureSet[i].enabled && ctx.textureSet[i].name == p_name){
			return i;
		}
	}

	std::cout << "Couldn't find texture for material use: " << p_name << std::endl;

    return -1;
}

bool RenderBatch::LoadModelTextures(tinygltf::Model& model, std::string& path) {




    if (model.images.empty()) {
        std::cout << "Model has no images." << std::endl;
        return true; // No images to load, not an error
    }

    // Step 1: Load all raw images
    for (size_t i = 0; i < model.images.size(); ++i) {
        tinygltf::Image& gltfImage = model.images[i];

		VkImage textureImage;
		VkImageView textureImageView;

		VkSampler textureSampler;

		if(gltfImage.name.empty()){
			std::cout << "WARNING: Texture being loaded has no name. It might conflict with pipeline or texture slots." << std::endl;
		}

        std::string tmpTexName = path + std::to_string(i);
		bool textureExists = false;

		for(TextureSlot slot: ctx.textureSet){
			if(slot.name == tmpTexName){
				std::cout << "Texture: \"" << gltfImage.uri << "\" is already in texture set. Skipping this texture." << std::endl; 
				textureExists = true;
			}
		}

		if(textureExists){
			continue;
		}

        // tinygltf loads image data into gltfImage.image (std::vector<unsigned char>)
        // and sets gltfImage.width, gltfImage.height, gltfImage.component (channels)

        // Check if image data is directly embedded or loaded from an external file
        if (!gltfImage.image.empty() && gltfImage.width > 0 && gltfImage.height > 0) {

			
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(ctx.physicalDevice, &properties);
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;

			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;

			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;

			if (vkCreateSampler(ctx.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture sampler!");
			}

			VkBuffer stagingBufferTexture;
			VkDeviceMemory stagingBufferMemoryTexture;

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = gltfImage.width * gltfImage.height * gltfImage.component;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

			VkBuffer buffer;
			VmaAllocation allocation;
			vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocCreateInfo, &buffer, &allocation, nullptr);

			void* data;
			vmaMapMemory(ctx.allocator, allocation, &data);
			memcpy(data, gltfImage.image.data(), static_cast<size_t>(gltfImage.width * gltfImage.height * gltfImage.component));
			vmaUnmapMemory(ctx.allocator, allocation);


			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = gltfImage.width;
			imageInfo.extent.height = gltfImage.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

			VmaAllocation alloc;
			vmaCreateImage(ctx.allocator, &imageInfo, &allocInfo, &textureImage, &alloc, nullptr);

			transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			copyBufferToImage(buffer, textureImage, static_cast<uint32_t>(gltfImage.width), static_cast<uint32_t>(gltfImage.height));

			transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = textureImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}

			vmaDestroyBuffer(ctx.allocator, buffer, allocation);



            

			for(int i = 0; i < ctx.textureSet.size();i++){
				if(!ctx.textureSet[i].enabled){
					ctx.textureSet[i].enabled = true;
					ctx.textureSet[i].name = tmpTexName;
					ctx.textureSet[i].textureImage = textureImage;
					ctx.textureSet[i].textureImageView = textureImageView;
					ctx.textureSet[i].textureSampler = textureSampler;
					  
					std::cout << "Loaded image '" << tmpTexName << "' ("
                      << gltfImage.width << "x" << gltfImage.height << ", "
                      << gltfImage.component << " channels) on index " << i << std::endl;

					break;
				}
			}

        } else {
            // This case should ideally not happen if tinygltf successfully loaded the model.
            // It might indicate a problem with the glTF file itself (e.g., invalid image URI)
            std::cerr << "Warning: Image data for texture is empty or invalid. Skipping." << std::endl;
        }
    }

	/*
    std::cout << "\nProcessing glTF Textures (linking images with samplers):" << std::endl;
    for (size_t i = 0; i < model.textures.size(); ++i) {
        const tinygltf::Texture& gltfTexture = model.textures[i];
        std::cout << "  Texture " << i << ": ";

        // gltfTexture.source is the index into model.images
        if (gltfTexture.source >= 0 && gltfTexture.source < loadedTextures.size()) {
            std::cout << "References image: " << loadedTextures[gltfTexture.source].name << std::endl;
            // You would typically use gltfTexture.sampler (index into model.samplers)
            // to get filtering and wrap modes for this specific texture.
        } else {
            std::cout << "References invalid image index: " << gltfTexture.source << std::endl;
        }
    }
		*/

    return true;
}

void RenderBatch::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
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

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx.graphicsQueue);

	vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

void RenderBatch::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
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

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx.graphicsQueue);

	vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

void RenderBatch::updateModelMatrices()
{
    graphicPipeline.pushConstant.modelBufferAddress = ctx.bufferAddress;

	for(int i = 0; i < objects.size();i++){

		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, objects[i].transformation.translation);
		modelMat = modelMat * glm::mat4_cast(objects[i].transformation.rotation);
		modelMat = glm::scale(modelMat, objects[i].transformation.scale);


		ctx.modelMatrixList[objects[i].modelIndex] = modelMat;
	}
}

void RenderBatch::uploadPD()
{
	for(int i = 0; i < objects.size();i++){
		for(int j = 0; j < objects[i].primitives.size();j++){
			size_t start = CTX::AUX::allocateVertexMemory(ctx, graphicPipeline.strideSize, objects[i].primitives[j].vertices.size());
			size_t startIndex = CTX::AUX::allocateIndexMemory(ctx, objects[i].primitives[j].indices.size());
			
			objects[i].primitives[j].startOffsetVertex = start;
			objects[i].primitives[j].vertexCount = objects[i].primitives[j].vertices.size();
			objects[i].primitives[j].endOffsetVertex = start + objects[i].primitives[j].vertices.size();

			objects[i].primitives[j].startOffsetIndex = startIndex;
			objects[i].primitives[j].indexCount = objects[i].primitives[j].indices.size();
			objects[i].primitives[j].endOffsetIndex = startIndex + objects[i].primitives[j].indices.size();


			uploadData(objects[i].primitives[j].dataBuffer, ctx.vertexBuffer, objects[i].primitives[j].dataSize, start*graphicPipeline.strideSize);
			uploadData(objects[i].primitives[j].indices.data(), ctx.indexBuffer, objects[i].primitives[j].indices.size() * sizeof(uint32_t), startIndex*sizeof(uint32_t));
			
			ctx.primitiveData.push_back(objects[i].primitives[j]);
		}
	}
}


