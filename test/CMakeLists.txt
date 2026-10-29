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
	add_qtest(test_esquema_cursos backend/test_esquema_cursos.cpp backend Qt6::Sql)
	add_qtest(test_backend_respaldo backend/test_respaldo.cpp backend Qt6::Sql)
	add_qtest(test_backend_apertura_base_datos backend/test_apertura_base_datos.cpp backend Qt6::Sql)
	add_qtest(test_backend_nucleo_datos backend/test_nucleo_datos.cpp backend Qt6::Sql)
	add_qtest(test_backend_cascada backend/test_cascada.cpp backend Qt6::Sql)
	add_qtest(test_backend_gestor_pendientes backend/test_gestor_pendientes.cpp backend Qt6::Sql)
	add_qtest(test_backend_contrato_interfaz backend/test_contrato_interfaz.cpp backend Qt6::Sql)
	add_qtest(test_backend_entity_graph backend/test_entity_graph.cpp backend)
	add_qtest(test_campos_curso backend/test_campos_curso.cpp backend Qt6::Sql)
	add_qtest(test_cargador_configuracion backend/test_cargador_configuracion.cpp backend)
	add_qtest(test_constructor_entrada backend/test_constructor_entrada.cpp backend)
	add_qtest(test_validador_entrada backend/test_validador_entrada.cpp backend)
	add_qtest(test_validador_salida backend/test_validador_salida.cpp backend)
	add_qtest(test_servicio_generacion backend/test_servicio_generacion.cpp backend)

	# Eventos del turno (los antiguos recesos): CRUD por identidad y derivación.
	add_qtest(test_servicio_turnos_eventos backend/test_servicio_turnos_eventos.cpp backend Qt6::Sql)

	# Funciones puras de eventos: ventana de posición, reparto y solapes.
	add_qtest(test_calculo_eventos backend/test_calculo_eventos.cpp backend)

	add_qtest(test_servicio_aula backend/test_servicio_aula.cpp backend Qt6::Sql)
	add_qtest(test_servicio_materias backend/test_servicio_materias.cpp backend Qt6::Sql)
	add_qtest(test_servicio_tipos_aula backend/test_servicio_tipos_aula.cpp backend Qt6::Sql)
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

	add_qtest(test_contrato_solver backend/test_contrato_solver.cpp backend)

	# Funciones puras de demanda y capacidad por tipo (FranjaSemana, T11).
	add_gtest(test_capacidad_tipo backend/test_capacidad_tipo.cpp backend)

	# Punto único de duración de la hora académica (rango, minutos y encaje, 011 T1).
	add_gtest(test_duracion_horaria backend/test_duracion_horaria.cpp backend)

	# Configuración global de la hora académica: valor efectivo y edición (011 T3).
	add_qtest(test_servicio_configuracion_horaria
				backend/test_servicio_configuracion_horaria.cpp
				backend Qt6::Sql
	)

	# Modelo puro de preferencias y validación compartida de franjas (012 T1).
	add_gtest(test_preferencias backend/test_preferencias.cpp backend)

	# Servicio de preferencias horarias del docente (012 T3).
	add_qtest(test_servicio_preferencias
				backend/test_servicio_preferencias.cpp
				backend Qt6::Sql
	)
endif()

# ============================================================
# Datos (ciclo de vida de la base del cliente) — requiere `datos`
# ============================================================
if(TARGET datos)
	add_qtest(test_datos_contexto_base_datos datos/test_contexto_base_datos.cpp datos Qt6::Sql)
endif()

# ============================================================
# Seed (datos de prueba del centro) — requiere el target `seed`
# ============================================================
if(TARGET seed)
	add_qtest(test_seed_perfil backend/test_seed_perfil.cpp seed)
	add_qtest(test_seed_aplicacion backend/test_seed_aplicacion.cpp seed Qt6::Sql)
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

# ============================================================
# Frontend (composición/uso del contrato del solver) — requiere `frontend` + `backend`
# ============================================================
if(TARGET frontend AND TARGET backend)
	# El conversor HorarioSalida -> visualización es lógica pura del frontend; se
	# prueba aquí porque las cabeceras viven bajo `src/frontend/src` (privadas).
	add_qtest(test_frontend_conversion frontend/test_conversion_entrada.cpp frontend)
	target_include_directories(test_frontend_conversion PRIVATE
					${CMAKE_SOURCE_DIR}/src/frontend/src)

	# Integración: composición de la entrada + solver con los datos sembrados.
	if(TARGET backend_solver AND TARGET seed)
		add_qtest(test_generacion_seed frontend/test_generacion_seed.cpp
						frontend backend_solver seed Qt6::Sql)
		target_include_directories(test_generacion_seed PRIVATE
						${CMAKE_SOURCE_DIR}/src/frontend/src)
	endif()
endif()

# ============================================================
# Serialización de los tests del socket IPC
# ============================================================
# Los tests que levantan `InternalServer` comparten el nombre fijo del socket IPC
# (middleware/messages.h): no pueden correr a la vez o uno borra el socket del otro
# (`QLocalServer::removeServer` antes de `listen`). Se serializan con un RESOURCE_LOCK
# de CTest; `test_instancia_unica` no entra porque usa un nombre único (nombreUnico()).
set_tests_properties(
	test_middleware_health_check
	test_middleware_transport
	test_ipc_aislamiento_bd
	PROPERTIES RESOURCE_LOCK servidor_ipc_middleware
)
