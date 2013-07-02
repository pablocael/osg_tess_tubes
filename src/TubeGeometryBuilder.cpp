#include "TubeGeometryBuilder.h"

#include <osg/Group>
#include <osg/LineWidth>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/LineWidth>

#include <cassert>

static void
LoadShaderSource( osg::Shader* shader, const std::string& fileName )
{
    std::string fqFileName = osgDB::findDataFile(fileName);
    if( fqFileName.length() != 0 )
    {
        shader->loadShaderSourceFromFile( fqFileName.c_str() );
    }
    else
    {
        osg::notify(osg::WARN) << "File \"" << fileName << "\" not found." << std::endl;
    }
}

static int index1Dfrom2D( int rowSize, int i, int j )
{
	return i * rowSize + j;
}

TubeGeometryBuilder::TubeGeometryBuilder()
{
	createShaderStuff();
}

static void clearTube( osg::Group* tubeGroup )
{
	tubeGroup->removeChildren( 0, tubeGroup->getNumChildren() );
	tubeGroup->getOrCreateStateSet()->clear();
}

void TubeGeometryBuilder::createTubeWithLOD( osg::Group* tubeGroup, double radius, osg::Vec4 color, 
			osg::Vec4 fluxColor, bool fluxUp, float fluxSpeed, int fluxStep, int numRadialVertices, float lineWidth )
{
	clearTube( tubeGroup );

	osg::Geode* cylinder = new osg::Geode();
	cylinder->addDrawable( makeCylinderGeometry( radius, color, numRadialVertices ) );

	osg::Geode* line = new osg::Geode();
	line->addDrawable( makeLineGeometry( color, lineWidth ) );

	tubeGroup->addChild( cylinder );
	tubeGroup->addChild( line );

	cylinder->getOrCreateStateSet()->setAttributeAndModes( _cylProgram, osg::StateAttribute::ON );
	line->getOrCreateStateSet()->setAttributeAndModes( _lineProgram, osg::StateAttribute::ON );

	osg::Uniform* timeUpdateUniform = new osg::Uniform( "TimeUpdate", 2.0f );
	timeUpdateUniform->setUpdateCallback( new TimeUpdate( fluxUp, fluxStep, fluxSpeed ) );
	tubeGroup->getOrCreateStateSet()->addUniform( timeUpdateUniform );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "fluxStep", static_cast<float>( fluxStep ) ) );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "color", color ) );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "fluxColor", fluxColor ) );
}

void TubeGeometryBuilder::createShaderStuff()
{
	_cylProgram = new osg::Program;
	_lineProgram = new osg::Program;
	_cylVertObj = new osg::Shader( osg::Shader::VERTEX );
	_lineVertObj = new osg::Shader( osg::Shader::VERTEX );
	_cylFragObj = new osg::Shader( osg::Shader::FRAGMENT );
	_lineFragObj = new osg::Shader( osg::Shader::FRAGMENT );
	_cylProgram->addShader( _cylFragObj );
	_cylProgram->addShader( _cylVertObj );
	_lineProgram->addShader( _lineFragObj );
	_lineProgram->addShader( _lineVertObj );

	_cylProgram->addBindAttribLocation( "fluxIndex", 6 );
	_lineProgram->addBindAttribLocation( "fluxIndex", 6 );

	LoadShaderSource( _cylVertObj, "shaders/tube.vert" );
	LoadShaderSource( _lineVertObj, "shaders/tube_line.vert" );
	LoadShaderSource( _lineFragObj, "shaders/tube_line.frag" );
	LoadShaderSource( _cylFragObj, "shaders/tube.frag" );
}

void TubeGeometryBuilder::reshapeTube( osg::Group* lod, double radius, co::int32 sectionVertices, osg::Vec4 color )
{
	osg::Geode* cylinder = lod->getChild( 0 )->asGeode();
	osg::Drawable* oldDrawable = cylinder->getDrawable( 0 );
	osg::Drawable* newDrawable = makeCylinderGeometry( radius, color, sectionVertices );
	cylinder->removeDrawable( oldDrawable );
	cylinder->addDrawable( newDrawable );
	//cylinder->replaceDrawable( oldDrawable, newDrawable );
}
void TubeGeometryBuilder::reshapeLine( osg::Group* lod, co::int32 lineWidth )
{
	osg::Geode* cylinder = lod->getChild( 1 )->asGeode();
	osg::Drawable* drawable = cylinder->getDrawable( 0 );
	osg::LineWidth* lineWidthState = new osg::LineWidth();
	lineWidthState->setWidth( lineWidth );
	drawable->getOrCreateStateSet()->setAttributeAndModes( lineWidthState, osg::StateAttribute::ON );
}

void TubeGeometryBuilder::setTrajectory( std::vector<osg::Vec3> trajectory, float verticalScale,
						float curveTolerance )
{
	_sections.clear();

	int numPointsInCurve = trajectory.size();

	// Scales vertically the original trajectory
	std::vector<osg::Vec3> scaledTraj;
	scaledTraj.reserve( numPointsInCurve );
	for( int i = 0; i < numPointsInCurve; i++ )
		scaledTraj.push_back( osg::Vec3( trajectory[i].x(), trajectory[i].y(), trajectory[i].z() * verticalScale ) );

	// The first point must be included
	osg::Vec3 tangent = scaledTraj[0] - scaledTraj[1];
	int secondPointIndex = 1;
	for(; tangent.length2() == 0; secondPointIndex++ )
		tangent = scaledTraj[0] - scaledTraj[secondPointIndex];
	TubeSectionBuilder firstPointBuilder = TubeSectionBuilder::getFirstPoint( scaledTraj[0], 
																			  scaledTraj[secondPointIndex] );
	
	_sections.push_back( firstPointBuilder.getSection() );

	TubeSectionBuilder previousPoint = firstPointBuilder;

	for( int i = secondPointIndex; i < numPointsInCurve-1; i++ ) // Do not operate on the first or the last point
	{
		TubeSectionBuilder currPointBuilder( scaledTraj[i], scaledTraj[i+1] );

		if( currPointBuilder.buildNewPoint( previousPoint, curveTolerance ) )
		{	
			_sections.push_back( currPointBuilder.getSection() );

			// reset the previousPoint
			previousPoint = currPointBuilder;

			// Upon reaching control point multiple of 32 we need to repeat it to connect the adjacent patches
			if( _sections.size() > 0 && _sections.size() % 32 == 0 )
			{
				_sections.push_back( currPointBuilder.getSection() );
			}
		}
	}
	// last point needs to be included always
	osg::Vec3 last = scaledTraj[numPointsInCurve-1];
	osg::Vec3 beforeLast = scaledTraj[numPointsInCurve-2];
	osg::Vec3 afterLast = last + last - beforeLast;
	TubeSectionBuilder lastPointBuilder( last, afterLast );
	lastPointBuilder.buildNewPoint( previousPoint, curveTolerance );
	_sections.push_back( lastPointBuilder.getSection() );
}

osg::Geometry* TubeGeometryBuilder::makeCylinderGeometry( double radius, osg::Vec4 color, int numRadialVertices )
{
	if( _sections.size() < 1 )
		throw std::exception( "Trajectory has not been set" );
	
	int numSections = _sections.size();

	// ----- Fill the Vertices and Normals array ----- //
	osg::Vec3Array* verts = new osg::Vec3Array;
	osg::Vec3Array* norms = new osg::Vec3Array;
	osg::FloatArray* distanceTo0 = new osg::FloatArray;
	float uIncrement = 1.0f / numRadialVertices;
	float currentDistanceTo0 = 0;
	osg::Vec3 lastPosition = _sections[0].position;
	for( int i = 0; i < numSections; i++ )
	{
		Section& section = _sections[i];
		osg::Vec3& Pc = section.position;
		osg::Vec3& Nc = section.normal;
		osg::Vec3& Bc = section.binormal;
		for( int j = 0; j < numRadialVertices; j++ )
		{
			float u = j * uIncrement;
			float theta = u * 6.283185307179586f; // u*2*pi
			
			osg::Vec3 C( radius * sin( theta ), radius * cos( theta ), 0.0 );
			osg::Vec3 finalPos( Pc.x() + C.x() * Nc.x() + C.y() * Bc.x(),
								Pc.y() + C.x() * Nc.y() + C.y() * Bc.y(),
								Pc.z() + C.x() * Nc.z() + C.y() * Bc.z() );

			verts->push_back( finalPos );

			osg::Vec3 normal = finalPos - Pc;
			normal.normalize();
			norms->push_back( normal );

			// Calculate current section's distance to first point and save it for flux animation
			osg::Vec3 lastSegment = Pc - lastPosition;
			currentDistanceTo0 += lastSegment.length();
			distanceTo0->push_back( currentDistanceTo0 );
			lastPosition = Pc;
		}
	}

	// ----- Fill the Indices array ----- //
	osg::DrawElementsUInt* inds = new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS, 0);
	for( int i = 0; i < numSections - 1; i++ )
	{
		for( int j = 0; j < numRadialVertices; j++ )
		{
			int iTop = i + 1;
			int jRight = ( j + 1 ) % numRadialVertices;
			int point = index1Dfrom2D( numRadialVertices, i, j );
			int pointRight = index1Dfrom2D( numRadialVertices, i, jRight );
			int pointTop = index1Dfrom2D( numRadialVertices, iTop, j );
			int pointTopRight = index1Dfrom2D( numRadialVertices, iTop, jRight );

			inds->push_back( point );
			inds->push_back( pointRight );
			inds->push_back( pointTopRight );
			inds->push_back( pointTop );
		}
	}

	
	osg::Geometry* geo = new osg::Geometry();
	geo->setVertexArray( verts );
	geo->setNormalArray( norms );
	geo->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
	geo->addPrimitiveSet( inds );
	geo->setVertexAttribArray( 6, distanceTo0 ); 
	geo->setVertexAttribBinding( 6, osg::Geometry::BIND_PER_VERTEX );
	return geo;
}

osg::Geometry* TubeGeometryBuilder::makeLineGeometry( osg::Vec4 color, float lineWidth )
{
	if( _sections.size() < 1 )
		throw std::exception( "Trajectory has not been set" );

	int numSections = _sections.size();

	// ----- Fill the Vertices and Normals array ----- //
	osg::Vec3Array* verts = new osg::Vec3Array;
	osg::FloatArray* distanceTo0 = new osg::FloatArray;
	float currentDistanceTo0 = 0;
	osg::Vec3 lastPosition = _sections[0].position;
	for( int i = 0; i < numSections; i++ )
	{
		Section& section = _sections[i];
		osg::Vec3& Pc = section.position;
		verts->push_back( Pc );

		// Calculate current section's distance to first point and save it for flux animation
		osg::Vec3 lastSegment = Pc - lastPosition;
		currentDistanceTo0 += lastSegment.length();
		distanceTo0->push_back( currentDistanceTo0 );
		lastPosition = Pc;
	}

	osg::Geometry* geo = new osg::Geometry();
	geo->setVertexArray( verts );
	geo->addPrimitiveSet( new osg::DrawArrays( GL_LINE_STRIP, 0, numSections ) );
	geo->setVertexAttribArray( 6, distanceTo0 ); 
	geo->setVertexAttribBinding( 6, osg::Geometry::BIND_PER_VERTEX );

	osg::LineWidth* lineWidthState = new osg::LineWidth();
	lineWidthState->setWidth( lineWidth );
	geo->getOrCreateStateSet()->setAttributeAndModes( lineWidthState, osg::StateAttribute::ON );

	return geo;
}

