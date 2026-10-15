# Spec 005 - Núcleo del solver y contrato de configuración

Estado: borrador

> **Cambio de requisito (modelo de dominio, 2026-10-15).** Tras aprobar la spec, el modelo de
> configuración de cursos cambió: el **modo de aula fija pasa a ser global** (booleano, por defecto
> verdadero) en lugar de un campo por curso; de cada curso sólo se configuran su `plan` (código) y su
> `num_estudiantes`; las **materias y horas de cada curso se derivan de su plan**, ya no de una
> relación persistida; y la **entrada embebe los planes de estudio** para que el solver resuelva sin
> tocar la base de datos. Afecta a RF-3 (D10), RF-7, RF-9, RF-10, RF-11 (V8), Definiciones, Casos
> límite y Criterios; el resto de RF no cambia. La spec vuelve a **borrador** (requiere re-revisión y
> re-aprobación).
>
> **Reencuadre de alcance (2026-10-15).** Esta spec cubre el **solver y todo lo suyo**: el contrato de
> configuración (forma del JSON y su serialización), el modelo (invariantes y función objetivo), las
> validaciones previas y la **construcción de la entrada del solver** desde los datos persistidos más el
> preset (RF-13). La spec 004 queda como **persistencia y servicios** puros. La frontera se mantiene:
> **el solver nunca abre la base**; 004 **persiste y expone**, 005 **lee y arma** la entrada.

> **Cambio de requisito anterior (identidad del curso en la salida), vigente.** El solver identifica
> cada curso en su salida por su **identidad de datos** (`anio` + `seccion`), **no** por el nombre
> derivado: el nombre (`"1ro A"`) es **presentación**, lo calcula quien muestra (frontend/consola).

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
por culpa de las horas.

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
  y que el suplente de mayor peso cubra solo cuando ningún titular puede, para respetar la carga
  docente.
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
- **Franja (slot):** intervalo dentro de un día (por ejemplo, una hora de clase). Todas las franjas
  tienen la misma duración en minutos, `duracion_minutos`.
- **`duracion_minutos`:** duración en minutos de cada franja; dato de la configuración. Fija la
  equivalencia entre horas y bloques y, por tanto, influye en toda conversión de horas a bloques.
- **Bloque:** unidad mínima de clase, igual a una franja. La conversión de horas a bloques es hacia
  arriba: `bloques(horas) = ceil(horas × 60 / duracion_minutos)`. **Toda comparación de horas se hace
  en bloques** (nunca horas contra bloques).
- **Turno:** conjunto de franjas de un día (por ejemplo, «mañana» o «tarde»).
- **Plan:** plan de estudios al que pertenece un curso o un docente. Un plan contiene sus materias con
  sus horas semanales.
- **Materias del curso:** las materias y sus horas semanales que un curso cursa, **derivadas de su
  plan de estudios** (plan → materias+horas). No provienen de una relación persistida curso–materia.
- **Planes embebidos:** los planes de estudio, con sus materias y horas semanales, viajan **dentro de
  la entrada del solver**, de modo que el solver resuelve sin consultar la base de datos. Se embeben
  solo los **planes referenciados por los cursos de la generación** (más los que usen los docentes);
  los embebe la **construcción de la entrada** (esta spec, RF-13).
- **Año:** nivel del curso (1, 2, 3, …).
- **Sección:** carácter que distingue cursos del mismo año (A, B, …).
- **Nombre derivado del curso:** **presentación** (su formato ordinal vive en la spec 004): se
  calcula a partir del año y la sección y lo resuelve **quien muestra** (frontend/consola). **No es un
  dato de identidad** y el **solver no lo calcula ni lo usa**.
- **Clave del curso en la salida:** la **identidad del curso** (`anio` + `seccion`), no el nombre
  derivado. El horario extraído y el archivo exportado agrupan las clases por curso usando esa
  identidad; el nombre derivado solo se calcula al mostrar (presentación).
- **Materia-curso:** la combinación (materia, curso) con sus horas semanales; la unidad a la que se
  asigna un docente. Sus horas provienen del plan del curso.
- **Docente titular:** docente para quien una materia forma parte de sus materias asignadas.
- **Docente suplente:** docente que puede cubrir una materia como reemplazo, con un **peso de
  preferencia por par** (docente, materia-suplente); a mayor peso, más preferido. El `peso` es **dato
  de la entrada**.
- **Candidato elegible de una materia-curso:** docente (titular o suplente) que puede dictarla
  respetando el plan, el turno, el año y su disposición horaria.
- **Tope de horas de aula (`horas_aula`):** cantidad de horas de aula del contrato de un docente. El
  solver lo compara **en bloques**: su tope es `bloques_aula(p) = ceil(horas_aula(p) × 60 /
  duracion_minutos)`.
- **`horas_requeridas`:** dato de dominio **opcional** del docente (horas semanales de su contrato).
  **El solver no lo usa**; solo cuenta `horas_aula`. Su verificación es un chequeo de coherencia del
  dato, no una regla del solver. Si no está presente, el chequeo se omite y no invalida la entrada.
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
- **PESO_HORAS:** peso entero de la función objetivo que penaliza cada bloque de exceso de horas de
  aula. **Constante del motor.**
- **`coste_máximo_de_suplente`:** cota superior del coste de cualquier suplente. **Constante del
  motor**, con `PESO_HORAS > coste_máximo_de_suplente > 0`.
- **Coste de suplente:** penalización entera por asignar una materia-curso a un suplente. Es **función
  del `peso` de preferencia del par**: decrece al aumentar el peso, es siempre estrictamente mayor que
  0 (el coste de un titular es 0) y nunca supera `coste_máximo_de_suplente`. Un `peso` de 0 produce el
  coste máximo.
- **`peso` de preferencia del par:** entero no negativo declarado en la entrada para cada par
  (docente, materia-suplente). A mayor `peso`, menor coste (más preferido). Las **constantes del
  motor** son solo `PESO_HORAS` y `coste_máximo_de_suplente`; el `peso` de cada par **no** es una
  constante del motor.

## Catálogo de reglas

Este catálogo reúne todas las reglas de la spec para que sea autocontenida. Cada código se desarrolla
en el RF indicado.

**Invariantes duras (D).** El solver nunca las viola.

- **D1.** Cada materia-curso la dicta exactamente un profesor. → RF-1
- **D2.** Solo el docente asignado a una materia-curso recibe clases de esa materia-curso. → RF-1
- **D3.** Cada materia-curso recibe exactamente sus horas semanales en bloques. → RF-1
- **D4.** Un curso no tiene dos clases a la vez. → RF-2
- **D5.** Un profesor no tiene dos clases a la vez. → RF-2
- **D6.** Un docente no da clase fuera de su turno. → RF-4
- **D7.** Un docente no da clase de cursos de otro plan. → RF-4
- **D8.** Un aula no aloja dos clases a la vez. → RF-2
- **D9.** Un curso móvil ocupa exactamente un aula en cada franja en que tiene clase. → RF-3
- **D10.** Un curso con aula fija (modo de aula fija activo) usa siempre su aula. → RF-3
- **D11.** Ninguna clase de un curso sale de las franjas de su turno. → RF-4
- **D12.** Eliminada: era redundante con D8 (materia única por aula).
- **D13.** Un docente no da clase de cursos fuera de sus años. → RF-4

**Prioridades blandas (S).** El solver las optimiza sin violar ninguna invariante.

- **S1.** No exceder el tope de horas de aula (exceso penalizado con `PESO_HORAS`). → RF-5
- **S2.** Preferir titulares antes que suplentes (coste de suplente decreciente con el `peso` del
  par). → RF-5

**Validaciones previas (V).** Si alguna falla, el solver no se invoca.

- **V1–V12.** Validaciones existentes: estructura y tipos, coherencia de dimensiones, índices en
  rango, turnos válidos, aula fija válida (según el modo global), número de estudiantes, listas de
  materias/profesores no vacías, un plan por curso y disponibilidad en rango. → RF-11
- **V13.** Cada materia-curso tiene al menos un candidato elegible. → RF-11
- **V14.** Un curso no necesita más bloques que franjas útiles tiene su turno. → RF-11
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
- **D3 → EL SISTEMA asigna a cada materia-curso exactamente sus horas semanales convertidas en
  bloques:** el número de bloques asignados a la materia-curso es exactamente
  `bloques(horas_semanales) = ceil(horas_semanales × 60 / duracion_minutos)`, con la misma conversión
  (y el mismo `duracion_minutos`) que usa el tope de horas de aula. Las horas semanales de una
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

### RF-4. Elegibilidad por turno, plan y año

Catálogo de invariantes D6, D7, D11 y D13 (duras):

- **D6 → SI un docente tiene turno definido, ENTONCES EL SISTEMA no le asigna clases fuera de ese
  turno.**
- **D7 → SI un docente tiene plan definido, ENTONCES EL SISTEMA no le asigna clases de cursos de otro
  plan.**
- **D13 → SI un docente tiene años definidos, ENTONCES EL SISTEMA no le asigna clases de cursos fuera
  de esos años.**
- **D11 → EL SISTEMA garantiza que ninguna clase de un curso sale de las franjas de su turno:** en la
  salida, toda clase de un curso tiene un (día, franja) perteneciente a las franjas de su turno; una
  clase fuera de ellas no es una solución válida.
- La invariante **D12 (materia única por aula) se elimina** por redundante con D8.

### RF-5. Función objetivo: tope de horas y preferencia de titulares

- **EL SISTEMA nunca viola una invariante dura para mejorar su función objetivo.**
- **S1 → EL SISTEMA evita que un docente supere su tope de horas de aula:** para cada docente, los
  bloques asignados no superan su tope **en bloques** (`bloques_aula(p) = ceil(horas_aula(p) × 60 /
  duracion_minutos)`) más un exceso no negativo; la comparación es siempre bloques contra bloques
  (nunca horas contra bloques) y usa el mismo `duracion_minutos` que D3. El exceso se penaliza con
  `PESO_HORAS` y se minimiza.
- **SI la demanda obliga a superar el tope de horas de aula de un docente, ENTONCES EL SISTEMA
  permite el exceso, lo paga en la función objetivo y lo minimiza**, en lugar de declarar el horario
  imposible.
- **S2 → EL SISTEMA prefiere docentes titulares antes que suplentes:** asignar a un titular no añade
  coste; asignar a un suplente añade un **coste entero positivo que decrece con el `peso` de
  preferencia del par (docente, materia-suplente)**, de modo que entre varios suplentes elegibles se
  prefiere el de mayor `peso`. El coste de cualquier suplente es siempre mayor que el de un titular
  (0) y nunca supera `coste_máximo_de_suplente`. La regla «un suplente solo cubre una materia-curso
  cuando ningún titular elegible puede» **emerge** de esta optimización; no se impone a mano.
- **SI el `peso` de un par (docente, materia-suplente) es 0, ENTONCES EL SISTEMA lo trata como la
  preferencia más baja entre suplentes** (coste igual a `coste_máximo_de_suplente`, mayor que el de
  cualquier suplente con `peso` positivo).
- **SI dos o más suplentes elegibles tienen el mismo `peso`, ENTONCES EL SISTEMA los considera
  equivalentes en coste y desempata de forma determinista**, de modo que la misma entrada produce la
  misma elección (RF-6).
- **EL SISTEMA usa pesos enteros:** las **constantes del motor** son `PESO_HORAS` y
  `coste_máximo_de_suplente`, con `PESO_HORAS > coste_máximo_de_suplente > 0`. El **`peso` de cada
  par** (docente, materia-suplente) es **dato de la entrada**, distinto de las constantes del motor.

### RF-6. Comportamiento y contrato del solver

- **EL SISTEMA o encuentra una solución que respeta todas las invariantes duras, o declara el
  problema no factible indicando la causa.**
- **EL SISTEMA no completa ni corrige la entrada:** si la entrada no permite una solución, no la
  «arregla» inventando datos.
- **EL SISTEMA es reproducible:** para la misma entrada produce el mismo resultado.

### RF-7. Contrato de configuración de cursos

- **EL SISTEMA identifica cada curso por su año (entero) y su sección (un carácter);** el nombre **no
  es un dato de identidad** y el solver **no lo calcula ni lo usa** (es presentación, spec 004).
- **SI una configuración de curso trae un campo de nombre, ENTONCES EL SISTEMA lo ignora** (tolerancia;
  no es un error ni motivo de rechazo).
- **CUANDO la entrada describa un curso, EL SISTEMA aceptará:** su año, su sección, su turno, su
  **plan** (código del plan de estudios, del que se derivan sus materias y horas) y su **número de
  estudiantes**; además, cuando el modo de aula fija está activo, el aula que le corresponde (índice
  válido de aula).
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
  (entero no negativo); es dato de la entrada (RF-5).
- **SI la lista de años de un docente está vacía, ENTONCES EL SISTEMA lo considerará apto para
  cualquier año.**

### RF-9. Índice de aula y traducción

- **EL SISTEMA representa las aulas por su índice en el conjunto de aulas del centro**, no por su
  nombre.
- **EL SISTEMA usa el índice de aula tanto en la asignación de salida como en el archivo exportado.**
- **CUANDO el modo de aula fija está activo, EL SISTEMA traduce el aula del curso a su índice** y lo
  usa como aula fija; el aula del curso se persiste como referencia al registro de aulas (spec 004).
- **EL SISTEMA identifica cada curso en la salida y en el archivo exportado por su identidad (año y
  sección)** (clave del horario), no por el índice de aula, ni por un nombre almacenado, ni por el
  nombre derivado (RF-3).
- La traducción de índice a nombre de aula queda a cargo de la interfaz de usuario (fuera de alcance).

### RF-10. Consistencia del contrato de configuración (serialización)

- **EL SISTEMA serializa y deserializa la configuración de forma consistente:** al guardar y volver a
  cargar una configuración con año, sección, modo de aula fija global, plan y número de estudiantes
  por curso, planes embebidos (con materias y horas), años de docente y `peso` de suplente, se obtiene
  una configuración equivalente (ida y vuelta sin pérdida).
- **SI una configuración serializada contiene los campos del contrato, ENTONCES EL SISTEMA los
  interpreta al cargarla.**
- **EL SISTEMA no incluye el nombre del curso en la configuración serializada** (se deriva de año y
  sección).
- **EL SISTEMA serializa el horario extraído agrupando las clases por curso con su identidad (año y
  sección) como clave**, sin escribir un nombre (ni almacenado ni derivado) por curso (coherente con
  el contrato de salida de la spec 002).

### RF-11. Validación previa (V1–V15)

- **MIENTRAS se prepara una generación, EL SISTEMA ejecutará la validación previa antes de invocar al
  solver; si alguna regla falla, no invocará al solver.**
- **EL SISTEMA conserva las validaciones V1–V12 existentes** (estructura y tipos, coherencia de
  dimensiones, índices en rango, turnos válidos, aula fija válida, número de estudiantes,
  materias/profesores no vacíos, un plan por curso y disponibilidad en rango).
- **V1 → SI un par (docente, materia-suplente) declara un `peso` negativo, ENTONCES EL SISTEMA
  rechazará la entrada** (chequeo de rango incluido en V1–V12).
- **V8 → SI el modo de aula fija está activo, ENTONCES EL SISTEMA exige que cada curso referencie un
  aula válida del centro; SI el modo está inactivo, ENTONCES EL SISTEMA no usa el aula de los
  cursos.** La validez la determina el **modo global**, no un campo por curso.
- **Como parte de los chequeos de datos, EL SISTEMA verifica que `horas_aula` no supere
  `horas_requeridas` cuando `horas_requeridas` está presente.** Es una comprobación de coherencia del
  dato de dominio, **no una regla del solver** (el solver solo usa `horas_aula` como tope); si
  `horas_requeridas` no está presente, el chequeo se omite y no invalida la entrada.
- **V13 → SI una materia-curso no tiene al menos un candidato elegible (titular o suplente) que
  respete plan, turno, año y disponibilidad, ENTONCES EL SISTEMA rechazará la entrada y no invocará al
  solver**, indicando la causa.
- **V14 → SI un curso necesita más bloques que franjas útiles tiene su turno, ENTONCES EL SISTEMA
  rechazará la entrada y no invocará al solver**, indicando la causa.
- **V15 → SI un curso es móvil (modo de aula fija inactivo) y el centro no dispone de aulas, ENTONCES
  EL SISTEMA rechazará la entrada y no invocará al solver**, indicando la causa.

### RF-12. Pruebas del solver en verde

- **EL SISTEMA deja en verde las pruebas del solver que hoy fallan**, incluidas las de un docente por
  materia-curso, la de suplente que reemplaza a un titular no elegible y las del escenario de gran
  tamaño.
- **Las pruebas escritas para el contrato anterior se reescriben al contrato nuevo** (el curso se
  identifica por su identidad —año+sección— en lugar del nombre almacenado, `aula_fija` global con
  aula por curso traducida a índice, materias derivadas del plan y años del docente). Reescribirlas no
  es «ajustarlas para que pasen»: deben **verificar el comportamiento correcto nuevo**.
- **CUANDO se corrija o añada una regla del solver, EL SISTEMA contará antes con una prueba que falle
  (rojo) y pasará después a verde**, siguiendo la política de pruebas del proyecto.

### RF-13. Construcción de la entrada del solver desde los datos persistidos y el preset

- **EL SISTEMA construye la entrada del solver a partir de los datos persistidos** (cursos, aulas,
  docentes, materias, planes de estudio y turnos) **más la configuración/preset del solver.**
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

- **RNF-1. Reproducibilidad:** la misma entrada produce siempre el mismo resultado, incluidos los
  empates de `peso` entre suplentes (RF-5, RF-6).
- **RNF-2. Desempeño:** el escenario de gran tamaño del conjunto de pruebas debe resolverse en menos
  de 60 segundos.
- **RNF-3. Mensajes de error en español:** todo rechazo o fallo de factibilidad identificará la causa
  y, cuando exista, la acción disponible para el usuario.
- **RNF-4. Trazabilidad:** toda regla de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Profesores «de sobra» para una materia-curso:** el solver no debe declarar imposible el horario,
  aunque la suma de topes de horas no cuadre con la demanda (RF-5).
- **Materia-curso sin candidato elegible** (por plan, turno, año o disponibilidad): la validación
  previa la rechaza y no se invoca al solver (RF-11, V13).
- **Curso cuyo turno no se solapa con la disposición del único candidato:** ningún candidato elegible;
  rechazo previo (RF-11, V13).
- **Curso con más bloques que franjas útiles de su turno:** rechazo previo (RF-11, V14).
- **Modo de aula fija inactivo (cursos móviles) en un centro sin aulas:** rechazo previo, no se invoca
  al solver (RF-11, V15).
- **Modo de aula fija activo y un curso sin aula válida:** rechazo previo (RF-7, V8).
- **Modo de aula fija inactivo y un curso con aula declarada:** el aula del curso se ignora; el curso
  es móvil (RF-7, V8).
- **Modo de aula fija no especificado:** se aplica el valor por defecto (verdadero), de forma que los
  cursos usan su aula fija (RF-7).
- **Plan de un curso sin materias:** el curso no aporta materia-curso que asignar; se aplica la
  validación previa correspondiente (RF-7, RF-11).
- **Docente con `horas_requeridas` menor que `horas_aula`:** rechazo previo por dato incoherente; es
  un chequeo de datos, no una regla del solver (RF-11). **Si `horas_requeridas` no está presente**, el
  chequeo se omite y la entrada no se invalida (RF-11).
- **Franjas de duración distinta de 60 minutos:** la conversión usa `duracion_minutos` y redondea
  hacia arriba tanto para las horas de materia (D3) como para el tope del docente (S1); se comparan
  bloques con bloques (RF-1, RF-5).
- **Varios suplentes elegibles para una materia-curso:** se prefiere el de mayor `peso` por par; un
  titular elegible siempre gana a cualquier suplente (RF-5, S2).
- **Suplente con `peso` 0:** preferencia más baja, coste máximo entre suplentes (RF-5).
- **Suplente con `peso` negativo:** rechazo previo (RF-11, V1).
- **Dos suplentes elegibles con el mismo `peso`:** equivalente en coste; desempate determinista y
  reproducible (RF-5, RF-6).
- **Dos cursos con la misma aula fija en la misma franja:** la invariante de aula no solapada (D8)
  impide la solución; si no hay alternativa, el solver declara no factible con causa (RF-2, RF-6).
- **Docente con años vacío:** apto para cualquier año (RF-8).
- **Clave del horario:** cada curso se agrupa en la salida bajo su identidad (año y sección); dos
  cursos con distinto año o sección producen claves distintas, y el nombre derivado se calcula solo al
  mostrar (RF-3, RF-9).
- **Ida y vuelta de una configuración con año, sección, modo de aula fija global, plan/número de
  estudiantes, planes embebidos, años de docente y `peso` de suplente:** debe ser equivalente y no
  contener nombre (RF-10).

## Fuera de alcance

- **Persistencia de los datos del curso y servicios de cursos** (spec 004, que **precede** a esta spec
  y le aporta los datos ya persistidos). En particular, **no se persiste** el plan, el número de
  estudiantes ni las materias del curso (ni la relación persistida curso–materia): son datos de esta
  configuración. La **construcción de la entrada** desde esos datos más el preset **sí** está en esta
  spec (RF-13).
- **Fijar al profesor a un salón en el modo de aulas móviles** («en aulas móviles, el profesor es fijo
  en un salón»): **queda diferido**, fuera de alcance por ahora.
- **Turnos automáticos y el toggle de `turno`** (spec 006): decidir si se usa el turno persistido del
  curso o si el solver lo elige. En esta spec el `turno` del curso se toma tal cual de su configuración
  (dato opcional, spec 004).
- **Generación parcial** y horarios previos (spec 007).
- **Capacidad de aula** (antigua S2) y **cobertura de emergencia** (antigua S3): no forman parte del
  catálogo de esta spec.
- **Reparto equilibrado (min-max)** de la carga entre docentes.
- **Descripción textual de franjas y recesos** y su relación con el horario mostrado.
- La **interfaz de usuario (Qt)** y la traducción de índice de aula a nombre; la aporta otro equipo.
- La **derivación del nombre del curso para mostrarlo** (presentación, formato definido en la spec
  004): la hacen quien muestra (frontend/consola), no el solver; la salida del solver identifica el
  curso por su identidad (año+sección).
- La invariante **D12 (materia única por aula)**, eliminada por redundante.
- El modelo descrito en `docs/solver/`, que queda como referencia histórica.

## Criterios de finalización

1. Las suites de pruebas del solver que hoy fallan pasan en verde, reescritas al contrato nuevo
   (RF-12).
2. El modelo respeta todas las invariantes duras del catálogo —D1–D11 y D13— y la función objetivo
   implementa S1 y S2 con `PESO_HORAS > coste_máximo_de_suplente > 0`, sin violar nunca una invariante
   (RF-1 a RF-5).
3. El contrato de configuración identifica el curso por año y sección, la **salida se indexa por esa
   identidad** (el nombre derivado es presentación, no del solver), trata el **modo de aula fija como
   global** (parte del preset) con plan y número de estudiantes por curso, deriva las materias del
   plan, admite años de docente y `peso` de suplente, y su serialización es consistente (RF-7 a RF-10).
4. La validación previa incluye V1–V15 (RF-11).
5. La entrada del solver se construye desde los datos persistidos más el preset, embebe solo los planes
   referenciados y traduce el aula del curso a su índice; el solver no consulta la base de datos
   (RF-13).
6. Existe comprobación verificable en pruebas para cada RF antes de dar la spec por implementada
   (RNF-4).

## Dudas abiertas

Ninguna. (Los planes embebidos son solo los **referenciados** por los cursos de la generación, y el
modo de aula fija **vive en el preset**; ambos cerrados.)
