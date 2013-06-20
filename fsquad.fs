/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform float alpha;

uniform float gamma;
uniform float grid;

void main()
{
    vec4 color = texture2D(texture0, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height));

    color.a = alpha;

    if (gamma > 0.0) color.rgb*=gamma;

    if (grid > 0.0)
    {
        vec3 col = vec3(0.0,0.0,0.0);

        float off = -time*2;
        
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

        if (((color.r + color.b + color.g) / 3.0) > 0.1)
        {
            if (color.r / col.g < 0.2) color.rgb = 0.0;
        }
        else
        color.rgb -= col.g*(0.5*cos(time*0.1+(gl_FragCoord.x+gl_FragCoord.y)));
    }

    gl_FragColor = color;
}

