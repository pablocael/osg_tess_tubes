project( osg_tess_tubes )

FILE( GLOB SHADERS ${CMAKE_SOURCE_DIR}/shaders/*.glsl )

SOURCE_GROUP( "shaders" FILES ${SHADERS} )

find_package( OpenGL REQUIRED )
find_package( OpenSceneGraph 3.0.1 REQUIRED osgViewer osgGA osgDB )
 
include_directories(
	${OPENSCENEGRAPH_INCLUDE_DIRS}
)

file( GLOB _SOURCE_FILES *.cpp )
file( GLOB _HEADER_FILES *.h )

add_executable( osg_tess_tubes ${_HEADER_FILES} ${_SOURCE_FILES} ${SHADERS} )

target_link_libraries( osg_tess_tubes ${OPENSCENEGRAPH_LIBRARIES} ${OPENGL_LIBRARIES} )
