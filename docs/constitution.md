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

- Los datos del usuario en SQLite (planes de estudio, docentes, aulas, asignaturas, cursos, turnos, recesos y configuración del solver) nunca se borran ni se sobrescriben sin confirmación explícita.
- Los presets y los horarios guardados se gestionan como archivos JSON, no como datos de la base de datos; el usuario es dueño de esos archivos.
- Toda operación destructiva o migración de esquema debe ser reversible o ir acompañada de respaldo.
- La información sensible no se incluye en logs, dumps ni specs; los fixtures de test usan datos ficticios.

## 6. Idioma del código y de los textos

- **Código, comentarios y identificadores nuevos:** español para comentarios, mensajes y nombres de tests; inglés para identificadores cuando siguen convención existente del módulo.
- **Docs, specs, planes, tareas y textos de usuario:** español en todo el proyecto.
- Consistencia por encima de preferencia personal: en un módulo existente se sigue el estilo que ya tenga.

## 7. El solver y su contrato

El solver es la única pieza que decide el horario. Esta sección fija su contrato con el resto del programa y su comportamiento autoritativo; los documentos de `docs/solver/` quedan como **referencia histórica** y no obligan. El catálogo normativo detallado (cada restricción con su formulación, el contrato JSON y las validaciones) vive en la spec 005.

- **Frontera.** El solver no abre la base de datos ni depende de la interfaz. Recibe una configuración ya validada y devuelve una asignación o un fallo. La base aporta las entidades de dominio; el preset, los parámetros de cálculo.
- **Comportamiento.** O encuentra una solución que respeta *todas* las restricciones duras, o declara el problema no factible con la causa. No completa ni corrige la entrada. Para la misma entrada el resultado es reproducible.

### 7.1. Qué garantiza (invariantes)

1. **Un docente por materia-curso.** Cada materia de cada curso la dicta exactamente un profesor; no se reparte.
2. **Horas de la materia-curso.** Cada materia-curso recibe exactamente sus horas semanales convertidas en bloques.
3. **Sin solapamientos.** Ni un curso, ni un profesor, ni un aula tienen dos clases a la vez.
4. **Turno, plan y año.** Un profesor no enseña fuera de su turno, fuera de su plan ni fuera de los años a los que está ligado.
5. **Aula.** Con el modo de aula fija **inactivo**, cada curso ocupa exactamente un aula en cada franja en que tiene clase (la decide el solver); con el modo **activo**, cada curso usa su aula fija.

### 7.2. Qué prioriza (función objetivo)

El solver minimiza una penalización entera y **nunca** viola una restricción dura para reducirla. Prioridades, de mayor a menor peso:

1. **No exceder las horas de aula de un profesor** (`PESO_HORAS`): cada profesor respeta el tope de su contrato; si la demanda lo obliga, el exceso se paga y se minimiza.
2. **Preferir titulares antes que suplentes.** Cada asignación a un suplente se penaliza con un coste entero positivo que **decrece con el `peso`** del par (docente, materia-suplente), de modo que entre suplentes se prefiere el de mayor peso. Un suplente solo cubre una materia-curso cuando ningún titular elegible puede; esta regla **emerge** de la optimización y no se impone a mano.

`PESO_HORAS` y los costes de suplente son constantes enteras del motor, con `PESO_HORAS > coste_máximo_de_suplente > 0`. El `peso` de cada par (docente, materia-suplente) es dato de la entrada.

### 7.3. Contrato de configuración

- La entrada es un JSON versionado; un preset es una configuración completa y reutilizable.
- **Cursos:** se identifican por `anio` (entero) y `seccion` (carácter); el nombre se deriva de ellos (`"1ro A"`) y lo calcula **quien muestra**, no el solver. De cada curso se configuran su `plan` (del que se derivan sus materias y horas) y su `num_estudiantes`; su `turno` y su `aula` provienen de los datos del curso. El **modo de aula fija es global** (booleano, por defecto verdadero): activo → cada curso usa su aula fija; inactivo → aulas móviles que decide el solver.
- **Planes:** la entrada **embebe** los planes de estudio con sus materias y horas, para que el solver no acceda a la base de datos.
- **Profesores:** además de materias (titular/suplente), turno y plan, tienen `anios` (años a los que pueden dictar; vacío = cualquiera).
- **Aulas:** se referencian por su **índice** en el pool de aulas, no por nombre; la interfaz y el CSV traducen índice→nombre.
- La base define la estructura de cursos con `anio`, `seccion`, `turno` y `aula` (opcional, referencia a un aula), y **sin `nombre` almacenado**; `(anio, seccion)` es único en todo el sistema. La relación persistida curso–materia **no existe**.
- La **construcción de la entrada** (leer los datos persistidos y el preset y producir el JSON) es responsabilidad de la **spec 005**; el solver **no** accede a la base.

### 7.4. Validación previa

Antes de construir el modelo se ejecutan las validaciones V1–V15; si alguna falla, el solver no se invoca. **V13** exige que cada materia-curso tenga al menos un candidato elegible (titular o suplente) que respete plan, turno, año y disponibilidad. **V14** rechaza un curso que necesite más bloques que franjas útiles tiene su turno. **V15** rechaza la entrada cuando el modo de aulas móviles está activo y el centro no dispone de aulas.

### 7.5. Fuera de alcance de este contrato

Turnos automáticos (spec 006), generación parcial (spec 007), capacidad de aula y cobertura de emergencia. El receso es metadata que el solver ignora; el horario del profesor se deriva por post-proceso.
