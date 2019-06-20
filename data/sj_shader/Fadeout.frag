#version 120
#extension GL_ARB_texture_rectangle : enable
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2DRect texture_0;
uniform float FadeVal;

void main(){
	/********************
	********************/
	vec2 tex_pos = gl_TexCoord[0].xy;
	
	vec4 color = texture2DRect(texture_0, tex_pos);
	color.rgb += vec3(FadeVal, FadeVal, FadeVal);
	color.rgb = vec3(min(color.r, 1.0), min(color.g, 1.0), min(color.b, 1.0));
	
	/********************
	********************/
	gl_FragColor = color;
}
