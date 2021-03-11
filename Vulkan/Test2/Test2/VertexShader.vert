#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragView;
layout(location = 4) out vec3 fragLight;
layout(location = 5) out mat4 fragNormTransform;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec3 inNormal;

layout(binding = 0) uniform UBO
{
	mat4 V;
	mat4 P;
	vec3 Light;						//mehrere Ermöglichen
} ubo;

layout(set = 0, binding = 1) buffer InstanceBuffer{
	mat4[] instances;
} instanceBuffer;

void main(){

	fragNormTransform = instanceBuffer.instances[gl_InstanceIndex];
	gl_Position = ubo.P * ubo.V * fragNormTransform * vec4(inPos,1.0);
	vec4 worldPos = fragNormTransform * vec4(inPos,1.0);

	fragColor = inColor;
	fragUV = inUv;

	fragNormal = mat3(ubo.V) * mat3(fragNormTransform) * inNormal;
	fragView = -(ubo.V * worldPos).xyz;
	fragLight =  mat3(ubo.V) * (ubo.Light - worldPos.xyz);
}