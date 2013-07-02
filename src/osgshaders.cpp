/* OpenSceneGraph example, osgshaders.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

/* file:        examples/osgglsl/osgshaders.cpp
 * author:        Mike Weiblen 2005-04-05
 *
 * A demo of the OpenGL Shading Language shaders using core OSG.
 *
 * See http://www.3dlabs.com/opengl2/ for more information regarding
 * the OpenGL Shading Language.
*/

#include <osg/Notify>
#include <osgGA/GUIEventAdapter>
#include <osgGA/GUIActionAdapter>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osgViewer/Viewer>
#include <osgGA/StateSetManipulator>
#include <osg/ShapeDrawable>

#include "TubeGeometryBuilder.h"

using namespace osg;

///////////////////////////////////////////////////////////////////////////

class KeyHandler: public osgGA::GUIEventHandler
{
    public:
        KeyHandler( std::string text ) : _text( text )
        {}

        bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& )
        {
            return false;
        }

		std::string _text;
};

///////////////////////////////////////////////////////////////////////////
std::vector<osg::Vec3> genTrajectory( int numPoints )
{
	std::vector<osg::Vec3> trajectory;

	float delta = 0;
	for( int i = 0; i < numPoints; i++ )
	{
		delta = 0.2 * i;
		trajectory.push_back( osg::Vec3( sin( delta ), delta, cos( delta ) ) );
	}
	return trajectory;
}


int main(int, char **)
{
    // construct the viewer.
    osgViewer::Viewer viewer;
	viewer.setUpViewInWindow(100,100,700,300);
	viewer.setLightingMode( osg::View::HEADLIGHT );
	viewer.addEventHandler( new KeyHandler( "HANDLER 1" ) );
	viewer.addEventHandler( new KeyHandler( "HANDLER 2" ) );
	viewer.addEventHandler( new KeyHandler( "HANDLER 4" ) );

	osgGA::StateSetManipulator* ssm = new osgGA::StateSetManipulator( viewer.getCamera()->getOrCreateStateSet() );

	// Create a testiung axis
	osg::Geode* axis = new osg::Geode();
	osg::Geometry* geometry = new osg::Geometry();
	axis->addDrawable(geometry); 
	osg::Vec3Array* vertices = new osg::Vec3Array;
	vertices->push_back( osg::Vec3( 0, 0, 0 ) );
	vertices->push_back( osg::Vec3( 1, 0, 0 ) );
	vertices->push_back( osg::Vec3( 0, 1, 0 ) );
	vertices->push_back( osg::Vec3( 0, 0, 1 ) );
	vertices->push_back( osg::Vec3( 2, 0.2*31, 0 ) );
	vertices->push_back( osg::Vec3( -2, 0.2*31, 0 ) );
	vertices->push_back( osg::Vec3( 0, 0.2*31, 2 ) );
	vertices->push_back( osg::Vec3( 0, 0.2*31, -2 ) );
	geometry->setVertexArray( vertices ); 
	osg::DrawElementsUInt* indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, 0);
	indices->push_back( 0 );
	indices->push_back( 1 );
	indices->push_back( 0 );
	indices->push_back( 2 );
	indices->push_back( 0 );
	indices->push_back( 3 );
	indices->push_back( 4 );
	indices->push_back( 5 );
	indices->push_back( 6 );
	indices->push_back( 7 );
	geometry->addPrimitiveSet( indices );
	osg::Vec4Array* colors = new osg::Vec4Array;
	
    colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f) ); //index 3 white
    colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f) ); //index 0 red
    colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f) ); //index 1 green
    colors->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f) ); //index 2 blue
	colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f) ); //index 3 white
	colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f) ); //index 3 white
	colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f) ); //index 3 white
	colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f) ); //index 3 white
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

	//Gets camera params
	osg::Camera* cam = viewer.getCamera();
	double fov,n,f,ar;
	cam->getProjectionMatrixAsPerspective( fov, ar, n, f );
	int screenSize = cam->getViewport()->height();

	// Creates the tube node.
	std::vector<osg::Vec3> traj = genTrajectory( 600 );
	TubeGeometryBuilder tgb;
	tgb.setTrajectory( traj );

	bool _fluxOn = true; 
	bool _fluxUp = true;
	co::int32 _fluxSpeed = 10;
	co::int32 _fluxStep = 10;
	double _tubeRadius = 0.4f;
	co::int32 _lineWidth = 4;
	co::int32 _sectionVertices = 10;
	double _curveTolerance = 0.001;
	double _verticalScale = 1;
	::osg::Vec4 _fluxColor = osg::Vec4(1,0,0,1);
	::osg::Vec4 _tubeColor = osg::Vec4(1,1,1,1);
	osg::Group* geode = new osg::Group;
	tgb.createTubeWithLOD( geode, _tubeRadius, _tubeColor, 
						_fluxColor, _fluxUp, _fluxSpeed, _fluxStep, _sectionVertices, _lineWidth );
	
	osg::Group* root = new osg::Group();
	root->addChild( geode );
	root->addChild( axis );
    viewer.setSceneData( root );

	// switch on the uniforms that track the modelview and projection matrices
    osgViewer::Viewer::Windows windows;
    viewer.getWindows(windows);
    for(osgViewer::Viewer::Windows::iterator itr = windows.begin();
        itr != windows.end();
        ++itr)
    {
        osg::State *s=(*itr)->getState();
        s->setUseModelViewAndProjectionUniforms(true);
        s->setUseVertexAttributeAliasing(true);
    }

	viewer.frame();
    return viewer.run();
}

/*EOF*/

