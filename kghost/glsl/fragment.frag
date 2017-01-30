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

float add_sampler(
	in sampler2D video_texture, 
	in sampler2D depth_texture, 
	in float depth_cutoff, 
	in float best_depth) 
{
	vec4 new_color = vec4(0.0, 0.0, 0.0, 1.0);
	float depth = texture2D(depth_texture, texcoord).a;
	float alpha = smooth_alpha(depth, depth_cutoff);

	//if (alpha > 0.1) {
		//alpha = 0.5;
	//}

	if (depth != 0.0) {
		if (depth < best_depth) {
			if (alpha > 0.0) {
				new_color = texture2D(video_texture, texcoord);
				new_color.a = alpha;
				if (alpha >= 1.0) {
					gl_FragColor = new_color;
				}
				else if (gl_FragColor.a > 0.1) {
					gl_FragColor = (gl_FragColor + new_color);
					gl_FragColor = new_color;
				}
				else {
					gl_FragColor = (gl_FragColor + new_color);
					//gl_FragColor = new_color;
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
	float test = depth_horizontal_pixel_stride;
	float test2 = depth_vertical_pixel_stride;

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
