/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform float alpha;

void main()
{
	float zoom = time*0.0001;

	if (zoom > 1.0) zoom = 1.0;

    vec4 color = texture2D(texture0, vec2(gl_FragCoord.x/width*zoom, gl_FragCoord.y/height*zoom));
    vec4 color3 = texture2D(texture0, vec2(gl_FragCoord.x/width*cos(time+gl_FragCoord.y), gl_FragCoord.y/height*sin(time+gl_FragCoord.x)));

    color.a = alpha;
    if (color.r > 0.1 && color.g > 0.1 && color.b > 0.1)
    {
//    	float effu = (0.1*cos(gl_FragCoord.y*0.1+time));
    	vec4 color2 = vec4(color.r*color3.r, color.g*color3.g, color.b*color3.b, color3.r*color.a*0.1*zoom);
    	gl_FragColor = color2+(color*zoom*(0.05*cos(time*0.01)));
    }
}

