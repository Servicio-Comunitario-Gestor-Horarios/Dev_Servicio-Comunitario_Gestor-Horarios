# test/CMakeLists.txt
# ===
# Tests del proyecto. Soporta QTest (tests Qt nativos) y Google Test
# (tests de lógica general con mocks).
#
# Flujo:
#   cmake --build build
#   ctest --test-dir build
#
# Para ejecutar un test específico:
#   ctest --test-dir build -R franja        # tests que contengan "franja"
#   ctest --test-dir build -R backend       # todos los tests de backend

enable_testing()

# ── Google Test (opcional) ──
find_package(GTest QUIET)
if(GTest_FOUND)
    message(STATUS "Google Test found — GTest tests enabled")
else()
    message(STATUS "Google Test NOT found — install libgtest-dev to enable")
endif()

# ── Helper: registrar un test con QTest ──
function(add_qtest NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    set_target_properties(${NAME} PROPERTIES AUTOMOC ON)
    target_link_libraries(${NAME} PRIVATE
        ${ARGN}
        Qt6::Test
        Qt6::Core
    )
    add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

# ── Helper: registrar un test con Google Test ──
function(add_gtest NAME SOURCE)
    if(NOT GTest_FOUND)
        message(WARNING "Skipping ${NAME} — Google Test not available")
        return()
    endif()
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE
        ${ARGN}
        GTest::GTest
        GTest::Main
    )
    add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

# ============================================================
# Tests de backend — datos (serialización con QTest)
# ============================================================

add_qtest(test_backend_franja_horaria
    backend/test_franja_horaria.cpp
    backend
)

add_qtest(test_backend_aula
    backend/test_aula.cpp
    backend
)

add_qtest(test_backend_materia
    backend/test_materia.cpp
    backend
)

add_qtest(test_backend_profesor
    backend/test_profesor.cpp
    backend
)

add_qtest(test_backend_horario
    backend/test_horario.cpp
    backend
)

add_qtest(test_backend_plan_estudio
    backend/test_plan_estudio.cpp
    backend
)

# ============================================================
# Tests de backend — lógica (Google Test, cuando existan)
# ============================================================
# add_gtest(test_backend_solver test/backend/test_solver.cpp backend)

enable_testing()

add_executable(

    test_backend_database

    test/backend/test_database.cpp

)

target_link_libraries(

    test_backend_database

    PRIVATE

    backend

    common

    Qt6::Sql

)

add_test(

    NAME backend/database

    COMMAND test_backend_database

)