#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;

layout(binding = 0) uniform UBO0
{
	mat4 MVP;
} ubo0;

void main(){
	gl_Position = ubo0.MVP * vec4(pos, 1.0);
	fragColor = color;
	fragUV = uv;
}