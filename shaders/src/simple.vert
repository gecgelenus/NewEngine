#version 460
#extension GL_EXT_buffer_reference : require

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 VPmatrix;
} camera;

layout(buffer_reference, std430, buffer_reference_align = 16) buffer ModelBuffer {
    mat4 modelList[]; 

};

layout(set = 1, binding = 0) uniform ModelMatrixUBO {
    mat4 model; 
} models[64];

layout(push_constant) uniform PushConstants {
    ModelBuffer modelBufferAddress;
} pc;


layout(location = 0) in vec3 pos;
//layout(location = 1) in vec4 color;
layout(location = 1) in vec2 UV;


layout(location = 3) in int modelIndex;
layout(location = 4) in int materialIndex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 inTexCoord;
layout(location = 2) flat out int outMaterialIndex;



void main() {
    ModelBuffer modelBuffer = ModelBuffer(pc.modelBufferAddress);

    gl_Position = camera.VPmatrix * modelBuffer.modelList[modelIndex] * vec4(pos, 1.0);
    fragColor = vec4(1.0f);
    inTexCoord = UV;
    outMaterialIndex = materialIndex;
}