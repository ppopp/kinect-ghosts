
uniform sampler2D live_video_texture;
uniform sampler2D live_depth_texture;
uniform float live_depth_cutoff;
uniform int count;
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
	vec4 depth_color = texture2D(depth_texture, texcoord);
	if (depth_color.r != 0.0) {
		if (depth_color.r < best_depth) {
			gl_FragColor = texture2D(video_texture, texcoord);
			gl_FragColor.a = smooth_alpha(depth_color.r, depth_cutoff);
			return depth_color.r;
		}
	}
	return best_depth;	
}

void main()
{
	// apparently there are issues if you loop through an array of samplers.
	// bummer
	vec4 depth_color;
	gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
	float best_depth = 100000.0;
	float alpha = 0.0;

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

	// place newest clip on top
	best_depth = add_sampler(
		live_video_texture, 
		live_depth_texture, 
		live_depth_cutoff, 
		best_depth);
}

