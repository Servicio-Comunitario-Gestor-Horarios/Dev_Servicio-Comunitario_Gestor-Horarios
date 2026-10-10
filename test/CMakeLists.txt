## test/CMakeLists.txt

find_package(GTest QUIET)
if(GTest_FOUND)
	message(STATUS "Google Test found — GTest tests enabled")
else()
	message(STATUS "Google Test NOT found — install libgtest-dev to enable")
endif()

function(add_qtest NAME SOURCE)
	add_executable(${NAME} ${SOURCE})
	set_target_properties(${NAME} PROPERTIES AUTOMOC ON)
	target_link_libraries(${NAME} PRIVATE ${ARGN} Qt6::Test Qt6::Core)
	add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

function(add_gtest NAME SOURCE)
	if(NOT GTest_FOUND)
		message(WARNING "Skipping ${NAME} — Google Test not available")
		return()
	endif()
	add_executable(${NAME} ${SOURCE})
	target_link_libraries(${NAME} PRIVATE ${ARGN} GTest::GTest GTest::Main)
	add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

# ============================================================
# Middleware (siempre; no depende de OR-Tools)
# ============================================================
add_qtest(test_middleware_health_check
				test_health_check.cpp
				middleware Qt6::Network
)

add_qtest(test_middleware_transport
				test_middleware_transport.cpp
				middleware Qt6::Network
)

# ============================================================
# App (arranque e instancia única) — no depende de OR-Tools
# ============================================================
# `app_core` es la librería de arranque sin UI donde vive `instancia_unica`,
# de modo que el ejecutable y el test enlazan la misma implementación.
add_qtest(test_instancia_unica test_instancia_unica.cpp app_core Qt6::Network)

# ============================================================
# Backend (persistencia) — requiere el target `backend`
# ============================================================
if(TARGET backend)
	add_qtest(test_backend_franja_horaria backend/test_franja_horaria.cpp backend)
	add_qtest(test_backend_aula backend/test_aula.cpp backend)
	add_qtest(test_backend_materia backend/test_materia.cpp backend)
	add_qtest(test_backend_profesor backend/test_profesor.cpp backend)
	add_qtest(test_backend_horario backend/test_horario.cpp backend)
	add_qtest(test_backend_plan_estudio backend/test_plan_estudio.cpp backend)
	add_qtest(test_backend_database backend/test_database.cpp backend Qt6::Sql)
	add_qtest(test_backend_version_esquema backend/test_version_esquema.cpp backend Qt6::Sql)
	add_qtest(test_backend_respaldo backend/test_respaldo.cpp backend Qt6::Sql)
	add_qtest(test_backend_apertura_base_datos backend/test_apertura_base_datos.cpp backend Qt6::Sql)
	add_qtest(test_backend_nucleo_datos backend/test_nucleo_datos.cpp backend Qt6::Sql)
	add_qtest(test_backend_cascada backend/test_cascada.cpp backend Qt6::Sql)
	add_qtest(test_backend_gestor_pendientes backend/test_gestor_pendientes.cpp backend Qt6::Sql)
	add_qtest(test_backend_contrato_interfaz backend/test_contrato_interfaz.cpp backend Qt6::Sql)
	add_qtest(test_backend_entity_graph backend/test_entity_graph.cpp backend)
	add_qtest(test_cargador_configuracion backend/test_cargador_configuracion.cpp backend)
	add_qtest(test_constructor_entrada backend/test_constructor_entrada.cpp backend)
	add_qtest(test_validador_entrada backend/test_validador_entrada.cpp backend)
	add_qtest(test_validador_salida backend/test_validador_salida.cpp backend)
	add_qtest(test_servicio_generacion backend/test_servicio_generacion.cpp backend)

	add_qtest(test_servicio_aula backend/test_servicio_aula.cpp backend Qt6::Sql)
	add_qtest(test_servicio_materias backend/test_servicio_materias.cpp backend Qt6::Sql)
	add_qtest(test_servicio_planes_estudio backend/test_servicio_planes_estudio.cpp backend Qt6::Sql)
	add_qtest(test_servicio_carga_horaria backend/test_servicio_carga_horaria.cpp backend Qt6::Sql)
	add_qtest(test_composicion_plan backend/test_composicion_plan.cpp backend Qt6::Sql)
	add_qtest(test_edge_cases backend/test_edge_cases.cpp backend)
	add_qtest(test_horario_salida backend/test_horario_salida.cpp backend)
	add_qtest(test_servicio_consulta_horario backend/test_servicio_consulta_horario.cpp backend Qt6::Sql)
	add_qtest(test_servicio_horario_salida backend/test_servicio_horario_salida.cpp backend Qt6::Sql)
	add_qtest(test_archivos_horario backend/test_archivos_horario.cpp backend)
	add_qtest(test_contrato_generacion backend/test_contrato_generacion.cpp backend)
	add_qtest(test_servicio_profesor backend/test_servicio_profesor.cpp backend Qt6::Sql)

	add_gtest(test_servicio_exportacion backend/test_servicio_exportacion.cpp backend)
endif()

# ============================================================
# Datos (ciclo de vida de la base del cliente) — requiere `datos`
# ============================================================
if(TARGET datos)
	add_qtest(test_datos_contexto_base_datos datos/test_contexto_base_datos.cpp datos Qt6::Sql)
endif()

# ============================================================
# Solver CP-SAT — requiere el target `backend_solver`
# ============================================================
if(TARGET backend_solver)
	add_gtest(test_backend_variables_cp_sat backend/test_variables_cp_sat.cpp backend_solver)
	add_qtest(test_solver_horarios backend/test_solver_horarios.cpp backend_solver)
	add_qtest(test_solver_benchmark backend/test_solver_benchmark.cpp backend_solver)
	add_qtest(test_ipc_aislamiento_bd test_ipc_aislamiento_bd.cpp backend_solver middleware Qt6::Sql)
endif()
