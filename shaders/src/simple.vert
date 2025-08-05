#version 460
#extension GL_EXT_buffer_reference : require

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 VPmatrix;
} camera;

struct Material{
    int textureEnabled;
    int baseColorFactorEnabled;

    int textureIndex;
    int padding;
    vec4 baseColorFactor;
};

layout(buffer_reference, std430) buffer ModelBuffer {
    mat4 modelList[]; 

};

layout(buffer_reference, std430) buffer MaterialBuffer {
    Material materials[];

};

layout(set = 1, binding = 0) uniform ModelMatrixUBO {
    mat4 model; 
} models[64];

layout(push_constant) uniform PushConstants {
    ModelBuffer modelBufferAddress;
    MaterialBuffer MaterialBufferAddress;
    
} pc;


layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;

//layout(location = 3) in vec4 color;


layout(location = 5) in int modelIndex;
layout(location = 6) in int materialIndex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 inTexCoord;
layout(location = 2) out vec3 outNormal;

layout(location = 3) flat out int outMaterialIndex;
layout(location = 4) flat out int textureEnabled;
layout(location = 5) flat out int baseColorFactorEnabled;
layout(location = 6) flat out int textureIndex;
layout(location = 7) flat out vec4 baseColorFactor;
layout(location = 8) out vec3 fragWorldPos;






void main() {
    ModelBuffer modelBuffer = ModelBuffer(pc.modelBufferAddress);
    MaterialBuffer materialBuffer = MaterialBuffer(pc.MaterialBufferAddress);

    gl_Position = camera.VPmatrix * modelBuffer.modelList[modelIndex] * vec4(pos, 1.0);
    //fragColor = color;
    inTexCoord = UV;
    outMaterialIndex = materialIndex;

    textureEnabled = materialBuffer.materials[materialIndex].textureEnabled;
    baseColorFactorEnabled = materialBuffer.materials[materialIndex].baseColorFactorEnabled;
    baseColorFactor = materialBuffer.materials[materialIndex].baseColorFactor;
    textureIndex = materialBuffer.materials[materialIndex].textureIndex;

    outNormal = mat3(transpose(inverse(modelBuffer.modelList[modelIndex]))) * normal;
    fragWorldPos = (modelBuffer.modelList[modelIndex] * vec4(pos, 1.0)).xyz;
}