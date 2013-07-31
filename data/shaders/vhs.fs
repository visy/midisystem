/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform sampler2D texture1;

float kernel[9];
vec2 offset[9];

uniform float effu;
uniform float beat;
uniform float noisetin;

float hash(float x)
{
    return fract(sin(x) * 43758.5453);
}

float noise(float x)
{
    float p = floor(x);
    float f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p * 57.0*cos(time);
    
    float a = hash(n);
    return cos(a);
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


float linecount = 90.0;
const vec4 gradA = vec4(0.0, 0.1, 0.0, 1.0);
const vec4 gradB = vec4(0.2, 0.5, 0.1, 1.0);
const vec4 gradC = vec4(0.9, 1.0, 0.6, 1.0);

vec2 pos, uv;

vec4 base(void)
{
    return texture2D(texture0, uv + .1 * noise(time) * vec2(0.1, 0.0));
}

float triangle(float phase)
{
    phase *= 2.0;
    return 1.0 - abs(mod(phase, 2.0) - 1.0);
}

float scanline(float factor, float contrast)
{
    vec4 v = base();
    float lum = .2 * v.x + .5 * v.y + (.3+time*0.001)  * v.z;
    lum *= noise(time);
    float tri = triangle(pos.y * linecount);
    tri = pow(tri, contrast * (1.0 - lum) + .5);
    return tri * lum;
}

vec4 gradient(float i)
{
    i = clamp(i, 0.0, 1.0) * 2.0;
    if (i < 1.0) {
        return (1.0 - i) * gradA + i * gradB;
    } else {
        i -= 1.0;
        return (1.0 - i) * gradB + i * gradC;
    }
}

vec4 vignette(vec4 at)
{
    float dx = 1.3 * abs(pos.x - .5);
    float dy = 1.3 * abs(pos.y - .5);
    return at * (1.0 - dx * dx - dy * dy);
}

void main()
{
	float step_w = (1.0+abs(cos(time*0.01)*9.0))/width;
	float step_h = (1.0+abs(sin(time*0.01)*9.0))/height;

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
    float val2 = -2.;
    float val3 = 1.;

    kernel[0] = val1;
    kernel[1] = val2;
    kernel[2] = val3;
    kernel[3] = val1;
    kernel[4] = 2.0;
    kernel[5] = val3;
    kernel[6] = val1;
    kernel[7] = val2;
    kernel[8] = val3;

    vec4 sum = vec4(0.0);
    int i;

    for (i = 0; i < 9; i++) {
            vec4 color = texture2D(texture0, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height) + offset[i]);
            vec4 color2 = texture2D(texture1, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height));
            sum += color * kernel[i]-(abs(cos(time*0.01))*0.1);
    }

    vec4 color = texture2D(texture0, vec2((gl_FragCoord.x)/width, (gl_FragCoord.y)/height));
    vec4 displace = vec4(sum.rgb*abs((cos(time*0.4)*0.2)),color.a);

    pos = uv = (gl_FragCoord.xy - vec2(0.0, 0.5)) / vec2(width,height);
    uv.y = floor(uv.y * linecount) / linecount;

    vec4 noi = vec4(0.0,0.0,0.0,0.0);

    if (noisetin > 0.0) {
        noi.r = hash(cos(noisetin+pos.x+pos.y+time)*100.0)*noisetin;
        noi.g = hash(sin(noisetin+pos.x+pos.y+time)*100.0)*noisetin;
        noi.b = hash(cos(noisetin+pos.x+pos.y+time)*100.0)*noisetin;
        noi.a = hash(sin(noisetin+pos.x+pos.y+time)*100.0)*noisetin;        
    }

    vec4 finalcolor;

    if (effu == 1.0) finalcolor = (vignette(gradient(scanline(0.2, 1.5-atan(beat)*6.0))) + vec4(displace.rgb+color.rgb,color.a));
    else if (effu == 2.0) finalcolor = (vec4(abs(color.r-beat*0.2),abs(color.g-beat*0.2),abs(color.b-beat*0.2),color.a));
    else if (effu == 3.0) finalcolor = vec4((displace.rgb+color.rgb),color.a);
    else finalcolor = vec4(displace.rgb+color.rgb,color.a);

    if (effu != 2.0 && effu != 3.0) gl_FragColor = finalcolor-noi;
    else if (effu == 3.0) {
        if (finalcolor.r > 0.3) gl_FragColor = finalcolor;
        else gl_FragColor = finalcolor/vec4(hash(tan(time+tan(pos.x+time)+tan(pos.y-time))),1.0,1.0,1.0);
    }
    else {
        if (finalcolor.r > 0.5)
            gl_FragColor = finalcolor+(noi*0.5);
        else
            gl_FragColor = displace;
    }
}

