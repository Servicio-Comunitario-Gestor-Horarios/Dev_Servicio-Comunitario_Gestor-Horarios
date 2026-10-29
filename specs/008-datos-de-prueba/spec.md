# Spec 008 - Datos de prueba del centro (Plan de Estudio 31059)

Estado: implementada (2026-10-17)

> **Cambio de requisitos (2026-10-17): 40 horas académicas por año.** La hora del 31059 es una **hora
> académica** (= un bloque), **no** una hora de reloj; la spec 005 se rectificó (1 hora = 1 bloque,
> `duracion_minutos` solo fija la duración real de la franja). Por eso el plan General **no** se adapta a
> «26 h / 38–40 bloques calculados»: cada año suma **40 horas académicas = 40 bloques**, el techo
> semanal (8 franjas × 5 días), y la escala del 31059 baja de 44/44/46/46/46 a **40/40/40/40/40**,
> conservando las 14 áreas y la progresión. Se **elimina** la conversión `ceil(1,5 × h)` y la nota de
> granularidad asociada. La spec **se reabrió y se re-aprobó el 2026-10-17**.
>
> **Aclaración (2026-10-17, la spec sigue `aprobada`).** La **demanda total es una sola: 520 horas
> académicas** (13 cursos × 40). Las **6 materias nuevas no añaden demanda**: sustituyen horas de áreas
> base de su plan y año, ya contadas en esas áreas, así que su carga es **0** en la comprobación. No se
> usa ninguna otra base (ni 556 ni ninguna suma del catálogo que doble cuente las nuevas) para la banda
> del 85–90 % de RF-7.

> **Nota de numeración.** La **006** (turnos automáticos) y la **007** (generación parcial) están
> reservadas en `MEMORY.md` para el épico del solver; esta spec toma el **008**.
>
> **Ronda 2 (decisiones cerradas).** El conjunto deja de ser genérico: reproduce el **Plan de Estudio
> de Educación Media General de Venezuela, código 31059** (Resolución DM/0033, Gaceta Oficial 41.221
> del 24-08-2017; base de los ajustes 2024-2025), con **cuatro planes** (General y tres áreas de
> desarrollo) y **cuatro variantes de docentes** (dos válidas y dos deliberadamente inválidas).
> **Solo se puebla la base**: no se generan presets. Las preguntas D1–D5 de la ronda 1 quedan
> cerradas.
>
> **Ronda 3 (horario; vigente salvo la carga).** Se fijan **franjas de 40 minutos**: mañana
> **07:00–12:30** y tarde **12:40–18:10** (campana, con el receso incluido en el `fin`), con **8 franjas
> por turno** y un **receso de 10 minutos tras la 2.ª franja** (08:20–08:30 y 14:00–14:10). La
> **rejilla aritmética** de la 005 (anclada en el inicio, sin desplazar por el receso) termina en
> **12:20** y **18:00**: el desfase de 10 min es una consecuencia asumida. Los cursos **se siembran sin
> turno**: lo asigna el preset (spec 005, V7). La parte de carga de esta ronda queda **sustituida** por
> la nota de cambio de requisitos del 2026-10-17.
>
> **Frontera con las specs 004 y 005.** Esta spec **no** cambia el esquema, ni el modelo, ni las reglas
> del solver: describe **qué datos ficticios** deben existir, **qué forma** tienen y **qué coherencia**
> guardan entre sí. Las nociones de franja, bloque, candidato elegible, suplente y turno/plan/años del
> docente son las de la **spec 005** y no se redefinen aquí.

## Contexto y objetivo

La base de datos del cliente nace **vacía** y no existe hoy ningún conjunto de datos de prueba. Probar
la aplicación, la consola de datos o el solver exige capturar a mano turnos, recesos, aulas, materias,
planes con su carga, docentes con sus materias y su disponibilidad, y cursos con su aula. Eso hace lenta
y frágil cualquier prueba y deja a las pruebas automatizadas sin un escenario común.

El objetivo es disponer de un **conjunto de datos de prueba ficticio, completo, internamente coherente,
determinista y reutilizable** que represente un centro de **Educación Media General** con **1.º a 5.º
año**, **dos turnos (mañana y tarde)**, el **plan oficial 31059** y **tres planes derivados** con áreas
de desarrollo distintas, **aulas suficientes para todos los cursos** y **cuatro variantes de
profesorado** que cubran la demanda y que dejen, en dos de ellas, huecos de cobertura identificables.

**Por qué la coherencia es un requisito y no un adorno.** Un conjunto que no respete las reglas del
solver sería inútil como escenario: o no permitiría generar ningún horario, o permitiría uno que no
ejercita nada. En particular, el conjunto debe garantizar que **cada materia-curso tenga al menos un
docente elegible** (con disponibilidad en las franjas del turno que el preset asigne al curso, que puede
ser **cualquiera de los dos**), que **la carga semanal de cada curso quepa en las franjas útiles de
cualquiera de los dos turnos** (40 franjas semanales) y que **cada curso tenga su propia aula fija**,
además de respetar los filtros de turno, plan y años de los docentes.

**Qué no es.** No es un importador de datos reales ni una carga inicial de producción: los datos son
**ficticios**. Tampoco es un generador de horarios ni de presets: el conjunto describe el centro
persistido, no la configuración de cálculo.

## Usuarios

- **Desarrollador o QA** que necesita una base con un centro completo en segundos para probar la
  aplicación, la consola de datos o la generación.
- **Equipo de pruebas** que necesita un escenario estable, con recuentos y nombres conocidos, para
  escribir y repetir pruebas automatizadas.
- **Equipo de desarrollo del solver** que necesita entradas que no hagan fracasar la validación previa
  por motivos ajenos al motor, y entradas que sí fallen por causas conocidas y reproducibles.
- **Usuario académico en evaluación** que quiere ver el sistema con un centro con la estructura del
  plan oficial antes de capturar sus propios datos.
- **Responsable de los datos** que necesita que aplicar el conjunto no destruya datos existentes.

## Historias de usuario

- HU-1. Como desarrollador, quiero poblar una base vacía con un centro completo en una sola operación,
  para probar la aplicación sin capturar decenas de registros a mano.
- HU-2. Como miembro del equipo de pruebas, quiero que el conjunto sea siempre el mismo y tenga
  recuentos conocidos, para poder afirmar sobre él sin recalcularlo.
- HU-3. Como usuario académico, quiero ver el plan oficial 31059 con sus áreas, años y carga semanal, y
  tres planes derivados por área de desarrollo, para reconocer un centro venezolano realista.
- HU-4. Como usuario académico, quiero cursos de 1.º a 5.º con 2 o 3 secciones, repartidos entre mañana
  y tarde, y aulas propias, para que el centro se parezca a uno real.
- HU-5. Como miembro del equipo del solver, quiero poder alternar variantes de profesorado (holgada,
  suplentes, sin profesorado y sin disponibilidad) sin tocar el resto de los datos, para probar el
  camino feliz y los dos modos de rechazo.
- HU-6. Como responsable de los datos, quiero que la aplicación falle si la base ya tiene datos y que
  todo reemplazo exija confirmación y respaldo, para no perder información por accidente.

## Definiciones

- **Área de formación:** cada una de las **14 áreas** del plan 31059 (Castellano; Inglés y otras lenguas
  extranjeras; Matemáticas; Educación Física; Arte y Patrimonio; Ciencias Naturales; Física; Química;
  Biología; Ciencias de la Tierra; Geografía, Historia y Ciudadanía; Formación para la Soberanía
  Nacional; Orientación y Convivencia; Participación en Grupos de Creación, Recreación y Producción).
- **Materia-año:** entrada del catálogo que representa **un área de formación en un año concreto**
  («Castellano 1º», «Física 3º»). El modelo persistido solo admite **una materia por plan en un único
  año**; por eso cada área se representa con **una materia por año** en el que se cursa, y no con una
  materia que se repita en varios años.
- **Catálogo de materias:** conjunto único de materias-año compartido por todos los planes. Un área
  común a varios planes usa la **misma** materia-año; lo que cambia por plan son **las horas** y la
  pertenencia.
- **Plan de estudios:** conjunto de materias-año con su **año** y su **carga semanal en horas**. Un plan
  no puede tener dos veces la misma materia ni dejar un año sin materias.
- **Área de desarrollo:** eje de especialización de un plan. Los cuatro planes son **General**
  (reproduce el 31059 sin especialización), **Ciencias Exactas** (eje en Matemáticas y Física),
  **Ciencias Naturales** (eje en Biología, Química y Ciencias de la Tierra) y **Ciencias Sociales**
  (eje en Geografía, Historia y Ciudadanía, y Formación para la Soberanía Nacional).
- **Horas semanales:** carga semanal declarada de una materia-año en **horas académicas**. Una hora
  académica **es un bloque** (no una hora de reloj): `bloques(h) = h`. El conjunto conserva del 31059 el
  reparto y la progresión y **ajusta la escala** para que cada año sume 40 horas académicas (RF-2).
- **Franja y bloque:** los de la spec 005. Una **franja** es un intervalo de un día; un **bloque** es la
  unidad mínima de clase (una franja). En este conjunto **una franja dura 40 minutos** y **una hora
  académica ocupa exactamente un bloque**, sin conversión ni redondeo; los 40 minutos son la duración
  **real** del bloque, no un factor de conversión.
- **Franjas útiles de un curso:** `nº_días × |franjas del turno del curso|`, según la spec 005. Con
  **8 franjas diarias y 5 días** son **40 franjas = 40 bloques = 40 horas académicas** por semana.
- **Carga de una materia-año:** horas académicas de la materia contadas una vez por **sección** del año
  (3 en 1.º–3.º y 2 en 4.º–5.º). Las **6 materias nuevas no añaden demanda**: sus horas **sustituyen**
  horas de áreas base del mismo plan y año, que ya están contadas en la carga de esas áreas; por eso su
  carga es **0** a efectos de esta comprobación.
- **Carga asignada de un docente:** suma de la carga de sus materias-año, con la convención anterior.
  Es la carga **potencial** que el docente podría cubrir y la que se compara con su tope en RF-6/RF-7.
- **Demanda total:** suma de la carga de todas las materias-año del catálogo con la convención anterior
  = **520 horas académicas** (13 cursos × 40 horas académicas por año). Es la **única** base de la
  banda del 85–90 % de RF-7; no se usa ninguna otra cifra.
- **Semana de referencia:** **5 días**. Es una propiedad del perfil y de la configuración que lo
  acompaña (`nº_días` de la spec 005); **no se persiste** y no forma parte de los datos sembrados.
- **Turno del curso:** **no se persiste** en el conjunto: los cursos se siembran **sin turno**. Quién
  decide el turno de cada curso es la **configuración del solver** (el preset, spec 005), que lo exige
  para generar (V7). Por eso el conjunto debe cubrir **los dos turnos**.
- **Docente elegible de una materia-curso:** el de la spec 005: **titular** de la materia o **suplente
  declarado**, que respeta **turno, plan y años** y tiene **disponibilidad** en las franjas del turno
  del curso. **La capacidad de horas no es una compuerta de elegibilidad:** es una preferencia del
  objetivo. Una materia-curso sin ningún docente elegible hace fallar la **validación previa** (V13).
- **Suplente:** docente declarado como tal para una materia en la **configuración del solver** (spec
  005), no en los datos persistidos. La base solo puede expresar **qué materias tiene asignadas** un
  docente y **cuándo está disponible**; la declaración de suplencia y su `peso` viven en el preset.
- **Variante de docentes:** conjunto completo e intercambiable de docentes con sus materias asignadas y
  su disponibilidad. Hay cuatro: **holgada**, **suplentes**, **sin profesorado** y **sin
  disponibilidad**. Las dos últimas son **deliberadamente inválidas**.
- **Reemplazo total:** borrado de **todos** los datos de dominio de la base de destino y carga completa
  del conjunto.
- **Reemplazo de docentes:** borrado **solo** de los docentes, sus materias asignadas y su
  disponibilidad, y carga de otra variante; el resto de los datos no se toca.
- **Base vacía:** base con el esquema esperado y **ningún** registro de dominio.
- **Datos ficticios:** nombres, correos y contactos inventados; ninguna persona real (constitución §5).

## Requisitos funcionales

### RF-1. Catálogo de materias y modelo materia-año

- **EL SISTEMA sembrará un catálogo único de materias-año** compartido por todos los planes.
- **EL SISTEMA representará cada área de formación con una materia por año** en el que se cursa: el
  nombre de la materia incluirá el área y el año (por ejemplo, «Castellano 1º», «Física 3º»).
- **SI dos o más planes incluyen la misma área en el mismo año, ENTONCES EL SISTEMA usará la misma
  materia del catálogo** y guardará las horas de cada plan en el enlace correspondiente.
- **SI dos planes incluyen la misma materia-año con horas distintas, ENTONCES EL SISTEMA conservará
  las dos cargas por separado**, sin duplicar la materia del catálogo.
- **EL SISTEMA no creará dos materias con el mismo nombre** ni reutilizará una materia-año en un año
  distinto dentro del mismo plan.
- **EL SISTEMA no dejará materias del catálogo sin ningún plan que las use** (catálogo sin huérfanas).

### RF-2. Plan General: escala de 40 horas académicas sobre el 31059

- **EL SISTEMA sembrará el plan General con las 14 áreas de formación del 31059**, su **progresión por
  año** y el **orden relativo** entre áreas, con una **carga de 40 horas académicas por año** (la escala
  del 31059 —44/44/46/46/46— reducida al techo semanal de 40 bloques). La tabla oficial es la
  **referencia de reparto y progresión**; la carga se adapta a la capacidad del turno.
- **EL SISTEMA respetará la progresión del plan:** Arte y Patrimonio y Ciencias Naturales solo en 1.º y
  2.º; Física, Química y Biología a partir de 3.º; Ciencias de la Tierra solo en 5.º; Formación para la
  Soberanía Nacional solo en 4.º y 5.º.
- **EL SISTEMA declarará estas horas académicas semanales por área y año en el plan General:**

  | Área de formación | 1.º | 2.º | 3.º | 4.º | 5.º |
  |---|---|---|---|---|---|
  | Castellano | 4 | 4 | 4 | 4 | 4 |
  | Inglés y otras lenguas extranjeras | 5 | 5 | 5 | 5 | 3 |
  | Matemáticas | 4 | 4 | 4 | 4 | 4 |
  | Educación Física | 5 | 5 | 5 | 5 | 5 |
  | Arte y Patrimonio | 4 | 4 | – | – | – |
  | Ciencias Naturales | 5 | 5 | – | – | – |
  | Física | – | – | 4 | 4 | 4 |
  | Química | – | – | 3 | 3 | 3 |
  | Biología | – | – | 3 | 3 | 3 |
  | Ciencias de la Tierra | – | – | – | – | 2 |
  | Geografía, Historia y Ciudadanía | 6 | 6 | 5 | 3 | 3 |
  | Formación para la Soberanía Nacional | – | – | – | 2 | 2 |
  | Orientación y Convivencia | 2 | 2 | 2 | 2 | 2 |
  | Participación en Grupos de Creación, Recreación y Producción | 5 | 5 | 5 | 5 | 5 |
  | **Total de horas académicas** | **40** | **40** | **40** | **40** | **40** |
  | **Materias por año** | **9** | **9** | **10** | **11** | **12** |

- **EL SISTEMA verificará la aritmética del plan General:** el número de materias por año es
  **9/9/10/11/12**, con **51 materias-año** en total, y la suma de horas académicas de **cada** año es
  **exactamente 40** (1 hora = 1 bloque, sin redondeo).
- **EL SISTEMA rechazará la aplicación si el plan General no coincide con esa tabla o si algún año no
  suma 40 horas académicas**, indicando el año y la diferencia.

### RF-3. Planes de área de desarrollo

- **EL SISTEMA sembrará cuatro planes:** el General (RF-2) y tres derivados: **Ciencias Exactas**,
  **Ciencias Naturales** y **Ciencias Sociales**.
- **EL SISTEMA mantendrá en cada plan derivado las 14 áreas del 31059** con su progresión por año, y
  añadirá **al menos dos materias nuevas propias de su área de desarrollo**, solo en 3.º, 4.º o 5.º
  (nunca en 1.º ni en 2.º).
- **EL SISTEMA asignará cada materia nueva a un solo plan:** las materias específicas de un área de
  desarrollo no se comparten con los otros planes derivados ni con el General.
- **EL SISTEMA variará las horas de al menos dos áreas base** en al menos un año respecto del **plan
  General adaptado** (RF-2), y documentará esas variaciones junto con el perfil.
- **EL SISTEMA mantendrá cada año de cada plan derivado en 40 horas académicas o menos**
  (`Σ_m horas_m ≤ 40`; 1 hora = 1 bloque, sin conversión), de modo que la carga quepa en las franjas
  del turno; cuando añada materias, reducirá horas de otras áreas del mismo año para no superar el
  límite. La justificación es el área de desarrollo: más carga en las áreas del eje, menos en las
  complementarias.
- **EL SISTEMA declarará en el perfil el total semanal por año de cada plan derivado, en horas
  académicas (= bloques), y el total coincidirá con la suma de sus materias-año.**
- **EL SISTEMA garantizará que los tres planes derivados sean distintos entre sí** (no solo en el
  nombre) y que ninguno quede con un año sin materias.
- **SI un plan derivado no cumple estas condiciones, ENTONCES EL SISTEMA rechazará la aplicación**
  indicando el plan, el año y la regla incumplida.

### RF-4. Cursos, secciones y turnos

- **EL SISTEMA sembrará cursos de 1.º a 5.º año** con **tres secciones en 1.º, 2.º y 3.º** (A, B, C) y
  **dos en 4.º y 5.º** (A, B): **13 cursos**.
- **EL SISTEMA usará secciones de un solo carácter en mayúscula** y no repetirá la pareja año+sección.
- **EL SISTEMA sembrará los cursos SIN turno:** ningún curso declara turno en los datos. El turno de cada
  curso lo decide la **configuración del solver** (el preset, spec 005), que lo exige para generar (V7).
  Por eso el conjunto debe cubrir **los dos turnos** en su profesorado (RF-6, RF-7) y no reparte cursos
  entre turnos.
- **EL SISTEMA asignará a cada curso un aula**, sin dejar ninguno sin aula.
- **EL SISTEMA sembrará dos turnos con 8 franjas diarias de 40 minutos**, ambos con el mismo número de
  franjas, con estas ventanas y este receso:

  | Turno | Ventana (campana) | Franjas | Receso |
  |---|---|---|---|
  | Mañana | 07:00–12:30 | 8 × 40 min | 10 min tras la 2.ª franja (08:20–08:30) |
  | Tarde | 12:40–18:10 | 8 × 40 min | 10 min tras la 2.ª franja (14:00–14:10) |

  El turno se registra con **inicio 07:00 y fin 12:30** (mañana) y **inicio 12:40 y fin 18:10** (tarde);
  el `fin` guardado **incluye el receso de 10 minutos** y representa el horario de campana. El receso se
  registra **después de la franja 2** (`despues_de_slot = 2`) en ambos turnos, con **inicio y fin
  explícitos** (08:20–08:30 y 14:00–14:10).
- **EL SISTEMA usará la rejilla aritmética de la spec 005:** los bloques se anclan en el **inicio del
  turno** y avanzan en pasos de `duracion_minutos`; **el receso no desplaza bloques**. Con `inicio` y
  `duracion_minutos = 40`, la rejilla es:

  | Bloque | Mañana | Tarde |
  |---|---|---|
  | 1 | 07:00–07:40 | 12:40–13:20 |
  | 2 | 07:40–08:20 | 13:20–14:00 |
  | 3 | 08:20–09:00 | 14:00–14:40 |
  | 4 | 09:00–09:40 | 14:40–15:20 |
  | 5 | 09:40–10:20 | 15:20–16:00 |
  | 6 | 10:20–11:00 | 16:00–16:40 |
  | 7 | 11:00–11:40 | 16:40–17:20 |
  | 8 | 11:40–**12:20** | 17:20–**18:00** |

  **Desfase de 10 minutos (consecuencia asumida).** La rejilla aritmética del solver termina en
  **12:20** (mañana) y **18:00** (tarde), mientras que el `fin` guardado es **12:30** y **18:10** porque
  incluye el receso. El modelo de la spec 005 **ignora el receso** y no desplaza bloques, así que el
  bloque 3 empieza a las 08:20 (mañana) y a las 14:00 (tarde), es decir, **durante la campana del
  receso**. El desfase se documenta y se asume: el conjunto **no** intenta corregirlo ni inventa una
  rejilla alternativa.

  > **Superado por la 011 (2026-10-29).** El inventario de 8 franjas de 40 min y la rejilla
  > `8 × 40 + 10` se sustituyen por 8 franjas de D = 35: Mañana 07:00–11:50 y Tarde 12:40–17:30,
  > recesos 08:10–08:20 y 13:50–14:00 (`8 × 35 + 10 = 290`). La rejilla de clases termina en 11:40 y
  > 17:20; el `fin` añade el evento de 10 min (el mismo desfase de 10 min que ya se documentaba aquí).

  **Nota de indexado de `despues_de_slot`.** El esquema admite el valor **0** y **no documenta** si el
  índice es 1-based (el receso cae tras la franja 2) o 0-based (tras la franja 1). Con
  `despues_de_slot = 2` el receso cae a las **08:20** (mañana) y **14:00** (tarde) **si el índice es
  1-based**, que es la convención que este conjunto asume; el plan debe **confirmar esa convención en la
  spec 004/005** y, si fuese 0-based, ajustar el valor a 1 para que el receso caiga en las mismas horas.
  La rejilla aritmética de bloques **no depende** de esa convención.
- **EL SISTEMA documentará que la semana de referencia del conjunto tiene 5 días** (de lunes a viernes)
  como propiedad del perfil y de la configuración que lo acompaña; **el número de días no se persiste**
  ni forma parte de los datos sembrados.

  **Nota aritmética.**

  - Con **8 franjas por día y 5 días** hay **40 franjas por semana** por turno: ese es el techo de carga
    de un curso, y coincide con `MAX_HORAS_SEMANALES_POR_ANIO = 40`.
  - La spec 005 fija **1 hora académica = 1 bloque**: `bloques(horas) = horas`. Los **40 minutos** son
    la duración **real** de la franja (rejilla horaria, receso y disponibilidad) y **no** intervienen en
    la conversión.
  - El 31059 declara **44–46 horas académicas** por año; como el techo es **40**, el conjunto **reduce
    la escala a 40/40/40/40/40** (RF-2) conservando las 14 áreas y la progresión. No hay “redondeo” ni
    pérdida por granularidad: cada materia ocupa exactamente sus horas académicas en bloques.
  - La configuración que acompañe al conjunto debe declarar la misma franja (40 minutos) y 8 franjas por
    turno; el conjunto **no genera presets** (ver «Fuera de alcance»).
  - **Receso y franjas útiles:** el receso **no** reduce las franjas útiles ni interviene en la
    traducción de disponibilidad (spec 005); es metadata del turno.

### RF-5. Aulas

- **EL SISTEMA sembrará al menos 13 aulas, y por defecto 16**, de modo que todos los cursos tengan aula
  propia y queden aulas de reserva.
- **EL SISTEMA dará a cada aula un nombre único**, una capacidad y una ubicación (edificio y piso)
  ficticios, **con variedad de capacidades y ubicaciones**.
- **EL SISTEMA incluirá al menos un laboratorio o taller** utilizable por las áreas de ciencias, para
  que el conjunto refleje un centro real.
- **EL SISTEMA fijará capacidades suficientes para un grupo completo**, de modo que el conjunto no
  plantee dudas de aforo al revisarlo a ojo (la capacidad no es una regla del solver, spec 005).
- **EL SISTEMA dará a cada curso un aula propia y distinta**, de modo que la unicidad del aula fija se
  cumpla por construcción (spec 005) con el **modo de aula fija, que es global y está activo por
  defecto**. Como los cursos no declaran turno (RF-4), no se puede —ni hace falta— razonar por turno
  para las aulas: basta con que **ningún aula se repita entre cursos**.

### RF-6. Variante de docentes «holgada»

- **EL SISTEMA sembrará una plantilla que cubra todas las materias-año de los cuatro planes en los dos
  turnos**, con al menos un titular por materia-año y, **para cada uno de los dos turnos**, al menos un
  docente con disponibilidad que solape una franja completa de ese turno. Como los cursos se siembran
  sin turno (RF-4), la cobertura debe ser **en ambos turnos siempre**: el preset puede asignar cualquier
  curso a cualquiera de los dos.
- **EL SISTEMA asignará a cada docente entre una y tres materias** y declarará su disponibilidad por
  día y rango horario, **alineada con las franjas de los turnos** (franjas completas, regla de la spec
  005), cubriendo uno o ambos turnos según el caso.
- **EL SISTEMA expresará la disponibilidad en rangos que empiecen y terminen en fronteras de la rejilla
  aritmética** (`inicio + k × 40`), de modo que cada rango cubra bloques completos. Ejemplos: en la
  mañana **`07:00–09:00` cubre 3 bloques** (07:00–07:40, 07:40–08:20 y 08:20–09:00); en la tarde
  **`12:40–15:20` cubre 4** (12:40–13:20, 13:20–14:00, 14:00–14:40 y 14:40–15:20); un rango como
  **`08:10–08:40` no contiene ningún bloque completo** y no cuenta. El receso **no desplaza los
  bloques** (RF-4): un rango puede abarcar la campana del receso (08:20–08:30 / 14:00–14:10) sin que
  cambien los bloques cubiertos.
- **EL SISTEMA mantendrá la carga asignada de cada docente claramente por debajo de su tope de horas de
  aula** (como máximo el 60 % del tope), **medida en horas académicas (= bloques)**, de modo que la
  variante holgada no dependa de la suplencia ni del exceso.
- **EL SISTEMA usará nombres, correos y teléfonos ficticios, con acentos y eñes**, para ejercitar el
  ordenamiento en español.
- **EL SISTEMA permitirá verificar a nivel de datos la cobertura de la variante:** cada materia-año de
  cada plan tiene al menos un docente asignado y, **para cada uno de los dos turnos**, al menos un
  docente de esa materia-año tiene disponibilidad que solapa **una franja completa** de ese turno.
- **EL SISTEMA dejará constancia en el perfil de los indicadores de cobertura** (materias-año cubiertas,
  docentes y carga asignada por docente), para que las pruebas los contrasten sin invocar al solver.

### RF-7. Variante de docentes «suplentes»

- **EL SISTEMA mantendrá la cobertura completa de materias-año y turnos** (como en RF-6), pero con una
  plantilla de titulares cuya **carga asignada esté cerca de su tope** (al menos el 80 % del tope).
- **EL SISTEMA sembrará docentes cualificados adicionales sin materias asignadas**, con disponibilidad
  en las franjas necesarias, para que la configuración del solver pueda declararlos como **suplentes**
  de las materias correspondientes.
- **EL SISTEMA dimensionará la plantilla de modo que la capacidad total de los titulares sea inferior a
  la demanda**: la suma de sus topes quedará **entre el 85 % y el 90 % de la demanda total** (10–15 %
  por debajo), medida en horas académicas. Con los 13 cursos a 40 horas académicas por año, la demanda
  de referencia es **520** y el rango de topes **442–468**. Así, un preset que declare a los docentes
  adicionales como suplentes tendrá que recurrir a ellos.
- **EL SISTEMA permitirá verificar a nivel de datos la variante:** la cobertura de materias-año y turnos
  se mantiene, la suma de topes de los titulares queda por debajo de la carga demandada y los docentes
  cualificados adicionales tienen disponibilidad que solapa las franjas necesarias.
- **EL SISTEMA no declarará el resultado de una generación:** la preferencia titular/suplente y el coste
  de suplente son los de la spec 005 y se comprueban con un preset de prueba aportado por las pruebas,
  no con estos datos.
- **EL SISTEMA conservará el resto de los datos idéntico a la variante holgada** (mismos planes,
  cursos, aulas y turnos), para que la única diferencia observable sea el profesorado.

### RF-8. Variantes inválidas de docentes

- **EL SISTEMA sembrará una variante «sin profesorado»** en la que **al menos una materia-año queda sin
  ningún docente que la tenga asignada**, de modo que la cobertura a nivel de datos falla en esa
  materia-año **en ambos turnos** (no habría candidato elegible para V13 con ningún turno que asigne el
  preset).
- **EL SISTEMA sembrará una variante «sin disponibilidad»** en la que **existen docentes asignados a
  todas las materias-año, pero para al menos una materia-año ninguno de sus docentes tiene
  disponibilidad que solape ninguna franja de ninguno de los dos turnos**: el solape es **nulo en ambos
  turnos**, no parcial, de modo que el fallo es de elegibilidad (V13) sea cual sea el turno que asigne
  el preset, y no de asignación de horas (D3).
- **EL SISTEMA no modelará la segunda invalidez como falta de horas:** la capacidad de horas es
  preferencia del objetivo y **no** una compuerta de elegibilidad (spec 005).
- **EL SISTEMA documentará, para cada variante inválida, la materia-año afectada y la condición de
  cobertura que falta** (sin docente asignado o sin solape de disponibilidad), indicando que el fallo
  cubre **ambos turnos**, de modo que una prueba pueda comprobarlo sobre los datos.
- **EL SISTEMA mantendrá el resto de los datos idéntico a la variante holgada**, para que el fallo de
  cobertura sea atribuible a la variante.

> **Nota de verificación (alcance del entregable).** El conjunto se verifica **a nivel de datos**:
> cobertura de candidatos, carga asignada, solapes y disponibilidad. **No promete por sí solo el
> comportamiento del solver:** comprobar V13 o una generación completa requiere un **preset de prueba
> aportado por las pruebas**, que **el conjunto no genera** y que queda fuera de su entregable. Las
> variantes describen el estado de los datos, no el veredicto de una generación.

### RF-9. Conmutación de variante de docentes

- **CUANDO se pida cambiar la variante de docentes, EL SISTEMA eliminará únicamente los docentes, sus
  materias asignadas y su disponibilidad**, y cargará la variante pedida.
- **EL SISTEMA dejará intactos los turnos, recesos, aulas, materias, planes y cursos** durante la
  conmutación: antes y después, esos datos son idénticos.
- **EL SISTEMA exigirá confirmación explícita antes de borrar docentes y dejará respaldo previo**
  (constitución §5), porque la operación es destructiva.
- **SI la variante pedida no existe, ENTONCES EL SISTEMA rechazará la operación sin tocar ningún dato.**
- **SI la conmutación falla a mitad, ENTONCES EL SISTEMA dejará la variante de docentes anterior
  completa**, sin estados intermedios.
- **CUANDO se aplique dos veces la misma variante, EL SISTEMA dejará el mismo estado final**, sin
  duplicar docentes, materias asignadas ni disponibilidades.

### RF-10. Aplicación inicial, reemplazo y protección de la base

- **CUANDO se aplique el conjunto, EL SISTEMA exigirá que se indique la base de destino**; nunca actuará
  sobre una base no indicada.
- **SI la base de destino no existe, ENTONCES EL SISTEMA la creará con el esquema esperado y la
  poblará.**
- **SI la base de destino existe y está vacía, ENTONCES EL SISTEMA la poblará.**
- **SI la base de destino ya tiene datos de dominio, ENTONCES EL SISTEMA fallará por defecto sin
  escribir**, indicando que la base no está vacía y que existe la opción de reemplazo.
- **SI se pide reemplazo total, ENTONCES EL SISTEMA exigirá confirmación explícita, dejará respaldo
  previo y solo después borrará y repoblará**; la operación es atómica.
- **SI el respaldo no se puede completar, ENTONCES EL SISTEMA abortará sin borrar nada** (constitución
  §5).
- **SI la base no tiene el esquema esperado, ENTONCES EL SISTEMA fallará sin escribir** (spec 004).
- **EL SISTEMA no dejará la base a medio poblar:** o quedan todos los datos del perfil, o no queda
  ninguno.

### RF-11. Determinismo e identificadores estables

- **CUANDO se aplique el mismo perfil dos veces sobre bases vacías, EL SISTEMA dejará los mismos
  datos:** mismos nombres, mismos códigos, mismos recuentos y mismas relaciones.
- **EL SISTEMA no usará azar ni dependerá de la fecha o la hora** para decidir qué datos crea.
- **EL SISTEMA usará identificadores legibles y estables** (correlativos por dominio y códigos de plan
  fijos) que permitan a las pruebas referirse a un registro concreto sin buscarlo por posición.
- **EL SISTEMA no incluirá las marcas de auditoría de los registros en la identidad ni en las
  comprobaciones del conjunto.**

### RF-12. Informe, verificabilidad y reutilización por las pruebas

- **CUANDO termine la aplicación, EL SISTEMA informará los recuentos por entidad** (turnos, recesos,
  aulas, materias, planes, materias por plan y por año, docentes, materias asignadas, disponibilidades y
  cursos) y el resultado coincidirá con el perfil.
- **EL SISTEMA informará el total semanal por año de cada plan, en horas académicas (= bloques), y lo
  contrastará con el declarado**, incluidos los **40/40/40/40/40** del plan General.
- **SI la aplicación falla, ENTONCES EL SISTEMA terminará con un resultado de fallo** y un mensaje en
  español que identifique la causa y la acción disponible.
- **EL SISTEMA expondrá el perfil, los recuentos esperados y los identificadores estables de forma
  reutilizable por las pruebas automatizadas**, para que estas afirmen sobre ellos sin recalcularlos a
  mano.
- **EL SISTEMA permitirá comprobar sobre los datos aplicados la cobertura de materias-año y turnos**, de
  modo que una prueba pueda verificar la coherencia sin generar un horario.

### RF-13. Documentación de uso

- **EL SISTEMA documentará cómo aplicar el conjunto, los cuatro planes, las cuatro variantes de docentes,
  las opciones de reemplazo y los recuentos esperados**, en español y en la documentación viva del
  proyecto.
- **La documentación advertirá de que los datos son ficticios**, de que el conjunto **no genera
  presets** y de qué datos del solver viven en el preset (spec 005).

## Requisitos no funcionales

- **RNF-1. Tiempo de aplicación:** aplicar el perfil por defecto sobre una base vacía termina en menos
  de 15 segundos en el entorno de referencia del proyecto.
- **RNF-2. Sin dependencias nuevas ni cambios de esquema:** el conjunto usa el modelo existente; no
  añade dependencias, tablas ni columnas.
- **RNF-3. Mensajes en español:** todo rechazo identifica la entidad, el parámetro o la regla
  incumplida y, cuando exista, la acción disponible.
- **RNF-4. Datos ficticios:** ningún dato personal real (nombres, correos, teléfonos); constitución §5.
- **RNF-5. Estabilidad:** el conjunto no depende del reloj, del orden de inserción ni de la máquina; dos
  aplicaciones del mismo perfil son equivalentes **salvo las marcas de auditoría** (`fecha_creacion` y
  `fecha_modificacion`, que se rellenan con el instante de la aplicación y quedan fuera de la
  equivalencia y de las comprobaciones, RF-11).
- **RNF-6. Atomicidad y reversibilidad:** toda aplicación o reemplazo es atómico y deja respaldo previo
  cuando destruye datos (constitución §5).
- **RNF-7. Trazabilidad:** cada RF de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Base de destino con datos:** falla por defecto sin escribir; con reemplazo total pide confirmación y
  exige respaldo previo (RF-10).
- **Base de destino inexistente:** se crea con el esquema esperado y se puebla (RF-10).
- **Base con un esquema que no es el esperado:** fallo sin escribir (RF-10).
- **Respaldo imposible o fallido:** se aborta sin borrar nada (RF-10).
- **Interrupción a mitad de la aplicación o del reemplazo:** la base queda como estaba (atomicidad,
  RF-9, RF-10).
- **Variante de docentes desconocida:** rechazo sin tocar datos (RF-9).
- **Conmutación de variante sobre una base sin docentes:** se cargan los de la variante pedida, sin
  error (RF-9).
- **Conmutación repetida de la misma variante:** mismo estado final, sin duplicados (RF-9).
- **Materia compartida entre planes con horas distintas:** una sola materia en el catálogo y una carga
  por plan, sin duplicarla (RF-1).
- **Materia nueva de un plan derivado:** solo aparece en ese plan; no se comparte (RF-3).
- **Plan derivado con un año por encima de 40 horas académicas:** se rechaza; la carga debe caber en
  las 40 franjas semanales del turno (RF-3, RF-4).
- **Plan General que no coincide con la tabla adaptada o con un año distinto de 40 horas académicas:**
  se rechaza indicando el año y la diferencia (RF-2).
- **Materia con más horas académicas que franjas tiene el año:** se rechaza; cada hora académica ocupa
  exactamente un bloque y la suma del año no puede superar 40 (RF-2, RF-3).
- **Materia-año sin ningún docente asignado (variante «sin profesorado»):** la cobertura a nivel de datos
  falla en esa materia-año **en ambos turnos**; documentada como tal (RF-8).
- **Cobertura de disponibilidad en un solo turno (variantes «holgada» y «suplentes»):** no basta; cada
  materia-año debe tener cobertura en **ambos** turnos, porque el preset elige el turno del curso
  (RF-6, RF-7).
- **Materia-año con solape nulo en ambos turnos (variante «sin disponibilidad»):** la cobertura falla por
  falta de solape, **no** por horas, sea cual sea el turno que asigne el preset (RF-8).
- **Materia-año con solape en uno solo de los turnos (variante «sin disponibilidad»):** no basta para
  invalidarla; la variante exige **solape nulo en ambos turnos** (RF-8).
- **Disponibilidad con solape parcial** (por ejemplo, 08:10–08:40): no contiene ningún bloque completo y
  no cuenta (RF-6).
- **Disponibilidad que abarca la campana del receso** (por ejemplo, 07:00–09:00): cubre **3 bloques**
  (07:00–07:40, 07:40–08:20 y 08:20–09:00); el receso no desplaza los bloques ni se descuenta de las
  franjas útiles (RF-4, RF-6).
- **Fin del turno frente a fin de la rejilla:** el turno se guarda hasta 12:30/18:10 (campana, con
  receso), pero la rejilla aritmética termina en 12:20/18:00; el desfase de 10 minutos es una
  consecuencia asumida del modelo de la 005 (RF-4).
- **Convención de indexado de `despues_de_slot`:** si fuese 0-based, el receso caería tras la franja 1 y
  habría que usar 1 para que quede a las 08:20/14:00; el plan debe confirmar la convención en la
  004/005 (RF-4).
- **Curso sin turno:** es el estado normal del conjunto; el turno lo aporta el preset al generar (RF-4).
- **Aula repetida entre dos cursos:** no ocurre; cada curso tiene un aula distinta (RF-5).
- **Sección repetida en el mismo año:** no ocurre; la pareja año+sección es única (RF-4).
- **Preset con una duración de franja distinta de 40 minutos o con otro número de franjas:** queda fuera
  del conjunto; los 40 minutos como duración **real** y las 8 franjas por turno (40 por semana) son la
  convención del conjunto (RF-2, RF-4), y el preset no lo genera este conjunto. La carga del plan **no**
  cambia con esa duración.
- **Nombres con acentos, eñes o mayúsculas mezcladas:** se siembran tal cual y se listan en orden español
  (RF-6).

## Fuera de alcance

- **El esquema de la base y cualquier migración:** no se tocan (spec 004).
- **El modelo del solver, sus invariantes y su validación:** son de la spec 005; aquí solo se exige que
  el conjunto los respete.
- **La generación de horarios:** el conjunto no genera ni guarda horarios.
- **Los presets del solver:** el conjunto **no** genera configuración de cálculo. El **plan de cada
  curso**, el **número de estudiantes** y los **filtros del docente** (turno, plan, años, horas de aula
  y suplencias con su `peso`) viven en el preset (spec 005). El conjunto solo garantiza que **los datos
  persistidos permiten construir presets coherentes** y ejercitar las cuatro variantes.
- **La comprobación funcional con el solver** (validación previa V13 y generación de un horario) usando
  un **preset de prueba aportado por las pruebas**: el conjunto no lo genera y no forma parte de su
  entregable. El conjunto solo garantiza la cobertura y la coherencia **a nivel de datos**.
- **La importación de datos reales** (hojas de cálculo, exportaciones de otro sistema) y la captura
  asistida de datos de un centro concreto.
- **La interfaz de usuario:** no se añaden pantallas.
- **Personas y matrículas reales:** los datos son ficticios.
- **Rendimiento del solver:** el conjunto no es un escenario de estrés.

## Criterios de finalización

1. Aplicar el perfil por defecto sobre una base vacía deja el centro completo: 2 turnos con su receso, 16
   aulas, el catálogo de materias-año, 4 planes, 13 cursos y una variante de docentes con sus materias y
   disponibilidad (RF-1, RF-4, RF-5, RF-6).

   > **Superado por la 011 (2026-10-29).** El inventario de 8 franjas de 40 min y la rejilla
   > `8 × 40 + 10` se sustituyen por 8 franjas de D = 35: Mañana 07:00–11:50 y Tarde 12:40–17:30,
   > recesos 08:10–08:20 y 13:50–14:00 (`8 × 35 + 10 = 290`). La rejilla de clases termina en 11:40 y
   > 17:20; el `fin` añade el evento de 10 min (el mismo desfase de 10 min que ya se documentaba aquí).

2. El plan General conserva las **14 áreas y la progresión del 31059** con **9/9/10/11/12 materias por
   año** (51 materias-año) y **40 horas académicas en cada año** (1 hora = 1 bloque, sin conversión);
   los tres planes derivados añaden sus materias de área, varían horas, **no superan 40 horas
   académicas por año**, no dejan años vacíos y declaran un total por año que coincide con la suma de
   sus materias-año (RF-2, RF-3).
3. Los dos turnos tienen **8 franjas de 40 minutos**, ventanas de campana **07:00–12:30** y
   **12:40–18:10**, **receso de 10 minutos tras la franja 2** (08:20–08:30 y 14:00–14:10) y **rejilla
   aritmética** de 07:00 a **12:20** y de 12:40 a **18:00** (el `fin` guardado incluye el receso) (RF-4).

   > **Superado por la 011 (2026-10-29).** El inventario de 8 franjas de 40 min y la rejilla
   > `8 × 40 + 10` se sustituyen por 8 franjas de D = 35: Mañana 07:00–11:50 y Tarde 12:40–17:30,
   > recesos 08:10–08:20 y 13:50–14:00 (`8 × 35 + 10 = 290`). La rejilla de clases termina en 11:40 y
   > 17:20; el `fin` añade el evento de 10 min (el mismo desfase de 10 min que ya se documentaba aquí).

4. Los cursos son 13 (3+3+3+2+2 secciones), **sin turno**, cada uno con un **aula distinta**; el preset
   aporta el turno al generar (RF-4, RF-5).
5. Las cuatro variantes existen y son intercambiables, y su diferencia se puede comprobar **a nivel de
   datos**: la holgada y la de suplentes cubren **todas las materias-año en ambos turnos** (en la
   segunda, la capacidad de los titulares queda entre el 85 % y el 90 % de la demanda y hay docentes
   cualificados sin materias asignadas); en las dos inválidas, una materia-año identificada queda sin
   cobertura (sin docente asignado / sin solape de disponibilidad **en ninguno de los dos turnos**)
   (RF-6, RF-7, RF-8). La comprobación funcional con el solver (V13/generación) requiere un preset
   aportado por las pruebas y **no** forma parte del entregable (nota de RF-8).
6. La conmutación de variante borra solo datos de docentes, deja idénticos los demás datos y es atómica,
   confirmada y con respaldo (RF-9).
7. La aplicación es determinista, atómica y no destructiva por defecto: falla con datos, exige
   confirmación y respaldo para reemplazar y nunca deja la base a medio poblar (RF-10, RF-11).
8. El informe de recuentos y totales por año (en horas académicas = bloques) coincide con el perfil, y
   el perfil y los recuentos son reutilizables por las pruebas automatizadas (RF-12).
9. Existe documentación de uso en español, con los cuatro planes, las cuatro variantes y la advertencia
   de datos ficticios y de que no se generan presets (RF-13).
10. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-7).

## Dudas abiertas

Ninguna.
