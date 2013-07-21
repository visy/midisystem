uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;

uniform float width;
uniform float height;

void main (void)
{
	vec2 p = vec2((gl_FragCoord.x/width), 1.0-(gl_FragCoord.y/height));
	float y = texture2D(y_tex, p).r;
	float u = texture2D(u_tex, p).r-0.5;
	float v = texture2D(v_tex, p).r-0.5;
	float r = y + 1.13983*v;
	float g = y - 0.39465*u-0.58060*v;
	float b = y + 2.03211*u;
	gl_FragColor  = vec4(r,g,b,1);
}
