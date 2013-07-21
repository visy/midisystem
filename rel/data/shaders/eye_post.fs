/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform sampler2D texture1;

float kernel[9];
vec2 offset[9];


float hash(float x)
{
    return fract(sin(x) * 43758.5453);
}

float noise(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y * 57.0 + p.z*113.0;
    
    float a = hash(n);
    return a;
}

float manhattan(vec3 v)
{
    v = abs(v);
    return v.x + v.y + v.z;
}

float cheby(vec3 v)
{
    v = abs(v);
    return v.x > v.y
    ? (v.x > v.z ? v.x : v.z)
    : (v.y > v.z ? v.y : v.z);
}

float vor(vec3 v)
{
    vec3 start = floor(v);
    
    float dist = 999999.0;
    vec3 cand;

    int z = 0;
    
    for(int y = 0; y <= 4; y += 1)
    {
        for(int x = 0; x <= 4; x += 1)
        {
            vec3 t = start + vec3(-2+x, -2+y, -2+(x*y));
            vec3 p = t + noise(t);

            float d = manhattan(p - v);

            if(d < dist)
            {
                dist = d;
                cand = p;
            }
        }
    }
    
    vec3 delta = cand - v;
    
    return manhattan(delta);
    //return noise(cand); //length(delta);
}


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
    kernel[4] = 1.0;
    kernel[5] = val3;
    kernel[6] = val1;
    kernel[7] = val2;
    kernel[8] = val3;

    vec4 sum = vec4(0.0);
    int i;

    for (i = 0; i < 9; i++) {
            vec4 color = texture2D(texture0, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height) + offset[i]);
            vec4 color2 = texture2D(texture1, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height));
            sum += color * kernel[i]-(color2*(tan(time*0.1)*0.01));
    }

    gl_FragColor = sum;
}

