# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/appchat_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/appchat_autogen.dir/ParseCache.txt"
  "appchat_autogen"
  )
endif()
