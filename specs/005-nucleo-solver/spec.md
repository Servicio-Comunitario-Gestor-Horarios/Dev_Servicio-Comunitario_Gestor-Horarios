# Spec 005 - Núcleo del solver y contrato de configuración

Estado: implementada (2026-10-17)

> **Cierre.** Implementada y aprobada por el reviewer (`VEREDICTO: APROBADO`, 2026-10-17): la
> rectificación se hizo en **T11**. La **T10** sigue **abierta** por los pendientes conocidos del
> solver (cobertura de tests por regla, RF-13 y residuales), documentados en `estado.md` y `MEMORY.md`.

> **Cambio de requisitos (2026-10-17): la hora académica es un bloque.** En Venezuela la **hora
> académica** es una unidad **pedagógica**, no una hora de reloj: 45 minutos tradicionalmente, 35 desde
> la Gaceta 42.505 (2022), y en Educación Media se organiza en **bloques** (80 min, 5 horas académicas
> al día). El plan **31059 declara 44/44/46/46/46 horas académicas semanales**, no horas de reloj. La versión
> anterior de esta spec convertía horas a bloques con `ceil(horas × 60 / duracion_minutos)`, es decir,
> trataba `horas` como horas de reloj: **era incoherente** con el dato real y con
> `MAX_HORAS_SEMANALES_POR_ANIO = 40` (40 bloques/semana = 8 al día × 5 días).
>
> Se **rectifica** el modelo: **una hora académica = un bloque**; `bloques(horas) = horas` y
> `bloques_aula(horas_aula) = horas_aula` (sin ×60 ni redondeo). `duracion_minutos` pasa a definir
> **solo la duración real de la franja** (rejilla horaria, receso y disponibilidad), no la conversión.
> Se actualizan Definiciones, D3, S1, V14, RF-1, RF-5 y los casos límite. **La spec se reabrió y se
> re-aprobó el 2026-10-17**: la rectificación obliga a revisar su plan, sus tareas y la 008, y el
> trabajo en código se hace en la tarea T11 del plan.

> **Revisión tras el veredicto del reviewer (2026-10-15, CAMBIOS NECESARIOS resueltos).** El usuario
> resolvió los cinco bloqueantes y se reescribe la spec. Prioridad declarada: **un motor funcional**;
> lo que no sea imprescindible se **difiere y se documenta** (ver «Fuera de alcance»).
> - **(1) Suplente vs horas → opción (A).** Un **candidato elegible** es *factibilidad previa*: el
>   titular (o suplente declarado) que respeta plan, turno y año **y tiene disponibilidad** para la
>   materia-curso (lo que evalúa **V13**); **no** exige capacidad de horas. Un **candidato preferido**
>   es *objetivo*: el elegible que **además tiene capacidad de horas libre**; por eso, con los
>   titulares a tope, el solver **prefiere** al suplente. Se mantiene `PESO_HORAS > COSTE_SUPLENTE`.
> - **(2) Coste del suplente → FIJO.** El coste de asignar a un suplente es una **constante entera**
>   `COSTE_SUPLENTE > 0`, con `PESO_HORAS > COSTE_SUPLENTE`. El **`peso`** del par
>   (docente, materia-suplente) **se acepta pero NO se usa** todavía. La fórmula futura que lo usará
>   queda **diferida y documentada**.
> - **(3) Definiciones.** Se definen **disponibilidad**, la **regla de traducción** rango→bloques
>   (cobertura completa, recesos excluidos), las **franjas útiles** de un curso (el receso no las
>   reduce) y se **enumeran V1–V12**, marcando el chequeo **NUEVO** (`peso` no negativo).
> - **(4) Condicional (filtros del docente).** El turno, el plan y los años **del docente** se
>   interpretan **condicionalmente**: definido ⇒ restringe; **ausente ⇒ cualquiera**. **No** aplica al
>   turno ni al plan **del curso**, que son obligatorios (ver «Re-revisión QA»). Coherente con
>   `docs/constitution.md` §7.1.4 (ya enmendada).
> - **(5) QUÉ/POR QUÉ.** Se retiran de esta spec las menciones de stack/arquitectura; el CÓMO vive en
>   `plan.md` (constitución §2).
> - Además se **arreglan en la spec** varias ambigüedades del QA y se **difieren y documentan** otras
>   (RNF-1 relajado, `peso` en el objetivo, desempate entre titulares, carga mín-máx, capacidad de
>   aula, receso y horario del profesor derivado). La spec vuelve a **borrador** (re-revisión y
>   re-aprobación).

> **Cambio de requisito (modelo de dominio, 2026-10-15).** Tras aprobar la spec, el modelo de
> configuración de cursos cambió: el **modo de aula fija pasa a ser global** (booleano, por defecto
> verdadero) en lugar de un campo por curso; de cada curso sólo se configuran su `plan` (código) y su
> `num_estudiantes`; las **materias y horas de cada curso se derivan de su plan**, ya no de una
> relación persistida; y la **entrada embebe los planes de estudio** para que el solver resuelva sin
> tocar la base de datos. Afecta a RF-3 (D10), RF-7, RF-9, RF-10, RF-11 (V8), Definiciones, Casos
> límite y Criterios; el resto de RF no cambia.

> **Reencuadre de alcance (2026-10-15).** Esta spec cubre el **solver y todo lo suyo**: el contrato de
> configuración (forma del JSON y su serialización), el modelo (invariantes y función objetivo), las
> validaciones previas y la **construcción de la entrada del solver** desde los datos persistidos más el
> preset (RF-13). La spec 004 queda como **persistencia y servicios** puros. La frontera se mantiene:
> **el solver nunca abre la base**; 004 **persiste y expone**, 005 **lee y arma** la entrada.

> **Identidad del curso en la salida, vigente.** El solver identifica cada curso en su salida por su
> **identidad de datos** (`anio` + `seccion`), **no** por el nombre derivado: el nombre (`"1ro A"`) es
> **presentación**, lo calcula quien muestra.

> **Re-revisión QA (2026-10-15).** Se cierran dos incoherencias internas y se aplican los apuntes
> menores del QA.
> - **A. Turno del curso obligatorio (opción ii).** En la 005 todo curso **declara turno**; si falta,
>   la entrada se **rechaza** (V7). **D11**, la definición de **«franjas útiles»** y **V14** se evalúan
>   contra el turno **declarado**: deja de existir el caso «turno ausente». La **asignación automática
>   de turno** (un curso sin turno ⇒ el solver lo elige) queda **diferida a la spec 006**.
> - **B. Plan del curso obligatorio.** El plan es la fuente de las materias y horas del curso; si falta,
>   lo **rechazan V10/V11**. La semántica «ausente ⇒ cualquiera» **sigue vigente solo para los filtros
>   del docente** (su turno, su plan, sus años), **no** para el turno ni el plan del curso.
> - **Menores.** (1) La **capacidad de horas libre** rige la **preferencia** titular/suplente (parte del
>   objetivo), **no** una compuerta de factibilidad: el exceso sigue siendo el último recurso y un
>   docente sí puede llegar a tope; se aclara V13 y se corrige la cita «RF-11/V13» usada como condición
>   de ejecución. (2) **V8**: el «o ausente» aplica **solo** cuando el modo de aula fija está
>   **inactivo**; con el modo activo el aula del curso es obligatoria. (3) En los criterios de
>   finalización, **V2 (`duracion_minutos > 0`)** cuenta como regla **nueva** junto al `peso` no
>   negativo. (4) Se define el símbolo **`COSTE_MAX_SUPLENTE`** de la fórmula diferida.

> **Cierre de revisión completa (2026-10-15).** Se cierran los bloqueantes **B1–B3**, los importantes
> **I1–I5** y los menores **M1–M7** del veredicto del reviewer:
> - **B1.** «Franjas útiles» de un curso = `nº_días × |franjas de su turno|` (semanal); **V14** se
>   reescribe como `Σ_m horas_m ≤ nº_días × |franjas del turno(c)|` (**fórmula vigente** tras la
>   rectificación del 2026-10-17: 1 hora académica = 1 bloque). Se alinean la
>   definición,
>   el catálogo, RF-11 y los casos límite.
> - **B2.** Se **separa** el vocabulario: **elegible** = *factibilidad previa* (titular o suplente ∧
>   plan ∧ turno ∧ año ∧ **disponibilidad**; lo que evalúa **V13**), **sin** capacidad de horas;
>   **preferido** = *objetivo* (elegible que **además** tiene capacidad de horas libre). Se cuadran
>   Definiciones, RF-5, V13 y casos límite.
> - **B3.** La spec queda **coherente** con `docs/constitution.md` §7.2/§7.4 (capacidad = preferencia,
>   no compuerta; V13 sin capacidad).
> - **I1.** RF-13 precisa el reparto de campos al armar cada curso: de la **base** `anio`/`seccion`/
>   `turno`/`aula`; del **preset** `plan`/`num_estudiantes` (el plan del curso **no** se persiste).
> - **I2.** Los **avisos** de la spec 002 (capacidad de aula, horas no cubiertas) se **retiran** hasta
>   nuevo aviso, con constancia anotada en `specs/002-generacion-horarios-ipc/spec.md`.
> - **I3.** `num_estudiantes` es **opcional**; **V9** solo se aplica cuando está presente.
> - **I4.** Las **categorías de causa** de no-factible cubren **D3, D6, D7 y D13**.
> - **I5.** Caso límite «candidato único con disponibilidad **insuficiente**»: V13 pasa y el solver cae
>   por **D3**.
> - **M1.** Se define el **origen de la rejilla de bloques**. **M2.** «Franjas del turno del curso».
>   **M3.** Versión/parseo → causa **«entrada inválida»**. **M4.** Resumen «D1–D11 y D13» aclarado (D12
>   no existe). **M5.** `horas_aula = 0` definido. **M6.** RNF-1/RNF-2 acotados. **M7.** «Fuera de
>   alcance» renombrada, con lo **diferido** etiquetado dentro.

## Contexto y objetivo

El solver es la única pieza que decide el horario. Hoy su comportamiento no está garantizado: dos
suites de pruebas del solver fallan de forma estable. La causa raíz es que el modelo impone una
**igualdad dura** sobre las horas de aula de cada profesor (cada profesor debe dar exactamente sus
horas de aula), lo que vuelve el problema imposible cuando hay profesores «de sobra». Además faltan
piezas centrales del modelo: no existe la regla de «un docente por materia-curso», ni el enlace entre
la clase y el aula, ni la restricción por año, y **no hay función objetivo**: el solver acepta
cualquier solución factible sin preferir unas sobre otras.

El contrato autoritativo del solver ya está fijado en `docs/constitution.md` §7; esta spec desarrolla
su catálogo normativo (cada invariante, cada prioridad, el contrato de configuración y las
validaciones). El objetivo es dejar una **base sólida y verificable** del solver sobre la que
construir después.

Esta spec cubre el **solver y todo lo suyo**: su **núcleo**, el **contrato de configuración** (forma
del JSON y su serialización), el **modelo** (invariantes duras y función objetivo), las
**validaciones previas** (V1–V15) y la **construcción de la entrada del solver** desde los datos
persistidos más el preset (RF-13). La **persistencia** de los datos del curso (año, sección, turno y
aula) y sus **servicios** son de la **spec 004**, que **precede** a esta spec y le aporta los datos ya
persistidos; esta spec (005) los **lee y arma** la entrada. La **gestión de los presets** como
archivos **no** entra aquí.

Una decisión de modelado clave guía esta spec: minimizar el «déficit de horas» es **inerte** (la suma
de horas asignadas es constante, porque las horas totales de cada materia-curso están fijadas por una
invariante). Por eso el comportamiento operativo correcto es **tope + exceso penalizado**: cada
profesor respeta su tope de horas de aula y, si la demanda lo obliga, el exceso se permite pero se
paga y se minimiza. Así se prioriza no sobrecargar a nadie y **nunca** se declara imposible un horario
por culpa de las horas. La pieza que resuelve el caso «titular a tope» es la **capacidad de horas libre**
como **preferencia** (RF-5): un titular sin capacidad libre deja de ser preferido y su cobertura recae
en un suplente; el exceso sigue permitido como último recurso.

Los documentos de `docs/solver/` describen un modelo más rico pero quedan como **referencia histórica**
y no obligan.

## Usuarios

- **Usuario académico:** genera horarios y confía en que el resultado respete las reglas del centro.
- **Usuario que describe un curso:** indica su plan y su número de estudiantes; el aula fija es una
  decisión global, no de cada curso.
- **Usuario que configura presets y docentes:** define disposición horaria, años, planes y materias de
  cada docente.
- **Equipo de desarrollo:** mantiene el solver; necesita cada regla con su prueba y una base didáctica
  (el trabajo posterior es explicativo).

## Historias de usuario

- HU-1. Como usuario académico, quiero que el horario generado respete siempre las reglas duras (un
  docente por materia, sin solapamientos, y el turno/plan/año de cada uno), para confiar en el
  resultado.
- HU-2. Como usuario académico, quiero que el solver no declare imposible un horario cuando hay
  profesores de sobra, para poder planificar con plantillas reales.
- HU-3. Como usuario académico, quiero que ningún profesor supere sus horas de aula si se puede evitar
  y que un suplente cubra solo cuando ningún titular puede, para respetar la carga docente.
- HU-4. Como usuario que describe un curso, quiero indicar su plan y su número de estudiantes y elegir
  globalmente si las aulas son fijas o móviles, para describir el centro sin repetir datos por curso.
- HU-5. Como usuario que configura docentes, quiero indicar a qué años puede dar clase cada profesor,
  para que no se le asignen cursos fuera de su alcance.
- HU-6. Como miembro del equipo de desarrollo, quiero que las pruebas actuales del solver estén en
  verde y que cada regla tenga su prueba, para tener una base sólida y didáctica.
- HU-7. Como usuario académico, quiero que la entrada del solver se arme sola a partir de mis datos
  guardados y de la configuración, para no volver a capturar cursos, planes ni aulas.

## Definiciones

- **Invariante dura:** condición que el horario debe cumplir siempre; el solver nunca la viola ni
  siquiera para mejorar su objetivo.
- **Prioridad blanda:** preferencia que el solver optimiza; puede ceder si respetarla implicara violar
  una invariante.
- **Franja (slot):** intervalo dentro de un día (por ejemplo, un bloque de clase). Todas las franjas
  tienen la misma duración **real** en minutos, `duracion_minutos`.
- **`duracion_minutos`:** duración **real** en minutos de cada franja; dato de la configuración, **mayor
  que 0**. Define la **rejilla horaria** (inicio y fin de cada franja, recesos y traducción de la
  disponibilidad) y **no** interviene en la conversión de horas a bloques.
- **Hora académica:** unidad pedagógica de carga de una materia. **Una hora académica equivale a un
  bloque**; no es una hora de reloj.
- **Bloque:** unidad mínima de clase, igual a una franja. La conversión de horas académicas a bloques es
  la identidad: `bloques(horas) = horas`. **Toda comparación de horas se hace en bloques** (nunca horas
  contra bloques).
- **Turno:** conjunto de franjas de un día (por ejemplo, «mañana» o «tarde»). El turno **del curso** es
  **obligatorio** en esta spec (RF-7); si un curso no lo declara, la entrada se **rechaza** (V7). La
  **asignación automática de turno** queda **diferida a la spec 006**.
- **Plan:** plan de estudios al que pertenece un curso o un docente. Un plan contiene sus materias con
  sus horas semanales. El plan **del curso** es **obligatorio** (es la fuente de sus materias y horas);
  si falta, la entrada se **rechaza** (V10/V11). En cambio, el plan **del docente** es un filtro
  **condicional** (ausente ⇒ cualquiera, RF-4).
- **Materias del curso:** las materias y sus horas semanales que un curso cursa, **derivadas de su
  plan de estudios** (plan → materias+horas). No provienen de una relación persistida curso–materia
  (spec 004).
- **Planes embebidos:** los planes de estudio, con sus materias y horas semanales, viajan **dentro de
  la entrada del solver**, de modo que el solver resuelve sin consultar la base de datos. Se embeben
  solo los **planes referenciados por los cursos de la generación** (más los que usen los docentes);
  los embebe la **construcción de la entrada** (RF-13).
- **Año:** nivel del curso (1, 2, 3, …); su rango y validación son de la spec 004.
- **Sección:** carácter que distingue cursos del mismo año (A, B, …); su rango y validación son de la
  spec 004.
- **Nombre derivado del curso:** **presentación** (su formato ordinal vive en la spec 004): se
  calcula a partir del año y la sección y lo resuelve **quien muestra**. **No es un dato de identidad**
  y el **solver no lo calcula ni lo usa**.
- **Clave del curso en la salida:** la **identidad del curso** (`anio` + `seccion`), no el nombre
  derivado. El horario extraído y el archivo exportado agrupan las clases por curso usando esa
  identidad; el nombre derivado solo se calcula al mostrar.
- **Materia-curso:** la combinación (materia, curso) con sus horas semanales; la unidad a la que se
  asigna un docente. Sus horas provienen del plan del curso.
- **Docente titular:** docente para quien una materia forma parte de sus materias asignadas.
- **Docente suplente:** docente que puede cubrir una materia como reemplazo, declarado en el par
  (docente, materia-suplente).
- **`peso` de preferencia del par:** entero **no negativo** declarado en la entrada para cada par
  (docente, materia-suplente). Se **acepta** en el contrato, pero **el solver no lo usa todavía** en su
  función objetivo (coste fijo, RF-5); su uso queda **diferido**.
- **Disponibilidad del docente:** los registros de disponibilidad del docente (**día + hora desde/hasta**)
  **traducidos a bloques** mediante la regla de traducción de abajo. Un docente **sin ningún bloque de
  disponibilidad en las franjas del turno del curso no es candidato** para esa materia-curso.
- **Regla de traducción de un rango horario a bloques (cobertura completa):** un bloque se marca
  disponible **solo si el bloque entero** está contenido en el rango:
  `[inicio_bloque, fin_bloque) ⊆ [desde, hasta)`. Un rango que **no cae alineado** con los bloques
  cubre únicamente los bloques enteros que contiene; los **solapes parciales no cuentan**. Los
  **recesos no intervienen** en esta traducción (no se descuentan ni se recortan).
- **Rejilla de bloques (origen de `inicio_bloque`/`fin_bloque`):** los bloques de cada día se alinean
  con el **inicio del turno** (la hora de comienzo de su primera franja). El bloque `k` (con `k ≥ 1`)
  de un turno abarca `[inicio_turno + (k − 1) × duracion_minutos, inicio_turno + k × duracion_minutos)`.
  `inicio_bloque` y `fin_bloque`, usados en la regla de traducción, son esos instantes: la rejilla
  **no** se origina en la medianoche ni en el primer registro de disponibilidad.
- **`nº_días`:** número de días de la semana considerados en la generación; es la dimensión de días de
  la configuración (`dimensiones.num_dias`). Entra como factor **semanal** en las **franjas útiles** de
  un curso y en **V14** (`Σ_m horas_m ≤ nº_días × |franjas del turno(c)|`). Los **recesos no
  lo reducen** (coherente con «el receso no las reduce»).
- **Franjas útiles de un curso:** el número de franjas **semanales** disponibles para el curso,
  `nº_días × |franjas del turno del curso|`, donde `nº_días` es el número de días de la semana de la
  generación y `|franjas del turno del curso|` es el número de franjas de su turno **declarado**
  (obligatorio, RF-7). **El receso no las reduce** (coherente con `docs/constitution.md` §7.5): un
  receso es metadata que el solver ignora.
- **Capacidad de horas libre:** un docente tiene capacidad de horas libre para una materia-curso cuando
  puede asumir sus bloques **sin superar su tope de horas de aula**. Un docente **a tope** (que ya
  alcanzó su tope —o cuyo tope es 0—) **no** tiene capacidad libre. Al inicio de una generación, todo
  docente con tope mayor que cero dispone de capacidad libre. La capacidad de horas libre rige la
  **preferencia** (noción de **candidato preferido**; parte del objetivo, S1/S2), **no** una compuerta
  de factibilidad ni un requisito de elegibilidad: el modelo puede asignar a un docente a tope (exceso
  como último recurso).
- **Candidato elegible de una materia-curso:** docente (**titular de la materia o suplente declarado
  del par**) que (i) respeta el **turno, el plan y los años** y (ii) tiene **disponibilidad** en las
  franjas del turno del curso. La elegibilidad es **factibilidad previa** (lo que evalúa **V13**),
  evaluada **sobre los contratos, antes de resolver**; **no** incluye la capacidad de horas. Un titular
  **sin disponibilidad no es elegible**; un titular **a tope sigue siendo elegible** (solo deja de ser
  **preferido**).
- **Candidato preferido de una materia-curso:** candidato **elegible** que **además tiene capacidad de
  horas libre** para ella. Es la noción del **objetivo** (S1/S2): con los titulares a tope (sin
  capacidad libre), el solver **prefiere** al **suplente elegible**; la preferencia **nunca** viola una
  invariante dura.
- **Tope de horas de aula (`horas_aula`):** cantidad de **horas académicas** de aula del contrato de un
  docente (entero **≥ 0**). El solver lo compara **en bloques** y, al ser 1 hora = 1 bloque, su tope es
  `bloques_aula(p) = horas_aula(p)` (sin multiplicar por 60 ni dividir por `duracion_minutos`).
  **`horas_aula = 0` es válido** (docente sin horas de aula asignadas, por ejemplo
  solo suplente): su tope es 0 bloques, así que **no tiene capacidad de horas libre** y **no es
  preferido**; sigue siendo **elegible** si tiene disponibilidad, y cualquier bloque que se le asigne
  cuenta como **exceso** (último recurso, S1).
- **`horas_requeridas`:** dato de dominio **opcional** del docente (**horas académicas** semanales de su
  contrato). **El solver no lo usa**; solo cuenta `horas_aula`. Su verificación es un chequeo de
  coherencia del dato, no una regla del solver. Si no está presente, el chequeo se omite y no invalida
  la entrada.
- **Exceso:** bloques asignados a un docente por encima de su tope de horas de aula (en bloques).
- **Modo de aula fija (global):** parámetro **booleano y global** de la configuración que **vive en el
  preset** (por defecto **verdadero**). Activo → cada curso usa su aula fija (la referenciada en los
  datos del curso). Inactivo → los cursos son móviles y el solver les asigna aula. **No** es un campo
  por curso.
- **Curso móvil:** curso cuando el modo de aula fija está **inactivo**; ocupa exactamente un aula en
  cada franja en que tiene clase.
- **Curso con aula fija:** curso cuando el modo de aula fija está **activo**; usa siempre su aula.
- **Índice de aula:** posición del aula en el conjunto de aulas del centro; es el identificador que
  usan la entrada, la salida y el archivo exportado. El aula fija de un curso se traduce a su índice.
- **`num_estudiantes` — dato opcional sin regla:** dato del curso **opcional**, **aceptado** por el
  contrato y **conservado** en la ida y vuelta, pero **sin regla de negocio**: ninguna regla del solver
  lo usa (la **capacidad de aula** está fuera de alcance). El contrato comprueba su forma (entero
  positivo, V9) **solo cuando está presente**; si falta, el chequeo se omite y la entrada no se
  invalida.
- **`PESO_HORAS`:** peso entero de la función objetivo que penaliza cada bloque de exceso de horas de
  aula. **Constante del motor.**
- **`COSTE_SUPLENTE`:** coste entero **fijo** por asignar una materia-curso a un suplente. **Constante
  del motor**, con `PESO_HORAS > COSTE_SUPLENTE > 0`.
- **Coste de suplente:** en esta versión es la **constante fija `COSTE_SUPLENTE`** por materia-curso
  cubierta por un suplente (el coste de un titular es 0). La fórmula futura que lo hará depender del
  `peso` del par queda **diferida** (ver «Fuera de alcance»).
- **Configuración equivalente:** una configuración es equivalente a otra cuando el ida y vuelta
  (serializar y volver a cargar) reproduce **los mismos campos del contrato sin pérdida**; los **campos
  desconocidos** presentes en un archivo se **toleran** (se ignoran al cargar) y no se consideran
  pérdida ni error.
- **Versión del JSON de configuración:** la configuración declara una **versión de formato**; esta
  spec produce y acepta la **versión `1.0`**. Una configuración con una versión **ausente o no
  reconocida** (por ejemplo, posterior) se **rechaza** indicando la causa, que pertenece a la categoría
  **(1) entrada inválida** de «Causa de un resultado no factible» (RF-10). Lo mismo aplica a un **fallo
  de parseo** del JSON (V1).
- **Causa de un resultado no factible:** la categoría —y la entidad afectada, cuando sea identificable—
  que explica por qué no existe solución que respete todas las invariantes duras. Categorías, en el
  **orden** en que se comprueban e informan: **(1) entrada inválida** (falla una validación previa
  V1–V15, incluida la versión/parseo del contrato); **(2) conflicto de aula** (invariantes D8/D9/D10);
  **(3) conflicto de persona** (profesor o curso: D4/D5/D6/D7/D11/D13); **(4) conflicto de asignación de
  docente** (cobertura de la materia-curso: D1/D2/D3). Se informa la **primera** categoría aplicable.
  Las categorías **cubren** todas las invariantes duras D1–D11 y D13 (D12 no existe, eliminada).

## Catálogo de reglas

Este catálogo reúne todas las reglas de la spec para que sea autocontenida. Cada código se desarrolla
en el RF indicado.

**Invariantes duras (D).** El solver nunca las viola.

- **D1.** Cada materia-curso la dicta exactamente un profesor. → RF-1
- **D2.** Solo el docente asignado a una materia-curso recibe clases de esa materia-curso. → RF-1
- **D3.** Cada materia-curso recibe exactamente sus horas académicas semanales en bloques (1 hora = 1
  bloque). → RF-1
- **D4.** Un curso no tiene dos clases a la vez. → RF-2
- **D5.** Un profesor no tiene dos clases a la vez. → RF-2
- **D6.** Un docente no da clase fuera de su turno. → RF-4
- **D7.** Un docente no da clase de cursos de otro plan. → RF-4
- **D8.** Un aula no aloja dos clases a la vez. → RF-2
- **D9.** Un curso móvil ocupa exactamente un aula en cada franja en que tiene clase. → RF-3
- **D10.** Un curso con aula fija (modo de aula fija activo) usa siempre su aula. → RF-3
- **D11.** Ninguna clase de un curso sale de las franjas de su turno (turno **declarado**, obligatorio). → RF-4
- **D12.** Eliminada: era redundante con D8 (materia única por aula).
- **D13.** Un docente no da clase de cursos fuera de sus años. → RF-4

**Prioridades blandas (S).** El solver las optimiza sin violar ninguna invariante.

- **S1.** No exceder el tope de horas de aula (exceso penalizado con `PESO_HORAS`). → RF-5
- **S2.** Preferir titulares antes que suplentes (coste **fijo** `COSTE_SUPLENTE` por suplente). → RF-5

**Validaciones previas (V).** Si alguna falla, el solver no se invoca.

- **V1–V12.** Validaciones existentes (enumeradas en RF-11): estructura y tipos de las secciones,
  dimensiones, índices en rango, turno de curso **presente y válido**, aula fija válida según el modo
  global, número de estudiantes, listas de materias/profesores no vacías, **exactamente un plan por
  curso** y disponibilidad en rango. **Las reglas NUEVAS** de este conjunto son el **chequeo de `peso`
  no negativo** y el de **`duracion_minutos > 0`** (RF-11, V2).
- **V13.** Cada materia-curso tiene al menos un candidato **elegible** (titular o suplente declarado ∧
  plan ∧ turno ∧ año ∧ **disponibilidad**), evaluado **sobre los contratos**, antes de resolver; **no**
  incluye capacidad de horas (esa es preferencia del objetivo). → RF-11
- **V14.** Un curso no necesita más bloques que franjas útiles tiene:
  `Σ_m horas_m ≤ nº_días × |franjas del turno(c)|` (1 hora académica = 1 bloque). → RF-11
- **V15.** Un curso móvil exige que el centro disponga de aulas. → RF-11

**Reglas que NO existen en esta spec (fuera de alcance):** la capacidad de aula (antigua S2) y la
cobertura de emergencia (antigua S3).

## Requisitos funcionales

### RF-1. Un docente por materia-curso

Catálogo de invariantes D1, D2 y D3 (duras):

- **D1 → EL SISTEMA garantiza que cada materia de cada curso la dicta exactamente un profesor:** la
  suma de asignaciones de una materia-curso sobre todos los profesores es exactamente 1; la docencia
  no se reparte.
- **D2 → EL SISTEMA garantiza que solo el docente asignado a una materia-curso recibe clases de esa
  materia-curso:** un profesor no asignado no puede tener ninguna franja de esa materia-curso.
- **D3 → EL SISTEMA asigna a cada materia-curso exactamente sus horas académicas semanales en
  bloques:** el número de bloques asignados a la materia-curso es exactamente `horas_semanales` (una
  hora académica = un bloque; no hay multiplicación por 60 ni redondeo). Las horas semanales de una
  materia-curso provienen del plan del curso (RF-7).

### RF-2. Sin solapamientos

Catálogo de invariantes D4, D5 y D8 (duras):

- **D4 → EL SISTEMA garantiza que un curso no tiene dos clases a la vez:** para cada curso, día y
  franja, como máximo una clase.
- **D5 → EL SISTEMA garantiza que un profesor no tiene dos clases a la vez:** para cada profesor, día
  y franja, como máximo una clase.
- **D8 → EL SISTEMA garantiza que un aula no aloja dos clases a la vez:** para cada aula, día y
  franja, como máximo una clase.

### RF-3. Enlace entre clase y aula e identidad del curso en la salida

Catálogo de invariantes D9 y D10 (duras), de la derivación del aula y de la identidad del curso en la
salida:

- **D9 → MIENTRAS el modo de aula fija esté inactivo, EL SISTEMA garantiza que un curso móvil ocupa
  exactamente un aula en cada franja en que tiene clase:** el número de aulas ocupadas por el curso en
  (día, franja) es igual al número de clases del curso en esa franja.
- **D10 → MIENTRAS el modo de aula fija esté activo, EL SISTEMA garantiza que un curso usa siempre su
  aula:** en la salida, toda clase de un curso declara como aula el índice de su aula fija; una clase
  que no lo cumpla no es una solución válida.
- **CUANDO se extraiga el horario o se exporte, EL SISTEMA identificará cada curso por su identidad
  (año y sección):** la identidad es la **clave del horario** (y del archivo exportado); las clases se
  agrupan por curso bajo esa clave. El nombre derivado (`"1ro A"`) **no** lo calcula el solver: es
  presentación de quien muestra.
- **CUANDO se extraiga el horario, EL SISTEMA informará el aula de cada clase como su índice en el
  conjunto de aulas:** para un curso con aula fija, su aula fija; para un curso móvil, el aula que
  resultó ocupada en esa franja.

### RF-4. Elegibilidad por turno, plan y año (condicional)

Catálogo de invariantes D6, D7, D11 y D13 (duras). El turno, el plan y los años **del docente** se
interpretan **condicionalmente**: si están **definidos, restringen**; si están **ausentes (o vacíos),
valen cualquiera**. Esa semántica **no** se aplica al **turno ni al plan del curso**, que son
**obligatorios** (RF-7; su falta se rechaza, V7 y V10/V11).

- **D6 → SI un docente tiene turno definido, ENTONCES EL SISTEMA no le asigna clases fuera de ese
  turno; SI el turno está ausente o vacío, ENTONCES EL SISTEMA lo considera apto para cualquier turno.**
- **D7 → SI un docente tiene plan definido, ENTONCES EL SISTEMA no le asigna clases de cursos de otro
  plan; SI el plan está ausente o vacío, ENTONCES EL SISTEMA lo considera apto para cualquier plan.**
- **D13 → SI un docente tiene años definidos, ENTONCES EL SISTEMA no le asigna clases de cursos fuera
  de esos años; SI la lista de años está vacía, ENTONCES EL SISTEMA lo considera apto para cualquier
  año.**
- **D11 → EL SISTEMA garantiza que ninguna clase de un curso sale de las franjas de su turno
  declarado:** en la salida, toda clase de un curso tiene un (día, franja) perteneciente a las franjas
  del turno **declarado** por el curso (obligatorio); una clase fuera de ellas no es una solución
  válida.
- La invariante **D12 (materia única por aula) se elimina** por redundante con D8.

### RF-5. Función objetivo: tope de horas y preferencia de titulares

- **EL SISTEMA nunca viola una invariante dura para mejorar su función objetivo.**
- **S1 → EL SISTEMA evita que un docente supere su tope de horas de aula:** para cada docente, los
  bloques asignados no superan su tope **en bloques** (`bloques_aula(p) = horas_aula(p)`, porque 1 hora
  académica = 1 bloque) más un exceso no negativo; la comparación es siempre bloques contra bloques
  (nunca horas contra horas de reloj) y **no** depende de `duracion_minutos`. El exceso se penaliza con
  `PESO_HORAS` y se minimiza.
- **SI la demanda obliga a superar el tope de horas de aula de un docente, ENTONCES EL SISTEMA
  permite el exceso, lo paga en la función objetivo y lo minimiza**, en lugar de declarar el horario
  imposible. El exceso es el **último recurso dentro de la optimización** (S1): la capacidad de horas
  libre **orienta** la elección titular/suplente, **no** es una compuerta de factibilidad, de modo que
  el modelo sí puede asignar a un docente a tope cuando no hay candidato con capacidad libre.
- **S2 → EL SISTEMA prefiere docentes titulares antes que suplentes:** asignar a un titular no añade
  coste; asignar a un suplente añade un **coste entero fijo `COSTE_SUPLENTE` por materia-curso**. El
  coste de cualquier suplente es siempre mayor que el de un titular (0). Con `PESO_HORAS >
  COSTE_SUPLENTE`, cubrir con un suplente es **más barato** que provocar exceso de horas de un titular
  (que cuesta `PESO_HORAS` por bloque); por eso, si un titular **no es preferido** (a tope, sin
  capacidad libre) **o no es elegible** (sin disponibilidad), su cobertura recae en un **suplente
  elegible**. La regla «un suplente solo cubre una materia-curso cuando ningún titular con capacidad
  libre puede» **emerge** de esta optimización; no se impone a mano.
- **EL SISTEMA distingue candidato elegible de candidato preferido:** **elegible** = titular o suplente
  declarado ∧ turno/plan/año ∧ **disponibilidad** (lo que evalúa **V13** sobre los contratos, antes de
  resolver, **sin** capacidad de horas); **preferido** = elegible que **además tiene capacidad de horas
  libre**. En la resolución, la capacidad de horas libre distingue al **preferido** (un docente que ya
  cubrió su tope deja de ser preferido), **no** es una compuerta de factibilidad.
- **EL SISTEMA acepta el `peso` de preferencia de cada par (docente, materia-suplente) como entero no
  negativo, pero hoy no lo usa** en la función objetivo: el coste del suplente es la constante fija
  `COSTE_SUPLENTE` (uso del `peso` **diferido**). Bajo el coste fijo, **todos los suplentes elegibles
  son equivalentes en coste**.
- **EL SISTEMA usa pesos enteros:** las **constantes del motor** son `PESO_HORAS` y `COSTE_SUPLENTE`,
  con `PESO_HORAS > COSTE_SUPLENTE > 0`. El **`peso` de cada par** (docente, materia-suplente) es
  **dato de la entrada**, distinto de las constantes del motor.

### RF-6. Comportamiento y contrato del solver

- **EL SISTEMA o encuentra una solución que respeta todas las invariantes duras, o declara el
  problema no factible indicando la causa.** La causa se informa con las **categorías de la definición
  «Causa de un resultado no factible», en su orden**: (1) entrada inválida, (2) conflicto de aula,
  (3) conflicto de persona (profesor/curso), (4) conflicto de asignación de docente.
- **EL SISTEMA no completa ni corrige la entrada:** si la entrada no permite una solución, no la
  «arregla» inventando datos.
- **EL SISTEMA es reproducible:** bajo el **mismo JSON y el mismo solver**, produce el mismo
  resultado (RNF-1). Esta reproducibilidad **no** cubre el desempate entre candidatos equivalentes
  (titulares entre sí o suplentes entre sí): cualquiera de ellos es válido.

### RF-7. Contrato de configuración de cursos

- **EL SISTEMA identifica cada curso por su año (entero, ≥ 1) y su sección (un carácter)** —cuyo rango
  y validación son de la **spec 004** (RF-3)—; el nombre **no es un dato de identidad** y el solver
  **no lo calcula ni lo usa** (es presentación, spec 004).
- **SI una configuración de curso trae un campo de nombre, ENTONCES EL SISTEMA lo ignora** (tolerancia;
  no es un error ni motivo de rechazo).
- **CUANDO la entrada describa un curso, EL SISTEMA aceptará:** su año, su sección, su **turno**
  (**obligatorio**), su **plan** (**obligatorio**: código del plan de estudios, del que se derivan sus
  materias y horas) y su **número de estudiantes** (dato **opcional** sin regla); además, cuando el modo
  de aula fija está activo, el aula que le corresponde (índice válido de aula).
- **SI un curso no declara turno, ENTONCES EL SISTEMA rechazará la entrada** (V7): el turno del curso
  es **obligatorio** en esta spec y D11 se evalúa contra el turno **declarado**. La **asignación
  automática de turno** queda **diferida a la spec 006**. La semántica «ausente ⇒ cualquiera» de RF-4
  **no** aplica aquí.
- **SI un curso no declara plan, ENTONCES EL SISTEMA rechazará la entrada** (V10/V11): el plan del curso
  es **obligatorio** porque es la fuente de sus materias y horas. La semántica «ausente ⇒ cualquiera»
  aplica solo al **plan del docente** (RF-4/RF-8), **no** al plan del curso.
- **EL SISTEMA trata el modo de aula fija como un parámetro global de la configuración** que **vive
  en el preset** (booleano, por defecto verdadero), **no** como un campo por curso.
- **SI el modo de aula fija está activo, ENTONCES EL SISTEMA hará que cada curso use su aula fija**, y
  exigirá que el curso declare un aula válida (V8).
- **SI el modo de aula fija está inactivo, ENTONCES EL SISTEMA tratará los cursos como móviles**, no
  usará el aula de ningún curso y el solver les asignará aula.
- **EL SISTEMA deriva las materias y las horas semanales de cada curso de su plan de estudios;** no
  acepta una lista de materias por curso.

### RF-8. Contrato de configuración de docentes

- **CUANDO la entrada describa un docente, EL SISTEMA aceptará además de sus materias
  (titular/suplente), su turno y su plan, la lista de años a los que puede dictar.**
- **EL SISTEMA acepta que cada par (docente, materia-suplente) declare su `peso` de preferencia**
  (entero no negativo); es dato de la entrada (RF-5). El contrato lo **acepta pero no lo usa** todavía.
- **SI la lista de años de un docente está vacía, ENTONCES EL SISTEMA lo considerará apto para
  cualquier año** (semántica condicional, RF-4).

### RF-9. Índice de aula y traducción

- **EL SISTEMA representa las aulas por su índice en el conjunto de aulas del centro**, no por su
  nombre.
- **EL SISTEMA usa el índice de aula tanto en la asignación de salida como en el archivo exportado.**
- **CUANDO el modo de aula fija está activo, EL SISTEMA traduce el aula del curso a su índice** y lo
  usa como aula fija; el aula del curso se persiste como referencia al registro de aulas (spec 004).
- **EL SISTEMA identifica cada curso en la salida y en el archivo exportado por su identidad (año y
  sección)** (clave del horario), no por el índice de aula, ni por un nombre almacenado, ni por el
  nombre derivado (RF-3).
- La traducción de índice de aula a nombre **no** es del solver: la resuelve **quien muestra** (fuera
  de alcance).

### RF-10. Consistencia del contrato de configuración (serialización y versión)

- **EL SISTEMA declara la versión del formato de configuración que produce y acepta: `1.0`.** **SI
  una configuración declara una versión ausente o no reconocida** (por ejemplo, posterior a la
  soportada), **ENTONCES EL SISTEMA la rechazará indicando la causa** (categoría **entrada inválida**
  de «Causa de un resultado no factible», RF-6).
- **EL SISTEMA serializa y deserializa la configuración de forma consistente:** al guardar y volver a
  cargar una configuración con año, sección, modo de aula fija global, plan por curso y número de
  estudiantes (opcional), planes embebidos (con materias y horas), turno/plan/años de docente y `peso`
  de suplente, se obtiene una **configuración equivalente** (ida y vuelta sin pérdida). La equivalencia
  **tolera campos desconocidos** presentes en el archivo (se ignoran al cargar) sin considerarlos
  pérdida.
- **SI una configuración serializada contiene los campos del contrato, ENTONCES EL SISTEMA los
  interpreta al cargarla.**
- **EL SISTEMA no incluye el nombre del curso en la configuración serializada** (se deriva de año y
  sección).
- **EL SISTEMA serializa el horario extraído agrupando las clases por curso con su identidad (año y
  sección) como clave**, sin escribir un nombre (ni almacenado ni derivado) por curso (coherente con
  el contrato de salida de la spec 002).

### RF-11. Validación previa (V1–V15)

- **MIENTRAS se prepara una generación, EL SISTEMA ejecutará la validación previa antes de invocar al
  solver; si alguna regla falla, no invocará al solver.** Las comprobaciones se ejecutan y se informan
  en el orden en que se enumeran.
- **V1–V12. Validaciones existentes**, enumeradas:
  - **V1. Estructura:** el JSON es parseable y contiene las secciones requeridas.
  - **V2. Tipos y rangos de campo:** cada sección y campo tiene el tipo esperado; además,
    **NUEVA: el `peso` de cada par (docente, materia-suplente) debe ser un entero no negativo** (un
    `peso` negativo se rechaza), y **NUEVA: `duracion_minutos` debe ser mayor que 0**.
  - **V3. Dimensiones:** las dimensiones declaradas coinciden con los tamaños de las listas.
  - **V4. Índices de materias en rango.**
  - **V5. Índices de profesores en rango.**
  - **V6. Índices de aulas en rango.**
  - **V7. Turno de curso presente y válido:** el curso **declara** un turno (obligatorio; si falta, se
    **rechaza**) y ese turno pertenece al conjunto de turnos.
  - **V8. Aula fija válida según el modo global:** con el modo de aula fija **activo**, cada curso
    **declara** un aula válida (obligatoria); con el modo **inactivo**, el aula del curso **puede estar
    ausente** (se ignora).
  - **V9. Número de estudiantes (opcional):** **solo cuando el dato está presente** se comprueba su
    forma (entero positivo); no tiene regla de negocio (dato sin regla). Si falta, el chequeo se omite
    y la entrada no se invalida.
  - **V10. Listas no vacías:** cada curso **declara su plan** y tiene materias (derivadas de él) y cada
    profesor materias asignadas o suplentes.
  - **V11. Exactamente un plan por curso:** cada curso declara **un** plan (ni ausente ni más de uno).
  - **V12. Disponibilidad en rango** de días y de bloques.
- **V8 → SI el modo de aula fija está activo, ENTONCES EL SISTEMA exige que cada curso referencie un
  aula válida del centro; SI el modo está inactivo, ENTONCES EL SISTEMA no usa el aula de los
  cursos.** La validez la determina el **modo global**, no un campo por curso; el «o ausente» aplica
  **solo** con el modo **inactivo**.
- **Como parte de los chequeos de datos, EL SISTEMA verifica que `horas_aula` no supere
  `horas_requeridas` cuando `horas_requeridas` está presente.** Es una comprobación de coherencia del
  dato de dominio, **no una regla del solver** (el solver solo usa `horas_aula` como tope); si
  `horas_requeridas` no está presente, el chequeo se omite y no invalida la entrada.
- **V13 → SI una materia-curso no tiene al menos un candidato elegible (titular o suplente declarado)
  que respete plan, turno y año y tenga disponibilidad, ENTONCES EL SISTEMA rechazará la entrada y no
  invocará al solver**, indicando la causa. V13 evalúa **candidatos sobre los contratos, antes de
  resolver**, y **no** exige capacidad de horas: por eso V13 **no rechaza por horas** y el exceso sigue
  siendo el último recurso (RF-5). La capacidad de horas libre es preferencia del objetivo (S1/S2), no
  una compuerta de factibilidad.
- **V14 → SI la suma de las horas académicas de las materias de un curso supera sus franjas útiles,
  ENTONCES EL SISTEMA rechazará la entrada y no invocará al solver**, indicando la causa: la condición
  es `Σ_m horas_m ≤ nº_días × |franjas del turno(c)|` (1 hora = 1 bloque). Las franjas útiles se
  calculan sobre el turno **declarado** (obligatorio) y **el receso no las reduce** (definiciones).
- **V15 → SI un curso es móvil (modo de aula fija inactivo) y el centro no dispone de aulas, ENTONCES
  EL SISTEMA rechazará la entrada y no invocará al solver**, indicando la causa.

### RF-12. Pruebas del solver en verde

- **EL SISTEMA deja en verde las pruebas del solver que hoy fallan**, incluidas las de un docente por
  materia-curso, la de suplente que reemplaza a un titular **sin capacidad de horas libre** (no
  preferido) y las del escenario de gran tamaño.
- **Las pruebas escritas para el contrato anterior se reescriben al contrato nuevo** (el curso se
  identifica por su identidad —año+sección— en lugar del nombre almacenado, `aula_fija` global con
  aula por curso traducida a índice, materias derivadas del plan y años del docente). Reescribirlas no
  es «ajustarlas para que pasen»: deben **verificar el comportamiento correcto nuevo**.
- **CUANDO se corrija o añada una regla del solver, EL SISTEMA contará antes con una prueba que falle
  (rojo) y pasará después a verde**, siguiendo la política de pruebas del proyecto.

### RF-13. Construcción de la entrada del solver desde los datos persistidos y el preset

- **EL SISTEMA construye la entrada del solver a partir de los datos persistidos** (cursos, aulas,
  docentes, materias, planes de estudio y turnos) **más la configuración/preset del solver.**
- **EL SISTEMA arma cada curso mezclando dos orígenes:** de la **base de datos** toma su identidad y
  ubicación —`anio`, `seccion`, `turno` y `aula`—; del **preset/entrada** toma su `plan` y su
  `num_estudiantes`. El **plan del curso no se persiste** (spec 004): vive solo en el preset.
- **EL SISTEMA incluye en la entrada los planes de estudio con sus materias y horas semanales (planes
  embebidos)**, de modo que el solver resuelva **sin consultar la base de datos.** Solo se embeben los
  planes **referenciados por los cursos de la generación** (más los que usen los docentes), para no
  cargar datos no usados.
- **EL SISTEMA deriva las materias y las horas semanales de cada curso de su plan;** no lee ninguna
  relación persistida curso–materia (spec 004).
- **SI el modo de aula fija está activo, ENTONCES EL SISTEMA traducirá el aula de cada curso a su
  índice y la enlazará en la entrada; SI está inactivo, ENTONCES EL SISTEMA no enlazará aulas por
  curso.**
- **EL SISTEMA arma la entrada leyendo los datos persistidos; el solver no consulta la base de datos**
  (constitución §7). La **persistencia** de esos datos y su **exposición** son de la spec 004.

## Requisitos no funcionales

- **RNF-1. Reproducibilidad:** bajo el **mismo JSON y el mismo solver** —entendido «mismo solver» como
  **misma versión y mismos parámetros**, incluida la **aleatoriedad/semilla**—, la entrada produce
  siempre el mismo resultado. **No** se exige desempatar entre candidatos equivalentes: dos titulares
  elegibles para la misma materia-curso (o dos suplentes, bajo el coste fijo) son intercambiables y
  **cualquiera de ellos es una solución válida** (desempate **diferido**).
- **RNF-2. Desempeño:** el escenario de gran tamaño del conjunto de pruebas debe resolverse en menos
  de 60 segundos, **medido en el entorno de referencia del banco de pruebas del proyecto** (el entorno
  donde se ejecuta la suite completa), no en hardware arbitrario.
- **RNF-3. Mensajes de error en español:** todo rechazo o fallo de factibilidad identificará la causa
  (categoría y entidad cuando sea posible) y, cuando exista, la acción disponible para el usuario.
- **RNF-4. Trazabilidad:** toda regla de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Profesores «de sobra» para una materia-curso:** el solver no debe declarar imposible el horario,
  aunque la suma de topes de horas no cuadre con la demanda (RF-5).
- **Titular con capacidad de horas agotada (a tope) y con un suplente elegible:** el titular deja de ser
  **preferido** (sin capacidad libre en la resolución); la cubre el suplente, que cuesta menos que
  provocar exceso (`PESO_HORAS > COSTE_SUPLENTE`) (RF-5).
- **Titular a tope y sin ningún suplente elegible:** no hay candidato con capacidad; como último
  recurso el exceso se permite y se penaliza, en lugar de declarar imposible el horario (RF-5).
- **Materia-curso sin candidato elegible** (por plan, turno, año o disponibilidad, evaluado antes de
  resolver): la validación previa la rechaza y no se invoca al solver (RF-11, V13).
- **Único candidato elegible con disponibilidad insuficiente para cubrir todos los bloques de la
  materia-curso:** V13 **pasa** (tiene al menos un bloque de disponibilidad), pero el solver no puede
  completar las horas: **no factible** con causa **conflicto de asignación de docente** (D3)
  (RF-6, RF-11).
- **Dos titulares elegibles para la misma materia-curso:** cualquiera de ellos es válido; el desempate
  está **diferido** (RNF-1) (RF-5).
- **Varios suplentes elegibles:** bajo el coste fijo `COSTE_SUPLENTE`, todos cuestan lo mismo:
  cualquiera es válido; el desempate está **diferido** (RNF-1) (RF-5).
- **`peso` de un par negativo:** rechazo previo (RF-11, V2).
- **`duracion_minutos` menor o igual que 0:** rechazo previo (RF-11, V2).
- **Rango de disponibilidad no alineado con los bloques:** se cubren solo los bloques enteros
  contenidos en el rango; los solapes parciales no cuentan (definición de regla de traducción).
- **Receso dentro de un turno:** no reduce las franjas útiles del curso ni interviene en la traducción
  de disponibilidad (definiciones; RF-11, V14).
- **Curso cuyo turno no se solapa con la disposición del único candidato:** ningún candidato elegible;
  rechazo previo (RF-11, V13).
- **Curso cuyas horas académicas semanales superan las franjas útiles de su turno declarado**
  (`Σ_m horas_m > nº_días × |franjas del turno|`): rechazo previo (RF-11, V14).
- **Modo de aula fija inactivo (cursos móviles) en un centro sin aulas:** rechazo previo, no se invoca
  al solver (RF-11, V15).
- **Modo de aula fija activo y un curso sin aula válida:** rechazo previo (RF-7, V8).
- **Modo de aula fija inactivo y un curso con aula declarada:** el aula del curso se ignora; el curso
  es móvil (RF-7, V8).
- **Modo de aula fija no especificado:** se aplica el valor por defecto (verdadero), de forma que los
  cursos usan su aula fija (RF-7).
- **Curso sin turno:** rechazo previo; el turno del curso es **obligatorio** (RF-7, RF-11, V7). La
  asignación automática de turno está **diferida a la spec 006**.
- **Curso sin plan:** rechazo previo; el plan del curso es **obligatorio** (es la fuente de sus materias
  y horas) (RF-7, RF-11, V10/V11). El «ausente ⇒ cualquiera» aplica solo a los filtros del docente.
- **Turno, plan o años de un docente ausentes o vacíos:** son filtros **condicionales**: no restringen;
  equivalen a cualquiera (RF-4, RF-8).
- **Plan de un curso sin materias:** el curso no aporta materia-curso que asignar; se aplica la
  validación previa correspondiente (RF-7, RF-11).
- **`num_estudiantes` presente:** se acepta y se conserva, pero no tiene regla (la capacidad de aula
  está fuera de alcance); se comprueba su forma (RF-11, V9).
- **`num_estudiantes` ausente:** es **opcional**; el chequeo de forma se omite y la entrada no se
  invalida (RF-11, V9).
- **Docente con `horas_requeridas` menor que `horas_aula`:** rechazo previo por dato incoherente; es
  un chequeo de datos, no una regla del solver (RF-11). **Si `horas_requeridas` no está presente**, el
  chequeo se omite y la entrada no se invalida (RF-11).
- **Franjas de cualquier duración real (40, 45 o 60 minutos):** `duracion_minutos` solo cambia la
  rejilla horaria; una materia de 4 horas académicas ocupa **4 bloques** en cualquier caso (D3, S1).
  Se comparan bloques con bloques y no hay redondeo.
- **Dos cursos con la misma aula fija en la misma franja:** la invariante de aula no solapada (D8)
  impide la solución; si no hay alternativa, el solver declara no factible con causa (RF-2, RF-6).
- **Docente con años vacío:** apto para cualquier año (RF-4, RF-8).
- **Clave del horario:** cada curso se agrupa en la salida bajo su identidad (año y sección); dos
  cursos con distinto año o sección producen claves distintas, y el nombre derivado se calcula solo al
  mostrar (RF-3, RF-9).
- **Ida y vuelta de una configuración con año, sección, modo de aula fija global, plan/número de
  estudiantes, planes embebidos, turno/plan/años de docente y `peso` de suplente:** debe ser
  equivalente y no contener nombre (RF-10).
- **Configuración con campos desconocidos:** se cargan y se toleran; el ida y vuelta sigue siendo
  equivalente (RF-10).
- **Configuración con versión ausente o no reconocida:** se rechaza indicando la causa (RF-10).

## Fuera de alcance

Estas piezas quedan **fuera de alcance** de esta spec. Las etiquetadas **Diferido:** no son
imprescindibles para un motor funcional: se **difieren y se documentan** para no perderlas de vista y
podrían retomarse más adelante (algunas en las specs 006/007). No se implementan en esta spec.

- **Diferido: el `peso` de suplente en la función objetivo.** Hoy el coste del suplente es la constante
  fija `COSTE_SUPLENTE`; el `peso` del par se **acepta pero no se usa**. La fórmula futura
  `coste(peso) = COSTE_MAX_SUPLENTE − min(peso, COSTE_MAX_SUPLENTE − 1)` (no creciente y acotada a
  `[1, COSTE_MAX_SUPLENTE]`) **reemplazará** a la constante. Incluye, por tanto, el desempate entre
  suplentes por `peso`. **`COSTE_MAX_SUPLENTE`** es una **constante entera del motor** que fija el
  **coste máximo de la fórmula futura** (el coste de preferencia más alta; en esta spec **no se usa**,
  pues el coste sigue siendo el fijo `COSTE_SUPLENTE`).
- **Diferido: el desempate entre dos titulares elegibles para la misma materia-curso.** Hoy **cualquiera
  vale**; RNF-1 se relaja a «mismo JSON ⇒ misma solución con el mismo solver» y **no** exige resolver
  empates entre titulares.
- **Diferido: el reparto de carga mín-máx** entre docentes.
- **Diferido: la capacidad de aula** (antigua S2) y la **cobertura de emergencia** (antigua S3). La
  capacidad de aula podría **reintroducirse más adelante como aviso post-solver** (fuera del solver),
  no como invariante.
- **El receso.** Es metadata que el solver **ignora**: no reduce las franjas útiles ni interviene en
  la traducción de disponibilidad (constitución §7.5).
- **El horario del profesor derivado.** Se deriva por **post-proceso** fuera del solver (constitución
  §7.5).
- **El bool `aula_fija` global ya está decidido**: es el **modo de aula fija global** de esta spec
  (RF-7), no un pendiente.
- **Persistencia de los datos del curso y servicios de cursos** (spec 004, que **precede** a esta spec
  y le aporta los datos ya persistidos). En particular, **no se persiste** el plan, el número de
  estudiantes ni las materias del curso (ni la relación persistida curso–materia): son datos de esta
  configuración. La **construcción de la entrada** desde esos datos más el preset **sí** está en esta
  spec (RF-13).
- **Diferido: fijar al profesor a un salón en el modo de aulas móviles** («en aulas móviles, el profesor
  es fijo en un salón»), fuera de alcance por ahora.
- **Diferido: turnos automáticos y el toggle de `turno`** (spec 006): decidir si el solver
  asigna el turno cuando el curso no lo declara. En esta spec el `turno` del curso es **obligatorio**: se
  toma tal cual de su configuración y, si falta, la entrada se **rechaza** (V7). La **asignación
  automática de turno** (un curso sin turno ⇒ el solver lo elige) queda **diferida a la spec 006**.
- **Diferido: generación parcial** y horarios previos (spec 007).
- **La presentación del horario y la traducción de índice de aula a nombre:** las resuelve **quien
  muestra**, fuera del solver.
- **La derivación del nombre del curso para mostrarlo** (presentación, formato definido en la spec
  004): la hacen quien muestra, no el solver; la salida del solver identifica el curso por su identidad
  (año+sección).
- La invariante **D12 (materia única por aula)**, eliminada por redundante.
- El modelo descrito en `docs/solver/`, que queda como referencia histórica.

## Criterios de finalización

1. Las suites de pruebas del solver que hoy fallan pasan en verde, reescritas al contrato nuevo
   (RF-12).
2. El modelo respeta todas las invariantes duras del catálogo —D1–D11 y D13 (**D12 no existe:** se
   eliminó por redundante con D8)— y la función objetivo implementa S1 y S2 con `PESO_HORAS >
   COSTE_SUPLENTE > 0` (coste de suplente **fijo**), sin violar nunca una invariante; un titular a tope
   deja de ser **preferido** y lo cubre un suplente (RF-1 a RF-5).
3. El contrato de configuración identifica el curso por año y sección, exige su **turno** y su **plan**
   (obligatorios, RF-7), la **salida se indexa por esa identidad** (el nombre derivado es presentación,
   no del solver), trata el **modo de aula fija como global** (parte del preset) con plan por curso y
   número de estudiantes (**opcional**), deriva las materias del plan, admite años de docente y `peso`
   de suplente (aceptado, sin uso), declara la versión `1.0` y su serialización es consistente y
   equivalente (RF-7 a RF-10).
4. La validación previa incluye V1–V15, con V1–V12 enumeradas (las reglas **nuevas** son el chequeo de
   `peso` no negativo y el de `duracion_minutos > 0`) (RF-11).
5. La entrada del solver se construye desde los datos persistidos más el preset, embebe solo los planes
   referenciados y traduce el aula del curso a su índice; el solver no consulta la base de datos
   (RF-13).
6. Existe comprobación verificable en pruebas para cada RF antes de dar la spec por implementada
   (RNF-4).
7. La carga se mide en **horas académicas = bloques**: `bloques(horas) = horas`,
   `bloques_aula(horas_aula) = horas_aula` y `duracion_minutos` solo afecta a la rejilla horaria; V14 se
   evalúa sobre `Σ_m horas_m` (RF-1, RF-5, RF-11).

## Dudas abiertas

Ninguna.
