#ifndef COMMON_SHADOW_GLSL
#define COMMON_SHADOW_GLSL

#include "common_sampling.glsl"

// World space position of the fragment in light's view space
vec3 Pos_toLightSpaceUVD(mat4 lightViewProjMatrix, vec3 worldPos) {
    vec4 lightSpacePos = lightViewProjMatrix * vec4(worldPos, 1.0);
    vec3 lightSpaceUVD = (lightSpacePos.xyz / lightSpacePos.w + 1.0) / 2.0;
    lightSpaceUVD.z = clamp(lightSpaceUVD.z, 0.0, 1.0);
    return lightSpaceUVD;
}

// Basic shadow mapping
float SM(sampler2D shadowMap, vec2 uv, float depth, float bias) {
    float refDepth = texture(shadowMap, uv).x; // reference depth value
    return (depth - bias < refDepth) ? 1.0 : 0.0; // visibility
}

// Cascaded shadow mapping
float CSM(sampler2DArray cascadedShadowMap, vec3 uvd, float depth, float bias) {
    float refDepth = texture(cascadedShadowMap, uvd).x; // reference depth value
    return (depth - bias < refDepth) ? 1.0 : 0.0; // visibility
}

// Percentage closer filtering
float PCF(sampler2D shadowMap,vec2 uv, float depth, float range, float bias) {
    const int numSamples = 32;
    const int numRings = 4;

    float visibility = 0.0;
    for (int i = 0; i < numSamples; i++) {
        vec2 x = PoissonSample(i, uv, numSamples, numRings);
        vec2 offset = x * range;
        visibility += SM(shadowMap, uv + offset, depth, bias);
    }

    return visibility / numSamples;
}

// Percentage closer soft shadow
float PCCS(sampler2D shadowMap,vec2 uv, float depth, float range, float bias) {
    const int numSamples = 32;
    const int numRings = 4;
    
    // average depth of blockers
    float avgRefDepth = 0.0;
    for (int i = 0; i < numSamples; i++) {
        vec2 x = PoissonSample(i, uv, numSamples, numRings);
        vec2 offset = x * range;
        avgRefDepth += texture(shadowMap, uv + offset).r;
    }
    avgRefDepth /= numSamples;

    // no shadow, just return 1.0
    if (depth - bias < avgRefDepth) {
        return 1.0;
    }

    const float lightSize = 0.02; // light size, control soft shadow range
    float penumbraSize = lightSize * (depth - avgRefDepth) / avgRefDepth; // penumbra size
    return PCF(shadowMap, uv, depth, penumbraSize, bias);
}

#endif