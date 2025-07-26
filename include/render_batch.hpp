#pragma once
#include "util.hpp"
#include <vulkan_context.hpp>
#include <vma.h>
#include "object.hpp"
#include "graphic_pipeline.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "tiny_gltf.h"

class RenderBatch{

    public:
        RenderBatch(vk_ctx& p_ctx, const std::string& p_name);
        void __RenderBatch();
        ~RenderBatch();


        void addObject(Object* p_object);
        void readGLTF(std::string& path);
        void updateDrawCommands();
        void updateTextureSets();
        void reloadObjectData();

        void processGltfScene(tinygltf::Model& model);
        void processGltfFile(std::string& path);

        void processNode(tinygltf::Model& model, std::string& path, const tinygltf::Node& node, Object* parentObject);
        bool LoadModelTextures(tinygltf::Model& model, std::string& path);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void updateModelMatrices();
        void updateMaterialIndices();

        void updateModelMatrixRecursive(Object* obj);

        void uploadPD();

        std::string name;
        vk_ctx& ctx;
        std::vector<Object*> objects;
        GraphicPipeline* graphicPipeline;

        std::vector<float> cpuVertexBuffer;
        std::vector<uint32_t> cpuIndexBuffer;

        
        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferAllocation;
        
        VkBuffer indexBuffer;
        VmaAllocation indexBufferAllocation;
        
        VkBuffer drawBuffer;
        VmaAllocation drawBufferAllocation;

        VkBuffer instanceBuffer;
        VmaAllocation instanceBufferAllocation;

        

        uint64_t currVertexBufferSize = 0;
        uint64_t currIndexBufferSize = 0;

        std::vector<VkDrawIndexedIndirectCommand> drawCommands;
        
        std::vector<VkSampler> samplers;
        std::unordered_map<VmaAllocation, VkImage> images;
        std::vector<VkImageView> imageViews;


        void uploadData(void* data, VkBuffer dstBuffer, size_t size, uint64_t dstOffset);
        VmaAllocationInfo createBuffer(VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& allocation);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset);
        void DBGEnumerateObjects();
        int findTextureIndex(std::string& p_name);
        int findMaterialIndex(std::string & p_name);

        int createMaterial(std::string& path, tinygltf::Model& model, tinygltf::Material& material);


        
};