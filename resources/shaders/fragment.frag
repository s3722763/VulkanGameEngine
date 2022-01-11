#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outFragColour;

layout (set = 1, binding = 0) uniform sampler2D diffuse;
//layout (set = 1, binding = 1) uniform sampler2D specular;

void main() {
	vec3 color = texture(diffuse, inTexCoord).xyz;

	vec3 lightDir = vec3(0.0, 0.0, 1.0);
	float diff = max(dot(inNormal, -lightDir), 0.0);
	color = color * diff;

	outFragColour = vec4(color, 1.0f);
}