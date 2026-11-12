# Plan 009 - Persistencia y servicios de tipos de aula y eventos

Base: 471311c

> **Alcance.** Este plan implementa la spec 009 (12 RF) sobre la base `471311c` (que contiene el
> esquema de `059168f`), que ya trae el cambio de esquema (catálogo de tipos, referencias de tipo y
> tabla de eventos renombrada) pero **no** la identidad ni el orden del evento, la marca de versión,
> los servicios, el seed ni la documentación. La 009 deja un **puente temporal** que sigue emitiendo
> la sección `recesos` del contrato 1.0 derivada de los eventos; la 010 sustituye el contrato por el
> 2.0. El frontend queda fuera y **no compila** hasta que su equipo migre; esta entrega se verifica
> **sin frontend**.
>
> **Estado de partida conocido.** En la base `471311c` (esquema de `059168f`) la suite está roja: el
> esquema tiene `TiposAula` y `Eventos`, pero `version_esquema` sigue esperando `Recesos` y la versión
> 2, los servicios consultan `Recesos` y los INSERT de aulas/materias no rellenan `id_tipo_aula`
> (`NOT NULL`). Las tareas están ordenadas para recuperar el verde: T1 deja el build verificable, T2 el
> esquema/versión, T3/T4 los eventos, T5/T6 los tipos, T7/T8 el seed y T9/T10 el puente y la
> documentación.

## 1. Archivos y responsabilidades

### 1.1 Build sin interfaz

- `CMakeLists.txt`: opción `BUILD_FRONTEND` (ON por defecto).
- `src/CMakeLists.txt`: `add_subdirectory(frontend)` solo si `BUILD_FRONTEND`; `app` se mantiene
  siempre porque `app_core` (instancia única) no depende de la UI.
- `src/app/CMakeLists.txt`: el ejecutable `gestor-horarios` (y sus fuentes de UI) dentro de
  `if(BUILD_FRONTEND)`; `app_core` siempre.
- `CMakePresets.json`: preset `backend` (configure/build/test) con `binaryDir=build-backend`,
  `BUILD_BACKEND=ON`, `BUILD_TESTING=ON`, `BUILD_FRONTEND=OFF`.
- `.gitignore`: `build-backend/`.
- `test/CMakeLists.txt`: el bloque 117-133 (frontend) **no cambia**; se añaden los `add_qtest` de
  `test_calculo_eventos`, `test_servicio_turnos_eventos` y `test_servicio_tipos_aula` en las secciones
  de backend ya existentes.

### 1.2 Esquema y versión

- `src/backend/src/database/migracion.cpp`: DDL del catálogo, aulas, materias y eventos con identidad
  y orden; `TiposAula` con clave normalizada almacenada + índice único; eventos con `id`, `orden`,
  unicidad y par de horas.
- `src/backend/include/backend/database/version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 3`.
- `src/backend/src/database/version_esquema.cpp`: `TABLAS_ESPERADAS`, `TABLAS_RETIRADAS`,
  `COLUMNAS_AULAS`, `COLUMNAS_MATERIAS`, `COLUMNAS_EVENTOS`, `COLUMNAS_TIPOS` y el mensaje de motivo.
- `test/backend/test_version_esquema.cpp`: versión 3, tablas/columnas, huella nueva y casos de
  rechazo.
- `test/backend/test_apertura_base_datos.cpp` y `test/datos/test_contexto_base_datos.cpp`: usan
  `VERSION_ESQUEMA_ACTUAL`; revisar solo si cambian los literales.

### 1.3 Eventos (con etapa de compatibilidad)

- `src/backend/include/backend/services/ServicioTurnosRecesos.hpp` y
  `src/backend/src/services/ServicioTurnosRecesos.cpp`: **nombre temporal hasta T9**. T3 añade
  `EventoDTO`, `TurnoDTO.eventos` y el CRUD de eventos; mantiene `RecesoDTO`/`TurnoDTO.recesos` y
  `agregarReceso`/`eliminarReceso` como **compatibilidad** (derivan del evento; no consultan la tabla
  retirada) para que `ConstructorEntradaSolver`, `ServicioGeneracion`, el seed, la consola y los tests
  sigan compilando. T9 renombra a `ServicioTurnosEventos` y retira la compatibilidad.
- `src/backend/include/backend/services/calculo_eventos.hpp` y
  `src/backend/src/services/calculo_eventos.cpp`: funciones puras de orden, ventana y solape.
- `src/backend/include/backend/services/NucleoDatos.hpp` y
  `src/backend/src/services/NucleoDatos.cpp`: métodos de eventos y tipos; claves de pendiente.
- `src/backend/src/services/cascada.cpp`: el dominio `turno` apunta a `Eventos` (T3 conserva el
  nombre lógico `receso`; T9 lo cambia a `evento`).
- `src/backend/src/services/ServicioGeneracion.cpp`: `huellaDatos` incluye el tipo de aulas y
  materias y los eventos de cada turno (T5, junto con el tipo en los DTO).
- `src/backend/CMakeLists.txt`: fuentes nuevas/renombradas (el renombrado final en T9).
- `test/backend/test_servicio_turnos_eventos.cpp`,
  `test/backend/test_calculo_eventos.cpp`, `test/backend/test_nucleo_datos.cpp` (existente),
  `test/backend/test_cascada.cpp` (existente), `test/backend/test_constructor_entrada.cpp` (existente),
  `test/backend/test_servicio_generacion.cpp` (huella).

### 1.4 Tipos de aula

- `src/backend/include/backend/services/ServicioTiposAula.hpp` y
  `src/backend/src/services/ServicioTiposAula.cpp`: DTO, CRUD, clave normalizada almacenada + índice
  único, borrado en uso.
- `src/backend/include/backend/services/ServicioAula.hpp` / `src/backend/src/services/ServicioAula.cpp`:
  `AulaDTO.idTipoAula` y `tipoAula`, firmas con tipo, SELECT con JOIN.
- `src/backend/include/backend/services/ServicioMaterias.hpp` /
  `src/backend/src/services/ServicioMaterias.cpp`: `MateriaDTO.idTipoAula` y `tipoAula`, firmas con
  tipo, SELECT con JOIN.
- `test/backend/test_servicio_tipos_aula.cpp` (nuevo), `test/backend/test_servicio_aula.cpp`,
  `test_servicio_materias.cpp`, `test_servicio_planes_estudio.cpp`, `test_composicion_plan.cpp`,
  `test_servicio_profesor.cpp`, `test_servicio_carga_horaria.cpp`, `test_gestor_pendientes.cpp`,
  `test/datos/test_contexto_base_datos.cpp` (llamadas a las firmas nuevas).

### 1.5 Seed

- `src/seed/include/seed/perfil.hpp`: **T7** añade `EventoDef`, `TurnoDef.eventos`, `AulaDef.tipo`,
  `Recuentos.eventos`/`tiposAula` y conserva `receso*`/`Recuentos.recesos` como compatibilidad;
  **T8** retira esos campos antiguos.
- `src/seed/src/perfil.cpp`: T7 rellena turnos con eventos, tipos de aulas, la regla de tipo de
  materia y la validación; T8 retira los campos antiguos de receso.
- `src/seed/src/sembrador.cpp`: T7 mantiene la inserción antigua de recesos; T8 inserta tipos,
  aulas/materias con tipo y eventos, actualiza recuentos, listas de tablas y orden de
  vaciado/reemplazo.
- `src/seed/src/docentes.cpp`: **T8** cambia `r.recesos` por `r.eventos` y añade `r.tiposAula`; sin
  este cambio no compila al retirar el campo en T8.
- `test/backend/test_seed_perfil.cpp` (T7) y `test/backend/test_seed_aplicacion.cpp` (T8).

### 1.6 Puente, consola y documentación

- `src/backend/src/services/ConstructorEntradaSolver.cpp` (y su cabecera): puente
  `eventos → RecesoConfig`; `DatosDominio.turnos` con eventos.
- `test/backend/test_constructor_entrada.cpp`.
- `src/consola/src/ConsolaDatos.cpp`: en T3 los comandos de turnos/eventos (`evento-*`) y la
  compatibilidad de lectura; en T6 los de aulas/materias con tipo y los de `tipos`; ayuda y listados.
- `docs/interfaz-frontend.md`: DTOs, firmas, cascada, pendientes, fórmula de duración y deuda del
  frontend.
- `docs/guias/consola-datos.md` (líneas 103-104: `turnos receso-agregar`/`receso-eliminar`).
- `docs/guias/datos-prueba.md` (líneas 17 y 83: fila «Recesos» y salida con «Recesos: 2»).

## 2. Interfaz (DTOs y servicios)

```cpp
struct EventoDTO {
    int     id = -1;             // identidad estable; -1 al crear
    QString nombre;              // no vacío
    int     despuesDeSlot = 0;   // 1..numSlots; 0 es inválido, NO «sin asignar»
    int     orden = 0;           // >=1; 0 al crear = automático
    int     duracion = 0;        // minutos, >0
    QTime   inicio;              // inválida = ausente
    QTime   fin;                 // inválida = ausente
};

struct TurnoDTO {
    QString nombre; QTime inicio; QTime fin; int numSlots = 0;
    QVector<EventoDTO> eventos;  // ordenados por (posición, orden, id)
};

struct TipoAulaDTO { int id = -1; QString nombre; };

// ServicioTurnosEventos
Resultado<TurnoDTO> crearTurno(...);                 // sin cambios de firma
Resultado<TurnoDTO> actualizarTurno(..., bool eliminarEventosFueraDeRango = false);
Resultado<TurnoDTO> obtenerTurno(const QString&) const;
Resultado<QVector<TurnoDTO>> listarTurnos() const;
bool eliminarTurno(const QString&);
Resultado<EventoDTO> agregarEvento(const QString& turno, const QString& nombre,
                                   int despuesDeSlot, int duracion,
                                   const QTime& inicio = QTime(), const QTime& fin = QTime(),
                                   int orden = 0);
Resultado<EventoDTO> actualizarEvento(int id, const QString& nombre, int despuesDeSlot,
                                      int duracion, const QTime& inicio, const QTime& fin,
                                      int orden);      // orden obligatorio; turno inmutable
bool eliminarEvento(int id);
Resultado<QVector<EventoDTO>> eventosFueraDeRango(const QString& turno, int numSlots) const;

// ServicioTiposAula
Resultado<TipoAulaDTO> crearTipoAula(const QString& nombre);
Resultado<TipoAulaDTO> obtenerTipoAula(int id) const;
Resultado<QVector<TipoAulaDTO>> listarTiposAula() const;
Resultado<TipoAulaDTO> renombrarTipoAula(int id, const QString& nombre);
Resultado<bool> eliminarTipoAula(int id);           // error si está en uso
```

`AulaDTO` y `MateriaDTO` añaden `int idTipoAula = -1; QString tipoAula;`. El tipo se **añade al
final** de las firmas con centinela `-1` **solo durante la transición** (T5-T8); el servicio
**rechaza** el centinela en tiempo de ejecución y **T9 retira los valores por defecto**, de modo que
la firma final exige el tipo y el compilador no permite omitirlo:

```cpp
// T5-T8 (transitorio): los consumidores aún no actualizados siguen compilando.
crearAula(nombre, capacidad, edificio = "", piso = "", idTipoAula = -1);
actualizarAula(id, nombre, capacidad, edificio = "", piso = "", idTipoAula = -1);
crearMateria(nombre, requisitos = "", idTipoAula = -1);
actualizarMateria(id, nombre, requisitos = "", idTipoAula = -1);

// T9 (final): sin valores por defecto silenciosos y sin reordenar (elección a).
crearAula(nombre, capacidad, edificio, piso, idTipoAula);
actualizarAula(id, nombre, capacidad, edificio, piso, idTipoAula);
crearMateria(nombre, requisitos, idTipoAula);
actualizarMateria(id, nombre, requisitos, idTipoAula);
```

Se elige la opción **(a)**: la firma final mantiene el **mismo orden** que la transitoria
(`idTipoAula` al final) y solo **quita los valores por defecto**; no hay reordenamiento de call sites
en T9. `NucleoDatos` replica el mismo orden y añade el CRUD de tipos.

**Turno del evento.** El turno es **inmutable** en la edición: `actualizarEvento` carga el evento,
valida que su turno siga existiendo (RF-1) y no acepta cambiarlo; para mover un evento de turno se
borra y se crea. `despuesDeSlot = 0` es siempre inválido; el 0 del DTO es un valor de construcción, no
un «sin asignar».

## 3. Funciones puras (sin E/S, deterministas)

- `calculo_eventos`:
  - `int duracionFranjaEstimada(const TurnoDTO&)`: `(minutosVentana − Σ duraciones) / numSlots`;
    **guarda**: si `numSlots <= 0`, la ventana es <= 0 o el reparto no es positivo, devuelve 0 y el
    servicio rechaza el encaje.
  - `QPair<QTime,QTime> ventanaDePosicion(const TurnoDTO&, int posicion)`: inicio =
    `turno.inicio + (posicion × franja + Σ duraciones anteriores)`, fin = inicio + Σ duraciones de la
    posición.
  - `bool eventoDentroDeVentana(const EventoDTO&, const TurnoDTO&, int posicion)`.
  - `bool haySolapeCronologico(const QVector<EventoDTO>&)`: ordena por `(posición, orden)` y exige
    `anterior.fin <= siguiente.inicio` cuando ambas horas existen.
  - `int ordenSiguiente(const QVector<EventoDTO>&, int posicion)`: máximo + 1, o 1 si no hay.
  - `QVector<EventoDTO> eventosFueraDeRango(const TurnoDTO&, int numSlots)`.
- `ServicioTiposAula`: `QString claveTipo(const QString&)` = `trimmed().toCaseFolded()` (solo para
  comparar; el nombre visible se conserva).
- `seed`: `QString tipoEspecializadoDeMateria(const QString& nombre)` con **prefijo/área**, no nombre
  exacto: `Física*`, `Química*`, `Biología*`, `Ciencias Naturales*`, `Ciencias de la Tierra*` →
  Laboratorio; `Informática*`/`Taller*` → Taller; `Educación Física*` → Cancha; resto → genérico. Así
  «Física Aplicada», «Química Aplicada» y «Ciencias de la Tierra Aplicadas» caen en laboratorio.
  `QString tipoFinalDeMateria(const QString&, const QSet<QString>& tiposDeAulas)` (si el especializado
  no tiene aula, genérico).
- `ConstructorEntradaSolver`: `QVector<RecesoConfig> recesosDesdeEventos(const TurnoDTO&)`; una
  entrada por evento, sin nombre ni orden (contrato 1.0). El constructor ya recibe `ahora`.

Ninguna función nueva lee el reloj; los instantes siguen inyectándose donde ya se hacía
(`construirJsonEntrada(..., ahora)`, `rutaRespaldoLibre(..., ahora)`).

## 4. Pseudocódigo de lo no trivial

### 4.1 Esquema y versión

```
VERSION_ESQUEMA_ACTUAL = 3

TiposAula(id PK AUTOINCREMENT, nombre TEXT NOT NULL CHECK(trim(nombre)=nombre AND length(nombre)>0),
          clave TEXT NOT NULL CHECK(length(clave)>0), timestamps)
CREATE UNIQUE INDEX TiposAula_clave ON TiposAula(clave)
Aulas(..., id_tipo_aula INTEGER NOT NULL REFERENCES TiposAula(id), ...)
Materias(..., id_tipo_aula INTEGER NOT NULL REFERENCES TiposAula(id), ...)
Eventos(id PK AUTOINCREMENT, nombre TEXT NOT NULL CHECK(length(trim(nombre))>0),
        turno TEXT NOT NULL REFERENCES Turnos(nombre),
        despues_de_slot INTEGER NOT NULL CHECK(despues_de_slot >= 1),
        orden INTEGER NOT NULL CHECK(orden >= 1),
        duracion INTEGER NOT NULL CHECK(duracion > 0),
        inicio TEXT, fin TEXT,
        CHECK((inicio IS NULL) = (fin IS NULL)),
        UNIQUE(turno, despues_de_slot, orden), timestamps)
```

`TABLAS_ESPERADAS` añade `TiposAula` y `Eventos`; `TABLAS_RETIRADAS` = `Curso_Materia`, `Recesos`.
`esquemaEsperado` comprueba las columnas de `Aulas`, `Materias`, `Eventos` y `TiposAula` (incluida
`clave`); el motivo nombra la tabla y la columna que falta. **El catálogo almacena la clave
normalizada** (recorte + `toCaseFolded()`, calculada por el servicio) y el **índice único va sobre
`clave`**: frena «Física»/«FÍSICA» y las escrituras directas. La huella nueva se registra en el
guardián; el `test_version_esquema` inserta dos filas con la misma `clave` (ASCII y acentuada) y
espera el fallo del índice.

### 4.2 Alta/edición de un evento

```
turno = obtenerTurno(turnoNombre); si no existe -> error
nombre no vacío; 1 <= pos <= turno.numSlots; duracion > 0
eventos = cargarEventos(turno) menos el candidato (si es edición)
orden = (orden == 0 y alta) ? ordenSiguiente(eventos, pos) : orden
orden >= 1; si ya existe (turno,pos,orden) -> error
si horas: ambas o ninguna; fin>inicio; (fin-inicio)/60 == duracion
eventos = eventos + candidato
si duracionFranjaEstimada(turno con eventos) <= 0 -> error de encaje
si horas y !eventoDentroDeVentana(candidato) -> error
si haySolapeCronologico(eventos) -> error
INSERT/UPDATE (la identidad no cambia al editar)
```

`actualizarEvento` exige `orden >= 1` (no hay “automático” en edición). El borrado no renumera.
La lista se devuelve ordenada por `(despues_de_slot, orden, id)`. Las horas ausentes se guardan como
`NULL` (no cadena vacía), para que el `CHECK` del par de horas funcione.

### 4.3 Actualización de un turno

```
si numSlots < turno.numSlots:
    afectados = eventos con pos > numSlots
    si afectados y !eliminarEventosFueraDeRango:
        error con turno, numSlots y lista de afectados (id, nombre, pos)
    si confirmado: DELETE afectados dentro de la transacción
si cambia la ventana [inicio,fin]:
    para cada evento con horas: si su intervalo no cabe -> error con los eventos
UPDATE Turnos
```

La reducción y el borrado de afectados son una sola transacción; si falla, no cambia nada.

### 4.4 Tipos de aula

```
clave = nombre.trimmed().toCaseFolded()
si clave vacía -> error
si existe otra fila con la misma clave (comparación en memoria con claveTipo) -> error de duplicado
crear/renombrar: guardar nombre = nombre.trimmed() y clave = clave; el índice único sobre `clave`
                 frena duplicados, incluidas escrituras directas y «Física»/«FÍSICA»
eliminar: si existe en Aulas.id_tipo_aula o Materias.id_tipo_aula ->
          error con la lista de aulas y materias dependientes (nunca se confirma)
```

Crear aula o materia: `idTipoAula > 0` y existente; si el catálogo está vacío, error. Actualizar sin
tipo (`idTipoAula <= 0`) se rechaza; no se conserva el anterior de forma implícita.

### 4.5 Seed: tipos, eventos y vaciado

```
tipos = {"Aula de estudio", "Laboratorio", "Taller"}   // los que usan las aulas
por cada aula: tipo = "Aula de estudio" | "Laboratorio" | "Taller"
tiposDeAulas = conjunto de tipos de las aulas
por cada materia: tipoFinalDeMateria(nombre, tiposDeAulas)
por cada turno: crearEvento("Receso", pos=2, orden=1, duracion=10, 08:20–08:30 / 14:00–14:10)
recuentos: eventos = 2, tiposAula = 3; recesos ya no existe (no se suma)
vaciado/reemplazo FK-seguro:
  Eventos, Disponibilidad_Profesor, Profesor_Materia, Cursos, PlanEstudio_Materia,
  PlanEstudio, Materias, Turnos, Aulas, TiposAula, Profesores
```

Las aulas y materias referencian el catálogo; los eventos referencian el turno. `Cursos` va antes que
`Turnos` y que `Aulas`; `Eventos` va antes que `Turnos`; `Aulas`/`Materias` antes que `TiposAula`.
`TiposAula`/`Eventos` cuentan como datos de dominio para la detección de base no vacía.

### 4.6 Puente eventos → sección `recesos`

```
for turno in datos.turnos:
    for evento in turno.eventos ordenados:
        recesos.append({turno, evento.despuesDeSlot, evento.duracion,
                        evento.inicio, evento.fin})
```

Una entrada por evento, aunque compartan posición; el nombre y el orden no viajan en el contrato 1.0.
`aulas[].tipo` sigue con el marcador `"regular"` hasta la 010 (RF-12 no añade tipos al contrato).

### 4.7 Huella de datos

```
por aula:      "A:<id>|<nombre>|<capacidad>|<idTipoAula>;"
por materia:   "M:<id>|<nombre>|<idTipoAula>;"
por turno:     "T:<nombre>|<inicio>|<fin>|<slots>;"
por evento:    "e:<id>|<nombre>|<pos>|<orden>|<duracion>|<inicio>|<fin>;"
```

El orden de recorrido es el de la instantánea; el hash SHA-256 resultante cambia si cambia un tipo o
un evento (RF-8). La prueba `test_servicio_generacion` modifica un `idTipoAula` y un evento y comprueba
que la huella cambia.

## 5. Decisiones (con la alternativa descartada)

1. **Evento con identidad propia y orden persistente.** `id` subrogado + único
   `(turno, posición, orden)`. Alternativa descartada: clave compuesta `(turno, posición)` o
   `(turno, posición, orden)` sin id; cambiar posición/orden rompería la identidad y la API de
   actualización/borrado.
2. **Orden estable.** Se guarda `orden` (máximo + 1 al crear) y se presenta por
   `(posición, orden, id)`; borrar no renumera. Alternativa descartada: `rowid`/orden de inserción;
   no es estable ni controlable y la especificación exige independencia del orden de registro.
3. **Puente `recesos` hasta la 010.** La 009 deriva la sección 1.0 de los eventos, una por evento.
   Alternativa descartada: cambiar ya a la sección de eventos/2.0; la 010 es la dueña del contrato y
   la 009 debe dejar el backend verde (RF-12).
4. **Build sin frontend.** Opción `BUILD_FRONTEND` (ON por defecto) + preset `backend` con
   `build-backend/`; `app_core` se mantiene siempre y el ejecutable GUI se omite. Alternativas
   descartadas: seguir construyendo el frontend (no compila hasta que su equipo migre); quitar
   `add_subdirectory(frontend)` del árbol (destructivo y dificulta la vuelta); duplicar el preset
   `full` (choca con el cache de root).
5. **Tests de frontend ligados al target.** `test/CMakeLists.txt:117-133` ya está guardado por
   `if(TARGET frontend AND TARGET backend)`; con `BUILD_FRONTEND=OFF` **no se configuran**
   `test_frontend_conversion` ni `test_generacion_seed`. No se reescriben: son deuda del equipo de
   frontend y el segundo además fija el 40 de la 008, que la 011 revisará. `test_instancia_unica`
   sigue porque `app_core` no depende de la UI. Alternativa descartada: borrar o comentar los tests;
   perderíamos la cobertura cuando el frontend vuelva.
6. **Unicidad normalizada de tipos.** **Se almacena la clave normalizada** (recorte de espacios
   extremos + `toCaseFolded()` Unicode), **calculada por el servicio**, y el **índice único va sobre
   `clave`**. Alternativa descartada: índice sobre `lower(trim(nombre))`; SQLite solo pliega ASCII y
   no frena «Física»/«FÍSICA» en escrituras directas. También se descarta `UNIQUE` sensible a
   mayúsculas o comprobar solo en la aplicación.
7. **Borrado del catálogo siempre rechazado en uso.** Error con dependientes; queda fuera del flujo
   genérico de “mostrar dependientes y confirmar”. Alternativa descartada: cascada o `SET NULL`;
   dejaría aulas/materias sin tipo (columnas `NOT NULL`).
8. **Regla de tipos del seed.** Ciencias experimentales → Laboratorio; informática/taller → Taller;
   Educación Física → Cancha; si no hay aula de ese tipo, genérico. El conjunto sembrado tiene
   Laboratorio y Taller, no Cancha: Educación Física queda genérica y la fila de taller es inocua
   (no hay materia de informática). Alternativa descartada: inventar una cancha o un aula nueva.
9. **Ventana de posición de la 009.** Se implementa la estimación `(ventana − Σ duraciones)/franjas`
   aprobada en la 009, aislada en `calculo_eventos`. Alternativa descartada: adelantar la duración
   fija de la 011; la 011 es otra spec y cambiará esta función sin tocar el servicio. La 009 se
   implementa tal como está aprobada.
10. **Reducción de franjas con confirmación explícita.** `actualizarTurno(..., bool)` y
    `eventosFueraDeRango`; sin confirmación se devuelve el error con los afectados. Alternativa
    descartada: borrar siempre (destructivo sin confirmación) o rechazar siempre (la spec pide
    confirmación).
11. **Consola.** Comandos `evento-agregar`/`evento-eliminar` (identidad) y `eventos` para listar; se
    retiran `receso-*`. Alternativa descartada: mantener alias de receso; la spec pide dejar de
    exponer recesos como concepto propio.
12. **Tipo en el contrato 1.0.** `aulas[].tipo` sigue con `"regular"` hasta la 010; la 009 no añade
    tipos al contrato (RF-12). Alternativa descartada: rellenar el tipo real ya; introduciría datos
    nuevos en el contrato que la 010 define.
13. **Base de datos de verificación.** `build-backend/` (nuevo, ignorado) con `BUILD_FRONTEND=OFF`;
    `build/` no se usa (cache de root) y `build-test/` está en modo frontend. Alternativa descartada:
    reutilizar `build-rel/` (Release y cache existente) o `build-test/` (BUILD_BACKEND=OFF).
14. **Tipo al final con centinela, retirado en T9.** Para que las tareas sigan compilando mientras los
    consumidores se actualizan por tramos, `idTipoAula` se añade al final con `-1`; el servicio rechaza
    `-1` en ejecución y **T9 elimina los valores por defecto**, de modo que la firma final exige el
    tipo (cambio limpio, sin valores por defecto silenciosos). Alternativa descartada: cambiar la
    posición del parámetro y romper de golpe `NucleoDatos`, seed, consola y tests en la misma tarea.
15. **Compatibilidad temporal del DTO de eventos.** T3 añade `EventoDTO`/`eventos` y conserva
    `RecesoDTO`/`recesos` y los métodos antiguos como capa derivada, para que constructor, huella,
    seed, consola y tests compilen sin un big-bang; **T9 la retira** y renombra la clase. Alternativa
    descartada: renombrar y actualizar todos los consumidores en T3; la tarea se disparaba y el árbol
    quedaba roto si faltaba un test.
16. **Turno inmutable y posición 0 inválida.** `actualizarEvento` valida el turno existente del evento
    pero no permite cambiarlo (mover = borrar + crear); `despuesDeSlot = 0` se rechaza siempre y el 0
    del DTO no significa «sin asignar». Alternativa descartada: aceptar cambio de turno en la edición,
    que obligaría a revalidar posición, orden y ventanas del turno destino.

## 6. Verificación por tarea

Comandos desde la raíz del repo. OR-Tools está en `$HOME/.local/ortools`; si el preset no lo
encuentra, se añade `-Dortools_DIR="$HOME/.local/ortools/lib/cmake/ortools"`.

### T1. Build sin frontend (10-15 min)

```
cmake --preset backend -Dortools_DIR="$HOME/.local/ortools/lib/cmake/ortools"
cmake --build --preset backend --parallel
ctest --test-dir build-backend -N
```

Esperado: configure y build **exit 0**; `ctest -N` lista `test_backend_version_esquema`,
`test_backend_nucleo_datos`, `test_seed_aplicacion`, `test_instancia_unica` y **no** lista
`test_frontend_conversion` ni `test_generacion_seed`. La suite completa aún puede estar roja por el
esquema WIP; T1 solo garantiza que el runner sin frontend existe y que **todos los targets compilan**.

### T2. Esquema v3 y versión (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos'
```

Esperado: build exit 0 y verde. `VERSION_ESQUEMA_ACTUAL == 3`; `crearEsquema` crea `TiposAula` y
`Eventos` y no `Recesos`; `TiposAula` guarda `clave` y el índice único sobre ella rechaza las parejas
«Laboratorio»/«laboratorio» y «Física»/«FÍSICA» (clave `física`); una base v2
(`VERSION_ESQUEMA_ACTUAL - 1`) falla al abrir; la huella de la versión 3 está registrada en
`HUELLAS_CONOCIDAS` y el guardián la acepta. La primera ejecución tras cambiar el DDL falla mostrando
la huella nueva: se copia y se repite.

### T3. Modelo y servicio de eventos, con compatibilidad (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_servicio_turnos_eventos'
```

Esperado: build exit 0 y verde. Se añaden `EventoDTO`/`TurnoDTO.eventos` y el CRUD de eventos sobre
`Eventos`; se conservan `RecesoDTO`/`recesos` y `agregarReceso`/`eliminarReceso` como capa derivada
(leen/escriben eventos), de modo que constructor, huella, seed, consola y tests siguen compilando.
Dos eventos en la misma posición se guardan con orden 1 y 2; la lista sale por posición/orden/id; el
borrado no renumera; la identidad permite editar/eliminar. La clase se sigue llamando
`ServicioTurnosRecesos` **hasta T9**; el test usa esa clase temporalmente. La consola añade `evento-*`
y conserva la lectura de recesos mientras dure la compatibilidad.

### T4. Coherencia temporal y turnos (25 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_calculo_eventos|test_servicio_turnos_eventos'
```

Esperado: build exit 0 y verde. Par de horas incompleto, `fin <= inicio`, diferencia distinta de la
duración, intervalo fuera de la ventana, solape dentro/entre posiciones y orden cronológico invertido
se rechazan; `duracionFranjaEstimada` con `numSlots <= 0` no divide por cero; reducir franjas sin
confirmar devuelve los afectados y con confirmación los borra; cambiar la ventana que deja fuera un
evento se rechaza.

### T5. Catálogo, tipos en aulas/materias y huella (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_servicio_tipos_aula|test_servicio_aula|test_servicio_materias|test_servicio_generacion'
```

Esperado: build exit 0 y verde. Renombrar solo mayúsculas no duplica (clave almacenada única; probar
«Laboratorio»/«laboratorio» y «Física»/«FÍSICA»); borrar un tipo en uso devuelve el error con
dependientes; el catálogo vacío impide crear aulas y materias (vía
`ServicioAula`/`ServicioMaterias`); aula/materia sin tipo o con tipo inexistente se rechazan;
actualizar sin tipo se rechaza; el aula/materia expone `idTipoAula` y `tipoAula`. `huellaDatos`
incluye `idTipoAula` y los eventos, y `test_servicio_generacion` comprueba que cambiar un tipo o un
evento cambia la huella. Los tests de aula/materia crean un tipo en su `setUp` y lo pasan en el
último argumento. El resto de consumidores sigue compilando con el centinela `-1` (que falla en
ejecución) hasta T6.

### T6. Fachada, cascada, puente y consumidores (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_backend_nucleo_datos|test_backend_cascada|test_gestor_pendientes|test_servicio_profesor|test_composicion_plan|test_servicio_carga_horaria|test_servicio_planes_estudio|test_datos_contexto_base_datos|test_constructor_entrada'
```

Esperado: build exit 0 y verde. `dependientesDe("turno")` lista eventos; `eliminarConCascada("turno")`
los borra antes del turno; la fachada expone los métodos nuevos; los consumidores de backend, consola
y tests pasan el tipo y usan `eventos`; el `INSERT` directo de materia de
`test_servicio_planes_estudio` incluye `id_tipo_aula`; el puente `eventos → recesos` se prueba en
`test_constructor_entrada` (una entrada por evento); la consola de aulas/materias y de `tipos` usa el
tipo. La huella ya se cubrió en T5. La compatibilidad del DTO sigue presente hasta T9.

### T7. Seed: perfil con tipos y eventos, con compatibilidad (25 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R '^test_seed_perfil$'
```

Esperado: build exit 0 y verde. Se añaden 3 tipos, tipo por aula, tipo resuelto por materia (las
«Aplicada» por prefijo/área) y un evento `Receso` por turno (posición 2, orden 1, 10 min,
08:20–08:30 / 14:00–14:10); los campos `receso*` y `Recuentos.recesos` **siguen existiendo como
compatibilidad** para que `sembrador.cpp` compile, y la validación rechaza un evento fuera de ventana
o un tipo sin aula.

### T8. Seed: aplicación, retirada de la compatibilidad y recuentos (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R '^test_seed_aplicacion$'
```

Esperado: build exit 0 y verde. Se insertan tipos, aulas/materias con tipo y eventos; `docentes.cpp`
pasa a `r.eventos`/`r.tiposAula`, se retiran `Recuentos.recesos` y los campos `receso*`, y
`resultado.recuentos.eventos == 2`/`tiposAula == 3`; la tabla `Eventos` tiene 2 filas con
nombre/orden; `Aulas.id_tipo_aula` y `Materias.id_tipo_aula` no nulos. El reemplazo total vacía en el
orden FK-seguro `Eventos, Disponibilidad_Profesor, Profesor_Materia, Cursos, PlanEstudio_Materia,
PlanEstudio, Materias, Turnos, Aulas, TiposAula, Profesores` (Eventos antes que Turnos; Cursos antes
que Turnos y Aulas; Aulas/Materias antes que TiposAula); la conmutación de docentes no toca tipos ni
eventos.

### T9. Cierre: retirar compatibilidad y defaults, renombrar el servicio (25 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_servicio_turnos_eventos|test_backend_nucleo_datos|test_backend_cascada|test_constructor_entrada|test_seed_aplicacion|test_servicio_aula|test_servicio_materias'
grep -rn "RecesoDTO\|agregarReceso\|eliminarReceso\|TurnoDTO.*recesos" src/backend src/seed src/consola test/backend
grep -rn "receso-" src/consola
```

Esperado: build exit 0, tests verdes y ambos `grep` sin coincidencias (el segundo detecta la ayuda
que aún ofreciera `turnos receso-agregar`/`receso-eliminar`). Se renombra
`ServicioTurnosRecesos`→`ServicioTurnosEventos` (archivos, clase, includes, CMake), se retiran
`RecesoDTO`/`TurnoDTO.recesos` y los métodos antiguos, el dominio de cascada pasa de `receso` a
`evento`, y **se eliminan los valores por defecto `-1` de `idTipoAula`** en `ServicioAula`,
`ServicioMaterias` y `NucleoDatos`: la firma final exige el tipo y **mantiene el orden**
(`idTipoAula` al final), sin reordenar call sites. El puente del constructor ya usa `eventos` desde T6
y su test sigue verde.

### T10. Documentación, deuda del frontend y regresión global (25 min)

```
grep -in "receso" docs/interfaz-frontend.md docs/guias/consola-datos.md docs/guias/datos-prueba.md
cmake --build --preset backend --parallel
ctest --preset backend
```

Esperado: build exit 0; en el `grep -i`, las **únicas** coincidencias permitidas son el **nombre del
evento «Receso»** (p. ej. `Evento Receso`); no puede aparecer `RecesoDTO`, `agregarReceso`,
`receso-*` ni «Recesos» como entidad. Las coincidencias restantes se revisan a mano (RF-11) y, si no
son el nombre del evento, se corrigen. `ctest --preset backend` termina **100 % verde**, sin
`test_frontend_conversion` ni `test_generacion_seed`. La guía de API documenta
`EventoDTO`/`TurnoDTO.eventos`, los tipos de aula y materia, el CRUD de tipos, la cascada de turno a
eventos, la clave de pendiente de evento, la fórmula de duración con la suma de eventos, **la deuda
del frontend** (heurísticas de tipo por nombre/texto libre y aritmética en minutos) y que **la
interfaz queda fuera de alcance**. Manual exacto:
```
./build-backend/src/seed/gestor-horarios-seed --db /tmp/009-nueva.db
./build-backend/src/consola/gestor-horarios-consola --db /tmp/009-nueva.db --cmd "turnos listar"
./build-backend/src/consola/gestor-horarios-consola --db /tmp/009-nueva.db --cmd "turnos evento-agregar \"Mañana\" \"Acto cívico\" 3 10"
./build-backend/src/consola/gestor-horarios-consola --db /tmp/009-nueva.db --cmd "turnos listar"
./build-backend/src/consola/gestor-horarios-consola --db /tmp/009-nueva.db --ayuda
```
Esperado: 2 eventos y luego 3; la ayuda no ofrece `receso-*`; base v3 reabrible; una base v2 sigue
fallando por `test_apertura`.

## 7. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|----|----------|--------------|
| RF-1 | T3, T8 | `test_servicio_turnos_eventos` (nombre, turno, posición, duración, horas); `test_seed_aplicacion` (2 eventos) |
| RF-2 | T2, T3 | `test_backend_version_esquema` (id/orden/unique); `test_servicio_turnos_eventos` (orden máximo+1, sin renumerar, orden de lista) |
| RF-3 | T4 | `test_calculo_eventos` y `test_servicio_turnos_eventos` (par, diferencia, ventana, solapes, cronología) |
| RF-4 | T4 | `test_servicio_turnos_eventos` (reducción con/sin confirmación, ventana del turno) |
| RF-5 | T2, T5 | `test_backend_version_esquema` (clave almacenada + índice único); `test_servicio_tipos_aula` (CRUD, renombre de mayúsculas, ASCII/acento, borrado en uso) y `test_servicio_aula`/`test_servicio_materias` (catálogo vacío / sin tipo) |
| RF-6 | T2, T5, T6, T9 | `test_backend_version_esquema` (columna); `test_servicio_aula` (tipo obligatorio/actualización sin tipo); `test_backend_nucleo_datos`; T9 compila sin valores por defecto |
| RF-7 | T2, T5, T6, T9 | `test_backend_version_esquema` (columna); `test_servicio_materias` (tipo obligatorio/actualización sin tipo); `test_backend_nucleo_datos`; T9 compila sin valores por defecto |
| RF-8 | T3, T5, T6, T8, T9, T10 | `test_backend_nucleo_datos` (eventos, tipos), `test_servicio_generacion` (huella con tipos y eventos, T5), `test_backend_cascada` (evento en turno), `test_gestor_pendientes`, `grep` de T9 y manual de consola de T10 |
| RF-9 | T2 | `test_backend_version_esquema` (v3, tablas/columnas, huella), `test_backend_apertura_base_datos` (v2 falla); `test_datos_contexto_base_datos` se une en T6 al actualizar la firma |
| RF-10 | T7, T8 | `test_seed_perfil` (regla de tipos, eventos, `docentes.cpp`), `test_seed_aplicacion` (recuentos, vaciado, reemplazo) |
| RF-11 | T10 | `grep` de las tres guías + revisión manual de DTOs/firmas/fórmula/deuda del frontend |
| RF-12 | T6, T9 | `test_constructor_entrada` (una entrada por evento) y T9 tras retirar la compatibilidad |
| RNF-1 | T3, T4, T5 | Mensajes de error de los tests de servicio (campo/valor/rango) |
| RNF-2 | T2 | `test_backend_apertura_base_datos` (sin migración, sin borrar) |
| RNF-3 | T7, T8 | `test_seed_perfil`/`test_seed_aplicacion` deterministas |
| RNF-4 | T2, T10 | `test_backend_version_esquema` + reapertura manual |
| RNF-5 | T10 | `grep` y revisión de `docs/interfaz-frontend.md` |
| RNF-6 | T1, T10 | `ctest -N` sin tests de frontend; `ctest --preset backend` verde |
| RNF-7 | Todas | Esta tabla + el `Hecho cuando` de cada tarea |

## 8. Condiciones y riesgos

- **El frontend no compila** hasta que su equipo adapte las llamadas; `full`/`dev-frontend` quedan
  rojos. La verificación de esta entrega es el preset `backend`.
- **Cada tarea deja el árbol compilando.** El `Hecho cuando` de T1 fija el comando
  `cmake --build --preset backend --parallel`; todas las tareas lo repiten antes de su `ctest`. T3
  conserva la compatibilidad del DTO/servicio y T9 la retira; así ningún corte intermedio deja
  consumidores rotos.
- **La suite de partida está roja** por el esquema a medio integrar del commit Base y por los
  servicios que aún consultan `Recesos`; T1 no exige verde, T2/T3 empiezan a recuperarlo y T10 cierra
  el verde global.
- **`test_instancia_unica`** depende de `app_core`; por eso `src/app` se sigue añadiendo y solo se
  omite el ejecutable GUI. Si se omitiera `app`, el test quedaría sin target.
- **Los tests de frontend 117-133** no se tocan; el guard existente los omite. `test_generacion_seed`
  queda fuera también para la 011, que deberá decidir si lo recupera con el frontend adaptado.
- **OR-Tools**: si el preset no lo encuentra, `-Dortools_DIR=$HOME/.local/ortools/lib/cmake/ortools`
  (valor ya presente en el cache de `build-rel`).
- **Sin migración**: una base v2 se rechaza; no se convierte `Recesos` ni se rellenan tipos.
