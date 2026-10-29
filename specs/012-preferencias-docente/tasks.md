# Tareas 012 - Preferencias horarias del docente

Todas las tareas: **20–30 min**. Orden y dependencias: T1 → T3; T2 → T3; T3 → T4; T3,T4 → T5; T2 → T6;
T6 → T7; T3,T4 → T8; todo → T9. Ninguna tarea cierra con el árbol sin compilar.

- [x] **T1. Modelo puro y validación compartida de día/rango (~25 min).** RF-1, RF-2, RNF-1
    - Hecho cuando: `cmake --preset backend`, `cmake --build --preset backend --parallel` y `ctest --test-dir build-backend --output-on-failure -R '^(test_preferencias|test_servicio_profesor)$'` están en verde; `franja_horaria::errorDeFranja` rechaza `-1`, `7`, horas inválidas e inicio ≥ fin con mensaje que incluye el valor y la acción, y acepta 0/6; `haySolape(08–10, 09–11)` es verdadero y `(08–10, 10–12)` falso; `textoDeSentido`/`sentidoDesdeTexto` hacen ida y vuelta; `sentidoValido` rechaza un valor casteado; `ServicioProfesor.cpp` usa la validación compartida y su suite sigue verde.

- [ ] **T2. Esquema v5: `Preferencia_Profesor`, catálogo y huella (~30 min).** RF-1, RF-6
    - Hecho cuando: `cmake --build --preset backend --parallel` compila y `ctest --test-dir build-backend --output-on-failure -R '^(test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos)$'` está verde; `VERSION_ESQUEMA_ACTUAL == 5`; `crearEsquema` crea `Preferencia_Profesor` con `id`, `id_Profesor`, `dia`, `hora_inicio`, `hora_fin`, `sentido`; los `CHECK` rechazan día -1/7, sentido distinto de `preferida`/`evitada` e inicio ≥ fin; el `UNIQUE` rechaza el duplicado exacto; `HUELLAS_CONOCIDAS` tiene la entrada 5 con la huella real que imprime el test al fallar (copiada, no inventada); una base con `user_version = 4` falla al abrir; el test `versionActual_esCuatro` se renombra a `versionActual_esCinco` y el comentario «La base se crea con el esquema v4» de `test_servicio_configuracion_horaria.cpp:15` pasa a v5.

- [ ] **T3. Servicio de preferencias: CRUD, validación y orden (~30 min).** RF-1, RF-2, RF-3, RNF-1
    - Hecho cuando: `cmake --build --preset backend --parallel` compila y `ctest --test-dir build-backend --output-on-failure -R '^test_servicio_preferencias$'` está verde; `agregar` guarda y devuelve el id; docente inexistente, día fuera de 0–6, horas inválidas, inicio ≥ fin, sentido inválido y duplicado exacto se rechazan; solape de sentidos opuestos se rechaza nombrando las dos franjas; solapes y rangos contiguos del **mismo** sentido se aceptan; días fuera de lunes–viernes se aceptan; `listar` ordena por día, inicio, fin e id y devuelve vacío sin error; `actualizar` cambia rango/sentido, rechaza el cambio que crea un solape opuesto **conservando el anterior**, rechaza el **cambio de día** que crea un solape opuesto y falla si el id no existe; `eliminar` distingue «no existe»; `limpiar` borra solo preferencias y no la disponibilidad; una preferencia que coincide con una franja de disponibilidad se acepta; **cada rechazo contiene el valor recibido y la acción disponible** (docente, día, rango, sentido, duplicado, solape, no existe).

- [ ] **T4. `ProfesorDTO` con preferencias y fachada `NucleoDatos` (~30 min).** RF-3
    - Hecho cuando: `cmake --build --preset backend --parallel` compila y `ctest --test-dir build-backend --output-on-failure -R '^(test_servicio_profesor|test_backend_nucleo_datos|test_servicio_preferencias)$'` está verde; `ProfesorDTO` devuelve `disponibilidad` y `preferencias` como listas distintas y ordenadas en `obtenerProfesor` y `listarProfesores`; `ProfesorDTO::toProfesor()` no copia preferencias; `NucleoDatos::agregarPreferencia`, `listarPreferencias`, `actualizarPreferencia`, `eliminarPreferencia` y `limpiarPreferencias` funcionan; una escritura fallida queda registrada como pendiente y se puede reintentar.

- [ ] **T5. Cascada y baja del docente (~25 min).** RF-4, RNF-4
    - Hecho cuando: `cmake --build --preset backend --parallel` compila y `ctest --test-dir build-backend --output-on-failure -R '^(test_backend_cascada|test_servicio_profesor|test_backend_nucleo_datos|test_servicio_preferencias)$'` está verde; `dependenciasDe("profesor", id)` lista la disponibilidad y las preferencias con una etiqueta legible y `dependenciasDe("profesor").size()` pasa de 2 a **3** en `test_cascada.cpp:140`; `eliminarConCascada("profesor", id)` borra docente, disponibilidad y preferencias sin huérfanas; `NucleoDatos::eliminarDocente` hace lo mismo, con la regresión de `test_servicio_profesor` y `test_backend_nucleo_datos`; si el borrado falla, no queda nada borrado.

- [ ] **T6. Seed: recuento 0 y tabla de dominio (~25 min).** RF-7
    - Hecho cuando: `cmake --build --preset backend --parallel` compila y `ctest --test-dir build-backend --output-on-failure -R '^(test_seed_perfil|test_seed_aplicacion)$'` está verde; `Recuentos.preferencias` existe y vale 0 para las cuatro variantes; `kTablasDominio` incluye `Preferencia_Profesor` y el comentario de `sembrador.cpp:34` pasa a 12 tablas / esquema v5; la base sembrada tiene 0 filas; `resumenTexto` imprime «Preferencias: 0»; una base con solo la configuración sigue siendo sembrable.

- [ ] **T7. Seed: borrado, atomicidad y determinismo (~30 min).** RF-7, RNF-3, RNF-4
    - Hecho cuando: `cmake --build --preset backend --parallel` compila y `ctest --test-dir build-backend --output-on-failure -R '^test_seed_aplicacion$'` está verde; con una preferencia preinsertada, `--solo-docentes` y `--reemplazar` terminan sin fallar por claves foráneas y dejan 0 preferencias; con una preferencia preinsertada y `simularFalloTrasDocentes`, la preferencia y la variante anterior siguen completas (rollback); dos siembras del mismo perfil coinciden también en `Preferencia_Profesor`; el comentario (`:347`) y la lista (`:350`) de «las 11 tablas» de
  `test_seed_aplicacion.cpp` pasan a 12 tablas con `Preferencia_Profesor`.

- [ ] **T8. Huella intacta y sin efecto en la generación (~20 min).** RF-5
    - Hecho cuando: `cmake --build --preset backend --parallel` compila y `ctest --test-dir build-backend --output-on-failure -R '^(test_servicio_generacion|test_servicio_preferencias)$'` está verde; `grep -rn "preferencia" src/backend/src/services/ServicioGeneracion.cpp src/backend/include/backend/services/ServicioGeneracion.hpp` está vacío; en `test_servicio_preferencias`, una instantánea con una `PreferenciaDTO` añadida a `ProfesorDTO::preferencias` da el **mismo** `huellaDatos` que sin ella; `ProfesorDTO::toProfesor()` no incluye preferencias; la condición futura queda anotada en el plan y en la guía (§8.1), no en el código.

- [ ] **T9. Documentación (la aplica el coordinator) y regresión global (~30 min).** RF-8, RNF-5, RNF-6
    - Hecho cuando: **el coordinator** aplica §8 del plan en `docs/interfaz-frontend.md` (constante y prosa de §2.2 a v5, dominios, bloque de `NucleoDatos`, tabla de DTO, **fila `profesor` de §2.4.1** con preferencias, §2.4.5 y tabla §4) y en `docs/guias/datos-prueba.md` (fila de preferencias 0 y ejemplo de salida con `Hora académica: 35 min` y `Preferencias: 0`); `grep -n "VERSION_ESQUEMA_ACTUAL = 5" docs/interfaz-frontend.md` devuelve una línea; `grep -n "profesor.*preferencias" docs/interfaz-frontend.md` devuelve la fila de §2.4.1; `grep -n "2.4.5" docs/interfaz-frontend.md` devuelve la sección; `grep -n "Preferencias (no se siembran)" docs/guias/datos-prueba.md` devuelve la fila; `ctest --preset backend` termina **54/54** (sin `test_frontend_conversion` ni `test_generacion_seed`); `git diff --stat` solo muestra archivos de este plan. **La nota de supersesión de la 008 (§8.2) la escribe el planner al cerrar; no es parte de T9.**

## Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|----|----------|--------------|
| RF-1 | T1, T2, T3, T4 | T1: modelo puro; T2: tabla/columnas; T3: persistencia y orden; T4: DTO del docente |
| RF-2 | T1, T3 | T1: día/rango/sentido y solape puros; T3: docente, duplicado, solape opuesto y cambio de sentido |
| RF-3 | T3, T4 | T3: CRUD y limpieza con `Resultado`; T4: preferencias junto a disponibilidad y fachada |
| RF-4 | T5 | Cascada y `eliminarProfesor` borran ambas; `dependientesDe` las lista |
| RF-5 | T8 | Huella intacta y `toProfesor()` sin preferencias; `grep` en `ServicioGeneracion` |
| RF-6 | T2 | v5, catálogo, `CHECK`/`UNIQUE`, huella v5 y v4 rechazada sin migrar |
| RF-7 | T6, T7 | Recuento 0, `kTablasDominio`, orden de borrado, atomicidad y determinismo |
| RF-8 | T9 | Guía con modelo, día 0–6, validaciones, distinción y sin efecto; guía del seed |
| RNF-1 | T1, T3 | Mensajes con campo, valor, causa y acción |
| RNF-2 | T2 | v4 falla al abrir; sin migración ni borrado |
| RNF-3 | T7 | Dos siembras dejan 0 y lo mismo; orden estable |
| RNF-4 | T5, T7 | Sin huérfanas; transacciones en baja y conmutación |
| RNF-5 | T9 | `ctest --preset backend` 54/54 sin frontend |
| RNF-6 | T1–T9 | Cada RF con tarea y verificación en esta tabla |
