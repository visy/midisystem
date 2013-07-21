/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform float alpha;

int scroll_trig = 0;
float trig_time = 0.0;
void main()
{
	float zoom = atan(time*0.00015);

    float scroll = 0.0;

	if (zoom > 0.95) {
        zoom = 1.0;
        scroll_trig = 1;
    }

    if (scroll_trig == 1)
    {
        scroll = -(time-9000.0)*0.1;
        if (scroll < -width/4.0) scroll = -width/4.0;
    }

    vec4 color = texture2D(texture0, vec2((scroll+gl_FragCoord.x)/width*zoom, (gl_FragCoord.y)/height*zoom));
    vec4 color3 = texture2D(texture0, vec2(gl_FragCoord.x/width*cos(time*0.01+gl_FragCoord.y*0.01+cos(time)*0.001), gl_FragCoord.y/height*sin(time*0.02+gl_FragCoord.x*0.02)));
    vec4 color4 = texture2D(texture0, vec2(gl_FragCoord.x/width*tan(time*0.001+gl_FragCoord.x*0.002), gl_FragCoord.y/height*tan(time*0.001+gl_FragCoord.x*0.002)));

    color.a = alpha;
    if (color.r > 0.1 && color.g > 0.1 && color.b > 0.1)
    {
//    	float effu = (0.1*cos(gl_FragCoord.y*0.1+time));
    	vec4 color2 = vec4(color.r*color3.r*0.2-color4.r*time*0.0007, color.g*color3.g*0.1-color4.r*time*0.0005, color.b*color3.b*0.2-color4.r*time*0.0004, color3.r*color.a*0.1+abs(zoom));
    	gl_FragColor = color2+(color*zoom*abs((0.5*sin(time*0.0005))));
    }
}

