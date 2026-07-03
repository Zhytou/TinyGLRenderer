#ifndef COMMON_RAYMARCHING_GLSL
#define COMMON_RAYMARCHING_GLSL

// March a ray through the scene to find the first hit object.
// Args:
//     origin: the origin of the ray march in world space
//     direction: the direction of the ray march in world space
//     uViewMatrix: the view matrix
//     uProjMatrix: the projection matrix
//     tDepthMap: the depth map texture
//     near: the near plane
//     far: the far plane
//     camera: the camera projection type, 0: perspective, 1: orthographic
//     face: the ray march to front face, 0: back face, 1: front face
// Returns:
//     the .xy represents the UV of the first hit object, .z represents the distance to the hit object, .w represents if hit or not
vec4 RayMarch(vec3 origin, vec3 direction, mat4 uViewMatrix, mat4 uProjMatrix, sampler2D tDepthMap, float near, float far, float camera, float face) {
    const int maxSteps = 150;   // max num of steps to march
    const float stride = 0.12; // length of each step
    const float bias = 0.005;

    float dis = 0.0;
    vec3 pos = (uViewMatrix * vec4(origin, 1.0)).xyz; // origin in view space
    vec3 dir = normalize(vec3(uViewMatrix) * direction); // direction in view space
    pos = pos + 0.00001 * dir; // start from a small offset to avoid self-intersection

    for (int i = 0; i <= maxSteps; i++) {
        dis += stride;
        pos += dir * stride;

        vec4 ndcPos = uProjMatrix * vec4(pos, 1.0);
        ndcPos.xyz /= ndcPos.w;
        vec2 uv = ndcPos.xy * 0.5 + 0.5;

        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            break; // out of screen
        }

        float z = -pos.z;
        float refz = 2 * texture(tDepthMap, uv).x - 1.0;
        refz = camera == 1.0 ? 0.5 * (near + far) - 0.5 * refz * (far - near) : -2.0 * far * near / (refz * (far - near) - (far + near));
        refz = -refz; // view space z is negative

        if (face == 0.0) {
            if (z + bias >= refz) {
                return vec4(uv, dis, 1.0);
            }
        } 
        else {
            if (z - bias <= refz) {
                return vec4(uv, dis, 1.0);
            }
        }
    }

    return vec4(0.0);
}

#endif