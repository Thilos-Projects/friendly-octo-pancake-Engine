#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragView;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in mat3 fragMat;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform pushConstanten{
	bool isPhong;
}pushConst;

struct Light{
	vec3 pos;
	vec3 color;
};

layout(set = 0, binding = 2) buffer LightBuffer{
	uint count;
	Light[] lights;
} lightBuffer;

void main(){
	
	vec3 texColor = {1.0f,0.0f,0.0f};
	vec3 texNormal =  fragNormal;

	vec3 N = normalize(texNormal);
	vec3 V = normalize(fragView);


	if(pushConst.isPhong){
		vec3 A = texColor * 0.0;										//0.0 durch varriable ersetzen
		
		vec3 D = vec3(0);
		vec3 S = vec3(0);
		for(uint i = 0; i < lightBuffer.count; i++){
			vec3 L = normalize(fragMat * (lightBuffer.lights[i].pos - fragPos));
			D += (max(dot(N, L) , 0.0) * lightBuffer.lights[i].color * 1);
			vec3 R = reflect(-L, N);
			S += (pow(max(dot(R, V), 0.0), 4) * vec3(0.3f));				//1.35 und 16 durch varriable ersetzen
		}
		
		D = vec3(D.x / lightBuffer.count, D.y / lightBuffer.count, D.z / lightBuffer.count);
		S = vec3(S.x / lightBuffer.count, S.y / lightBuffer.count, S.z / lightBuffer.count);

		outColor = vec4(A + D + S, 1.0);
	}
	else
	{
		vec3 L = normalize(fragMat * (lightBuffer.lights[0].pos - fragPos));
		vec3 R = reflect(-L, N);
		if(pow(max(dot(R,V),0.0),5.0f) > 0.8 && false){
			outColor = vec4(1.0);
		}
		else if(dot(V, N) < 0.5){
			outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		else if(max(dot(N, L) , 0.0) >= 0.25){
			outColor = vec4(texColor, 1.0f);
		}else{
			outColor = vec4(texColor / 5.0f, 1.0f);
		}
	}

	//outColor = vec4(lightBuffer.lights[0].pos, 1.0f);

}