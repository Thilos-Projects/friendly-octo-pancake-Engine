#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragView;
layout(location = 4) in vec3 fragLight;								//mehrere Lichter Ermöglichen
layout(location = 5) in mat4 fragNormTransform;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;
layout(binding = 2) uniform sampler2D norms;

layout(push_constant) uniform pushConstanten{
	bool isPhong;
}pushConst;

void main(){

	//outColor = vec4(fragColor,1.0);
	//outColor = texture(tex,fragUV);
	
	vec3 texColor ={1.0f,0.0f,0.0f};// texture(tex,fragUV).xyz;
	vec3 texNormal = fragNormal;// texture(norms,fragUV).xyz;

	vec3 N = normalize(texNormal);
	vec3 L = normalize(fragLight);									//pro Licht
	vec3 V = normalize(fragView);

	vec3 R = reflect(-L, N);										//pro Licht

	if(pushConst.isPhong){
		vec3 A = texColor * 0.0;										//0.1 durch varriable ersetzen
		vec3 D = max(dot(N, L) , 0.0) * texColor * 1;					//1 durch varriable ersetzen	//pro Licht
		vec3 S = pow(max(dot(R, V), 0.0), 4) * vec3(0.3f);				//1.35 und 16 durch varriable ersetzen	//pro Licht

		outColor = vec4(A + D + S, 1.0);
	}
	else
	{
		if(pow(max(dot(R,V),0.0),5.0f) > 0.8 && false){
			outColor = vec4(1.0);
		}
		else if(dot(V, N) < 0.5){
			outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		else if(max(dot(N, L) , 0.0) >= 0.25){
			outColor = vec4(texColor, 1.0f);
		}
		else
			outColor = vec4(texColor / 5.0f, 1.0f);
	}
}