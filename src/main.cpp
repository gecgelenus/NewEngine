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
 
}


int main(){
    
    vk_ctx ctx{};
    pCtx = &ctx;
    
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
    // TODO: MAKE THIS SHIT A POINTER SO ITS NOT FREED TWICE!!
  

    RenderBatch rBatch(ctx, "Test");
    rBatch.graphicPipeline = pipeline;
    std::string pathFile = "/home/talha/Desktop/dice.glb";
    std::string pathFile2 = "/home/talha/Desktop/space.glb";
    std::string pathFile3 = "/home/talha/Desktop/vulkan.glb";


    rBatch.processGltfFile(pathFile2);
    rBatch.processGltfFile(pathFile);
    rBatch.processGltfFile(pathFile3);


    rBatch.reloadObjectData();
    rBatch.updateDrawCommands();
    rBatch.updateTextureSets();

    rBatch.objects[0]->transformation.translation = glm::vec3(1.0f, 0.0f, 0.0f);
    rBatch.objects[1]->transformation.translation = glm::vec3(-1.0f, 0.0f, 0.0f);
    

   




    


    
    RenderQueue renderQueue(ctx, instance_params);
    renderQueue.addBatch(&rBatch);


    glfwSetKeyCallback(ctx.window, key_callback);
    for(int i = 0; i < renderQueue.batchList.size();i++){
        renderQueue.batchList[i]->graphicPipeline->pushConstant.modelBufferAddress = ctx.bufferAddress;
    }

    while(!glfwWindowShouldClose(ctx.window)){
            glfwPollEvents(); 
            CTX::checkExpiredAllocations(ctx);
            for(int i = 0; i < renderQueue.batchList.size();i++){
                renderQueue.batchList[i]->updateModelMatrices();
            }

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
	


            CTX::AUX::uploadData(ctx, ctx.modelMatrixList.data(), ctx.deviceBuffer, modelDataSize, 0);
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