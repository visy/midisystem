/* Fragment shader */

uniform float width;
uniform float height;

uniform float time;
uniform sampler2D texture0;
uniform float alpha;

uniform float gamma;

void main()
{
    vec4 color = texture2D(texture0, vec2(gl_FragCoord.x/width, gl_FragCoord.y/height));

    color.a = alpha;

    if (gamma > 0.0) color.rgb*=gamma;
    gl_FragColor = color;
}

