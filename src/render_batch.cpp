#include "render_batch.hpp"
#include <iostream>



RenderBatch::RenderBatch(vk_ctx& p_ctx, const std::string & p_name)
	: ctx(p_ctx)
{
    name = p_name;
    __RenderBatch();
}

void RenderBatch::__RenderBatch()
{

    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    vertexBuffer, vertexBufferAllocation);

    CTX::AUX::createBuffer(ctx,(VkDeviceSize)SIZE_KB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		indexBuffer, indexBufferAllocation);

    CTX::AUX::createBuffer(ctx,(VkDeviceSize)SIZE_KB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		drawBuffer, drawBufferAllocation);
	
	CTX::AUX::createBuffer(ctx,(VkDeviceSize)SIZE_KB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		instanceBuffer, instanceBufferAllocation);

	
}

RenderBatch::~RenderBatch()
{
    vmaDestroyBuffer(ctx.allocator, vertexBuffer ,vertexBufferAllocation);
	ctx.bufferAllocations.erase(vertexBufferAllocation);
    
	vmaDestroyBuffer(ctx.allocator, indexBuffer ,indexBufferAllocation);
	ctx.bufferAllocations.erase(indexBufferAllocation);
    
	vmaDestroyBuffer(ctx.allocator, instanceBuffer ,instanceBufferAllocation);
	ctx.bufferAllocations.erase(instanceBufferAllocation);
    
	vmaDestroyBuffer(ctx.allocator, drawBuffer ,drawBufferAllocation);
	ctx.bufferAllocations.erase(drawBufferAllocation);

	for(VkSampler s: samplers){
		vkDestroySampler(ctx.device, s, nullptr);
	}

	for(auto i: images){
		vmaDestroyImage(ctx.allocator, i.second, i.first);
	}
	
	for(VkImageView iw: imageViews){
		vkDestroyImageView(ctx.device, iw, nullptr);
	}
	
	
	delete graphicPipeline;

}

void RenderBatch::addObject(Object *p_object)
{
	/*

	p_object.vertexIndex = cpuVertexBuffer.size();
	cpuVertexBuffer.insert(cpuVertexBuffer.end(), p_object.vertexData.begin(), p_object.vertexData.end());
	p_object.indexIndex = cpuIndexBuffer.size();
	cpuIndexBuffer.insert(cpuIndexBuffer.end(), p_object.indexData.begin(), p_object.indexData.end());
	objects.push_back(p_object);
	uploadData(cpuVertexBuffer.data(), vertexBuffer, cpuVertexBuffer.size() * sizeof(float), 0);
	uploadData(cpuIndexBuffer.data(), indexBuffer, cpuIndexBuffer.size() * sizeof(uint32_t), 0);
	updateDrawCommands();
	*/
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
	std::stringstream ss;
	ss << "Processing scene: " << scene.name;
	ctx.console->output(ss.str(), IMGUI_COLOR_BLUE);

	LoadModelTextures(model, path);
	updateTextureSets();


    // Iterate through the root nodes of the scene
    for (int nodeIdx : scene.nodes) {
        const tinygltf::Node& node = model.nodes[nodeIdx];
        

        processNode(model, path, node, nullptr); // Start with identity matrix for root

    }

	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(ctx.allocator, ctx.materialBufferAllocation, &allocationInfo);
	VkDeviceSize materialDataSize = ctx.materialList.size()*sizeof(Material);

	if(allocationInfo.size < materialDataSize){
		CTX::AUX::enlargeBuffer(ctx, materialDataSize, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    		ctx.materialBuffer, ctx.materialBufferAllocation);
		
		
		VkBufferDeviceAddressInfo addressInfoMaterial{};
    	addressInfoMaterial.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    	addressInfoMaterial.buffer = ctx.materialBuffer;

    	ctx.materialBufferAddress = vkGetBufferDeviceAddress(ctx.device, &addressInfoMaterial);
	}
	
	

	CTX::AUX::uploadData(ctx, ctx.materialList.data(), ctx.materialBuffer, ctx.materialList.size()*sizeof(Material), 0);
	graphicPipeline->pushConstant.materialBufferAddress = ctx.materialBufferAddress;
}

// Recursive helper function to traverse the node hierarchy
void RenderBatch::processNode(tinygltf::Model& model, std::string& path, const tinygltf::Node& node, Object* parentObject) {
    
	Object* obj = new Object(ctx);

	obj->parentObject = parentObject;
	obj->modelIndex = ctx.modelMatrixCheck.size();

	ctx.modelMatrixCheck.push_back(true);
	ctx.modelMatrixList.push_back(glm::mat4(1.0f));

	obj->name = node.name;



	if(parentObject != nullptr){
		std::cout << "Loading object: " << obj->name  << "(" << obj->objectID << ") child of" << parentObject->name << std::endl;

		parentObject->childObjects.push_back(obj);
		obj->parentTransformation = parentObject->transformation;

		obj->transformation.translation = glm::vec3(0.0f); 
		obj->transformation.rotation = glm::quat();    
		obj->transformation.scale = glm::vec3(1.0f);      
	}else{
		std::cout << "Loading object: " << obj->name << "(" << obj->objectID << ")" <<std::endl;

		obj->parentTransformation = {};

		obj->transformation.translation = glm::vec3(1.0f); // Default to no translation
		obj->transformation.rotation = glm::quat();    // Default to no rotation (identity quaternion)
		obj->transformation.scale = glm::vec3(1.0f);      // Default to no scaling (identity scale)
	}

    // Check if translation is provided, otherwise use default
    if (node.translation.size() == 3) {
        obj->transformation.translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
	}
    // Check if rotation is provided, otherwise use default
    if (node.rotation.size() == 4) {
        // Correctly construct glm::quat from glTF (x, y, z, w) order
        obj->transformation.rotation = glm::quat(
            node.rotation[3], // w
            node.rotation[0], // x
            node.rotation[1], // y
            node.rotation[2]  // z
        );
    }
    // Check if scale is provided, otherwise use default
    if (node.scale.size() == 3) {
        obj->transformation.scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
    }





    // If this node has a mesh, process it
    if (node.mesh > -1) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];

        // Iterate over primitives within the mesh
        for (const auto& primitive : mesh.primitives) {

			ObjectPrimitive tmpPrimitive;
			tmpPrimitive.mode = primitive.mode;
            
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

			    if (primitive.attributes.count("COLOR_0")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("COLOR_0")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t stride = accessor.ByteStride(bufferView);

                // Vertex colors can be VEC3 (RGB) or VEC4 (RGBA)
                // They can also have different component types (FLOAT, UNSIGNED_BYTE, UNSIGNED_SHORT)
                // When componentType is not FLOAT, they should be normalized in the shader.
                // Here, we'll read them into floats, normalizing if necessary.

                for (size_t i = 0; i < accessor.count; ++i) {
                    glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f); // Default to opaque black

                    if (accessor.type == TINYGLTF_TYPE_VEC3) {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                            const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                            color = glm::vec4(values[0], values[1], values[2], 1.0f);
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                            const uint8_t* values = reinterpret_cast<const uint8_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 255.0f,
                                static_cast<float>(values[1]) / 255.0f,
                                static_cast<float>(values[2]) / 255.0f,
                                1.0f
                            );
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* values = reinterpret_cast<const uint16_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 65535.0f,
                                static_cast<float>(values[1]) / 65535.0f,
                                static_cast<float>(values[2]) / 65535.0f,
                                1.0f
                            );
                        } else {
                            std::cerr << "Warning: Unsupported COLOR_0 component type for VEC3: " << accessor.componentType << std::endl;
                        }
                    } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                            const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                            color = glm::vec4(values[0], values[1], values[2], values[3]);
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                            const uint8_t* values = reinterpret_cast<const uint8_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 255.0f,
                                static_cast<float>(values[1]) / 255.0f,
                                static_cast<float>(values[2]) / 255.0f,
                                static_cast<float>(values[3]) / 255.0f
                            );
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* values = reinterpret_cast<const uint16_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 65535.0f,
                                static_cast<float>(values[1]) / 65535.0f,
                                static_cast<float>(values[2]) / 65535.0f,
                                static_cast<float>(values[3]) / 65535.0f
                            );
                        } else {
                            std::cerr << "Warning: Unsupported COLOR_0 component type for VEC4: " << accessor.componentType << std::endl;
                        }
                    } else {
                        std::cerr << "Warning: COLOR_0 attribute is not VEC3 or VEC4 type. Skipping." << std::endl;
                    }
                    tmpPrimitive.colors.emplace_back(color); // Assuming you have a std::vector<glm::vec4> colors in ObjectPrimitive
                }
            }
            
            // You can also access primitive.material here:
            if (primitive.material > -1) {
                tinygltf::Material& material = model.materials[primitive.material];
				tmpPrimitive.materialName = material.name;
				std::string tmpPath = path;

				int materialIndex = findMaterialIndex(tmpPrimitive.materialName);
				
				if(materialIndex == -1){
					materialIndex = createMaterial(tmpPath, model, material);
				}

				tmpPrimitive.materialIndex = materialIndex;

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

			tmpPrimitive.modelIndex = obj->modelIndex;
			obj->primitives.push_back(tmpPrimitive);

		}
    }
	
	
	objects.push_back(obj);

    // Recursively process children nodes
	if(node.children.size() > 0){
		
		
		for (int childIdx : node.children) {
    		
			const tinygltf::Node& childNode = model.nodes[childIdx];
        	processNode(model, path, childNode, obj); // Pass the accumulated transform
    	}
	}
    
}

void RenderBatch::updateDrawCommands()
{
	drawCommands.clear();

	uint32_t currentInstance = 0;

	for(int i = 0; i < objects.size();i++){
		for(int j = 0; j < objects[i]->primitives.size();j++){
			VkDrawIndexedIndirectCommand tmpCmd{};
			tmpCmd.firstIndex = objects[i]->primitives[j].indexOffset;
			tmpCmd.firstInstance = currentInstance;
			tmpCmd.indexCount = objects[i]->primitives[j].indices.size();
			tmpCmd.instanceCount = 1;
			tmpCmd.vertexOffset = objects[i]->primitives[j].vertexOffset;
			/*
			std::cout << "Draw command --> Index count: " << tmpCmd.indexCount << 
			  " vertex offset: " << tmpCmd.vertexOffset << std::endl;
			*/
			drawCommands.push_back(tmpCmd);
			objects[i]->primitives[j].modelIndex = currentInstance;
			currentInstance++;
		}
	}
	VmaAllocationInfo allocationInfo;

	VkDeviceSize drawDataSize = drawCommands.size() * sizeof(VkDrawIndexedIndirectCommand);
	vmaGetAllocationInfo(ctx.allocator, drawBufferAllocation, &allocationInfo);
	
	if(allocationInfo.size < drawDataSize){
		CTX::AUX::enlargeBuffer(ctx, (VkDeviceSize)drawDataSize, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    	VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
	    drawBuffer, drawBufferAllocation);
	}
	CTX::AUX::uploadData(ctx, drawCommands.data(), drawBuffer, drawDataSize, 0);
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
            descriptorWriteTex.dstSet = ctx.textureDescriptorSet; // Use outer loop variable
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
		objects[i]->formatData(graphicPipeline->interfaceVariables, graphicPipeline->strideSize);

		for(int j = 0; j < objects[i]->primitives.size(); j++){
			dataSize += objects[i]->primitives[j].dataSize;
			objects[i]->primitives[j].modelIndex = objects[i]->modelIndex;

			indexEntryCount += objects[i]->primitives[j].indices.size();
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
		for(int j = 0; j < objects[i]->primitives.size(); j++){
			memcpy(dataBuffer + tmpDataOffset, objects[i]->primitives[j].dataBuffer,  objects[i]->primitives[j].dataSize);
			objects[i]->primitives[j].dataOffset = tmpDataOffset;
			tmpDataOffset +=  objects[i]->primitives[j].dataSize;
			
			objects[i]->primitives[j].vertexOffset = tmpVertexOffset;
			tmpVertexOffset += objects[i]->primitives[j].vertices.size();


			memcpy(tmpIndexBuffer.data() + tmpIndexOffset, objects[i]->primitives[j].indices.data(),  objects[i]->primitives[j].indices.size() * sizeof(uint32_t));
			objects[i]->primitives[j].indexOffset = tmpIndexOffset;
			tmpIndexOffset += objects[i]->primitives[j].indices.size();


			objects[i]->primitives[j].drawIndex = tmpDrawIndex;

			tmpInstanceBuffer[tmpDrawIndex].materialIndex = objects[i]->primitives[j].materialIndex;
			tmpInstanceBuffer[tmpDrawIndex].modelIndex = objects[i]->primitives[j].modelIndex;

			tmpDrawIndex++;


		}
	}
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(ctx.allocator, vertexBufferAllocation, &allocationInfo);
	
	if(allocationInfo.size < dataSize){
		CTX::AUX::enlargeBuffer(ctx, (VkDeviceSize)dataSize, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    	VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    vertexBuffer, vertexBufferAllocation);
	}
	CTX::AUX::uploadData(ctx, dataBuffer, vertexBuffer, dataSize, 0);
	

	VkDeviceSize indexDataSize = indexEntryCount * sizeof(uint32_t);
	vmaGetAllocationInfo(ctx.allocator, indexBufferAllocation, &allocationInfo);
	if(allocationInfo.size < indexDataSize){
		CTX::AUX::enlargeBuffer(ctx, (VkDeviceSize)indexDataSize, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    	VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	    indexBuffer, indexBufferAllocation);
	}
	CTX::AUX::uploadData(ctx, tmpIndexBuffer.data(), indexBuffer, indexDataSize, 0);
	
	
	VkDeviceSize instanceDataSize = instanceCount * sizeof(InstanceInfo);
	vmaGetAllocationInfo(ctx.allocator, instanceBufferAllocation, &allocationInfo);
	if(allocationInfo.size < instanceDataSize){
		CTX::AUX::enlargeBuffer(ctx, (VkDeviceSize)instanceDataSize, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    	VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    instanceBuffer, instanceBufferAllocation);
	}
	CTX::AUX::uploadData(ctx, tmpInstanceBuffer.data(), instanceBuffer, instanceDataSize, 0);

	updateDrawCommands();



	free(dataBuffer);
}







void RenderBatch::DBGEnumerateObjects()
{
	std::cout << "------------- OBJECT LIST ON RENDER BATCH --------------" << name << std::endl;
	for(int i = 0; i < objects.size(); i++){
		std::cout << "Index: " << i << " --> " <<  objects[i]->name << std::endl;
		
		std::cout << "Translation: (x=" << objects[i]->transformation.translation.x << ", y=" << objects[i]->transformation.translation.y << ", z=" << objects[i]->transformation.translation.z << ")" << std::endl;
		std::cout << "Rotation: (x=" << objects[i]->transformation.rotation.x << ", y=" << objects[i]->transformation.rotation.y << ", z=" << objects[i]->transformation.rotation.z << ", w=" << objects[i]->transformation.rotation.w << ")" << std::endl;
		std::cout << "Scale: (x=" << objects[i]->transformation.scale.x << ", y=" << objects[i]->transformation.scale.y << ", z=" << objects[i]->transformation.scale.z << ")" << std::endl;

		std::cout << "**** Primitives Start ****" << std::endl;
		std::cout << "Primitive count: " << objects[i]->primitives.size() << std::endl;
		for(int j = 0; j < objects[i]->primitives.size(); j++){
			
			std::cout << "Index: " << j << ", Mode: " << objects[i]->primitives[j].mode << ", Vertices: " << objects[i]->primitives[j].vertices.size() << 
			", Normals: " << objects[i]->primitives[j].normals.size() << 
			", UV: " << objects[i]->primitives[j].UV.size() << 
			", Indices: " << objects[i]->primitives[j].indices.size() << 
			", Material name: " << objects[i]->primitives[j].materialName << std::endl;
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

int RenderBatch::findMaterialIndex(std::string & p_name)
{

	for(int i = 0; i < ctx.materialNames.size();i++){
		if(ctx.materialNames[i] == p_name){
			return i;
		}
	}


    return -1;
}

int RenderBatch::createMaterial(std::string& path, tinygltf::Model& model, tinygltf::Material &material)
{

	//std::cout << "Creating material:" << material.name << std::endl;

	Material tmpMat;
	tmpMat.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	if (material.pbrMetallicRoughness.baseColorFactor.size() == 4) {
		tmpMat.baseColorFactor = glm::vec4(
			material.pbrMetallicRoughness.baseColorFactor[0],
			material.pbrMetallicRoughness.baseColorFactor[1],
			material.pbrMetallicRoughness.baseColorFactor[2],
			material.pbrMetallicRoughness.baseColorFactor[3]
		);

		
		tmpMat.baseColorFactorEnabled = 1;
	
	}else{
		tmpMat.baseColorFactorEnabled = 0;
	}

	if(material.pbrMetallicRoughness.baseColorTexture.index > -1){
		int texIndex = findTextureIndex(path.append(std::to_string(model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source)));
		//std::cout << "Material: " << material.name  << " uses texture " << path << std::endl;

		if(texIndex < 0){
			tmpMat.textureIndex = 0;
			tmpMat.textureEnabled = 0;
			
		}else{
			tmpMat.textureIndex = texIndex;
			tmpMat.textureEnabled = 1;
		}
	}else{
		tmpMat.textureEnabled = 0;
		tmpMat.textureIndex = 0;
	}

	ctx.materialNames.push_back(material.name);
	ctx.materialList.push_back(tmpMat);

	if(ctx.materialList.size() != ctx.materialNames.size()){
		throw std::runtime_error("Internal consistency on material list!");
	}

	

    return ctx.materialList.size()-1;
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

			samplers.push_back(textureSampler);


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
			images.insert({alloc, textureImage});

			CTX::AUX::transitionImageLayout(ctx, textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			CTX::AUX::copyBufferToImage(ctx, buffer, textureImage, static_cast<uint32_t>(gltfImage.width), static_cast<uint32_t>(gltfImage.height));

			CTX::AUX::transitionImageLayout(ctx, textureImage, VK_FORMAT_R8G8B8A8_SRGB,
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
			imageViews.push_back(textureImageView);

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





void RenderBatch::updateModelMatrices()
{
    graphicPipeline->pushConstant.modelBufferAddress = ctx.bufferAddress;
	std::vector<VkBufferCopy> regions;
	for(int i = 0; i < objects.size();i++){

		if(objects[i]->parentObject == nullptr){
			updateModelMatrixRecursive(objects[i], regions);
		}

	}

	
	if(regions.size() <= 0){
		return;
	}


	for(int i = 0; i < objects.size();i++){
		
		ctx.modelMatrixList[objects[i]->modelIndex] = objects[i]->modelMatrix;
	}


	
	// Check if buffer is enough to contain data
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(ctx.allocator, ctx.deviceBufferAllocation, &allocationInfo);
	VkDeviceSize modelDataSize = ctx.modelMatrixList.size()*sizeof(glm::mat4);

	if(allocationInfo.size < modelDataSize){

	

		CTX::AUX::enlargeBuffer(ctx, modelDataSize, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			ctx.deviceBuffer, ctx.deviceBufferAllocation);
		
		
		VkBufferDeviceAddressInfo addressInfoMaterial{};
		addressInfoMaterial.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfoMaterial.buffer = ctx.deviceBuffer;

		ctx.bufferAddress = vkGetBufferDeviceAddress(ctx.device, &addressInfoMaterial);
	}

	// Copy data
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	CTX::AUX::createBuffer(ctx, modelDataSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
		VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		stagingBuffer, stagingBufferAllocation);

	void* data;
	vmaMapMemory(ctx.allocator, stagingBufferAllocation, &data);
	memcpy(data, ctx.modelMatrixList.data(), (size_t)modelDataSize);
	vmaUnmapMemory(ctx.allocator, stagingBufferAllocation);


	CTX::AUX::copyBuffer(ctx, stagingBuffer, ctx.deviceBuffer, regions);

	ctx.bufferAllocations.erase(stagingBufferAllocation);
	vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingBufferAllocation);
}




void RenderBatch::updateModelMatrixRecursive(Object *obj, std::vector<VkBufferCopy>& regions)
{
	obj->localMatrix = glm::mat4(1.0f);

	if(obj->transformation != obj->oldTransformation){
		obj->oldTransformation = obj->transformation;
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), obj->transformation.translation);
		glm::mat4 rotationMatrix = glm::mat4_cast(obj->transformation.rotation);
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), obj->transformation.scale);


		obj->localMatrix = translationMatrix * rotationMatrix * scaleMatrix;
		if(obj->parentObject != nullptr){
			obj->modelMatrix = obj->parentObject->modelMatrix * obj->localMatrix;
		}else{
			obj->modelMatrix = obj->localMatrix;
		}

		VkBufferCopy copyCmd{};
		copyCmd.dstOffset = sizeof(glm::mat4)*obj->modelIndex;
		copyCmd.srcOffset = sizeof(glm::mat4)*obj->modelIndex;
		copyCmd.size = sizeof(glm::mat4);
		regions.push_back(copyCmd);
	}

	if(obj->childObjects.size() > 0){
		for(int i = 0; i < obj->childObjects.size(); i++){
			updateModelMatrixRecursive(obj->childObjects[i], regions);
		}
	}
}

