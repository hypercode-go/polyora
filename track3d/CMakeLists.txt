
SET(EXECUTABLE track3d)

INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(QTKPT_MOC_FILES glbox.h track3d_view.h)
ADD_EXECUTABLE(${EXECUTABLE}
               glbox.cpp glbox.h
               ipltexture.cpp ipltexture.h
               main.cpp
               track3d_view.cpp track3d_view.h
               ${QTKPT_MOC_FILES} )

ADD_DEPENDENCIES(${EXECUTABLE} polyora)

IF (WIN32)
	ADD_DEPENDENCIES(${EXECUTABLE} videosource)
ENDIF(WIN32)
	
INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} ${VIDEOSOURCE_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${EXECUTABLE} script polyora videosource
	${VIDEOSOURCE_LIBRARIES}
	${OpenCV_LIBS}
	${QT_LIBRARIES}
	${OPENGL_gl_LIBRARY}
	)

INSTALL(TARGETS track3d RUNTIME DESTINATION bin COMPONENT tools)
