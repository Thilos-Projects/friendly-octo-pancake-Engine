#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragView;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out mat3 fragMat;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(binding = 0) uniform UBO
{
	mat4 V;
	mat4 P;
} ubo;

layout(set = 0, binding = 1) buffer InstanceBuffer{
	mat4[] instances;
} instanceBuffer;

void main(){

	mat4 fragNormTransform = instanceBuffer.instances[gl_InstanceIndex];
	gl_Position = ubo.P * ubo.V * fragNormTransform * vec4(inPos,1.0);
	vec4 worldPos = fragNormTransform * vec4(inPos,1.0);

	fragNormal = mat3(ubo.V) * mat3(fragNormTransform) * inNormal;
	fragView = -(ubo.V * worldPos).xyz;
	fragPos = worldPos.xyz;
	fragMat = mat3(ubo.V);
}