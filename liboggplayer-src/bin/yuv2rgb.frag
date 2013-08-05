uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;

void main (void)
{
	float y = texture2D(y_tex, vec2(gl_TexCoord[0])).r;
	float u = texture2D(u_tex, vec2(gl_TexCoord[0])).r-0.5;
	float v = texture2D(v_tex, vec2(gl_TexCoord[0])).r-0.5;
	float r = y + 1.13983*v;
	float g = y - 0.39465*u-0.58060*v;
	float b = y + 2.03211*u;
	gl_FragColor  = vec4(r,g,b,1);
}
