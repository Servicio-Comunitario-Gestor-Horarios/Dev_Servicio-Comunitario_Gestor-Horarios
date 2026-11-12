# Plan 013 - Preferencias del docente en la generación

Base: **`c7b9427`** (HEAD tras committear la **015** y la **016**: esquema **v6**, fila del tipo general,
disparadores, comprobación en tres momentos, ayuda estricta, V18, huella y docs) **+ la 013 en el árbol de
trabajo** (la implementación no se ha commiteado todavía). El `Base` original del plan era
`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`; la re-base del 2026-11-05 lo sustituye por `c7b9427`.
**El ancla de RNF-4 y del criterio 11 es el escenario posterior a la 015/016** —seed sin «Aula de
estudio», base **v6** con el tipo general garantizado—, no el escenario de `bb1767a`.

> **Alcance.** Implementa la **spec 013** (9 RF) sobre la base de arriba (`c7b9427` **más la 013 en el
> árbol**). Toca **backend** (contrato, construcción de la entrada,
> objetivo, huella y resultado), un **acoplamiento mínimo del frontend** (huella y diagnóstico, sin vistas
> nuevas) y **documentación**. No cambia el esquema, no migra datos y no toca el contrato de salida.
>
> **Decisiones del usuario (D-1…D-8) y correcciones B1–B3** están fijadas en la spec y no se reinterpretan.
> Este plan incorpora además los hallazgos **P1–P15** del QA del plan.
>
> **Re-base sobre la 015/016 (2026-11-05).** La 013 se planificó contra `bb1767a` y se implementó **después**
> de la 015 (esquema **v6** con el tipo general garantizado y protegido) y de la 016 (comprobación y
> cierre), ya committeadas en `c7b9427`. Consecuencias **documentales**, sin cambios de comportamiento:
> - **Esquema:** el vigente es **v6**; la 013 **no** lo cambia ni migra nada (§7).
> - **Ancla de la no regresión (RNF-4 y criterio 11):** deja de ser «el escenario de `bb1767a`» y pasa a ser
>   el **escenario posterior a la 015/016** — perfil del seed **sin** «Aula de estudio» y base **v6** con el
>   tipo general garantizado por el esquema. La referencia cambia; el requisito y el criterio no.
> - **Golden del JSON sin preferencias (T3):** se compara contra el que produce **el mismo árbol sin la
>   013**, no contra un archivo de `bb1767a`.
>
> **Punto de partida verificado (`c7b9427`, con la 015/016 committeadas):**
> - `SolverConfig` en `src/backend/include/backend/solver/config/solver_config.hpp`; parser y **V1–V18** en
>   `src/backend/src/solver/config/solver_config.cpp`. El contrato tiene **diez secciones obligatorias y
>   dos opcionales** (`aula_fija` y la nueva `preferencias`); **`meta` no es una sección**: es un objeto
>   tolerado que añade `ConstructorEntradaSolver` (`docs/contrato-solver-2.0.md:53-68`).
> - Objetivo: **S1** (`PESO_HORAS=1000`), **S2** (`COSTE_SUPLENTE=100`) y **S3** (`PESO_AGRUPACION=10`).
>   `test_contrato_solver` fija `PESO_AGRUPACION == 10` y las relaciones de orden.
> - `construirJsonEntrada` compone desde `DatosDominio` + preset; `ProfesorDTO::preferencias` existe (012)
>   y `huellaDatos(datos, D)` las ignora a propósito.
> - La rejilla (inicio de turno + duración, bloque entero, eventos que no cortan) está en
>   `ConstructorEntradaSolver.cpp` (`TurnoOrdenado`, `ordenarTurnos`, `slotsDeFranja`), en un namespace
>   anónimo.
> - El seed siembra **0** preferencias: **toda prueba nueva necesita fixtures propios**.

## 1. Arquitectura y archivos

### 1.1 Modelo en el contrato, V19 y presets (RF-2, RF-7)

- `solver_config.hpp`:
  ```cpp
  /// Preferencia ya PROYECTADA: día de la generación y franjas alcanzadas.
  /// Simétrica de `Disponibilidad{dia, slot}`; el docente va por índice.
  struct PreferenciaSolverConfig {
      int          docente = -1;     ///< índice en `profesores` (V19)
      int          dia = -1;         ///< día de la GENERACIÓN (0 = lunes … num_dias-1)
      QVector<int> slot;             ///< slots absolutos del día
      QString      sentido;          ///< "preferida" | "evitada"
  };
  ```
  y `QVector<PreferenciaSolverConfig> preferencias;` en `SolverConfig`, después de `profesores`.
- `solver_config.cpp`:
  - `parsearPreferencias` / `aJson`, mismo estilo que `Disponibilidad`; sección `obj["preferencias"]`.
  - `fromJson`: ausente = sin preferencias (no es error).
  - **`toJson`: emite la sección SOLO si `preferencias` no está vacía.** Así, sin preferencias, el JSON es
    **idéntico** al que produce el mismo árbol **sin la 013** y el seed no gana una sección vacía. La omisión es también la que exige el
    preset (§1.1.1). **RNF-4 exige horario y coste idénticos, no igualdad de bytes del JSON**; con la
    omisión se cumple además la igualdad del JSON, que es una verificación extra y más barata de leer.
  - **V19**, en la cadena **después de V18**:
    ```
    QString validarPreferencias(const SolverConfig& c):
      para cada p (en el orden del arreglo):
        p.docente < 0 || >= num_profesores       -> "V19: … índice de docente %1 fuera de rango (0–%2) …"
        p.dia < 0 || >= num_dias                 -> "V19: … día %1 fuera de rango (0–%2) del docente %3 …"
        cualquier s de p.slot < 0 || >= num_slots_dia -> "V19: … franja %1 fuera de rango (0–%2) …"
        sentido ∉ {"preferida","evitada"}        -> "V19: … sentido «%1» no válido (preferida/evitada) …"
      "" si todo está en rango
    ```
    Orden **docente → día → franja → sentido**; todos los mensajes citan valor recibido y rango admitido.
- Comentarios de rango: `solver_config.cpp:372` («V1-V15») y `solver.cpp:57` («V1–V18») pasan a **V1–V19**.

#### 1.1.1 Preset ≠ configuración (B2)

`CargadorConfiguracionSolver::guardar` (`src/backend/src/services/CargadorConfiguracionSolver.cpp:98`)
elimina la clave antes de escribir:

```cpp
QJsonObject obj = config.toJson();
obj.remove(QStringLiteral("preferencias"));   // el preset no persiste preferencias
```

`cargar` no cambia: `fromJson` tolera la sección si aparece y **no** es fuente de preferencias. La
equivalencia de ida y vuelta se cumple en la **configuración en memoria**, no en el preset (acotación de
la 005/010, §1.8).

### 1.2 Rejilla compartida y proyección (RF-3)

- **Nuevo** `src/backend/include/backend/services/proyeccion_preferencias.hpp` y `.cpp` (puro, sin
  OR-Tools ni `QSql*`):
  ```cpp
  struct TurnoProyectado { QString nombre; QTime inicio, fin; int numSlots = 0; int offset = 0; };
  QVector<TurnoProyectado> ordenarTurnos(const QVector<TurnoDTO>& turnos);   // movido del constructor

  /// Slots absolutos del día cubiertos por `franja` (bloque COMPLETO dentro del rango), recorriendo
  /// todos los turnos. La MISMA función sirve a la disponibilidad y a las preferencias.
  QVector<int> slotsDeFranja(const FranjaHoraria& franja,
                             const QVector<TurnoProyectado>& turnos, int duracionMinutos);

  enum class MotivoPreferencia { Ninguno, DocenteNoParticipa, DiaFueraDeSemana, SinFranjaDelTurno };
  QString textoDeMotivo(MotivoPreferencia);

  struct PreferenciaProyectada { int docente; int dia; QVector<int> slot; SentidoPreferencia sentido; };
  struct PreferenciaNoAplicada { int docente; FranjaHoraria franja;      // día de DOMINIO y HH:mm
                                 SentidoPreferencia sentido; MotivoPreferencia motivo; };
  struct ProyeccionPreferencias { QVector<PreferenciaProyectada> intervinientes;
                                  QVector<PreferenciaNoAplicada> noAplicadas; };

  ProyeccionPreferencias proyectarPreferencias(const DatosDominio& datos,
                                               const SolverConfig& config,   // parcial, sin preferencias
                                               int duracionMinutos);
  ```
- `ConstructorEntradaSolver.cpp` pierde su copia de la rejilla y usa la del módulo (una sola
  implementación).
- **Pseudocódigo** (idéntico al de la revisión anterior en lo esencial):
  ```
  turnos = ordenarTurnos(datos.turnos)
  para i en 0..docentes.size()-1:
      candidatas = {(c,m) : docenteCandidatoElegible(config, cursos[c], profesores[i], m)}
      participa  = !candidatas.vacio()
      slotsPosibles = ∪ slotsTurno(config, cursos[c].turno) sobre las candidatas
      para cada preferencia (orden 012: dia, inicio, fin, id):
          si !participa                                   -> noAplicada DocenteNoParticipa
          diaGen = franja.dia - 1                          # dominio 0=domingo → generación 0=lunes
          si franja.dia == 0 o diaGen >= num_dias          -> noAplicada DiaFueraDeSemana
          alcanzables = slotsDeFranja(franja, turnos, D) ∩ slotsPosibles
          si alcanzables.vacio()                           -> noAplicada SinFranjaDelTurno
          intervinientes += {i, diaGen, alcanzables, sentido}
  ordenar canónicamente por (docente, dia, sentido, slot)
  ```
  - Una entrada por preferencia interviniente; los slots de **una** preferencia se deduplican.
  - **La deduplicación para el objetivo es otra cosa** (P1): el objetivo cuenta por
    `(docente, día, slot)` **una sola vez por sentido**, aunque dos preferencias del mismo sentido se
    solapen (§1.5).
- `test/backend/test_proyeccion_preferencias.cpp` (**nuevo**, `add_qtest(… backend)`): contención, bordes,
  multi-turno, cruce entre turnos, evento que no corta, domingo, sábado con `num_dias` 5 y 6, docente que
  no participa, preferencia de tarde con docente candidato solo de mañana, y orden canónico.

### 1.3 Construcción de la entrada (RF-2, RF-3, RF-7)

- `ConstructorEntradaSolver.hpp`:
  ```cpp
  struct EntradaConstruida {
      QJsonObject json;
      QVector<PreferenciaProyectada> intervinientes;
      QVector<PreferenciaNoAplicada> noAplicadas;
  };
  Resultado<EntradaConstruida> construirEntradaCompleta(const DatosDominio&, const SolverConfig&,
                                                        int duracionMinutos, const QDateTime& ahora);
  Resultado<QJsonObject> construirJsonEntrada(...);   // envoltorio; firma de hoy sin cambios
  ```
- Orden estricto (RF-2, B2): construir la config **sin** preferencias → `proyectarPreferencias` →
  `salida.preferencias = intervinientes` (la del preset **nunca** se copia) → `toJson` + `meta` →
  **validar después** (lo hace `ServicioGeneracion::generar` con `validarEntradaSolver`).
- `test_constructor_entrada.cpp`: preferencias desde los datos, sección del preset ignorada, arreglo
  vacío sin preferencias, `noAplicadas` con motivo; **con 0 preferencias el JSON es idéntico al actual**
  (golden del propio test, que no se toca).

### 1.4 Huella de datos (RF-6)

- `ServicioGeneracion.hpp`:
  ```cpp
  QString huellaDatos(const DatosDominio& datos, int duracionMinutos,
                      const QVector<PreferenciaProyectada>& intervinientes);
  ```
- `ServicioGeneracion.cpp`: tras la duración, si hay intervinientes, añade `pref:<docente>,<dia>,<sentido>,<slot>[,…];`
  en orden canónico y con slots ascendentes. **Sin intervinientes no se emite el bloque** → la huella vale
  exactamente lo mismo que hoy.
- Call sites (todos): `src/frontend/src/services/entrada_solver_builder.cpp:294`;
  `test/backend/test_servicio_generacion.cpp` (267-268, 272, 277, 281, 285, 300, 304, 310, 316-318,
  326-327) pasando `{}` o una lista explícita; `test/backend/test_servicio_preferencias.cpp:510-529`, que
  **se reescribe**: una preferencia **interviniente** (pasada explícitamente en la lista) cambia la huella
  y una **de domingo** (lista `{}`) no la cambia; `toProfesor()` sigue ignorándolas.
  `test/frontend/test_generacion_seed.cpp` no la llama (seed con 0 preferencias).

### 1.5 Objetivo: dos fases con jerarquía lexicográfica (RF-1, RF-4, RF-5)

- `contrato_solver.hpp`: `constexpr int PESO_PREFERENCIA = 25;` con
  `PESO_AGRUPACION (10) < 25 < COSTE_SUPLENTE (100)`.
- `solver.hpp` / `solver.cpp`:
  ```cpp
  struct AnclajeEstructural {
      QMap<std::tuple<int,int,int>, int> docentePorMateria;   // z* (p,c,m) — P11
      QMap<int, int>                     excesoPorDocente;   // e*[p]
  };
  AnclajeEstructural extraerAnclaje(const CpSolverResponse&, const SolverConfig&);
  void fijarAnclaje(const AnclajeEstructural&);
  LinearExpr expresionPreferencias(const SolverConfig&);   // construida UNA vez por fase
  ```
- **Deduplicación del objetivo (P1):** dos `QSet<std::tuple<int,int,int>>` (`kEvitadas`, `kPreferidas`)
  con clave **(docente, día, slot)** construidas desde `config.preferencias`; una franja alcanzada por
  dos preferencias del mismo sentido suma **una vez**. Una franja en ambos conjuntos (sentidos opuestos
  hechos a mano) aparece en los dos términos y se **compensa**.
- **Pseudocódigo de `resolver`:**
  ```
  # FASE 1 — referencia (modelo de HOY: S1+S2+S3)
  crear modelo/vars; crearVariables; agregarRestricciones; construirObjetivo
  r1 = resolver(6.0 dt, 1 worker, seed 0)
  si r1 no es factible -> analizarFallo(r1)  (comportamiento de hoy)
  si config.preferencias vacío -> return r1        # RNF-4: idéntico
  anclaje = extraerAnclaje(r1)

  # FASE 2a — cumplimiento de preferencias, con z* y e* FIJADOS
  crear modelo/vars; crearVariables; agregarRestricciones; fijarAnclaje(anclaje)
  model.Minimize(expresionPreferencias(config))
  r2a = resolver(2.0 dt, 1 worker, seed 0)
  si r2a INFEASIBLE o UNKNOWN -> return r1         # P6/P7: cae a la referencia
  si r2a MODEL_INVALID u otro estado -> return analizarFallo(r2a)   # P6: se PROPAGA

  # FASE 2b — S3 como desempate, con la preferencia FIJADA a su óptimo
  crear modelo/vars; crearVariables; agregarRestricciones; fijarAnclaje(anclaje)
  model.AddEquality(expresionPreferencias(config), valorOptimo2a)
  construirObjetivoSoloS3()                        # PESO_AGRUPACION · Σ isolated/longGroup
  r2b = resolver(1.0 dt, 1 worker, seed 0)
  si r2b no es factible/óptimo dentro del plazo -> return r2a   # P7: preferencia ya garantizada
  return r2b, tiempo = t1 + t2a + t2b
  ```
  - **Por qué lexicográfica (P2):** con `PESO_PREFERENCIA = 25` frente a `PESO_AGRUPACION = 10` la
    preferencia **no** gana en agregado (una franja evitada por clase acumula y S3 tiene hasta ~1 unidad
    por clase). Una cota fija exigiría `PESO_PREFERENCIA > PESO_AGRUPACION · nº_clases` (≈ 5200), fuera
    del rango 20–50 que fija la spec. Por eso la jerarquía se impone **por fases**: 2a optimiza la
    preferencia y 2b solo desempata con S3. Con eso «gana la preferencia» es **estructural**.
  - **Garantía estructural (RF-1):** 2a y 2b fijan `z*` y `e*` por **igualdad**, así que la asignación de
    docente por materia-curso y el exceso por docente son los de la referencia.
  - **Determinismo:** las tres fases usan 1 worker, seed 0 y presupuestos deterministas; la fase 1 es el
    modelo de hoy, luego la referencia es la misma que produce el motor sin la sección.
  - **Coste:** hasta **tres** resoluciones. Presupuesto 6.0 + 2.0 + 1.0; las 2a/2b no se ejecutan sin
    preferencias. Umbral de RNF-3 en §7.
- `test/backend/test_solver_preferencias.cpp` (**nuevo**, `add_qtest(… backend_solver)`):

| Caso | Qué fija |
|---|---|
| `mismasPreferencias_mismaAsignacionDocenteYExceso` | RF-1: `(p,c,m)` activos y exceso por docente **idénticos** con y sin preferencias. El exceso se calcula **en el test** contando clases por docente en `HorarioSalida` y restándole `horas_aula` (P14); no se añade API |
| `sinPreferencias_salidaIdenticaALaActual` | RNF-4: misma salida que la misma entrada sin la sección |
| `evitada_mueveLaClaseFueraDeLaFranja` | RF-4: se mueve si hay alternativa |
| `preferida_atraeLaClase` | RF-4: coste negativo equivalente |
| `dosPreferenciasSolapadasMismoSentido_cuentanUnaVez` | **P1**: dos evitadas solapadas sobre la misma franja no duplican la penalización (se compara con una sola preferencia equivalente) |
| `evitada_acumuladaNoProvocaSuplente` | RF-1/RF-5: 5 h evitadas no cambian el titular |
| `evitada_acumuladaNoProvocaExceso` | RF-1/RF-5: 5 h evitadas no crean exceso |
| `preferenciaGanaALaAgrupacion` | **P2**: conflicto directo entre evitar una franja y mantener un grupo de 2–3 → **gana la preferencia** |
| `preferenciaCedeSiNoHayAlternativa` | RF-4: sin otra franja libre, la clase cae en la evitada y no se declara no factible |
| `preferenciaNoRompeInvariantesDuras` | RF-4: D3/D4/D5/D11 intactas |
| `ordenDeDeclaracionNoInfluye` | RNF-2: dos órdenes del arreglo → mismo horario |
| `fase2Infeasible_devuelveLaReferencia` | **P6/P7**: forzando una preferencia imposible, se devuelve la solución de la fase 1 (y no un error) |

- `test/backend/test_solver_horarios.cpp`: sin cambios.

### 1.6 Acoplamiento mínimo del frontend (RF-6, RF-8)

- `src/frontend/src/services/entrada_solver_builder.hpp`:
  ```cpp
  struct Composicion {
      QJsonObject entrada;
      ContextoHorario contexto;
      QStringList avisos;
      QString huella;                                   ///< con las preferencias intervinientes
      QVector<PreferenciaNoAplicada> preferenciasNoAplicadas;
  };
  QString huellaActual(const NucleoDatos& nucleo, const OpcionesUI& opciones);
  ```
- `entrada_solver_builder.cpp`: `construirEntrada` usa `construirEntradaCompleta`, rellena `huella`
  (`huellaDatos(datos, duracionEfectiva, intervinientes)`) y `preferenciasNoAplicadas`; `huellaActual`
  compone la entrada y devuelve `comp.huella` (si la composición falla, cae a `huellaDatos(datos, 35, {})`,
  como hoy). Para que ambas usen el **mismo** preset se extrae el bloque de composición del preset a
  `SolverConfig presetDesdeOpciones(const NucleoDatos&, const DatosDominio&, const OpcionesUI&)`.
- `src/frontend/src/views/schedule_generation_widget.cpp` (P10):
  - el método es **`leerOpciones()`** (`:944`, miembro de la clase);
  - **`:131`** (lambda `[this]`) → `GeneracionSolver::huellaActual(*m_nucleo, leerOpciones())`;
  - **`:973`** → `GeneracionSolver::huellaActual(*m_nucleo, opciones)`, con el `opciones` de `:954`;
  - **`:974`** → `m_servicio->generar(m_entradaActual, huella, m_preferenciasNoAplicadas)`, con el
    miembro nuevo que guarda el diagnóstico de la última composición.
  **Sin vistas, diálogos ni widgets nuevos.**

### 1.7 Resultado y cumplimiento (RF-8)

- **Nuevo** `src/backend/include/backend/services/preferencias_cumplimiento.hpp` y `.cpp` (puro):
  ```cpp
  enum class EstadoPreferencia { NoAplicada, Incumplida, Respetada };
  struct ClaseIncumplida { QString curso; int materia; int slot; };   // curso = "anio|seccion"
  struct PreferenciaEstado {
      int docente; int dia;                 ///< dia en numeración de DOMINIO (0=domingo … 6=sábado)
      QVector<int> slot;                    ///< franjas alcanzadas (slot absoluto del día)
      QString sentido;
      EstadoPreferencia estado;
      MotivoPreferencia motivo;             ///< Ninguno si está respetada o incumplida
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
  - **Un solo numerado de día en el resultado (P5):** el de **dominio** de la 012 (0=domingo … 6=sábado);
    se obtiene con `diaDominio = diaGeneracion + 1` al construir el estado. `diaGeneracion` no sale del
    módulo.
  - **Motivo `Ninguno`** para respetadas e incumplidas; solo las no aplicadas llevan (a)/(b)/(c).
  - **Clases incumplidas:** para una **evitada** con clases en sus franjas, se listan
    `{curso, materia, slot}` de cada clase que cae dentro (RF-8: se informa «cuántas franjas y qué clases
    la incumplen»). Para una **preferida** sin clases no hay clases que listar: lista vacía.
  - Precedencia: **no aplicada** (viene de `noAplicadas`) → **incumplida** → **respetada**. Orden
    canónico por `(docente, dia, sentido, slot)`.
- `ServicioGeneracion`:
  - `void generar(const QJsonObject& entrada, const QString& huella,
    const QVector<PreferenciaNoAplicada>& noAplicadas = {});`
  - `ResultadoGeneracion` gana `CumplimientoPreferencias cumplimiento;` (aditivo).
  - En `alResponder`, tras `analizarSalidaSolver`: `m_ultimo.cumplimiento = analizarPreferencias(m_config,
    horario, m_noAplicadas)`.
  - El contrato de **salida** del solver no cambia.
- `test_servicio_generacion.cpp`: estados (respetada, evitada incumplida con la lista de clases, evitada
  parcialmente ocupada, preferida sin clase, no aplicada con cada motivo), el **día de dominio** en el
  resultado y los tres totales.

### 1.8 Documentación (RF-9)

- `docs/contrato-solver-2.0.md`:
  - `:53` → **diez secciones obligatorias y dos opcionales** (`aula_fija` y `preferencias`); **`meta` no
    es una sección** (objeto tolerado que añade el constructor).
  - Fila nueva de `preferencias` (arreglo, opcional, ausente = sin preferencias) con la forma
    `{docente, dia, slot[], sentido}` y **V19**.
  - Sección nueva «Preferencias del docente»: representación ya **proyectada** (día de la generación +
    franjas + sentido, docente por índice), proyección ≠ conversión de horas a bloques, solo viajan las
    **intervinientes**, el preset **no** las guarda ni las aporta, V19 no valida rangos de reloj.
  - `:27` y `:469` → **V1–V19**.
- `docs/constitution.md`: **§7.2** con **S1, S2, preferencia y S3** en orden
  (`PESO_HORAS > COSTE_SUPLENTE > PESO_PREFERENCIA > PESO_AGRUPACION`), la garantía de que no cambia el
  reparto de docentes ni el exceso, y constancia de que **S3 no figuraba**; **§7.4** → **V1–V19**;
  **§7.5** con las preferencias que no intervienen y su exclusión como causa.
- Notas de **acotación/supersesión**: 005 «Configuración equivalente» (`:303-306`) y 010 ida y vuelta
  (`:213-216`) —la equivalencia de la sección es de la **configuración**, no del preset—; 012 RF-5 y
  «Fuera de alcance» superados; `009-agrupacion-bloques-materias` anota que la preferencia va por encima
  de S3 sin cambiar su definición; citas **V1–V18/V1–V15** de 005/010 → **V1–V19**; queda anotado, sin
  resolver, el riesgo de las dos «009» y del doble uso de «S3».

## 2. Decisiones (con la alternativa descartada)

1. **Tres resoluciones (2 fases, la 2 en 2a/2b) para la jerarquía.** Descartado: **escalar constantes**
   para dominancia agregada (`W_pref > PESO_AGRUPACION · nº_clases`), fuera del rango 20–50 que fija la
   spec y con ruptura de `test_contrato_solver`; y descartado: **solo el término negativo sin anclaje**
   (no garantiza RF-1).
2. **Anclaje por igualdad (`z*`, `e*`)**, no por cota: `exceso[p] ≤ e*[p]` permitiría menos exceso y
   cambiaría la distribución de la referencia.
3. **La 2b es opcional y cae a la 2a**: la preferencia ya está garantizada; S3 solo desempata. Así el
   plazo no puede degradar la jerarquía.
4. **Deduplicación del objetivo por (docente, día, slot) y sentido** con `QSet<std::tuple<int,int,int>>`
   (P1, P11); el estado se conserva **por preferencia**.
5. **`toJson` omite la sección vacía** (P3): sin preferencias el JSON es el de hoy. `guardar` elimina la
   clave aunque haya entradas.
6. **La sección lleva día y franjas proyectadas** (no reloj) y el docente por **índice**.
7. **Una sola rejilla** (`proyeccion_preferencias`), usada por disponibilidad y preferencias.
8. **La proyección trabaja sobre la configuración parcial** y reutiliza `docenteCandidatoElegible` y
   `slotsTurno`: «participa» e «interviene» son la misma noción que V13.
9. **`EntradaConstruida` con envoltorio**: las ~30 llamadas de `construirJsonEntrada` no cambian.
10. **Solo las franjas alcanzables** viajan en la configuración (las de turnos donde el docente es
    candidato); la huella coincide así con lo que interviene.
11. **`analizarPreferencias` recibe las no aplicadas** de la composición (la config no las lleva).
12. **El estado no viaja en la salida del solver**; el contrato de salida no cambia.
13. **`huellaActual` compone la entrada** (mismo camino que la generación): una segunda proyección en la
    interfaz sería una divergencia silenciosa.
14. **El exceso se calcula en el test** (P14) desde las clases y `horas_aula`; no se amplía la API.
15. **Fixtures propios** en todos los tests nuevos (el seed tiene 0 preferencias).

## 3. Presupuesto y umbrales (RNF-3)

- Fase 1: `max_deterministic_time(6.0)` (sin cambios), 1 worker, seed 0.
- Fase 2a: 2.0 · Fase 2b: 1.0. Sin preferencias no se ejecutan.
- **Escalera de plazo:** si la 2a agota el plazo o no es factible → **se devuelve la fase 1** (que ya
  cumple la garantía estructural; la spec lo autoriza: «el mejor horario que cumpla la garantía dentro del
  plazo»). Si la 2b no cabe → **se devuelve la 2a**. Solo `MODEL_INVALID` y errores internos se propagan.
- **Umbral (T6 y T10):** en el escenario de referencia el total con preferencias debe quedar **< 60 s** y
  **≤ +50 %** sobre la misma entrada sin preferencias. Si no cabe: bajar 2a/2b a 1.0/0.5 y volver a medir;
  si **ni con la escalera de plazo** se cumple, se propone cambio de requisitos (no antes).

## 4. Verificación por tarea

Comunes (cada tarea empieza con el build):

```bash
# backend y tests sin interfaz
cmake --preset backend && cmake --build --preset backend --parallel
ctest --preset backend
# suite completa (incluye frontend y el seed)
cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

**Ninguna tarea se cierra con el árbol sin compilar ni con tests en rojo.** Test primero (rojo).

- **T1 (~35 min, `backend`):** build + `-R '^(test_contrato_solver|test_cargador_configuracion|test_validador_entrada)$'`
  verde; ida y vuelta de la configuración **en memoria** con preferencias; con 0 preferencias la sección
  **no está** en el JSON; el archivo de preset guardado desde una configuración con preferencias **no**
  contiene la clave (y la configuración en memoria sí las conserva: P13); V19 rechaza los cuatro defectos
  en orden con valor y rango; `PESO_AGRUPACION == 10` sigue verde.
- **T2 (~30 min, `backend`):** build + `-R '^(test_proyeccion_preferencias|test_constructor_entrada)$'`
  verde; una sola implementación de la rejilla; casos de borde y motivos (c) > (a) > (b); la
  disponibilidad no cambia.
- **T3 (~30 min, `backend`):** build + `-R '^(test_constructor_entrada|test_validador_entrada)$'` verde;
  preferencias de los datos, sección del preset ignorada, `noAplicadas` con motivo, JSON idéntico con 0
  preferencias, entrada válida por V19.
- **T4 (~30 min, `backend` + `full-release`):** build backend + `-R '^(test_servicio_generacion|test_servicio_preferencias)$'`
  verde; huella con intervinientes explícitos (P15), sin intervinientes = golden; build `full-release` y
  `-R '^test_generacion_seed$'` verde.
- **T5 (~45–60 min, `backend`):** build + `-R '^(test_solver_preferencias|test_solver_horarios|test_contrato_solver)$'`
  verde; casos de §1.5 (incluidos el solapado que cuenta una vez, el exceso calculado en el test y el
  fallback INFEASIBLE); sin preferencias, salida idéntica.
- **T6 (~35 min, `backend`):** build + `-R '^(test_solver_preferencias|test_solver_benchmark)$'` verde;
  `preferenciaGanaALaAgrupacion`; escalera de plazo probada (2a→1, 2b→2a) y `MODEL_INVALID` propagado;
  `test_solver_benchmark` amplía el escenario con preferencias, **aserta el techo de 60 s** y registra en
  `journal.md` la comparación con la referencia (el **+50 % se documenta, no se aserta**, por depender de la
  máquina); dos ejecuciones seguidas dan el mismo horario.
- **T7 (~30 min, `backend`):** build + `-R '^(test_servicio_generacion|test_proyeccion_preferencias)$'`
  verde; estado por preferencia con **día de dominio**, motivo `Ninguno` y lista de clases incumplidas;
  agregado de tres totales.
- **T8 (~30 min, docs):** `grep -n "preferencias" docs/contrato-solver-2.0.md` (sección y fila);
  `grep -n "diez secciones obligatorias y dos opcionales" docs/contrato-solver-2.0.md`;
  `grep -n "V1–V19" docs/contrato-solver-2.0.md docs/constitution.md`;
  `grep -n "PESO_PREFERENCIA" docs/constitution.md`; el documento dice que `meta` no es sección.
- **T9 (~20 min, docs):** las cuatro notas de acotación/supersesión y las citas de rango corregidas;
  `git diff --stat` acotado a `docs/` y `specs/*/spec.md`.
- **T10 (~30 min, `backend` + `full-release`):** build y **suite completa** en verde (58 + tests nuevos);
  `ctest --preset backend` verde; `journal.md` con la evidencia de las diez tareas.

## 5. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|----|----------|--------------|
| RF-1 | T5, T10 | igualdad de `(p,c,m)` y exceso (calculado en el test) con y sin preferencias; suite completa |
| RF-2 | T1, T3 | ida y vuelta en memoria; sección ausente = sin preferencias; preset sin la clave; datos como única fuente |
| RF-3 | T2, T3 | casos de borde, filtro de día, motivos (c)>(a)>(b); JSON con día de generación y franjas |
| RF-4 | T5 | evitada mueve, preferida atrae, cede sin alternativa, invariantes intactas |
| RF-5 | T1, T5, T6 | orden de constantes; 5 h evitadas sin suplente ni exceso; **preferencia gana a S3**; medición |
| RF-6 | T4 | huella con intervinientes explícitos, sin intervinientes = golden; frontend igual |
| RF-7 | T1, T3 | V19 en orden; `V1–V19`; no aplicables no se rechazan |
| RF-8 | T7 | estado con día de dominio, motivo `Ninguno`, clases incumplidas y agregado |
| RF-9 | T8, T9 | contrato (diez + dos, `meta`, V1–V19), constitución §7.2/§7.4/§7.5, acotación y supersesiones |
| RNF-1 | T1, T2, T3, T7 | mensajes con valor, rango y acción |
| RNF-2 | T5, T6 | `ordenDeDeclaracionNoInfluye`; dos ejecuciones idénticas |
| RNF-3 | T6 | 60 s y ≤ +50 % con la escalera de plazo de §3 |
| RNF-4 | T3, T5, T10 | JSON y horario idénticos sin preferencias; S1/S2/S3 sin cambios |
| RNF-5 | T10 | `ctest --preset backend` verde sin interfaz |
| RNF-6 | T3, T4 | solo lectura; nada migra |
| RNF-7 | T1–T10 | esta tabla |

## 6. Riesgos y supuestos

- **Coste de dos resoluciones extra.** Mitigado por la escalera de plazo (§3): la garantía de RF-1 no
  depende de que la 2b termine. Medición en T6 y T10.
- **La 2a ignora S3**, así que su horario puede empeorar la agrupación; la 2b lo corrige. Si la 2b cae por
  plazo, el horario sigue siendo válido (preferencia > agrupación) pero menos compacto.
- **La fase 1 es la referencia por definición**, aunque acabe en `FEASIBLE` y no en `OPTIMAL` (igual que
  hoy).
- **Huella:** toda generación con intervinientes cambia el valor; los horarios guardados antes pasan a
  «datos anteriores» (declarado en RF-6).
- **Dos composiciones por generación** (huella al inicio y al recibir): lectura del núcleo, milisegundos.
- **Acoplamiento del frontend:** `Composicion` gana dos campos, `huellaActual` un parámetro y la vista
  tres líneas; sin ellas no compila `full-release` (T4).
- **`num_dias` del preset** decide si el sábado aplica; probado en T2.
- **Preferencias solapadas del mismo sentido** no se fusionan en la configuración, pero el objetivo las
  cuenta **una vez** por franja (P1).
- **Textos de la 012** («no intervienen») quedan superados; se anotan en T9.

## 7. Restricciones (no se tocan)

- Esquema (**v6**, el de la 015) y migraciones: la 013 **no** cambia la base ni migra datos.
- Contrato de **salida** del solver.
- Interfaz de usuario: sin vistas, diálogos ni widgets nuevos.
- `PESO_HORAS`, `COSTE_SUPLENTE` y `PESO_AGRUPACION`; invariantes D1–D13 y S1/S2/S3.
- El seed: sigue con **0** preferencias (y, desde la 015, ya **no** declara «Aula de estudio»: lo
  garantiza el esquema).
