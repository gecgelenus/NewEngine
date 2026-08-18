#include "render_queue.hpp"
#include <iostream>
#include <sstream>
#include "object.hpp"
#include "graphic_pipeline.hpp"
#include <iomanip>

RenderQueue::RenderQueue(vk_ctx &p_ctx, vk_instance_params &p_instance_params) : ctx(p_ctx)
{
    interface = new Interface(ctx);
    interface->init();

    instance_params = p_instance_params;
    createSyncObjects();
}

RenderQueue::~RenderQueue()
{
    // Destroy synchronization objects
    for (size_t i = 0; i < instance_params.framesOnFlight; i++)
    {
        vkDestroySemaphore(ctx.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(ctx.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(ctx.device, inFlightFences[i], nullptr);
    }

    delete interface;
}


void RenderQueue::drawQueue()
{

    vkWaitForFences(ctx.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    // Use ctx.device and ctx.swapchain as members of vk_ctx
    auto result = vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Handle swapchain recreation here, then continue loop
        // For now, let's just return to simplify, but this is incomplete.
        std::cerr << "Swapchain out of date, recreating." << std::endl;
        CTX::recreateSwapchain(ctx, instance_params);
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    VkResult fenceStatus = vkGetFenceStatus(ctx.device, inFlightFences[currentFrame]);
    if (fenceStatus == VK_NOT_READY)
    {
        std::cerr << "Fence still in use at frame " << currentFrame << std::endl;
        throw std::runtime_error("mal");
    }
    vkResetFences(ctx.device, 1, &inFlightFences[currentFrame]);

    processCameraInput();
    updateCamera(imageIndex);

    renderUI();
    drawData = ImGui::GetDrawData();
    vkResetCommandBuffer(ctx.commandBuffers[currentFrame], 0);

    recordCommandBuffer(ctx.commandBuffers[currentFrame], imageIndex);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
    VkSwapchainKHR swapChains[] = {ctx.swapchain};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &ctx.commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
    {
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
    

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        ALERT("SWAPCHAIN", "SWAPCHAIN OUT OF DATE");
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "Swapchain out of date, recreating." << std::endl;
        CTX::recreateSwapchain(ctx, instance_params);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    ctx.latest_frame = currentFrame;
    currentFrame = (currentFrame + 1) % instance_params.framesOnFlight;

    if(!ImGui::GetIO().WantCaptureMouse && ctx.pendingClick){
        CTX::AUX::processPendingClick(ctx);
        
    }
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

    for (size_t i = 0; i < instance_params.framesOnFlight; i++)
    {
        if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++)
    {
        if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a swapchain image");
        }
    }
}

void RenderQueue::processCameraInput()
{
    static bool firstInput = false;
    static double lastX = 0.0, lastY = 0.0;

    float deltaTime = CTX::getDeltaTime();

    if (ctx.camera.inputEnabled)
    {
        double xpos, ypos;
        glfwGetCursorPos(ctx.window, &xpos, &ypos);

        if (!firstInput)
        {
            firstInput = true;
            glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            lastX = xpos;
            lastY = ypos;
            return;
        }

        float dx = float(xpos - lastX);
        float dy = float(lastY - ypos);
        lastX = xpos;
        lastY = ypos;

        ctx.camera.horizontalAngle -= ctx.camera.sensivity * deltaTime * dx;
        ctx.camera.verticalAngle += ctx.camera.sensivity * deltaTime * dy;

        const float maxVerticalAngle = glm::radians(89.0f);
        if (ctx.camera.verticalAngle > maxVerticalAngle)
            ctx.camera.verticalAngle = maxVerticalAngle;
        else if (ctx.camera.verticalAngle < -maxVerticalAngle)
            ctx.camera.verticalAngle = -maxVerticalAngle;

        ctx.camera.direction = glm::vec3(
            cos(ctx.camera.verticalAngle) * sin(ctx.camera.horizontalAngle),
            sin(ctx.camera.verticalAngle),
            cos(ctx.camera.verticalAngle) * cos(ctx.camera.horizontalAngle));

        glm::vec3 right = glm::vec3(
            sin(ctx.camera.horizontalAngle - 3.14f / 2.0f),
            0,
            cos(ctx.camera.horizontalAngle - 3.14f / 2.0f));

        ctx.camera.up = glm::cross(right, ctx.camera.direction);

        if (glfwGetKey(ctx.window, GLFW_KEY_W) == GLFW_PRESS)
        {
            ctx.camera.position += ctx.camera.direction * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_S) == GLFW_PRESS)
        {
            ctx.camera.position -= ctx.camera.direction * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_D) == GLFW_PRESS)
        {
            ctx.camera.position += right * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_A) == GLFW_PRESS)
        {
            ctx.camera.position -= right * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(ctx.window, GLFW_KEY_SPACE) == GLFW_REPEAT)
        {
            ctx.camera.position += ctx.camera.up * deltaTime * ctx.camera.speed;
        }
        if (glfwGetKey(ctx.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(ctx.window, GLFW_KEY_LEFT_CONTROL) == GLFW_REPEAT)
        {
            ctx.camera.position -= ctx.camera.up * deltaTime * ctx.camera.speed;
        }
    }
    else
    {
        if (firstInput)
            glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstInput = false;
    }
}

void RenderQueue::updateCamera(uint32_t index)
{
    glm::mat4 viewMatrix = glm::lookAt(ctx.camera.position, ctx.camera.position + ctx.camera.direction, ctx.camera.up);
    glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(ctx.camera.FOV), 1600 / (float)1000, ctx.camera.nearPlane, ctx.camera.farPlane);
    perspectiveMatrix[1][1] *= -1;
    glm::mat4 VPMul = perspectiveMatrix * viewMatrix;
    memcpy(ctx.camera.mappedData[index], &VPMul, sizeof(glm::mat4));
}

void RenderQueue::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index)
{

    PushConstant cnst{};
    cnst.modelBufferAddress = ctx.bufferAddress;
    cnst.materialBufferAddress = ctx.materialBufferAddress;
    cnst.selectedModel = ctx.selectedModel;


    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkClearValue depthColor{}; 
    depthColor.depthStencil = {1.0f, 0};

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }



    // SHADOW PASS START
    
    const VkRenderingAttachmentInfo shadowMapAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.shadowMapImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = depthColor
    };

    VkRect2D scissorShadowPass{};
    scissorShadowPass.offset = {0, 0};
    scissorShadowPass.extent = {2048, 2048};

    const VkRenderingInfo renderInfoShadowPass{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = scissorShadowPass,
        .layerCount = 1,
        .colorAttachmentCount = 0,
        .pDepthAttachment = &shadowMapAttachmentInfo,
        .pStencilAttachment = nullptr,               
    };

    VkViewport viewportShadowMap{};
    viewportShadowMap.x = 0.0f;
    viewportShadowMap.y = 0.0f;
    viewportShadowMap.width = static_cast<float>(2048);
    viewportShadowMap.height = static_cast<float>(2048);
    viewportShadowMap.minDepth = 0.0f;
    viewportShadowMap.maxDepth = 1.0f;


    VkImageMemoryBarrier2 shadowMapBarrierAttach{};
    shadowMapBarrierAttach.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    shadowMapBarrierAttach.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    shadowMapBarrierAttach.srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    shadowMapBarrierAttach.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    shadowMapBarrierAttach.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadowMapBarrierAttach.oldLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    shadowMapBarrierAttach.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    shadowMapBarrierAttach.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowMapBarrierAttach.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowMapBarrierAttach.image = ctx.shadowMapImage;
    shadowMapBarrierAttach.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VkDependencyInfo dependencyShadowMapAttach{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependencyShadowMapAttach.imageMemoryBarrierCount = 1;
    dependencyShadowMapAttach.pImageMemoryBarriers = &shadowMapBarrierAttach;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyShadowMapAttach);


    vkCmdBeginRendering(commandBuffer, &renderInfoShadowPass);
    for (int i = 0; i < ctx.pipelineBatches.size(); i++)
    {

        VkPipeline currentPipeline = ctx.dephtPipeline->pipeline;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewportShadowMap);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissorShadowPass);

        vkCmdPushConstants(
            commandBuffer,
            ctx.globalPipelineLayout,              // The pipeline layout that defines the range
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // Must match stageFlags used in VkPushConstantRange
            0,                                                         // Offset into the push constant block (usually 0)
            sizeof(PushConstant),                                      // Size of the data being pushed
            &cnst              // Pointer to your C++ data
        );

        // Assuming these buffers are created and valid for the GraphicPipeline
        VkBuffer vertexBuffers[] = {ctx.globalVertexBuffer, ctx.instanceBuffer};
        VkDeviceSize offsets[] = {0, 0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

        VkBuffer indexBuffer = ctx.globalIndexBuffer; // Get index buffer from pipeline
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet descriptorSets[] = {ctx.cameraDescriptorSets[index], ctx.addressDescriptorSet, ctx.textureDescriptorSet, ctx.shadowMapDescriptorSet};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.globalPipelineLayout, 0, 4, descriptorSets, 0, nullptr);
        // Make sure graphicPipeline->drawBuffer and graphicPipeline->drawCommands are valid
        vkCmdDrawIndexedIndirect(commandBuffer, ctx.drawBuffer, ctx.pipelineBatches[i].start*sizeof(VkDrawIndexedIndirectCommand), ctx.pipelineBatches[i].end - ctx.pipelineBatches[i].start, sizeof(VkDrawIndexedIndirectCommand));
    }


    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier2 shadowMapBarrier{};
    shadowMapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    shadowMapBarrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    shadowMapBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadowMapBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    shadowMapBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    shadowMapBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    shadowMapBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    shadowMapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowMapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowMapBarrier.image = ctx.shadowMapImage;
    shadowMapBarrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VkDependencyInfo dependencyShadowMap{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependencyShadowMap.imageMemoryBarrierCount = 1;
    dependencyShadowMap.pImageMemoryBarriers = &shadowMapBarrier;
    vkCmdPipelineBarrier2(commandBuffer, &dependencyShadowMap);

    // SHADOW PASS END







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

    VkImageMemoryBarrier preRenderBarrierID = {};
    preRenderBarrierID.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    preRenderBarrierID.srcAccessMask = 0; // No previous access
    preRenderBarrierID.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    preRenderBarrierID.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Or VK_IMAGE_LAYOUT_PRESENT_SRC_KHR if already presented
    preRenderBarrierID.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    preRenderBarrierID.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrierID.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrierID.image = ctx.IDImages[index]; // Use p_instance_params for swapchain images
    preRenderBarrierID.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    preRenderBarrierID.subresourceRange.baseMipLevel = 0;
    preRenderBarrierID.subresourceRange.levelCount = 1;
    preRenderBarrierID.subresourceRange.baseArrayLayer = 0;
    preRenderBarrierID.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,             // This barrier happens very early in the pipeline
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before writing to color attachment
        0,                                             // Dependency flags
        0, nullptr,                                    // Memory barriers
        0, nullptr,                                    // Buffer memory barriers
        1, &preRenderBarrier                           // Image memory barriers
    );

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,             // This barrier happens very early in the pipeline
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before writing to color attachment
        0,                                             // Dependency flags
        0, nullptr,                                    // Memory barriers
        0, nullptr,                                    // Buffer memory barriers
        1, &preRenderBarrierID                           // Image memory barriers
    );

    VkImageMemoryBarrier depthBarrier = {};
depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
depthBarrier.srcAccessMask = 0;
depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // not DEPTH_ATTACHMENT_OPTIMAL
depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
depthBarrier.image = ctx.depthImage;                 
depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
depthBarrier.subresourceRange.baseMipLevel = 0;
depthBarrier.subresourceRange.levelCount = 1;
depthBarrier.subresourceRange.baseArrayLayer = 0;
depthBarrier.subresourceRange.layerCount = 1;

vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &depthBarrier
);

    const VkRenderingAttachmentInfo color_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, // Non-KHR
        .imageView = ctx.swapchainImageViews[index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Use non-KHR
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor,
    };
    
    const VkRenderingAttachmentInfo id_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, // Non-KHR
        .imageView = ctx.IDImageViews[index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Use non-KHR
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .color = { .uint32 = {0, 0, 0, 0} } }
    };

    // Add depth attachment info
    const VkRenderingAttachmentInfo depth_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.depthImageView, // Assuming depthImageView is correctly setup
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Or STORE if you need to read depth later
        .clearValue = depthColor,                    // Use your depth clear value
    };

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = ctx.swapchainExtent;

    VkRenderingAttachmentInfo colorAttachmentInfos[] = {color_attachment_info, id_attachment_info};

    const VkRenderingInfo render_info{
        // Use non-KHR version if Vulkan 1.3+
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO, // Non-KHR
        .renderArea = scissor,
        .layerCount = 1,
        .colorAttachmentCount = 2,
        .pColorAttachments = colorAttachmentInfos,
        .pDepthAttachment = &depth_attachment_info, // Add depth attachment
        .pStencilAttachment = nullptr,              // Or &depth_attachment_info if using stencil
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
    for (int i = 0; i < ctx.pipelineBatches.size(); i++)
    {

        VkPipeline currentPipeline;

        for(int j = 0; j < ctx.pipelines.size();j++){
            if(ctx.pipelines[j]->id == ctx.pipelineBatches[i].pipelineIndex){
                currentPipeline = ctx.pipelines[j]->pipeline;
                break;
            }
        }

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdPushConstants(
            commandBuffer,
            ctx.globalPipelineLayout,              // The pipeline layout that defines the range
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // Must match stageFlags used in VkPushConstantRange
            0,                                                         // Offset into the push constant block (usually 0)
            sizeof(PushConstant),                                      // Size of the data being pushed
            &cnst              // Pointer to your C++ data
        );

        // Assuming these buffers are created and valid for the GraphicPipeline
        VkBuffer vertexBuffers[] = {ctx.globalVertexBuffer, ctx.instanceBuffer};
        VkDeviceSize offsets[] = {0, 0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

        VkBuffer indexBuffer = ctx.globalIndexBuffer; // Get index buffer from pipeline
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet descriptorSets[] = {ctx.cameraDescriptorSets[index], ctx.addressDescriptorSet, ctx.textureDescriptorSet};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.globalPipelineLayout, 0, 3, descriptorSets, 0, nullptr);
        // Make sure graphicPipeline->drawBuffer and graphicPipeline->drawCommands are valid
        vkCmdDrawIndexedIndirect(commandBuffer, ctx.drawBuffer, ctx.pipelineBatches[i].start*sizeof(VkDrawIndexedIndirectCommand), ctx.pipelineBatches[i].end - ctx.pipelineBatches[i].start, sizeof(VkDrawIndexedIndirectCommand));
    }

    vkCmdEndRendering(commandBuffer);        // end main pass (2 color attachments)

    // OUTLINE PASS

    VkImageMemoryBarrier2 preRenderBarrierOutlineColor{};
    preRenderBarrierOutlineColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    preRenderBarrierOutlineColor.image = ctx.swapchainImages[index];
    preRenderBarrierOutlineColor.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    preRenderBarrierOutlineColor.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    preRenderBarrierOutlineColor.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    preRenderBarrierOutlineColor.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    preRenderBarrierOutlineColor.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    preRenderBarrierOutlineColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    preRenderBarrierOutlineColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrierOutlineColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrierOutlineColor.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,1};

    VkImageMemoryBarrier2 preRenderBarrierOutlineID{};
    preRenderBarrierOutlineID.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    preRenderBarrierOutlineID.image = ctx.IDImages[index];
    preRenderBarrierOutlineID.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    preRenderBarrierOutlineID.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    preRenderBarrierOutlineID.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    preRenderBarrierOutlineID.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    preRenderBarrierOutlineID.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    preRenderBarrierOutlineID.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    preRenderBarrierOutlineID.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrierOutlineID.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preRenderBarrierOutlineID.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,1};


    VkImageMemoryBarrier2 outlineBarriers[] = {preRenderBarrierOutlineColor, preRenderBarrierOutlineID};

    VkDependencyInfo outlineDepInfo{};
    outlineDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    outlineDepInfo.imageMemoryBarrierCount = 2;
    outlineDepInfo.pImageMemoryBarriers = outlineBarriers;

    vkCmdPipelineBarrier2(commandBuffer, &outlineDepInfo);

    const VkRenderingAttachmentInfo color_attachment_info_outline{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, // Non-KHR
        .imageView = ctx.swapchainImageViews[index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Use non-KHR
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor,
    };

    // Add depth attachment info
    const VkRenderingAttachmentInfo depth_attachment_info_outline{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.depthImageView, // Assuming depthImageView is correctly setup
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Or STORE if you need to read depth later
        .clearValue = depthColor,                    // Use your depth clear value
    };



    VkRenderingAttachmentInfo colorAttachmentInfosOutline[] = {color_attachment_info_outline};

    const VkRenderingInfo render_info_outline{
        // Use non-KHR version if Vulkan 1.3+
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO, // Non-KHR
        .renderArea = scissor,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = colorAttachmentInfosOutline,
        .pDepthAttachment = &depth_attachment_info, // Add depth attachment
        .pStencilAttachment = nullptr,              // Or &depth_attachment_info if using stencil
    };



    // Use the function pointers loaded by vk_ctx
    vkCmdBeginRendering(commandBuffer, &render_info_outline);
    for (int i = 0; i < ctx.pipelineBatches.size(); i++)
    {

        VkPipeline currentPipeline = ctx.outlinePipeline->pipeline;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdPushConstants(
            commandBuffer,
            ctx.globalPipelineLayout,              // The pipeline layout that defines the range
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // Must match stageFlags used in VkPushConstantRange
            0,                                                         // Offset into the push constant block (usually 0)
            sizeof(PushConstant),                                      // Size of the data being pushed
            &cnst              // Pointer to your C++ data
        );

        // Assuming these buffers are created and valid for the GraphicPipeline
        VkBuffer vertexBuffers[] = {ctx.globalVertexBuffer, ctx.instanceBuffer};
        VkDeviceSize offsets[] = {0, 0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

        VkBuffer indexBuffer = ctx.globalIndexBuffer; // Get index buffer from pipeline
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet descriptorSets[] = {ctx.cameraDescriptorSets[index], ctx.addressDescriptorSet, ctx.textureDescriptorSet, ctx.IDImageSets[index]};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.globalPipelineLayout, 0, 4, descriptorSets, 0, nullptr);
        // Make sure graphicPipeline->drawBuffer and graphicPipeline->drawCommands are valid
        vkCmdDrawIndexedIndirect(commandBuffer, ctx.drawBuffer, ctx.pipelineBatches[i].start*sizeof(VkDrawIndexedIndirectCommand), ctx.pipelineBatches[i].end - ctx.pipelineBatches[i].start, sizeof(VkDrawIndexedIndirectCommand));
    }


    















    vkCmdEndRendering(commandBuffer);

    VkRenderingAttachmentInfo imguiColor{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.swapchainImageViews[index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,     // keep what the main pass drew
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    VkRenderingInfo imguiInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = scissor,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &imguiColor,
    };
    vkCmdBeginRendering(commandBuffer, &imguiInfo);
    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    vkCmdEndRendering(commandBuffer);

    // --- FINAL IMAGE LAYOUT TRANSITION FOR PRESENTATION ---
    VkImageMemoryBarrier postRenderBarrier = {};
    postRenderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postRenderBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // We just wrote to it
    postRenderBarrier.dstAccessMask = 0;                                    // No specific access after this for presentation
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
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Dst stage: Effectively, before presentation
        0,                                             // Dependency flags
        0, nullptr,                                    // Memory barriers
        0, nullptr,                                    // Buffer memory barriers
        1, &postRenderBarrier                          // Image memory barriers
    );

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void RenderQueue::guiListObjects()
{
}

void RenderQueue::renderUI()
{
    auto renderTotalStartTimer = std::chrono::high_resolution_clock::now();
    static bool dock_initialized = false;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();

    // Create a full-screen invisible host window
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                    ImGuiWindowFlags_NoBackground;
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("MainDockSpaceHost", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    // Create dockspace that fills entire screen
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);

    if (!dock_initialized)
    {
        dock_initialized = true;

        ImGui::DockBuilderRemoveNode(dockspace_id); // clear layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, io.DisplaySize);

        // Split dockspace
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_right, dock_id_left, dock_id_bottom, dock_id_center;

        dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
        dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
        dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

        // Dock windows
        ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
        ImGui::DockBuilderDockWindow("LeftPanel", dock_id_left);
        ImGui::DockBuilderDockWindow("Console", dock_id_bottom);

        ImGui::DockBuilderFinish(dockspace_id);
    }
    ImGui::End();
    auto renderLeftPanelTimerStart= std::chrono::high_resolution_clock::now();
    renderLeftPanel();
    auto renderLeftPanelTimerEnd = std::chrono::high_resolution_clock::now();
    diag_renderLeft = std::chrono::duration_cast<std::chrono::microseconds>(renderLeftPanelTimerEnd - renderLeftPanelTimerStart).count();

    auto renderRightPanelTimerStart= std::chrono::high_resolution_clock::now();
    renderRightPanel();
    auto renderRightPanelTimerEnd = std::chrono::high_resolution_clock::now();
    diag_renderRight = std::chrono::duration_cast<std::chrono::microseconds>(renderRightPanelTimerEnd - renderRightPanelTimerStart).count();
    
    auto renderConsoleTimerStart = std::chrono::high_resolution_clock::now();
    renderConsole();
    auto renderConsoleTimerEnd = std::chrono::high_resolution_clock::now();
    diag_renderConsole = std::chrono::duration_cast<std::chrono::microseconds>(renderConsoleTimerEnd - renderConsoleTimerStart).count();
    
    
    ImGui::Render();

    auto renderTotalEndTimer = std::chrono::high_resolution_clock::now();
    diag_totalRenderUI = std::chrono::duration_cast<std::chrono::microseconds>(renderTotalEndTimer - renderTotalStartTimer).count();

    // VERBOSE(GRAPHICS_PIPELINE_CTX, "Total UI render time: %f", diag_totalRenderUI);
    // VERBOSE(GRAPHICS_PIPELINE_CTX, "Left panel render time: %f", diag_renderLeft);
    // VERBOSE(GRAPHICS_PIPELINE_CTX, "Right panel render time: %f", diag_renderRight);
    // VERBOSE(GRAPHICS_PIPELINE_CTX, "Console render time: %f", diag_renderConsole);



}

void RenderQueue::renderLeftPanel()
{
    ImGui::Begin("LeftPanel");
  
    for (int j = 0; j < ctx.objects.size(); j++)
    {
        if (ctx.objects[j]->parentObject == nullptr)
        {
            std::stringstream ss;
            ss << "[" << ctx.objects[j]->objectID << "] "<< ctx.objects[j]->name.c_str();
            if (ctx.objects[j]->childObjects.size() > 0)
            {
                
                if (ImGui::TreeNode(ss.str().c_str()))
                {

                    
                    if (ImGui::IsItemClicked())
                    {
                        selectedItem = ctx.objects[j]->objectID;
                    }

                    addChildObjectsToList(ctx.objects[j]);
                    ImGui::TreePop();
                }
            }
            else
            {
                if (selectedItem == ctx.objects[j]->objectID)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                }

                ImGui::Text(ss.str().c_str());
                if (selectedItem == ctx.objects[j]->objectID)
                {
                    ImGui::PopStyleColor();
                }
            }
            if (ImGui::IsItemClicked())
            {
                selectedItem = ctx.objects[j]->objectID;
            }
        }
    }

        

       
    

    ImGui::End();
}

void RenderQueue::renderRightPanel()
{
    ImGui::Begin("Inspector");

    VmaAllocationInfo vertexStat;
    VmaAllocationInfo indexStat;
    VmaAllocationInfo instanceStat;
    VmaAllocationInfo drawStat;


    vmaGetAllocationInfo(ctx.allocator, ctx.globalVertexBufferAllocation, &vertexStat);
    vmaGetAllocationInfo(ctx.allocator, ctx.globalIndexBufferAllocation, &indexStat);
    vmaGetAllocationInfo(ctx.allocator, ctx.instanceBufferAllocation, &instanceStat);
    vmaGetAllocationInfo(ctx.allocator, ctx.drawBufferAllocation, &drawStat);

    std::stringstream infss;
    infss << "Vertex buffer memory usage: " << std::fixed << std::setprecision(3) << float(vertexStat.size)/(SIZE_MB)   << " MB";
    ImGui::Text(infss.str().c_str());
    infss.str("");

    infss << "Index buffer memory usage: " << std::fixed << std::setprecision(3) << float(indexStat.size)/(SIZE_MB)   << " MB";  
    ImGui::Text(infss.str().c_str());
    infss.str("");
    
    infss << "Instance buffer memory usage: " << std::fixed << std::setprecision(3) << float(instanceStat.size)/(SIZE_MB)   << " MB";  
    ImGui::Text(infss.str().c_str());
    infss.str("");
    
    infss << "Draw buffer memory usage: " << std::fixed << std::setprecision(3) << float(drawStat.size)/(SIZE_MB)   << " MB";  
    ImGui::Text(infss.str().c_str());

    ImGui::Separator(); 



    if (selectedItem > -1 && selectedItem < ctx.objectIDNext)
    {
        std::string tmpStr = "";

        tmpStr = "Object Properites";
        ImGui::Text(tmpStr.c_str());

        tmpStr = "Name: " + ctx.objectIDMap[selectedItem]->name;
        ImGui::Text(tmpStr.c_str());

        if (ctx.objectIDMap[selectedItem]->parentObject != nullptr)
        {
            tmpStr = "Parent object name: " + ctx.objectIDMap[selectedItem]->parentObject->name;
            ImGui::Text(tmpStr.c_str());
        }

        tmpStr = "Position: " + std::to_string(ctx.objectIDMap[selectedItem]->transformation.translation.x) + " " +
                 std::to_string(ctx.objectIDMap[selectedItem]->transformation.translation.y) + " " +
                 std::to_string(ctx.objectIDMap[selectedItem]->transformation.translation.z) + " ";
        ImGui::Text(tmpStr.c_str());
        
        tmpStr = "Primitive count: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives.size());

        if(ctx.objectIDMap[selectedItem]->primitives.size() > 0){
            tmpStr = "Primitives: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives.size());
            if(ImGui::TreeNode(tmpStr.c_str())){
                for(int i = 0; i < ctx.objectIDMap[selectedItem]->primitives.size(); i++){
                    tmpStr = std::to_string(i);
                    if(ImGui::TreeNode(tmpStr.c_str())){
                        tmpStr = "Vertex count: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].vertices.size());
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "Normal count: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].normals.size());
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "UV count: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].UV.size());
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "Vertex color count: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].colors.size());
                        ImGui::Text(tmpStr.c_str());
                        
                        tmpStr = "Index count: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].indices.size());
                        ImGui::Text(tmpStr.c_str());


                        tmpStr = "Material used: " + ctx.objectIDMap[selectedItem]->primitives[i].materialName
                        + " (" + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].materialIndex) + ")";
                        ImGui::Text(tmpStr.c_str());

                        std::stringstream ss;
                        ss << "0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(ctx.objectIDMap[selectedItem]->primitives[i].dataBuffer);

                        tmpStr = "Data buffer address: " + ss.str();
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "Data buffer offset: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].dataOffset);
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "Data buffer size: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].dataSize);
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "Stride: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].stride);
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "Model index: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].modelIndex);
                        ImGui::Text(tmpStr.c_str());
                        
                        tmpStr = "Draw index: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].drawIndex);
                        ImGui::Text(tmpStr.c_str());

                        tmpStr = "Draw mode: " + std::to_string(ctx.objectIDMap[selectedItem]->primitives[i].mode);
                        ImGui::Text(tmpStr.c_str());

                        
                        ImGui::TreePop();
                    }
                
                }
            
                ImGui::TreePop();
            
            }
        }
        ImGui::Text(tmpStr.c_str());

        ImGui::Text("Position (x,y,z): ");
        ImGui::SameLine();

        if(ImGui::InputFloat3("##position", &(ctx.objectIDMap[selectedItem]->transformation.translation.x), "%.3f")){

        }


        if(ctx.objectIDMap[selectedItem]->primitives.size() > 0){
            
        }
        tmpStr = "Material: " + ctx.objectIDMap[selectedItem]->name;
        ImGui::Text(tmpStr.c_str());
    }
    ImGui::End();
}

void RenderQueue::renderConsole()
{
        
        static char textBuffer[512];


        ImGui::Begin("Console");

        ImGui::BeginChild("OutputField", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);

        for (const ConsoleText& message : ctx.console->consoleBuffer) {
            ImGui::TextColored(message.color, message.text.c_str());
        }

        ImGui::EndChild();
        ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
        
        if(ImGui::InputText("##position", textBuffer, 512, input_flags)){
            if(strlen(textBuffer) > 0){
                ctx.console->processCommand(textBuffer);
            }
            textBuffer[0] = '\0';
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::End();
}

void RenderQueue::addChildObjectsToList(Object *obj)
{
    for (int i = 0; i < obj->childObjects.size(); i++)
    {
        std::stringstream ss;
        ss << "[" << obj->childObjects[i]->objectID << "] "<< obj->childObjects[i]->name.c_str();
        if (obj->childObjects[i]->childObjects.size() > 0)
        {
            if (ImGui::TreeNode(ss.str().c_str()))
            {
                
                if (ImGui::IsItemClicked())
                {
                    selectedItem = obj->childObjects[i]->objectID;
                }
                addChildObjectsToList(obj->childObjects[i]);
                ImGui::TreePop();
            }
        }
        else
        {
            if (selectedItem == obj->childObjects[i]->objectID)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            }
            ImGui::Text(ss.str().c_str());
            if (selectedItem == obj->childObjects[i]->objectID)
            {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemClicked())
            {
                selectedItem = obj->childObjects[i]->objectID;
            }
        }
    }
}

