/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform float alpha;

float kernel[9];
vec2 offset[9];

float step_w = 1.0/width;
float step_h = 1.0/height;

float ypos = 0.0;
float ypos2 = 0.0;

void main()
{

    offset[0] = vec2(-step_w, -step_h);
    offset[1] = vec2(0.0, -step_h);
    offset[2] = vec2(step_w, -step_h);
    offset[3] = vec2(-step_w, 0.0);
    offset[4] = vec2(0.0, 0.0);
    offset[5] = vec2(step_w, 0.0);
    offset[6] = vec2(-step_w, step_h);
    offset[7] = vec2(0.0, step_h);
    offset[8] = vec2(step_w, step_h);

    kernel[0] = 0.;
    kernel[1] = 1.;
    kernel[2] = 0.;
    kernel[3] = 1.;
    kernel[4] = -4.0*cos(time*10.5);
    kernel[5] = 1.;
    kernel[6] = 0.;
    kernel[7] = 1.;
    kernel[8] = 0.;

    vec4 sum = vec4(0.0);
    int i;

    float timespeed = 1.0;

    if (time > 56500.0) timespeed=2.8;

    ypos = height;
    if (time > 18000.0) ypos = height-(time-18000.0)*(0.006*timespeed);

    if (ypos < 0.0) ypos = 0.0;

    ypos2 = height;
    if (time > 13500.0) ypos2 = height-(time-13500.0)*(0.005*timespeed);

    if (ypos < 0.0) ypos = 0.0;
    if (ypos2 < 0.0) ypos2 = 0.0;

    if (time > 36000.0) ypos = ypos2;

    if (gl_FragCoord.y > ypos2)
    {
        for (i = 0; i < 9; i++) {
                vec4 c = vec4(0,0,0,0);
                if (gl_FragCoord.y > ypos) c = texture2D(texture1, vec2(gl_FragCoord.x/width, gl_FragCoord.y/(height)) + offset[i]);
                vec4 c2 = texture2D(texture0, vec2(gl_FragCoord.x/width, gl_FragCoord.y/(height)) + offset[i]);
                sum += ((c+c2)/2.0) * kernel[i];
        }

        vec4 color = sum;

        color.a = alpha;

        vec3 col = vec3(0.0,0.0,0.0);

        float off = -time*2.0;

        float grid = 1.0;
        
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

        if ((color.r + color.b + color.g)/3.0 > 0.3 && col.g > 0.2) { color.rgb = vec3(0.0,0.0,0.0); }

        if ((color.r + color.b + color.g)/3.0 < 0.8) color.rgb = vec3(0.0,0.0,0.0);
        gl_FragColor = color;
    }
}

