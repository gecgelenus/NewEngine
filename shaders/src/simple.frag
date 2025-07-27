#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 outNormal;

layout(location = 3) flat in int outMaterialIndex;
layout(location = 4) flat in int textureEnabled;
layout(location = 5) flat in int baseColorFactorEnabled;
layout(location = 6) flat in int textureIndex;
layout(location = 7) flat in vec4 baseColorFactor;
layout(location = 8) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;


layout(set = 2, binding = 0) uniform sampler2D texImage[64];

void main(){
    vec4 materialColor;

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

    // --- Hardcoded Light Properties (Phong Model) ---
    vec3 lightPos = vec3(5.0, 5.0, 5.0); // World space light position
    vec3 lightColor = vec3(1.0, 1.0, 1.0); // White light
    float lightPower = 50.0; // Adjust for brightness

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
    vec3 finalLightedColor = ambient + (diffuse + specular) * attenuation;

    // Multiply the material's albedo by the calculated light.
    // Ensure final color has alpha from materialColor.
    outColor = vec4(finalLightedColor, materialColor.a);
}