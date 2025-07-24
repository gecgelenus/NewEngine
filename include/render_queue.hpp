#pragma once
#include "util.hpp"
#include "vulkan_context.hpp"
#include "render_batch.hpp"
#include "Interface.hpp"
#include "resource_manager.hpp"

class RenderQueue{

    public:

        RenderQueue(vk_ctx&, vk_instance_params&);
        ~RenderQueue();

        void addBatch(RenderBatch&);
        void drawQueue();


        vk_ctx& ctx;
        vk_instance_params instance_params;
        std::vector<RenderBatch> batchList;
        Interface* interface;
        ImDrawData* drawData = nullptr;
        uint32_t currentFrame = 0;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        bool cameraInputEnabled = false;
        GraphicPipeline pipeline;

        void processCameraInput();
        void updateCamera(uint32_t index);
        void createSyncObjects();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index);

};