uniform sampler2D videoTexture;
uniform sampler2D depthTexture;
uniform float depthCutoff;

varying vec2 texcoord;

void main()
{
	vec4 depthColor = texture2D(depthTexture, texcoord);
	gl_FragColor = texture2D(videoTexture, texcoord);
	if (depthColor.r > depthCutoff) {
		gl_FragColor.a = 0.0;
	}
	if (depthColor.r == 0.0) {
		gl_FragColor.a = 0.0;
	}
	
}
