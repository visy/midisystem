/* Fragment shader */

uniform float width;
uniform float height;

varying float zpos;

vec4 sampleEffect(float x, float y)
{
	float v = (gl_FragCoord.x+x / gl_FragCoord.y+y) * cos(zpos) * sin(zpos);
	v += (gl_FragCoord.y+y / gl_FragCoord.x+x) * tan(zpos) * atan(zpos);
	v -= (gl_FragCoord.y+y * gl_FragCoord.x+x) * cos(zpos) * tan(zpos);

	float v2 = (0.5*zpos+gl_FragCoord.x+x) + (0.4*zpos+gl_FragCoord.y+y);
	v2 -= (gl_FragCoord.y+y / gl_FragCoord.x+x) * tan(zpos) * atan(zpos);
	v2 += (gl_FragCoord.y+y * gl_FragCoord.x+x) * cos(zpos) * tan(zpos);

	vec4 color = vec4(v+v2*0.2-zpos*100.0-zpos,v*v2*0.15-zpos,v2-zpos,0.2);

	return color;	
}

void main()
{

	gl_FragColor = (sampleEffect(0.0,0.0) + sampleEffect(1.0,0.0) + sampleEffect(-1.0,0.0) + sampleEffect(0.0,-1.0) + sampleEffect(0.0,1.0))/vec4(1.0,1.0,1.0,1.0);


}

