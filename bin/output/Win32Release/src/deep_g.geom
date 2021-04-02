#version 430

layout(triangles) in;
layout(triangle_strip , max_vertices = 6) out;

in vec3 pos[];
in vec2 tex[];
in vec3 normal[];
in vec4 clipspace[];

out vec3 Pos;
out vec2 Tex;
out vec3 Normal;
out vec4 ClipSpace;
out int gl_Layer;

void main()
{
	for(int i = 0; i < 2 ; i++)
	{		
		gl_Layer = i;
		for(int j = 0; j < 3; j++)
		{	
			ClipSpace = clipspace[j];
			Pos = pos[j];
			Tex = tex[j];
			Normal = normal[j];
			gl_Position = gl_in[j].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}	
}

