add_library(middleware STATIC
    internalserver.cpp
    internalserver.h
    messages.h
)

set_target_properties(middleware PROPERTIES AUTOMOC ON)

target_include_directories(middleware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(middleware PUBLIC
    Qt6::Core
    Qt6::Network
)

set_project_warnings(middleware)
