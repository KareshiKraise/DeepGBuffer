#version 430
layout (location = 0) out vec3 pos_tex;
layout (location = 1) out vec3 normal_tex;
layout (location = 2) out vec3 col_tex;

in vec3 Pos;
in vec2 Tex;
in vec3 Normal;
in vec4 ClipSpace;
in int gl_Layer;

layout (binding = 0) uniform sampler2D texture_diffuse1;
layout (binding = 1) uniform sampler2DArray prevDepth;

const float MINIMUM_SEPARATION = 1.0f;

uniform float near;
uniform float far;
uniform mat4 Proj;

void main()
{
	if(gl_Layer == 0) {
		pos_tex = Pos;
		normal_tex = normalize(Normal);
		col_tex.rgb = texture(texture_diffuse1, Tex).rgb;		
	}

	else if(gl_Layer == 1)
	{
		vec2 ndc = (ClipSpace.xy/ClipSpace.w) * 0.5f + 0.5f;	
	
		//minimum separation is in linear world space
		//first linearize z from prev layer depth buffer
	
		float prevLayerZ = texture(prevDepth, vec3(ndc, 0)).r;
		float prevLayerZLinear = 2.0f * prevLayerZ - 1.0f;
		prevLayerZLinear = 2.0f * near * far / (far + near - prevLayerZLinear * (far - near));
		
		//add minimum separation
		prevLayerZLinear += MINIMUM_SEPARATION;
	
		//convert back to nonlinear depth space
		float compareDepth = (far + near - 2.0f * near * far / prevLayerZLinear) / (far - near);
		compareDepth = (compareDepth + 1.0f) / 2.0f;						
	
		if(gl_FragCoord.z > compareDepth) 
		{
			pos_tex = Pos;
			normal_tex = normalize(Normal);
			col_tex.rgb = texture(texture_diffuse1, Tex).rgb;			
		}
		else
		{
			discard;
		}
	}
}  