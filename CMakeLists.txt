cmake_minimum_required(VERSION 3.0)
project(RTSPReceiverTest)

#----------------------------------------------------------------------
# On Aqueti systems, required header files will be in /usr/local/aqueti/include
# and static libraries will be in /usr/local/aqueti/lib on non-Windows systems
# and in folders under C:/Program Files/Aqueti on Windows.


#----------------------------------------------------------------------
# Packages required to build.
find_package(Threads REQUIRED)

#----------------------------------------------------------------------
# Set CMake and compiler flags.

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_CFLAGS} -O3 -std=c++11 -rdynamic")

#----------------------------------------------------------------------
# On Windows, we need to explicitly export functions.  This tells the
# code that we're in this case

#add_definitions(-D_AQTDUMPLOG)

#----------------------------------------------------------------------
# Header files
set(HEADER_FILES 
        Base64.h
		StringParse.h
		RTPParse.h
		RTSPParse.h
		RTSPReceiver.h
   )

#----------------------------------------------------------------------
# Source files
set(SOURCE_FILES 
	StringParse.cpp
	RTPParse.cpp
	RTSPParse.cpp
	RTSPReceiver.cpp
	RTSPReceiverTest.cpp
)

#----------------------------------------------------------------------
# Construct the executable.
add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${CMAKE_PROJECT_NAME}
       ${CMAKE_THREAD_LIBS_INIT}
       AQT
)



