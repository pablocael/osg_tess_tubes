#version 400
in vec4 osg_Vertex;
in float distanceTo0;

out vec3 vPosition;
out float vDistanceTo0;

 void main( void )
 {
	vDistanceTo0 = distanceTo0;
	
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4( osg_Vertex.xyz, 1.0f );
 }
 