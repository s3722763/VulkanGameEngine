#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;

layout (location = 0) out vec2 outUV;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 view;
	mat4 proj;
	mat4 viewProj;
} cameraData;

layout(push_constant) uniform constants {
	vec4 data;
	mat4 renderMatrix;
} PushConstants;

void main() {
	mat4 transformationMatrix = cameraData.viewProj *  PushConstants.renderMatrix;
	
	gl_Position = transformationMatrix * vec4(vPosition, 1.0f);
	outUV = vUV;
}