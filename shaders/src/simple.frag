#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) flat in int outMaterialIndex;


layout(location = 0) out vec4 outColor;


layout(set = 2, binding = 0) uniform sampler2D texImage[64];

void main(){
    outColor = texture(texImage[outMaterialIndex], inTexCoord);
}