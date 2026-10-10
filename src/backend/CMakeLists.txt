# src/backend/CMakeLists.txt
# ===
# Módulo backend: lógica de optimización y procesamiento.
# Aquí vive todo lo relacionado con OR-Tools: modelos CP-SAT,
# algoritmos de asignación de horarios, restricciones, etc.
#
# API pública → include/backend/ (visible para otros módulos)
# Implementación → src/ (privada del módulo)
#
# Dependencias externas:
#   - ortools::ortools  → optimización (CP-SAT, routing, etc.)

add_library(backend STATIC)

target_sources(backend PRIVATE
				# Data structs
				src/data/aula.cpp
				src/data/franja_horaria.cpp
				src/data/horario.cpp
				src/data/materia.cpp
				src/data/plan_estudio.cpp
				src/data/profesor.cpp
				src/data/horario_salida.cpp

				# Database
				src/database/DatabaseManager.cpp
				src/database/migracion.cpp
				src/database/version_esquema.cpp
				src/database/respaldo.cpp
				src/database/apertura_base_datos.cpp

				# Services - Issue #23 (Aulas)
				src/services/ServicioAula.cpp

				# Services - Issue #21 (Materias - usando tablas existentes)
				src/services/ServicioMaterias.cpp
				src/services/ServicioPlanesEstudio.cpp
				src/services/ServicioCargaHoraria.cpp
				src/services/ServicioComposicionPlan.cpp
				src/services/ServicioExportacion.cpp
				src/services/ServicioHorarioSalida.cpp
				src/services/ServicioConsultaHorario.cpp
				src/services/ServicioProfesor.cpp

				# Services - Nucleo de datos (Cursos, Turnos/Recesos y fachada)
				src/services/ServicioCursos.cpp
				src/services/ServicioTurnosRecesos.cpp
				src/services/NucleoDatos.cpp
				src/services/GestorPendientes.cpp
				src/services/cascada.cpp

				# Configuracion del solver (JSON, sin OR-Tools)
				src/solver/config/solver_config.cpp
				src/services/CargadorConfiguracionSolver.cpp
				src/services/ConstructorEntradaSolver.cpp
				src/services/ValidadorEntradaSolver.cpp
				src/services/ValidadorSalidaSolver.cpp
				src/services/ServicioGeneracion.cpp
				include/backend/services/ServicioGeneracion.hpp
)

target_include_directories(backend PUBLIC
				${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(backend PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(backend PUBLIC
				Qt6::Core
				Qt6::Sql
)

set_project_warnings(backend)

# `ServicioGeneracion` es un QObject (señal `finalizada`): necesita moc.
set_target_properties(backend PROPERTIES AUTOMOC ON)

# -- backend_solver: motor CP-SAT --
add_library(backend_solver STATIC)

target_sources(backend_solver PRIVATE
				src/solver/variables.cpp
				src/solver/solver.cpp
				src/solver/servicio_solver.cpp
				src/solver/restricciones/restricciones_turnos.cpp
				src/solver/restricciones/restricciones_aulas.cpp
				src/solver/restricciones/restricciones_profesores.cpp
)

target_include_directories(backend_solver PUBLIC
				${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(backend_solver PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(backend_solver PUBLIC
				ortools::ortools
				backend
)

set_project_warnings(backend_solver)
