#include "TubeGeometryBuilder.h"

#include <osg/Group>
#include <osg/LineWidth>
#include <osg/PatchParameter>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/LineWidth>

#include <cassert>
#include <iostream>

static osg::ref_ptr<osg::Program> s_cylProgram;
static osg::ref_ptr<osg::Program> s_lineProgram;
static osg::ref_ptr<osg::Shader>  s_cylVertObj;
static osg::ref_ptr<osg::Shader>  s_lineVertObj;
static osg::ref_ptr<osg::Shader>  s_cylFragObj;
static osg::ref_ptr<osg::Shader>  s_lineFragObj;
static osg::ref_ptr<osg::Shader>  s_cylEvalObj;
static osg::ref_ptr<osg::Shader>  s_cylControlObj;
static bool shaderLoaded = false;


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
}

static void clearTube( osg::Group* tubeGroup )
{
	tubeGroup->removeChildren( 0, tubeGroup->getNumChildren() );
	tubeGroup->getOrCreateStateSet()->clear();
}

void TubeGeometryBuilder::createTubeWithLOD( osg::Group* tubeGroup, osg::Camera* cam, float radius, float minRadius, 
	osg::Vec4 color, osg::Vec4 fluxColor, bool fluxUp, float fluxSpeed, int fluxStep, int numRadialVertices, float lineWidth )
{
	if(!shaderLoaded)
	{
		createShaderStuff();
		shaderLoaded = true;
	}

	clearTube( tubeGroup );

	osg::Geode* cylinder = new osg::Geode();
	cylinder->addDrawable( makeCylinderGeometry( radius, color, numRadialVertices ) );

	tubeGroup->addChild( cylinder );

	cylinder->getOrCreateStateSet()->setAttributeAndModes( s_cylProgram, osg::StateAttribute::ON );

	osg::Matrixd m = cam->getViewMatrix();
	osg::Uniform* mvpInverseUniform = new osg::Uniform( "MVPinverse", osg::Matrixf() );
	mvpInverseUniform->setUpdateCallback( new MVPInverseCallback( cam ) );
	tubeGroup->getOrCreateStateSet()->addUniform( mvpInverseUniform );

	osg::Uniform* screenUniform = new osg::Uniform( "screenWidth", 10000.0f );
	screenUniform->setUpdateCallback( new ScreenCallback( cam ) );
	tubeGroup->getOrCreateStateSet()->addUniform( screenUniform );

	osg::Uniform* timeUpdateUniform = new osg::Uniform( "TimeUpdate", 2.0f );
	timeUpdateUniform->setUpdateCallback( new TimeUpdate( fluxUp, fluxStep, fluxSpeed ) );
	tubeGroup->getOrCreateStateSet()->addUniform( timeUpdateUniform );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "fluxStep", static_cast<float>( fluxStep ) ) );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "color", color ) );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "radius", radius ) );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "minRadius", minRadius ) );

	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "fluxColor", fluxColor ) );

	// TODO: Needs a global OSG uniform for the lights intead of this!
	tubeGroup->getOrCreateStateSet()->addUniform( new osg::Uniform( "lightPos", ::osg::Vec3( 10000, 10000, 10000 ) ) );

	cylinder->getOrCreateStateSet()->setAttribute(new osg::PatchParameter(32));
}

void TubeGeometryBuilder::createShaderStuff()
{
	s_cylProgram = new osg::Program;
	s_cylVertObj = new osg::Shader( osg::Shader::VERTEX );
	s_cylFragObj = new osg::Shader( osg::Shader::FRAGMENT );
	s_cylControlObj = new osg::Shader( osg::Shader::TESSCONTROL );
    s_cylEvalObj = new osg::Shader( osg::Shader::TESSEVALUATION );
	s_cylProgram->addShader( s_cylFragObj );
	s_cylProgram->addShader( s_cylVertObj );
	s_cylProgram->addShader( s_cylControlObj );
	s_cylProgram->addShader( s_cylEvalObj );

	s_cylProgram->addBindAttribLocation( "Normal", 2 );
	s_cylProgram->addBindAttribLocation( "Binormal", 3 );
	s_cylProgram->addBindAttribLocation( "distanceTo0", 6 );

	LoadShaderSource( s_cylVertObj, "shaders/tube.vert" );
	LoadShaderSource( s_cylFragObj, "shaders/tube.frag" );
	LoadShaderSource( s_cylControlObj, "shaders/tube.control" );
	LoadShaderSource( s_cylEvalObj, "shaders/tube.eval" );
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
	
	osg::Vec3Array* pos = new osg::Vec3Array;
	osg::Vec3Array* nor = new osg::Vec3Array;
	osg::Vec3Array* bin = new osg::Vec3Array;

	osg::FloatArray* distanceTo0 = new osg::FloatArray;
	float uIncrement = 1.0f / numRadialVertices;
	float currentDistanceTo0 = 0;
	osg::Vec3 lastPosition = _sections[0].position;

	int numSections = 0;
	for( int i = 0; i < _sections.size(); i++ )
	{
		Section& section = _sections[i];
		pos->push_back( section.position );
		nor->push_back( section.normal );
		bin->push_back( section.binormal );

		numSections++;

		if( numSections > 0 && numSections % 32 == 0 )
		{
			pos->push_back( section.position );
			nor->push_back( section.normal );
			bin->push_back( section.binormal );
			numSections++;
		}

		// Calculate current section's distance to first point and save it for flux animation
		osg::Vec3 lastSegment = section.position - lastPosition;
		currentDistanceTo0 += lastSegment.length();
		distanceTo0->push_back( currentDistanceTo0 );
		lastPosition = section.position;
	}

	osg::Geometry* geo = new osg::Geometry();
	geo->setVertexArray(pos);
	geo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::PATCHES,0,numSections));
	geo->setVertexAttribArray( 2, nor ); 
	geo->setVertexAttribBinding( 2, osg::Geometry::BIND_PER_VERTEX );
	geo->setVertexAttribArray( 3, bin ); 
	geo->setVertexAttribBinding( 3, osg::Geometry::BIND_PER_VERTEX );
	geo->setVertexAttribArray( 6, distanceTo0 ); 
	geo->setVertexAttribBinding( 6, osg::Geometry::BIND_PER_VERTEX );
	return geo;
}

