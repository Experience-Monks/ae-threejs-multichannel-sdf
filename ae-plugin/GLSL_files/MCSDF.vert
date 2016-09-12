void main( void )
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
	//gl_FrontColor = vec4(1.0, 0.0, 0.0, 1.0); // Hard-code red for testing purposes
}