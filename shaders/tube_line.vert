attribute float fluxIndex;

varying float vDistanceTo0;

 void main( void )
 {
	vDistanceTo0 = fluxIndex;
	
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4( gl_Vertex.xyz, 1.0f );
 }
 