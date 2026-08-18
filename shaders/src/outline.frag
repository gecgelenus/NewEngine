#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 outNormal;

layout(location = 3) flat in int outMaterialIndex;
layout(location = 4) flat in int textureEnabled;
layout(location = 5) flat in int baseColorFactorEnabled;
layout(location = 6) flat in int textureIndex;
layout(location = 7) flat in vec4 baseColorFactor;
layout(location = 8) in vec3 fragWorldPos;
layout(location = 9) in vec4 fragLightPos;
layout(location = 10) flat in int outModelIndex;
layout(location = 11) flat in int outSelectedIndex;


layout(location = 0) out vec4 outColor;



struct LightEntry{
    mat4 matrix;
    vec4 pos;
    vec4 color;
};


layout(buffer_reference, std430) buffer LightBuffer {
    LightEntry lights[]; 

};

layout(set = 1, binding = 0, std430) readonly buffer AddressTable {
    uint64_t addresses[];
} addressTable;

layout(set = 2, binding = 0) uniform sampler2D texImage[64];
layout(set = 3, binding = 0) uniform sampler2D lightSampler;





void main(){

    LightBuffer lightBuffer = LightBuffer(addressTable.addresses[2]);

    vec4 tmpColor = vec4(0, 0, 0, 0);
    

    if(outModelIndex == outSelectedIndex && outModelIndex != 0){
        tmpColor = vec4(1, 0, 0, 1);
    }
    outColor = tmpColor;
}