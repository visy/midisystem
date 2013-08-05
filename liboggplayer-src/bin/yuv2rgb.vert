void main(void)
{
   vec4 v = gl_Vertex;
   gl_TexCoord[0] = gl_MultiTexCoord0;
   gl_Position = gl_ModelViewProjectionMatrix * v;
}
