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


        void processGltfFile(std::string& path);
        void processNode(tinygltf::Model& model, std::string& path, const tinygltf::Node& node, Object* parentObject);
        bool LoadModelTextures(tinygltf::Model& model, std::string& path);

        void addObject(Object* p_object);

        void reloadObjectData();

        void updateModelMatrices();
        void updateModelMatrixRecursive(Object* obj, std::vector<VkBufferCopy>& regions);

        void updateDrawCommands();
        void updateTextureSets();






        std::string name;
        vk_ctx& ctx;
        std::vector<Object*> objects;
        GraphicPipeline* graphicPipeline;


        
        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferAllocation;
        
        VkBuffer indexBuffer;
        VmaAllocation indexBufferAllocation;
        
        VkBuffer drawBuffer;
        VmaAllocation drawBufferAllocation;
        std::vector<VkDrawIndexedIndirectCommand> drawCommands;

        VkBuffer instanceBuffer;
        VmaAllocation instanceBufferAllocation;


        
        std::vector<VkSampler> samplers; // For resource lifetime tracking
        std::unordered_map<VmaAllocation, VkImage> images; // For resource lifetime tracking
        std::vector<VkImageView> imageViews; // For resource lifetime tracking


        void DBGEnumerateObjects();
        int findTextureIndex(std::string& p_name);
        int findMaterialIndex(std::string & p_name);

        int createMaterial(std::string& path, tinygltf::Model& model, tinygltf::Material& material);


        
};

