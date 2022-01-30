#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 worldPos;

layout (set = 1, binding = 0) uniform sampler2D diffuse;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main() {
	outPosition = vec4(worldPos, 1.0);
	outNormal = vec4(normal, 1.0);
	outAlbedo = texture(diffuse, texCoord);
	//outAlbedo = vec4(1.0, 1.0, 1.0, 1.0);
}