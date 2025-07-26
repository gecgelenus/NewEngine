#pragma once
#include "util.hpp"
#include "spirv_reflect.h"



#include <vulkan_context.hpp>
#include <iostream>


#define VERTEX_TYPE_3F 1
#define VERTEX_TYPE_2F 2





class Object{
    public:
        
        Object(vk_ctx& p_ctx);

        ~Object();

        void formatData(std::vector<SpvReflectInterfaceVariable*> inputVars, uint32_t strideSize);
        void serializePrimitive(std::vector<SpvReflectInterfaceVariable*> inputVars, uint32_t index, uint32_t strideSize);

        vk_ctx& ctx;

        std::string name;
        glm::vec2 position;
        glm::vec3 rotation;
        float scale;
        std::vector<float> vertexData;
        std::vector<int32_t> instanceData;
        uint32_t modelIndex;
        Object* parentObject;
        std::vector<Object*> childObjects;
        uint32_t objectID;

        glm::mat4 modelMatrix;
        glm::mat4 localMatrix;

    

        std::vector<ObjectPrimitive> primitives; 
        ObjectTransformation transformation;
        ObjectTransformation parentTransformation;
        uint64_t vertexIndex;
        std::vector<uint32_t> indexData;
        uint64_t indexIndex;
};
