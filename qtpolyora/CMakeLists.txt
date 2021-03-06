
SET(EXECUTABLE qtpolyora)

INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(QTKPT_MOC_FILES glbox.h vsview.h)
ADD_EXECUTABLE(${EXECUTABLE} glbox.cpp glbox.h ipltexture.cpp
	ipltexture.h main.cpp vsview.cpp vsview.h ${QTKPT_MOC_FILES} )

ADD_DEPENDENCIES(${EXECUTABLE} polyora)

IF (WIN32)
	ADD_DEPENDENCIES(${EXECUTABLE} videosource)
ENDIF(WIN32)

INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} ${VIDEOSOURCE_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${EXECUTABLE} polyora
	${VIDEOSOURCE_LIBRARIES}
	${OpenCV_LIBS}
	${QT_LIBRARIES}
	${OPENGL_gl_LIBRARY}
	)

INSTALL(TARGETS qtpolyora RUNTIME DESTINATION bin COMPONENT tools)

# Windows way of installing things is to include DLLs in packages...
IF(WIN32)
FILE(GLOB OpenCV_DLLS "${OpenCV_DIR}/bin/*.dll")
INSTALL(FILES ${OpenCV_DLLS} DESTINATION bin COMPONENT tools)
INSTALL(FILES
        ${QT_BINARY_DIR}/QtOpenGL4.dll
        ${QT_BINARY_DIR}/QtGui4.dll 
        ${QT_BINARY_DIR}/QtCore4.dll
        ${QT_BINARY_DIR}/QtScript4.dll
        ${QT_BINARY_DIR}/QtOpenGLd4.dll
        ${QT_BINARY_DIR}/QtGuid4.dll 
        ${QT_BINARY_DIR}/QtCored4.dll
        ${QT_BINARY_DIR}/QtScriptd4.dll
        DESTINATION bin COMPONENT tools)
ENDIF(WIN32) 
