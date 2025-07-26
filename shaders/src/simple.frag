#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) flat in int outMaterialIndex;
layout(location = 3) flat in int textureEnabled;
layout(location = 4) flat in int baseColorFactorEnabled;
layout(location = 5) flat in int textureIndex;
layout(location = 6) flat in vec4 baseColorFactor;

layout(location = 0) out vec4 outColor;


layout(set = 2, binding = 0) uniform sampler2D texImage[64];

void main(){

    if(baseColorFactorEnabled == 1 && textureEnabled == 1){
        outColor = texture(texImage[textureIndex], inTexCoord) * baseColorFactor;
    }else if(baseColorFactorEnabled == 0 && textureEnabled == 1){
        outColor = texture(texImage[textureIndex], inTexCoord);
    }else if(baseColorFactorEnabled == 1 && textureEnabled == 0){
        outColor = baseColorFactor;
    }else{
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
    }

}