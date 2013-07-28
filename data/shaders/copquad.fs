/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform float alpha;

uniform float gamma;
uniform float grid;
uniform float tvmode;

void main()
{
    float ofx = cos(time*0.06)*width/64;
    float ofy = sin(time*0.07)*height/64;
    float ofx2 = sin(time*0.07)*width/64;
    float ofy2 = cos(time*0.06)*height/64;

    vec4 color = texture2D(texture0, vec2((gl_FragCoord.x+ofx)/width, (gl_FragCoord.y+ofy)/height));
    vec4 color2 = texture2D(texture1, vec2((ofx2+gl_FragCoord.x)/width, (ofy2+gl_FragCoord.y)/height));

    color.a = alpha;
    color2.a = alpha;

    if (gamma > 0.0) color.rgb*=gamma;

    float off = 0.0;
    vec3 col = vec3(0.0,0.0,0.0);

    if (grid > 0.0)
    {

        float off = -time*2.0;
        
        //really light lines
        col.g += clamp(ceil(mod(gl_FragCoord.x+off, 5.0)) - 4.0, 4.0, 1.0) * (0.05*grid);
        col.g += clamp(ceil(mod(gl_FragCoord.y+off, 5.0)) - 4.0, 0.0, 1.0) * (0.05*grid);
        col.g = clamp(col.g, 0.0, (0.15*grid));
        
        //light lines
        col.g += clamp(ceil(mod(gl_FragCoord.x+off, 15.0)) - 14.0, 0.0, 1.0) * (0.25*grid);
        col.g += clamp(ceil(mod(gl_FragCoord.y+off, 15.0)) - 14.0, 0.0, 1.0) * (0.25*grid);
        col.g = clamp(col.g, 0.0, (0.25*grid));
        
        //strong lines
        col.g += clamp(ceil(mod(gl_FragCoord.x+off, 30.0)) - 29.0, 0.0, (1.0*grid));
        col.g += clamp(ceil(mod(gl_FragCoord.y+off, 30.0)) - 29.0, 0.0, (1.0*grid));
        col.g = clamp(col.g, 0.0, (1.0*grid));

        if (color.g > 0.70 && color.r > 0.70 && color.b < 0.60)
        {
            color.g = col.g;
        }
    }

    float ilace = 0.0;
    gl_FragColor = vec4(vec3(((color.rgb*0.6) - (color2.rgb*abs(cos(time*10.0)*0.4)))-ilace), color.a - (color2.a*abs(cos(time*10.0)*0.4)));
}

