# test/CMakeLists.txt
# ===
# Tests del proyecto. La estructura espeja src/ para facilitar
# la correspondencia entre código y pruebas.
#
# Cada test se registra con add_test() y se ejecuta con ctest.
#
# Flujo:
#   cmake --build build
#   ctest --test-dir build
#
# ¿Cómo agregar un test?
#   1. Crear el archivo en el directorio matching de test/
#      (ej: test/backend/test_schedule_model.cpp)
#   2. Agregar acá:
#        add_executable(test_backend_schedule test/backend/test_schedule_model.cpp)
#        target_link_libraries(test_backend_schedule PRIVATE backend common)
#        add_test(NAME backend/schedule COMMAND test_backend_schedule)

enable_testing()

# TODO: Agregar tests acá
#
# Backend:
#   add_executable(test_backend_data test/backend/test_data_models.cpp)
#   target_link_libraries(test_backend_data PRIVATE backend common)
#   add_test(NAME backend/data COMMAND test_backend_data)
#
# Middleware:
#   add_executable(test_middleware_routing test/middleware/test_routing.cpp)
#   target_link_libraries(test_middleware_routing PRIVATE middleware common)
#   add_test(NAME middleware/routing COMMAND test_middleware_routing)
#
# Frontend:
#   add_executable(test_frontend_login test/frontend/test_login_dialog.cpp)
#   target_link_libraries(test_frontend_login PRIVATE frontend common)
#   add_test(NAME frontend/login COMMAND test_frontend_login)
