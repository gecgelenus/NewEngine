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

layout(location = 0) out vec4 outColor;
layout(location = 1) out uint outID;


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


float ShadowCalculation(vec4 fragPosLightSpace, vec3 fraglightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(lightSampler, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.0025 * (1.0 -  dot(normalize(outNormal), fraglightDir)), 0.0005);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}



void main(){
    vec4 materialColor;

    LightBuffer lightBuffer = LightBuffer(addressTable.addresses[2]);

    // Determine the base material color (albedo) from your existing logic
    if(baseColorFactorEnabled == 1 && textureEnabled == 1){
        materialColor = texture(texImage[textureIndex], inTexCoord) * baseColorFactor;
    }else if(baseColorFactorEnabled == 0 && textureEnabled == 1){
        materialColor = texture(texImage[textureIndex], inTexCoord);
    }else if(baseColorFactorEnabled == 1 && textureEnabled == 0){
        materialColor = baseColorFactor;
    }else{
        materialColor = vec4(1.0, 1.0, 1.0, 1.0); // Default to white
    }

    vec3 lightPos = lightBuffer.lights[0].pos.xyz;
    vec3 lightColor = lightBuffer.lights[0].color.xyz;
    float lightPower = lightBuffer.lights[0].color.w;

    vec3 ambientColor = vec3(0.1, 0.1, 0.1); // Small constant ambient light

    // Material properties (can be adjusted for different looks)
    float shininess = 32.0; // For specular highlight sharpness

    // --- Calculate Lighting Components ---
    vec3 norm = normalize(outNormal); // Ensure normal is normalized

    // Light direction (from fragment to light)
    vec3 lightDir = normalize(lightPos - fragWorldPos);

    // View direction (from fragment to camera/viewer).
    // For simplicity, assume camera is at (0,0,0) or sufficiently far away for orthographic.
    // For proper perspective, you'd pass camera position as a uniform.
    vec3 viewPos = vec3(0.0, 0.0, 0.0); // Example: Camera at origin (adjust as needed)
    vec3 viewDir = normalize(viewPos - fragWorldPos);

    // Ambient component
    vec3 ambient = ambientColor * materialColor.rgb;

    // Diffuse component
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff * materialColor.rgb;

    // Specular component
    vec3 reflectDir = normalize(reflect(-lightDir, norm)); // Reflected light direction
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = lightColor * spec; // Specular color typically just light color

    // Attenuation based on distance to light (optional but physically more correct)
    float distance = length(lightPos - fragWorldPos);
    float attenuation = lightPower / (distance * distance); // Inverse square law

    // Combine all components
    float shadow = ShadowCalculation(fragLightPos, lightDir); 
    vec3 finalLightedColor = ambient + (1.0 - shadow) * (diffuse + specular) * attenuation;

    // Multiply the material's albedo by the calculated light.
    // Ensure final color has alpha from materialColor.
    outColor = vec4(finalLightedColor, materialColor.a);
    outID = uint(outModelIndex);
}