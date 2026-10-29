# Plan 012 - Preferencias horarias del docente

Base: 3007c54

> **Alcance.** Este plan implementa la spec 012 (8 RF) sobre `3007c54`, que ya contiene la **009**, la
> **010** y la **011** cerradas e implementadas (esquema **v4**, contrato 2.0, configuración global de
> la hora académica). La entrega es **backend, datos, seed y consola** (`BUILD_FRONTEND=OFF`, preset
> `backend`). **No** se toca el solver, ni el contrato, ni la generación, ni la interfaz: las
> preferencias solo se persisten, se validan y se exponen. No hay migración: la base sube a **esquema
> v5** y una base v4 se rechaza al abrir.
>
> **Punto de partida verificado.** `3007c54` es HEAD de `feat/Base-datos`, con el árbol limpio y la
> suite del preset `backend` en **52/52**. La disponibilidad del docente vive en la tabla
> `Disponibilidad_Profesor` y se gestiona dentro de **`ServicioProfesor`** (`agregarDisponibilidad`,
> `eliminarDisponibilidad`, `limpiarDisponibilidad`, `cargarDisponibilidad`); **no existe** un
> `ServicioDisponibilidad` separado. Este plan replica ese modelo en una tabla y un servicio propios.

## 1. Arquitectura y archivos

### 1.1 Modelo puro y validación compartida (RF-1, RF-2, RNF-1)

- **Nuevo** `src/backend/include/backend/services/franja_horaria_validacion.hpp` y
  `src/backend/src/services/franja_horaria_validacion.cpp`:
  - `DIA_MIN = 0`, `DIA_MAX = 6`.
  - `QString errorDeFranja(const FranjaHoraria&)`: día 0–6 **citando el valor recibido**, horas válidas,
    inicio anterior al fin y acción disponible («Corrija el día», «Corrija el rango»). Cadena vacía si
    la franja es válida.
  - **Se reutiliza en la disponibilidad:** `ServicioProfesor.cpp` deja su `validarFranja` estático y
    pasa a `franja_horaria::errorDeFranja`. Es la **única** definición de día/rango; los mensajes de
    disponibilidad mejoran (incluyen el valor) sin cambiar su semántica. Alternativa descartada:
    duplicar la regla en preferencias (dos mensajes y dos fuentes de verdad).
- **Nuevo** `src/backend/include/backend/services/preferencias.hpp` y
  `src/backend/src/services/preferencias.cpp` (puro, sin `QSql*`):
  - `enum class SentidoPreferencia { Preferida, Evitada };`
  - `QString textoDeSentido(SentidoPreferencia)` → `"preferida"` / `"evitada"` (forma persistida).
  - `bool sentidoDesdeTexto(const QString&, SentidoPreferencia*)` (lectura de la base).
  - `bool sentidoValido(SentidoPreferencia)`: defensa ante un valor casteado fuera del enumerado (la
    base tiene además `CHECK`).
  - `bool haySolape(const FranjaHoraria&, const FranjaHoraria&)`: `a.inicio < b.fin && b.inicio < a.fin`;
    **los rangos contiguos no se solapan**.

### 1.2 Esquema y versión (RF-1, RF-6)

- `src/backend/src/database/migracion.cpp`: tabla `Preferencia_Profesor` (§3), después de
  `Disponibilidad_Profesor`.
- `src/backend/include/backend/database/version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 5` y comentario
  de la v5.
- `src/backend/src/database/version_esquema.cpp`: `Preferencia_Profesor` en `TABLAS_ESPERADAS` y
  `COLUMNAS_PREFERENCIAS = {id, id_Profesor, dia, hora_inicio, hora_fin, sentido}` en
  `TABLAS_CON_COLUMNAS`.
- `test/backend/test_version_esquema.cpp`: versión 5, tabla/columnas, `CHECK` de día/sentido/horas,
  `UNIQUE` del duplicado exacto y huella v5 en `HUELLAS_CONOCIDAS` **en el mismo cambio**; renombrar
  `versionActual_esCuatro` → `versionActual_esCinco` (`:142`).
- `test/backend/test_servicio_configuracion_horaria.cpp:15`: el comentario «La base se crea con el
  esquema v4» pasa a v5 (el comportamiento del test no cambia).

### 1.3 Servicio de preferencias (RF-1, RF-2, RF-3, RNF-1)

- **Nuevo** `src/backend/include/backend/services/ServicioPreferencias.hpp` y
  `src/backend/src/services/ServicioPreferencias.cpp`:
  - `struct PreferenciaDTO { int id = 0; QString idProfesor; FranjaHoraria franja;
    SentidoPreferencia sentido = SentidoPreferencia::Preferida; };`
  - `Resultado<PreferenciaDTO> agregar(const QString& idProfesor, const FranjaHoraria&,
    SentidoPreferencia)`.
  - `Resultado<QVector<PreferenciaDTO>> listar(const QString& idProfesor) const` (orden de RF-1).
  - `Resultado<PreferenciaDTO> actualizar(int id, const FranjaHoraria&, SentidoPreferencia)`.
  - `Resultado<bool> eliminar(int id)` (error explícito si no existe; un `bool` no podría decirlo).
  - `Resultado<int> limpiar(const QString& idProfesor)` (número de filas borradas; 0 es válido).
  - Toda la validación de RF-2 en el servicio, con una sola consulta de las preferencias del
    docente+día para duplicados y solapes.

### 1.4 DTO del docente y fachada `NucleoDatos` (RF-3)

- `src/backend/include/backend/services/ServicioProfesor.hpp`: `ProfesorDTO` gana
  `QVector<PreferenciaDTO> preferencias` (junto a `disponibilidad`, **sin mezclarlas**) y declara
  `bool cargarPreferencias(ProfesorDTO&) const` **en la sección privada**, junto a
  `cargarDisponibilidad`/`cargarMaterias` (no se expone en la API del servicio).
- `src/backend/src/services/ServicioProfesor.cpp`: `obtenerProfesor` y `listarProfesores` cargan las
  preferencias con `ServicioPreferencias(m_db).listar(id)`, **sin duplicar el SELECT ni el orden**.
  `ProfesorDTO::toProfesor()` las **ignora** (el modelo del solver no cambia).
- `src/backend/include/backend/services/NucleoDatos.hpp` / `src/backend/src/services/NucleoDatos.cpp`:
  miembro `ServicioPreferencias m_preferencias` y métodos `agregarPreferencia`,
  `listarPreferencias`, `actualizarPreferencia`, `eliminarPreferencia` y `limpiarPreferencias`, con
  `conPendiente` como el resto de escrituras de la fachada (todas devuelven `Resultado`). Claves de
  pendiente: alta `idProfesor|dia|inicio|fin|sentido`; modificación y baja, `id`; limpieza,
  `idProfesor`.

### 1.5 Cascada y baja del docente (RF-4)

- `src/backend/src/services/cascada.cpp`: nueva relación del dominio `profesor`:
  `preferencia` → tabla `Preferencia_Profesor`, columna `id_Profesor`, `columnaId id`, etiqueta
  `sentido || ' ' || dia || ' ' || hora_inicio || '-' || hora_fin`, descripción «Preferencia horaria».
  Con eso, `dependientesDe("profesor", id)` la muestra antes de confirmar y `eliminarConCascada` la
  borra antes que el docente.
- `src/backend/src/services/ServicioProfesor.cpp`: `eliminarProfesor` añade el borrado de
  `Preferencia_Profesor` a su transacción (antes de `Profesores`), igual que ya hace con disponibilidad
  y materias. Son **dos caminos** de baja (`NucleoDatos::eliminarDocente` y
  `NucleoDatos::eliminarConCascada`) y ambos deben quedar cubiertos.

### 1.6 Seed y conmutación de variante (RF-7)

- `src/seed/include/seed/perfil.hpp`: `Recuentos` gana `int preferencias = 0` (0 en el conjunto;
  es un recuento de dominio, no como la configuración de la 011).
- `src/seed/src/sembrador.cpp`:
  - `kTablasDominio` incluye `Preferencia_Profesor` (12 tablas de dominio del esquema v5) y se
    actualiza el comentario de `src/seed/src/sembrador.cpp:34` («Las 11 tablas … esquema v4»).
  - `leerRecuentos` cuenta la tabla; `recuentosCoinciden` la compara; `resumenTexto` imprime
    `Preferencias: 0`.
  - `--solo-docentes` borra `Preferencia_Profesor` **antes** que `Disponibilidad_Profesor`,
    `Profesor_Materia` y `Profesores`.
  - El reemplazo total añade `Preferencia_Profesor` a la lista FK-segura antes de `Profesores`.
  - No se inserta ninguna preferencia: el conjunto queda vacío.
- `src/seed/src/docentes.cpp`: `recuentosEsperados` deja `preferencias = 0` explícito para todas las
  variantes.

### 1.7 Huella de datos (RF-5)

- **No se toca** `huellaDatos` ni `DatosDominio`: las preferencias no viajan al solver ni entran en la
  instantánea de generación. `ProfesorDTO::toProfesor()` las ignora.
- Queda anotada la condición futura **en este plan y en la guía (§8.1), no en el código**: cuando una
  spec del solver las consuma, deberán añadirse al DTO de dominio y a la huella en el mismo cambio.
  `ServicioGeneracion.{hpp,cpp}` no debe contener la palabra «preferencia».

### 1.8 Documentación (RF-8, la aplica el coordinator)

- `docs/interfaz-frontend.md`: DTO, bloque de `NucleoDatos`, nueva §2.4.5, tabla §4 y nota de que no
  hay cambios de firma que rompan. Contenido exacto en §8.
- `docs/guias/datos-prueba.md`: el conjunto siembra **0** preferencias (fila de la tabla de entidades).
- `specs/008-datos-de-prueba/spec.md`: al cerrar, el planner anota la supersesión de RF-9 en lo que
  toca a las preferencias (§8.2).

### 1.9 Pruebas

- `src/backend/CMakeLists.txt`: añadir `franja_horaria_validacion.cpp`, `preferencias.cpp` y
  `ServicioPreferencias.cpp`; `test/CMakeLists.txt`: los dos binarios nuevos.
- **Nuevas:** `test/backend/test_preferencias.cpp` (puro) y
  `test/backend/test_servicio_preferencias.cpp` (con base de datos), registradas en
  `test/CMakeLists.txt` (`add_qtest(... backend)` y `add_qtest(... backend Qt6::Sql)`).
- **Ampliadas:**
  - `test/backend/test_servicio_profesor.cpp`: DTO con preferencias y baja.
  - `test/backend/test_cascada.cpp`: dependencias (`dependenciasDe("profesor").size()` 2 → 3) y
    borrado.
  - `test/backend/test_version_esquema.cpp`: v5, restricciones y huella; renombrar
    `versionActual_esCuatro` → `versionActual_esCinco`.
  - `test/backend/test_servicio_configuracion_horaria.cpp:15`: el comentario «La base se crea con el
    esquema v4» pasa a v5 (sin cambio de comportamiento).
  - `test/backend/test_seed_perfil.cpp` y `test_seed_aplicacion.cpp`: recuento 0, orden de borrado,
    atomicidad y determinismo; comentario (`:347`) y lista (`:350`) de «las 11 tablas» → 12 con
    `Preferencia_Profesor`.
  - `test/backend/test_nucleo_datos.cpp`: fachada.
  - `test/backend/test_servicio_preferencias.cpp`: prueba de huella (añade una `PreferenciaDTO` a
    `ProfesorDTO::preferencias` y verifica que `huellaDatos` no cambia); `test_servicio_generacion`
    queda como **regresión** de la huella actual.
- **Sin tocar:** `test/frontend/*` (fuera del preset `backend`) y los tests que no hablan de docentes,
  preferencias, cascada o seed.

## 2. Funciones puras y pseudocódigo

### 2.1 `franja_horaria` (validación compartida)

```
errorDeFranja(f):
    si f.dia < 0 o f.dia > 6:
        "El día debe ser un entero entre 0 (domingo) y 6 (sábado); se recibió <dia>. Corrija el día."
    si !f.inicio.isValid() o !f.fin.isValid():
        "Las horas de la franja deben ser válidas (HH:mm); se recibió <inicio>–<fin>. Corrija el rango."
    si f.inicio >= f.fin:
        "La hora de inicio (<inicio>) debe ser anterior a la de fin (<fin>). Corrija el rango."
    ""
```

### 2.2 `preferencias` (puro)

```
textoDeSentido(Preferida) = "preferida"
textoDeSentido(Evitada)   = "evitada"
sentidoDesdeTexto("preferida", *s) = true; "evitada" = true; otro = false
sentidoValido(s) = (s == Preferida || s == Evitada)
haySolape(a, b) = a.inicio < b.fin && b.inicio < a.fin
```

### 2.3 Servicio

```
agregar(idProfesor, franja, sentido):
    id vacío -> error "El docente es obligatorio para registrar una preferencia. Indique el
                       docente y vuelva a intentarlo."
    e = franja_horaria::errorDeFranja(franja); si e -> error   // ya trae valor y acción
    si !sentidoValido(sentido) ->
        error "El sentido de la preferencia debe ser «preferida» o «evitada»; se recibió
               <static_cast<int>(sentido)>. Corrija el sentido."
    SELECT 1 FROM Profesores WHERE id = :id   // directo: no se usa ServicioProfesor (ciclo)
    si no hay fila -> error "No existe el docente «X». Regístrelo antes de guardar preferencias."
    existentes = SELECT ... WHERE id_Profesor = :id AND dia = :diaNuevo
    para cada existente:
        si mismo día/inicio/fin/sentido ->
            error "Ya existe una preferencia idéntica para el docente «X» el día D de
                   <inicio> a <fin> (<sentido>). Cambie el rango o el sentido, o edite la
                   preferencia existente."
        si sentido != existente.sentido y haySolape(franja, existente.franja):
            error "La preferencia <inicio>–<fin> (<sentido>) se solapa con <inicio>–<fin>
                   (<otro sentido>) del docente «X» el día D. Cambie el rango o el sentido de
                   una de las dos."
    INSERT (id_Profesor, dia, hora_inicio, hora_fin, sentido)
    -> PreferenciaDTO con el id insertado

listar(idProfesor):
    SELECT id, id_Profesor, dia, hora_inicio, hora_fin, sentido
      WHERE id_Profesor = :id
      ORDER BY dia, hora_inicio, hora_fin, id
    -> vector (vacío si el docente no tiene; no es error)

actualizar(id, franja, sentido):
    id <= 0 -> error "El identificador de la preferencia no es válido; se recibió N.
                      Recargue la lista y vuelva a intentarlo."
    fila = SELECT ... WHERE id = :id;
    si no -> error "No se encontró la preferencia con id N. Recargue la lista y vuelva a
                    intentarlo."
    (defensivo) SELECT 1 FROM Profesores WHERE id = fila.idProfesor; si no ->
        error "No existe el docente «X» de la preferencia. Recargue la lista y vuelva a
               intentarlo."
    errorDeFranja / sentidoValido -> error
    // Se comparan duplicado y solape contra el DÍA NUEVO de la franja, no el anterior,
    // excluyendo la propia fila:
    demás = SELECT ... WHERE id_Profesor = fila.idProfesor AND dia = :diaNuevo AND id != :id
    duplicado exacto / solape opuesto contra `demás` -> error con las mismas acciones
    UPDATE ... SET dia, hora_inicio, hora_fin, sentido WHERE id = :id (el docente no cambia)
    -> PreferenciaDTO actualizado

eliminar(id):
    id <= 0 -> error "El identificador de la preferencia no es válido; se recibió N.
                      Recargue la lista y vuelva a intentarlo."
    DELETE ... WHERE id = :id; 0 filas ->
        error "No se encontró la preferencia con id N. Recargue la lista y vuelva a intentarlo."
    -> ok

limpiar(idProfesor):
    idProfesor vacío -> error "El docente es obligatorio para limpiar sus preferencias.
                                Indique el docente y vuelva a intentarlo."
    DELETE ... WHERE id_Profesor = :id -> número de filas (0 es válido)
```

## 3. Esquema v5

DDL en `migracion.cpp`, después de `Disponibilidad_Profesor`:

```sql
CREATE TABLE IF NOT EXISTS Preferencia_Profesor (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  id_Profesor TEXT NOT NULL REFERENCES Profesores(id),
  dia TEXT NOT NULL CHECK(dia IN ('0','1','2','3','4','5','6')),
  hora_inicio TEXT NOT NULL,
  hora_fin TEXT NOT NULL,
  sentido TEXT NOT NULL CHECK(sentido IN ('preferida','evitada')),
  fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CHECK(hora_inicio < hora_fin),
  UNIQUE(id_Profesor, dia, hora_inicio, hora_fin, sentido)
);
```

- Se replica el **tipo TEXT** de `Disponibilidad_Profesor` para `dia` y el formato `HH:mm` de las horas;
  el `CHECK(dia IN ('0'…'6'))` cierra la escritura directa y el servicio valida el entero con mensaje.
  El orden `dia, hora_inicio, hora_fin` funciona con un solo dígito y `HH:mm`.
- `UNIQUE` es la última barrera del **duplicado exacto** y, al ser un índice compuesto que empieza
  por `id_Profesor`, cubre también las consultas por docente/día; **no se añade un índice extra**.
  Los solapes de sentidos opuestos son una regla de servicio (no expresable con un `CHECK`).
- `CHECK(hora_inicio < hora_fin)` compara texto `HH:mm` (cero rellenado), que es orden cronológico.
- `VERSION_ESQUEMA_ACTUAL = 5`; catálogo y `HUELLAS_CONOCIDAS` en el mismo cambio. La huella real se
  copia del fallo del test.

## 4. Servicio: interfaz

```cpp
struct PreferenciaDTO {
    int id = 0;
    QString idProfesor;
    FranjaHoraria franja;                     // dia 0–6, inicio, fin
    SentidoPreferencia sentido = SentidoPreferencia::Preferida;
};

class ServicioPreferencias {
public:
    explicit ServicioPreferencias(QSqlDatabase& db);

    Resultado<PreferenciaDTO> agregar(const QString& idProfesor,
                                      const FranjaHoraria& franja,
                                      SentidoPreferencia sentido);
    Resultado<QVector<PreferenciaDTO>> listar(const QString& idProfesor) const;
    Resultado<PreferenciaDTO> actualizar(int id, const FranjaHoraria& franja,
                                         SentidoPreferencia sentido);
    Resultado<bool> eliminar(int id);
    Resultado<int>  limpiar(const QString& idProfesor);
};
```

- `listar` no valida que el docente exista: un docente sin preferencias devuelve una lista vacía (caso
  límite de la spec).
- `limpiar` no exige que el docente exista: devuelve 0 filas si no hay; solo falla por id vacío o SQL.
- `actualizar` **no cambia el docente**: mover una preferencia de docente es borrar y crear. Evita
  revalidar dos docentes a la vez y mantiene la operación acotada.

## 5. DTO y fachada

```cpp
// ServicioProfesor.hpp
struct ProfesorDTO {
    ...
    QVector<DisponibilidadDTO> disponibilidad;
    QVector<PreferenciaDTO>    preferencias;   // dato distinto, no se mezcla
    QVector<MateriaAsignadaDTO> materias;
};

// NucleoDatos
Resultado<PreferenciaDTO> agregarPreferencia(const QString& idProfesor,
                                             const FranjaHoraria& franja,
                                             SentidoPreferencia sentido);
Resultado<QVector<PreferenciaDTO>> listarPreferencias(const QString& idProfesor) const;
Resultado<PreferenciaDTO> actualizarPreferencia(int id, const FranjaHoraria& franja,
                                                SentidoPreferencia sentido);
Resultado<bool> eliminarPreferencia(int id);
Resultado<int>  limpiarPreferencias(const QString& idProfesor);
```

- `ServicioProfesor::cargarPreferencias` delega en `ServicioPreferencias::listar` (una sola definición
  del SELECT y del orden). `obtenerProfesor` y `listarProfesores` la llaman junto a disponibilidad y
  materias.
- `ProfesorDTO::toProfesor()` **no** copia preferencias: el modelo del solver no cambia (RF-5).

## 6. Cascada y baja

- `dependenciasDe("profesor")` añade la relación de preferencias. `dependientesDe` la muestra con la
  etiqueta `sentido dia inicio-fin` y `eliminarConCascada` la borra antes del docente.
- `ServicioProfesor::eliminarProfesor` borra `Preferencia_Profesor` en su transacción, antes de
  `Profesores`; `NucleoDatos::eliminarDocente` sigue delegando en él.
- Un fallo en cualquier paso revierte todo (transacción).

## 7. Seed

- `Recuentos.preferencias = 0`; `leerRecuentos` cuenta `Preferencia_Profesor`; `recuentosCoinciden` la
  compara; `resumenTexto` imprime `Preferencias: 0`.
- `kTablasDominio` incluye `Preferencia_Profesor`: una base con preferencias ya no es «vacía» para la
  siembra (es dato de dominio, a diferencia de `ConfiguracionHoraria`).
- `--solo-docentes`: `DELETE Preferencia_Profesor` → `Disponibilidad_Profesor` → `Profesor_Materia` →
  `Profesores`, todo en la transacción única. Si falla, rollback completo.
- Reemplazo total: `Preferencia_Profesor` en la lista FK-segura antes de `Profesores`.
- No se inserta ninguna preferencia: el conjunto queda con **0** y el determinismo se mantiene.

## 8. Documentación que aplica el coordinator (RF-8)

### 8.1 Contenido exacto

**`docs/interfaz-frontend.md`**

1. **§2.2, `:106` y `:118-121`:** `constexpr int VERSION_ESQUEMA_ACTUAL = 4;` pasa a **`= 5`** con el
   comentario «v5: preferencias horarias del docente». El párrafo completo de `:118-121` queda así:
   > `decidirApertura` es pura (sin E/S): `versionArchivo <= 0` representa versión ausente.
   > `crearEsquema` aplica el catálogo completo (`Migracion::aplicarPaso`) y fija `user_version` en una
   > sola transacción; si algo falla, revierte. La versión **5** añade las preferencias
   > (`Preferencia_Profesor`) al catálogo de la **v4**. La **v4** añadió la configuración global de la
   > hora académica (`ConfiguracionHoraria`, una fila con `duracion_minutos` entre 30 y 90) al catálogo
   > `TiposAula` (con su `clave` normalizada) y `Eventos` (identidad y orden) de la **v3**; el catálogo
   > esperado incluye las tablas y sus columnas. Una base con la versión anterior (**4**) **no se
   > migra**: se rechaza al abrir.
2. §2.4, párrafo inicial: añadir «preferencias horarias» a la lista de dominios de la fachada.
3. §2.4, bloque de `NucleoDatos`, tras los métodos de docentes: las cinco operaciones de preferencias
   con las firmas de §5.
4. §2.4, tabla de DTO: añadir `preferencias` a `ProfesorDTO` y una fila para `PreferenciaDTO`
   (`id`, `idProfesor`, `franja`, `sentido`).
5. **§2.4.1, fila `profesor` de la tabla de cascada (hoy `:307`):** la fila pasa de «profesor-materia y
   disponibilidad» a «profesor-materia, disponibilidad y **preferencias**».
6. Nueva **§2.4.5** «Preferencias horarias (`ServicioPreferencias`)»:
   - Modelo: docente, día, inicio, fin y **sentido** (`preferida`/`evitada`), con `id` propio; es un
     dato **distinto** de la disponibilidad (factibilidad) y **no** interviene todavía en la generación.
   - Día **0–6** (0 = domingo … 6 = sábado), el mismo rango de la disponibilidad; sábado y domingo se
     guardan y se exponen, pero no intervienen en la generación actual.
   - Validación: docente existente; día 0–6; inicio anterior al fin; sentido válido; **sin solapes de
     sentidos opuestos** en el mismo docente y día; **sin duplicado exacto**; los solapes y rangos
     contiguos del **mismo sentido** se aceptan y no se fusionan; el cambio de sentido se revalida y,
     si contradice, se rechaza conservando el anterior; el cambio de día se compara contra el **día
     nuevo**.
   - Orden: día, hora de inicio, hora de fin e `id`.
   - Operaciones: `agregar`, `listar`, `actualizar`, `eliminar` y `limpiar`; errores en español con
     campo, valor, causa y **acción**; `eliminar` devuelve `Resultado<bool>` para poder decir «no
     existe» y `limpiar` devuelve `Resultado<int>` con el número de filas (0 es válido).
   - Baja del docente y cascada: borran preferencias y disponibilidad en una transacción;
     `dependientesDe("profesor", id)` las muestra antes de confirmar.
   - Sin efecto en la generación/huella; `ProfesorDTO::toProfesor()` las ignora. **Condición futura:**
     cuando una spec del solver las consuma, deberán entrar en `DatosDominio` y en la huella.
   - **Cambios de firma:** ninguno rompe la compilación; `ProfesorDTO` gana un campo aditivo y las
     operaciones de `NucleoDatos` son nuevas.
7. §4, tabla resumen: fila `PreferenciaDTO` / CRUD de preferencias + cascada.

**`docs/guias/datos-prueba.md`**

- Tabla §1: fila `Preferencias (no se siembran) | 0`.
- **Ejemplo de salida (`:82-93`):** añadir `Hora académica: 35 min (configuraciones: 1)` tras
  `Variante:` (deuda de la 011 que el ejemplo arrastraba) y `Preferencias: 0` en la línea de
  `Cursos…`; el resto del ejemplo no cambia.

### 8.2 Supersesión de la 008 (la anota el planner al cerrar)

No es una tarea del implementer. Al cerrar la 012, el planner añadirá en
`specs/008-datos-de-prueba/spec.md` (RF-9 y la definición de reemplazo de docentes) una nota:

> **Superado en lo que toca a las preferencias por la 012 (fecha de cierre).** La conmutación de
> variante borra también `Preferencia_Profesor` antes que los docentes; la 008 conserva el resto de su
> RF-9.

La spec 012 ya declara la supersesión; la nota solo la deja visible en el texto histórico.

### 8.3 Cierre documental

Al cerrar (tras `VEREDICTO: APROBADO`), el **planner** actualiza `estado.md` (fase cerrada, veredicto,
commits y deudas) y deja el **contenido completo de `MEMORY.md`** —hoy dice «esquema v4» en la decisión
de la 011—; lo aplica el **coordinator**, porque el planner no escribe en la raíz. También anota la
supersesión de la 008 (§8.2).

## 9. Decisiones (con la alternativa descartada)

1. **Tabla y servicio separados de la disponibilidad.** Descartado: añadir `sentido` a
   `Disponibilidad_Profesor` (mezclaría factibilidad y calidad, obligaría a un nullable y contaminaría
   el DTO del solver). La spec lo decide así.
2. **`dia` como TEXT con `CHECK` de 0–6, replicando la disponibilidad.** Descartado: `INTEGER` (más
   natural, pero dos tablas hermanas con tipos distintos y conversiones en cada consulta).
3. **`Resultado<T>` también en `eliminar`/`limpiar`.** Descartado: `bool` como en disponibilidad (no
   puede «indicar que no existe», que la spec exige en el caso límite).
4. **El docente es inmutable en `actualizar`.** Descartado: permitir cambiar `id_Profesor` (obligaría a
   revalidar contra las preferencias de dos docentes; para moverla se borra y se crea).
5. **Validación de día/rango compartida con la disponibilidad.** Descartado: duplicar la regla en
   preferencias (dos mensajes y dos fuentes). El refactor de `ServicioProfesor` es mínimo y sus tests
   no fijan el texto.
6. **`ServicioProfesor::cargarPreferencias` delega en el servicio de preferencias.** Descartado:
   repetir el SELECT y el `ORDER BY` en dos sitios. La comprobación «¿existe el docente?» del servicio
   de preferencias es un `SELECT 1 FROM Profesores` directo: usar `ServicioProfesor` crearía un ciclo
   de cabeceras.
7. **`Recuentos.preferencias` en el seed.** Descartado: un campo aparte como `configuraciones` de la
   011 (la configuración no es dominio; las preferencias sí).
8. **Dos caminos de baja actualizados (`eliminarProfesor` y cascada).** Descartado: solo la cascada (el
   borrado directo de `NucleoDatos::eliminarDocente` dejaría huérfanas o fallaría por FK).
9. **Sin comando de consola nuevo.** Ningún RF lo pide; la consola debe seguir compilando y pasando.
10. **Sin cambios en la huella ni en el solver.** Descartado: incluirlas ya (no intervienen en la
    generación; la spec lo prohíbe hasta que el solver las consuma).
11. **Verificación con `ctest --preset backend`.** El README cita `ctest --preset full`, pero el
    frontend está fuera de alcance y el preset `backend` fija `BUILD_FRONTEND=OFF`: compila backend,
    datos, seed, consola y sus pruebas. `full` queda para el contenedor con la interfaz.

## 10. Verificación por tarea

Comandos comunes:

```bash
cmake --preset backend
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R '<patrón>'
ctest --preset backend
```

Todas las tareas están estimadas en **20–30 min**. Si alguna se estira, se para y se propone dividir la
spec, no se superan las 10 tareas.

- **T1 (~25 min):** `-R '^(test_preferencias|test_servicio_profesor)$'` verde; `errorDeFranja(-1)`,
  `(7)` y una franja con inicio ≥ fin devuelven mensaje con valor y acción; `haySolape` acepta
  08–10/10–12 y rechaza 08–10/09–11; el sentido inválido casteado se rechaza; `ServicioProfesor`
  compila usando la validación compartida.
- **T2 (~30 min):** `-R '^(test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos)$'`
  verde; `VERSION_ESQUEMA_ACTUAL == 5`; la tabla y sus columnas están en el catálogo; los `CHECK` de
  día/sentido/horas y el `UNIQUE` del duplicado exacto rechazan; `HUELLAS_CONOCIDAS` tiene la entrada 5;
  una base v4 falla al abrir; el test se llama `versionActual_esCinco` (antes `…Cuatro`) y el comentario
  «La base se crea con el esquema v4» de `test_servicio_configuracion_horaria.cpp:15` pasa a v5.
- **T3 (~30 min):** `-R '^test_servicio_preferencias$'` verde; alta/listado/actualización/borrado/
  limpieza; día, rango, sentido y duplicado exacto rechazados; solape opuesto rechazado con las dos
  franjas; solape/contiguo del mismo sentido aceptado; orden por día/inicio/fin/id; cambio de sentido
  conflictivo rechazado conservando el anterior; **cambio de día que crea un solape opuesto** rechazado;
  preferencia que coincide con disponibilidad aceptada; **cada rechazo contiene el valor recibido y la
  acción disponible** (docente, día, rango, sentido, duplicado, solape, no existe).
- **T4 (~30 min):** `-R '^(test_servicio_profesor|test_backend_nucleo_datos|test_servicio_preferencias)$'`
  verde; `ProfesorDTO` devuelve preferencias y disponibilidad separadas y ordenadas; `toProfesor()` no
  las copia; las cinco operaciones de `NucleoDatos` funcionan y un fallo queda como pendiente.
- **T5 (~25 min):** `-R '^(test_backend_cascada|test_servicio_profesor|test_backend_nucleo_datos|test_servicio_preferencias)$'`
  verde; `dependenciasDe("profesor", id)` lista disponibilidad y preferencias
  (`dependenciasDe("profesor").size()` pasa de 2 a **3** en `test_cascada.cpp:140`);
  `eliminarConCascada` y `eliminarDocente` borran ambas sin huérfanas; `eliminarProfesor` sigue
  cubierto por `test_servicio_profesor` (que no debe perder sus casos) y `eliminarDocente` por
  `test_backend_nucleo_datos`; un fallo deja todo.
- **T6 (~25 min):** `-R '^(test_seed_perfil|test_seed_aplicacion)$'` verde; `Recuentos.preferencias == 0`;
  la base sembrada tiene 0 filas; el resumen contiene «Preferencias: 0»; `kTablasDominio` la incluye y
  el comentario de `sembrador.cpp:34` dice 12 tablas / esquema v5.
- **T7 (~30 min):** `-R '^test_seed_aplicacion$'` verde; tras `--solo-docentes` y `--reemplazar` la tabla
  queda vacía sin fallar por FK; con una preferencia preinsertada y un fallo simulado, la preferencia y
  la variante anterior siguen completas; dos siembras coinciden también en `Preferencia_Profesor`; el
  comentario (`:347`) y la lista (`:350`) de «las 11 tablas» de `test_seed_aplicacion.cpp` pasan a 12
  tablas con `Preferencia_Profesor`.
- **T8 (~20 min):** `-R '^(test_servicio_generacion|test_servicio_preferencias)$'` verde;
  `grep -rn "preferencia" src/backend/src/services/ServicioGeneracion.cpp
  src/backend/include/backend/services/ServicioGeneracion.hpp` vacío; en `test_servicio_preferencias`,
  construir una instantánea, añadir una `PreferenciaDTO` a `ProfesorDTO::preferencias` y comprobar que
  `huellaDatos` **no cambia**; `ProfesorDTO::toProfesor()` no incluye preferencias; la condición futura
  queda anotada en el plan y en la guía (§8.1), no en el código.
- **T9 (~30 min):** el coordinator aplica §8 en `docs/interfaz-frontend.md` (constante y prosa de §2.2,
  dominios, bloque de `NucleoDatos`, tabla de DTO, **fila `profesor` de §2.4.1**, §2.4.5 y tabla §4) y en
  `docs/guias/datos-prueba.md` (fila de preferencias 0 y ejemplo de salida con `Hora académica: 35 min`
  y `Preferencias: 0`); `ctest --preset backend` **54/54**; `grep -n "VERSION_ESQUEMA_ACTUAL = 5"
  docs/interfaz-frontend.md` devuelve una línea; `grep -n "profesor.*preferencias" docs/interfaz-frontend.md`
  devuelve la fila de §2.4.1; `grep -n "2.4.5" docs/interfaz-frontend.md` devuelve la sección;
  `grep -n "Preferencias (no se siembran)" docs/guias/datos-prueba.md` devuelve la fila;
  `git diff --stat` acotado a los archivos de este plan.

## 11. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|----|----------|--------------|
| RF-1 | T1, T2, T3, T4 | T1: modelo puro; T2: tabla/columnas; T3: persistencia y orden; T4: DTO del docente |
| RF-2 | T1, T3 | T1: día/rango/sentido y solape puros; T3: docente, duplicado, solape opuesto y cambio de sentido |
| RF-3 | T3, T4 | T3: CRUD y limpieza con `Resultado`; T4: preferencias junto a disponibilidad y fachada |
| RF-4 | T5 | Cascada y `eliminarProfesor` borran ambas; `dependientesDe` las lista |
| RF-5 | T8 | Huella intacta y `toProfesor()` sin preferencias; grep en `ServicioGeneracion` |
| RF-6 | T2 | v5, catálogo, `CHECK`/`UNIQUE`, huella v5 y v4 rechazada sin migrar |
| RF-7 | T6, T7 | Recuento 0, `kTablasDominio`, orden de borrado, atomicidad y determinismo |
| RF-8 | T9 | Guía con modelo, día 0–6, validaciones, distinción y sin efecto; guía del seed |
| RNF-1 | T1, T3 | Mensajes con campo, valor, causa y acción |
| RNF-2 | T2 | v4 falla al abrir; sin migración ni borrado |
| RNF-3 | T7 | Dos siembras dejan 0 y lo mismo; orden estable |
| RNF-4 | T5, T7 | Sin huérfanas; transacciones en baja y conmutación |
| RNF-5 | T9 | `ctest --preset backend` 54/54 sin frontend |
| RNF-6 | T1–T9 | Cada RF con tarea y verificación en esta tabla |

## 12. Riesgos y supuestos

- **Bases v4 dejan de abrir.** Política de la 009 (RF-6): no hay migración. El usuario debe crear una
  base nueva con respaldo o restaurar; el diálogo de apertura ya lo ofrece.
- **Las preferencias no cambian la generación todavía.** Es deliberado (RF-5): no entran en la huella ni
  en `DatosDominio`; un cambio no marca «datos anteriores». El usuario no debe esperar efecto en los
  horarios hasta la spec del solver.
- **La conmutación de variante borra las preferencias.** Es una operación destructiva (con `--si` y
  respaldo); el usuario pierde las preferencias de la variante anterior. La spec lo decide así para no
  dejar huérfanas.
- **Dos caminos de baja.** `eliminarDocente` y `eliminarConCascada` deben borrar preferencias; si se
  olvida uno, SQLite rechaza por FK. T5 cubre ambos.
- **Numeración de días.** 0 = domingo … 6 = sábado, igual que `FranjaHoraria` y la disponibilidad; no
  hay conversión. La generación actual usa lunes–viernes (días 1–5); sábado/domingo se guardan sin
  efecto.
- **N+1 al listar docentes.** Cargar preferencias por docente añade una consulta por docente, como ya
  ocurre con la disponibilidad; con 65 docentes es aceptable. Si creciera, se resolvería con un JOIN,
  fuera de esta spec.
- **`dia` TEXT.** El orden y el `CHECK` funcionan con 0–6 de un dígito; el servicio escribe siempre la
  forma canónica. Una escritura directa con otro formato la rechaza el `CHECK` o el servicio.
- **El refactor de `validarFranja`** cambia el texto de los rechazos de disponibilidad (incluye el valor
  recibido); no cambia la aceptación/rechazo. `test_servicio_profesor` se mantiene verde.
- **Frontend:** el DTO gana un campo aditivo y `NucleoDatos` gana métodos; no hay rotura de firma. La
  guía lo declara.

## 13. Restricciones de la spec (no se tocan)

- El solver, el contrato de configuración, la generación y la huella.
- El tope de 40 horas académicas y la duración de la hora académica (011).
- La interfaz de usuario (spec 001); solo se entregan servicios y guía.
- La migración de bases y la fusión de solapes del mismo sentido.
- La siembra de preferencias de ejemplo: el conjunto queda vacío.
