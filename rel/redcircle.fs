/* Fragment shader */

uniform float width;
uniform float height;
uniform sampler2D texture0;
void main()
{
	vec4 texcolor = texture2D(texture0, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height));
	gl_FragColor = vec4(0.5+cos(texcolor.g*0.001+gl_FragCoord.x*4+sin(gl_FragCoord.y*0.1)*cos(texcolor.b*0.3)*texcolor.r)*texcolor.g,0.0,0.0,0.5+cos(texcolor.r*0.01)*0.5);
}

