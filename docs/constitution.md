# Constitución del proyecto

> Principios rectores del desarrollo con Spec-Driven Development en Gestor-Horarios.
> Revisar con `/sdd-constitution`. Cambios aquí mandan sobre specs, planes y código.

## 1. Simplicidad del stack

- Se usa el stack ya establecido del proyecto: C++17, Qt6, OR-Tools, SQLite, CMake/Ninja, CTest.
- Cualquier nueva dependencia o tecnología necesita justificación explícita en el plan y aprobación del usuario.
- Se prefiere la solución más simple que cumpla la spec; no se anticipa complejidad que la spec no pida.

## 2. Relación entre spec y código

- La spec manda: si algo no está en la spec, no se implementa.
- La spec describe el QUÉ y el POR QUÉ; el plan describe el CÓMO. Nada de stack ni nombres de archivos en `spec.md`.
- Todo cambio de requisitos entra primero en la spec, luego en `plan.md`/`tasks.md` y al final en el código.
- Cada función o módulo nuevo debe poder trazarse a un RF de alguna spec.

## 3. Separación entre lógica e interfaz

- La lógica de negocio y el solver viven en `src/backend` (y `src/common`); la interfaz Qt vive en `src/frontend`; `src/middleware` solo transporta y valida.
- El backend no depende de Qt Widgets; la lógica es testeable sin levantar la UI.
- Reglas de negocio nuevas: primero en backend con tests, después se reflejan en la interfaz.

## 4. Política de tests

- Tests primero: en cada tarea, los tests se escriben en rojo antes que el código, y la tarea solo se marca al pasar en verde.
- Comando de tests del proyecto: `cmake --preset full && cmake --build build && ctest --preset full`.
- Toda RF debe tener al menos una comprobación verificable en `tasks.md` ("Hecho cuando: …"). Sin cobertura, la tarea no está hecha.
- Se usan los frameworks ya integrados: Google Test (lógica/backend) y Qt Test (middleware/frontend).

## 5. Protección de los datos del usuario

- Los datos del usuario en SQLite (planes de estudio, docentes, aulas, tipos de aula, asignaturas, cursos, turnos, eventos y configuración del solver) nunca se borran ni se sobrescriben sin confirmación explícita.
- Los presets y los horarios guardados se gestionan como archivos JSON, no como datos de la base de datos; el usuario es dueño de esos archivos.
- Toda operación destructiva o migración de esquema debe ser reversible o ir acompañada de respaldo.
- La información sensible no se incluye en logs, dumps ni specs; los fixtures de test usan datos ficticios.

## 6. Idioma del código y de los textos

- **Código, comentarios y identificadores nuevos:** español para comentarios, mensajes y nombres de tests; inglés para identificadores cuando siguen convención existente del módulo.
- **Docs, specs, planes, tareas y textos de usuario:** español en todo el proyecto.
- Consistencia por encima de preferencia personal: en un módulo existente se sigue el estilo que ya tenga.

## 7. El solver y su contrato

El solver es la única pieza que decide el horario. Esta sección fija su contrato con el resto del programa y su comportamiento autoritativo; los documentos de `docs/solver/` quedan como **referencia histórica** y no obligan. El catálogo normativo detallado (cada restricción con su formulación, el contrato JSON y las validaciones) vive en la spec 005.

> **Enmienda (2026-10-17).** La **hora académica es un bloque**, no una hora de reloj. Se sustituye la
> conversión `bloques(horas) = ceil(horas × 60 / duracion_minutos)` por la identidad
> `bloques(horas) = horas`; el tope del docente se mide como `bloques_aula(horas_aula) = horas_aula`; y
> `duracion_minutos` define **solo la duración real** de la franja (rejilla horaria, receso y
> disponibilidad). Afecta a §7.1.2 y a **V14** (§7.4); el detalle normativo queda en la spec 005.

- **Frontera.** El solver no abre la base de datos ni depende de la interfaz. Recibe una configuración ya validada y devuelve una asignación o un fallo. La base aporta las entidades de dominio; el preset, los parámetros de cálculo.
- **Comportamiento.** O encuentra una solución que respeta *todas* las restricciones duras, o declara el problema no factible **con la causa cuando una categoría la explica y sin atribuirla cuando ninguna aplica**. No completa ni corrige la entrada. Para la misma entrada el resultado es reproducible.

### 7.1. Qué garantiza (invariantes)

1. **Un docente por materia-curso.** Cada materia de cada curso la dicta exactamente un profesor; no se reparte.
2. **Horas de la materia-curso.** Cada materia-curso recibe exactamente sus **horas académicas semanales en bloques** (1 hora académica = 1 bloque).
3. **Sin solapamientos.** Ni un curso, ni un profesor, ni un aula tienen dos clases a la vez.
4. **Turno, plan y año.** Un profesor **no enseña fuera del turno, el plan ni los años que tenga definidos**; un criterio **ausente** se interpreta como **cualquiera**. (Se refiere a los filtros del **profesor**; el turno y el plan del **curso** son obligatorios para generar — ver spec 005.)
5. **Aula.** Toda clase se imparte en un aula cuyo **tipo** coincide con el tipo de aula requerido por su materia. Con el modo de aula fija **inactivo**, el solver elige un aula del tipo requerido en cada franja en que el curso tiene clase; con el modo **activo**, cada curso declara un **aula de referencia**: la usa cuando su tipo coincide, y las clases cuya materia requiere otro tipo se imparten en un aula de ese tipo y el curso **vuelve** a la referencia en las demás. Mientras un curso tiene una clase desplazada en una franja, su aula de referencia queda **vacía y bloqueada** en esa franja. La **exclusividad de aula** rige también las clases desplazadas. El **aula de referencia es siempre del tipo general/ordinario** del centro (el tipo de clave normalizada `aula de estudio`); las aulas de los demás tipos **nunca** quedan reservadas.

### 7.2. Qué prioriza (función objetivo)

El solver minimiza una penalización entera y **nunca** viola una restricción dura para reducirla. Prioridades, de mayor a menor peso:

1. **No exceder las horas de aula de un profesor** (`PESO_HORAS`): cada profesor respeta el tope de su contrato; si la demanda lo obliga, el exceso se paga y se minimiza.
2. **Preferir titulares antes que suplentes.** Cada asignación a un suplente se penaliza con un coste entero positivo **fijo** (`COSTE_SUPLENTE`). Como asignar a un suplente cuesta más que dar clase a un titular con **capacidad de horas libre**, un suplente solo cubre una materia-curso cuando los titulares **elegibles** ya **no tienen horas libres** (no son **preferidos**) o no tienen disponibilidad. La **elegibilidad** es factibilidad previa (plan, turno, año, disponibilidad); la **capacidad de horas** es preferencia del objetivo, **no** una compuerta. Esta regla **emerge** de la optimización y no se impone a mano.

`PESO_HORAS` y `COSTE_SUPLENTE` son constantes enteras del motor, con `PESO_HORAS > COSTE_SUPLENTE > 0`. El `peso` de cada par (docente, materia-suplente) es dato de la entrada (hoy **sin uso** en el objetivo; ver spec 005).

### 7.3. Contrato de configuración

- La entrada es un JSON versionado; un preset es una configuración completa y reutilizable.
- **Cursos:** se identifican por `anio` (entero) y `seccion` (carácter); el nombre se deriva de ellos (`"1ro A"`) y lo calcula **quien muestra**, no el solver. De cada curso se configuran su `plan` (del que se derivan sus materias y horas) y su `num_estudiantes`; su `turno` y su `aula` provienen de los datos del curso; esa `aula` **debe ser del tipo general/ordinario** (V18). El **modo de aula fija es global** (booleano, por defecto verdadero): activo → el aula del curso es su **aula de referencia** (§7.1.5: desplazamiento y vuelta); inactivo → el solver elige el aula de cada clase. El **turno y el plan del curso son obligatorios para generar** (si faltan, la entrada se rechaza); la **asignación automática de turno** se difiere a la spec 006. La configuración **no** lleva datos que distingan los tipos generales: la convención es el nombre del tipo (clave normalizada `aula de estudio`).
- **Planes:** la entrada **embebe** los planes de estudio con sus materias y horas, para que el solver no acceda a la base de datos.
- **Profesores:** además de materias (titular/suplente), turno y plan, tienen `anios` (años a los que pueden dictar; vacío = cualquiera).
- **Aulas:** se referencian por su **índice** en el pool de aulas, no por nombre; la interfaz y el CSV traducen índice→nombre. Cada aula declara su **tipo** y cada materia el **tipo de aula requerido**; la igualdad es exacta sobre la **clave normalizada** (sin espacios extremos y sin distinguir mayúsculas), aunque el nombre visible del tipo se conserva.
- **Eventos:** la entrada lleva los eventos de cada turno (nombre, posición, orden, duración y horario opcional), que el solver ignora; sustituyen a la sección de recesos. La **identidad** del evento no viaja en el contrato.
- La base define la estructura de cursos con `anio`, `seccion`, `turno` y `aula` (opcional, referencia a un aula), y **sin `nombre` almacenado**; `(anio, seccion)` es único en todo el sistema. La relación persistida curso–materia **no existe**.
- La **construcción de la entrada** (leer los datos persistidos y el preset y producir el JSON) es responsabilidad de la **spec 005**; el solver **no** accede a la base.
- La versión del contrato es **2.0**; una configuración `1.0` o sin versión se rechaza.

### 7.4. Validación previa

Antes de construir el modelo se ejecutan las validaciones V1–V18; si alguna falla, el solver no se invoca. **V13** exige que cada materia-curso tenga al menos un candidato **elegible** (titular o suplente) que respete plan, turno, año y disponibilidad (la capacidad de horas es preferencia del objetivo, no de V13). **V7** exige que cada curso declare un turno válido y **V10/V11** que declare un plan. **V14** rechaza un curso que necesite más horas académicas que franjas útiles tiene su turno (`Σ_m horas_m ≤ nº_días × |franjas del turno|`). **V15** rechaza la entrada cuando el modo de aulas móviles está activo y el centro no dispone de aulas. **V1** espera la sección de **eventos** (no la de recesos); el contenido de V1, V2, V16, V17 y V18 vive en la spec 005 y en la 010.

### 7.5. Fuera de alcance de este contrato

Turnos automáticos (spec 006), generación parcial (spec 007), cobertura de emergencia. La **capacidad de aula** y las **horas no cubiertas** podrían volver **como avisos posteriores** (ver spec 002). El **evento** de turno es metadata que el solver ignora; el horario del profesor se deriva por post-proceso.
