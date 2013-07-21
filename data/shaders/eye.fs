/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;

varying float zpos;

uniform sampler2D texture0;

vec4 sampleEffect(float x, float y)
{
	float v = (gl_FragCoord.x+x / gl_FragCoord.y+y) * cos(zpos) * sin(zpos);
	v += (gl_FragCoord.y+y / gl_FragCoord.x+x) * tan(zpos) * atan(zpos);
	v -= (gl_FragCoord.y+y * gl_FragCoord.x+x) * cos(zpos) * tan(zpos);

	float v2 = (0.5*zpos+gl_FragCoord.x+x) + (0.4*zpos+gl_FragCoord.y+y);
	v2 -= (gl_FragCoord.y+y / gl_FragCoord.x+x) * tan(zpos) * atan(zpos);
	v2 += (gl_FragCoord.y+y * gl_FragCoord.x+x) * cos(zpos) * tan(zpos);

	vec4 color = vec4(v+v2*0.1-zpos*10.0-zpos,v*v2*0.15-zpos,v2-zpos*0.1,0.02);

	return color;	
}

void main()
{

	vec4 outcolor = (sampleEffect(0.0,0.0) + sampleEffect(1.0,0.0) + sampleEffect(-1.0,0.0) - sampleEffect(0.0,-1.0) + sampleEffect(0.0,1.0))*vec4(1.0,0.5,0.3,0.001);

	vec4 finaleffu = outcolor*vec4(-1.0*0.2,1.0*0.4,-1.0*0.4+cos(time*0.000001)*0.3,1.0)+outcolor*0.5-0.4*cos(gl_FragCoord.x*(0.005+atan(time*0.05*gl_FragCoord.x))+gl_FragCoord.y*(0.1+atan(time*0.01)));

	vec4 texcolor = texture2D(texture0, vec2((gl_FragCoord.x/width)+sin(time*0.01)*0.01, (gl_FragCoord.y/height)+cos(time*0.005)*0.01));

	outcolor = vec4(texcolor.r,finaleffu.r/texcolor.r*0.2,finaleffu.r-texcolor.r*0.1,finaleffu.a*(sin(time*0.000007)*texcolor.r*0.2));

//	outcolor.b *= texcolor.g*0.3;

	float redv = sin(time*0.00003)*0.98;

	float timefade = time*0.000005;

	gl_FragColor = vec4(outcolor.r,outcolor.r*(0.2+redv),outcolor.r*(0.2+redv),outcolor.a*redv*timefade);

}

