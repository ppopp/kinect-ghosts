uniform int count;
uniform float depth_horizontal_pixel_stride;
uniform float depth_vertical_pixel_stride;
uniform sampler2D video_texture_00;
uniform sampler2D depth_texture_00;
uniform float depth_cutoff_00;
uniform sampler2D video_texture_01;
uniform sampler2D depth_texture_01;
uniform float depth_cutoff_01;
uniform sampler2D video_texture_02;
uniform sampler2D depth_texture_02;
uniform float depth_cutoff_02;
uniform sampler2D video_texture_03;
uniform sampler2D depth_texture_03;
uniform float depth_cutoff_03;
uniform sampler2D video_texture_04;
uniform sampler2D depth_texture_04;
uniform float depth_cutoff_04;
uniform sampler2D video_texture_05;
uniform sampler2D depth_texture_05;
uniform float depth_cutoff_05;
uniform sampler2D video_texture_06;
uniform sampler2D depth_texture_06;
uniform float depth_cutoff_06;
uniform sampler2D video_texture_07;
uniform sampler2D depth_texture_07;
uniform float depth_cutoff_07;

varying vec2 texcoord;

float smooth_alpha(in float input_depth, in float depth_cutoff) {
	float range = 0.01;
	float max_depth = depth_cutoff + range;
	if (input_depth == 0.0) {
		return 0.0;
	}
	if (input_depth < depth_cutoff) {
		return 1.0;
	}
	if (input_depth < max_depth) {
		return 1.0 - (input_depth - depth_cutoff) / range;
	}
	return 0.0;
}


float blurred_alpha_5x5_gaussian(sampler2D tex, vec2 center) {
    float alpha = 0.0;
    // Gaussian Kernel 5 x 5
    
    alpha += texture2D(tex, center + vec2(-2.0 * depth_horizontal_pixel_stride, -2.0 * depth_vertical_pixel_stride)).a * 0.003765;
    alpha += texture2D(tex, center + vec2(-2.0 * depth_horizontal_pixel_stride, -1.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(-2.0 * depth_horizontal_pixel_stride, 0.0 * depth_vertical_pixel_stride)).a * 0.023792;
    alpha += texture2D(tex, center + vec2(-2.0 * depth_horizontal_pixel_stride, 1.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(-2.0 * depth_horizontal_pixel_stride, 2.0 * depth_vertical_pixel_stride)).a * 0.003765;
    alpha += texture2D(tex, center + vec2(-1.0 * depth_horizontal_pixel_stride, -2.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(-1.0 * depth_horizontal_pixel_stride, -1.0 * depth_vertical_pixel_stride)).a * 0.059912;
    alpha += texture2D(tex, center + vec2(-1.0 * depth_horizontal_pixel_stride, 0.0 * depth_vertical_pixel_stride)).a * 0.094907;
    alpha += texture2D(tex, center + vec2(-1.0 * depth_horizontal_pixel_stride, 1.0 * depth_vertical_pixel_stride)).a * 0.059912;
    alpha += texture2D(tex, center + vec2(-1.0 * depth_horizontal_pixel_stride, 2.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(0.0 * depth_horizontal_pixel_stride, -2.0 * depth_vertical_pixel_stride)).a * 0.023792;
    alpha += texture2D(tex, center + vec2(0.0 * depth_horizontal_pixel_stride, -1.0 * depth_vertical_pixel_stride)).a * 0.094907;
    alpha += texture2D(tex, center + vec2(0.0 * depth_horizontal_pixel_stride, 0.0 * depth_vertical_pixel_stride)).a * 0.150342;
    alpha += texture2D(tex, center + vec2(0.0 * depth_horizontal_pixel_stride, 1.0 * depth_vertical_pixel_stride)).a * 0.094907;
    alpha += texture2D(tex, center + vec2(0.0 * depth_horizontal_pixel_stride, 2.0 * depth_vertical_pixel_stride)).a * 0.023792;
    alpha += texture2D(tex, center + vec2(1.0 * depth_horizontal_pixel_stride, -2.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(1.0 * depth_horizontal_pixel_stride, -1.0 * depth_vertical_pixel_stride)).a * 0.059912;
    alpha += texture2D(tex, center + vec2(1.0 * depth_horizontal_pixel_stride, 0.0 * depth_vertical_pixel_stride)).a * 0.094907;
    alpha += texture2D(tex, center + vec2(1.0 * depth_horizontal_pixel_stride, 1.0 * depth_vertical_pixel_stride)).a * 0.059912;
    alpha += texture2D(tex, center + vec2(1.0 * depth_horizontal_pixel_stride, 2.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(2.0 * depth_horizontal_pixel_stride, -2.0 * depth_vertical_pixel_stride)).a * 0.003765;
    alpha += texture2D(tex, center + vec2(2.0 * depth_horizontal_pixel_stride, -1.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(2.0 * depth_horizontal_pixel_stride, 0.0 * depth_vertical_pixel_stride)).a * 0.023792;
    alpha += texture2D(tex, center + vec2(2.0 * depth_horizontal_pixel_stride, 1.0 * depth_vertical_pixel_stride)).a * 0.015019;
    alpha += texture2D(tex, center + vec2(2.0 * depth_horizontal_pixel_stride, 2.0 * depth_vertical_pixel_stride)).a * 0.003765;
     			
    return alpha;
} 

float blurred_alpha_average(sampler2D tex, vec2 center, int size) {
	float alpha = texture2D(tex, center).a;
	int cntr = 0;
	if (alpha != 0.0) {
		cntr = 1;
	}
	float half_width = depth_horizontal_pixel_stride * float(size) / 2.0;
	float half_height = depth_vertical_pixel_stride * float(size) / 2.0;

	for (float x = -half_width; x < half_width; x += depth_horizontal_pixel_stride) {
		for (float y = -half_height; y < half_height; y += depth_vertical_pixel_stride) {
			float val = texture2D(tex, center + vec2(x, y)).a;
			if (val != 0.0) {
				alpha += val;
				cntr += 1;
			}
		}
	}
	if (cntr > 0) {
		return alpha / float(cntr);
	}
	return 1.0;
}

float add_sampler(
	in sampler2D video_texture, 
	in sampler2D depth_texture, 
	in float depth_cutoff, 
	in float best_depth) 
{
	vec4 new_color = vec4(0.0, 0.0, 0.0, 1.0);
	//float depth = texture2D(depth_texture, texcoord).a;
	//float depth = blurred_alpha_5x5_gaussian(depth_texture, texcoord);
	float depth = blurred_alpha_average(depth_texture, texcoord, 15);
	float alpha = smooth_alpha(depth, depth_cutoff);

	if (depth != 0.0) {
		if (depth < best_depth) {
			if (alpha > 0.0) {
				new_color = texture2D(video_texture, texcoord);
				new_color.a = alpha;
				if (alpha >= 1.0) {
					gl_FragColor = new_color;
				}
				else {
					gl_FragColor = (gl_FragColor + new_color);
				}
				return depth;
			}
		}
		else if (gl_FragColor.a < 1.0) {
			new_color = texture2D(video_texture, texcoord);
			new_color.a = alpha;
			gl_FragColor = (new_color + gl_FragColor);
		}
			
	}
	return best_depth;	
}

void main()
{
	// apparently there are issues if you loop through an array of samplers.
	// bummer
	gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
	float best_depth = 100000.0;

	if (count > 0) {
		best_depth = add_sampler(
			video_texture_00,
			depth_texture_00,
			depth_cutoff_00,
			best_depth);
	}
	if (count > 1) {
		best_depth = add_sampler(
			video_texture_01,
			depth_texture_01,
			depth_cutoff_01,
			best_depth);
	}
	if (count > 2) {
		best_depth = add_sampler(
			video_texture_02,
			depth_texture_02,
			depth_cutoff_02,
			best_depth);
	}
	if (count > 3) {
		best_depth = add_sampler(
			video_texture_03,
			depth_texture_03,
			depth_cutoff_03,
			best_depth);
	}
	if (count > 4) {
		best_depth = add_sampler(
			video_texture_04,
			depth_texture_04,
			depth_cutoff_04,
			best_depth);
	}
	if (count > 5) {
		best_depth = add_sampler(
			video_texture_05,
			depth_texture_05,
			depth_cutoff_05,
			best_depth);
	}
	if (count > 6) {
		best_depth = add_sampler(
			video_texture_06,
			depth_texture_06,
			depth_cutoff_06,
			best_depth);
	}
	if (count > 7) {
		best_depth = add_sampler(
			video_texture_07,
			depth_texture_07,
			depth_cutoff_07,
			best_depth);
	}
}

