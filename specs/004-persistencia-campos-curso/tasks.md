# Tareas 004 — Base de datos y servicios de curso

- Spec: `specs/004-persistencia-campos-curso/spec.md` (**aprobada**)
- Plan: `specs/004-persistencia-campos-curso/plan.md`
- Depende de: `specs/001-base-datos-local` y `specs/003-cierre-001-base-datos` (implementadas; **esta spec anula su versionado/migración**).
- Sucesora: `specs/005-nucleo-solver` (define el modelo del solver, **construye la entrada** desde los datos persistidos y gestiona los presets).
- Interfaz de consumo (otro equipo): `docs/interfaz-frontend.md`. **`src/frontend` no se toca.**
- Comando de tests (contenedor `gestor-dev`, OR-Tools solo en Docker):
  `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"`
- Registro de tests: `test/CMakeLists.txt` con `add_qtest(<nombre> backend/test/<archivo>.cpp backend Qt6::Sql)` (o `add_gtest`).
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.
- **Modo didáctico:** el **agente escribe los tests**; el **usuario escribe el código de producción**; el agente revisa.

> **Nota de secuencia.** **T2–T6 son un único refactor acoplado** del modelo `Cursos`: al reescribir la tabla (T2), los tests que tocan `Cursos` quedan rojos hasta que T3–T6 adaptan servicios, DTO, cascada y huella. La suite de dominio **vuelve a verde al cerrar T6**; a partir de T6 cada tarea es autónoma. La **construcción de la entrada del solver** ya no vive aquí: es de la **spec 005** (ver plan §8).

---

- [ ] **T1. Funciones puras de identidad, nombre y orden de curso.** RF-1, RF-2, RF-3, RF-4
    - Tests (rojo): `test/backend/test_campos_curso.cpp` **(ya creado, en rojo)** — registrado con `add_qtest(test_campos_curso backend/test_campos_curso.cpp backend Qt6::Sql)`; incluye `backend/services/ServicioCursos.hpp`. Cubre `nombreCurso` (1→`1ro`, 2→`2do`, 3→`3ro`, 4/5/6→`to`, 10→`10to`, `"4to B"`, `Ñ`); `normalizarSeccion` (`" a "`→`A`, `"a"`→`A`, `ñ`→`Ñ`, `á`→`Á`); `validarIdentidadCurso` (año 0/-1 rechazado; sección vacía, solo espacios o de 2 caracteres rechazada; `Ñ`/`Á` aceptadas; mensaje con campo **y** valor); `cursoAntes` (años distintos → numérico, mismo año → sección, colación española `A` antes que `Á`, misma identidad → no anterior).
    - Código: declarar **inline** en `src/backend/include/backend/services/ServicioCursos.hpp` el struct `IdentidadCurso` y las funciones `normalizarSeccion`, `validarIdentidadCurso`, `nombreCurso`, `cursoAntes` (esta reutiliza `nombreAntes` de `backend/services/ordenacion.hpp`). Sin estado, sin E/S, sin reloj. **No** se crea `backend/dominio/`.
    - Hecho cuando: `ctest` pasa `test_campos_curso`; para la misma entrada las funciones devuelven el mismo resultado y no dependen de Qt Widgets ni del reloj.

- [ ] **T2. Esquema único y colapso del versionado (una sola creación + «esquema esperado»).** RF-6, RF-1; RNF-1, RNF-3
    - Tests (rojo): `test/backend/test_esquema_cursos.cpp` **(nuevo)** — `Cursos` **no** tiene `nombre`/`num_estudiantes`/`codigo_plan`/`aula_fija`; tiene `anio`, `seccion`, `aula` y sellos; **no existe** `Curso_Materia`; `UNIQUE(anio,seccion)` rechaza un `INSERT` directo duplicado (soslayando el servicio); `CHECK` rechaza año 0 y sección de 2 caracteres; FK rechaza `aula` inexistente. Reescribir `test/backend/test_version_esquema.cpp` (`decidirApertura` → Crear/Abrir/FalloEsquema; `crearEsquema` idempotente; `esquemaEsperado`; sin `aplicarHasta`), `test/backend/test_apertura_base_datos.cpp` (`OkCreada`/`OkAbierta`/`FalloEsquema`/`FalloApertura`; sin migración/respaldo/progreso), `test/backend/test_respaldo.cpp` (usa `crearEsquema`), `test/backend/test_contrato_interfaz.cpp` (estados nuevos de apertura). Registrar los tests nuevos.
    - Código: `migracion.{hpp,cpp}` — `Migracion::aplicarPaso(QSqlDatabase&)` crea **todo** el catálogo de una vez: `Cursos(anio CHECK>=1, seccion CHECK 1 char, turno FK, aula FK→Aulas, sellos, UNIQUE(anio,seccion))`, **sin `Curso_Materia`**. `version_esquema.{hpp,cpp}` — `VERSION_ESQUEMA_ACTUAL=2`, `EstadoApertura{Crear,Abrir,FalloEsquema}`, `decidirApertura` pura, `crearEsquema` (una transacción: `aplicarPaso` + `user_version`), `esquemaEsperado` (catálogo de tablas/columnas + ausencia de `Curso_Materia`); se retiran `PasoMigracion`/`aplicarHasta`. `apertura_base_datos.{hpp,cpp}` — `Estado{OkCreada,OkAbierta,FalloEsquema,FalloApertura}`, `Resultado{estado,detalle}` sin `rutaRespaldo`, `Opciones{versionEsperada,crear,verificar}`; sin respaldo previo. `DatabaseManager::runMigrations` con `crearEsquema`/`esquemaEsperado`.
    - Hecho cuando: `ctest` pasa `test_esquema_cursos`, `test_version_esquema`, `test_apertura_base_datos`, `test_respaldo` y `test_contrato_interfaz`; ninguna rama hace `DROP`, `ALTER`, migración de datos ni respaldo previo (RNF-1).
    - Nota: al cerrar T2, los tests que usan `Cursos`/`CursoDTO` quedan rojos hasta T3–T6 (nota de secuencia).

- [ ] **T3. `CursoDTO` y lectura de cursos (mapeo, consulta y `listar` ordenado; sin materias).** RF-2, RF-4, RF-5
    - Tests (rojo): `test/backend/test_nucleo_datos.cpp` **(modificado)** — el esquema **no** tiene `Curso_Materia`; un curso guardado se devuelve con `anio`, `seccion`, `turno` y `aula`; `listarCursos` queda ordenado por año ascendente y, a igual año, por sección con colación española (acentos/mayúsculas); sin operaciones de materias de curso.
    - Código: `ServicioCursos.hpp` — `CursoDTO { id, anio, seccion, turno, aula }`; **se eliminan** `CursoMateriaDTO` y `materias`; firmas `crearCurso(anio,seccion,turno,aula)`. `ServicioCursos.cpp` — `mapearARecord` lee los campos nuevos; `obtenerCurso`/`listarCursos` (orden con `cursoAntes`); **se retiran** `cargarMaterias`/`asignarMateria`/`quitarMateria`. `NucleoDatos.{hpp,cpp}` — propaga las firmas y **retira** `asignarMateriaACurso`/`quitarMateriaDeCurso`. Adaptar `src/consola` y los call sites de test que construyen `CursoDTO`/llaman a `crearCurso` para que el build compile (la escritura se completa en T4).
    - Hecho cuando: `ctest` pasa la suite de `Cursos` de `test_nucleo_datos`; un curso guardado se devuelve con identidad/turno/aula y la lista queda ordenada por año y sección.

- [ ] **T4. Alta y actualización de cursos (identidad, unicidad y aula referenciada).** RF-1, RF-2, RF-3; RNF-2
    - Tests (rojo): `test/backend/test_nucleo_datos.cpp` — crear con año+sección ya existentes se rechaza con mensaje que identifica el campo y sugiere «elija otro año o sección»; igual al actualizar; actualizar sin cambiar la identidad se acepta (no se auto-rechaza); año 0/-1 y sección vacía/solo espacios/de 2 caracteres se rechazan con mensaje; sección no ASCII aceptada; **registrar/actualizar con un aula inexistente se rechaza con mensaje**; un curso sin aula es válido.
    - Código: `crear`/`actualizar` en `ServicioCursos.cpp` con `validarIdentidadCurso`, pre-chequeo de duplicado año+sección y de **existencia del aula**, escritura de `anio`/`seccion`/`turno`/`aula` y `fecha_modificacion = CURRENT_TIMESTAMP`; defensa ante `UNIQUE` y `FOREIGN KEY` (mensajes en español). Clave de la operación pendiente de alta de curso en `NucleoDatos` = `"<anio>-<seccion>"` normalizado.
    - Hecho cuando: `ctest` pasa; no queda un estado con dos cursos del mismo año y sección (ni por el servicio ni por un `INSERT` directo), el aula referenciada existe siempre, y los rechazos cumplen RNF-2.

- [ ] **T5. Cascadas: retirar curso–materia y plan→cursos; conservar aula/turno→cursos con anulación.** RF-1, RF-5
    - Tests (rojo): `test/backend/test_cascada.cpp` **(modificado)** — `dependenciasDe("plan")` **ya no** arrastra cursos; `dependenciasDe("materia")` y `dependenciasDe("curso")` **ya no** tienen `curso_materia`; `dependenciasDe("aula")`/`("turno")` listan los cursos como **anulación**. `eliminarConCascada("aula", id)` deja los cursos **sin aula** (no los borra); `eliminarConCascada("turno", nombre)` los deja **sin turno**; `plan`/`materia` conservan sus cascadas de borrado; la atomicidad se mantiene.
    - Código: `cascada.{hpp,cpp}` — retirar la relación `curso_materia` (en `materia` y `curso`) y la dependencia `plan → cursos`; añadir a `RelacionCascada` el modo de acción (`eliminar` vs `anularReferencia`) y marcar `aula`/`turno` → cursos como anulación (etiqueta `anio || ' ' || seccion`). `NucleoDatos::borrarRecursivo` — para las relaciones de anulación, `UPDATE <tabla> SET <columna> = NULL WHERE <columna> = :id` dentro de la transacción atómica.
    - Hecho cuando: `ctest` pasa `test_cascada`; borrar un aula/turno referenciado solo anula la referencia de los cursos (el curso permanece) y el diálogo de dependientes muestra «año sección».

- [ ] **T6. Huella de datos sin las materias del curso.** RF-5
    - Tests (rojo): `test/backend/test_servicio_generacion.cpp` **(modificado)** — `huellaDatos` cambia de forma predecible al cambiar año/sección/turno/aula y es **estable** ante cambios no persistidos (p. ej., materias del curso); ya no depende de `nombre`/`numEstudiantes`/`codigoPlan`.
    - Código: `ServicioGeneracion.cpp` — `huellaDatos` canoniza `id`, `anio`, `seccion`, `turno`, `aula` del curso y **no** recorre materias del curso.
    - Hecho cuando: `ctest` pasa `test_servicio_generacion`; la huella refleja solo los datos persistidos y es reproducible (cierre del refactor T2–T6: suite de dominio en verde).

- [ ] **T7. Consola de datos adaptada al modelo nuevo.** RF-2, RF-4
    - Tests (rojo): verificación por la consola no interactiva (`gestor-horarios-consola --cmd`) — `cursos crear <anio> <seccion> [turno] [aula]` registra; `cursos listar` muestra `id | <nombre derivado> | anio=… | seccion=… | turno=… | aula=…`; `cursos actualizar <id> <anio> <seccion> […]` actualiza; el duplicado año+sección devuelve mensaje en español; no hay acciones `asignar`/`quitar` de materias.
    - Código: `src/consola/src/ConsolaDatos.cpp` — comandos y etiquetas de `cursos` al modelo nuevo; retirar `asignar`/`quitar`; textos de ayuda en español.
    - Hecho cuando: el ejecutable compila con `--preset full`, los comandos anteriores operan sobre el modelo nuevo y los mensajes son los de RNF-2.

- [ ] **T8. Contrato de consumo del frontend actualizado.** RF-2, RF-4, RF-6
    - Tests (rojo): `test/backend/test_contrato_interfaz.cpp` — sigue verde; fija que `CursoDTO` expone `{ id, anio, seccion, turno, aula }` (sin `nombre`/`numEstudiantes`/`codigoPlan`/`aulaFija`/`materias`), las firmas `crearCurso(anio,seccion,turno,aula)`/`actualizarCurso(id,anio,seccion,turno,aula)` y la ausencia de operaciones curso–materia.
    - Código: ninguno de producto; se actualiza `docs/interfaz-frontend.md` — tabla de `CursoDTO`, firmas de `NucleoDatos`, clave de pendiente de alta de curso, estados de `AperturaBaseDatos` y cascada `aula`/`turno` con anulación de referencia.
    - Hecho cuando: `ctest` pasa el test de contrato y `docs/interfaz-frontend.md` describe, para cada elemento, qué expone, su firma/contrato, sus estados/errores y cómo debe tratarlo el frontend.

- [ ] **T9. Cierre: retirada completa y suite verde.** RF-1…RF-6; RNF-1, RNF-2, RNF-3
    - Tests (verificación final): la suite completa pasa con el comando del contenedor.
    - Código: barrido de cierre — comprobar que **no** quedan referencias a `Curso_Materia`, `CursoMateriaDTO`, `cargarMaterias`, `asignarMateria`/`quitarMateria` de curso, ni a `nombre`/`numEstudiantes`/`codigoPlan`/`aulaFija` como campos de curso en `src/`, `docs/` ni `test/`; adaptar cualquier test ajeno que aún use la firma antigua.
    - Hecho cuando: `ctest --preset full --output-on-failure` pasa toda la suite; cada RF tiene su comprobación verde; y se actualiza `MEMORY.md` (fase e historial).

---

## Cobertura

- **RF-1** (identidad, unicidad, aula, baja de aula): T2 (esquema `UNIQUE`/`CHECK`/FK + `esquemaEsperado`), T4 (validación, duplicado, aula existente, defensa `UNIQUE`/FK), T5 (cascada `aula`→cursos con anulación), T1 (`validarIdentidadCurso`).
- **RF-2** (servicios y orden): T1 (`nombreCurso`, `cursoAntes`), T3 (DTO, `obtener`/`listar` ordenado, retirada de materias), T4 (CRUD), T7 (consola), T8 (contrato).
- **RF-3** (validación y normalización): T1 (`normalizarSeccion`, `validarIdentidadCurso`, `ß`→`SS`), T4 (mensajes campo+valor, sección no ASCII).
- **RF-4** (exposición en el dominio): T3 (`CursoDTO`/`mapearARecord`), T7 (consola), T8 (contrato).
- **RF-5** (retirada curso–materia, cascadas, huella): T3/T4 (sin operaciones de materias), T5 (cascada), T6 (`huellaDatos`).
- **RF-6** (esquema único y colapso): T2 (`Migracion`/`VersionEsquema`/`AperturaBaseDatos`/`DatabaseManager`), T8 (contrato de apertura).
- **RNF-1** (sin operación destructiva): T2 (sin migración/respaldo previo; `FalloEsquema` no borra).
- **RNF-2** (mensajes en español): T4 (servicio), T7 (consola).
- **RNF-3** (trazabilidad): cada RF tiene al menos un «Hecho cuando» verificable en T1–T9; sin cobertura no se marca la tarea.
