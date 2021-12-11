#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outFragColour;

layout (set = 1, binding = 0) uniform sampler2D tex1;

void main() {
	vec3 color = texture(tex1, inTexCoord).xyz;
	outFragColour = vec4(color, 1.0f);
}