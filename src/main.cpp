#include <vulkan_context.hpp>
#include <Interface.hpp>
#include "vma.h"
#include "graphic_pipeline.hpp"
#include <iostream>
#include <vulkan/vulkan.hpp>
#include "object.hpp"
#include "render_queue.hpp"
#include "console.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

vk_ctx* pCtx = nullptr;

PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;


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
    ctx.console = new ConsoleInstance(ctx);
    
    vk_instance_params instance_params{};
    
    instance_params.enableValidationLayers = true;
    instance_params.windowTitle = "Engine";
    instance_params.windowHeight = 900;
    instance_params.windowWidth = 1600;
    instance_params.windowResizable = false;
    instance_params.physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    instance_params.preferedPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    ctx.params = instance_params;
    CTX::initContext(ctx, instance_params);
    
    pCtx = &ctx;
    {

    INFO("Swapchain images", "Swapchain image size: %u", ctx.swapchainImageViews.size());
    GraphicPipeline* pipeline = new GraphicPipeline(ctx, GRAPHICS_COLOR, "../shaders/bin/simple.vert.spv","../shaders/bin/simple.frag.spv",instance_params);
    GraphicPipeline* pipeline2 = new GraphicPipeline(ctx, GRAPHICS_DEPTH, "../shaders/bin/simpleShadow.vert.spv","../shaders/bin/simple.frag.spv",instance_params);
    

    ctx.dephtPipeline = pipeline2;
    
    ctx.pipelines.push_back(pipeline);



    std::string pathFile = "/home/talha/Desktop/engine_stuffs/dice.glb";
    
    std::string pathFile2 = "/home/talha/Desktop/engine_stuffs/orta.glb";


    std::string pathFileCube = "/home/talha/Desktop/engine_stuffs/cube.glb";
    std::string pathFileSphere = "/home/talha/Desktop/engine_stuffs/sphere.glb";
    std::string pathFileTerrain = "/home/talha/Desktop/engine_stuffs/terrain.glb";
    


    CTX::AUX::processGltfFile(ctx, pathFile2);


    CTX::AUX::processGltfFile(ctx, pathFile);
    CTX::AUX::processGltfFile(ctx, pathFileCube);
    CTX::AUX::processGltfFile(ctx, pathFileSphere);
    CTX::AUX::processGltfFile(ctx, pathFileTerrain);





	ctx.objects[0]->formatData(pipeline2);

    for(int i = 1; i < ctx.objects.size(); i++){
		ctx.objects[i]->formatData(pipeline);
	}


    CTX::reloadObjectData(ctx);
    CTX::sortObjectPrimitives(ctx);
    

    
    

    glm::mat4 lightView = glm::lookAt(glm::vec3(-15.0f, 15.0f, -15.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    lightProj[1][1] *= -1;


    glm::mat4 m = lightProj * lightView;

    LightEntry tmpLight{};
    tmpLight.matrix = m;
    tmpLight.color = glm::vec4(1.0f, 1.0f, 0.0f, 250.0f);
    tmpLight.pos = glm::vec4(15.0f, 15.0f, 15.0f, 0.0f);

    ctx.lights.push_back(tmpLight);

    CTX::AUX::uploadDataDeviceBuffer(ctx, ctx.lights.data(), ctx.lightBufferAddress, ctx.lightBufferAllocation,
    ctx.lightBuffer, ctx.lights.size()*sizeof(LightEntry), 0);

    SetDebugUtilsObjectNameEXT =
    (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
    ctx.instance,
    "vkSetDebugUtilsObjectNameEXT");

    VkDebugUtilsObjectNameInfoEXT lightBufferName = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
    .pNext = NULL,
    .objectType = VK_OBJECT_TYPE_BUFFER,
    .objectHandle = (uint64_t)ctx.lightBuffer,
    .pObjectName = "LightBuffer",
    };
    SetDebugUtilsObjectNameEXT(ctx.device, &lightBufferName);


    

    
    RenderQueue renderQueue(ctx, instance_params);
    ctx.rQueue = &renderQueue;


    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCharCallback(ctx.window, char_callback);




    

    while(!glfwWindowShouldClose(ctx.window)){
            glfwPollEvents(); 
            CTX::checkExpiredAllocations(ctx);
            CTX::updateModelMatrices(ctx);
            CTX::AUX::updateAddressBuffer(ctx);
            
            renderQueue.drawQueue();
            
    }
    
    
    // Wait for the device to finish all pending operations before destroying
    vkDeviceWaitIdle(ctx.device);

    
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    for(int i = 0; i < ctx.pipelines.size();i++){
        delete ctx.pipelines[i];
    }

    ImGui::DestroyContext();

    }
    CTX::destroyContext(ctx, instance_params);
    
    return 0; // Return 0 for success
}
