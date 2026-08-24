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

add_qtest(test_backend_database
    backend/test_database.cpp
    backend
    Qt6::Sql
)

add_qtest(test_backend_entity_graph
    backend/test_entity_graph.cpp
    backend
)

# ============================================================
# Tests de middleware — health-check IPC
# ============================================================

add_qtest(test_middleware_health_check
    test_health_check.cpp
    middleware
    Qt6::Network
)

# ============================================================
# Tests de backend — lógica (Google Test, cuando existan)
# ============================================================
add_gtest(test_backend_variables_cp_sat
    backend/test_variables_cp_sat.cpp
    backend
)



# Tests para servicios del backend
add_qtest(test_servicio_aula
    backend/test_servicio_aula.cpp
    backend
    Qt6::Sql
)

add_qtest(test_servicio_materias
    backend/test_servicio_materias.cpp
    backend
    Qt6::Sql
)

add_qtest(test_servicio_planes_estudio
    backend/test_servicio_planes_estudio.cpp
    backend
    Qt6::Sql
)

add_qtest(test_servicio_carga_horaria
    backend/test_servicio_carga_horaria.cpp
    backend
    Qt6::Sql
)

add_qtest(test_composicion_plan
    backend/test_composicion_plan.cpp
    backend
    Qt6::Sql
)

# ============================================================
# Tests de backend — edge cases
# ============================================================

add_qtest(test_edge_cases
    backend/test_edge_cases.cpp
    backend
)

# ============================================================
# Tests de backend — solver CP-SAT
# ============================================================

add_qtest(test_solver_horarios
    backend/test_solver_horarios.cpp
    backend
)

add_qtest(test_solver_benchmark
    backend/test_solver_benchmark.cpp
    backend
)

# ============================================================
# Tests de middleware — rutas CRUD IPC
# ============================================================

add_qtest(test_middleware_crud_routes
    test_crud_routes.cpp
    middleware
    Qt6::Network
)

# ============================================================
# Tests de backend — exportación (Google Test)
# ============================================================
add_gtest(test_servicio_exportacion
    backend/test_servicio_exportacion.cpp
    backend
)

# ============================================================
# Tests de backend — salidas y consultas de horario
# ============================================================

add_qtest(test_horario_salida
    backend/test_horario_salida.cpp
    backend
)

add_qtest(test_servicio_consulta_horario
    backend/test_servicio_consulta_horario.cpp
    backend
    Qt6::Sql
)

add_qtest(test_servicio_horario_salida
    backend/test_servicio_horario_salida.cpp
    backend
    Qt6::Sql
)
