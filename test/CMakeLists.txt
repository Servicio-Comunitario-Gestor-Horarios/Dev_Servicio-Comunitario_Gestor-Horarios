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

	# Predicado puro de la fila del tipo general y traducción del rechazo (015 T4).
	add_gtest(test_tipo_general backend/test_tipo_general.cpp backend)

	# Comprobación única de validez de la base (016 T1).
	add_gtest(test_comprobacion_validez backend/test_comprobacion_validez.cpp backend)

	# Funciones puras de la rejilla de bloques de dos horas (014 T2).
	add_gtest(test_bloques backend/test_bloques.cpp backend)

	# Informe de bloques, desvío, huecos y separación (014 T6).
	add_gtest(test_analisis_bloques backend/test_analisis_bloques.cpp backend)

	# Configuración global de la hora académica: valor efectivo y edición (011 T3).
	add_qtest(test_servicio_configuracion_horaria
				backend/test_servicio_configuracion_horaria.cpp
				backend Qt6::Sql
	)

	# Modelo puro de preferencias y validación compartida de franjas (012 T1).
	add_gtest(test_preferencias backend/test_preferencias.cpp backend)

	# Rejilla compartida y proyección reloj → franja de las preferencias (013 T2).
	add_gtest(test_proyeccion_preferencias backend/test_proyeccion_preferencias.cpp backend)

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
	# Fixture de aceptación sin EF (014 T7): datos del centro de 24 cursos.
	add_library(fixture_centro STATIC backend/fixtures/centro_sin_ef.cpp)
	target_link_libraries(fixture_centro PUBLIC backend)
	target_include_directories(fixture_centro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/backend)

	add_gtest(test_backend_variables_cp_sat backend/test_variables_cp_sat.cpp backend_solver)
	add_qtest(test_solver_horarios backend/test_solver_horarios.cpp backend_solver)
	# Emparejamiento en bloques de dos horas: segmento lleno, tramo impar y tamaño (014 T3a).
	add_gtest(test_solver_bloques backend/test_solver_bloques.cpp backend_solver)
	# Aforo simultáneo de la cancha: dos cursos en el mismo par, tres en la misma franja y el
	# par cruzado forzado (018 T2).
	add_gtest(test_solver_aforo backend/test_solver_aforo.cpp backend_solver)
	# Aceptación del centro sin EF (014 T7). Es lento: etiqueta `slow` y fuera de la suite normal.
	add_gtest(test_solver_centro_sin_ef backend/test_solver_centro_sin_ef.cpp
					fixture_centro backend_solver)
	set_tests_properties(test_solver_centro_sin_ef PROPERTIES LABELS "slow")
	add_qtest(test_solver_benchmark backend/test_solver_benchmark.cpp backend_solver)
	# Objetivo con preferencias: referencia, anclaje estructural y deduplicación (013 T5).
	add_gtest(test_solver_preferencias backend/test_solver_preferencias.cpp backend_solver)
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

	# La rejilla dirigida por datos (turnos, recesos y bloques de varias franjas)
	# es lógica pura del modelo de la visualización.
	add_qtest(test_cuadricula_horario frontend/test_cuadricula_horario.cpp frontend)
	target_include_directories(test_cuadricula_horario PRIVATE
					${CMAKE_SOURCE_DIR}/src/frontend/src)

	# Ayuda estricta que resuelve el tipo general (016 T4): falla en vez de devolver otro tipo.
	add_qtest(test_tipos_aula_ui frontend/test_tipos_aula_ui.cpp frontend)
	target_include_directories(test_tipos_aula_ui PRIVATE
					${CMAKE_SOURCE_DIR}/src/frontend/src)

	# Selectores de la vista de generación (años como desplegable múltiple y
	# materias suplentes en rejilla) y los ajustes visuales de sus tablas.
	if(TARGET seed)
		add_qtest(test_selector_generacion frontend/test_selector_generacion.cpp
						frontend seed Qt6::Sql Qt6::Widgets)
		target_include_directories(test_selector_generacion PRIVATE
						${CMAKE_SOURCE_DIR}/src/frontend/src)
	endif()

	# Composición de la entrada del solver con el centro real, sin resolver (017-T4):
	# compone los 24 cursos con plan/turno/matrícula, valida V1–V19 y hace ida y vuelta
	# del preset; no genera. El informe declara el conjunto generable desde la 018.
	if(TARGET seed)
		add_qtest(test_entrada_seed frontend/test_entrada_seed.cpp frontend seed Qt6::Sql)
		target_include_directories(test_entrada_seed PRIVATE
						${CMAKE_SOURCE_DIR}/src/frontend/src)
	endif()

	# Aceptación lenta del centro real con Educación Física (018 T5): siembra, compone la
	# entrada desde la base y resuelve. Etiqueta `slow` y fuera de la suite normal.
	if(TARGET seed AND TARGET backend_solver)
		add_qtest(test_solver_centro_real frontend/test_solver_centro_real.cpp
						frontend seed backend_solver Qt6::Sql)
		target_include_directories(test_solver_centro_real PRIVATE
						${CMAKE_SOURCE_DIR}/src/frontend/src)
		set_tests_properties(test_solver_centro_real PROPERTIES LABELS "slow"
						ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=900000")
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
