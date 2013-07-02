#version 400
in vec4 osg_Vertex;
in float fluxIndex;

out vec3 vNormal;
out vec4 vPosition;
out vec3 vLightPos;
out float vDistanceTo0;

uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat3 osg_NormalMatrix;

 void main( void )
 {
	vNormal = normalize( osg_NormalMatrix * gl_Normal );
    vPosition = osg_ModelViewMatrix * osg_Vertex;
	vLightPos = vec3(0,-1,0);
	vDistanceTo0 = fluxIndex;
	
	gl_Position = osg_ProjectionMatrix * osg_ModelViewMatrix * vec4( osg_Vertex.xyz, 1.0f );
 }
 