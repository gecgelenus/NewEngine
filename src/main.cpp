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
#include <chrono>
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

    auto startupStart = std::chrono::high_resolution_clock::now();

    vk_ctx ctx{};
    pCtx = &ctx;
    ctx.console = new ConsoleInstance(ctx);
    
    vk_instance_params instance_params{};
    
    instance_params.enableValidationLayers = true;
    instance_params.windowTitle = "Engine";
    instance_params.windowHeight = 1000;
    instance_params.windowWidth = 1600;
    instance_params.windowResizable = false;
    instance_params.physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    ctx.printCPUTime = false;
    ctx.printGPUTime = false;


    ctx.params = instance_params;
    CTX::initContext(ctx, instance_params);
    
    pCtx = &ctx;
    {

    INFO("Swapchain images", "Swapchain image size: %u", ctx.swapchainImageViews.size());
    GraphicPipeline* pipeline = new GraphicPipeline(ctx, "../shaders/bin/simple.vert.spv","../shaders/bin/simple.frag.spv",instance_params);
    
    ctx.pipelines.push_back(pipeline);


    std::string pathFile3 = "/home/talha/Desktop/orta.glb";
    auto t0 = std::chrono::high_resolution_clock::now();

        CTX::AUX::processGltfFile(ctx, pathFile3);






    for(int i = 0; i < ctx.objects.size(); i++){
		ctx.objects[i]->transformation.translation.y = i*1;
        ctx.objects[i]->formatData(pipeline);
	}
auto t1 = std::chrono::high_resolution_clock::now();

    CTX::reloadObjectData(ctx);
auto t2 = std::chrono::high_resolution_clock::now();

    CTX::sortObjectPrimitives(ctx);
    

double formatMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
double yuklemeMs  = std::chrono::duration<double, std::milli>(t2 - t1).count();

std::cout << "Veri formatlama:       " << formatMs  << " ms"
          << "  (" << ctx.objects.size() << " nesne)\n";
std::cout << "Model yukleme (100x):  " << yuklemeMs << " ms"
          << "  (ortalama " << yuklemeMs / 100.0 << " ms/model)\n";
std::cout << "Toplam:                " << (yuklemeMs + formatMs) << " ms\n";


   




    

    
    RenderQueue renderQueue(ctx, instance_params);
    ctx.rQueue = &renderQueue;


    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCharCallback(ctx.window, char_callback);





    bool firstFrameRendered = false;

    while(!glfwWindowShouldClose(ctx.window)){
            glfwPollEvents();
            CTX::checkExpiredAllocations(ctx);
            CTX::updateModelMatrices(ctx);


            renderQueue.drawQueue();

            if(!firstFrameRendered){
                firstFrameRendered = true;
                auto startupEnd = std::chrono::high_resolution_clock::now();
                double startupMs = std::chrono::duration<double, std::milli>(startupEnd - startupStart).count();
                std::cout << "Startup to first rendered frame: " << startupMs << " ms" << std::endl;
            }

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
