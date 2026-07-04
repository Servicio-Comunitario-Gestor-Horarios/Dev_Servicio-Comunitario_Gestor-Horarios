cmake_minimum_required(VERSION 3.16)
project(GestorHorariosMiddleware LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(CMAKE_AUTOMOC ON)

# Componente Común (Cabeceras compartidas)
add_library(middleware_common INTERFACE)
target_include_directories(middleware_common INTERFACE common/)

# Servidor IPC + Archivos de Prueba del Cliente (Middleware Server)
add_executable(middleware_server
    server/main.cpp
    server/internalserver.cpp
    server/internalserver.h
    client/internalclient.cpp
    client/internalclient.h
)
target_link_libraries(middleware_server PRIVATE Qt6::Core Qt6::Network middleware_common)

# Cliente IPC (Librería estática por si la necesita el Frontend)
add_library(middleware_client STATIC client/internalclient.cpp client/internalclient.h)
target_link_libraries(middleware_client PRIVATE Qt6::Core Qt6::Network middleware_common)