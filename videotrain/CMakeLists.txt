SET(EXECUTABLE videotrain)
ADD_EXECUTABLE(${EXECUTABLE} videotrain.cpp)

INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
TARGET_LINK_LIBRARIES(${EXECUTABLE} videosource polyora ${OpenCV_LIBS} )
