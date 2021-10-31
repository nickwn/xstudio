#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

void main() {
    gl_Position = vec4(inPosition, 1.0); //mvp.proj * mvp.view * mvp.model * vec4(inPosition, 1.0);
    gl_PointSize = 2.f;
}