/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;

uniform float effu;

float H = 0.02;
float S = ((3./2.) * H/sqrt(3.));

vec2 hexCoord(ivec2 hexIndex) {
    int i = hexIndex.x;
    int j = hexIndex.y;
    vec2 r;
    r.x = i * S;
    r.y = j * H + (i%2) * H/2.;
    return r;
}

ivec2 hexIndex(vec2 coord) {
    ivec2 r;
    float x = coord.x;
    float y = coord.y;
    int it = int(floor(x/S));
    float yts = y - float(it%2) * H/2.;
    int jt = int(floor((1./H) * yts));
    float xt = x - it * S;
    float yt = yts - jt * H;
    int deltaj = (yt > H/1.5)? 1:0;
    float fcond = S * (1./2.) * abs(0.5 - yt/H);

    if (xt > fcond) {
        r.x = it;
        r.y = jt;
    }
    else {
        r.x = it - 1;
        r.y = jt - (r.x%2) + deltaj;
    }

    return r;
}

void main(void)
{
    H = 0.01;
    float S = ((((2.)/(2.*1.5)) * H/sqrt(3.*0.5)));

    vec2 xy = vec2((gl_FragCoord.x/width), (gl_FragCoord.y/height));
    ivec2 hexIx = hexIndex(xy);
    vec2 hexXy = hexCoord(hexIx);
    vec4 acol = texture2D(texture0, xy);
    vec4 fcol = texture2D(texture0, hexXy);
    vec4 b = (fcol-(acol*0.4))*1.7;
    if (effu == 0.0)
        gl_FragColor = acol;
    else
        gl_FragColor = vec4(b.r,b.g,b.b,1.0);

}

