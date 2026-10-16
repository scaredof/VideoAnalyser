cmake_minimum_required(VERSION 3.16.0)
project(Analyzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread ")

set(CMAKE_BUILD_TYPE DEBUG)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(ARCH_X86_64 "x86_64")
set(ARCH_AARCH64 "aarch64")
message(STATUS "System architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# third lib start
include_directories(/usr/local/include)

link_directories(/usr/local/lib})
link_directories(/usr/local/lib64})
link_directories(/usr/local/lib64})
include_directories(/usr/local/ffmpeg/include) #ffmpeg
link_directories(/usr/local/ffmpeg/lib) #ffmpeg
find_package(OpenCV REQUIRED) #opencv

include_directories(/usr/local/onnxruntime/include) #onnxruntime
link_directories(/usr/local/onnxruntime/lib) #onnxruntime	

include_directories(/usr/local/openvino/3rdparty/tbb/include) #openvino-tbb
link_directories(/usr/local/openvino/3rdparty/tbb/lib) #openvino-tbb

include_directories(/usr/local/openvino/include) #openvino
if(${ARCH_X86_64} STREQUAL ${CMAKE_SYSTEM_PROCESSOR})
  link_directories(/usr/local/openvino/lib/intel64) #openvino(x86_64)
elseif(${ARCH_AARCH64} STREQUAL ${CMAKE_SYSTEM_PROCESSOR})
  link_directories(/usr/local/openvino/lib/aarch64) #openvino(aarch64)
endif()
	
# third lib end

set(source
        Analyzer/Core/Utils/CalcuIOU.cpp
        Analyzer/Core/Utils/Request.cpp
        Analyzer/Core/Algorithm.cpp
        Analyzer/Core/AlgorithmOnYolo.cpp
        Analyzer/Core/AlgorithmOvYolo.cpp
        Analyzer/Core/Analyzer.cpp
        Analyzer/Core/AvPullStream.cpp
        Analyzer/Core/AvPushStream.cpp
        Analyzer/Core/Config.cpp
        Analyzer/Core/Frame.cpp
        Analyzer/Core/GenerateAlarmVideo.cpp
        Analyzer/Core/Scheduler.cpp
        Analyzer/Core/Server.cpp
        Analyzer/Core/Worker.cpp
        Analyzer/main.cpp)

add_executable(Analyzer ${source})
target_link_libraries(Analyzer ${OpenCV_LIBS})
target_link_libraries(Analyzer event curl jsoncpp avformat avcodec avutil swscale swresample pthread)
target_link_libraries(Analyzer onnxruntime) #onnxruntime
if(${ARCH_X86_64} STREQUAL ${CMAKE_SYSTEM_PROCESSOR})
  target_link_libraries(Analyzer tbb openvino openvino_intel_cpu_plugin openvino_intel_gpu_plugin) #openvino(x86_64)
elseif(${ARCH_AARCH64} STREQUAL ${CMAKE_SYSTEM_PROCESSOR})
  target_link_libraries(Analyzer tbb openvino openvino_arm_cpu_plugin) #openvino(aarch64)
endif()
