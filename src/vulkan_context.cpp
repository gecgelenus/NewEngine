#include "vulkan_context.hpp"
#include <GLFW/glfw3.h>
#include <colorlog.h>

#include <limits>

#define VMA_IMPLEMENTATION
#include "vma.h"
#include <iostream>

#include <object.hpp>
#include "console.hpp"

/*
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    // Optional: if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    //    // You can add a breakpoint here to catch warnings/errors in your debugger
    // }

    // The callback returns a VkBool32, indicating whether the Vulkan call that triggered
    // the message should be aborted. VK_FALSE means the application should continue.
    // For debugging, returning VK_TRUE might be useful to immediately stop at an error.
    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional: A pointer to user-defined data passed to the callback
    createInfo.flags = 0; 
}

VkDebugUtilsMessengerEXT debugMessenger;

// Helper function to create the debug messenger
VkResult CreateDebugUtilsMessengerEXT(vk_ctx& ctx, VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    // Dynamically load the extension function address
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void setupDebugMessenger(vk_ctx& ctx) {

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(ctx, ctx.instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

*/
void CTX::createInstance(vk_ctx& context, const vk_instance_params& p_instance_params){

    context.textureSet.resize(64);


    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    //extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    if(p_instance_params.enableValidationLayers){
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    

    if (vkCreateInstance(&createInfo, nullptr, &(context.instance)) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Instance couldn't created");
        throw std::runtime_error("failed to create instance!");
    }else{
        SUCCESS(DEBUG_CTX, "Instance created");
    }

}


void CTX::pickPhysicalDevice(vk_ctx& context, const vk_instance_params& p_instance_params){

    context.physicalDevice = nullptr;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);

    if(deviceCount == 0){
        ALERT(DEBUG_CTX, "Couldn't find any supported physical device");
        throw std::runtime_error("Couldn't find any supported physical device");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());
    
    for(VkPhysicalDevice p_device: devices){
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(p_device, &deviceProperties);
        std::cout << "device: " << deviceProperties.deviceName << std::endl;
        std::cout << "type: " << deviceProperties.deviceType << std::endl;


        if (deviceProperties.deviceType == p_instance_params.physicalDeviceType) {
            std::cout << "Selected device: " << deviceProperties.deviceName << std::endl;

            context.physicalDevice = p_device;
            break;
        }

    }

    if(context.physicalDevice == nullptr){
        ALERT(DEBUG_CTX, "Couldn't find any suitable GPU type.");
        throw std::runtime_error("Couldn't find any suitable GPU type.");
    }else{
        SUCCESS(DEBUG_CTX, "Selected physical device.");
    }

}


void CTX::createSurface(vk_ctx& context){
        if (glfwCreateWindowSurface(context.instance, context.window, nullptr, &(context.surface)) != VK_SUCCESS)
		{
            ALERT(DEBUG_CTX,"failed to create window surface!");
			throw std::runtime_error("failed to create window surface!");
		}else{
            SUCCESS(DEBUG_CTX, "Created window surface");
        }
}


void CTX::createWindow(vk_ctx& context, const vk_instance_params& p_instance_params){
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if(p_instance_params.windowResizable == false){
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }else{
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    }
    

    context.window = glfwCreateWindow(p_instance_params.windowWidth, p_instance_params.windowHeight, p_instance_params.windowTitle.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(context.window);

    if(context.window == nullptr){
        ALERT(DEBUG_CTX, "Window couldn't created.");
    }else{
        SUCCESS(DEBUG_CTX, "Window created");
    }

    if(p_instance_params.windowStickKeys == true){
        glfwSetInputMode(context.window, GLFW_STICKY_KEYS, GL_TRUE);
    }else{
        glfwSetInputMode(context.window, GLFW_STICKY_KEYS, GL_FALSE);
}

if(p_instance_params.windowStartCursorDisabled == true){
    glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
}

void CTX::createLogicalDevice(vk_ctx& context, const vk_instance_params& p_instance_params){

    float queuePriority = 1.0f;

    context.graphicsFamilyIndex = CTX::AUX::getGraphicsQueueIndex(context.physicalDevice);

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = context.graphicsFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
            .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;
    deviceFeatures.multiDrawIndirect = VK_TRUE;
    
    

    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.runtimeDescriptorArray = VK_TRUE; // Enable the feature
    vulkan12Features.descriptorIndexing = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan12Features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;



    vulkan12Features.pNext = &dynamic_rendering_feature;




        


    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features = deviceFeatures; // Set traditional features here
    deviceFeatures2.pNext = &vulkan12Features; // Link Vulkan 1.2 features
    


    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;

    createInfo.pEnabledFeatures = VK_NULL_HANDLE;

    if(p_instance_params.enableValidationLayers == true){
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = &deviceFeatures2;

    

    if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &(context.device)) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Logical device coulnd't created.");
        throw std::runtime_error("Logical device coulnd't created.");
    }else{
        SUCCESS(DEBUG_CTX, "Logical device created");
    }
    vkGetDeviceQueue(context.device, context.graphicsFamilyIndex, 0, &(context.graphicsQueue));
    vkGetDeviceQueue(context.device, context.graphicsFamilyIndex, 0, &(context.transferQueue));


}

uint32_t CTX::AUX::getGraphicsQueueIndex(VkPhysicalDevice physicalDevice){
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, families.data());

    for(int i = 0; i < queueFamilyCount; i++){
        if((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)){
            return i;
        }
    }
    



    return 0; 
    // TODO: Possible bug, make this return code more interesting 
    // so we can see there is an error.
}

VkSurfaceFormatKHR CTX::AUX::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR CTX::AUX::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D CTX::AUX::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,const vk_ctx& context)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(context.window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

SwapChainSupportDetails CTX::AUX::querySwapChainSupport(const vk_ctx& context)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

    void CTX::createSwapchain(vk_ctx& context, const vk_instance_params& p_instance_params)
	{
		SwapChainSupportDetails swapChainSupport = CTX::AUX::querySwapChainSupport(context);

		VkSurfaceFormatKHR surfaceFormat = CTX::AUX::chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = CTX::AUX::chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = CTX::AUX::chooseSwapExtent(swapChainSupport.capabilities, context);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		context.minImage = swapChainSupport.capabilities.minImageCount;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = context.surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;	  // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		context.swapchainImageFormat = surfaceFormat.format;
		context.swapchainExtent = extent;

		if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &(context.swapchain)) != VK_SUCCESS)
		{
            ALERT(DEBUG_CTX, "Swapchain couldn't created");
			throw std::runtime_error("Swapchain couldn't created");
		}else{
            SUCCESS(DEBUG_CTX, "Created swapchain");
        }

		vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, nullptr);
		context.swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, context.swapchainImages.data());
        SUCCESS(DEBUG_CTX, "Created swapchain images");
	}


    void CTX::createSwapchainImageViews(vk_ctx& context){
        context.swapchainImageViews.resize(context.swapchainImages.size());

		for (size_t i = 0; i < context.swapchainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = context.swapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = context.swapchainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(context.device, &createInfo, nullptr, &(context.swapchainImageViews[i])) != VK_SUCCESS)
			{
                ALERT(DEBUG_CTX, "Swapchain image view couldn't created");
				throw std::runtime_error("Swapchain image view couldn't created");
			}
		}
        SUCCESS(DEBUG_CTX, "Created swapchain image views");
    }

void CTX::createDepthResources(vk_ctx& context, const vk_instance_params& p_instance_params)
{
    CTX::AUX::createImage(context, context.swapchainExtent.width, context.swapchainExtent.height, p_instance_params.msaaSamples, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, context.depthImage, context.depthImageAllocation);
    
    SUCCESS(DEBUG_CTX, "Created depth image");

    VkImageViewCreateInfo createInfo{}; 
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = context.depthImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context.device, &createInfo, nullptr, &(context.depthImageView)) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to depth image view!");
        throw std::runtime_error("Failed to depth image view!");
    }else{
        SUCCESS(DEBUG_CTX, "Created depth image view");
    }
}

void CTX::createColorResources(vk_ctx& context, const vk_instance_params& p_instance_params)
{
    VkFormat colorFormat = context.swapchainImageFormat;
    SUCCESS(DEBUG_CTX, "Created color image");

    CTX::AUX::createImage(context, context.swapchainExtent.width, context.swapchainExtent.height, 
        p_instance_params.msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
         VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
           context.colorImage, context.colorImageAllocation);
    
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = context.colorImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = context.swapchainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context.device, &createInfo, nullptr, &(context.colorImageView)) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to create color image view");
        throw std::runtime_error("Failed to create color image view");
    }else{
        SUCCESS(DEBUG_CTX, "Created color image view");
    }
}


void CTX::AUX::uploadData(vk_ctx &ctx, void *p_data, VkBuffer p_dstBuffer, size_t p_size, uint64_t p_dstOffset)
{
        
        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;

        CTX::AUX::createBuffer(ctx, p_size, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            stagingBuffer, stagingBufferAllocation);

        void* data;
        vmaMapMemory(ctx.allocator, stagingBufferAllocation, &data);
        memcpy(data, p_data, (size_t)p_size);
        vmaUnmapMemory(ctx.allocator, stagingBufferAllocation);


        CTX::AUX::copyBuffer(ctx, stagingBuffer, p_dstBuffer, p_size,0 ,p_dstOffset);
        ctx.bufferAllocations.erase(stagingBufferAllocation);
        vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingBufferAllocation);
}


void CTX::AUX::uploadDataDeviceBuffer(vk_ctx &ctx, void *p_data, VkDeviceAddress& deviceAddress, VmaAllocation& p_allocation, VkBuffer& p_dstBuffer, size_t p_size, uint64_t p_dstOffset)
{

        VmaAllocationInfo allocationInfo;
        vmaGetAllocationInfo(ctx.allocator, p_allocation, &allocationInfo);

        if(allocationInfo.size < p_size){
            CTX::AUX::enlargeBuffer(ctx, p_size, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, p_dstBuffer, p_allocation);
            
            VkBufferDeviceAddressInfo addressInfo{};
            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.buffer = p_dstBuffer;

            deviceAddress = vkGetBufferDeviceAddress(ctx.device, &addressInfo);
        }


        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;

        CTX::AUX::createBuffer(ctx, p_size, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            stagingBuffer, stagingBufferAllocation);

        void* data;
        vmaMapMemory(ctx.allocator, stagingBufferAllocation, &data);
        memcpy(data, p_data, (size_t)p_size);
        vmaUnmapMemory(ctx.allocator, stagingBufferAllocation);


        CTX::AUX::copyBuffer(ctx, stagingBuffer, p_dstBuffer, p_size,0 ,p_dstOffset);
        ctx.bufferAllocations.erase(stagingBufferAllocation);
        vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingBufferAllocation);
}

void CTX::createCommandPool(vk_ctx& context)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags =  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = context.graphicsFamilyIndex;

    if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &(context.commandPool)) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to create command pool");
        throw std::runtime_error("Failed to create command pool!");
    }else{
        SUCCESS(DEBUG_CTX, "Created command buffer");
    }


    VkCommandPoolCreateInfo poolInfo2{};
    poolInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo2.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo2.queueFamilyIndex = context.graphicsFamilyIndex;

    if (vkCreateCommandPool(context.device, &poolInfo2, nullptr, &(context.commandPoolCopy)) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to create command pool");
        throw std::runtime_error("Failed to create command pool!");
    }else{
        SUCCESS(DEBUG_CTX, "Created command buffer");
    }
}

void CTX::createCommandBuffers(vk_ctx& context, const vk_instance_params& p_instance_params)
{
    context.commandBuffers.resize(p_instance_params.framesOnFlight);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)context.commandBuffers.size();

    if (vkAllocateCommandBuffers(context.device, &allocInfo, context.commandBuffers.data()) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to allocate command buffers");
        throw std::runtime_error("Failed to allocate command buffers!");
    }else{
        SUCCESS(DEBUG_CTX, "Created command buffers, size: %u", (unsigned int)context.commandBuffers.size());
    }
}

void CTX::createMemoryAllocator(vk_ctx& context){
    VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorCreateInfo.physicalDevice = context.physicalDevice;
	allocatorCreateInfo.device = context.device;
	allocatorCreateInfo.instance = context.instance;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;


	if(vmaCreateAllocator(&allocatorCreateInfo, &(context.allocator)) != VK_SUCCESS){
        ALERT(DEBUG_CTX, "Failed to create memory allocator");
        throw std::runtime_error("Failed to create memory allocator");
    }else{
        SUCCESS(DEBUG_CTX, "Created memory allocator");
    }
}
void CTX::createGlobalDescriptorLayouts(vk_ctx& p_ctx){
    VkDescriptorSetLayoutBinding CameraMatrix{};
    CameraMatrix.binding = 0;
    CameraMatrix.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	CameraMatrix.descriptorCount = 1;
	CameraMatrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	CameraMatrix.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding CameraBindings[] = {CameraMatrix};

	VkDescriptorSetLayoutCreateInfo Camera_layoutInfo{};
	Camera_layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	Camera_layoutInfo.bindingCount = 1;
	Camera_layoutInfo.pBindings = CameraBindings;

    if (vkCreateDescriptorSetLayout(p_ctx.device, &Camera_layoutInfo, nullptr, &(p_ctx.setCameraLayout)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout: VP matrix");
	}

    VkDescriptorSetLayoutBinding AddressMatrix{};
    AddressMatrix.binding = 0;
    AddressMatrix.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	AddressMatrix.descriptorCount = 1;
	AddressMatrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	AddressMatrix.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding AddressBindings[] = {AddressMatrix};

	VkDescriptorSetLayoutCreateInfo AddressLayoutInfo{};
	AddressLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	AddressLayoutInfo.bindingCount = 1;
	AddressLayoutInfo.pBindings = AddressBindings;

    if (vkCreateDescriptorSetLayout(p_ctx.device, &AddressLayoutInfo, nullptr, &(p_ctx.setAddressLayout)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout: Model matrix");
	}


    VkDescriptorSetLayoutBinding texture_set{};
    texture_set.binding = 0;
    texture_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_set.descriptorCount = MAX_TEXTURE_BIND;
	texture_set.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texture_set.pImmutableSamplers = nullptr;

    VkDescriptorBindingFlags bindlessFlags =
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | // Allows variable count at alloc time
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |          // Allows some array elements to be unbound
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |        // Crucial for bindless: update after set is bound
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT; // Allows updating even if GPU is pending on it


    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{};
    bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsCreateInfo.pNext = nullptr;
    bindingFlagsCreateInfo.bindingCount = 1;
    bindingFlagsCreateInfo.pBindingFlags = &bindlessFlags;


    VkDescriptorSetLayoutBinding texture_bindings[] = {texture_set};

	VkDescriptorSetLayoutCreateInfo texture_layoutInfo{};
	texture_layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	texture_layoutInfo.bindingCount = 1;
	texture_layoutInfo.pBindings = texture_bindings;
    texture_layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    texture_layoutInfo.pNext = &bindingFlagsCreateInfo;

    if (vkCreateDescriptorSetLayout(p_ctx.device, &texture_layoutInfo, nullptr, &(p_ctx.setTextureLayout)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout: Model matrix");
	}


    VkDescriptorSetLayout layouts[] = { p_ctx.setCameraLayout , p_ctx.setAddressLayout, p_ctx.setTextureLayout};


	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Accessible by VS and FS
	pushConstantRange.offset = 0; // Start from the beginning of the push constant block
	pushConstantRange.size = sizeof(PushConstant); // Size of your data

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
	pipelineLayoutInfo.pSetLayouts = layouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	if (vkCreatePipelineLayout(p_ctx.device, &pipelineLayoutInfo, nullptr, &(p_ctx.globalPipelineLayout)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

}

float CTX::getDeltaTime(){
    static float lastTime = 0;
    float currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);
	lastTime = currentTime;
    return deltaTime;
}

void CTX::destroyBuffer(vk_ctx &ctx, VkBuffer buffer, VmaAllocation allocation)
{
    ctx.bufferAllocations.erase(allocation);
    vmaDestroyBuffer(ctx.allocator, buffer, allocation);
}

void CTX::checkExpiredAllocations(vk_ctx &ctx)
{

    for(int i = ctx.expiredAllocations.size()-1; i >= 0; i--){
        if(ctx.expiredAllocations[i].remainingCycle == 0){
            destroyBuffer(ctx, ctx.expiredAllocations[i].buffer, ctx.expiredAllocations[i].allocation);
            ctx.expiredAllocations.erase( ctx.expiredAllocations.begin() + i);
        }else{
            ctx.expiredAllocations[i].remainingCycle--;
        }
    }
}

void CTX::createCameraResources(vk_ctx& ctx){
    VkDeviceSize VPBufferSize = sizeof(glm::mat4);
    
    ctx.camera.buffers.resize(ctx.swapchainImageViews.size());
	ctx.camera.bufferAllocations.resize(ctx.swapchainImageViews.size());
	ctx.camera.mappedData.resize(ctx.swapchainImageViews.size());

	for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++) {
		

		VmaAllocationInfo info =  CTX::AUX::createBuffer(ctx, VPBufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ctx.camera.buffers[i], ctx.camera.bufferAllocations[i]);


		ctx.camera.mappedData[i] = info.pMappedData;
	}

    ctx.camera.horizontalAngle = 3.14f;
    ctx.camera.verticalAngle = 0.0f;

    ctx.camera.speed = 5.0f;
    ctx.camera.sensivity = 0.5f;

    ctx.camera.FOV = 45.0f;
    ctx.camera.nearPlane = 0.01f;
    ctx.camera.farPlane = 500.0f;

    ctx.camera.inputEnabled = false;

}

VmaAllocationInfo CTX::AUX::createBuffer(vk_ctx& context, VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& allocation)
{
	VmaAllocationInfo info;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = memoryType;
	vmaCreateBuffer(context.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &info);
    
    context.bufferAllocations.insert({allocation, buffer});
	return info;
}

VmaAllocationInfo CTX::AUX::createImage(vk_ctx &ctx, uint32_t width, uint32_t height, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage &image, VmaAllocation &allocation)
{

	    VmaAllocationInfo info;


        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSample;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        vmaCreateImage(ctx.allocator, &imageInfo, &allocInfo, &image, &allocation, &info);

    return info;
}

void CTX::AUX::enlargeBuffer(vk_ctx &ctx, VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer &buffer, VmaAllocation &allocation)
{
    VmaAllocation tmpAllocation;
    VkBuffer tmpBuffer;
    
    CTX::AUX::createBuffer(ctx, size, memoryType, usage, tmpBuffer, tmpAllocation);

    VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(ctx.allocator, allocation, &allocationInfo);


    std::cout << "Enlarging buffer from " << allocationInfo.size << " to " << size << std::endl;

    CTX::AUX::copyBuffer(ctx, buffer, tmpBuffer, allocationInfo.size, 0, 0);

    //ctx.bufferAllocations.erase(allocation);
    //vmaDestroyBuffer(ctx.allocator, buffer, allocation);
    
    AllocationBundle tmpBundle;
    tmpBundle.buffer = buffer;
    tmpBundle.allocation = allocation;
    tmpBundle.remainingCycle = ctx.swapchainImageViews.size() + 1;

    ctx.expiredAllocations.push_back(tmpBundle);


    buffer = tmpBuffer;
    allocation = tmpAllocation;

}

void CTX::initContext(vk_ctx& context, const vk_instance_params& p_instance_params){
    CTX::createWindow(context, p_instance_params);
	CTX::createInstance(context, p_instance_params);
	CTX::createSurface(context);
	CTX::pickPhysicalDevice(context, p_instance_params);
	CTX::createLogicalDevice(context, p_instance_params);
	CTX::createMemoryAllocator(context);

	CTX::createSwapchain(context, p_instance_params);
	CTX::createSwapchainImageViews(context);
	CTX::createColorResources(context, p_instance_params);
	CTX::createDepthResources(context, p_instance_params);
	CTX::createCommandPool(context);
    //setupDebugMessenger(context);
	CTX::createCommandBuffers(context, p_instance_params);
	CTX::createCameraResources(context);
    CTX::createGlobalBuffers(context);
    CTX::createGlobalDescriptorLayouts(context);
    CTX::createGlobalDescriptorPool(context);
    CTX::allocateGlobalDescriptorSets(context);

}

void CTX::destroyContext(vk_ctx& context, const vk_instance_params& p_instance_params){
    INFO(DEBUG_CTX, "Starting cleanup routine of vulkan context");
    VERBOSE(DEBUG_CTX, "Destroying command pool...");
    vkDestroyCommandPool(context.device, context.commandPool, nullptr);
    vkDestroyCommandPool(context.device, context.commandPoolCopy, nullptr);
    
    for(int i = 0; i < context.objects.size();i++){
        for(int j = 0; j < context.objects[i]->primitives.size(); j++){
            vmaVirtualFree(context.globalVertexVirtualBlock, context.objects[i]->primitives[j].virtualVertexAllocation);
            vmaVirtualFree(context.globalIndexVirtualBlock, context.objects[i]->primitives[j].virtualIndexAllocation);
        }
    }


    CTX::AUX::destroyBuffer(context, context.deviceBuffer, context.deviceBufferAllocation);
    CTX::AUX::destroyBuffer(context, context.addressBuffer, context.addressBufferAllocation);
    CTX::AUX::destroyBuffer(context, context.materialBuffer, context.materialBufferAllocation);


    CTX::AUX::destroyBuffer(context, context.globalVertexBuffer, context.globalVertexBufferAllocation);
    CTX::AUX::destroyBuffer(context, context.globalIndexBuffer, context.globalIndexBufferAllocation);
    CTX::AUX::destroyBuffer(context, context.instanceBuffer, context.instanceBufferAllocation);
    CTX::AUX::destroyBuffer(context, context.drawBuffer, context.drawBufferAllocation);

    vmaDestroyVirtualBlock(context.globalVertexVirtualBlock);
    vmaDestroyVirtualBlock(context.globalIndexVirtualBlock);


    vkDestroyDescriptorSetLayout(context.device, context.setCameraLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, context.setAddressLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, context.setTextureLayout, nullptr);

    vkDestroyDescriptorPool(context.device, context.globalDescriptorPool, nullptr);
    vkDestroyPipelineLayout(context.device, context.globalPipelineLayout, nullptr);



    for(int i = 0; i < context.camera.bufferAllocations.size();i++){

        CTX::AUX::destroyBuffer(context, context.camera.buffers[i], context.camera.bufferAllocations[i]);

       
    }

    for(VkSampler s: context.samplers){
		vkDestroySampler(context.device, s, nullptr);
	}

	for(auto i: context.images){
		vmaDestroyImage(context.allocator, i.second, i.first);
	}
	
	for(VkImageView iw: context.imageViews){
		vkDestroyImageView(context.device, iw, nullptr);
	}



    VERBOSE(DEBUG_CTX, "Destroying depth image view...");
    vkDestroyImageView(context.device, context.depthImageView, nullptr);
    VERBOSE(DEBUG_CTX, "Destroying depth image...");
    vmaDestroyImage(context.allocator, context.depthImage, context.depthImageAllocation);
    
    VERBOSE(DEBUG_CTX, "Destroying color image view...");
    vkDestroyImageView(context.device, context.colorImageView, nullptr);
    VERBOSE(DEBUG_CTX, "Destroying color image...");
    vmaDestroyImage(context.allocator, context.colorImage, context.colorImageAllocation);

    VERBOSE(DEBUG_CTX, "Destroying swapchain image views...");
    for (auto imageView : context.swapchainImageViews)
    {
        vkDestroyImageView(context.device, imageView, nullptr);
    }

    VERBOSE(DEBUG_CTX, "Destroying swapchain...");
    vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);

    if(context.bufferAllocations.size() > 0){
        std::cout << "Dangling buffers: " << context.bufferAllocations.size() << std::endl;
        for(auto alloc: context.bufferAllocations){
            vmaDestroyBuffer(context.allocator, alloc.second, alloc.first);
        }
    }

    vmaDestroyAllocator(context.allocator);

    VERBOSE(DEBUG_CTX, "Destroying logical device...");
    vkDestroyDevice(context.device, nullptr);

    VERBOSE(DEBUG_CTX, "Destroying surface...");
    vkDestroySurfaceKHR(context.instance, context.surface, nullptr);

    VERBOSE(DEBUG_CTX, "Destroying instance...");
    vkDestroyInstance(context.instance, nullptr);

    VERBOSE(DEBUG_CTX, "Destroying window...");
    glfwDestroyWindow(context.window);

    VERBOSE(DEBUG_CTX, "Terminating GLFW...");
    glfwTerminate();

    INFO(DEBUG_CTX, "Vulkan context cleaned up");
    
}

void CTX::recreateSwapchain(vk_ctx& context, const vk_instance_params& p_instance_params){
    VERBOSE(DEBUG_CTX, "Recreating swapchain...");
    VERBOSE(DEBUG_CTX, "Destroying swapchain image views...");
    for (auto imageView : context.swapchainImageViews)
    {
        vkDestroyImageView(context.device, imageView, nullptr);
    }

    VERBOSE(DEBUG_CTX, "Destroying swapchain...");
    vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);

    createSwapchain(context, p_instance_params);
    createSwapchainImageViews(context);
}



void CTX::createGlobalBuffers(vk_ctx& ctx){




    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB*400, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    ctx.globalVertexBuffer, ctx.globalVertexBufferAllocation);

    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB*50, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    ctx.instanceBuffer, ctx.instanceBufferAllocation);

    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB*50, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    ctx.globalIndexBuffer, ctx.globalIndexBufferAllocation);

        CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB*50, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
    ctx.drawBuffer, ctx.drawBufferAllocation);


    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    ctx.deviceBuffer, ctx.deviceBufferAllocation);

    VkBufferDeviceAddressInfo addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = ctx.deviceBuffer;

    ctx.bufferAddress = vkGetBufferDeviceAddress(ctx.device, &addressInfo);



    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    ctx.materialBuffer, ctx.materialBufferAllocation);

    VkBufferDeviceAddressInfo addressInfoMaterial{};
    addressInfoMaterial.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfoMaterial.buffer = ctx.materialBuffer;

    ctx.materialBufferAddress = vkGetBufferDeviceAddress(ctx.device, &addressInfoMaterial);

    
    
    
    
    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    ctx.addressBuffer, ctx.addressBufferAllocation);




    VmaVirtualBlockCreateInfo virtualBlockInfo = {};
    virtualBlockInfo.size = SIZE_MB*400; // Must match the real buffer's size

    vmaCreateVirtualBlock(&virtualBlockInfo, &ctx.globalVertexVirtualBlock);

    virtualBlockInfo.size = SIZE_MB*50; // Must match the real buffer's size

    vmaCreateVirtualBlock(&virtualBlockInfo, &ctx.globalIndexVirtualBlock);




}


void CTX::AUX::copyBuffer(vk_ctx& ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = ctx.commandPoolCopy;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer tmpCommandBuffer;
	vkAllocateCommandBuffers(ctx.device, &allocInfo, &tmpCommandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(tmpCommandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset; // Optional
	copyRegion.dstOffset = dstOffset; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(tmpCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(tmpCommandBuffer);


        // Create a fence for this specific submission
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0; // Create an unsignaled fence

    VkFence copyFence;
    if (vkCreateFence(ctx.device, &fenceInfo, nullptr, &copyFence) != VK_SUCCESS) {
        vkFreeCommandBuffers(ctx.device, ctx.commandPoolCopy, 1, &tmpCommandBuffer); // Clean up
        throw std::runtime_error("Failed to create copy fence!");
    }


	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tmpCommandBuffer;


	if (vkQueueSubmit(ctx.transferQueue, 1, &submitInfo, copyFence) != VK_SUCCESS) {
        vkDestroyFence(ctx.device, copyFence, nullptr);
        vkFreeCommandBuffers(ctx.device, ctx.commandPoolCopy, 1, &tmpCommandBuffer);
        throw std::runtime_error("Failed to submit copy command buffer!");
    }
	vkWaitForFences(ctx.device, 1, &copyFence, VK_TRUE, UINT64_MAX); // Wait indefinitely

    // Clean up the fence and command buffer
    vkDestroyFence(ctx.device, copyFence, nullptr);
    vkFreeCommandBuffers(ctx.device, ctx.commandPoolCopy, 1, &tmpCommandBuffer);
}

void CTX::AUX::copyBuffer(vk_ctx &ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, std::vector<VkBufferCopy> &cmds)
{
    VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = ctx.commandPoolCopy;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer tmpCommandBuffer;
	vkAllocateCommandBuffers(ctx.device, &allocInfo, &tmpCommandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(tmpCommandBuffer, &beginInfo);

	vkCmdCopyBuffer(tmpCommandBuffer, srcBuffer, dstBuffer, cmds.size(), cmds.data());

	vkEndCommandBuffer(tmpCommandBuffer);


        // Create a fence for this specific submission
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0; // Create an unsignaled fence

    VkFence copyFence;
    if (vkCreateFence(ctx.device, &fenceInfo, nullptr, &copyFence) != VK_SUCCESS) {
        vkFreeCommandBuffers(ctx.device, ctx.commandPoolCopy, 1, &tmpCommandBuffer); // Clean up
        throw std::runtime_error("Failed to create copy fence!");
    }


	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tmpCommandBuffer;


	if (vkQueueSubmit(ctx.transferQueue, 1, &submitInfo, copyFence) != VK_SUCCESS) {
        vkDestroyFence(ctx.device, copyFence, nullptr);
        vkFreeCommandBuffers(ctx.device, ctx.commandPoolCopy, 1, &tmpCommandBuffer);
        throw std::runtime_error("Failed to submit copy command buffer!");
    }
	vkWaitForFences(ctx.device, 1, &copyFence, VK_TRUE, UINT64_MAX); // Wait indefinitely

    vkDestroyFence(ctx.device, copyFence, nullptr);
    vkFreeCommandBuffers(ctx.device, ctx.commandPoolCopy, 1, &tmpCommandBuffer);
}

void CTX::createGlobalDescriptorPool(vk_ctx& ctx){


    VkDescriptorPoolSize CameraSize{};
	CameraSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	CameraSize.descriptorCount = static_cast<uint32_t>(ctx.swapchainImageViews.size());

    VkDescriptorPoolSize AddressSize{};
	AddressSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	AddressSize.descriptorCount = static_cast<uint32_t>(ctx.swapchainImageViews.size()) * 1;

    VkDescriptorPoolSize TextureSize{};
	TextureSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	TextureSize.descriptorCount = static_cast<uint32_t>(ctx.swapchainImageViews.size()) * MAX_TEXTURE_BIND;

	VkDescriptorPoolSize sizes[] = { CameraSize, AddressSize, TextureSize};


    VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 3;
	poolInfo.pPoolSizes = sizes;
	poolInfo.maxSets = static_cast<uint32_t>(ctx.swapchainImageViews.size()*3);
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	if (vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &(ctx.globalDescriptorPool)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool");
	}
}
void CTX::allocateGlobalDescriptorSets(vk_ctx& ctx){
    
    std::vector<VkDescriptorSetLayout> CameraLayouts(ctx.swapchainImageViews.size(), ctx.setCameraLayout);


	VkDescriptorSetAllocateInfo CameraAllocInfo{};
	CameraAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	CameraAllocInfo.descriptorPool = ctx.globalDescriptorPool;
	CameraAllocInfo.descriptorSetCount = static_cast<uint32_t>(ctx.swapchainImageViews.size());
	CameraAllocInfo.pSetLayouts = CameraLayouts.data();

	ctx.cameraDescriptorSets.resize(ctx.swapchainImageViews.size());
	if (vkAllocateDescriptorSets(ctx.device, &CameraAllocInfo, ctx.cameraDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}


    for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++) {

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = ctx.camera.buffers[i]; // <--- Your actual camera uniform buffer
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(glm::mat4); // The size of the data in your buffer

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = ctx.cameraDescriptorSets[i]; // The descriptor set to update
		descriptorWrite.dstBinding = 0;                       // Matches layout(binding = 0) in shader
		descriptorWrite.dstArrayElement = 0;                  // If it's not an array, this is 0
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Matches layout in shader
		descriptorWrite.descriptorCount = 1;                  // Matches descriptorCount in layout
		descriptorWrite.pBufferInfo = &bufferInfo;            // Link to your buffer info
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);
	}


    VkDescriptorSetAllocateInfo AddressAllocInfo{};
	AddressAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AddressAllocInfo.descriptorPool = ctx.globalDescriptorPool;
	AddressAllocInfo.descriptorSetCount = 1;
	AddressAllocInfo.pSetLayouts = &(ctx.setAddressLayout);

    if (vkAllocateDescriptorSets(ctx.device, &AddressAllocInfo, &(ctx.addressDescriptorSet)) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

    uint32_t variableDescriptorCount = MAX_TEXTURE_BIND; 

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo{};
    variableDescriptorCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableDescriptorCountInfo.descriptorSetCount = 1;
    variableDescriptorCountInfo.pDescriptorCounts = &variableDescriptorCount;

        
    VkDescriptorSetAllocateInfo TextureAllocInfo{};
    TextureAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    TextureAllocInfo.pNext = &variableDescriptorCountInfo; // Link the variable count info
    TextureAllocInfo.descriptorPool = ctx.globalDescriptorPool;
    TextureAllocInfo.descriptorSetCount = 1;
    TextureAllocInfo.pSetLayouts = &(ctx.setTextureLayout);

    if (vkAllocateDescriptorSets(ctx.device, &TextureAllocInfo, &(ctx.textureDescriptorSet)) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}



    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = ctx.addressBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(glm::mat4);



    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = ctx.addressDescriptorSet; // The descriptor set to update
    descriptorWrite.dstBinding = 0;                       // Matches layout(binding = 0) in shader
    descriptorWrite.dstArrayElement = 0;                  // If it's not an array, this is 0
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Matches layout in shader
    descriptorWrite.descriptorCount = 1;                  // Matches descriptorCount in layout
    descriptorWrite.pBufferInfo = &bufferInfo;            // Link to your buffer info
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);



}



void CTX::AUX::copyBufferToImage(vk_ctx& ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = ctx.commandPoolCopy;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx.graphicsQueue);

	vkFreeCommandBuffers(ctx.device, ctx.commandPoolCopy, 1, &commandBuffer);
}

void CTX::AUX::transitionImageLayout(vk_ctx& ctx ,VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = ctx.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx.graphicsQueue);

	vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

void CTX::reloadObjectData(vk_ctx& ctx){

    uint32_t drawIndexCounter = 0;
    ctx.instanceInfos.clear();
    ctx.drawCommands.clear();
    ctx.primitiveData.clear();

    for(int i = 0; i < ctx.objects.size(); i++){
        for(int j = 0; j < ctx.objects[i]->primitives.size(); j++){
            CTX::AUX::uploadData(ctx, ctx.objects[i]->primitives[j].dataBuffer, ctx.globalVertexBuffer,
                ctx.objects[i]->primitives[j].dataSize, ctx.objects[i]->primitives[j].virtualVertexOffset);

            CTX::AUX::uploadData(ctx, ctx.objects[i]->primitives[j].indices.data(), ctx.globalIndexBuffer,
                ctx.objects[i]->primitives[j].indices.size()*sizeof(uint32_t), ctx.objects[i]->primitives[j].virtualIndexOffset);
            
            ctx.objects[i]->primitives[j].modelIndex = ctx.objects[i]->modelIndex;
            
            
            ctx.primitiveData.push_back(&ctx.objects[i]->primitives[j]);
            


        }
    }

};



void CTX::AUX::updateModelMatrixRecursive(Object *obj, std::vector<VkBufferCopy>& regions)
{
	obj->localMatrix = glm::mat4(1.0f);

	if(obj->transformation != obj->oldTransformation){
		obj->oldTransformation = obj->transformation;
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), obj->transformation.translation);
		glm::mat4 rotationMatrix = glm::mat4_cast(obj->transformation.rotation);
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), obj->transformation.scale);


		obj->localMatrix = translationMatrix * rotationMatrix * scaleMatrix;
		if(obj->parentObject != nullptr){
			obj->modelMatrix = obj->parentObject->modelMatrix * obj->localMatrix;
		}else{
			obj->modelMatrix = obj->localMatrix;
		}

		VkBufferCopy copyCmd{};
		copyCmd.dstOffset = sizeof(glm::mat4)*obj->modelIndex;
		copyCmd.srcOffset = sizeof(glm::mat4)*obj->modelIndex;
		copyCmd.size = sizeof(glm::mat4);
		regions.push_back(copyCmd);
	}

	if(obj->childObjects.size() > 0){
		for(int i = 0; i < obj->childObjects.size(); i++){
			CTX::AUX::updateModelMatrixRecursive(obj->childObjects[i], regions);
		}
	}
}

void CTX::updateModelMatrices(vk_ctx& ctx)
{
    
	std::vector<VkBufferCopy> regions;
	for(int i = 0; i < ctx.objects.size();i++){

		if(ctx.objects[i]->parentObject == nullptr){
			CTX::AUX::updateModelMatrixRecursive(ctx.objects[i], regions);
		}

	}

	
	if(regions.size() <= 0){
		return;
	}


	for(int i = 0; i < ctx.objects.size();i++){
		
		ctx.modelMatrixList[ctx.objects[i]->modelIndex] = ctx.objects[i]->modelMatrix;
	}


	
	// Check if buffer is enough to contain data
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

	// Copy data
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	CTX::AUX::createBuffer(ctx, modelDataSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
		VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		stagingBuffer, stagingBufferAllocation);

	void* data;
	vmaMapMemory(ctx.allocator, stagingBufferAllocation, &data);
	memcpy(data, ctx.modelMatrixList.data(), (size_t)modelDataSize);
	vmaUnmapMemory(ctx.allocator, stagingBufferAllocation);


	CTX::AUX::copyBuffer(ctx, stagingBuffer, ctx.deviceBuffer, regions);

	ctx.bufferAllocations.erase(stagingBufferAllocation);
	vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingBufferAllocation);
}

void CTX::AUX::updateTextureSets(vk_ctx& ctx)
{
	for (size_t frameIndex = 0; frameIndex < ctx.swapchainImageViews.size(); frameIndex++) {

            std::vector<VkDescriptorImageInfo> descriptorInfos;

            // Populate all 64 descriptorInfos
            for (int textureIndex = 0; textureIndex < ctx.textureSet.size(); textureIndex++) {


                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                if(ctx.textureSet[textureIndex].enabled){
				imageInfo.imageView = ctx.textureSet[textureIndex].textureImageView;
                imageInfo.sampler = ctx.textureSet[textureIndex].textureSampler; // <--- ASSIGN THE SAMPLER HERE!
				}else{
									imageInfo.imageView = ctx.textureSet[0].textureImageView;
                	imageInfo.sampler = ctx.textureSet[0].textureSampler; // <--- ASSIGN THE SAMPLER HERE!

				}

                descriptorInfos.push_back(imageInfo);
            }

            VkWriteDescriptorSet descriptorWriteTex{};
            descriptorWriteTex.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWriteTex.dstSet = ctx.textureDescriptorSet; // Use outer loop variable
            descriptorWriteTex.dstBinding = 0;
            descriptorWriteTex.dstArrayElement = 0;
            descriptorWriteTex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // <--- CHANGE TYPE HERE!
            descriptorWriteTex.descriptorCount = descriptorInfos.size();
            descriptorWriteTex.pImageInfo = descriptorInfos.data();
            descriptorWriteTex.pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(ctx.device, 1, &descriptorWriteTex, 0, nullptr);
        }

}

bool CTX::AUX::LoadModelTextures(vk_ctx& ctx, tinygltf::Model& model, std::string& path) {




    if (model.images.empty()) {
        std::cout << "Model has no images." << std::endl;
        return true; // No images to load, not an error
    }

    // Step 1: Load all raw images
    for (size_t i = 0; i < model.images.size(); ++i) {
        tinygltf::Image& gltfImage = model.images[i];

		VkImage textureImage;
		VkImageView textureImageView;

		VkSampler textureSampler;

		if(gltfImage.name.empty()){
			std::cout << "WARNING: Texture being loaded has no name. It might conflict with pipeline or texture slots." << std::endl;
		}

        std::string tmpTexName = path + std::to_string(i);
		bool textureExists = false;

		for(TextureSlot slot: ctx.textureSet){
			if(slot.name == tmpTexName){
				std::cout << "Texture: \"" << gltfImage.uri << "\" is already in texture set. Skipping this texture." << std::endl; 
				textureExists = true;
			}
		}

		if(textureExists){
			continue;
		}

        // tinygltf loads image data into gltfImage.image (std::vector<unsigned char>)
        // and sets gltfImage.width, gltfImage.height, gltfImage.component (channels)

        // Check if image data is directly embedded or loaded from an external file
        if (!gltfImage.image.empty() && gltfImage.width > 0 && gltfImage.height > 0) {

			
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(ctx.physicalDevice, &properties);
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;

			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;

			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;

			if (vkCreateSampler(ctx.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture sampler!");
			}

			ctx.samplers.push_back(textureSampler);


			VkBuffer stagingBufferTexture;
			VkDeviceMemory stagingBufferMemoryTexture;

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = gltfImage.width * gltfImage.height * gltfImage.component;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

			VkBuffer buffer;
			VmaAllocation allocation;
			vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocCreateInfo, &buffer, &allocation, nullptr);

			void* data;
			vmaMapMemory(ctx.allocator, allocation, &data);
			memcpy(data, gltfImage.image.data(), static_cast<size_t>(gltfImage.width * gltfImage.height * gltfImage.component));
			vmaUnmapMemory(ctx.allocator, allocation);


			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = gltfImage.width;
			imageInfo.extent.height = gltfImage.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

			VmaAllocation alloc;
			vmaCreateImage(ctx.allocator, &imageInfo, &allocInfo, &textureImage, &alloc, nullptr);
			ctx.images.insert({alloc, textureImage});

			CTX::AUX::transitionImageLayout(ctx, textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			CTX::AUX::copyBufferToImage(ctx, buffer, textureImage, static_cast<uint32_t>(gltfImage.width), static_cast<uint32_t>(gltfImage.height));

			CTX::AUX::transitionImageLayout(ctx, textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = textureImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}
			ctx.imageViews.push_back(textureImageView);

			vmaDestroyBuffer(ctx.allocator, buffer, allocation);



            

			for(int i = 0; i < ctx.textureSet.size();i++){
				if(!ctx.textureSet[i].enabled){
					ctx.textureSet[i].enabled = true;
					ctx.textureSet[i].name = tmpTexName;
					ctx.textureSet[i].textureImage = textureImage;
					ctx.textureSet[i].textureImageView = textureImageView;
					ctx.textureSet[i].textureSampler = textureSampler;
					  
					std::cout << "Loaded image '" << tmpTexName << "' ("
                      << gltfImage.width << "x" << gltfImage.height << ", "
                      << gltfImage.component << " channels) on index " << i << std::endl;

					break;
				}
			}

        } else {
            // This case should ideally not happen if tinygltf successfully loaded the model.
            // It might indicate a problem with the glTF file itself (e.g., invalid image URI)
            std::cerr << "Warning: Image data for texture is empty or invalid. Skipping." << std::endl;
        }
    }

	/*
    std::cout << "\nProcessing glTF Textures (linking images with samplers):" << std::endl;
    for (size_t i = 0; i < model.textures.size(); ++i) {
        const tinygltf::Texture& gltfTexture = model.textures[i];
        std::cout << "  Texture " << i << ": ";

        // gltfTexture.source is the index into model.images
        if (gltfTexture.source >= 0 && gltfTexture.source < loadedTextures.size()) {
            std::cout << "References image: " << loadedTextures[gltfTexture.source].name << std::endl;
            // You would typically use gltfTexture.sampler (index into model.samplers)
            // to get filtering and wrap modes for this specific texture.
        } else {
            std::cout << "References invalid image index: " << gltfTexture.source << std::endl;
        }
    }
		*/

    return true;
}


void CTX::AUX::processGltfFile(vk_ctx& ctx, std::string& path) {
    
	tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    std::cout << "Loading GLTF file: " << path << std::endl;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);

	if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
    printf("Failed to parse glTF\n");
    return;
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

    std::cout << "Processing scene: " << scene.name << std::endl;
	std::stringstream ss;
	ss << "Processing scene: " << scene.name;
	ctx.console->output(ss.str(), IMGUI_COLOR_BLUE);

	CTX::AUX::LoadModelTextures(ctx, model, path);
	CTX::AUX::updateTextureSets(ctx);


    // Iterate through the root nodes of the scene
    for (int nodeIdx : scene.nodes) {
        const tinygltf::Node& node = model.nodes[nodeIdx];
        

        processNode(ctx, model, path, node, nullptr); // Start with identity matrix for root

    }

    int objectsAfter = ctx.objects.size();

	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(ctx.allocator, ctx.materialBufferAllocation, &allocationInfo);
	VkDeviceSize materialDataSize = ctx.materialList.size()*sizeof(Material);

	if(allocationInfo.size < materialDataSize){
		CTX::AUX::enlargeBuffer(ctx, materialDataSize, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    		ctx.materialBuffer, ctx.materialBufferAllocation);
		
		
		VkBufferDeviceAddressInfo addressInfoMaterial{};
    	addressInfoMaterial.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    	addressInfoMaterial.buffer = ctx.materialBuffer;

    	ctx.materialBufferAddress = vkGetBufferDeviceAddress(ctx.device, &addressInfoMaterial);
	}
	
	


	CTX::AUX::uploadData(ctx, ctx.materialList.data(), ctx.materialBuffer, ctx.materialList.size()*sizeof(Material), 0);
	
}

// Recursive helper function to traverse the node hierarchy
void CTX::AUX::processNode(vk_ctx& ctx, tinygltf::Model& model, std::string& path, const tinygltf::Node& node, Object* parentObject) {
    
	Object* obj = new Object(ctx);

	obj->parentObject = parentObject;
	obj->modelIndex = ctx.modelMatrixCheck.size();

	ctx.modelMatrixCheck.push_back(true);
	ctx.modelMatrixList.push_back(glm::mat4(1.0f));

	obj->name = node.name;

    std::cout << "Creating object: " << obj->name << " (ID: " << obj->objectID << ")" << std::endl;

	if(parentObject != nullptr){
		std::cout << "Loading object: " << obj->name  << "(" << obj->objectID << ") child of" << parentObject->name << std::endl;

		parentObject->childObjects.push_back(obj);
		obj->parentTransformation = parentObject->transformation;

		obj->transformation.translation = glm::vec3(0.0f); 
		obj->transformation.rotation = glm::quat();    
		obj->transformation.scale = glm::vec3(1.0f);      
	}else{
		std::cout << "Loading object: " << obj->name << "(" << obj->objectID << ")" <<std::endl;

		obj->parentTransformation = {};

		obj->transformation.translation = glm::vec3(1.0f); // Default to no translation
		obj->transformation.rotation = glm::quat();    // Default to no rotation (identity quaternion)
		obj->transformation.scale = glm::vec3(1.0f);      // Default to no scaling (identity scale)
	}

    // Check if translation is provided, otherwise use default
    if (node.translation.size() == 3) {
        obj->transformation.translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
	}
    // Check if rotation is provided, otherwise use default
    if (node.rotation.size() == 4) {
        // Correctly construct glm::quat from glTF (x, y, z, w) order
        obj->transformation.rotation = glm::quat(
            node.rotation[3], // w
            node.rotation[0], // x
            node.rotation[1], // y
            node.rotation[2]  // z
        );
    }
    // Check if scale is provided, otherwise use default
    if (node.scale.size() == 3) {
        obj->transformation.scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
    }





    // If this node has a mesh, process it
    if (node.mesh > -1) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];

        // Iterate over primitives within the mesh
        for (const auto& primitive : mesh.primitives) {

			ObjectPrimitive tmpPrimitive;
			tmpPrimitive.mode = primitive.mode;
            
			if (primitive.attributes.count("POSITION")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                // Ensure data type is float (most common for positions)
                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    std::cerr << "Warning: POSITION attribute is not float type. Skipping." << std::endl;
                    continue;
                }
                // Ensure it's a VEC3 (most common for positions)
                if (accessor.type != TINYGLTF_TYPE_VEC3) {
                     std::cerr << "Warning: POSITION attribute is not VEC3 type. Skipping." << std::endl;
                    continue;
                }

                // Calculate the start of the data within the buffer
                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                // Determine stride (bytes between consecutive vertices)
                size_t stride = accessor.ByteStride(bufferView); // Use accessor.ByteStride for correct stride

                // Iterate through vertices
                for (size_t i = 0; i < accessor.count; ++i) {
                    const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                    tmpPrimitive.vertices.emplace_back(values[0], values[1], values[2]);
                }
            }

            // --- NORMAL data (similar logic) ---
            if (primitive.attributes.count("NORMAL")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC3) {
                    std::cerr << "Warning: NORMAL attribute is not float VEC3. Skipping." << std::endl;
                    continue;
                }

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t stride = accessor.ByteStride(bufferView);

                for (size_t i = 0; i < accessor.count; ++i) {
                    const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                    tmpPrimitive.normals.emplace_back(values[0], values[1], values[2]);
                }
            }

            // --- TEXCOORD_0 data (similar logic) ---
            if (primitive.attributes.count("TEXCOORD_0")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC2) {
                    std::cerr << "Warning: TEXCOORD_0 attribute is not float VEC2. Skipping." << std::endl;
                    continue;
                }

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t stride = accessor.ByteStride(bufferView);

                for (size_t i = 0; i < accessor.count; ++i) {
                    const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                    tmpPrimitive.UV.emplace_back(values[0], values[1]);
                }
            }

			    if (primitive.attributes.count("COLOR_0")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("COLOR_0")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t stride = accessor.ByteStride(bufferView);

                // Vertex colors can be VEC3 (RGB) or VEC4 (RGBA)
                // They can also have different component types (FLOAT, UNSIGNED_BYTE, UNSIGNED_SHORT)
                // When componentType is not FLOAT, they should be normalized in the shader.
                // Here, we'll read them into floats, normalizing if necessary.

                for (size_t i = 0; i < accessor.count; ++i) {
                    glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f); // Default to opaque black

                    if (accessor.type == TINYGLTF_TYPE_VEC3) {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                            const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                            color = glm::vec4(values[0], values[1], values[2], 1.0f);
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                            const uint8_t* values = reinterpret_cast<const uint8_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 255.0f,
                                static_cast<float>(values[1]) / 255.0f,
                                static_cast<float>(values[2]) / 255.0f,
                                1.0f
                            );
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* values = reinterpret_cast<const uint16_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 65535.0f,
                                static_cast<float>(values[1]) / 65535.0f,
                                static_cast<float>(values[2]) / 65535.0f,
                                1.0f
                            );
                        } else {
                            std::cerr << "Warning: Unsupported COLOR_0 component type for VEC3: " << accessor.componentType << std::endl;
                        }
                    } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                            const float* values = reinterpret_cast<const float*>(dataPtr + i * stride);
                            color = glm::vec4(values[0], values[1], values[2], values[3]);
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                            const uint8_t* values = reinterpret_cast<const uint8_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 255.0f,
                                static_cast<float>(values[1]) / 255.0f,
                                static_cast<float>(values[2]) / 255.0f,
                                static_cast<float>(values[3]) / 255.0f
                            );
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* values = reinterpret_cast<const uint16_t*>(dataPtr + i * stride);
                            color = glm::vec4(
                                static_cast<float>(values[0]) / 65535.0f,
                                static_cast<float>(values[1]) / 65535.0f,
                                static_cast<float>(values[2]) / 65535.0f,
                                static_cast<float>(values[3]) / 65535.0f
                            );
                        } else {
                            std::cerr << "Warning: Unsupported COLOR_0 component type for VEC4: " << accessor.componentType << std::endl;
                        }
                    } else {
                        std::cerr << "Warning: COLOR_0 attribute is not VEC3 or VEC4 type. Skipping." << std::endl;
                    }
                    tmpPrimitive.colors.emplace_back(color); // Assuming you have a std::vector<glm::vec4> colors in ObjectPrimitive
                }
            }
            
            // You can also access primitive.material here:
            if (primitive.material > -1) {
                tinygltf::Material& material = model.materials[primitive.material];
				tmpPrimitive.materialName = material.name;
				std::string tmpPath = path;

				int materialIndex = CTX::AUX::findMaterialIndex(ctx, tmpPrimitive.materialName);
				
				if(materialIndex == -1){
					materialIndex = CTX::AUX::createMaterial(ctx, tmpPath, model, material);
				}

				tmpPrimitive.materialIndex = materialIndex;

			}

			if (primitive.indices > -1) { // Check if indices are defined for this primitive
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

                // Determine the type of indices and copy them
                switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                        const uint8_t* indices = reinterpret_cast<const uint8_t*>(dataPtr);
                        for (size_t i = 0; i < accessor.count; ++i) {
                            tmpPrimitive.indices.push_back(static_cast<uint32_t>(indices[i]));
                        }
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        const uint16_t* indices = reinterpret_cast<const uint16_t*>(dataPtr);
                        for (size_t i = 0; i < accessor.count; ++i) {
                            tmpPrimitive.indices.push_back(static_cast<uint32_t>(indices[i]));
                        }
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                        const uint32_t* indices = reinterpret_cast<const uint32_t*>(dataPtr);
                        // Can copy directly if all your indices are uint32_t
                        tmpPrimitive.indices.assign(indices, indices + accessor.count);
                        break;
                    }
                    default:
                        std::cerr << "Warning: Unsupported index component type: " << accessor.componentType << std::endl;
                        break;
                }

            }
			tmpPrimitive.modelIndex = obj->modelIndex;
			obj->primitives.push_back(tmpPrimitive);

		}
    }
	
	ctx.objects.push_back(obj);
	std::cout << "Added object to context. Total objects: " << ctx.objects.size() << std::endl;
	

    // Recursively process children nodes
	if(node.children.size() > 0){
		
		
		for (int childIdx : node.children) {
    		
			const tinygltf::Node& childNode = model.nodes[childIdx];
        	processNode(ctx, model, path, childNode, obj); // Pass the accumulated transform
    	}
	}
    
}



int CTX::AUX::findTextureIndex(vk_ctx& ctx, std::string & p_name)
{

	for(int i = 0; i < ctx.textureSet.size();i++){
		if(ctx.textureSet[i].enabled && ctx.textureSet[i].name == p_name){
			return i;
		}
	}

	std::cout << "Couldn't find texture for material use: " << p_name << std::endl;

    return -1;
}

int CTX::AUX::findMaterialIndex(vk_ctx& ctx, std::string & p_name)
{

	for(int i = 0; i < ctx.materialNames.size();i++){
		if(ctx.materialNames[i] == p_name){
			return i;
		}
	}


    return -1;
}

int CTX::AUX::createMaterial(vk_ctx& ctx, std::string& path, tinygltf::Model& model, tinygltf::Material &material)
{

	//std::cout << "Creating material:" << material.name << std::endl;

	Material tmpMat;
	tmpMat.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	if (material.pbrMetallicRoughness.baseColorFactor.size() == 4) {
		tmpMat.baseColorFactor = glm::vec4(
			material.pbrMetallicRoughness.baseColorFactor[0],
			material.pbrMetallicRoughness.baseColorFactor[1],
			material.pbrMetallicRoughness.baseColorFactor[2],
			material.pbrMetallicRoughness.baseColorFactor[3]
		);

		
		tmpMat.baseColorFactorEnabled = 1;
	
	}else{
		tmpMat.baseColorFactorEnabled = 0;
	}

	if(material.pbrMetallicRoughness.baseColorTexture.index > -1){
		int texIndex = CTX::AUX::findTextureIndex(ctx, path.append(std::to_string(model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source)));
		//std::cout << "Material: " << material.name  << " uses texture " << path << std::endl;

		if(texIndex < 0){
			tmpMat.textureIndex = 0;
			tmpMat.textureEnabled = 0;
			
		}else{
			tmpMat.textureIndex = texIndex;
			tmpMat.textureEnabled = 1;
		}
	}else{
		tmpMat.textureEnabled = 0;
		tmpMat.textureIndex = 0;
	}

	ctx.materialNames.push_back(material.name);
	ctx.materialList.push_back(tmpMat);

	if(ctx.materialList.size() != ctx.materialNames.size()){
		throw std::runtime_error("Internal consistency on material list!");
	}

	

    return ctx.materialList.size()-1;
}

void CTX::sortObjectPrimitives(vk_ctx& ctx)
{
    std::sort(ctx.primitiveData.begin(), ctx.primitiveData.end(),
        [](const ObjectPrimitive* a, const ObjectPrimitive* b) {
            return a->pipelineIndex < b->pipelineIndex;
        });

    ctx.pipelineBatches.clear();
    
    uint32_t currentPipelineIndex = ctx.primitiveData[0]->pipelineIndex;
    uint32_t lastPipelineEnd = 0;
    for(int i = 0; i < ctx.primitiveData.size();i++){
        if(currentPipelineIndex != ctx.primitiveData[i]->pipelineIndex){
            PipelineBatch tmpBatch{};
            tmpBatch.pipelineIndex = currentPipelineIndex;
            tmpBatch.start = lastPipelineEnd;
            tmpBatch.end = i;
            ctx.pipelineBatches.push_back(tmpBatch);
            lastPipelineEnd = i;
            currentPipelineIndex = ctx.primitiveData[i]->pipelineIndex;
        }
    }

    PipelineBatch tmpBatch{};
    tmpBatch.pipelineIndex = currentPipelineIndex;
    tmpBatch.start = lastPipelineEnd;
    tmpBatch.end = ctx.primitiveData.size();
    ctx.pipelineBatches.push_back(tmpBatch);
    
    std::cout << "Primitive count: " << ctx.primitiveData.size() << std::endl;

    for(int i = 0; i < ctx.pipelineBatches.size(); i++){
        std::cout << "Pipeline index: " << ctx.pipelineBatches[i].pipelineIndex << " start: " << 
        ctx.pipelineBatches[i].start << " end: " << 
        ctx.pipelineBatches[i].end << " " << std::endl;
    }

    ctx.instanceInfos.clear();
    ctx.drawCommands.clear();
    uint32_t drawIndexCounter = 0;
    
    for(int i = 0; i < ctx.primitiveData.size();i++){
        ctx.primitiveData[i]->drawIndex = drawIndexCounter;

        InstanceInfo tmpInfo{};
        tmpInfo.modelIndex = ctx.primitiveData[i]->modelIndex;
        tmpInfo.materialIndex = ctx.primitiveData[i]->materialIndex;
        
        ctx.instanceInfos.push_back(tmpInfo);

        VkDrawIndexedIndirectCommand tmpCmd{};
        tmpCmd.firstIndex = ctx.primitiveData[i]->virtualIndexOffset / sizeof(uint32_t);
        tmpCmd.indexCount = ctx.primitiveData[i]->indices.size();
        tmpCmd.firstInstance = drawIndexCounter;
        tmpCmd.instanceCount = 1;
        tmpCmd.vertexOffset = ctx.primitiveData[i]->virtualVertexOffset / ctx.primitiveData[i]->stride;

        ctx.drawCommands.push_back(tmpCmd);


        drawIndexCounter++;
    }
    
    

    CTX::AUX::uploadData(ctx, ctx.instanceInfos.data(), ctx.instanceBuffer,
                ctx.instanceInfos.size()*sizeof(InstanceInfo), 0);

    CTX::AUX::uploadData(ctx, ctx.drawCommands.data(), ctx.drawBuffer,
                ctx.drawCommands.size()*sizeof(VkDrawIndexedIndirectCommand), 0);
}


void CTX::AUX::destroyBuffer(vk_ctx& ctx, VkBuffer buffer, VmaAllocation allocation){
        
    ctx.bufferAllocations.erase(allocation);
    vmaDestroyBuffer(ctx.allocator, buffer, allocation);
}
