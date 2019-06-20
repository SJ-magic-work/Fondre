#version 120
#extension GL_ARB_texture_rectangle : enable
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2DRect Mask_0;
uniform sampler2DRect Mask_1;
uniform sampler2DRect Mask_2;
uniform sampler2DRect Mask_3;
uniform sampler2DRect Mask_4;
uniform sampler2DRect Mask_5;
uniform sampler2DRect Mask_6;
uniform sampler2DRect Mask_7;

// uniform float val_min;

void main(){
	/********************
	********************/
	vec2 tex_pos = gl_TexCoord[0].xy;
	
	vec4 color_0 = texture2DRect(Mask_0, tex_pos);
	vec4 color_1 = texture2DRect(Mask_1, tex_pos);
	vec4 color_2 = texture2DRect(Mask_2, tex_pos);
	vec4 color_3 = texture2DRect(Mask_3, tex_pos);
	vec4 color_4 = texture2DRect(Mask_4, tex_pos);
	vec4 color_5 = texture2DRect(Mask_5, tex_pos);
	vec4 color_6 = texture2DRect(Mask_6, tex_pos);
	vec4 color_7 = texture2DRect(Mask_7, tex_pos);
	
	vec4 color = min(min(min(min(min(min(min(color_0, color_1), color_2), color_3), color_4), color_5), color_6), color_7);
	
	// color.rgb = max(vec3(val_min, val_min, val_min), color.rgb);
	
	/********************
	********************/
	gl_FragColor = color;
}
