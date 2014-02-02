attribute vec2 position;
varying vec2 texcoord;

void main()
{
	vec2 flipped = vec2(position[0], -1.0 * position[1]);
	gl_Position = vec4(position, 0.0, 1.0);
	texcoord = flipped * vec2(0.5) + vec2(0.5);
}

