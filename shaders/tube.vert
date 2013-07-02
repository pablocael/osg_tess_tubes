#version 400
in vec4 osg_Vertex;
in vec3 Normal;
in vec3 Binormal;
in float distanceTo0;

out vec3 vNormal;
out vec3 vPosition;
out vec3 vBinormal;
out float vDistanceTo0;

 void main( void )
 {
	vNormal = Normal;
    vPosition = osg_Vertex.xyz;
	vBinormal = Binormal;
	vDistanceTo0 = distanceTo0;
 }
 