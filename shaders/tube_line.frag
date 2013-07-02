uniform float fluxStep;
uniform float TimeUpdate;
uniform vec4 color;
uniform vec4 fluxColor;

varying float vDistanceTo0;

void main( void )
{
	float hasNoFlux = mod( vDistanceTo0 + TimeUpdate, fluxStep ) / fluxStep;

	gl_FragColor = mix( fluxColor, color, sin( hasNoFlux * 3.14 ) ) * vec4( 0.65, 0.65, 0.65, 1.0 );
}
