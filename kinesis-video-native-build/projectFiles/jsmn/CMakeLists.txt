cmake_minimum_required(VERSION 2.8)

macro(FORCE_MSVS_CRT_TYPE)
    if(MSVC)
      set(variables
		CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
      )

      if(${MSVS_CRT_TYPE} STREQUAL "" OR ${MSVS_CRT_TYPE} STREQUAL "static")
        message(STATUS "Using static CRT")
        foreach(variable ${variables})
		message(STATUS "The variables are ${variable} ${${variable}}")
          if(${variable} MATCHES "/MD")
	    if("Debug" STREQUAL "${CMAKE_BUILD_TYPE}" OR "Debug" STREQUAL "${CMAKE_CONFIGURATION_TYPES}")
              string(REGEX REPLACE "/MD" "/MTd" ${variable} "${${variable}}")
            else()
              string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
            endif()
          endif()
      endforeach()

    else()
      message(STATUS "Using dynamic CRT")
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
            string(REGEX REPLACE "/MT" "/MDd" ${variable} "${${variable}}")
          else()
            string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
          endif()
        endif()
      endforeach()
    endif()
  endif()
endmacro()

project(jsmn C)
set(MSVS_CRT_TYPE "static")

add_library(jsmn STATIC jsmn.h jsmn.c)
