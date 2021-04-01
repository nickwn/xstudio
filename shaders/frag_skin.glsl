#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

void main() 
{
    const vec3 blueLightColor = vec3(.76f, .91f, 1.f);
    const vec3 blueLightDir = vec3(0.f, -1.f, 1.f);
    const vec3 yellowLightColor = vec3(1.f, .95f, .83f);
    const vec3 yellowLightDir = vec3(0.f, -1.f, -1.f);
    
    const vec3 objColor = vec3(1.f, 1.f, 1.f);

    vec3 color = vec3(0.2f);
    color += objColor * max(0.f, dot(inNormal, -blueLightDir)) * blueLightColor;
    color += objColor * max(0.f, dot(inNormal, -yellowLightDir)) * yellowLightColor;

    // shitty dynamic range
    color = smoothstep(0.f, 1.f, color * .7f);

    outColor = vec4(color, 1.0);
}