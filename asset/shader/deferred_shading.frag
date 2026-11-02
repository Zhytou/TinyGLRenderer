#version 450

#include "common_brdf.glsl"
#include "common_normal.glsl"
#include "common_shadow.glsl"
#include "common_screenspace.glsl"

layout(location = 0) in vec2 iFragUV;

// ubo block
layout(std140, binding = 0) uniform CameraBlock {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    mat4 uInvViewMatrix;
    mat4 uInvProjMatrix;
    vec3 uCameraPos;
    float uCameraType;
    float uFov;
    float uNear;
    float uFar;
    float uAspect;
};
// ubo
layout(std140, binding = 2) uniform MainLightBlock {
    mat4 uViewProjMatrices[4];
    vec4 uSplits; // cascaded shadow map splits in z axis of view space, x split[0], y split[1], z split[2], w split[3]
    vec4 uRadiuses; // the radius of each level of cascaded shadow map, used for dynamic bias
};

// ssbo array
struct Light {
    mat4 viewProjMatrix; 
    vec4 colorIntensity;
    vec4 vectorType; // use .w to distinguish between directional and point light
    vec4 uvOffsetScale;
    vec4 sceneBounds; // .x height .y width .zw reserved
};
layout(std430, binding = 0) buffer LightBuffer {
    Light uLights[];
};
uniform int uLightCount;

layout(binding = 8) uniform sampler2D tAlbedoMap;
layout(binding = 9) uniform sampler2D tNormalMap;
layout(binding = 10) uniform sampler2D tMRAOMap;
layout(binding = 11) uniform sampler2D tDepthMap; // GL_DEPTH_COMPONENT24, .x is the depth value
layout(binding = 14) uniform samplerCube tIBLDiffuseMap;
layout(binding = 15) uniform samplerCube tIBLSpecularMap;
layout(binding = 26) uniform samplerCube tIBLBRDFLUTMap;
layout(binding = 18) uniform sampler2DArray tCascadedShadowMap; // cascaded shadow map is TEXTURE_2D_ARRAY with 4 layers
layout(binding = 19) uniform sampler2D tAtlasShadowMap; // GL_DEPTH_COMPONENT24, .x is the depth value

layout(location = 0) out vec4 oFragColor;

void main() {    
    vec3 albedo = texture(tAlbedoMap, iFragUV).rgb;
    vec3 mrao   = texture(tMRAOMap, iFragUV).rgb;
    float depth = texture(tDepthMap, iFragUV).x;

    float metallic = mrao.r;
    float roughness = mrao.g;
    float ao        = mrao.b;

    vec3 viewPos  = Pos_toView(iFragUV, depth, uInvProjMatrix);
    vec3 worldPos = Pos_toWorld(iFragUV, depth, uInvViewMatrix, uInvProjMatrix);

    vec3 V = normalize(uCameraPos - worldPos); // frag -> camera
    vec3 N = N_decode(texture(tNormalMap, iFragUV).xyz);
    vec3 F0 =  mix(vec3(0.04), albedo, metallic);
    
    // ----------------------------------------------------------------
    // Evaluate direct light color
    // ----------------------------------------------------------------
    vec3 dLightColor = vec3(0.0);
    for (int i = 0; i < uLightCount; i++) {
        vec3 L = uLights[i].vectorType.w == 0.0 ? normalize(-uLights[i].vectorType.xyz) : normalize(uLights[i].vectorType.xyz - worldPos); // frag -> light
        vec3 color = BRDF(L, V, N, F0, albedo, metallic, roughness) * uLights[i].colorIntensity.rgb * uLights[i].colorIntensity.w;

        float cosTheta = clamp(dot(N, L), 0.0, 1.0);
        float slope = sqrt(1.0 - cosTheta * cosTheta) / (cosTheta + 0.001); // tan(theta)

        float visibility = 1.0;
        if (i == 0) {
            int layer = 3; // the cascaded shadow map layer, default is the farthest layer
            for (int slice = 0; slice < 4; ++slice) {
                if (viewPos.z > uSplits[slice]) { // z is negative, e.g. depth = -5.0 > -10.0, in Level 0
                    layer = slice;
                    break;
                }
            }
            vec3 lightSpaceUVD = Pos_toLightSpaceUVD(uViewProjMatrices[layer], worldPos); 
            vec3 cascadedUVD = vec3(lightSpaceUVD.xy, float(layer));

            ivec2 size = textureSize(tCascadedShadowMap, 0).xy;
            float texelSize = 2 * uRadiuses[layer] / max(size.x, size.y); 
            float bias = max(texelSize * slope * 0.5, texelSize * 0.1);

            visibility = CSM(tCascadedShadowMap, cascadedUVD, lightSpaceUVD.z, bias);            
        } else {
            vec3 lightSpaceUVD = Pos_toLightSpaceUVD(uLights[i].viewProjMatrix, worldPos); // (u, v, depth)
            vec2 atlasUV = uLights[i].uvOffsetScale.xy + lightSpaceUVD.xy * uLights[i].uvOffsetScale.zw;
            
            vec2 size = vec2(textureSize(tCascadedShadowMap, 0).xy) * uLights[i].uvOffsetScale.zw;
            float texelSize = max(uLights[i].sceneBounds.x / size.x, uLights[i].sceneBounds.y / size.y);
            float bias = max(texelSize * slope * 0.5, texelSize * 0.1);

            visibility = SM(tAtlasShadowMap, atlasUV, lightSpaceUVD.z, bias);
        }

        dLightColor += color * visibility;
    }    

    // ----------------------------------------------------------------
    // Evaluate indirect light color
    // ----------------------------------------------------------------
    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    vec3 kS = F_Schlick(NdotV, F0);
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    vec3 irradiance = texture(tIBLDiffuseMap, N).rgb;
    vec3 indLightColor = kD * albedo * irradiance;

    oFragColor = vec4(dLightColor + indLightColor, 1.0);
}
