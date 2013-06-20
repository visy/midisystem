
uniform float time;
/* Vertex shader */
void main(void)
{
	vec4 v = vec4(gl_Vertex);
	v.z *= cos(time*0.1)*3.0;
	v.x *= sin(time*0.1)*3.0;
	v.y *= cos(time*0.1)*3.0;
	gl_Position = gl_ModelViewProjectionMatrix * v;
}

