#extension GL_OES_standard_derivatives : enable

uniform sampler2D videoTexture;
uniform float weight;


float median(float r, float g, float b) {
  return max(min(r, g), min(max(r, g), b));
}
void main( void )
{
	//simplest texture lookup
    vec3 sample = 1.0 - texture2D( videoTexture, gl_TexCoord[0].xy).gba;
    float sigDist = median(sample.r, sample.g, sample.b) - weight;
    float alpha = clamp(sigDist/fwidth(sigDist) + 0.5, 0.0, 1.0);
    gl_FragColor = vec4(alpha, 1.0, 1.0, 1.0);
//    gl_FragColor.rgb = vec3(texture2D( videoTexture, gl_TexCoord[0].xy).a);
//    gl_FragColor.a = 1.0;
}
