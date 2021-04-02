#version 430


layout (binding = 0) uniform sampler2DArray fbcol;
layout (binding = 1) uniform sampler2DArray normcol;
layout (binding = 2) uniform sampler2DArray prevDepth;
layout (binding = 3) uniform sampler2D occlusion;
layout (binding = 4) uniform sampler2D occlusionBlur;
layout (binding = 5) uniform sampler2DArray fbpos;


in vec2 tex;
out vec4 col;

void main()
{	

		float val = texture(occlusionBlur, tex).r;
		col = vec4(val, val, val, 1.0f);
	
}



//else if (gl_FragCoord.x > 640 && gl_FragCoord.y < 360)
	//{
	//	float r = texture(prevDepth, vec3(tex, 0)).r;
	//	r = 2.0f * r - 1.0f;
	//	float n = 0.1f;
	//	float f = 1000.0f;
	//	float c = (2.0f * n) / (f + n - r * (f - n));
	//	col = vec4(c, c, c, 1.0);
	//}

	/*--------------visualize depth-----------------------------------------*/
	//float val = texture(occlusionBlur, tex).r;	
	//col = vec4(val, val, val, 1.0f);
	//if(gl_FragCoord.x < 640)
	//{
	//	col = texture(fbcol, vec3(tex, 0));		
	//}
	//else if (gl_FragCoord.x > 640)
	//{
	//	//visualize separation
	//	//col = texture(fbcol, vec3(tex, 1));
	//
	//	//visualize normals
	//	//col = texture(normcol, vec3(tex, 1));
	//
	//	//visualize depthbuffer
	//	float r = texture(prevDepth, vec3(tex, 0)).r;
	//	r = 2.0f * r - 1.0f;
	//	float n = 0.1f;
	//	float f = 1000.0f;
	//	float c = (2.0f * n) / (f + n - r * (f - n));
	//	col = vec4(c, c, c, 1.0);
	//}
