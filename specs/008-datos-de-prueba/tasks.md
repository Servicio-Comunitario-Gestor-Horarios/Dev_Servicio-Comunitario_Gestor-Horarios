# Tareas 008 - Datos de prueba del centro (Plan 31059)

Base: f94acba455a04ae80364c82e1f07ae16d9f156ec

- [x] **T1. Módulo `seed` y plan General.** `src/seed` (CMake, `perfil.hpp`, `perfil.cpp`), `option(BUILD_SEED)` y `add_subdirectory(seed)`. Tipos del perfil, catálogo de 51 materias-año del 31059, plan General de la tabla adaptada, `bloques()` y totales por año; `test/backend/test_seed_perfil.cpp` con los casos del plan General y `bloques()` (rojo primero), registrado en `test/CMakeLists.txt`. RF-1, RF-2, RNF-2
    - Hecho cuando: `cmake --build build --target test_seed_perfil && ctest --test-dir build -R test_seed_perfil --output-on-failure` pasa con el plan General en **40/40/40/40/40 horas académicas** (1 hora = 1 bloque), 9/9/10/11/12 materias y `bloques(h) == h`, y `cmake --preset dev-frontend` sigue configurando sin el módulo.

- [x] **T2. Planes derivados y validación del perfil.** EXA/NAT/SOC con los cambios de §4.2, catálogo de 57 materias, `validarPerfil()` (progresión, nuevas por plan, variaciones, sin huérfanas, ≤ 40 horas académicas) y sus casos en `test_seed_perfil.cpp`. RF-1, RF-3
    - Hecho cuando: `ctest --test-dir build -R test_seed_perfil --output-on-failure` pasa con 53 cargas por derivado, **Σ horas = 40 en cada año**, ≥2 materias nuevas en 4.º–5.º y rechazo con mensaje en español de un plan con año > 40 horas académicas, una materia huérfana y un aula repetida.

- [x] **T3. Turnos, recesos, aulas y cursos.** Turnos mañana 07:00–12:30 y tarde 12:40–18:10 (8 slots, receso 10 min tras el slot 2), 16 aulas, 13 cursos sin turno con aula distinta; validación y casos nuevos en `test_seed_perfil.cpp`. RF-4, RF-5
    - Hecho cuando: `ctest --test-dir build -R test_seed_perfil --output-on-failure` pasa comprobando inicio/fin/slots, `despues_de_slot = 2`, horas 08:20–08:30 y 14:00–14:10, 13 cursos con `turno` vacío y aulas únicas.

- [x] **T4. Variantes de docentes, carga y cobertura.** `perfil.hpp`/`perfil.cpp`: `cargaMateria()`, `cargaAsignada()`, `demandaTotal()` (funciones sobre `Perfil`); `docentes.hpp`/`docentes.cpp`: `docentesCumplenCarga()`, `coberturaDeVariante()` y las cuatro variantes (holgada 65, suplentes 43, sin profesorado 64, sin disponibilidad 65); casos en `test_seed_perfil.cpp`. RF-6, RF-7, RF-8
    - Hecho cuando: `ctest --test-dir build -R test_seed_perfil --output-on-failure` pasa con `demandaTotal() == 520`, `cargaMateria("Matemática Aplicada") == 0` (las 6 nuevas no añaden demanda), cargas por materia de 0 a 18, holgada con `carga ≤ 0,6 × tope` (tope 30; el máximo, 18, es exactamente el 60 %), suplentes con cada titular `carga ≥ 0,8 × tope` y ratio carga/tope entre 1,00 y 1,15, `Σ topes` = **461** dentro de `[0.85, 0.90] × demandaTotal()` (88,65 %), recuentos 65/65/325, 43/57/215, 64/64/320 y 65/65/325, cobertura en ambos turnos en las válidas y fallo en `Física 3º` en las dos inválidas.

- [x] **T5a. Aplicador: apertura, detección y catálogo.** `sembrador.hpp`/`sembrador.cpp`: `aplicar()` con `ContextoBaseDatos`, detección de datos, transacción, inserción de turnos+recesos, aulas, materias, planes y **210 cargas por SQL directo** (nunca `asignarCarga`), cursos; `test/backend/test_seed_aplicacion.cpp` con crear/poblar y base con datos (rojo primero). La comprobación de recuentos cubre solo las tablas de esta tarea; T5b la completa. RF-4, RF-5, RF-10, RNF-3
    - Hecho cuando: `cmake --build build --target test_seed_aplicacion && ctest --test-dir build -R test_seed_aplicacion --output-on-failure` pasa con 2 turnos, 2 recesos (`(Mañana, 2, 08:20, 08:30)` y `(Tarde, 2, 14:00, 14:10)`), 16 aulas, 57 materias, 4 planes, 210 cargas, 13 cursos con `turno` NULL, 0 docentes y `BaseConDatos` sin escribir en la segunda aplicación.

- [x] **T5b. Aplicador: docentes, enlaces y recuentos.** Inserciones de docentes, `Profesor_Materia` y `Disponibilidad_Profesor` dentro de la transacción; recuentos completos contra `recuentosEsperados()`, determinismo entre dos bases y rollback con `simularFalloTrasDocentes`; casos nuevos en `test_seed_aplicacion.cpp`. RF-6, RF-7, RF-10, RF-11, RNF-5
    - Hecho cuando: `ctest --test-dir build -R test_seed_aplicacion --output-on-failure` pasa con 65/65/325 en la holgada, dos bases idénticas salvo `fecha_creacion`/`fecha_modificacion` y, tras el fallo simulado en una base nueva, 0 filas en las 10 tablas.

- [x] **T6. Reemplazo, respaldo, conmutación y atomicidad.** `--reemplazar`, `--solo-docentes`, `Respaldo` con `rutaRespaldoLibre()`, `--si`, borrados por SQL directo y rollback que preserva los datos previos; casos nuevos en `test_seed_aplicacion.cpp`. RF-9, RF-10, RNF-6
    - Hecho cuando: `ctest --test-dir build -R test_seed_aplicacion --output-on-failure` pasa comprobando respaldo válido (`validarRespaldo`), `ConfirmacionRequerida` sin `--si`, docentes 43 tras `--solo-docentes` con el resto de tablas (incluidas las 210 cargas) idéntico y base previa intacta tras el fallo simulado en reemplazo.

- [x] **T7. Ejecutable `gestor-horarios-seed`.** `src/seed/src/main.cpp` con `--db`, `--variante`, `--reemplazar`, `--solo-docentes`, `--si`, `--ayuda`, `--version`, salida y códigos de §7; target y `PROJECT_VERSION` en `src/seed/CMakeLists.txt`. RF-4, RF-9, RF-10, RF-13
    - Hecho cuando: `cmake --build build --target gestor-horarios-seed` y los cuatro comandos de §11 devuelven `ok`, código 2 con «la base ya tiene datos», código 6 con variante inexistente y `--ayuda` lista todas las opciones.

- [x] **T8. Informe de recuentos, materias por año y totales.** `resumenTexto(perfil, variante, recuentos)` leyendo la base y los totales por plan/año; caso `tiempoDeAplicacion` (< 15 s) en `test_seed_aplicacion.cpp` y salida definitiva de la CLI. RF-12, RNF-1
    - Hecho cuando: `ctest --test-dir build -R test_seed --output-on-failure` pasa y la CLI imprime la variante, los recuentos por entidad, `Materias por año` (GEN 9/9/10/11/12; derivados 9/9/10/12/13), las cuatro líneas de plan con sus **40/40/40/40/40 horas académicas** y la ruta del respaldo, en menos de 15 s.

- [x] **T9. Documentación y cierre.** `docs/guias/datos-prueba.md` (uso, variantes, recuentos, seguridad, «no genera presets») y verificación de la suite completa. RF-13, RNF-4
    - Hecho cuando: `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"` queda en verde (total previo del entorno + 2, sin rojos; el total depende de `find_package(GTest)`) y la guía existe con los comandos y la advertencia de datos ficticios y de que no se generan presets.
