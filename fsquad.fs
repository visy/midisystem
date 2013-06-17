/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;

float kernel[9];
vec2 offset[9];

void main()
{
	float step_w = (1.0+abs(cos(time*0.00001)*9.0))/width;
	float step_h = (1.0+abs(sin(time*0.00001)*9.0))/height;

    offset[0] = vec2(-step_w, -step_h);
    offset[1] = vec2(0.0, -step_h);
    offset[2] = vec2(step_w, -step_h);
    offset[3] = vec2(-step_w, 0.0);
    offset[4] = vec2(0.0, 0.0);
    offset[5] = vec2(step_w, 0.0);
    offset[6] = vec2(-step_w, step_h);
    offset[7] = vec2(0.0, step_h);
    offset[8] = vec2(step_w, step_h);

    /* SHARPEN KERNEL
     0 -1  0
    -1  5 -1
     0 -1  0
    */


    float val1 = 1.;
    float val2 = 5.;
    float val3 = 1.;

    kernel[0] = val1;
    kernel[1] = val2;
    kernel[2] = val3;
    kernel[3] = val1;
    kernel[4] = 1;
    kernel[5] = val3;
    kernel[6] = val1;
    kernel[7] = val2;
    kernel[8] = val3;

    vec4 sum = vec4(0.0);
    int i;

    for (i = 0; i < 9; i++) {
            vec4 color = texture2D(texture0, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height) + offset[i]);
            sum += color * kernel[i];
    }

    gl_FragColor = sum;
}

