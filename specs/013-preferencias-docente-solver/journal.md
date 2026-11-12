# Journal 013 - Preferencias del docente en la generación

Registro de la implementación de las tareas del plan 013. Una entrada por tarea ejecutada; las entradas
son **aditivas** (no se reescriben las anteriores).

- **Base:** `bb1767af343c108b20de2b2b4de7ebe1d7658c2f` **+ la 015 y la 016 aplicadas en el árbol de
  trabajo (sin commits)**. HEAD sigue en `bb1767a`. **Si la 015/016 se commitean antes de empezar, el
  `Base` pasa a ser ese sha de cierre.** _(Actualizado: se commitearon como **`c7b9427`**, que es la base
  efectiva de la implementación; la 013 sigue en el árbol.)_
- **Fecha de apertura:** 2026-11-04.
- **Estado:** implementación **completa** (T1–T10, 2026-11-06); **APROBADO** del revisor con la
  corrección documental de **H2** aplicada (2026-11-06). Pendiente de marcar la spec como
  implementada (lo hace el planner).
- **Aprobaciones:** spec **2026-11-04** · plan/tareas **2026-11-04** (10 tareas; el exceso de duración de
  **T1/T5/T6** sobre la guía de 20–30 min fue **aceptado por el usuario**, sin dividir la spec y con
  RF-8 dentro de la 013).
- **Alcance de la entrega:** contrato 2.0 con la sección opcional `preferencias`, proyección reloj → franja
  y filtro de día, huella con las preferencias intervinientes, objetivo en dos fases (S1/S2 fijos, 2a
  preferencia y 2b S3), estado por preferencia en el resultado del backend y documentación. **Sin** cambios
  de esquema (vigente **v6**, el de la 015; la 013 **no** lo cambia), del contrato de salida, de las
  constantes S1/S2/S3 ni del seed (que sigue con 0 preferencias y ya no declara «Aula de estudio»).

```text
Comandos de referencia
  backend:      cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
  suite completa: cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

---

## T1 - Preferencia proyectada en el contrato, V19 y preset sin la sección

**Fecha:** 2026-11-06
**Requisito:** RF-2, RF-7, RNF-1. **Plan:** §1.1, §1.1.1 y §4-T1.

### Rojo (tests primero)

Tests escritos antes del código en los tres binarios de la tarea. La primera compilación falla porque
el tipo y el miembro aún no existen:

```text
$ cmake --build --preset backend --parallel --target test_contrato_solver \
    test_cargador_configuracion test_validador_entrada
error: 'const struct SolverConfig' has no member named 'preferencias'
error: 'PreferenciaSolverConfig' ... was not declared   # (y usos en qtest de los tres binarios)
ninja: build stopped: subcommand failed.
```

Casos nuevos (rojo → verde):

- `test_contrato_solver`: `preferencias_sinSeccionYseccionVacia_sonEquivalentesYnoSeSerializan`,
  `preferencias_formaProyectada_seParsean`, `preferencias_idaYVueltaEnMemoria_sinPerdida`,
  `preferencias_camposDesconocidos_seToleran`; `constantes_jerarquiaConAgrupacion`
  (`PESO_AGRUPACION == 10`) sigue sin tocar y verde.
- `test_cargador_configuracion`: `guardarPreset_omiteLaSeccionDePreferencias`,
  `presetConPreferencias_seCargaSinUsarse`.
- `test_validador_entrada`: `v19_indiceDeDocenteFueraDeRango_falla`, `v19_diaFueraDeRango_falla`,
  `v19_franjaFueraDeRango_falla`, `v19_sentidoInvalido_falla`,
  `v19_primerDefectoEnOrdenDocenteDiaFranjaSentido`, `v19_preferenciasEnRangoYseccionVacia_pasan`.

### Verde

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && \
  ctest --test-dir build-backend --output-on-failure \
  -R '^(test_contrato_solver|test_cargador_configuracion|test_validador_entrada)$'
1/3 Test #21: test_cargador_configuracion ......   Passed
2/3 Test #23: test_validador_entrada ...........   Passed
3/3 Test #42: test_contrato_solver .............   Passed
100% tests passed out of 3          (EXIT=0)

$ ctest --preset backend
56/56 Test #56: test_ipc_aislamiento_bd ...........   Passed
100% tests passed out of 56        Total Test time (real) = 17.19 sec
```

Ejecución por función (evidencia de los criterios de T1):

```text
TestContratoSolver: 7 passed (incluye preferencias_idaYVueltaEnMemoria_sinPerdida y
                    constantes_jerarquiaConAgrupacion)
TestCargadorConfiguracion: 4 passed (guardarPreset_omiteLaSeccionDePreferencias,
                    presetConPreferencias_seCargaSinUsarse)
TestValidadorEntrada: 8 passed (los cuatro defectos de V19 y el orden docente→día→franja→sentido)
```

### Archivos tocados

- `src/backend/include/backend/solver/config/solver_config.hpp` — `PreferenciaSolverConfig{docente,
  dia, slot[], sentido}` y `QVector<PreferenciaSolverConfig> preferencias` (tras `profesores`).
- `src/backend/src/solver/config/solver_config.cpp` — `parsearPreferencias`/`aJson`, lectura en
  `fromJson` (ausente = sin preferencias), `toJson` **omite** la sección vacía, **V19**
  (`validarPreferencias`, tras V18, sin tocar el orden V1–V18) y comentario de rango a **V1–V19**.
- `src/backend/src/services/CargadorConfiguracionSolver.cpp` — `guardar` elimina `preferencias` del
  objeto antes de escribir; `cargar` no cambia (tolera la sección).
- `src/backend/include/backend/services/CargadorConfiguracionSolver.hpp` — comentario: el preset no
  persiste preferencias.
- `src/backend/src/solver/solver.cpp` — comentario de rango a **V1–V19**.
- `test/backend/test_contrato_solver.cpp`, `test/backend/test_cargador_configuracion.cpp`,
  `test/backend/test_validador_entrada.cpp` — casos de arriba.

### Hallazgos / desviaciones

- **Sin desviaciones de alcance.** No se ha tocado la proyección (T2), la construcción (T3), el
  objetivo (T5), la huella (T4) ni nada de la 016.
- **Valores por defecto del parseo de una preferencia** (no cubierto por el plan): `docente` y `dia`
  ausentes se leen con centinela `-1` (el del struct) para que V19 los rechace por rango en vez de
  convertirlos silenciosamente en 0; `slot` ausente = arreglo vacío y `sentido` ausente = `""` (V19
  lo rechaza por sentido). La sección ausente sigue siendo «sin preferencias» y no es error.
- **Textos de V19** (valor recibido + rango admitido + acción, RNF-1), en el orden
  docente → día → franja → sentido:
  1. `V19: la preferencia %1 declara el docente %2, fuera del rango de docentes (0–%3); use el índice de un docente de la configuración.`
  2. `V19: la preferencia %1 del docente %2 declara el día %3, fuera del rango de la generación (0–%4); use un día de la semana declarada.`
  3. `V19: la preferencia %1 del docente %2 declara la franja %3, fuera del rango del día (0–%4); use una franja declarada.`
  4. `V19: la preferencia %1 del docente %2 declara el sentido «%3», no válido (preferida/evitada); use «preferida» o «evitada».`
  `%1` es el número de preferencia (1-based) en el orden del arreglo.
- **«Cargar un preset que contiene la sección sin usarla»** se comprueba en el cargador como: el
  archivo se acepta, la configuración en memoria conserva lo declarado **sin reproyectar ni
  completar**, y al guardar de nuevo como preset la clave **no** se propaga. El «no es fuente» para
  la generación se cierra en T3 (la construcción sobrescribe con las proyecciones de los datos).
- **Ida y vuelta:** se comparan campo a campo (docente, día, slot, sentido) y `toJson` de la
  **configuración**; en el preset no se compara `toJson` de ida y vuelta porque `guardar` omite la
  sección (P13).
- **Efecto de la omisión:** el JSON con 0 preferencias no gana la clave, así que el golden de T3 y los
  presets 2.0 anteriores siguen siendo byte-equivalentes.

---

## T2 - Rejilla compartida, proyección reloj → franja y filtro de día (puro)

**Fecha:** 2026-11-06
**Requisito:** RF-3, RNF-1. **Plan:** §1.2 y §4-T2.

### Rojo (tests primero)

`test/backend/test_proyeccion_preferencias.cpp` (17 casos, `add_gtest(... backend)` en
`test/CMakeLists.txt`) escrito antes del módulo:

```text
$ cmake --build --preset backend --parallel
test/backend/test_proyeccion_preferencias.cpp:9:10: fatal error:
  backend/services/proyeccion_preferencias.hpp: No existe el fichero o el directorio
ninja: build stopped: subcommand failed.
```

### Verde

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
57/57 Test #57: test_ipc_aislamiento_bd ...............   Passed
100% tests passed out of 57        Total Test time (real) = 14.29 sec

$ ctest --test-dir build-backend -R '^(test_proyeccion_preferencias|test_constructor_entrada)$'
1/2 Test #22: test_constructor_entrada .........   Passed
2/2 Test #49: test_proyeccion_preferencias .....   Passed
100% tests passed out of 2

$ ./build-backend/test/test_proyeccion_preferencias
[==========] 17 tests from 1 test suite ran.  [  PASSED  ] 17 tests.
```

17 casos: `rejilla_ordenaTurnosPorInicioYAcumulaOffsets`,
`rejilla_soloAlcanzaBloquesEnterosDentroDelRango`, `rejilla_bordesDeFranjaSinCreditoParcial`,
`rejilla_recorreTodosLosTurnosDelDia`, `rejilla_cruceEntreTurnosAcumulaMananaYTarde`,
`rejilla_elEventoNoCortaNiDesplazaLaRejilla`, `domingo_noAplicaPorDiaFueraDeSemana`,
`sabadoNoDeclarado_noAplica`, `sabadoDeclarado_aplicaYConvierteElDiaDeLaGeneracion`,
`docenteQueNoParticipa_motivoDocenteNoParticipa`,
`preferenciaDeTardeConDocenteSoloDeManana_motivoSinFranjaDelTurno`,
`rangoSinNingunaFranjaDeLaRejilla_motivoSinFranjaDelTurno`,
`motivos_precedenciaDocenteNoParticipaGanaADiaYFranja`,
`interveniente_llevaDiaDeLaGeneracionYFranjasAlcanzadas`,
`interveniente_queCruzaTurnosEsUnaEntradaConSusFranjas`, `intervinientes_salenEnOrdenCanonico`,
`textoDeMotivo_esEspanolYdistingueLosTresMotivos`.

### Archivos tocados

- **Nuevo** `src/backend/include/backend/services/proyeccion_preferencias.hpp` y
  `src/backend/src/services/proyeccion_preferencias.cpp` (puro): `TurnoProyectado`,
  `ordenarTurnos`, `slotsDeFranja`, `MotivoPreferencia`/`textoDeMotivo`,
  `PreferenciaProyectada`, `PreferenciaNoAplicada`, `ProyeccionPreferencias` y
  `proyectarPreferencias`.
- `src/backend/src/services/ConstructorEntradaSolver.cpp` — **desaparecen** su `TurnoOrdenado`,
  su `ordenarTurnos` y su `slotsDeFranja` locales; la disponibilidad usa la rejilla compartida.
  Se retira además el helper `intArray` (muerto y con aviso de «no se usa» al recompilar) y los
  `#include` que dejaron de hacer falta.
- `src/backend/CMakeLists.txt` — `src/services/proyeccion_preferencias.cpp` en el target `backend`.
- `test/CMakeLists.txt` — `add_gtest(test_proyeccion_preferencias ... backend)`.
- **Nuevo** `test/backend/test_proyeccion_preferencias.cpp`.

### Regla de proyección y motivos

- Franja = `(día de la generación, slot absoluto del día)`. Se recorre **cada turno** del día con
  su propia rejilla (ancla en `inicio`, paso `duracionMinutos`) y solo se emiten los bloques que
  caben **enteros** (`bloqueInicio >= inicio ∧ bloqueFin <= fin`). Los **eventos no cortan ni
  desplazan** (no se consultan). Los slots de un turno continúan donde terminan los del anterior
  (`ordenarTurnos` acumula `offset`).
- Filtro de día: `diaGeneracion = franja.dia - 1`; aplica si `1 ≤ dia ≤ 6` y
  `diaGeneracion < num_dias`. Domingo nunca; sábado solo si la semana declarada lo incluye.
- «Participa» = `docenteCandidatoElegible` (V13) de al menos una materia-curso; las franjas
  alcanzables son la **unión de los slots de los turnos de esos cursos**. Motivos con precedencia
  **(c) > (a) > (b)**: `DocenteNoParticipa` → `DiaFueraDeSemana` → `SinFranjaDelTurno`
  (`alcanzables = slotsDeFranja ∩ slotsPosibles` vacío). `noAplicadas` conserva la franja de
  **dominio** y un solo motivo; los `intervinientes` salen en orden canónico
  `(docente, día, sentido, franjas)`, con slots ascendentes y sin repetir.

### Hallazgos / desviaciones

- **Sin desviaciones de alcance:** no se toca la construcción (T3), el objetivo (T5), la huella
  (T4), el estado por preferencia (T7) ni la comprobación de validez (016). `seed.md` intacto.
- **Correspondencia de índices:** `proyectarPreferencias` asume que `config.profesores[i]` es el
  docente `i` de `datos.docentes`; es el orden en que construye la entrada y queda documentado en
  el header. Se recorre `min(datos.docentes.size(), config.profesores.size())` como guarda.
- **Slots de una preferencia:** se ordenan y deduplican al proyectar (la deduplicación del
  **objetivo** por `(docente, día, slot)` es de T5, no de aquí).
- **Orden canónico por sentido:** se usa el orden del enumerado (`Preferida` < `Evitada`) de forma
  explícita con `static_cast<int>`; el orden de declaración no influye (RNF-2).
- **`textoDeMotivo`** (RNF-1): vacío para `Ninguno`; «el docente no participa en la generación: no
  es candidato elegible de ninguna materia-curso»; «el día queda fuera de la semana declarada de la
  generación»; «el rango no alcanza ninguna franja del turno de un curso donde el docente es
  candidato elegible».
- **Disponibilidad sin cambios:** `test_constructor_entrada` sigue verde (la rejilla movida es
  idéntica: mismo ancla, mismo paso, mismo criterio de bloque entero).

---

## T3 - Construcción de la entrada: preferencias desde los datos, preset ignorado y orden con V19

**Fecha:** 2026-11-06
**Requisito:** RF-2, RF-3, RF-7, RNF-4. **Plan:** §1.3 y §4-T3.

### Rojo (tests primero)

Cinco casos nuevos en `test_constructor_entrada.cpp` escritos antes de la función:

```text
$ cmake --build --preset backend --parallel --target test_constructor_entrada
test_constructor_entrada.cpp:525:32: error: ‘construirEntradaCompleta’ no se declaró en este ámbito
```

Casos: `preferenciasSeConstruyenDesdeLosDatos`, `seccionDelPreset_seIgnoraYseSobrescribe`,
`docenteSinPreferenciasNoEmiteYelIndiceEsElDeLaConfiguracion`,
`preferenciaNoAplicada_noViajaYseInformaConSuMotivo`, `sinPreferencias_jsonIdenticoAlDeHoy`.

### Verde

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
57/57 Test #57: test_ipc_aislamiento_bd ...............   Passed
100% tests passed out of 57        Total Test time (real) = 15.67 sec

$ ctest --test-dir build-backend -R '^(test_constructor_entrada|test_validador_entrada)$'
1/2 Test #22: test_constructor_entrada .........   Passed
2/2 Test #23: test_validador_entrada ...........   Passed
100% tests passed out of 2

$ ./build-backend/test/test_constructor_entrada
Totals: 29 passed, 0 failed, 0 skipped, 0 blacklisted
```

### Firma final

```cpp
struct EntradaConstruida {
    QJsonObject                    json;
    QVector<PreferenciaProyectada> intervinientes;
    QVector<PreferenciaNoAplicada> noAplicadas;
};
Resultado<EntradaConstruida> construirEntradaCompleta(const DatosDominio&, const SolverConfig&,
                                                      int duracionMinutos, const QDateTime& ahora);
Resultado<QJsonObject> construirJsonEntrada(...);   // envoltorio: firma de hoy, solo el JSON
```

La fachada `ConstructorEntradaSolver::construir` sigue delegando en `construirJsonEntrada`; el
único call site de producción (`entrada_solver_builder.cpp`, frontend) no cambia en T3.

### Cómo se construye y se sobrescribe la sección

Orden: se arma la configuración **sin** preferencias (la salida se compone campo a campo, así que
la sección del preset nunca se copia) → `proyectarPreferencias(datos, salida, duracionMinutos)`
sobre la configuración ya completa (cursos, planes, turnos y disponibilidad) → se vuelca cada
interviniente a `salida.preferencias` con `textoDeSentido` → `toJson()` + `meta`. El resultado
devuelve además `intervinientes` y `noAplicadas` (con su motivo). Con 0 intervinientes el arreglo
queda vacío y `toJson` omite la sección.

### Archivos tocados

- `src/backend/include/backend/services/ConstructorEntradaSolver.hpp` — `EntradaConstruida`,
  `construirEntradaCompleta` y el comentario del envoltorio (incluye `proyeccion_preferencias.hpp`).
- `src/backend/src/services/ConstructorEntradaSolver.cpp` — la implementación pasa a
  `construirEntradaCompleta`, se proyectan las preferencias y `construirJsonEntrada` queda como
  envoltorio que conserva la firma y propaga el error.
- `test/backend/test_constructor_entrada.cpp` — cinco casos nuevos y dos helpers
  (`preferencia`, `agregarBeto`).

### Hallazgos / desviaciones

- **Sin desviaciones de alcance:** no se toca la huella (T4), el objetivo (T5) ni el estado por
  preferencia (T7); `seed.md` intacto y sin commits.
- **Golden sin preferencias:** el JSON de un dominio sin preferencias conserva **exactamente** las
  doce claves de hoy (`version`, `aula_fija`, `dimensiones`, `franja_horaria`, `turnos`, `eventos`,
  `planes`, `cursos`, `profesores`, `materias`, `aulas`, `meta`) y no gana `preferencias`; los
  tests actuales del binario no se han tocado. El envoltorio devuelve **el mismo** objeto JSON que
  `construirEntradaCompleta` (comprobado con `QCOMPARE`).
- **V19 tras construir:** la entrada con una preferencia proyectada (lunes 07:00–08:10, 35 min →
  día 0, slots {0, 1}) valida con `validarEntradaSolver`; también la de un segundo docente, cuyo
  índice viaja como 1. La construcción **no** valida por dentro (se mantiene el orden del flujo:
  construir → validar en `ServicioGeneracion`).
- **Preset:** se comprueba en dos direcciones —con datos sin preferencias no aparece la sección
  aunque el preset declare una; con datos con preferencia viaja solo la proyectada de los datos
  (día 0 / slots {0,1}), no la del preset (día 4 / slot {5}).
- **No aplicadas:** una preferencia de domingo no viaja y se informa en `noAplicadas` con
  `DiaFueraDeSemana` y su franja de dominio.

---

## T4 - Huella con intervinientes y cableado mínimo del frontend

**Fecha:** 2026-11-06
**Requisito:** RF-6, RNF-6. **Plan:** §1.4, §1.6 y §4-T4.

### Rojo (tests primero)

Tests escritos antes de cambiar la firma; la compilación falla en los tres binarios de la tarea:

```text
$ cmake --build --preset backend --parallel --target test_servicio_generacion test_servicio_preferencias
test_servicio_preferencias.cpp:512:47: error: demasiados argumentos para la función
  ‘QString huellaDatos(const DatosDominio&, int)’
test_servicio_generacion.cpp:333:39: error: demasiados argumentos ...
```

Casos nuevos: `huellaDatos_sinIntervinientes_conservaElValorDeHoy` (golden),
`huellaDatos_unaPreferenciaIntervinienteCambiaLaHuella` (de punta a punta: datos → proyección →
huella), `huellaDatos_unaPreferenciaNoIntervinienteNoCambiaLaHuella` (domingo, lista vacía),
`huellaDatos_elOrdenDePreferenciasYfranjasNoAlteraLaHuella`,
`datosAnteriores_siCambiaUnaPreferenciaInterviniente`; y en `test_servicio_preferencias` se
**reescribe** `huellaDatos_noCambiaConLasPreferenciasYtoProfesorLasIgnora` como
`huellaDatos_cambiaSoloConLasPreferenciasIntervinientesYtoProfesorLasIgnora`.

### Verde

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
100% tests passed out of 57        Total Test time (real) = 15.27 sec

$ cmake --preset full-release && cmake --build build-rel --parallel   (sin errores ni avisos)
$ ctest --test-dir build-rel -R '^test_generacion_seed$'
1/1 Test #62: test_generacion_seed .............   Passed   25.18 sec
$ ctest --preset full-release
62/62 Test #62: test_generacion_seed ..................   Passed   27.57 sec
100% tests passed out of 62        Total Test time (real) = 46.20 sec
```

**`full-release` pasa de 61 a 62** binarios: el test nuevo de T2 (`test_proyeccion_preferencias`)
ahora entra en la cuenta de la suite completa. El backend sigue en **57/57**.

### Firma final y qué entra

```cpp
QString huellaDatos(const DatosDominio& datos, int duracionMinutos,
                    const QVector<PreferenciaProyectada>& intervinientes);
```

- **Entra:** la duración efectiva (`dur:`), la instantánea de datos como hasta hoy y, **solo si la
  lista no está vacía**, un bloque por preferencia interviniente con la forma
  `pref:<docente>,<dia>,<sentido>,<slot>[,<slot>…];` — docente por índice, **día de la
  generación**, sentido («preferida»/«evitada») y franjas.
- **No entra:** el rango de reloj declarado, las preferencias que **no intervienen** (no llegan a
  la lista) ni nada del preset.
- **Sin intervinientes** no se emite el bloque: la huella es **exactamente** la de antes de la 013.
  Golden capturado con el binario **previo** al cambio y el mismo fixture `dominio()` a D = 35:
  `3c50f3abc8e7c294815cb32bf6e80b8837265950341f6a07fbbe576fd0ed1330`.
- **Canonización:** se copia la lista, se ordenan las franjas de cada preferencia y se ordenan las
  preferencias por `(docente, día, sentido, franjas)` antes de hashear (mismo criterio que la
  proyección).

### Composicion y huellaActual

```cpp
struct Composicion {
    QJsonObject entrada;
    ContextoHorario contexto;
    QStringList avisos;
    QString huella;                                          // 013, RF-6
    QVector<PreferenciaNoAplicada> preferenciasNoAplicadas;  // 013, RF-8 (diagnóstico)
};
QString huellaActual(const NucleoDatos& nucleo, const OpcionesUI& opciones);
```

- `construirEntrada` usa `construirEntradaCompleta` y rellena `entrada` (el JSON),
  `huella` (`huellaDatos(datos, duracionEfectiva, intervinientes)`) y `preferenciasNoAplicadas`.
- `huellaActual(nucleo, opciones)` **compone la entrada por el mismo camino** y devuelve
  `comp.huella`; si la composición falla (entrada vacía), cae al respaldo de hoy
  `huellaDatos(datos, duracion, {})` con 35 min si la configuración no se puede leer.
- Vista (`schedule_generation_widget.cpp`): la lambda de `setClient` (`:131`) pasa
  `leerOpciones()`; la generación (`:989`) pasa el `opciones` ya calculado de la vista. El
  frontend compila en `full-release`.

### Call sites actualizados

- Backend: `ServicioGeneracion.cpp` (implementación) y `ServicioGeneracion.hpp` (firma).
- Tests: todos los `huellaDatos(...)` de `test_servicio_generacion.cpp` pasan `{}` **explícitamente**
  (el golden, los de tipo/eventos/duración y los dos lambdas del servicio) y los nuevos pasan listas
  explícitas; `test_servicio_preferencias.cpp` reescrito con la interviniente explícita y `{}` para
  la de domingo.
- Frontend: `entrada_solver_builder.cpp` (huella de la composición y `huellaActual`),
  `entrada_solver_builder.hpp` (`Composicion` y firma) y `schedule_generation_widget.cpp` (`:131` y
  `:989`). `test_generacion_seed.cpp` no la llama (seed con 0 preferencias) y sigue verde.

### Archivos tocados

- `src/backend/include/backend/services/ServicioGeneracion.hpp` — firma de tres argumentos y
  documentación de qué entra en la huella.
- `src/backend/src/services/ServicioGeneracion.cpp` — bloque `pref:` con canonización; sin
  intervinientes no se emite.
- `src/frontend/src/services/entrada_solver_builder.hpp` — `Composicion` gana `huella` y
  `preferenciasNoAplicadas`; `huellaActual` gana `opciones`.
- `src/frontend/src/services/entrada_solver_builder.cpp` — usa `construirEntradaCompleta` y compone
  la huella con las intervinientes; `huellaActual` delega en la composición.
- `src/frontend/src/views/schedule_generation_widget.cpp` — `:131` con `leerOpciones()` y `:989`
  con el `opciones` calculado.
- `test/backend/test_servicio_generacion.cpp` — `{}` explícito en todos los call sites + cinco casos;
  `test/backend/test_servicio_preferencias.cpp` — caso de huella reescrito.

### Hallazgos / desviaciones

- **`presetDesdeOpciones` no se extrae** (previsto en §1.6): al delegar `huellaActual` en
  `construirEntrada`, el preset se compone **una sola vez** en un único camino y no hay copia que
  pueda divergir. La extracción era para evitar duplicar el bloque del preset, que ya no se duplica.
  El criterio de la tarea (mismo preset en las dos rutas) se cumple por construcción.
- **Dos composiciones por generación** (la lambda de la huella al recibir y la de `onGenerarClicked`):
  es el coste aceptado en §6 del plan; son milisegundos de lectura del núcleo.
- **`generar` sigue con dos argumentos:** pasarle `preferenciasNoAplicadas` es T7, no T4.
- Sin cambios de esquema, sin migración (RNF-6), `seed.md` intacto y sin commits.

---

## T5 - Objetivo: fase 1 de referencia y fase 2 con anclaje, deduplicación y fallback

**Fecha:** 2026-11-06
**Requisito:** RF-1, RF-4, RF-5. **Plan:** §1.5, §2 y §4-T5. **Estimación aceptada:** ~45–60 min.

### Rojo (tests primero)

`test/backend/test_solver_preferencias.cpp` (11 casos, `add_gtest(... backend_solver)`) escrito
antes del código; la compilación falla por la constante y por el presupuesto:

```text
$ cmake --build --preset backend --parallel --target test_solver_preferencias
test_solver_preferencias.cpp:464:13: error: ‘PresupuestoFases’ no es un miembro de ‘Solver’
```

### Verde

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
58/58 Test #58: test_ipc_aislamiento_bd ...............   Passed
100% tests passed out of 58        Total Test time (real) = 15.03 sec

$ ctest --test-dir build-backend -R '^(test_solver_preferencias|test_solver_horarios|test_contrato_solver)$'
1/3 test_contrato_solver ... Passed
2/3 test_solver_horarios ... Passed
3/3 test_solver_preferencias ... Passed
100% tests passed out of 3

$ ./build-backend/test/test_solver_preferencias
[==========] 11 tests from 1 test suite ran.  [  PASSED  ] 11 tests.   (149 ms)
```

Extra (no exigido por T5, para no romper T4/T6): `full-release` compila y
`test_generacion_seed` + `test_solver_horarios` pasan en `build-rel` (63 binarios; el seed sigue
con 0 preferencias y por tanto ejecuta solo la fase 1).

### Tabla de casos

| Caso | Resultado |
|---|---|
| `sinPreferencias_salidaIdenticaALaActual` | golden del horario de la referencia (RNF-4), sin fase 2 |
| `mismasPreferencias_mismaAsignacionDocenteYExceso` | misma asignación `(curso,materia)→docentes` y mismo exceso (calculado en el test con `horas_aula`, P14); el horario sí cambia (control) |
| `evitada_mueveLaClaseFueraDeLaFranja` | evita las franjas exactas de la referencia → ninguna clase queda en ellas |
| `preferida_atraeLaClase` | las dos clases acaban en las franjas preferidas |
| `dosPreferenciasSolapadasMismoSentido_cuentanUnaVez` | `{0,1}` + `{1,2}` ≡ una sola `{0,1,2}`: misma salida |
| `evitadaAcumulada_noProvocaSuplente` | 5 h evitadas: las 5 clases siguen en el titular |
| `evitadaAcumulada_noProvocaExceso` | y el exceso del titular sigue en 0 |
| `preferenciaCedeSiNoHayAlternativa` | todo evitado: factible, 2 clases; nunca infactible |
| `preferenciaNoRompeInvariantesDuras` | exclusividad de curso/docente/aula por franja, turno, D3 y docente que opta por la materia |
| `ordenDeDeclaracionNoInfluye` | mismo horario con las preferencias en otro orden |
| `fase2SinSolucion_devuelveLaReferencia` | fase 2 sin solución por presupuesto → se devuelve la referencia, no un error |

Además, `test_contrato_solver` gana `constantes_jerarquiaConPreferencia` (`PESO_PREFERENCIA == 25`
y `PESO_AGRUPACION < PESO_PREFERENCIA < COSTE_SUPLENTE < PESO_HORAS`).

### Cómo quedó `resolver`

```cpp
struct PresupuestoFases { double referencia = 6.0; double preferencias = 2.0; };
ResultadoSolver resolver(const SolverConfig& config);
ResultadoSolver resolver(const SolverConfig& config, const PresupuestoFases& presupuesto);
```

1. **Fase 1 (referencia):** `prepararModelo` (reinicia `model`/`vars`, `crearVariables` +
   `agregarRestricciones`) + `construirObjetivo` (S1+S2+S3, sin cambios) y `ejecutar(6.0)` con 1
   worker, seed 0. Si no es factible → `analizarFallo` (comportamiento de hoy). Se extrae el
   `HorarioSalida` **en ese momento**, con `vars` correspondiendo a la respuesta.
2. **Sin preferencias:** se devuelve la referencia tal cual (no se construye la fase 2).
3. **Fase 2a:** `extraerAnclaje` (z* por `(p,c,m)` y e* por docente desde la respuesta 1) →
   `prepararModelo` → `fijarAnclaje` (`AddEquality` para cada z y cada e) →
   `Minimize(expresionPreferencias)` → `ejecutar(2.0)`.
4. **Fallback:** `INFEASIBLE`/`UNKNOWN` → se devuelve el horario de la referencia. `MODEL_INVALID`
   y cualquier otro estado → `analizarFallo` y `exito = false` (se propaga).
5. `tiempo_ms` y `metadata.tiempo_ejecucion_ms` son el total de las fases ejecutadas.

### Término de preferencias y deduplicación

```cpp
LinearExpr Solver::expresionPreferencias(const SolverConfig& config) const;
```

- Dos conjuntos de claves `(docente, día, slot)` —evitadas y preferidas— construidos desde
  `config.preferencias`; una franja alcanzada por dos preferencias del mismo sentido entra **una
  vez**. Una franja en ambos sentidos se suma y se resta (se compensa).
- Recorre `vars.assignment` y añade `+PESO_PREFERENCIA·x` para cada clase en franja evitada y
  `−PESO_PREFERENCIA·x` en franja preferida (mismo peso, unidad por franja).
- `PESO_PREFERENCIA = 25` en `contrato_solver.hpp` con tres `static_assert` del orden
  S1 > S2 > preferencia > S3.

### Archivos tocados

- `src/backend/include/backend/solver/config/contrato_solver.hpp` — `PESO_PREFERENCIA` y
  `static_assert` de la jerarquía.
- `src/backend/include/backend/solver/solver.hpp` — `PresupuestoFases`, `AnclajeEstructural` y los
  métodos privados de fase/anclaje/expresión.
- `src/backend/src/solver/solver.cpp` — fases, `esFactible`, anclaje por igualdad, deduplicación y
  fallback; `FranjaPreferencia` como clave con `qHash`.
- `test/CMakeLists.txt` — `add_gtest(test_solver_preferencias ... backend_solver)`.
- `test/backend/test_solver_preferencias.cpp` (**nuevo**), `test/backend/test_contrato_solver.cpp`.

### Hallazgos / desviaciones

- **Target CMake `backend_solver`, no `backend`.** El plan decía «`add_gtest` en el bloque
  backend», pero el test incluye `solver.hpp` y necesita las cabeceras de OR-Tools, que solo expone
  `backend_solver` (como `test_solver_horarios` y `test_solver_benchmark`). Se registra en el bloque
  `if(TARGET backend_solver)`; la función sigue siendo `add_gtest`.
- **`QSet` con clave propia en vez de `QSet<std::tuple<int,int,int>>`.** Qt no ofrece `qHash` para
  `std::tuple` (la compilación falla con «The key type must have a qHash overload»). Se usa
  `FranjaPreferencia{docente, dia, slot}` con `operator==` y `qHash` propios —el mismo patrón que
  `FranjaSemana` de la 005—, que es exactamente la clave `(docente, día, slot)` que pide la tarea.
- **`PresupuestoFases` (sobrecarga aditiva).** Con el anclaje correcto la fase 2 es **factible por
  construcción** (la solución de la referencia la satisface), así que `INFEASIBLE` no es alcanzable
  y el único camino real al fallback es que se agote el presupuesto (`UNKNOWN`). Para poder
  ejercitarlo sin tocar los valores de producción se añade una sobrecarga con presupuesto inyectable
  (por defecto 6.0/2.0, los del plan); T6 necesitará lo mismo para probar la escalera 2a→1/2b→2a.
  La prueba usa `preferencias = 1e-9` y **comprueba que la salida es la referencia**: una fase 2
  factible habría movido las clases, como demuestra `preferida_atraeLaClase`.
- **Tiempos medidos** (fixture de 1 curso, `configDosDias`): referencia **6 ms**; con preferencias
  (fase 1 + fase 2a) **4 ms** en la misma tanda —diferencias de pocos milisegundos, dominadas por el
  arranque del solver, no por las fases—.
- **Sin fase 2b ni desempate por S3** (T6): la fase 2a ignora S3 a propósito. Sin estado por
  preferencia (T7) ni documentación (T8/T9). `seed.md` intacto y sin commits.

---

## T6 - Jerarquía preferencia > S3 (fase 2b), presupuesto y medición

**Fecha:** 2026-11-06
**Requisito:** RF-5, RNF-2, RNF-3. **Plan:** §1.5, §3 y §4-T6.

### Rojo (tests primero)

Tres casos nuevos en `test_solver_preferencias.cpp` y el escenario con preferencias en
`test_solver_benchmark.cpp`, escritos antes de la fase 2b:

```text
$ cmake --build --preset backend --parallel --target test_solver_preferencias test_solver_benchmark
test_solver_preferencias.cpp:510:15: error: ‘struct Solver::PresupuestoFases’ has no member named ‘agrupacion’
```

### Verde

```text
$ ./build-backend/test/test_solver_preferencias
[==========] 14 tests from 1 test suite ran.  [  PASSED  ] 14 tests.

$ ./build-backend/test/test_solver_benchmark
=== BENCHMARK CON PREFERENCIAS ===
Referencia (sin preferencias): 3937 ms
Con preferencias (fase 1 + 2a + 2b): 4676 ms / 4658 ms
Sobrecoste frente a la referencia: 18.7706 % (umbral +50 %, se documenta en journal.md)
Totals: 5 passed, 0 failed, 0 skipped, 0 blacklisted, 21256ms

$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
58/58 Test #58: test_ipc_aislamiento_bd ...............   Passed
100% tests passed out of 58        Total Test time (real) = 28.11 sec
```

Extra (no exigido por T6, para no romper T4/T10): `full-release` compila y
`test_generacion_seed` + `test_solver_horarios` pasan en `build-rel`.

### Fase 2b y escalera

```text
# FASE 2a (T5): anclaje z*/e* + minimizar la expresión de preferencias
# FASE 2b:
prepararModelo(config); fijarAnclaje(anclaje)
model.AddEquality(expresionPreferencias(config), valorPreferencia2a)
construirObjetivoSoloS3()          # PESO_AGRUPACION · Σ isolated
fase2b = ejecutar(1.0)
```

- `valorPreferencia2a` se toma de la respuesta de la 2a con
  `static_cast<int64_t>(std::llround(fase2.objective_value()))` (la expresión es entera).
- La solución de la 2a se extrae **antes** de reconstruir el modelo (los índices de variable
  cambian), y la de la 2b la sustituye solo si es utilizable.
- **Escalera:** 2a `INFEASIBLE`/`UNKNOWN` → referencia; 2b `INFEASIBLE`/`UNKNOWN` → 2a. Solo
  `MODEL_INVALID` u otro estado se propaga vía `analizarFallo` con `exito = false`.
- `construirObjetivoSoloS3` reproduce **exactamente** el término S3 de la fase 1 (hoy suma solo
  `isolated`; `longGroup` se crea pero no entra en el objetivo — hallazgo anotado, fuera de
  alcance cambiarlo).
- Presupuestos: referencia 6.0 · preferencias 2.0 · agrupación 1.0 (§3), inyectables para probar
  la escalera sin tocar producción.

### Casos nuevos

| Caso | Qué fija |
|---|---|
| `preferenciaGanaALaAgrupacion` | conflicto directo: preferir {0,3} deja las dos clases aisladas (S3 las penaliza) y aun así **gana la preferencia**; el horario acaba exactamente en las franjas preferidas |
| `fase2b_mejoraLaAgrupacionSinCambiarLaPreferencia` | con la 2b agotada y sin ella: mismo cumplimiento (ninguna clase en franjas evitadas) y `aisladas(con2b) ≤ aisladas(solo2a)` |
| `fase2bAgotada_devuelveLaFase2a` | 2b→2a: la solución sigue siendo la de la 2a (preferencia cumplida), ni referencia ni error |
| `benchmark_SolverGrandeConPreferencias` | escenario grande con 20 preferencias: techo de 60 s asertado, **mismo horario en dos ejecuciones** y sobrecoste registrado |

### Medición (RNF-3)

| Escenario | Tiempo |
|---|---|
| Referencia sin preferencias (10 prof, 15 materias, 6 aulas, 8 cursos, 5 días, 6 franjas) | **3937 ms** (y 3973 ms en el benchmark previo) |
| Con preferencias (fase 1 + 2a + 2b) | **4676 ms** / **4658 ms** |
| Sobrecoste | **+18,8 %** (umbral +50 %: se documenta, no se aserta) |
| Techo | < 60 s (asertado en las dos ejecuciones) |
| Determinismo | dos ejecuciones seguidas → mismo resumen de horario (RNF-2) |

### Archivos tocados

- `src/backend/include/backend/solver/solver.hpp` — `PresupuestoFases.agrupacion = 1.0` y
  `construirObjetivoSoloS3`.
- `src/backend/src/solver/solver.cpp` — fase 2b con igualdad del valor de preferencia, extracción
  ordenada de soluciones y escalera 2b→2a→referencia.
- `test/backend/test_solver_preferencias.cpp` — 3 casos y el helper `clasesAisladas`.
- `test/backend/test_solver_benchmark.cpp` — `crearConfigGrandeConPreferencias`,
  `resumenDeSalida` y `benchmark_SolverGrandeConPreferencias`.

### Hallazgos / desviaciones

- **Sin desviaciones de alcance** respecto de T6. No se toca el estado por preferencia (T7) ni la
  documentación (T8/T9); `seed.md` intacto y sin commits.
- **`objective_value()`** se usa para el término independiente de la igualdad de la 2b: es la suma
  entera de la expresión de preferencias de la mejor solución de la 2a y se redondea antes de
  pasarla a `AddEquality`.
- **`longGroup` no está en el objetivo** del motor (se crea en `crearVariables` y no se usa). La 2b
  minimiza el S3 **vigente** (solo `isolated`) para no cambiar el comportamiento de la fase 1 ni de
  la referencia; queda anotado como observación ajena a esta spec.
- **El benchmark tarda ~21 s** (referencia + dos ejecuciones con preferencias). El techo de 60 s se
  aserta; el +50 % solo se documenta, como pide la tarea.

---

## T7 - Estado por preferencia y cumplimiento en el resultado del backend

**Fecha:** 2026-11-06
**Requisito:** RF-8, RNF-1. **Plan:** §1.7 y §4-T7.

### Rojo (tests primero)

Casos nuevos en `test_servicio_generacion.cpp` antes del módulo:

```text
$ cmake --build --preset backend --parallel --target test_servicio_generacion
test_servicio_generacion.cpp:13:10: fatal error:
  backend/services/preferencias_cumplimiento.hpp: No existe el fichero o el directorio
```

### Verde

```text
$ ./build-backend/test/test_servicio_generacion
Totals: 34 passed, 0 failed, 0 skipped, 0 blacklisted

$ ctest --test-dir build-backend -R '^(test_servicio_generacion|test_proyeccion_preferencias)$'
1/2 test_servicio_generacion ... Passed
2/2 test_proyeccion_preferencias ... Passed

$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
58/58 Test #58: test_ipc_aislamiento_bd ...............   Passed
100% tests passed out of 58        Total Test time (real) = 29.31 sec
```

Extra: `full-release` compila (sin avisos nuevos en los ficheros tocados) y `test_generacion_seed`
pasa en `build-rel`.

### API y estructura

```cpp
enum class EstadoPreferencia { NoAplicada, Incumplida, Respetada };

struct ClaseIncumplida { QString curso; int materia; int slot; };   // curso = "anio|seccion"

struct PreferenciaEstado {
    int docente; int dia;              // día de DOMINIO (0=domingo … 6=sábado)
    QVector<int> slot;                 // franjas alcanzadas (vacío en las no aplicadas)
    QString sentido;                   // "preferida" | "evitada"
    EstadoPreferencia estado;
    MotivoPreferencia motivo;          // Ninguno salvo en las no aplicadas
    QVector<ClaseIncumplida> clasesIncumplidas;
};

struct CumplimientoPreferencias {
    QVector<PreferenciaEstado> porPreferencia;
    int respetadas = 0, incumplidas = 0, noAplicadas = 0;
};

CumplimientoPreferencias analizarPreferencias(const SolverConfig& config,
                                              const HorarioSalida& horario,
                                              const QVector<PreferenciaNoAplicada>& noAplicadas);
```

- Precedencia: **no aplicada** (viene de `noAplicadas`, con su motivo) → **incumplida** →
  **respetada**. Evitada incumplida si alguna de sus franjas lleva una clase **de su docente**;
  preferida incumplida si ninguna la lleva.
- `motivo = Ninguno` en respetadas e incumplidas; solo las no aplicadas llevan (a)/(b)/(c).
- Orden canónico por `(docente, día, sentido, franjas)`; agregado con los tres totales.

### Día de dominio y clases incumplidas

- **Día:** las intervinientes llegan con el día de la **generación** (0 = lunes) y el estado sale
  con `diaDominio = diaGeneracion + 1` (0 = domingo … 6 = sábado). Las no aplicadas conservan la
  franja de dominio que ya trae el diagnóstico. El día de generación no sale del módulo.
- **Clases incumplidas:** se calculan en `clasesEnFranjas(horario, docente, dia, franjas)`: recorre
  el horario y toma las clases con `profesor == docente`, en ese día y en una franja alcanzada;
  para cada una informa `{curso ("anio|seccion"), materia, slot}`, ordenadas por curso, franja y
  materia. Solo se rellenan en las **evitadas incumplidas**; una preferida sin clases deja la lista
  vacía (no hay clases que listar).

### Servicio y consumidores

- `void generar(const QJsonObject&, const QString&, const QVector<PreferenciaNoAplicada>& = {});`
  guarda el diagnóstico en `m_noAplicadas`.
- `ResultadoGeneracion` gana `CumplimientoPreferencias cumplimiento;` (**aditivo**) y se rellena en
  `alResponder` tras `analizarSalidaSolver`.
- **El contrato de salida del solver no cambia:** `cumplimiento` no viaja en el JSON de salida.
- **Cableado del único consumidor de producción** (plan §1.6, `:974`): la vista guarda
  `m_preferenciasNoAplicadas` de la última composición y la pasa a `generar`; sin esto el
  diagnóstico de las no aplicadas nunca llegaría al resultado. Sin vistas ni widgets nuevos.

### Casos nuevos

`cumplimiento_evitadaRespetada` · `cumplimiento_evitadaIncumplidaListaLasClasesOcupadas` ·
`cumplimiento_evitadaParcialmenteOcupadaEsIncumplida` · `cumplimiento_preferidaSinClaseEsIncumplida` ·
`cumplimiento_preferidaConClaseEsRespetada` · `cumplimiento_soloCuentanLasClasesDelPropioDocente` ·
`cumplimiento_noAplicadasConservanSuMotivo` · `cumplimiento_totalesYordenCanonico` ·
`servicio_cumplimientoEnElResultado` · `servicio_sinPreferencias_cumplimientoVacio`.

### Archivos tocados

- **Nuevo** `src/backend/include/backend/services/preferencias_cumplimiento.hpp` y `.cpp` (puro).
- `src/backend/CMakeLists.txt` — `preferencias_cumplimiento.cpp` en el target `backend`.
- `src/backend/include/backend/services/ServicioGeneracion.hpp` — `cumplimiento` en
  `ResultadoGeneracion`, tercer argumento de `generar` y `m_noAplicadas`.
- `src/backend/src/services/ServicioGeneracion.cpp` — guarda el diagnóstico y calcula el
  cumplimiento tras analizar la salida.
- `src/frontend/src/views/schedule_generation_widget.{hpp,cpp}` — `m_preferenciasNoAplicadas` y su
  paso a `generar` (§1.6).
- `test/backend/test_servicio_generacion.cpp` — helpers de horario/preferencia y diez casos.

### Hallazgos / desviaciones

- **Orden canónico y no aplicadas:** la no aplicada (día de dominio) puede quedar antes que una
  interviniente (día de generación + 1); la primera versión de
  `servicio_cumplimientoEnElResultado` asumía lo contrario y se corrigió al ver el orden real
  (día 0 de la no aplicada frente al día 1 de la interviniente).
- **Sentido desconocido:** un valor distinto de «evitada» se trata como «preferida», coherente con
  el objetivo; V19 lo rechaza en el camino normal.
- **`slot` vacío en las no aplicadas:** el diagnóstico no transporta franjas alcanzadas (no
  intervinieron), así que no se inventan.
- Sin estado en la salida del solver (RF-8), sin documentación (T8/T9), `seed.md` intacto y sin
  commits.

---

## T8 - Documentación del contrato y de la constitución

**Fecha:** 2026-11-06
**Requisito:** RF-9. **Plan:** §1.8 y §4-T8.

### Cambios

**`docs/contrato-solver-2.0.md`**

- Cabecera: el responsable del comportamiento añade la **spec 013**.
- §1: la cita del catálogo completo pasa a **V1–V19**.
- §2: «El contrato tiene **diez secciones obligatorias y dos opcionales** (`aula_fija` y
  `preferencias`)»; fila nueva de `preferencias` en la tabla (arreglo opcional, forma ya proyectada,
  **V19**); nota de que **`meta` no es una sección** (objeto tolerado que añade el constructor).
- §2.8 nueva, «`preferencias` (opcional, spec 013)»: forma (`docente` por índice, `dia` de la
  generación, `slot[]`, `sentido`), sin rangos de reloj; **proyección** (bloque entero, todos los
  turnos, los eventos no cortan, filtro de día dominio → generación con domingo fuera y sábado solo
  si `num_dias` lo incluye); **solo viajan las intervinientes**; **el preset no las aporta** (se
  omiten al guardar y se ignoran al cargar; la ida y vuelta es de la configuración y con 0
  preferencias la sección se omite del JSON); **V19** valida forma y referencias y **no** rangos de
  reloj, coherencia con el turno, plan, años ni solapes.
- §6: el contrato de salida no incluye el estado de las preferencias (lo calcula el backend).
- §9: cita del catálogo a **V1–V19** y referencia nueva a la spec 013.

**`docs/constitution.md`**

- **§7.2**: nota de enmienda (2026-11-06) que reconoce que solo figuraban S1 y S2 y que S3 no estaba;
  lista completa **S1 / S2 / preferencia / S3**, con la preferencia **blanda** (no altera la
  factibilidad, no elige docente, no cambia el exceso; con las mismas preferencias, la asignación de
  docente por materia-curso y el exceso son los de la referencia y la preferencia solo decide dónde
  caen las clases) y el orden `PESO_HORAS > COSTE_SUPLENTE > PESO_PREFERENCIA > PESO_AGRUPACION`
  (1000 > 100 > 25 > 10).
- **§7.4**: **V1–V19** y párrafo de **V19** (forma y referencias de la sección opcional
  `preferencias`; no valida rangos de reloj ni coherencia con el turno, plan o años), con el detalle
  en la 013.
- **§7.5**: nota de las preferencias que **no intervienen** (no viajan, no entran en la huella, no
  son causa de no factibilidad; se informan como no aplicadas) y de que nunca convierten en elegible
  a un docente, no cambian el reparto ni el exceso y no actúan fuera de la semana declarada.

### Verificación

```text
$ grep -n "diez secciones obligatorias y dos opcionales" docs/contrato-solver-2.0.md
54:El contrato tiene **diez secciones obligatorias y dos opcionales** (`aula_fija` y `preferencias`). El

$ grep -n "preferencias" docs/contrato-solver-2.0.md
16, 54, 69, 215, 218, 235, 240, 242, 243, 418, 527

$ grep -n "V1–V19" docs/contrato-solver-2.0.md docs/constitution.md
docs/contrato-solver-2.0.md:28:  | `"version": "2.0"` | se acepta y se valida el resto (V1–V19) | — |
docs/contrato-solver-2.0.md:519: - Núcleo del solver … validaciones V1–V19:
docs/constitution.md:95: Antes de construir el modelo se ejecutan las validaciones V1–V19; …

$ grep -n "PESO_PREFERENCIA" docs/constitution.md
76:3. **Preferencia horaria del docente** (`PESO_PREFERENCIA`): …
79:`PESO_HORAS`, `COSTE_SUPLENTE`, `PESO_PREFERENCIA` y `PESO_AGRUPACION` son constantes …

$ grep -n "meta" docs/contrato-solver-2.0.md
65 (metadata de eventos), 73 (`meta` no es una sección), 275, 392, 482
```

### Hallazgos / desviaciones

- **Autorización expresa para la constitución.** Mi rol de implementador tiene denegada la edición de
  `docs/constitution.md` y ordena parar y avisar; T8 la exige. Se paró, se dejó el parche redactado en
  el journal y el coordinator **autorizó** su aplicación; solo entonces se editó. Queda constancia
  para que el cambio normativo no parezca unilateral.
- Sin cambios de código, tests ni `seed.md`; **no** se tocan las supersesiones de las specs (T9).

---

## T9 - Supersesiones, acotación de la equivalencia y riesgo documental

**Fecha:** 2026-11-06
**Requisito:** RF-9, RNF-4. **Plan:** §1.8 y §4-T9.

### Cambios

Solo notas aditivas de supersesión/cita; **no se ha reescrito ningún RF**.

**`specs/005-nucleo-solver/spec.md`**

- Alcance (:128): la cita del catálogo pasa a **V1–V19**; la nota de supersesión del rango añade la
  013 y deja «V1 a V15» como histórico sin confundirlo con el catálogo vigente.
- «Configuración equivalente» (:308-312): nota de que, para la sección opcional `preferencias`, la
  equivalencia es de la **configuración** y **no del preset** (el preset la omite al guardar y la
  ignora al cargar; sin preferencias no se emite).
- «Causa de un resultado no factible» y **RF-11**: catálogo a **V1–V19**; nota nueva de la 013 con
  V19 (forma y referencias de `preferencias`, sin rangos de reloj ni coherencia con el turno) y
  remisión a la 013 y a `docs/constitution.md` §7.4.
- Criterio de finalización 4: V1–V19 y la 013 en la nota de rango.
- «Fuera de alcance» (:804-810): **riesgo documental anotado y sin resolver** — las **dos specs
  «009»** (`009-tipos-de-aula-y-eventos` y `009-agrupacion-bloques-materias`) y el **doble uso de
  «S3»** (cobertura de emergencia en la 005 frente a agrupación en la 009-agrupacion; en la 013, S3
  es la agrupación). No se renumera ni se renombra.

**`specs/010-solver-tipos-y-eventos/spec.md`**

- Nota de enmienda de la constitución (:64-65): el rango histórico se escribe «V1 a V18» y se añade
  que hoy es **V1–V19** (013).
- **RF-1, equivalencia de ida y vuelta** (:222-226): nota de que para `preferencias` la equivalencia
  es de la configuración y no del preset.
- **RF-5**: las validaciones previas de la 005 se citan como «V1 a V15 entonces; hoy V1–V19».
- **RF-7** (actualización del texto de la 005): el catálogo pasa a **V1–V19** (la 013 añade V19).
- **RF-8** (§7.4): nota de que hoy el catálogo es V1–V19 con V19 de la 013.
- **RNF-4**: la cita histórica pasa a «V1 a V15».
- Criterio de finalización 8: «V1 a V18; hoy V1–V19 por la 013».

**`specs/012-preferencias-docente/spec.md`**

- **RF-5**: nota de que queda **superado por la 013** (las preferencias intervinientes ya entran en
  la huella y un cambio suyo marca «datos anteriores»), incluido el caso límite «Cambio de
  preferencias durante una generación», que lleva su propia nota.
- **«Fuera de alcance»**: nota de que los dos primeros puntos («uso por el solver, peso y prioridad»
  y «contrato de configuración y generación») dejan de estar fuera de alcance; el modelo persistido
  y sus servicios siguen siendo de la 012 y la interfaz sigue fuera.

**`specs/009-agrupacion-bloques-materias/spec.md`**

- Nota tras la cabecera: la **preferencia horaria** va **por encima de S3** y por debajo de S2
  (`PESO_AGRUPACION < PESO_PREFERENCIA < COSTE_SUPLENTE`; hoy 10 < 25 < 100) y la jerarquía se
  impone por fases; **la definición de S3 no cambia** (esta spec sigue siendo su fuente). Remite al
  riesgo del doble «S3» y de las dos «009» anotado en la 005 y en la 013.

### Verificación

```text
$ grep -rn "013" specs/005-nucleo-solver/spec.md specs/010-solver-tipos-y-eventos/spec.md \
    specs/012-preferencias-docente/spec.md specs/009-agrupacion-bloques-materias/spec.md
specs/005-nucleo-solver/spec.md:134,136,308,311,330,601,611,614,807,811,862,865,866
specs/010-solver-tipos-y-eventos/spec.md:65,222,225,394,417,597
specs/012-preferencias-docente/spec.md:150,152,238,245,246
specs/009-agrupacion-bloques-materias/spec.md:7,10,13

$ grep -rn "V1–V15\|V1–V18" specs/005-nucleo-solver/spec.md specs/010-solver-tipos-y-eventos/spec.md
sin citas obsoletas

$ git diff --stat -- specs/ docs/
 docs/constitution.md                             |  12 +-      (T8, ya verificado)
 docs/contrato-solver-2.0.md                      |  59 +-      (T8, ya verificado)
 specs/005-nucleo-solver/spec.md                  |  59 +-      (T9)
 specs/009-agrupacion-bloques-materias/spec.md    |   8 +       (T9)
 specs/010-solver-tipos-y-eventos/spec.md         |  28 +-      (T9)
 specs/012-preferencias-docente/spec.md           |  14 +       (T9)
 specs/013-preferencias-docente-solver/journal.md | 809 +++++--- (evidencia)
 specs/013-preferencias-docente-solver/tasks.md   |  18 +-      (casillas)
 8 files changed, 914 insertions(+), 93 deletions(-)
```

### Hallazgos / desviaciones

- **Citas históricas sin el rango literal:** las notas que cuentan qué decía un rango anterior usan
  «V1 a V15»/«V1 a V18» para no dejar citas obsoletas que el grep de aceptación confundiría con el
  catálogo vigente. El catálogo vigente se escribe siempre **V1–V19**.
- `docs/` no se toca en T9 (es de T8); no se tocan código, tests ni `seed.md`.

---

## T10 - Regresión global y suite completa

**Fecha:** 2026-11-06
**Requisito:** RNF-4, RNF-5, RNF-7. **Plan:** §4-T10.

### Regresión sin preferencias

La identidad con «el mismo árbol sin la 013» se fija con tres goldens capturados **antes** de
introducir el consumo de preferencias, más la comprobación añadida del escenario del seed:

| Qué | Dónde | Evidencia |
|---|---|---|
| JSON de la entrada con 0 preferencias | `test_constructor_entrada::sinPreferencias_jsonIdenticoAlDeHoy` (T3) | el envoltorio devuelve el mismo objeto que la completa; la entrada conserva **exactamente las doce claves** de hoy y **no** gana `preferencias`; la configuración resultante tiene 0 preferencias |
| Huella sin intervinientes | `test_servicio_generacion::huellaDatos_sinIntervinientes_conservaElValorDeHoy` (T4) | hash **`3c50f3abc8e7c294815cb32bf6e80b8837265950341f6a07fbbe576fd0ed1330`**, capturado con el binario previo a la 013 para el mismo `dominio()` y D = 35 |
| Horario del solver sin preferencias | `test_solver_preferencias::sinPreferencias_salidaIdenticaALaActual` (T5) | resumen **`1|A|1|2|0|0|0\n1|A|1|3|0|0|0`**, capturado con el motor previo; `test_solver_horarios` (sin tocar) sigue verde |
| Escenario del seed (posterior a la 015/016) | `test_generacion_seed` (frontera, comprobación añadida en T10) | el seed siembra **0 preferencias**: la entrada compuesta **no** declara `preferencias` y `preferenciasNoAplicadas` está vacía; el resto de la prueba (builder → preset → solver → visualización) sigue verde |

Con 0 preferencias el solver ejecuta **solo la fase 1** (el modelo de hoy: S1 + S2 + S3, mismo
objetivo y mismo presupuesto de 6.0 s), así que no hay cambio de horario ni de coste; y `toJson` omite
la sección vacía, de modo que el JSON es el mismo. No se comparó contra un archivo de `bb1767a`
(ancla supersedida por la re-base del 2026-11-05: escenario posterior a la 015/016).

### Suite

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
58/58 Test #58: test_ipc_aislamiento_bd ...............   Passed
100% tests passed out of 58        Total Test time (real) = 26.50 sec

$ cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
63/63 Test #63: test_generacion_seed ..................   Passed   20.70 sec
100% tests passed out of 63        Total Test time (real) = 50.72 sec
```

**58/58 backend** y **63/63 full-release** (58 del backend + los 5 binarios de interfaz que solo
compilan y corren en la suite completa: `test_frontend_conversion`, `test_cuadricula_horario`,
`test_tipos_aula_ui`, `test_selector_generacion` y `test_generacion_seed`). Compilación sin errores.

### Medición de T6 (referencia vs con preferencias)

Escenario grande del benchmark (10 docentes, 15 materias, 6 aulas, 8 cursos, 5 días × 6 franjas),
medido en T6 y vigente para el cierre:

| Escenario | Tiempo |
|---|---|
| Referencia sin preferencias | **3937 ms** (3973 ms en `benchmark_solverGrandeResuelve`) |
| Con preferencias (fase 1 + 2a + 2b) | **4676 ms / 4658 ms** |
| Sobrecoste | **+18,8 %** (umbral +50 %: documentado, no asertado) |
| Techo | **< 60 s** asertado en las dos ejecuciones |
| Determinismo | dos ejecuciones seguidas → **mismo horario** (RNF-2) |

Los tiempos del escenario pequeño de `test_solver_preferencias` son de milisegundos: referencia 6 ms
y fase 1 + 2a 4 ms en la misma tanda.

### Cierre

Tabla final de la spec 013 (diez tareas, todas con su verificación en verde):

| Tarea | Estado | Evidencia |
|---|---|---|
| T1 · contrato, V19 y preset sin la sección | hecha | 3 binarios verdes; sección opcional, `toJson` la omite vacía, V19 con orden docente→día→franja→sentido, `PESO_AGRUPACION == 10` |
| T2 · rejilla compartida y proyección | hecha | `test_proyeccion_preferencias` (17 casos); `test_constructor_entrada` verde |
| T3 · construcción de la entrada | hecha | 5 casos nuevos; preset sobrescrito; golden de 0 preferencias; entrada válida por V19 |
| T4 · huella con intervinientes y frontend | hecha | golden `3c50f3…`; intervinientes y orden; `Composicion.huella`; `huellaActual(nucleo, opciones)` |
| T5 · fases 1/2a, anclaje y deduplicación | hecha | 11 casos nuevos (14 en el binario tras T6); igualdad de docente/exceso; solapadas que cuentan una vez; fallback |
| T6 · fase 2b, presupuestos y medición | hecha | 3 casos nuevos (14 en el binario); preferencia > S3; escalera 2b→2a→1; +18,8 % (< +50 %) y techo de 60 s |
| T7 · estado por preferencia y cumplimiento | hecha | `analizarPreferencias` con 10 casos nuevos (34 en el binario); día de dominio; clases incumplidas; tres totales en el resultado |
| T8 · contrato y constitución | hecha | contrato con diez + dos secciones, `meta` fuera, V1–V19; §7.2 S1/S2/preferencia/S3, §7.4 V1–V19, §7.5 no aplicadas |
| T9 · supersesiones y riesgo documental | hecha | notas en 005/010/012/009-agrupacion; sin citas V1–V15/V1–V18 en 005/010; riesgo de las dos «009» y del doble «S3» anotado |
| T10 · regresión y suites | hecha | 58/58 backend y 63/63 full-release; goldens sin preferencias; seed sin la sección |

**Estado de la spec:** implementación **completa** (T1–T10). La spec **no** se marca como
`implementada`: eso corresponde al cierre tras el APROBADO del revisor. Sin commits.

---

## Validación (2026-11-06) - corrección de H2: documentación del frontend

**Hallazgo H2.** `docs/interfaz-frontend.md` y `docs/guias/migracion-frontend.md` seguían describiendo
la API anterior a la 013 (huella de dos argumentos, `construirJsonEntrada` como camino del builder,
`Composicion` sin `huella`/diagnóstico, preset de «13 secciones»). Solo documentación: **sin cambios de
código ni de tests**.

### `docs/interfaz-frontend.md`

- **§2.4 (preferencias):** el «sin efecto en la generación» pasa a «efecto en la generación (013)»:
  las intervinientes entran en la huella y un cambio suyo marca «datos anteriores»; el resultado
  informa el cumplimiento.
- **§3.6 (`ServicioGeneracion`):** `ResultadoGeneracion` muestra `CumplimientoPreferencias cumplimiento;`
  y `generar` gana el tercer argumento con el diagnóstico de no aplicadas. El párrafo de construcción
  pasa a `construirEntradaCompleta` → `EntradaConstruida{json, intervinientes, noAplicadas}` y a
  **V1–V19**, con `construirJsonEntrada` como **envoltorio**; se añade el párrafo «Preferencias en la
  generación» (cumplimiento aditivo; el contrato de salida del solver no cambia).
- **§3.2:** el preset **omite** `preferencias` y el contrato tiene **diez secciones obligatorias y dos
  opcionales**; `meta` no es sección.
- **§3.7 (builder):** `Composicion{entrada, contexto, avisos, huella, preferenciasNoAplicadas}` y
  `construirEntradaCompleta`; la vista describe el preset con las diez + dos secciones y la
  sobrescritura de la duración por la construcción.
- **§4 (tabla):** fila del constructor actualizada y fila nueva de `Composicion::huella` /
  `huellaActual(nucleo, opciones)`.
- **§6 (deuda de la 011):** puntos 1 y 2 reescritos al camino vigente (013) y nota de actualización
  2026-11-06 con las suites (58/58 y 63/63).

### `docs/guias/migracion-frontend.md`

- Título y resumen incluyen la **013**, con fila nueva en la tabla de cambios.
- **§2.1:** el ejemplo usa `construirEntradaCompleta` y compone `entrada`, `huella`
  (`huellaDatos(datos, D, intervinientes)`) y `preferenciasNoAplicadas`; se explicita que
  `construirJsonEntrada` queda como envoltorio.
- **Punto 7 y checklist:** `huellaDatos(datos, D, intervinientes)` vía `huellaActual(nucleo,
  opciones)`, y `generar` con el diagnóstico de no aplicadas.

### Verificación

```text
$ grep -n "construirEntradaCompleta\|Composicion\|huellaDatos\|cumplimiento" docs/interfaz-frontend.md | head -20
484 huellaDatos(datos, D, intervinientes) · 487 cumplimiento · 658 CumplimientoPreferencias cumplimiento
688 construirEntradaCompleta · 692 huellaDatos(datos, D, intervinientes) · 697 Composicion::preferenciasNoAplicadas
698 ResultadoGeneracion::cumplimiento · 769 Composicion{entrada, contexto, avisos, huella, …
770 construirEntradaCompleta · 803 construirEntradaCompleta · 892 construirEntradaCompleta(datos, preset, D, ahora)
897 Composicion::huella / huellaActual(nucleo, opciones) · 967–975 construirEntradaCompleta y huella
992–996 Actualización 2026-11-06 (013) con cumplimiento

$ grep -n "huellaDatos\|construirEntradaCompleta\|013" docs/guias/migracion-frontend.md | head -10
1 (009–013) · 13 (specs …, 013, …) · 27 (fila 013) · 44 (§2.1 vigente desde la 013)
52–64 (ejemplo y firma con construirEntradaCompleta y huellaDatos(datos, D, intervinientes))
126 (punto 7: huella con intervinientes)

$ grep -nE "huellaDatos\([^,()]+, [^,()]+\)" docs/interfaz-frontend.md docs/guias/migracion-frontend.md
sin huellaDatos de dos argumentos
```

No quedan referencias a `huellaDatos(datos, D)` de dos argumentos; `construirJsonEntrada` solo
aparece etiquetado como **envoltorio** o en la comparación histórica «ANTES» de la guía.

