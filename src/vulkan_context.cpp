#include "vulkan_context.hpp"
#include <GLFW/glfw3.h>
#include <colorlog.h>

#include <limits>

#define VMA_IMPLEMENTATION
#include "vma.h"
#include <iostream>

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

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
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
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context.depthImage, context.depthImageMemory);
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

    CTX::AUX::createImage(context, context.swapchainExtent.width, context.swapchainExtent.height, p_instance_params.msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context.colorImage, context.colorImageMemory);
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

void CTX::AUX::createBuffer(vk_ctx& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to create buffer! (AUX)");
        throw std::runtime_error("failed to create buffer! (AUX)");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context,memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to allocate buffer memory! (AUX)");
        throw std::runtime_error("failed to allocate buffer memory! (AUX)");
    }

    vkBindBufferMemory(context.device, buffer, bufferMemory, 0);
}

void CTX::AUX::createImage(vk_ctx& context, uint32_t width, uint32_t height, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
{
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

    if (vkCreateImage(context.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to create image! (AUX)");
        throw std::runtime_error("failed to create image! (AUX)");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = CTX::AUX::findMemoryType(context,memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to allocate image memory! (AUX)");
        throw std::runtime_error("failed to allocate image memory! (AUX)");
    }

    vkBindImageMemory(context.device, image, imageMemory, 0);
}

void CTX::AUX::createDImage(vk_ctx& context, uint32_t width, uint32_t height, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
{
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
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.samples = numSample;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to create image! (AUX)");
        throw std::runtime_error("failed to create image! (AUX)");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = CTX::AUX::findMemoryType(context,memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        ALERT(DEBUG_CTX, "Failed to allocate image memory! (AUX)");
        throw std::runtime_error("failed to allocate image memory! (AUX)");
    }

    vkBindImageMemory(context.device, image, imageMemory, 0);
}

uint32_t CTX::AUX::findMemoryType(vk_ctx& context, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    ALERT(DEBUG_CTX, "Failed to find suitable memory type!");
    throw std::runtime_error("failed to find suitable memory type!");
}

size_t CTX::AUX::allocateVertexMemory(vk_ctx &context, size_t stride, size_t size)
{
    size_t start = 0;
    if(context.vertexLast % stride != 0){
        start = context.vertexLast / stride + stride;
    }else{
        start = context.vertexLast / stride;
    }
    context.vertexLast = start;
    context.vertexLast += size * stride;
    return start;
}

size_t CTX::AUX::allocateIndexMemory(vk_ctx &context, size_t size)
{

    context.indexLast += size;

    return context.indexLast - size;
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

        vmaFreeMemory(ctx.allocator, stagingBufferAllocation);
}

void CTX::createCommandPool(vk_ctx& context)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = context.graphicsFamilyIndex;

    if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &(context.commandPool)) != VK_SUCCESS)
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
    VkDescriptorSetLayoutBinding VP_matrix{};
    VP_matrix.binding = 0;
    VP_matrix.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VP_matrix.descriptorCount = 1;
	VP_matrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	VP_matrix.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding VP_bindings[] = {VP_matrix};

	VkDescriptorSetLayoutCreateInfo VP_layoutInfo{};
	VP_layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VP_layoutInfo.bindingCount = 1;
	VP_layoutInfo.pBindings = VP_bindings;

    if (vkCreateDescriptorSetLayout(p_ctx.device, &VP_layoutInfo, nullptr, &(p_ctx.setCameraLayout)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout: VP matrix");
	}

    VkDescriptorSetLayoutBinding Model_matrix{};
    Model_matrix.binding = 0;
    Model_matrix.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	Model_matrix.descriptorCount = 64;
	Model_matrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	Model_matrix.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding Model_bindings[] = {Model_matrix};

	VkDescriptorSetLayoutCreateInfo Model_layoutInfo{};
	Model_layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	Model_layoutInfo.bindingCount = 1;
	Model_layoutInfo.pBindings = Model_bindings;

    if (vkCreateDescriptorSetLayout(p_ctx.device, &Model_layoutInfo, nullptr, &(p_ctx.setModelMatLayout)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout: Model matrix");
	}


    VkDescriptorSetLayoutBinding texture_set{};
    texture_set.binding = 0;
    texture_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_set.descriptorCount = 64;
	texture_set.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texture_set.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding texture_bindings[] = {texture_set};

	VkDescriptorSetLayoutCreateInfo texture_layoutInfo{};
	texture_layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	texture_layoutInfo.bindingCount = 1;
	texture_layoutInfo.pBindings = texture_bindings;

    if (vkCreateDescriptorSetLayout(p_ctx.device, &texture_layoutInfo, nullptr, &(p_ctx.setTextureLayout)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout: Model matrix");
	}

}

float CTX::getDeltaTime(){
    static float lastTime = 0;
    float currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);
	lastTime = currentTime;
    return deltaTime;
}

void CTX::reloadDrawBuffer(vk_ctx & ctx)
{
    ctx.drawCommands.clear();
    ctx.drawCommands.resize(ctx.primitiveData.size());

    for(int i = 0; i < ctx.drawCommands.size();i++){
        VkDrawIndexedIndirectCommand tmpCmd{};
        tmpCmd.firstIndex = ctx.primitiveData[i].startOffsetIndex;
        tmpCmd.firstInstance = 0;
        tmpCmd.indexCount = ctx.primitiveData[i].indexCount;
        tmpCmd.instanceCount = 1;
        tmpCmd.vertexOffset = ctx.primitiveData[i].startOffsetVertex;
        ctx.drawCommands[i] = tmpCmd;
    }

    CTX::AUX::uploadData(ctx, ctx.drawCommands.data(), ctx.drawBuffer, ctx.drawCommands.size() * sizeof(VkDrawIndexedIndirectCommand), 0);
}

void CTX::reloadInstanceBuffer(vk_ctx & ctx)
{
    ctx.instanceInfos.clear();
    ctx.instanceInfos.resize(ctx.primitiveData.size());

    for(int i = 0; i < ctx.instanceInfos.size();i++){
        ctx.instanceInfos[i].materialIndex = ctx.primitiveData[i].materialIndex;
        ctx.instanceInfos[i].modelIndex = ctx.primitiveData[i].modelIndex;
    }   

    CTX::AUX::uploadData(ctx, ctx.instanceInfos.data(), ctx.instanceBuffer, ctx.instanceInfos.size() * sizeof(InstanceInfo), 0);

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

	return info;
}





void CTX::initContext(vk_ctx& context, const vk_instance_params& p_instance_params){
    CTX::createWindow(context, p_instance_params);
	CTX::createInstance(context, p_instance_params);
	CTX::createSurface(context);
	CTX::pickPhysicalDevice(context, p_instance_params);
	CTX::createLogicalDevice(context, p_instance_params);
	CTX::createSwapchain(context, p_instance_params);
	CTX::createSwapchainImageViews(context);
	CTX::createColorResources(context, p_instance_params);
	CTX::createDepthResources(context, p_instance_params);
	CTX::createCommandPool(context);
	CTX::createCommandBuffers(context, p_instance_params);
    CTX::createGlobalDescriptorLayouts(context);
	CTX::createMemoryAllocator(context);
	CTX::createCameraResources(context);
    CTX::createGlobalBuffers(context);

}

void CTX::destroyContext(vk_ctx& context, const vk_instance_params& p_instance_params){
    INFO(DEBUG_CTX, "Starting cleanup routine of vulkan context");
    VERBOSE(DEBUG_CTX, "Destroying command pool...");
    vkDestroyCommandPool(context.device, context.commandPool, nullptr);
    // When command pool is destroyed, all command buffers are also implicitly freed.
    
    

    VERBOSE(DEBUG_CTX, "Destroying depth image view...");
    vkDestroyImageView(context.device, context.depthImageView, nullptr);
    VERBOSE(DEBUG_CTX, "Destroying depth image...");
    vkDestroyImage(context.device, context.depthImage, nullptr);
    VERBOSE(DEBUG_CTX, "Freeing depth image memory...");
    vkFreeMemory(context.device, context.depthImageMemory, nullptr);
    
    VERBOSE(DEBUG_CTX, "Destroying color image view...");
    vkDestroyImageView(context.device, context.colorImageView, nullptr);
    VERBOSE(DEBUG_CTX, "Destroying color image...");
    vkDestroyImage(context.device, context.colorImage, nullptr);
    VERBOSE(DEBUG_CTX, "Freeing color image memory...");
    vkFreeMemory(context.device, context.colorImageMemory, nullptr);

    VERBOSE(DEBUG_CTX, "Destroying swapchain image views...");
    for (auto imageView : context.swapchainImageViews)
    {
        vkDestroyImageView(context.device, imageView, nullptr);
    }

    VERBOSE(DEBUG_CTX, "Destroying swapchain...");
    vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);

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



    CTX::AUX::createBuffer(ctx, (VkDeviceSize)SIZE_MB*50, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    ctx.deviceBuffer, ctx.deviceBufferAllocation);

    VkBufferDeviceAddressInfo addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = ctx.deviceBuffer;

    ctx.bufferAddress = vkGetBufferDeviceAddress(ctx.device, &addressInfo);


}


void CTX::AUX::copyBuffer(vk_ctx& ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
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

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset; // Optional
	copyRegion.dstOffset = dstOffset; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx.graphicsQueue); // TODO: Implement async transfer

	vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}