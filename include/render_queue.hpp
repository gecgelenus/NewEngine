#pragma once
#include "util.hpp"
#include "vulkan_context.hpp"
#include "Interface.hpp"
#include <sstream>
#include "console.hpp"

class RenderQueue{

    public:

        RenderQueue(vk_ctx&, vk_instance_params&);
        ~RenderQueue();

        void drawQueue();


        vk_ctx& ctx;
        vk_instance_params instance_params;
        Interface* interface;
        ImDrawData* drawData = nullptr;
        uint32_t currentFrame = 0;

        int selectedItem = -1;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        bool cameraInputEnabled = false;

        std::vector<std::string> consoleBuffer;
    
        void processCameraInput();
        void updateCamera(uint32_t index);
        void createSyncObjects();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index);

        void guiListObjects();

        void renderUI();
        void renderLeftPanel();
        void renderRightPanel();
        void renderConsole();




        void addChildObjectsToList(Object* obj);

};