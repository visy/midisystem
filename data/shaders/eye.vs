/* Vertex shader */
uniform float waveTime;
uniform float waveWidth;
uniform float waveHeight;

varying float zpos;

void main(void)
{
	vec4 v = vec4(gl_Vertex);
 
	v.z = cos(waveTime*100.0)/(waveWidth * v.x + waveTime) * (waveWidth * v.y + waveTime) * waveHeight;	
	zpos = v.z;

	gl_Position = gl_ModelViewProjectionMatrix * v;
}

