#include <vulkan_context.hpp>
#include <Interface.hpp>
#include "vma.h"
#include "graphic_pipeline.hpp"
#include <iostream>
#include <vulkan/vulkan.hpp>
#include "object.hpp"
#include "render_batch.hpp"
#include "render_queue.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

vk_ctx* pCtx = nullptr;



void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE) {
        if (action == GLFW_PRESS && pCtx != nullptr) {
            if(pCtx->camera.inputEnabled){
                pCtx->camera.inputEnabled = false;
            }else{
                pCtx->camera.inputEnabled = true;
            }
        }

    }
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}
void char_callback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
}
int main(){
    
    vk_ctx ctx{};
    pCtx = &ctx;
    ctx.console = new ConsoleInstance();
    
    vk_instance_params instance_params{};
    
    instance_params.enableValidationLayers = true;
    instance_params.windowTitle = "Engine";
    instance_params.windowHeight = 1000;
    instance_params.windowWidth = 1600;
    instance_params.windowResizable = false;
    instance_params.physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    CTX::initContext(ctx, instance_params);
    
    pCtx = &ctx;
    {

    INFO("Swapchain images", "Swapchain image size: %u", ctx.swapchainImageViews.size());
    GraphicPipeline* pipeline = new GraphicPipeline(ctx, "../shaders/bin/simple.vert.spv","../shaders/bin/simple.frag.spv",instance_params);
  

    RenderBatch rBatch(ctx, "Test");
    rBatch.graphicPipeline = pipeline;
    std::string pathFile = "/home/talha/Desktop/dice.glb";
    std::string pathFile2 = "/home/talha/Desktop/space.glb";
    std::string pathFile3 = "/home/talha/Desktop/room.glb";


    CTX::AUX::processGltfFile(ctx, pathFile2);

    CTX::AUX::processGltfFile(ctx, pathFile3);

    CTX::AUX::processGltfFile(ctx, pathFile);




    for(int i = 0; i < ctx.objects.size(); i++){
		ctx.objects[i]->formatData(pipeline->interfaceVariables, pipeline->strideSize);
	}

    CTX::reloadObjectData(ctx);
    
    
    // Add debugging output
    std::cout << "Total objects loaded: " << ctx.objects.size() << std::endl;
    for(int i = 0; i < ctx.objects.size(); i++) {
        std::cout << "Object " << i << ": " << ctx.objects[i]->name 
                  << " (ID: " << ctx.objects[i]->objectID << ")" 
                  << " Primitives: " << ctx.objects[i]->primitives.size() << std::endl;
    }
    
    // Only set transformations if objects exist
    

   




    

    
    RenderQueue renderQueue(ctx, instance_params);
    renderQueue.addBatch(&rBatch);


    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCharCallback(ctx.window, char_callback);

    for(int i = 0; i < renderQueue.batchList.size();i++){
        renderQueue.batchList[i]->graphicPipeline->pushConstant.modelBufferAddress = ctx.bufferAddress;
        renderQueue.batchList[i]->graphicPipeline->pushConstant.materialBufferAddress = ctx.materialBufferAddress;

    }



    while(!glfwWindowShouldClose(ctx.window)){
            glfwPollEvents(); 
            CTX::checkExpiredAllocations(ctx);
            CTX::updateModelMatrices(ctx);

            
            renderQueue.drawQueue();
            
    }
    
    
    // Wait for the device to finish all pending operations before destroying
    vkDeviceWaitIdle(ctx.device);

    
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    }
    CTX::destroyContext(ctx, instance_params);
    
    return 0; // Return 0 for success
}