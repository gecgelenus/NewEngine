#include "render_queue.hpp"
#include <iostream>


RenderQueue::RenderQueue(vk_ctx & p_ctx, vk_instance_params& p_instance_params):
    ctx(p_ctx)
{
    interface = new Interface(ctx);
    interface->init();


    instance_params = p_instance_params;
    createSyncObjects();
}

RenderQueue::~RenderQueue()
{
        // Destroy synchronization objects
    for (size_t i = 0; i < instance_params.framesOnFlight; i++) {
        vkDestroySemaphore(ctx.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(ctx.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(ctx.device, inFlightFences[i], nullptr);
    }
    
}

void RenderQueue::addBatch(RenderBatch & p_batch)
{
    batchList.push_back(p_batch);
}

void RenderQueue::drawQueue()
{
            vkWaitForFences(ctx.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
            uint32_t imageIndex;
            // Use ctx.device and ctx.swapchain as members of vk_ctx
            auto result = vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

            if(result == VK_ERROR_OUT_OF_DATE_KHR){
                // Handle swapchain recreation here, then continue loop
                // For now, let's just return to simplify, but this is incomplete.
                std::cerr << "Swapchain out of date, recreating." << std::endl;
                CTX::recreateSwapchain(ctx, instance_params);

            } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
                throw std::runtime_error("failed to acquire swap chain image!");
            }
            vkResetFences(ctx.device, 1, &inFlightFences[currentFrame]);

            processCameraInput();
            updateCamera(currentFrame);
            
            interface->start_frame();
            ImGui::Begin("My Custom Window");
            ImGui::Text("Hello from ImGui!");
            static float f = 0.0f;
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::End();

            ImGui::Render();
            drawData = ImGui::GetDrawData();
            vkResetCommandBuffer(ctx.commandBuffers[currentFrame], 0);


            recordCommandBuffer(ctx.commandBuffers[currentFrame], imageIndex); 

            VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };
            VkSwapchainKHR swapChains[] = { ctx.swapchain }; 




            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &ctx.commandBuffers[currentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
            
            if (vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr; // Optional

            result = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo); // Use ctx.presentQueue

            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR){
			    ALERT("SWAPCHAIN", "SWAPCHAIN OUT OF DATE");
		    }

            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR){
                std::cerr << "Swapchain out of date, recreating." << std::endl;
                CTX::recreateSwapchain(ctx, instance_params);
                
            } else if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to present swap chain image!");
            }
            
            currentFrame = (currentFrame + 1) % instance_params.framesOnFlight;
}

void RenderQueue::createSyncObjects()
{
    imageAvailableSemaphores.resize(instance_params.framesOnFlight);
    renderFinishedSemaphores.resize(ctx.swapchainImageViews.size());
    inFlightFences.resize(instance_params.framesOnFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < instance_params.framesOnFlight; i++) {
        if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
    
    for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++) {
        if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a swapchain image");
        }
    }
}

void RenderQueue::processCameraInput()
{
    static bool firstInput = false;
    if(ctx.camera.inputEnabled){
        if(!firstInput){
            firstInput = true;
            glfwSetCursorPos(ctx.window, 1600 / 2, 1000 / 2);
            return;
        }

        double xpos, ypos;
        float deltaTime = CTX::getDeltaTime();

        glfwGetCursorPos(ctx.window, &xpos, &ypos);

        glfwSetCursorPos(ctx.window, 1600 / 2, 1000 / 2);

        ctx.camera.horizontalAngle -= ctx.camera.sensivity * deltaTime * float(xpos - 1600 / 2);
        ctx.camera.verticalAngle += ctx.camera.sensivity * deltaTime * float(1000 / 2 - ypos);

        const float maxVerticalAngle = glm::radians(89.0f);
        if (ctx.camera.verticalAngle > maxVerticalAngle)
            ctx.camera.verticalAngle = maxVerticalAngle;
        else if (ctx.camera.verticalAngle < -maxVerticalAngle)
            ctx.camera.verticalAngle = -maxVerticalAngle;

        ctx.camera.direction = glm::vec3(
            cos(ctx.camera.verticalAngle) * sin(ctx.camera.horizontalAngle),
            sin(ctx.camera.verticalAngle),
            cos(ctx.camera.verticalAngle) * cos(ctx.camera.horizontalAngle)
        );

        glm::vec3 right = glm::vec3(
            sin(ctx.camera.horizontalAngle - 3.14f / 2.0f),
            0,
            cos(ctx.camera.horizontalAngle - 3.14f / 2.0f)
        );

        ctx.camera.up = glm::cross(right, ctx.camera.direction);

        if (glfwGetKey(ctx.window, GLFW_KEY_W) == GLFW_PRESS) {
            ctx.camera.position += ctx.camera.direction * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_S) == GLFW_PRESS) {
            ctx.camera.position -= ctx.camera.direction * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_D) == GLFW_PRESS) {
            ctx.camera.position += right * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_A) == GLFW_PRESS) {
            ctx.camera.position -= right * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(ctx.window, GLFW_KEY_SPACE) == GLFW_REPEAT) {
            ctx.camera.position += ctx.camera.up * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(ctx.window, GLFW_KEY_LEFT_CONTROL) == GLFW_REPEAT) {
            ctx.camera.position -= ctx.camera.up * deltaTime * ctx.camera.speed;
        }
        
    }else{
        firstInput = false;
    }
}

void RenderQueue::updateCamera(uint32_t index)
{
    glm::mat4 viewMatrix = glm::lookAt(ctx.camera.position, ctx.camera.position +  ctx.camera.direction, ctx.camera.up);
	glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(ctx.camera.FOV), 1600 / (float)1000, ctx.camera.nearPlane, ctx.camera.farPlane);
	perspectiveMatrix[1][1] *= -1;
	glm::mat4 VPMul = perspectiveMatrix * viewMatrix;
	memcpy(ctx.camera.mappedData[index], &VPMul, sizeof(glm::mat4));
}

void RenderQueue::recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t index)
{

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    VkClearValue depthColor{}; // Not used for color_attachment_info directly, but good to have for depth
    depthColor.depthStencil = { 1.0f, 0 };
    // VkClearValue clearColorArray[] = { clearColor, depthColor }; // Only used with Render Passes, not dynamic rendering clear values

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; 
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    
    // IMAGE LAYOUT TRANSITION FOR COLOR ATTACHMENT
    // Initial transition from UNDEFINED (or whatever it starts as after acquire)
    // to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL for rendering.
    // If your swapchain images are acquired into PRESENT_SRC_KHR, you might need
    // to transition FROM that to COLOR_ATTACHMENT_OPTIMAL.
    VkImageMemoryBarrier preRenderBarrier = {};
    preRenderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    preRenderBarrier.srcAccessMask = 0; // No previous access
    preRenderBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    preRenderBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Or VK_IMAGE_LAYOUT_PRESENT_SRC_KHR if already presented
    preRenderBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    preRenderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrier.image = ctx.swapchainImages[index]; // Use p_instance_params for swapchain images
    preRenderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    preRenderBarrier.subresourceRange.baseMipLevel = 0;
    preRenderBarrier.subresourceRange.levelCount = 1;
    preRenderBarrier.subresourceRange.baseArrayLayer = 0;
    preRenderBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // This barrier happens very early in the pipeline
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before writing to color attachment
        0,                               // Dependency flags
        0, nullptr,                      // Memory barriers
        0, nullptr,                      // Buffer memory barriers
        1, &preRenderBarrier             // Image memory barriers
    );


    const VkRenderingAttachmentInfo color_attachment_info { // Use non-KHR version if Vulkan 1.3+
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, // Non-KHR
        .imageView = ctx.swapchainImageViews[index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Use non-KHR
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor,
    };

    // Add depth attachment info
    const VkRenderingAttachmentInfo depth_attachment_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.depthImageView, // Assuming depthImageView is correctly setup
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, // Or STORE if you need to read depth later
        .clearValue = depthColor, // Use your depth clear value
    };


    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = ctx.swapchainExtent;

    const VkRenderingInfo render_info { // Use non-KHR version if Vulkan 1.3+
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO, // Non-KHR
        .renderArea = scissor,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = &depth_attachment_info, // Add depth attachment
        .pStencilAttachment = nullptr, // Or &depth_attachment_info if using stencil
    };

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(ctx.swapchainExtent.width);
    viewport.height = static_cast<float>(ctx.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;


    // Use the function pointers loaded by vk_ctx
    vkCmdBeginRendering(commandBuffer, &render_info);
    for(int i = 0; i < batchList.size();i++){
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batchList[i].graphicPipeline.pipeline);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdPushConstants(
            commandBuffer,
            batchList[i].graphicPipeline.pipelineLayout,                     // The pipeline layout that defines the range
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // Must match stageFlags used in VkPushConstantRange
            0,                                  // Offset into the push constant block (usually 0)
            sizeof(PushConstant),            // Size of the data being pushed
            &(batchList[i].graphicPipeline.pushConstant)               // Pointer to your C++ data
        );

        // Assuming these buffers are created and valid for the GraphicPipeline
        VkBuffer vertexBuffers[] = { batchList[i].vertexBuffer, batchList[i].instanceBuffer};
        VkDeviceSize offsets[] = { 0, 0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

        VkBuffer indexBuffer = batchList[i].indexBuffer; // Get index buffer from pipeline
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet descriptorSets[] = {batchList[i].graphicPipeline.descriptorSetsVP[index], batchList[i].graphicPipeline.descriptorSetsModel[index], batchList[i].graphicPipeline.descriptorSetsTexture[index]};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batchList[i].graphicPipeline.pipelineLayout, 0, 3, descriptorSets, 0, nullptr);
        // Make sure graphicPipeline.drawBuffer and graphicPipeline.drawCommands are valid
        vkCmdDrawIndexedIndirect(commandBuffer, batchList[i].drawBuffer, 0, batchList[i].drawCommands.size(), sizeof(VkDrawIndexedIndirectCommand));
    }
    
    
    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

    // Use the function pointers loaded by vk_ctx
    vkCmdEndRendering(commandBuffer);

    // --- FINAL IMAGE LAYOUT TRANSITION FOR PRESENTATION ---
    VkImageMemoryBarrier postRenderBarrier = {};
    postRenderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postRenderBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // We just wrote to it
    postRenderBarrier.dstAccessMask = 0; // No specific access after this for presentation
    postRenderBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    postRenderBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    postRenderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postRenderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postRenderBarrier.image = ctx.swapchainImages[index]; // The current swapchain image
    postRenderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    postRenderBarrier.subresourceRange.baseMipLevel = 0;
    postRenderBarrier.subresourceRange.levelCount = 1;
    postRenderBarrier.subresourceRange.baseArrayLayer = 0;
    postRenderBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Src stage: after color attachment writes
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,              // Dst stage: Effectively, before presentation
        0,                                             // Dependency flags
        0, nullptr,                                    // Memory barriers
        0, nullptr,                                    // Buffer memory barriers
        1, &postRenderBarrier                         // Image memory barriers
    );



    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}