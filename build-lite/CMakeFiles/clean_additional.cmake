# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "src/frontend/CMakeFiles/frontend_autogen.dir/AutogenUsed.txt"
  "src/frontend/CMakeFiles/frontend_autogen.dir/ParseCache.txt"
  "src/frontend/frontend_autogen"
  "src/middleware/CMakeFiles/middleware_autogen.dir/AutogenUsed.txt"
  "src/middleware/CMakeFiles/middleware_autogen.dir/ParseCache.txt"
  "src/middleware/middleware_autogen"
  )
endif()
