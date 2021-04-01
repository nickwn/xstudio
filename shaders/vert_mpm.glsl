#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform UniformBufferObject{
    mat4 model;
} m;

void main() {
    gl_Position = m.model * vec4(inPosition, 1.0);
    gl_PointSize = 3.f;
}