 #ifndef _TUBE_GEOMETRY_BUILDER_
#define _TUBE_GEOMETRY_BUILDER_

#include <osg/Vec3>

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>

#include <cassert>
#include <iostream>

namespace co {
typedef int int32;
}

class Section;

class TimeUpdate: public osg::Uniform::Callback
{
    public:
		// stepSpeec is how maxy times faster than 1 update per second. i.e. stepSpeed = 1 means 1 update per sec. 
		TimeUpdate( bool fluxUp = true, int fluxStep = 7, float updateSpeed = 20.0f ) : _fluxStep( fluxStep ),
			_fluxUp( fluxUp ), _lastTime( 0 ), _updateSpeed( updateSpeed )
		{
		}

        virtual void operator() ( osg::Uniform* uniform, osg::NodeVisitor* nv )
        {
           double timeNow = nv->getFrameStamp()->getReferenceTime() * _updateSpeed;
			int elapsedTime = timeNow - _lastTime;
			if( elapsedTime > _fluxStep )
				_lastTime = timeNow;

			if( _fluxUp )
				uniform->set( static_cast<float>( elapsedTime ) );
			else
				uniform->set( static_cast<float>( _fluxStep - elapsedTime ) );
        }
private:
	int _lastTime;
	bool _fluxUp;
	int _fluxStep;
	float _updateSpeed;
};

/*
	This class gets a list of points and creates a proper tube/line geometry from it.
	Additionaly, it may cull points from the trajectory if they are colinear.
*/
class TubeGeometryBuilder
{
public:
	/*
		The instance of this class will reuse the same shaders for every tube built
	*/
	TubeGeometryBuilder();

	/*
		Sets the list of points that will be used for tube creation. 
		\curveTolerance is how much tolerance for culling a trajectory point, it means
		how much rotation is accepted to have between 2 points. if set to a high value,
		only the first and the last point will be used.
	*/
	void setTrajectory( std::vector<osg::Vec3> trajectory, float verticalScale = 1.0f, 
		float curveTolerance = 0.001f );

	/*
		This function creates a LOD node with a cylinder for close view and a line for far view.
		\fluxColor is the color that of the "flowing" segments.
		\fluxSpeed is how many times faster than 1 update per second the flux should be.
		\numRadialVertices is the amount of vertices per section of the tube.
	*/
	void createTubeWithLOD( osg::Group* tubeGroup, float radius, osg::Vec4 color = osg::Vec4( 1,0,0,1 ), 
			osg::Vec4 fluxColor = osg::Vec4( 1,1,1,1 ), bool fluxUp = true, float fluxSpeed = 20.0f, 
			int fluxStep = 8, int numRadialVertices = 10, float lineWidth = 4.0f );

	static void disableFlux( osg::Group* lod )
	{
		lod->getOrCreateStateSet()->removeUniform( "TimeUpdate" );
		osg::Vec4 colorVec;
		lod->getOrCreateStateSet()->getUniform( "color" )->get( colorVec );
		lod->getOrCreateStateSet()->getUniform( "fluxColor" )->set( colorVec );
	}

	static void enableOrChangeFlux( osg::Group* lod, bool fluxUp, float fluxSpeed, int fluxStep, ::osg::Vec4 color )
	{
		osg::StateSet* lodSS = lod->getOrCreateStateSet();

		osg::Uniform* oldUniform = lodSS->getUniform( "TimeUpdate" );
		if( oldUniform )
			lodSS->removeUniform( "TimeUpdate" );

		osg::Uniform* timeUpdateUniform = new osg::Uniform( "TimeUpdate", 2.0f );
		timeUpdateUniform->setUpdateCallback( new TimeUpdate( fluxUp, fluxStep, fluxSpeed ) );
		lodSS->addUniform( timeUpdateUniform );
		lodSS->getUniform( "fluxColor" )->set( color );
		lodSS->getUniform( "fluxStep" )->set( static_cast<float>( fluxStep ) );
	}

	osg::Geometry* makeCylinderGeometry( double radius, osg::Vec4 color, int numRadialVertices = 10 );

	osg::Geometry* makeLineGeometry( osg::Vec4 color, float lineWidth );


private:

	void createShaderStuff();

	struct Section
	{
		osg::Vec3 position;
		osg::Vec3 normal;
		osg::Vec3 binormal;
	}; 

	std::vector<Section> _sections;

	osg::ref_ptr<osg::Program> _cylProgram;
	osg::ref_ptr<osg::Program> _lineProgram;

	osg::ref_ptr<osg::Shader>  _cylVertObj;
	osg::ref_ptr<osg::Shader>  _lineVertObj;

	osg::ref_ptr<osg::Shader>  _cylFragObj;
	osg::ref_ptr<osg::Shader>  _lineFragObj;

	osg::ref_ptr<osg::Shader>  _cylEvalObj;
	osg::ref_ptr<osg::Shader>  _cylControlObj;

private:
	/*!
	\brief This class is a builder for the every possible point of a streamline. It calculates the necessary
	data for the streamline rendering algorithm: tangent, normal and binormal. It also calculates how
	colinear the point is if compared to adjascent points. This colinearity is used to decide if the
	point will be used of culled from the final streamline geometry
	*/
	class TubeSectionBuilder
	{
	public:

		//! Needs the point position and adjascent point position to build all the necessary data
		TubeSectionBuilder( osg::Vec3 pointPosition, osg::Vec3 nextPointPosition )
		{
			_section.position = pointPosition;
			_nextPointPosition = nextPointPosition;
		}

		static TubeSectionBuilder getFirstPoint( osg::Vec3 firstPointPosition,
													osg::Vec3 secondPointPosition )
		{
			TubeSectionBuilder firstPoint = TubeSectionBuilder( firstPointPosition, secondPointPosition );
			firstPoint.buildFirstPoint();
			return firstPoint;
		}

		/*!  Calculates the tangent normal and binormal of a point. Also	checks if the point
			does not deviate from the previous points line. In case it does not deviate,
			returns false, as the point is a mere repetition of the previous one 
			with a different position, and should not be included in the streamline */
		bool buildNewPoint( TubeSectionBuilder& previousPoint, float curveTolerance = 0.0001f )
		{
			osg::Vec3 equalNextPoint( _section.position - _nextPointPosition ); equalNextPoint.normalize();
			osg::Vec3 equalPrevPoint( _section.position - previousPoint.getSection().position ); equalPrevPoint.normalize();
			_tangent = equalNextPoint - equalPrevPoint;
			if( _tangent.length2() == 0 )
			{
				return false;
			}
			_tangent.normalize();

			osg::Vec3 rotationAxis = previousPoint.getTangent() ^ _tangent;
			if( rotationAxis.length2() < curveTolerance )
			{
				_section.normal = previousPoint.getNormal();
				_section.binormal = previousPoint.getBinormal();
				return false;
			}
			else
			{
				rotationAxis.normalize();
				float rotationAngle = acos( previousPoint.getTangent() * _tangent );

				osg::Matrix rotMat = osg::Matrix::rotate( rotationAngle, rotationAxis );

				_section.normal = previousPoint.getNormal() * rotMat; 
				_section.normal.normalize();

				_section.binormal =  previousPoint.getBinormal() * rotMat;
				_section.binormal.normalize();

				return true;
			}
		}

		const Section& getSection() { return _section; }
		const osg::Vec3 getNormal() { return _section.normal; }
		const osg::Vec3 getBinormal() { return _section.binormal; }
		const osg::Vec3 getPosition() { return _section.position; }
		const osg::Vec3 getTangent() { return _tangent; }

	private:
		void buildFirstPoint()
		{
			_tangent = _section.position - _nextPointPosition;
			assert( _tangent.length2() > 0 );

			_tangent.normalize();	
			_section.normal = osg::Vec3( -_tangent[1] + _tangent[2], _tangent[0] + _tangent[2], -_tangent[0] - _tangent[1] );
			_section.normal.normalize();
			_section.binormal = _tangent ^ _section.normal;
		}
	private:
		Section _section;
		osg::Vec3 _tangent;
		osg::Vec3 _nextPointPosition;
	};
};

#endif