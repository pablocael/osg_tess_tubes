#version 400
uniform float fluxStep;
uniform float TimeUpdate;
uniform vec4 color;
uniform vec4 fluxColor;

in vec3 vNormal;
in vec4 vPosition;
in vec3 vLightPos;
in float vDistanceTo0;

void main( void )
{
	vec3 lightPower = vec3( 0.5, 0.5 ,0.5 );
	
	vec3 s = normalize( vLightPos - vPosition.xyz );
	vec3 v = normalize( -vPosition.xyz );
	vec3 r = reflect( -s, vNormal );
	vec3 ambient = vec3( 0.3, 0.3, 0.3 );
	float sDotN = max( dot( s, vNormal ), 0.0 );
	vec3 diffuse = lightPower * sDotN;

	float spec = pow( max( dot(r,v), 0.0 ), 3 );
	
	vec3 lightTotal = ambient + diffuse + vec3( spec, spec, spec );

	float hasNoFlux = mod( vDistanceTo0 + TimeUpdate, fluxStep ) / fluxStep;

	vec4 finalColor = mix( fluxColor, color, sin( hasNoFlux * 3.14 ) );
	
	gl_FragColor = vec4( finalColor.xyz * lightTotal, finalColor.w );
}
