# Tareas 016 — Comprobación y cierre del tipo general de aula

> Base: **`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`** (HEAD) **+ la 015 aplicada en el árbol, sin
> commits** (esquema **v6**). `HEAD` sigue en `bb1767a`: si la 015 se commitea antes de empezar, el `Base`
> **pasa a ser ese sha**. Plan: `plan.md`. Spec: `spec.md` (aprobada el 2026-11-05).
> **Nueve tareas, en orden de dependencia. T1, T2, T4 y T7 se estiman por encima de 30 min** (declaradas;
> el usuario autorizó seguir sin parada de aprobación del plan).
> Una tarea cada vez: **test en rojo → código → verde → marcar y parar**. La evidencia (comandos, salidas,
> mediciones, hallazgos) va en `journal.md`, nunca aquí. **Ninguna tarea cierra con el árbol sin compilar
> ni con tests rojos.**
> **T8 la ejecutó el planner (2026-11-05), no el implementer:** la re-base de la 013 es trabajo del
> **planner**. Queda marcada y el implementer solo la verificó (evidencia en `journal.md` y `estado.md`).
> **Las nueve tareas están cerradas** (T1–T7 y T9 por el implementer; T8 por el planner); la spec está en
> **validación** y no se marca `implementada` hasta el APROBADO.

```text
Comandos de referencia (los de la tarea son estos, sin variantes)
  backend:      cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
  full-release: cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

- [x] **T1. Comprobación única de validez y causa/acción de las dos ramas al abrir.** RF-1 · RNF-2, RNF-3,
      RNF-5 · ~40–50 min ⚠️
    - Hecho cuando: existe `validez_base::comprobar` (y `mensajeDeVersion`/`mensajeDeEsquema`, puros) que
      **reutiliza `VersionEsquema::esquemaEsperado`** y no escribe una segunda guardia; **`Causa` es
      diagnóstico interno** y los consumidores usan solo `valida()`/`mensaje` (el fallo de lectura de la
      versión se informa como `Version`); `test_comprobacion_validez` pasa: base válida → `Valida` y
      mensaje vacío; **sin la fila del general** (montaje **P10** de la 015: `DROP TRIGGER
      TiposAula_general_no_borrar`/`…_no_renombrar` antes de mutar, base en memoria por caso), sin el
      índice o sin cada disparador → `Esquema` y el mensaje nombra el objeto/la fila; con una tabla
      retirada → `Esquema`; versión 5 → `Version` con «crear una base nueva» y «restaurar un respaldo de la
      versión anterior volverá a fallar»; `noModificaElCatalogo` (mismo `sqlite_master` y `user_version`
      antes y después); `noDejaTransaccionAbierta` y `funcionaDentroDeUnaTransaccion`;
      `aperturaYComprobacionCoinciden` (mismo defecto ⇒ `AperturaBaseDatos::abrir` falla y `comprobar` es
      inválida; base buena ⇒ ambos ok); **`test_backend_apertura_base_datos` en verde con las dos ramas
      alineadas**: la de versión exige la acción nueva y la de **esquema** exige «un respaldo **de la
      versión vigente**» (hoy dice «o restaurar un respaldo» a secas); el coste medido (ms) queda en
      `journal.md`.
    - Comando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
    - Foco del rojo/verde: `ctest --preset backend -R '^(test_comprobacion_validez|test_backend_apertura_base_datos|test_backend_version_esquema)$'`

- [x] **T2. Antes de generar: el servicio no envía y la vista comprueba antes de componer.** RF-1 · RNF-3 · ~35–45
      min ⚠️
    - Hecho cuando: `ServicioGeneracion` tiene `Estado::BaseInvalida` y el parámetro `revisarBase` (antes de
      `padre`), y `generar()` comprueba **antes** de `validarEntradaSolver`; `test_servicio_generacion`
      pasa `baseInvalida_noEnvia` (el puerto recibe **0** llamadas, `finalizada()` una vez, mensaje con
      causa y acción) y `baseInvalidaTienePrioridadSobreLaEntrada` (entrada también inválida ⇒
      `BaseInvalida`); `NucleoDatos::comprobarValidezBase()` existe y `test_backend_nucleo_datos` lo prueba
      válida y sin disparador; `test_contrato_generacion` enumera **`Estado::BaseInvalida`** en
      `estadosDeGeneracion_estanExponidos` (el contrato observable queda completo);
      `schedule_generation_widget.cpp` comprueba al pulsar «Generar» antes de
      `leerOpciones()`/`construirEntrada` y cablea `revisarBase` en el único `new ServicioGeneracion`;
      `full-release` compila y pasa.
    - Comando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
      y después `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release`
    - Foco del rojo/verde: `ctest --preset backend -R '^(test_servicio_generacion|test_backend_nucleo_datos|test_contrato_generacion)$'`

- [x] **T3. Antes de guardar la configuración de la hora académica.** RF-1 · RNF-3 · 20–30 min
    - Hecho cuando: `ServicioConfiguracionHoraria::guardar` comprueba la base **antes** del rango y del
      *upsert* y, si no es válida, devuelve el mensaje de la causa + «No se guardó la configuración de la
      hora académica.» **sin escribir**; `test_servicio_configuracion_horaria` pasa
      `guardar_baseSinElTipoGeneral_rechazaSinEscribir`, `guardar_baseSinDisparador_rechazaSinEscribir`
      (mensaje con el nombre del disparador), `guardar_baseInvalidaTienePrioridadSobreElRango` (base
      inválida + 29 ⇒ mensaje de base) y `consultar_conBaseInvalida_sigueLeyendo`; los casos de rango
      existentes siguen verdes.
    - Comando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
    - Foco del rojo/verde: `ctest --preset backend -R '^test_servicio_configuracion_horaria$'`

- [x] **T4. El frontend no sustituye el tipo general.** RF-2 · RNF-1, RNF-4 · ~35–45 min ⚠️
    - Hecho cuando: `TiposAulaUI::idTipoGeneral` devuelve `Resultado<int>` y **nunca** otro tipo;
      `test_tipos_aula_ui` (binario nuevo) pasa `encuentraElGeneral`,
      `sinElGeneral_fallaYNoDevuelveElPrimero` (catálogo con «Laboratorio» y «Taller» ⇒ `!ok`),
      `catalogoVacio_falla`, `aceptaMayusculasYEspacios` («  AULA DE ESTUDIO ») y el mensaje nombra el
      tipo general y la acción (restaurar respaldo / base nueva) sin errores técnicos; las tres vistas
      (`classroom_list_widget`, `subject_list_widget`, `teacher_list_widget` en alta **y** edición)
      resuelven el tipo **antes de abrir el diálogo o de escribir**, con aviso y salida si la lectura del
      catálogo falla o si no hay tipo general; `nombreTipoGeneral` y `poblar` no cambian; `full-release`
      compila y pasa. El registro en `test/CMakeLists.txt` va **dentro de `if(TARGET frontend AND TARGET
      backend)`**, porque `frontend` no exporta `src/frontend/src` y el preset `backend` no debe verlo:
      ```cmake
      # dentro de if(TARGET frontend AND TARGET backend)
      add_qtest(test_tipos_aula_ui frontend/test_tipos_aula_ui.cpp frontend)
      target_include_directories(test_tipos_aula_ui PRIVATE ${CMAKE_SOURCE_DIR}/src/frontend/src)
      ```
    - Comando: `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release`
    - Foco del rojo/verde: `ctest --preset full-release -R '^test_tipos_aula_ui$'`

- [x] **T5. Mensaje de V18: convención y acción, sin validación nueva.** RF-3 · RNF-1, RNF-4 · 20–30 min
    - Hecho cuando: el texto de V18 explica que el tipo general es el de nombre «Aula de estudio», que el
      aula recibida es de otro tipo y la acción doble (asignar un aula del tipo general y comprobar que la
      base es válida), **conservando** el prefijo `V18:`; `test_validador_entrada` pasa con
      `exigeRechazoV18` ampliado (convención + acción) y los **seis** casos `v18_*` siguen verdes; la
      cadena de validaciones no gana ninguna validación ni cambia el orden (V19 sigue libre para la 013);
      **esta tarea edita** `docs/contrato-solver-2.0.md` con la lectura nueva del mensaje **sin** cambiar la
      versión 2.0 ni la regla.
    - Comando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
    - Foco del rojo/verde: `ctest --preset backend -R '^test_validador_entrada$'`

- [x] **T6. Tipo general fuera de la huella de datos (fijado con pruebas).** RF-4 · 15–20 min
    - Hecho cuando: `test_servicio_generacion` pasa
      `huellaDatos_noIncluyeElTipoGeneralNiSuNombreVisible` (dos dominios que solo difieren en el nombre
      visible del tipo dan la **misma** huella) sin tocar el código de `huellaDatos`;
      `huellaDatos_cambiaConElTipoDeAula` sigue verde (la identidad sí entra); y
      `grep -n "TiposAula\|tipo_general" src/backend/src/services/ServicioGeneracion.cpp` devuelve **0**.
    - Comando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
    - Foco del rojo/verde: `ctest --preset backend -R '^test_servicio_generacion$'`

- [x] **T7. Documentación de la API y cierre de la deuda de la 009.** RF-5 · ~30–40 min ⚠️
    - Hecho cuando: `docs/interfaz-frontend.md` §2.2 documenta `validez_base::comprobar` y que la validez
      se comprueba **al abrir, antes de generar y antes de guardar** (sin repetir la 015); §2.4.3 añade la
      **ayuda estricta** (falla, no sustituye), las operaciones rechazadas, el mensaje, que la base nueva ya
      trae el general, que **no hay campos ni firmas nuevas** y que **la deuda de la 009 sobre la ayuda
      queda atendida** —**§2.4.3 no tenía descripción previa de esa ayuda, así que no se retira nada**; lo
      único que se retira es la frase «catálogo vacío» que ya quitó la 015—; §2.4.4 el rechazo del
      guardado; §3.1 la firma con `revisarBase`, `Estado::BaseInvalida` y que la huella no incluye el
      catálogo; §5 la deuda atendida y la **pantalla** del catálogo como deuda viva;
      `docs/guias/migracion-frontend.md` tiene la fila de la 016 y el checklist de la ayuda; T7 **solo
      verifica** el `docs/contrato-solver-2.0.md` que editó T5 (sigue en 2.0); `docs/constitution.md`
      **intacto**.
    - Comando de verificación (documental):
      ```bash
      git diff --stat -- docs/interfaz-frontend.md docs/guias/migracion-frontend.md docs/contrato-solver-2.0.md
      grep -n "validez_base\|antes de generar" docs/interfaz-frontend.md
      grep -n "deuda de la 009" docs/interfaz-frontend.md
      grep -n "016" docs/guias/migracion-frontend.md
      ```

- [x] **T8. Re-base de la 013 sobre el esquema v6 (planner, sin código).** RF-6 · 25–30 min
    > **Ejecutada por el planner (2026-11-05), no por el implementer.** El implementer solo la verificó
    > con los `grep` de abajo; el registro de la ejecución está en `journal.md` y en `estado.md`.
    - Hecho cuando (**el planner**): `specs/013-preferencias-docente-solver/plan.md` tiene `Base` en el
      estado posterior a la 015 (sha de cierre de la 015 si está commiteada; si no, «`bb1767a` + 015 y 016
      en el árbol») y la restricción dice **«Esquema (v6)»**; `spec.md` RNF-4 y criterio 11 llevan la
      **nota de supersesión del ancla** (escenario de datos de prueba y preset **posteriores a la 015**:
      v6, tipo general garantizado, perfil sin «Aula de estudio») **sin cambiar sus requisitos**;
      `tasks.md` alinea `Base` y el criterio de T10; `estado.md` recoge la re-base y que la 013 **no tiene
      huella de esquema propia**. El implementer **solo verifica**: `grep -rn "Esquema (v5)"
      specs/013-preferencias-docente-solver/` = **0**, `grep -n "Base:" specs/013-preferencias-docente-solver/plan.md`
      con la base nueva, nota de supersesión presente en RNF-4/criterio 11, y **el alcance comprobado por
      instantánea** (el directorio **no está bajo seguimiento**: `git diff --stat` saldría vacío siempre):
      `git status --porcelain > /tmp/016-antes-T8.txt` **antes** de editar y, después,
      `git status --porcelain | diff /tmp/016-antes-T8.txt -`, exigiendo que las únicas diferencias estén
      bajo `specs/013*/` (alternativa equivalente: `git add -N specs/013-preferencias-docente-solver/` y
      `git diff --stat -- specs/013-preferencias-docente-solver/`).
    - Comando de verificación (documental):
      ```bash
      # antes de editar
      git status --porcelain > /tmp/016-antes-T8.txt
      grep -rn "Esquema (v5)" specs/013-preferencias-docente-solver/          # 0
      grep -n "Base:" specs/013-preferencias-docente-solver/plan.md           # la base nueva
      grep -n "supersedida\|posterior a la 015" specs/013-preferencias-docente-solver/spec.md
      # después de editar: solo specs/013* cambió
      git status --porcelain | diff /tmp/016-antes-T8.txt -
      #   o bien: git add -N specs/013-preferencias-docente-solver/
      #           git diff --stat -- specs/013-preferencias-docente-solver/
      ```

- [x] **T9. Regresión global.** RF-1..RF-6 · RNF-6 · 20–30 min
    - Hecho cuando: `ctest --preset backend` y `ctest --preset full-release` en verde (**56/56** y
      **61/61** esperados; toda diferencia se explica en `journal.md`); `grep -n "Aula de estudio"` en
      `src/` (sin comentarios) sigue en **1** (`common/tipo_general.hpp`); ninguna tarea dejó tests rojos
      ni el árbol sin compilar; `journal.md` recoge la evidencia de T1–T9, incluida la medición de la
      comprobación y los totales.
    - Comando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
      y después `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release`

## Tamaño

Nueve tareas, en orden de dependencia, **dentro del límite de 10** y con una tarea de holgura para el QA.
**T1, T2, T4 y T7 superan los 30 min** y quedan declaradas; el usuario autorizó seguir sin parada de
aprobación del plan (2026-11-05). Si el QA de la implementación añade trabajo, el primer re-corte es unir
T5 y T6 (ambas son de una sola pieza y sus pruebas viven en binarios existentes); si aun así no cabe, se
propone dividir la spec.
