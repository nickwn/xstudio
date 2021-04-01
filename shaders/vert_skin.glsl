#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uvec2 inWeights;

layout(location = 0) out vec3 outNormal;

layout(binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

layout(std140, binding = 1) readonly restrict buffer BoneTransforms{
    mat4 data[];
} bones;

// TODO: numWeights/totalWeight isn't really needed, just hard-code 4 blends like a normal person
void main() {

    float totalWeight = 0.f;
    vec4 deformedPosition = vec4(0.f, 0.f, 0.f, 1.f);
    vec4 deformedNormal = vec4(0.f);
    uint numWeights = inWeights.y >> 24;
    for (uint i = 0; i < min(numWeights, 3); i++)
    {
        const uint idx = i / 2u;
        const uint subIdx = i % 2u;
        const uint boneShift = subIdx * 16;
        const uint weightShift = subIdx * 16 + 8;

        const uint boneIdx = (inWeights[idx] >> boneShift) & 0xFF;
        const mat4 boneMat = bones.data[boneIdx];

        const uint uweight = (inWeights[idx] >> weightShift) & 0xFF;
        const float weight = float(uweight) / 255.f;
        totalWeight += weight;
        
        deformedPosition += weight * boneMat * vec4(inPosition, 1.f);
        deformedNormal += weight * boneMat * vec4(inNormal, 0.f);
    }

    if (numWeights == 4)
    {
        const float weight = 1.f - totalWeight;
        const uint boneIdx = (inWeights[1] >> 16) & 0xFF;
        const mat4 boneMat = bones.data[boneIdx];

        deformedPosition += weight * boneMat * vec4(inPosition, 1.f);
        deformedNormal += weight * boneMat * vec4(inNormal, 0.f);
    }

    outNormal = normalize(deformedNormal.xyz);

    gl_Position = mvp.proj * mvp.view * mvp.model * deformedPosition;
}