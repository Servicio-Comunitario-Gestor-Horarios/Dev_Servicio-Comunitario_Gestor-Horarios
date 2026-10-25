# Contrato 2.0 del solver — referencia viva

> **Qué es este documento.** La referencia **vigente** del JSON de entrada del solver (el «preset») y
> de lo que el solver devuelve. Sirve para dos cosas: **escribir o revisar un preset a mano** y
> **integrar la generación desde la interfaz**. No es la guía de servicios: la API que el frontend
> consume (`ServicioGeneracion`, `CargadorConfiguracionSolver`, `ServicioTurnosEventos`, …) está en
> `docs/interfaz-frontend.md`; su deuda pendiente, en la **§5** de ese mismo documento.
>
> Los documentos de `docs/solver/` son **referencia histórica** y no obligan
> (`docs/constitution.md` §7); el texto normativo vive en
> [`specs/005-nucleo-solver/spec.md`](../specs/005-nucleo-solver/spec.md) y en
> [`specs/010-solver-tipos-y-eventos/spec.md`](../specs/010-solver-tipos-y-eventos/spec.md). Este
> documento describe el contrato **2.0**, que es el único que el código acepta hoy.

Versión documentada: **2.0**. Responsable del comportamiento: las specs 005 (núcleo) y 010 (tipos de
aula y eventos). Modelo de datos persistido: [`Diagrama-MER.md`](Diagrama-MER.md).

---

## 1. La versión es 2.0 y solo 2.0

El campo `version` es obligatorio y su valor debe ser exactamente `"2.0"`. **La versión se comprueba
antes que las secciones**: un 1.0 se rechaza por versión, no por «le falta `eventos`».

| Qué declara el archivo | Resultado | Mensaje (resumen) |
|---|---|---|
| `"version": "2.0"` | se acepta y se valida el resto (V1–V17) | — |
| `"version": "1.0"` | **entrada inválida** | «V1: versión de configuración no reconocida ('1.0'); se espera '2.0'.» |
| Versión ausente (o vacía) | **entrada inválida** | «V1: la configuración no declara versión; se espera '2.0'.» |
| Cualquier otra (`"9.9"`, `"2"`, …) | **entrada inválida** | el mismo mensaje, nombrando la versión encontrada |

Reglas asociadas:

- **No hay conversión.** Un preset 1.0 no se reinterpreta, no se completa con tipos por defecto y no
  se reescribe: se rechaza al usarlo y el archivo queda intacto en disco.
- **Al cargar un preset desde disco**, el rechazo añade el **archivo** y la **acción disponible**
  (RF-2 de la 010):
  > La configuración de «/ruta/al/preset.json» declara la versión «1.0»; solo se acepta la versión
  > 2.0. Genere una configuración 2.0 desde los datos con la versión actual.

  Si no declara versión, el texto es «… no declara versión; solo se acepta la versión 2.0. Genere una
  configuración 2.0 desde los datos con la versión actual.»
- **Al guardar o construir** una configuración se sella siempre `"2.0"`; nunca se emite `1.0` ni el
  campo vacío.
- Los presets 1.0 que ya existan en `datos-prueba/` (p. ej.
  `datos-prueba/preset-generacion-31059.json`) **no se migran**: hay que **regenerarlos** con la
  versión actual desde los datos del centro.

---

## 2. Forma del contrato

Diez secciones obligatorias y una opcional. El orden no importa; los **campos desconocidos se
toleran** (se ignoran), pero una sección obligatoria ausente es «V1: falta la seccion '‹clave›'».

| Sección | Tipo | Contenido | Validaciones que la miran |
|---|---|---|---|
| `version` | texto | `"2.0"` | **V1** |
| `aula_fija` | booleano, opcional | modo de aula fija (por defecto `true`) | **V8**, **V15** y el modelo |
| `dimensiones` | objeto | contadores del centro | **V3**, V12, V14 |
| `franja_horaria` | objeto | duración de la franja | **V2**, V14 |
| `turnos` | objeto con `turnos[]` | turnos con sus franjas y ventana | **V2**, V7, V13, V14 |
| `eventos` | arreglo | eventos del turno (metadata) | **V1**, **V2** |
| `planes` | arreglo | planes embebidos con materias y horas | **V4**, V10, V11, V13, V14 |
| `cursos` | arreglo | año, sección, turno, plan, aula de referencia | **V6**, V7, V8, V9, V10, V11, V14, **V17** |
| `profesores` | arreglo | materias, turno/plan/años y disponibilidad | **V4**, **V12**, V13, V2 |
| `materias` | arreglo | nombre y **tipo de aula requerido** | **V4**, **V16**, **V17** |
| `aulas` | arreglo | nombre, **tipo** y capacidad | **V6**, **V16**, **V17** |

### 2.1 `dimensiones`

```json
"dimensiones": {
  "num_profesores": 2, "num_materias": 3, "num_aulas": 3,
  "num_cursos": 2, "num_dias": 5, "num_slots_dia": 8
}
```

Los cuatro primeros deben **coincidir con el tamaño** de las listas correspondientes (**V3**);
`num_dias` y `num_slots_dia` acotan la disponibilidad (**V12**) y la rejilla temporal.

### 2.2 `franja_horaria`

```json
"franja_horaria": { "duracion_minutos": 40, "slots_por_turno": 8 }
```

`duracion_minutos` debe ser **> 0** (**V2**). Con la hora académica actual, un bloque es una hora
académica: `duracion_minutos` describe la **duración real** de la franja (rejilla, evento y
disponibilidad) y **no** convierte horas a bloques. `slots_por_turno` no interviene en la validación
ni en el modelo: la rejilla efectiva de cada turno es su lista `slot`.

### 2.3 `turnos`

```json
"turnos": {
  "turnos": [
    { "nombre": "Mañana", "slot": [0,1,2,3,4,5,6,7], "inicio": "07:00", "fin": "12:30" },
    { "nombre": "Tarde",  "slot": [8,9,10,11,12,13,14,15], "inicio": "12:40", "fin": "18:10" }
  ]
}
```

- `nombre` es la clave de referencia: los **cursos** (`curso.turno`), los **profesores**
  (`profesor.turno`, vacío = cualquiera) y los **eventos** (`evento.turno`) lo nombran así. Un turno
  inexistente es **V7** (curso) o **V2** (evento).
- `slot` son los **slots absolutos del día** de ese turno; su tamaño es el número de franjas del turno
  y acota la posición de un evento (**V2**) y las franjas útiles de **V14**.
- `inicio`/`fin` (`HH:mm`) son la **ventana real del turno**. Solo hacen falta para validar eventos
  con horario (**V2**): sin ventana válida, cualquier evento con `inicio`/`fin` se rechaza.

### 2.4 `planes`

```json
"planes": [
  { "codigo": "Ciencias",
    "materias": [
      { "materia_idx": 0, "curso": 1, "horas_semanales": 2 },
      { "materia_idx": 1, "curso": 1, "horas_semanales": 2 }
    ] }
]
```

El plan **se embebe** para que el solver no abra la base de datos. `materia_idx` indexa `materias`
(**V4**); `curso` es el **año** al que aplica la carga; `horas_semanales` son horas académicas
(= bloques, **V14**). Un curso debe referenciar **exactamente un** plan existente (**V11**) y tener al
menos una materia en él (**V10**).

### 2.5 `cursos`

```json
"cursos": [
  { "anio": 1, "seccion": "A", "turno": "Mañana", "plan": "Ciencias",
    "num_estudiantes": 20, "aula": 0 }
]
```

- La identidad es `(anio, seccion)`; **no hay `nombre`** en el contrato: lo deriva quien muestra.
- `turno` (**V7**) y `plan` (**V10**, **V11**) son obligatorios para generar.
- `aula` es un **índice** al arreglo `aulas` (**V6**). Con `aula_fija: true` es obligatorio y es el
  **aula de referencia** del curso (**V8**); con `aula_fija: false` se ignora.
- `num_estudiantes` es opcional (ausente = sin comprobación, **V9**); con `> 0` se compara con la
  capacidad del **aula real** de cada clase (aviso P3).

### 2.6 `profesores`

```json
"profesores": [
  { "nombre": "Ana", "horas_requeridas": 30, "horas_aula": 26,
    "turno": "", "plan": "", "anios": [1],
    "materias_asignadas": [0, 1],
    "materias_suplente": [ { "materia_idx": 2, "peso": 1 } ],
    "disponibilidad": [ { "dia": 0, "slot": [0,1,2,3,4,5,6,7] } ] }
]
```

- `materias_asignadas` (titular) y `materias_suplente` (con `peso`, índice y peso ≥ 0) son los
  índices de `materias` (**V4**); al menos una de las dos listas debe existir (**V10**).
- `turno` y `plan` vacíos significan **cualquiera**; `anios` vacío, **cualquier año**.
- `disponibilidad` es por día y lista de slots, dentro de `num_dias`/`num_slots_dia` (**V12**); **debe
  solapar con las franjas del turno del curso** para que el docente sea elegible (**V13**).
- `horas_requeridas` es opcional; si existe, `horas_aula` no puede superarla.

### 2.7 `materias` y `aulas`: los tipos

```json
"materias": [
  { "nombre": "Matemática", "tipo": "Aula de estudio" },
  { "nombre": "Tecnología", "tipo": "Laboratorio" }
],
"aulas": [
  { "nombre": "A1", "tipo": "Aula de estudio", "capacidad": 30 },
  { "nombre": "L1", "tipo": "Laboratorio",     "capacidad": 25 }
]
```

- **`tipo` es obligatorio en toda aula y en toda materia** (**V16**). No se completa con un valor por
  defecto: una configuración sin tipos se rechaza nombrando la entidad («el aula «A1» no declara tipo
  de aula», «la materia «Tecnología» no declara tipo de aula requerido»). La **construcción de la
  entrada** desde los datos falla antes, con la misma exigencia: si un aula o una materia persistida
  no tiene tipo, no se emite JSON.
- Cada materia de cada curso exige que **exista al menos un aula de su tipo** en el centro
  (**V17**), **en los dos modos** de aula (fija y móvil). Si no existe, la entrada es inválida y el
  mensaje nombra el tipo, la materia y el curso.
- `capacidad` ≤ 0 significa «sin capacidad declarada» (no genera aviso P3).

---

## 3. Tipos de aula: nombre y clave de comparación

| Concepto | Dónde vive | Ejemplo |
|---|---|---|
| Nombre visible del tipo | `aulas[].tipo` y `materias[].tipo` (lo que se guarda y muestra) | `"Aula de estudio"`, `"Laboratorio"` |
| Clave de comparación | `claveTipo(nombre)` = `trim` + **plegado Unicode de caja** | `"  LABORATORIO "` → `"laboratorio"` |

Consecuencias prácticas al escribir un preset:

- La igualdad es **exacta sobre la clave normalizada**: mayúsculas/minúsculas y espacios en los
  extremos no importan (`"Laboratorio"` = `" LABORATORIO "`), pero **no** hay sinónimos,
  ni jerarquías, ni normalización de acentos ni de espacios internos (`"Fisica"` ≠ `"Física"`,
  `"Aula  comun"` ≠ `"Aula común"`).
- El **nombre visible se conserva tal cual** se declaró; el catálogo de tipos (`TiposAula`) es la
  fuente de verdad del nombre y de su clave única. Al componer el preset, copie **el nombre del
  catálogo** en `aulas[].tipo` y `materias[].tipo` (`AulaDTO.tipoAula` / `MateriaDTO.tipoAula`).
- Un tipo declarado que ninguna materia requiere **no se rechaza** (puede haber aulas de un tipo sin
  uso en ese centro).

---

## 4. Eventos del turno

Los eventos (recesos, actos, intervalos) son **metadata**: el solver **los ignora** y no los
replanifica. Sustituyen a la antigua sección `recesos` del contrato 1.0.

```json
"eventos": [
  { "nombre": "Receso", "turno": "Mañana", "despues_de_slot": 2,
    "orden": 1, "duracion": 30, "inicio": "08:30", "fin": "09:00" }
]
```

| Campo | Obligatorio | Regla (**V2**) |
|---|---|---|
| `nombre` | sí | no vacío |
| `turno` | sí | debe existir en `turnos` |
| `despues_de_slot` | sí | **posición** del evento: `1..nº de franjas del turno` |
| `orden` | sí | `>= 1`; desempata eventos de la misma posición |
| `duracion` | sí | `> 0` minutos |
| `inicio` / `fin` | no | horario opcional (`HH:mm`): o **los dos** o **ninguno**; `fin > inicio`; `fin − inicio` = `duracion` |

Además, **por turno**: el intervalo debe caber en la **ventana de su posición** (calculada con
`duracion_minutos` y los eventos de esa posición) y dentro de la ventana `inicio`/`fin` del turno; los
eventos no pueden **solaparse** ni ir en **orden cronológico invertido**. Si un evento trae horario,
el turno **debe** declarar una ventana válida.

- La **identidad** del evento (su `id` en la base) **no viaja** en el contrato: dentro del preset un
  evento se distingue por `nombre`, `turno`, `despues_de_slot` y `orden`.
- La sección es **obligatoria** (puede ir vacía: `"eventos": []`) y se llama `eventos`. `recesos` ya
  no es una sección requerida; en un 2.0 se ignora como campo desconocido, y no se traduce a nada.
- El horario del profesor y los huecos reales se derivan **después**, en post-proceso.

---

## 5. Semántica del aula de referencia (modo `aula_fija: true`)

**El modo de aula fija ya no significa «el curso está siempre en su aula».** El aula del curso pasa a
ser su **aula de referencia**, y rige siempre la **regla de tipo**: toda clase se imparte en un aula
cuyo tipo coincide con el que requiere su materia.

Con `aula_fija: true`:

1. **Referencia.** Cada curso declara `aula` (índice a `aulas`); esa es su aula de referencia.
2. **Coincidencia de tipo.** Si la materia de una clase requiere el tipo del aula de referencia, la
   clase se imparte **en el aula de referencia** (no en otra aula del mismo tipo).
3. **Desplazamiento.** Si la materia requiere **otro** tipo, esa clase se imparte en un **aula del
   tipo requerido** (elegida por el solver) y el curso **vuelve** a su referencia en las demás
   franjas. Un curso puede tener clases en varias aulas del centro a lo largo del día.
4. **Referencia vacía y bloqueada.** Mientras el curso tiene una clase **desplazada** en una franja,
   su aula de referencia queda **vacía y bloqueada** en esa franja: ni el propio curso ni **ningún
   otro** puede usarla. La reserva **no** se extiende a franjas en las que el curso no tiene clase, ni
   a todo el turno.
5. **Exclusividad.** Dos cursos no comparten aula en la misma franja, **incluidas las clases
   desplazadas** (D8 se aplica a todas las aulas reales).
6. **Degradación prevista.** Si el tipo requerido no tiene aulas suficientes, la entrada sigue siendo
   válida (**V17** solo exige que exista al menos una) y el resultado puede ser **no factible con
   causa «conflicto de aula»**. El solver **no** reasigna a un aula de otro tipo ni **libera** la
   referencia bloqueada para hacer factible el problema.

Con `aula_fija: false` (aulas móviles):

- El solver elige en cada franja un aula del **tipo exacto** requerido por la materia de la clase; el
  `aula` del curso se ignora (**V8** no aplica).
- El centro debe tener aulas (**V15**), y cada tipo requerido al menos una (**V17**).

Errores de lectura frecuentes:

| Creencia | Realidad 2.0 |
|---|---|
| «Curso fijo = siempre en su aula» | Solo las clases del **tipo de la referencia**; el resto se desplazan y vuelven. |
| «Su aula se reserva todo el turno» | Solo en las franjas con una clase **desplazada**; el resto queda libre. |
| «Otro curso puede usar la referencia mientras está fuera» | **No**: en esa franja está vacía y **bloqueada**. |
| «Si el laboratorio no alcanza, el solver usa un aula común» | **No**: nunca cambia el tipo; el resultado es no factible (conflicto de aula). |

---

## 6. La salida informa el **aula real** de cada clase

Cada asignación del horario de salida trae el **índice del aula donde la clase se impartió de
verdad**, también en modo de aula fija (fragmento; la raíz del archivo lleva además `metadata`):

```json
"horarios": {
  "1ro A": {
    "turno": "Mañana",
    "dias": [
      { "dia": 0,
        "asignaciones": [
          { "slot": 0, "materia": 0, "profesor": 0, "aula": 2 },
          { "slot": 1, "materia": 1, "profesor": 0, "aula": 5 }
        ] }
    ]
  }
}
```

- Un curso fijo puede mostrar **aulas distintas** en el mismo día: la de referencia (`aula: 2`) en las
  clases de su tipo y la desplazada (`aula: 5`, un laboratorio) en las demás.
- `aula` es un **índice** al arreglo `aulas` del preset; traducirlo a nombre es responsabilidad de
  quien muestra (la interfaz y el CSV).
- Las validaciones posteriores (`ValidadorSalidaSolver`: P3 capacidad, P4 solapamientos) operan sobre
  ese **aula real**, no sobre el aula declarada del curso. La capacidad se compara con el aula donde
  se dio la clase; un aula con `capacidad <= 0` no genera aviso.

---

## 7. Ejemplo mínimo válido (2.0)

Un centro de un curso, un docente, dos materias de tipos distintos, un aula de referencia y un
laboratorio, y un receso. **Se acepta tal cual** (`SolverConfig::fromJson` devuelve `ok`).

```json
{
  "version": "2.0",
  "aula_fija": true,
  "dimensiones": {
    "num_profesores": 1,
    "num_materias": 2,
    "num_aulas": 2,
    "num_cursos": 1,
    "num_dias": 5,
    "num_slots_dia": 6
  },
  "franja_horaria": { "duracion_minutos": 45, "slots_por_turno": 6 },
  "turnos": {
    "turnos": [
      { "nombre": "manana", "slot": [0, 1, 2, 3, 4, 5], "inicio": "07:00", "fin": "12:00" }
    ]
  },
  "eventos": [
    { "nombre": "Receso", "turno": "manana", "despues_de_slot": 2,
      "orden": 1, "duracion": 30, "inicio": "08:30", "fin": "09:00" }
  ],
  "planes": [
    { "codigo": "Ciencias",
      "materias": [
        { "materia_idx": 0, "curso": 1, "horas_semanales": 2 },
        { "materia_idx": 1, "curso": 1, "horas_semanales": 2 }
      ] }
  ],
  "cursos": [
    { "anio": 1, "seccion": "A", "turno": "manana", "plan": "Ciencias",
      "num_estudiantes": 20, "aula": 0 }
  ],
  "profesores": [
    { "nombre": "Ana", "horas_aula": 4, "turno": "", "plan": "", "anios": [1],
      "materias_asignadas": [0, 1], "materias_suplente": [],
      "disponibilidad": [ { "dia": 0, "slot": [0, 1, 2, 3, 4, 5] } ] }
  ],
  "materias": [
    { "nombre": "Matemática", "tipo": "Aula de estudio" },
    { "nombre": "Tecnología", "tipo": "Laboratorio" }
  ],
  "aulas": [
    { "nombre": "A1", "tipo": "Aula de estudio", "capacidad": 30 },
    { "nombre": "L1", "tipo": "Laboratorio", "capacidad": 25 }
  ]
}
```

Cómo se lee:

- El curso `1ro A` declara `aula: 0` → su **aula de referencia** es `A1` («Aula de estudio»).
- `Matemática` exige «Aula de estudio»: se imparte en `A1`. `Tecnología` exige «Laboratorio»: se
  imparte en `L1`, y mientras tanto `A1` queda **vacía y bloqueada** en esa franja.
- El receso va después de la franja 2 y cabe en su ventana (08:30–09:00): es metadata, no ocupa
  franja. El turno declara `inicio`/`fin` porque el evento trae horario.
- V14: 2 + 2 = 4 bloques frente a 5 días × 6 franjas.

---

## 8. Contraejemplo: un 1.0 se rechaza

Un preset 1.0 típico (sin `eventos`, sin tipos, con `recesos` y `version: "1.0"`):

```json
{
  "version": "1.0",
  "aula_fija": true,
  "recesos": [ { "despues_de_slot": 2, "duracion": 30 } ],
  "materias": [ { "nombre": "Matemática" } ],
  "aulas": [ { "nombre": "A1", "capacidad": 30 } ]
}
```

Rechazo al cargarlo desde disco:

> La configuración de «/ruta/preset.json» **declara la versión «1.0»**; **solo se acepta la versión
> 2.0**. **Genere una configuración 2.0** desde los datos con la versión actual.

No se convierte, no se completan tipos y el archivo no se toca. Para usarlo: **regenerarlo** desde
los datos del centro (que ya persisten `TiposAula`, `id_tipo_aula` y los eventos con identidad) con
la versión actual.

Un 1.0 **completo** (todas las secciones, con `recesos` y sin tipos) es el fixture versionado
[`test/data/preset-1.0.json`](../test/data/preset-1.0.json), que la suite usa justo para comprobar
este rechazo y que sus bytes no cambian.

---

## 9. Referencias

- Núcleo del solver, restricciones y validaciones V1–V15:
  [`specs/005-nucleo-solver/spec.md`](../specs/005-nucleo-solver/spec.md).
- Tipos de aula, eventos, contrato 2.0, V16/V17 y salida con aula real:
  [`specs/010-solver-tipos-y-eventos/spec.md`](../specs/010-solver-tipos-y-eventos/spec.md) y su
  [`plan.md`](../specs/010-solver-tipos-y-eventos/plan.md) (§2).
- Persistencia de tipos y eventos (catálogo, `id_tipo_aula`, identidad y orden):
  [`specs/009-tipos-de-aula-y-eventos/spec.md`](../specs/009-tipos-de-aula-y-eventos/spec.md).
- Servicios que consume el frontend y **deuda del frontend** (migración a `Resultado`, tipos en los
  formularios, `tipoAula` en lugar de heurísticas por nombre):
  [`docs/interfaz-frontend.md`](interfaz-frontend.md) — §3.2 (presets) y **§5 (deuda)**.
- Modelo de datos persistido: [`docs/Diagrama-MER.md`](Diagrama-MER.md).
- Invariantes y contrato normativo resumido: [`docs/constitution.md`](constitution.md) §7.
