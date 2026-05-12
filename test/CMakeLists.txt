# test/CMakeLists.txt
# ===
# Configuración de tests con CTest.
# Cada test se registra con add_test() y se ejecuta con ctest.
#
# Flujo:
#   cmake --build build
#   ctest --test-dir build
#
# ¿Cómo agregar un test?
#   1. Crear test/<nombre>/test_<cosa>.cpp
#   2. Agregar acá:
#        add_executable(test_<cosa> test/<nombre>/test_<cosa>.cpp)
#        target_link_libraries(test_<cosa> PRIVATE <modulo>)
#        add_test(NAME test_<cosa> COMMAND test_<cosa>)
#
# Podés usar cualquier framework de testing (Catch2, Google Test,
# doctest). Cuando decidas cuál, agregalo como dependencia y
# actualizá los target_link_libraries acá.

enable_testing()

# TODO: Agregar tests acá
# ej:
# add_executable(test_backend test/backend/test_schedule_model.cpp)
# target_link_libraries(test_backend PRIVATE backend)
# add_test(NAME test_backend COMMAND test_backend)
