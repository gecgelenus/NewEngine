#pragma once
#include <unordered_map>
#include "util.hpp"
#include "console.hpp"
#include "vma.h"


#include <GLFW/glfw3.h>

#include <stdexcept>

#define DEBUG_CTX "Vulkan Context"

#define MAX_TEXTURE_BIND 8192



struct PushConstant{
    uint64_t modelBufferAddress;
    uint64_t materialBufferAddress;

};

class Object;

struct InstanceInfo{
    int32_t modelIndex;
    int32_t materialIndex;
};


struct AllocationBundle{
    VkBuffer buffer;
    VmaAllocation allocation;
    uint32_t remainingCycle;
};


struct ObjectTransformation{
    glm::vec3 translation;
    glm::quat rotation;

    glm::vec3 scale;
    glm::mat4 matrix;

    bool operator==(const ObjectTransformation&) const = default;
    

};


struct Material{
    int32_t textureEnabled;
    int32_t baseColorFactorEnabled;

    int32_t textureIndex;
    int32_t padding;
    glm::vec4 baseColorFactor;
};


struct ObjectPrimitive{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec2> UV;

    std::vector<uint32_t> indices;

    std::string materialName;
    
    void* dataBuffer = nullptr;
    
    uint32_t dataSize;
    uint32_t dataOffset;

    uint32_t pipelineIndex;


    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t drawIndex = 0;

    int32_t modelIndex;
    int32_t materialIndex;

    size_t startOffsetVertex = 0;
    size_t vertexCount = 0;
    size_t endOffsetVertex = 0;

    size_t startOffsetIndex = 0;
    size_t indexCount = 0;
    size_t endOffsetIndex = 0;

    size_t stride = 0;

    uint32_t mode;
};


struct Camera{

    float inputEnabled;

    float FOV;
    float nearPlane;
    float farPlane;

    float horizontalAngle;
    float verticalAngle;

    
    float speed;
    float sensivity;
    
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;

    std::vector<VkBuffer> buffers;
    std::vector<VmaAllocation> bufferAllocations; 
    std::vector<void*> mappedData; 



};

struct TextureSlot{
    bool enabled = false;
    std::string name;
    VkImage textureImage;
    VkImageView textureImageView;
    VkSampler textureSampler;
};

struct vk_ctx
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue transferQueue;
    
    
    uint32_t graphicsFamilyIndex = 0;

    VkDescriptorPool globalDescriptorPool; // Used for camera, buffer addresses and textures

    VkDescriptorSetLayout setCameraLayout;
    VkDescriptorSetLayout setAddressLayout;
    VkDescriptorSetLayout setTextureLayout;

    std::vector<VkDescriptorSet> cameraDescriptorSets;
    VkDescriptorSet addressDescriptorSet;
    VkDescriptorSet textureDescriptorSet;

    VkPipelineLayout globalPipelineLayout;





    VmaAllocator allocator;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    std::unordered_map<VmaAllocation, VkBuffer> bufferAllocations; 
    std::vector<AllocationBundle> expiredAllocations;

    VkBuffer instanceBuffer;
    VmaAllocation instanceBufferAllocation;

    VkBuffer drawBuffer;
    VmaAllocation drawBufferAllocation;

    VkBuffer addressBuffer;
    VmaAllocation addressBufferAllocation;

    
    std::vector<ObjectPrimitive> primitiveData;
    std::vector<VkDrawIndexedIndirectCommand> drawCommands;
    std::vector<InstanceInfo> instanceInfos;


    std::vector<bool> modelMatrixCheck;
    std::vector<glm::mat4> modelMatrixList;

    std::vector<std::string> materialNames;
    std::vector<Material> materialList;

    uint32_t objectIDNext = 0;
    std::unordered_map<uint32_t, Object*> objectIDMap;


    VkBuffer deviceBuffer;
    VmaAllocation deviceBufferAllocation;
    VkDeviceAddress bufferAddress;
    
    VkBuffer materialBuffer;
    VmaAllocation materialBufferAllocation;
    VkDeviceAddress materialBufferAddress;
    
    
    GLFWwindow* window;

    uint32_t minImage = 0;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    VkImage depthImage;
	VmaAllocation depthImageAllocation;
	VkImageView depthImageView;

	VkImage colorImage;
    VmaAllocation colorImageAllocation;
    VkImageView colorImageView;

    VkCommandPool commandPool;
    VkCommandPool commandPoolCopy;

    std::vector<VkCommandBuffer> commandBuffers;
    Camera camera;
    std::vector<TextureSlot> textureSet;
    
    ConsoleInstance* console;
    

};

struct vk_instance_params{
    // INSTANCE PARAMETERS
    bool enableValidationLayers = false;
    
    // PHYSICAL DEVICE PARAMETERS
    uint32_t physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    // LOGICAL DEVICE PARAMETERS
    bool samplerAnisotropy = false;
    bool sampleRateShading = false;
    bool multiDrawIndirect = false;
    bool dynamicRendering = false;
    
    // GLFW PARAMETERS
    std::string windowTitle = "";
    uint32_t windowWidth = 0;
    uint32_t windowHeight = 0;
    bool windowResizable = true;
    bool windowStickKeys = false;
    bool windowStartCursorDisabled = false;

    // GRAPHIC PIPELINE PARAMETERS
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t framesOnFlight = 3;



};

namespace CTX{
    
    void initContext(vk_ctx&, const vk_instance_params&);
    void destroyContext(vk_ctx&, const vk_instance_params&);
    
    void createInstance(vk_ctx& , const vk_instance_params&);
    void pickPhysicalDevice(vk_ctx&, const vk_instance_params&);
    void createSurface(vk_ctx&);
    void createWindow(vk_ctx&, const vk_instance_params&);
    void createLogicalDevice(vk_ctx&, const vk_instance_params&);
    void createSwapchain(vk_ctx&, const vk_instance_params&);
    void createSwapchainImageViews(vk_ctx&);
    void createColorResources(vk_ctx&, const vk_instance_params&);
    void createDepthResources(vk_ctx&, const vk_instance_params&);
    void createCommandPool(vk_ctx&);
    void createCommandBuffers(vk_ctx&, const vk_instance_params&);
    void createMemoryAllocator(vk_ctx&);
    void createGlobalDescriptorLayouts(vk_ctx&);
    void createGlobalDescriptorPool(vk_ctx&);
    void allocateGlobalDescriptorSets(vk_ctx&);

    void createCameraResources(vk_ctx&);
    void recreateSwapchain(vk_ctx&, const vk_instance_params&);
    void createGlobalBuffers(vk_ctx&);

    float getDeltaTime();


    void destroyBuffer(vk_ctx& ctx, VkBuffer buffer, VmaAllocation allocation);
    void checkExpiredAllocations(vk_ctx& ctx);

    namespace AUX{

        VmaAllocationInfo createBuffer(vk_ctx& context, VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& allocation);
        VmaAllocationInfo createImage(vk_ctx& ctx, uint32_t width, uint32_t height, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage &image,  VmaAllocation& allocation);

        void enlargeBuffer(vk_ctx& ctx, VkDeviceSize size, int memoryType, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& allocation);
        
        void uploadData(vk_ctx& ctx, void* data, VkBuffer dstBuffer, size_t size, uint64_t dstOffset);
        void uploadDataDeviceBuffer(vk_ctx& ctx, void* data, VkDeviceAddress& deviceAddress, VmaAllocation& allocation, VkBuffer& dstBuffer, size_t size, uint64_t dstOffset);
        
        void copyBuffer(vk_ctx& ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset);
        void copyBuffer(vk_ctx& ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, std::vector<VkBufferCopy>& cmds);
        
        void copyBufferToImage(vk_ctx& ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        
        void transitionImageLayout(vk_ctx& ctx ,VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);



        uint32_t getGraphicsQueueIndex(VkPhysicalDevice physicalDevice);
        SwapChainSupportDetails querySwapChainSupport(const vk_ctx& context);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

	    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, const vk_ctx& context);

        
    }
}


const std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME, // For presentation on OS swapchain
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, // For bypassing renderpass
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME ,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };

	const std::vector<const char *> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
        
    };