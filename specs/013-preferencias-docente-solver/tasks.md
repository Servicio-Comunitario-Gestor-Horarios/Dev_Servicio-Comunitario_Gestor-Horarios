# Tareas 013 - Preferencias del docente en la generación

> Base: **`c7b9427`** (HEAD tras committear la **015** y la **016**) **+ la 013 en el árbol de trabajo**
> · Plan: `plan.md` · Spec: `spec.md`
> El `Base` original era `bb1767af343c108b20de2b2b4de7ebe1d7658c2f`; la re-base del 2026-11-05 lo sustituye
> por `c7b9427`. **El ancla de RNF-4 y del criterio 11 es el escenario posterior a la 015/016** (seed sin
> «Aula de estudio», base **v6** con el tipo general garantizado). Esquema vigente **v6**; la 013 no lo cambia.
> **Aprobadas 2026-11-04**; **implementadas y validadas el 2026-11-06** (T1–T10, ver `journal.md`). Diez tareas, **sin dividir la spec**: el exceso de
> duración de **T1, T5 y T6** sobre la guía de 20–30 min fue **aceptado por el usuario** y RF-8 se queda en
> la 013. **T1–T10 hechas (2026-11-06); implementación completa, pendiente del APROBADO del revisor.**
> Una tarea cada vez: **test en rojo → código → verde → marcar y parar**. La evidencia (comandos, salidas,
> hallazgos, commits) va en `journal.md`.
> Cada tarea empieza con el **build** y ninguna cierra con el árbol sin compilar ni con tests en rojo.
> Diez tareas en orden de dependencia. **T1, T5 y T6 se estiman por encima de 30 min** (ver la nota final
> sobre el tamaño).

- [x] **T1. Preferencia proyectada en el contrato, V19 y preset sin la sección.** RF-2, RF-7, RNF-1
      · ~35 min ⚠️
    - Hecho cuando: `test_contrato_solver` hace ida y vuelta de **la configuración en memoria** con
      preferencias y comprueba que con 0 preferencias la clave **no aparece** en el JSON
      (`toJson` omite la sección vacía); `test_cargador_configuracion` comprueba que el **archivo** de
      preset guardado desde una configuración con preferencias **no** contiene la clave, que la
      configuración en memoria sí las conserva (no se compara `toJson` de ida y vuelta, que difiere por la
      omisión), y que un preset que sí la contiene se carga sin usarla; `test_validador_entrada` rechaza
      por V19 en el orden docente → día → franja → sentido, citando valor y rango admitido;
      `PESO_AGRUPACION == 10` sigue verde.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest
      --test-dir build-backend --output-on-failure -R
      '^(test_contrato_solver|test_cargador_configuracion|test_validador_entrada)$'`.

- [x] **T2. Rejilla compartida, proyección reloj → franja y filtro de día (puro).** RF-3, RNF-1 · ~30 min
    - Hecho cuando: existe `proyeccion_preferencias` con `ordenarTurnos`/`slotsDeFranja` y el constructor
      usa **esa** implementación (la suya desaparece); `test_proyeccion_preferencias` pasa contención
      completa, bordes de franja, turnos múltiples, cruce entre turnos, rango sobre un evento, domingo,
      sábado con `num_dias` 5 y 6, docente que no participa y preferencia de tarde con docente candidato
      solo de mañana; los motivos salen en orden **(c) > (a) > (b)**; la disponibilidad no cambia de
      resultado.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest
      --test-dir build-backend --output-on-failure -R
      '^(test_proyeccion_preferencias|test_constructor_entrada)$'`.

- [x] **T3. Construcción de la entrada: preferencias desde los datos, preset ignorado y orden con V19.**
      RF-2, RF-3, RF-7, RNF-4 · ~30 min
    - Hecho cuando: `construirEntradaCompleta` devuelve `json`, `intervinientes` y `noAplicadas`, y
      `construirJsonEntrada` mantiene su firma devolviendo solo el JSON; la configuración lleva día de la
      generación, franjas alcanzables e índice de docente; la sección del preset se ignora y se
      sobrescribe; un docente sin preferencias deja el arreglo vacío; con **0 preferencias** el JSON es
      idéntico al que produce el mismo árbol **sin la 013** (golden del test actual, sin tocar; ya no se
      compara contra un archivo de `bb1767a`); la entrada valida con V19.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest
      --test-dir build-backend --output-on-failure -R
      '^(test_constructor_entrada|test_validador_entrada)$'`.

- [x] **T4. Huella con intervinientes y cableado mínimo del frontend.** RF-6, RNF-6 · ~30 min
    - Hecho cuando: `huellaDatos(datos, duracion, intervinientes)` sustituye a la firma de dos argumentos y
      **todos** los call sites están actualizados, pasando la lista **explícitamente** (`{}` donde no hay
      preferencias, una preferencia de domingo con `{}`); una preferencia interviniente cambia la huella y
      sin intervinientes vale lo mismo que hoy (golden); el orden de preferencias y de slots no altera la
      huella; `Composicion` expone `huella` y `preferenciasNoAplicadas`, `huellaActual(nucleo, opciones)`
      usa `leerOpciones()` en `:131` y `:973` de la vista y el frontend compila.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest
      --test-dir build-backend --output-on-failure -R
      '^(test_servicio_generacion|test_servicio_preferencias)$'`; después
      `cmake --preset full-release && cmake --build build-rel --parallel && ctest --test-dir build-rel
      --output-on-failure -R '^test_generacion_seed$'`.

- [x] **T5. Objetivo: fase 1 de referencia y fase 2 con anclaje, deduplicación y fallback.** RF-1, RF-4,
      RF-5 · ~45–60 min ⚠️
    - Hecho cuando: `PESO_PREFERENCIA = 25` (con `PESO_AGRUPACION < PESO_PREFERENCIA < COSTE_SUPLENTE`
      comprobado); el solver ejecuta la fase 1 (modelo de hoy = referencia) y, si hay preferencias, la fase
      2 con `z*` y `e*` **fijados por igualdad**; el término de preferencias deduplica por
      `(docente, día, slot)` con dos `QSet<std::tuple<int,int,int>>` (evitadas/preferidas), de modo que dos
      preferencias solapadas del mismo sentido cuentan una vez; sin preferencias no se ejecuta la fase 2;
      `test_solver_preferencias` pasa su tabla (igualdad de asignación de docente y de exceso —calculado en
      el test desde las clases y `horas_aula`—; evitada que mueve; preferida que atrae; solapadas que
      cuentan una vez; 5 h evitadas sin suplente ni exceso; cede si no hay alternativa; invariantes
      intactas; orden de declaración que no influye; **fallback a la referencia si la fase 2 es
      INFEASIBLE/UNKNOWN**); `test_solver_horarios` sigue verde.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest
      --test-dir build-backend --output-on-failure -R
      '^(test_solver_preferencias|test_solver_horarios|test_contrato_solver)$'`.

- [x] **T6. Jerarquía preferencia > S3 (fase 2b), presupuesto y medición.** RF-5, RNF-2, RNF-3 · ~35 min ⚠️
    - Hecho cuando: la fase 2b fija el valor óptimo de la preferencia con una igualdad y minimiza solo S3;
      `preferenciaGanaALaAgrupacion` pasa (en conflicto directo gana la preferencia); si la 2a o la 2b no
      caben en su presupuesto, se devuelve la solución anterior de la escalera (2a→1, 2b→2a) y **solo**
      `MODEL_INVALID` u otro error interno se propaga; `test_solver_benchmark` incluye el escenario con
      preferencias, **aserta el techo de 60 s** y registra en `journal.md` la comparación con la referencia
      (el +50 % se documenta, no se aserta por depender de la máquina); dos ejecuciones seguidas dan el
      mismo horario.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest
      --test-dir build-backend --output-on-failure -R
      '^(test_solver_preferencias|test_solver_benchmark)$'`.

- [x] **T7. Estado por preferencia y cumplimiento en el resultado del backend.** RF-8, RNF-1 · ~30 min
    - Hecho cuando: `analizarPreferencias(config, horario, noAplicadas)` (pura) devuelve el estado por
      preferencia con precedencia **no aplicada (con motivo) → incumplida → respetada**, con el **día de
      dominio** de la 012 en el resultado (0=domingo … 6=sábado), `MotivoPreferencia::Ninguno` para
      respetadas e incumplidas, y la **lista de clases incumplidas** (`curso`, `materia`, `slot`) en las
      evitadas ocupadas; `ServicioGeneracion::generar` acepta el diagnóstico y `ResultadoGeneracion` expone
      `cumplimiento` con los tres totales; el contrato de salida del solver no cambia.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest
      --test-dir build-backend --output-on-failure -R
      '^(test_servicio_generacion|test_proyeccion_preferencias)$'`.

- [x] **T8. Documentación del contrato y de la constitución.** RF-9 · ~30 min
    - Hecho cuando: `docs/contrato-solver-2.0.md` declara **diez secciones obligatorias y dos opcionales**
      (`aula_fija` y `preferencias`), describe la sección con su forma, la proyección, que solo viajan las
      intervinientes, que el preset no las aporta, que `meta` **no** es sección y que V19 no valida rangos
      de reloj; sus citas de rango dicen **V1–V19**; `docs/constitution.md` §7.2 lista
      **S1/S2/preferencia/S3** con el orden de pesos y la garantía, §7.4 pasa a **V1–V19** y §7.5 anota las
      preferencias que no intervienen.
    - Verificación: `grep -n "diez secciones obligatorias y dos opcionales" docs/contrato-solver-2.0.md`,
      `grep -n "preferencias" docs/contrato-solver-2.0.md`, `grep -n "V1–V19" docs/contrato-solver-2.0.md
      docs/constitution.md`, `grep -n "PESO_PREFERENCIA" docs/constitution.md`, `grep -n "meta" 
      docs/contrato-solver-2.0.md`.

- [x] **T9. Supersesiones, acotación de la equivalencia y riesgo documental.** RF-9, RNF-4 · ~20 min
    - Hecho cuando: la 005 («Configuración equivalente», `:303-306`) y la 010 (ida y vuelta, `:213-216`)
      llevan la nota de que, para las preferencias, la equivalencia es de la configuración y no del preset;
      la 012 anota que su RF-5 y su «Fuera de alcance» quedan superados; la 009-agrupacion anota que la
      preferencia va por encima de S3 sin cambiar su definición; las citas **V1–V18/V1–V15** de las specs
      005 y 010 pasan a **V1–V19**; queda anotado (sin resolver) el riesgo de las dos «009» y del doble uso
      de «S3».
    - Verificación: `grep -rn "013" specs/005-nucleo-solver/spec.md specs/010-solver-tipos-y-eventos/spec.md
      specs/012-preferencias-docente/spec.md specs/009-agrupacion-bloques-materias/spec.md`;
      `git diff --stat` acotado a `docs/` y `specs/`.

- [x] **T10. Regresión global y suite completa.** RNF-4, RNF-5, RNF-7 · ~30 min
    - Hecho cuando: sin preferencias, el JSON de la entrada y el horario son idénticos a los que produce el
      mismo árbol sin la 013 (escenario posterior a la 015/016: seed sin «Aula de estudio», base v6 con el
      tipo general garantizado);
      `ctest --preset backend` y `ctest --preset full-release` (58 + los tests nuevos) en verde;
      `journal.md` recoge la evidencia de las diez tareas, incluidos los tiempos de T6.
    - Verificación: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset
      backend`; después `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset
      full-release`.

## Tamaño y corte propuesto (para el usuario)

- Diez tareas, pero **T1 (30–40), T5 (45–60) y T6 (30–40)** superan los 30 min. El resto cabe.
- Si se quiere cumplir el límite estricto de 20–30 min por tarea, el corte natural es **sacar RF-8** (T7:
  estado por preferencia y `ResultadoGeneracion::cumplimiento`) a una **spec posterior** (p. ej.
  `015-cumplimiento-de-preferencias`): es un artefacto **aditivo** que no condiciona el modelo, el
  contrato, la huella ni el objetivo, y deja la 013 en **9 tareas** con T5/T6 todavía algo por encima.
  Alternativa más agresiva: mover también el **cableado del frontend** (parte de T4) a esa spec, aceptando
  que la huella del cliente no incluya preferencias hasta entonces (no se recomienda: marcaría «datos
  anteriores» en toda generación con preferencias).
- **No se divide sin decisión del usuario**; estas diez tareas quedan como reparto aprobable.
