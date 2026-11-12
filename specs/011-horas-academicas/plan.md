# Plan 011 - Duración de la hora académica

Base: 8e0602c

> **Alcance.** Este plan implementa la spec 011 (9 RF) sobre `8e0602c`, que ya contiene la **009**
> cerrada (esquema v3 con `TiposAula`/`Eventos`, catálogo y huella de esquema, servicios, cascada,
> huella de datos, seed y guía de API) y la **010** cerrada (contrato 2.0 con `duracion_minutos` en V2
> solo `> 0`, eventos del turno y capacidad por tipo). La entrega es **backend, datos, seed y consola**
> (`BUILD_FRONTEND=OFF`, preset `backend`); la interfaz queda fuera y **no compila** contra las firmas
> nuevas (RF-9 lo documenta como deuda). No hay migración: la base sube a **esquema v4** y una base v3
> se rechaza al abrir.
>
> **Punto de partida verificado.** `8e0602c` es HEAD de `feat/Base-datos` con el árbol limpio y la
> suite del preset `backend` en **50/50**. Hoy la duración vive en tres sitios incoherentes: el preset
> (45 por defecto en el frontend), el seed (8 franjas de 40 min) y el reparto de la ventana en
> `calculo_eventos::duracionFranjaEstimada`. Este plan deja **un solo** valor (configuración global),
> **una sola** conversión horas→minutos y **una sola** definición de franja (= D).
>
> **Lo que consume la 011 ya existe:** `ContextoBaseDatos`/`DatabaseManager` crean el esquema único;
> `VersionEsquema::crearEsquema` fija la marca; `NucleoDatos` es la fachada que usa el frontend; el
> constructor `construirJsonEntrada(datos, preset, ahora)` es puro y no lee la base; el seed siembra
> turnos a través de `NucleoDatos`.

## 1. Arquitectura y archivos

### 1.1 Punto único de duración (RF-1, RF-3, RNF-4)

- **Nuevo** `src/backend/include/backend/services/duracion_horaria.hpp` y
  `src/backend/src/services/duracion_horaria.cpp`. Namespace `duracion_horaria`, funciones puras y
  constantes:
  - `POR_DEFECTO = 35`, `MINIMA = 30`, `MAXIMA = 90`.
  - `QString errorDeRango(int duracionMinutos)`: **el** punto único de validación del rango; devuelve
    el mensaje en español o cadena vacía.
  - `int minutosDeHorasAcademicas(int horasAcademicas, int duracionMinutos)`: **el** punto único de
    horas académicas → minutos (`horas × D`). Ninguna otra pieza hace **esa conversión**; multiplicar
    por D en el encaje (`franjas × D`) o en el ancho de la franja no es una conversión horas→minutos.
  - `int franjasUtilizables(int ventanaMinutos, int tiempoEventos, int duracionMinutos)`:
    `floor((ventana − tiempoEventos) / D)`; `0` si D o la ventana no son positivos.
  - `int deficitDeEncaje(int numSlots, int ventanaMinutos, int tiempoEventos, int duracionMinutos)`:
    `max(0, numSlots × D + tiempoEventos − ventana)`.
- **Consumidores:** `solver_config.cpp` (V2 del contrato), `calculo_eventos.cpp` (encaje y ventana de
  posición), `ServicioConfiguracionHoraria`, `ServicioTurnosEventos`, `perfil.cpp` (seed) y
  `ConstructorEntradaSolver.cpp`. Las pruebas nuevas `test_duracion_horaria` y las de contrato lo
  ejercitan directamente.
- **Sin consumidor de negocio hoy para `minutosDeHorasAcademicas`**: el backend no presenta minutos
  fuera de la rejilla; el punto existe para que la deuda del frontend no reimplemente `× D` y se cubre
  con los casos límite de la spec (40 h × 35 = 1400; 40 h × 30 = 1200). No se inventa una pantalla ni
  un comando de consola para darle uso.

### 1.2 Configuración global y servicios (RF-1, RF-2)

- **Nuevo** `src/backend/include/backend/services/ServicioConfiguracionHoraria.hpp` y
  `src/backend/src/services/ServicioConfiguracionHoraria.cpp`:
  - `struct ConfiguracionHoraria { int duracionMinutos = 0; bool porDefecto = false; };`
  - `Resultado<ConfiguracionHoraria> consultar() const`: valor efectivo (fila guardada o 35 con
    `porDefecto = true` si no hay fila); **no falla** por ausencia de fila.
  - `Resultado<ConfiguracionHoraria> guardar(int duracionMinutos)`: valida con
    `duracion_horaria::errorDeRango` y hace *upsert* de la fila `id = 1`; si la validación falla,
    **no escribe** y conserva el valor anterior.
- `src/backend/include/backend/services/NucleoDatos.hpp` / `src/backend/src/services/NucleoDatos.cpp`:
  fachada `configuracionHoraria()` y `guardarConfiguracionHoraria(int)`, que delegan en un miembro
  `ServicioConfiguracionHoraria m_configuracion` (inicializado en el constructor, como los demás
  servicios). Son la vía del frontend y del seed; **sin** `GestorPendientes` (la configuración es
  global, no un registro de dominio reintentable).
- **Regla «solo el servicio escribe»:** la única escritura fuera del servicio es la fila por defecto
  que inserta la creación del esquema (§3), que no es una edición de usuario. Se verifica con `grep` en
  T3.

### 1.3 Esquema y versión (RF-7, RF-2)

- `src/backend/src/database/migracion.cpp`: tabla `ConfiguracionHoraria` (DDL en §3) y su fila por
  defecto `(1, 35)`.
- `src/backend/include/backend/database/version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 4` y comentario
  de la v4.
- `src/backend/src/database/version_esquema.cpp`: `ConfiguracionHoraria` en `TABLAS_ESPERADAS` y
  `COLUMNAS_CONFIGURACION = {id, duracion_minutos}` en `TABLAS_CON_COLUMNAS`.
- `test/backend/test_version_esquema.cpp`: versión 4, tabla/columnas, fila por defecto, rechazo de una
  segunda fila y huella v4 en `HUELLAS_CONOCIDAS` **en el mismo cambio**.

### 1.4 Franja de clase y encaje del turno (RF-4)

> **Reparto en dos pasos, para que ninguna tarea cierre con el árbol sin compilar.** T4 **añade** las
> funciones nuevas y **conserva** temporalmente `duracionFranjaEstimada` y las firmas actuales de
> `ventanaDePosicion`/`eventoDentroDeVentana` como envoltorios finos de las nuevas (sin duplicar la
> aritmética). T5, T6 y T8 migran sus llamadores a las firmas con D; **T9** borra el legado y deja el
> `grep` de cierre vacío.

- `src/backend/include/backend/services/calculo_eventos.hpp` / `.cpp` (T4):
  - Se añaden `franjasUtilizables(turno, D)`, `deficitDeEncaje(turno, D)` y `errorDeEncaje(turno, D)`
    (mensaje único con turno, D y déficit) y las **sobrecargas con D** de
    `ventanaDePosicion(turno, posicion, D)` y `eventoDentroDeVentana(evento, turno, posicion, D)`; la
    ventana de la posición *k* usa **D** como ancho y la última llega a `turno.fin` (definición N5 de la
    009).
  - Se hacen públicos `minutosDeVentana(turno)` y `tiempoDeEventos(turno)` (hoy en un namespace
    anónimo).
  - `duracionFranjaEstimada` conserva su fórmula de reparto y las firmas antiguas de
    `ventanaDePosicion`/`eventoDentroDeVentana` quedan como **legado temporal** que delega en las nuevas
    con `D = duracionFranjaEstimada(turno)`; su comentario se actualiza para decir que se retiran en T9
    (`calculo_eventos.hpp:34-36`, `calculo_eventos.cpp:59-63`). **No** se añaden llamadores nuevos al
    legado.
- `src/backend/src/services/ServicioTurnosEventos.cpp` (T5): lee la D efectiva con
  `ServicioConfiguracionHoraria(m_db).consultar()` y valida el encaje en el alta y la actualización del
  turno **y** en el alta/edición de eventos (un evento puede romper la invariante). Migra sus seis
  llamadas (`:114, :122, :124, :315, :330, :334`) a las firmas con D. El mensaje es el de
  `errorDeEncaje`.
- `src/backend/src/solver/config/solver_config.cpp` (T6): migra `:516, :523, :526` a las firmas con D y
  actualiza el comentario de `:424-425`.
- `src/seed/src/perfil.cpp` (T8): valida los turnos del perfil con `POR_DEFECTO` (el seed declara turnos
  válidos con D = 35) y con `errorDeEncaje`; migra `:556, :558` y actualiza el comentario de `:282-285`.
- **T9 (limpieza):** se borran `duracionFranjaEstimada` y las dos sobrecargas antiguas; los casos de
  `test_calculo_eventos` con la firma vieja (2 args de `ventanaDePosicion`, 3 de
  `eventoDentroDeVentana`, líneas 111-203) se migran a D o se retiran si duplican los de T4; el build
  prueba que no queda ninguna llamada vieja y el `grep` de cierre se revisa a mano (ver §9).

### 1.5 Contrato 2.0 (RF-5, RF-1)

- `src/backend/src/solver/config/solver_config.cpp`:
  - V2 (`validarTiposYRangos`) rechaza la duración con `duracion_horaria::errorDeRango` (30–90) en vez
    de `> 0`.
  - V1 (`validarEstructura`) rechaza una duración **no entera** (p. ej. `35.5`) citando el valor
    recibido, antes de que el parseo la convierta en `0`.
  - `validarEventos` valida el encaje de **todos** los turnos con ventana (hoy solo si tienen eventos),
    con `calculo_eventos::errorDeEncaje`; se mantienen las comprobaciones de ventana de posición,
    cronología y solape.
- No cambia la forma del contrato, ni la versión 2.0, ni el modelo del solver.

### 1.6 Constructor de la entrada (RF-5, RF-3)

- `src/backend/include/backend/services/ConstructorEntradaSolver.hpp` / `.cpp`:
  - Nueva firma **obligatoria** `construirJsonEntrada(datos, parametros, duracionMinutos, ahora)`
    (también en la fachada `ConstructorEntradaSolver::construir`). No se deja sobrecarga con la firma
    antigua: quien no pase D no compila.
  - `salida.franja_horaria = parametros.franja_horaria;` y **sobrescritura incondicional**
    `salida.franja_horaria.duracion_minutos = duracionMinutos;`.
  - La rejilla de disponibilidad (`slotsDeFranja`) usa **`duracionMinutos`**, no el valor del preset.
  - Tras la sobrescritura se valida la duración resultante con `duracion_horaria::errorDeRango`; si no
    es válida, la construcción falla sin emitir JSON.
  - Sigue siendo pura: **no** lee la base; D llega como parámetro.

### 1.7 Huella de datos (RF-6)

- `src/backend/include/backend/services/ServicioGeneracion.hpp` / `.cpp`: `huellaDatos(const
  DatosDominio&, int duracionMinutos)` incluye D en el resumen canónico, de forma determinista.
- `ServicioGeneracion` no cambia de forma: el callback `huellaActual` que recibe el llamador debe
  construir la huella con la D efectiva; un cambio de D entre el envío y la respuesta marca «datos
  anteriores» sin código nuevo. **Hallazgo:** hoy `schedule_generation_widget.cpp` construye el
  servicio con un callback vacío (`{}`), así que el marcado no está cableado en la interfaz; la 011
  deja la función con D y lo documenta como deuda del frontend (§7), no lo cablea.

### 1.8 Seed (RF-8)

- `src/seed/src/perfil.cpp`: turnos de 8 franjas con D = 35 y ventanas ajustadas a la invariante
  (07:00–11:50 y 12:40–17:30), evento «Receso» en 08:10–08:20 y 13:50–14:00; validación con la nueva
  ventana y el encaje.
- `src/seed/include/seed/sembrador.hpp` / `src/seed/src/sembrador.cpp`:
  - `ResultadoAplicacion` gana `int duracionMinutos` y `int configuraciones`.
  - La siembra asegura la configuración en 35 dentro de la transacción y **antes** de crear turnos,
    con `NucleoDatos::guardarConfiguracionHoraria(35)` (idempotente, una fila).
  - `leerConfiguracionSembrada` lee valor y recuento de `ConfiguracionHoraria` (como los recuentos).
  - `ConfiguracionHoraria` **no** entra en `kTablasDominio`: no bloquea una base recién creada.
  - `resumenTexto` imprime `Hora académica: 35 min (configuraciones: 1)`.
- `src/seed/src/main.cpp`: pasa los dos valores nuevos al informe.

### 1.9 Documentación (RF-9, la aplica el coordinator)

- `docs/interfaz-frontend.md` (guía de API), `docs/guias/datos-prueba.md` y
  `docs/guias/datos-prueba-frontend.md` (inventario y guía del seed) y `docs/contrato-solver-2.0.md`
  (rango de V2 y ejemplos de turnos). Contenido exacto en §7.

### 1.10 Pruebas

- **Nuevas:** `test/backend/test_duracion_horaria.cpp` y
  `test/backend/test_servicio_configuracion_horaria.cpp`, registradas en `test/CMakeLists.txt` con
  `add_qtest(... backend)` y `add_qtest(... backend Qt6::Sql)`.
- **Reescritas (no ajustadas a ciegas):**
  - `test_calculo_eventos.cpp`: **T4 añade** los casos de D, franjas utilizables, déficit y ventana de
    posición; los casos de `duracionFranjaEstimada` se conservan (la función sigue) hasta que **T9** los
    retira junto con ella.
  - `test_constructor_entrada.cpp`: firma con D; dominio y expectativas a 35 (evento 07:35–07:45;
    disponibilidad de 07:00–09:00 → 3 franjas, no 2); se añaden sobrescritura y rango.
  - `test_validador_entrada.cpp`: `jsonValido()` construye con D = 35; V2 rechaza 1, 5, 29 y 91 y
    acepta 30 y 90; rechaza 35.5; se añade el encaje. Los horarios de los eventos de las pruebas de
    ventana y cronología (hoy 08:36–08:46, calculados con el reparto de 48 min) se **recalculan** con la
    ventana de 35 (08:10–08:20, 08:10–08:22…) porque si no cambian de significado.
  - `test_servicio_turnos_eventos.cpp`: mensajes de encaje con turno/D/déficit y comentarios con D.
  - `test_servicio_generacion.cpp`: D en `construirJsonEntrada` y en `huellaDatos`; caso de cambio de
    D → huella distinta → «datos anteriores».
  - `test_seed_perfil.cpp` y `test_seed_aplicacion.cpp`: turnos, eventos, configuración y resultado.
  - `test_version_esquema.cpp`: v4, tabla, fila por defecto y huella.
  - `test/test_ipc_aislamiento_bd.cpp`: pasa D a la construcción.
- **No se tocan:** `test/frontend/*` (fuera del preset `backend`), `test/data/preset-1.0.json`
  (rechazo por versión) ni los tests que no hablan de duración.

## 2. Funciones puras y pseudocódigo

### 2.1 `duracion_horaria` (sin E/S, deterministas, sin reloj)

```
errorDeRango(D):
    si D < 30 o D > 90:
        "La duración de la hora académica debe ser un entero entre 30 y 90 minutos; se recibió
         <D>. Ajuste la duración a un valor del rango (30–90 minutos)."
    si no: ""

minutosDeHorasAcademicas(horas, D):
    horas < 0 -> 0
    D no válida -> 0
    horas * D

franjasUtilizables(ventana, eventos, D):
    si D <= 0 o ventana <= 0: 0
    max(0, ventana - max(0, eventos)) / D      // división entera

deficitDeEncaje(numSlots, ventana, eventos, D):
    si D <= 0 o ventana <= 0: 0
    max(0, numSlots * D + max(0, eventos) - ventana)
```

### 2.2 `calculo_eventos`

```
minutosDeVentana(turno): turno.inicio.secsTo(turno.fin)/60  (0 si horas inválidas)
tiempoDeEventos(turno):   Σ max(0, evento.duracion)

franjasUtilizables(turno, D) = duracion_horaria::franjasUtilizables(
                                   minutosDeVentana(turno), tiempoDeEventos(turno), D)

errorDeEncaje(turno, D):
    si errorDeRango(D) no vacío -> ese mensaje
    si minutosDeVentana(turno) <= 0 ->
        "El turno «X» no declara una ventana válida de inicio y fin. Declare sus horas (HH:mm)
         para poder comprobar el encaje con la hora académica."
    util = franjasUtilizables(turno, D)
    si turno.numSlots <= util -> ""
    deficit = deficitDeEncaje(turno.numSlots, ventana, eventos, D)
    "El turno «X» no encaja con una hora académica de D min: N franjas × D min + E min de
     eventos = T min y su ventana es de V min (déficit F min). Ajuste el número de franjas,
     la ventana o la duración de la hora académica."

ventanaDePosicion(turno, k, D):
    si k fuera de 1..numSlots o D <= 0 o ventana <= 0 -> (QTime(), QTime())
    inicio = turno.inicio + (k*D + Σ duraciones de eventos con despuesDeSlot < k) minutos
    fin    = (k == numSlots) ? turno.fin : inicio + Σ duraciones de eventos en k
    (inicio, fin)

eventoDentroDeVentana(evento, turno, k, D):
    sin horas -> true
    ventana = ventanaDePosicion(turno, k, D)
    ventana válida y ventana.first <= evento.inicio y evento.fin <= ventana.second
```

Mientras dure la transición (T4–T8), las firmas antiguas de `ventanaDePosicion`/`eventoDentroDeVentana`
delegan en las nuevas con `D = duracionFranjaEstimada(turno)`; `duracionFranjaEstimada` conserva su
fórmula de reparto hasta T9. Así el comportamiento previo no cambia y no hay dos aritméticas.

### 2.3 Servicio de configuración

```
consultar():
    fila = SELECT duracion_minutos FROM ConfiguracionHoraria WHERE id = 1
    error de SQL -> Resultado::error("No se pudo consultar la duración de la hora académica.
                     Compruebe que la base esté disponible e intente de nuevo.")
    sin fila     -> {35, porDefecto = true}
    con fila     -> {valor, porDefecto = false}

guardar(D):
    e = duracion_horaria::errorDeRango(D); si e no vacío -> error(e)  // no escribe
    INSERT INTO ConfiguracionHoraria (id, duracion_minutos) VALUES (1, :D)
      ON CONFLICT(id) DO UPDATE SET duracion_minutos = excluded.duracion_minutos,
                                    fecha_modificacion = CURRENT_TIMESTAMP
    error de SQL -> error("No se pudo guardar la duración de la hora académica; intente de nuevo.")
    -> consultar()
```

`guardar` no abre transacción: es una sola sentencia y debe participar de la transacción del
llamador (el seed la usa dentro de su transacción única).

### 2.4 Constructor (orden obligatorio)

```
construirJsonEntrada(datos, preset, D, ahora):
    1. tipos obligatorios de aulas y materias (sin cambios)
    2. salida = …; salida.franja_horaria = preset.franja_horaria
    3. salida.franja_horaria.duracion_minutos = D          // sobrescritura SIEMPRE
    4. e = errorDeRango(salida.franja_horaria.duracion_minutos); si e no vacío -> error
    5. slotsDeFranja(..., D)                               // la rejilla usa D, no el preset
    6. resto de la construcción y JSON 2.0
```

## 3. Esquema v4

DDL en `migracion.cpp`, antes de `Cursos`:

```sql
CREATE TABLE IF NOT EXISTS ConfiguracionHoraria (
  id INTEGER PRIMARY KEY CHECK(id = 1),
  duracion_minutos INTEGER NOT NULL CHECK(duracion_minutos BETWEEN 30 AND 90),
  fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
INSERT OR IGNORE INTO ConfiguracionHoraria (id, duracion_minutos) VALUES (1, 35);
```

- `CHECK(id = 1)` hace imposible una segunda configuración (RF-1); el `CHECK` del rango es la última
  barrera para escrituras directas (la validación de negocio es la de `duracion_horaria`).
- La fila por defecto es parte de la **creación del esquema** (RF-2); `consultar()` sigue devolviendo
  35 con `porDefecto` si alguien la borra.
- `VERSION_ESQUEMA_ACTUAL = 4`; catálogo y `HUELLAS_CONOCIDAS` se actualizan en el mismo cambio. La
  huella real se copia del fallo del test (paso 3 de su comentario); no se inventa.

## 4. Servicio de configuración: interfaz

```cpp
struct ConfiguracionHoraria { int duracionMinutos = 0; bool porDefecto = false; };

class ServicioConfiguracionHoraria {
public:
    explicit ServicioConfiguracionHoraria(QSqlDatabase& db);
    Resultado<ConfiguracionHoraria> consultar() const;   // valor efectivo, nunca falla por no haber fila
    Resultado<ConfiguracionHoraria> guardar(int duracionMinutos); // valida 30–90 y no toca el previo si falla
};
```

Y en `NucleoDatos`: `configuracionHoraria()` / `guardarConfiguracionHoraria(int)`.

## 5. Contrato y construcción

- **V1:** `franja_horaria.duracion_minutos`, si está, debe ser un número entero; `35.5` se rechaza
  citando `35.5` (RNF-1). No se introduce ninguna regla nueva sobre el resto del contrato.
- **V2:** el rango 30–90 sale de `duracion_horaria::errorDeRango` (mismo punto que la configuración).
- **V2 (turnos):** `errorDeEncaje(turnoDto, D)` para **todo** turno con `inicio`/`fin` válidos, tenga o
  no eventos. Un turno sin ventana no se puede comprobar (el contrato lo permite; el dominio siempre la
  trae).
- **Dos caminos distintos, a propósito:**
  - Un **preset en memoria** con 0, 5, 45 o sin campo que llega al constructor **no es error**: se
    sobrescribe con D (caso límite de la spec).
  - Un **contrato JSON que llega sin construcción** (cargador de preset, IPC, generación) con 5 se
    rechaza en V2 (RF-5).
- La construcción no valida el resto del contrato: la generación lo hace en `ServicioGeneracion::generar`
  → `validarEntradaSolver` (V1–V18), incluido el encaje.

## 6. Seed

- Turnos del perfil (sustituyen el inventario de 40 min de la 008):
  - Mañana 07:00–11:50, 8 franjas, «Receso» después de la franja 2 en 08:10–08:20.
  - Tarde 12:40–17:30, 8 franjas, «Receso» después de la franja 2 en 13:50–14:00.
  - Comprobación: `8 × 35 + 10 = 290` = ventana; sobrante 0. La **rejilla aritmética de clases** va de
    07:00 a **11:40** y de 12:40 a **17:20** (`8 × 35 = 280`); el `fin` **11:50/17:30** añade los 10 min
    del evento «Receso», el mismo desfase de 10 min que documenta la 008 (12:20 → 12:30).
- La siembra asegura `ConfiguracionHoraria(1, 35)` antes de crear turnos, dentro de la transacción; si
  el usuario la había cambiado, la deja en 35 (RF-8). El reemplazo total no borra esa tabla.
- `ResultadoAplicacion` informa `duracionMinutos = 35` y `configuraciones = 1`; el resumen lo imprime.
- `--solo-docentes` también asegura la configuración y no la duplica.
- Determinismo: dos siembras del mismo perfil dejan el mismo valor, el mismo recuento y los mismos
  turnos.

## 7. Documentación que aplica el coordinator (RF-9)

### 7.1 Contenido exacto por archivo

**`docs/interfaz-frontend.md`**

1. **§2.2, línea 106:** `constexpr int VERSION_ESQUEMA_ACTUAL = 3;` pasa a **`= 4`** y el comentario a
   «v4: configuración global de la hora académica (una fila)». La vecina de línea 124 no cambia.
2. **§2.4.2, línea 343:** sustituir el apartado «Duración de la franja» por: la franja dura **D**
   (global, 35 por defecto), la rejilla no reparte la ventana, el encaje es
   `numSlots × D + Σ eventos ≤ ventana` y el rechazo nombra turno, D y déficit (`errorDeEncaje`);
   `duracionFranjaEstimada` ya no existe.
3. Añadir §2.4.4 «Duración de la hora académica (`ServicioConfiguracionHoraria`)» con: DTO
   `ConfiguracionHoraria`, `consultar()` (valor efectivo y `porDefecto`), `guardar()` (30–90; inválida
   no cambia el anterior), la fachada `NucleoDatos::configuracionHoraria`/
   `guardarConfiguracionHoraria`, `duracion_horaria::minutosDeHorasAcademicas` como único punto horas →
   minutos, y que el tope de 40 horas académicas no cambia.
4. **§3.6, línea 682:** el campo `duracion_minutos` del preset es **derivado**; `construirJsonEntrada`
   lo sobrescribe siempre y un cambio de configuración desde que se guardó el preset no es error ni
   aviso. Donde dice «40 min en los datos de prueba del centro», pasa a **35 min** y se marca como deuda
   (la vista no se toca).
5. **§4:** filas para `ConfiguracionHoraria`, `errorDeEncaje` y la firma nueva del constructor.
6. **§5, punto 4, línea 758:** la deuda de la 009 decía «usar la suma de `duracion` (o directamente
   `duracionFranjaEstimada(turno)`)»; pasa a las funciones nuevas con D (`errorDeEncaje`, ventana de
   posición con D) y a `NucleoDatos::configuracionHoraria()` para la D efectiva.
7. Nueva **§6** «Deuda del frontend y alcance (spec 011)»:
   - `construirJsonEntrada(datos, preset, duracionMinutos, ahora)` **rompe** la compilación de
     `src/frontend`; el builder debe leer D con `NucleoDatos::configuracionHoraria()`.
   - `huellaDatos(datos, duracionMinutos)` cambia de firma y la vista debe cablear `huellaActual`
     (hoy pasa `{}`): sin eso, «datos anteriores» no se marca aunque D cambie.
   - El selector de duración del editor de presets y de la vista de generación **deja de ofrecerse**.
   - `schedule_generation_widget.cpp` sigue derivando la duración y haciendo aritmética en minutos;
     `horasAula` sigue en minutos: deuda de su equipo.
   - `test/frontend/test_generacion_seed.cpp` (fija 40) queda fuera del preset `backend` y se reescribe
     al migrar el frontend.

**`docs/guias/datos-prueba.md`**

- Línea 16, turnos: `Mañana 07:00–11:50 · Tarde 12:40–17:30, 8 franjas de 35 min`.
- Línea 17, eventos: `Receso de 10 min tras la 2.ª franja: 08:10–08:20 y 13:50–14:00`.
- **Línea 151**, viñeta de duración: «Los **40 minutos** son la duración real…» pasa a **35 minutos**,
  con el encaje `8 × 35 + 10 = 290`; la siembra asegura la configuración global en 35 (una fila, no es
  dato de dominio).

**`docs/guias/datos-prueba-frontend.md`**

- **Líneas 58-59**, tabla de entidades: turnos a `Mañana 07:00–11:50 · Tarde 12:40–17:30, 8 franjas de
  35 min` y eventos a `08:10–08:20 (Mañana) y 13:50–14:00 (Tarde)`.
- **Líneas 72-74**, «Turnos (rejilla aritmética)»: pasos de **35 min**; la **rejilla de clases** va de
  07:00 a **11:40** y de 12:40 a **17:20** (`8 × 35 = 280`); `Turnos.fin` = **11:50/17:30**, que añade
  los **10 min del evento «Receso»**: el desfase sigue existiendo y es el tiempo del evento, el mismo
  que documenta la 008. **No** se escribe que el `fin` coincida con la rejilla ni se repiten las horas
  viejas.
- **Línea 30**, tiempo real: `40 bloques × 35 min = 1400 min = **23 h 20 min**` (no 1600 / 26 h 40), y
  solo para presentación, con `duracion_horaria::minutosDeHorasAcademicas`.
- **Línea 34**: `duracion_minutos` ya **no** es solo del preset: es la configuración global persistida
  (`ConfiguracionHoraria`, una fila, 35 por defecto, rango 30–90); el campo del preset es derivado y se
  sobrescribe.
- **Línea 200**, checklist: «(y 26 h 40 min solo como dato de tiempo real)» pasa a «23 h 20 min».
- **Línea 203**, checklist: «(35 min por bloque, o el valor efectivo de la configuración)» en lugar de
  «40 min»; matizar «No descuentes su duración de las franjas útiles»: el solver ignora los eventos,
  pero el **encaje** del turno es `slots × D + Σ eventos ≤ ventana`.

**`docs/guias/consola-datos.md`**

- **Líneas 159-160:** el ejemplo crea el turno `manana` con **07:00–11:50** y añade el receso en
  **08:10–08:20** (antes 12:30 y 08:20–08:30, que dejaban el ejemplo inválido con D = 35 porque la
  ventana de la posición 2 es 08:10–08:20).

**`docs/contrato-solver-2.0.md`**

- §2.2: `duracion_minutos` es un entero **30–90** (V2) y, en la construcción, sale de la configuración
  global (el valor del preset se sobrescribe); añadir la invariante `slots × D + Σ eventos ≤ ventana`.
  El ejemplo pasa a `"franja_horaria": { "duracion_minutos": 35, "slots_por_turno": 8 }`.
- §2.3: ejemplo coherente con el seed:
  `{"nombre":"Mañana","slot":[0,…,7],"inicio":"07:00","fin":"11:50"}`,
  `{"nombre":"Tarde","slot":[8,…,15],"inicio":"12:40","fin":"17:30"}`. Comprobación: `8 × 35 = 280`.
- §4: el ejemplo del evento «Receso» pasa a `"duracion": 10, "inicio": "08:10", "fin": "08:20"` para
  encajar en el turno de §2.3 (`8 × 35 + 10 = 290`); el resto de la tabla de campos no cambia.
- El ejemplo completo de §7 (45 min, 6 franjas, ventana 07:00–12:00, evento 30 min) ya cumple
  `6 × 45 + 30 = 300`; se deja como está.

### 7.2 Anotación de supersesiones en los specs históricos (las aplicó el planner, no el implementer)

Solo el planner edita `spec.md`; por eso esto **no** es una tarea de `tasks.md` ni las aplica el
implementer. Se hizo al cerrar la fase, como la 010 con la 005, y **no** basta con el preámbulo de la
011: el precedente de la 010 dejó notas inline en los textos históricos. **Autoría:** las aplicó el
planner; viajaron en el commit de T10 (`059815b`) porque el coordinator aplicó los parches de `docs/`
en el mismo cierre. Inserciones exactas, todas con fecha **2026-10-29**:

- **`specs/009-tipos-de-aula-y-eventos/spec.md`**:
  - Definición «Duración estimada de la franja de clase» (nota en **143**) y **RF-3** (nota en **233**):
    > **Superada por la 011 (2026-10-29).** La franja de clase dura la hora académica (D, 35 por
    > defecto, rango 30–90); la ventana del turno no se reparte y el sobrante no se usa.
  - **RF-11** (nota en **375**):
    > **Superada en la fórmula por la 011 (2026-10-29).** La guía documenta D y el encaje
    > `slots × D + Σ eventos ≤ ventana`, no el reparto de la ventana.
- **`specs/008-datos-de-prueba/spec.md`**:
  - **RF-4** (nota en **272**), **criterio 1** (nota en **550**) y **criterio 3** (nota en **564**):
    > **Superado por la 011 (2026-10-29).** El inventario de 8 franjas de 40 min y la rejilla
    > `8 × 40 + 10` se sustituyen por 8 franjas de D = 35: Mañana 07:00–11:50 y Tarde 12:40–17:30,
    > recesos 08:10–08:20 y 13:50–14:00 (`8 × 35 + 10 = 290`). La rejilla de clases termina en 11:40 y
    > 17:20; el `fin` añade el evento de 10 min (mismo desfase que la 008).
- **`specs/005-nucleo-solver/spec.md`**:
  - Nota existente de **V2** (**616-618**): se quitó «pendiente de la 011 futura» y quedó así:
    > **Estrechada por la 011 (2026-10-29).** V2 exige `duracion_minutos` entero entre 30 y 90; la
    > procedencia es la configuración global y V2 valida además el encaje
    > `slots × D + Σ eventos ≤ ventana`.
  - Definición de `duracion_minutos` (nota en **188**) y caso límite (nota en **728**):
    > **Superada en el rango por la 011 (2026-10-29).** Ya no basta `> 0`: fuera de 30–90 se rechaza.
- **`specs/010-solver-tipos-y-eventos/spec.md`**:
  - **V2** (nota en **298**):
    > **Ampliada por la 011 (2026-10-29).** El rango de `duracion_minutos` es 30–90 y V2 valida el
    > encaje del turno; el resto de V2 no cambia.
- **Verificación de cierre (planner):** `grep -n "011 (2026-10-29)" specs/009-*/spec.md
  specs/008-*/spec.md specs/005-*/spec.md specs/010-*/spec.md` devuelve 10 notas (009: 3; 008: 3;
  005: 3; 010: 1).

## 8. Decisiones (con la alternativa descartada)

1. **Un módulo puro `duracion_horaria` para rango, minutos y encaje entero.** Descartado: mantener el
   `> 0` literal en V2 y repetir 30–90 en el servicio (dos fuentes; rompe RNF-4).
2. **D entra al constructor como parámetro obligatorio y se sobrescribe siempre.** Descartado: que el
   constructor lea la configuración (rompe la pureza y «el solver no lee la base») y conservar una
   sobrecarga sin D (permite olvidar la sobrescritura).
3. **Tabla `ConfiguracionHoraria` con `CHECK(id = 1)` y fila por defecto en la creación.** Descartado:
   una tabla clave/valor genérica (no expresa «una sola» ni el rango) y no poner `CHECK` (una escritura
   directa podría guardar 5).
4. **`ServicioTurnosEventos` lee la D efectiva del servicio de configuración.** Descartado: inyectar D
   en su constructor (cada llamada —seed, consola, tests— tendría que recordarla y podrían desincronizarse)
   y validar solo en la generación (se almacenarían turnos inválidos).
5. **El alta/edición de eventos también valida el encaje.** Descartado: validar solo el turno (un evento
   grande dejaría el turno inválido sin aviso). Es consecuencia de la invariante de RF-4, no un requisito
   nuevo.
6. **`duracionFranjaEstimada` se retira en T9, no en T4.** Descartado: borrarla en T4 (T5, T6 y T8 aún
   la llaman y el build de backend —que incluye `seed`— fallaría) y dejarla para siempre (código muerto
   y una segunda definición de franja que la spec supersede).
7. **El seed ajusta las ventanas a 11:50/17:30.** Descartado: conservar 12:30/18:10 (cabe, pero deja 40
   min de sobrante y contradice «sustituir el inventario de 40 min»).
8. **`huellaDatos(datos, D)` cambia de firma.** Descartado: una sobrecarga sin D (permitiría una huella
   que ignora D y rompe RF-6).
9. **Valor y recuento de la configuración en `ResultadoAplicacion`, no en `Recuentos`.** Descartado:
   añadirlos a `Recuentos` (mezclaría la configuración con los recuentos de dominio y con la política de
   base vacía).
10. **La edición de la configuración no revalida los turnos existentes.** Descartado: bloquear la
    edición si algún turno deja de encajar (la spec no lo pide; impediría cambiar la configuración
    antes de corregir turnos). La generación rechaza el turno y lo explica con turno, D y déficit.
11. **No entero: comprobación de tipo en V1 + API `int`.** Descartado: confiar en que el parseo lo
    convierta en 0 (el mensaje diría «se recibió 0» y RNF-1 exige el valor recibido).
12. **Migración en dos pasos con legado temporal (T4 añade, T5/T6/T8 migran, T9 borra).** Descartado:
    cambiar las firmas y borrar en T4 (cada tarea intermedia cerraría con el árbol sin compilar, porque
    `ServicioTurnosEventos`, `solver_config` y `perfil` siguen llamando) y duplicar la aritmética en dos
    implementaciones (dos definiciones de franja que podrían divergir).

## 9. Verificación por tarea

Comandos comunes (desde la raíz del repo):

```bash
cmake --preset backend
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R '<patrón>'
ctest --preset backend
```

Todas las tareas están estimadas en **20–30 min**, con T9 en **25** desde que incluye migrar los casos
antiguos de `test_calculo_eventos`. T2, T5, T6, T7 y T8 están en el techo (30 min). **T7 es el más
ajustado** (constructor + huella): si no cabe, se para y se propone dividir la **spec** (011a: T1–T4;
011b: T5–T10), igual que si se estiran T5, T6 o T8; no se añaden tareas por encima de 10.

- **T1 (~25 min):** `-R '^test_duracion_horaria$'` verde; los casos de 30/90 aceptados, 29/91 y 5
  rechazados con el rango **y la acción**, `minutosDeHorasAcademicas(40, 35) == 1400`, `(40, 30) == 1200`
  y `franjasUtilizables(290, 10, 45) == 6`.
- **T2 (~30 min):** `-R '^(test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos)$'`
  verde; `VERSION_ESQUEMA_ACTUAL == 4`; la base recién creada tiene 1 fila con 35; `INSERT` con
  `id = 2` falla; una base v3 falla al abrir; `HUELLAS_CONOCIDAS` tiene la entrada 4.
- **T3 (~25 min):** `-R '^(test_servicio_configuracion_horaria|test_backend_nucleo_datos)$'` verde; sin
  fila → `{35, true}`; `guardar(29)`/`guardar(91)` fallan y conservan el valor; `guardar(45)` deja 1
  fila; `guardar(30)` y `guardar(90)` aceptan.
- **T4 (~25 min):** `-R '^test_calculo_eventos$'` verde con los casos **nuevos y los antiguos**
  (el legado sigue vivo); `errorDeEncaje(8 slots, 290, 10, 45)` cita «Mañana», 45 y «déficit 80»;
  `(8, 370, 10, 35)` vacío; `(5, 150, 10, 35)` déficit 35; el turno sin ventana devuelve el mensaje con
  la acción; `ventanaDePosicion` de la posición 2 con D = 35 da 08:10–08:20. **No** se exige el `grep`
  vacío: la eliminación es de T9.
- **T5 (~30 min):** `-R '^(test_servicio_turnos_eventos|test_backend_nucleo_datos|test_backend_cascada)$'`
  verde; con la configuración en 35 se crea el turno de 8 franjas y ventana 290 y, tras
  `guardarConfiguracionHoraria(45)`, un `agregarEvento` de 10 min en la posición 2 se rechaza con
  turno, 45 y déficit 80; con la configuración en 45, `crearTurno` de 8 franjas y ventana 290 se
  rechaza (déficit 70); un evento que rompe la invariante se rechaza sin escribir; los turnos de 5
  franjas en 5 h siguen pasando.
- **T6 (~30 min):** `-R '^(test_validador_entrada|test_cargador_configuracion|test_contrato_solver)$'`
  verde; 1, 5, 29 y 91 fallan por V2 con el rango; 30 y 90 pasan; 35.5 falla por V1 citando 35.5; un
  turno que no encaja (6 franjas, ventana 07:00–07:30, sin eventos) falla por V2;
  `configValida()` (45/6/300/30) sigue válida.
- **T7 (~30 min):** `-R '^(test_constructor_entrada|test_validador_entrada|test_servicio_generacion|test_ipc_aislamiento_bd)$'`
  verde; el preset con 0/5/45 no bloquea y la salida lleva D = 35; `construirJsonEntrada(..., 5, ...)`
  falla por rango; la disponibilidad 07:00–09:00 devuelve 3 franjas con 35; `huellaDatos(d, 35)` es
  estable y distinta de `huellaDatos(d, 45)`; el flujo con `huellaActual` que devuelve la huella con 45
  termina en `DatosAnteriores`.
- **T8 (~30 min):** `-R '^(test_seed_perfil|test_seed_aplicacion)$'` verde; turnos
  07:00–11:50/12:40–17:30 y recesos 08:10–08:20/13:50–14:00; tras sembrar, 1 fila con 35 y
  `resultado.duracionMinutos == 35`, `resultado.configuraciones == 1`; una base con solo la
  configuración se puede sembrar sin `--reemplazar`; tras `--reemplazar` sigue habiendo 1 fila con 35.
- **T9 (~25 min):** `cmake --build --preset backend --parallel` compila (el build es la prueba de que
  no queda ninguna llamada de 2/3 argumentos: las sobrecargas ya no existen),
  `ctest --test-dir build-backend --output-on-failure -R '^test_calculo_eventos$'` está verde y
  `ctest --preset backend` termina en **52/52**; los casos de `ventanaDePosicion` (2 args) y
  `eventoDentroDeVentana` (3 args) de `test_calculo_eventos.cpp:111-203` se migran a las firmas con D
  (o se retiran si duplican los de T4); `grep -rn "duracionFranjaEstimada" src test` está **vacío**;
  `grep -rn "ventanaDePosicion\|eventoDentroDeVentana" src` devuelve solo la definición nueva de cada
  función (con D); el `grep` en `test` se revisa a mano para confirmar que no queda ningún caso con la
  firma vieja (la garantía real es que el build compila).
- **T10 (~30 min):** el coordinator aplica §7; `ctest --preset backend` **52/52** (los 2 binarios
  nuevos); `grep -n "duracionFranjaEstimada\|VERSION_ESQUEMA_ACTUAL = 3\|40 min" docs/interfaz-frontend.md`
  vacío; `grep -n "40 min\|40 minutos\|8 franjas de 40\|08:20–08:30\|14:00–14:10\|12:30\|18:10"
  docs/guias/datos-prueba.md` vacío; `grep -n "8 franjas de 40\|08:20–08:30\|14:00–14:10\|pasos de 40\|12:20\|18:00\|12:30\|18:10\|40 min\|1600\|26 h 40"
  docs/guias/datos-prueba-frontend.md` vacío; **en positivo**, `grep -n "11:40\|17:20"
  docs/guias/datos-prueba-frontend.md` devuelve la rejilla nueva (07:00–11:40 / 12:40–17:20);
  `grep -n '"duracion_minutos": 40' docs/contrato-solver-2.0.md`
  vacío; `git diff --stat` acotado a los archivos de este plan.

## 10. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|----|----------|--------------|
| RF-1 | T1, T2, T3, T6 | T1: 30/90 aceptan y 29/91 rechazan; T2: `CHECK(id=1)` rechaza la 2.ª fila; T3: edición inválida no cambia el valor; T6: V2 aplica el mismo rango |
| RF-2 | T2, T3 | T2: la base nueva queda con 35; T3: `consultar()` devuelve `{35, true}` sin fila, `guardar` crea/actualiza y una edición inválida conserva el anterior |
| RF-3 | T1, T7 | T1: 40 h × 35 = 1400 y × 30 = 1200 en el único punto; T7: la rejilla de disponibilidad usa D y el contrato lleva D; el tope de 40 no se recalcula |
| RF-4 | T4, T5, T6, T8, T9 | T4: `franjasUtilizables` y déficit 80/35 y ventana con D; T5: el servicio rechaza turnos y eventos que no encajan; T6: la generación rechaza el turno que no encaja; T8: el perfil se valida con D y encaje; T9: se retira el legado y queda una sola definición |
| RF-5 | T6, T7 | T6: V2 30–90 y V1 no entero; T7: sobrescritura incondicional (preset 0/5/45 → 35) y fallo si D sale del rango |
| RF-6 | T7 | `huellaDatos(d, 35) != huellaDatos(d, 45)` y el flujo marca `DatosAnteriores` |
| RF-7 | T2 | Versión 4, catálogo, huella v4 y base v3 rechazada sin migrar |
| RF-8 | T8 | Turnos/recesos nuevos, 1 fila con 35, valor y recuento en el resultado, base con solo configuración sembrable, `--reemplazar` idempotente |
| RF-9 | T10 | La guía documenta el servicio, el valor efectivo, el rango, el punto único, la sobrescritura del preset y la deuda; `grep` sin las firmas viejas |
| RNF-1 | T1, T3, T4 | Mensajes con campo, valor, rango o déficit y acción |
| RNF-2 | T2 | Base v3 falla al abrir; no hay borrado ni migración; el respaldo del seed no cambia |
| RNF-3 | T1, T8 | Funciones sin reloj ni azar; dos siembras dejan lo mismo |
| RNF-4 | T1, T4, T6, T9 | Un solo `errorDeRango`, un solo `minutosDeHorasAcademicas`, un solo `errorDeEncaje`; T9 elimina el legado y deja una sola definición |
| RNF-5 | T10 | `ctest --preset backend` 52/52 sin `test_frontend_conversion` ni `test_generacion_seed` |
| RNF-6 | T1–T10 | Cada RF tiene al menos una tarea y una verificación en esta tabla |

## 11. Riesgos y supuestos

- **Bases existentes (v3) dejan de abrir.** Es la política de la 009 (RF-7): no hay migración. El
  usuario debe crear una base nueva con respaldo o restaurar; el diálogo de apertura ya lo ofrece.
  Afecta a cualquier base creada antes de este cambio.
- **El frontend no compila** (`construirJsonEntrada` cambia de firma; el selector de duración deja de
  ofrecerse). Es deuda declarada en la spec y documentada en RF-9; la entrega se verifica con
  `BUILD_FRONTEND=OFF`.
- **«Datos anteriores» no está cableado en la interfaz:** la vista construye `ServicioGeneracion` con
  `huellaActual = {}`. La 011 deja `huellaDatos` con D y lo documenta como deuda; sin ese cableado, un
  cambio de D no se marca en la práctica. El mecanismo y su prueba viven en el backend.
- **`test/frontend/test_generacion_seed.cpp` fija 40.** Decisión cerrada: queda como deuda del frontend
  (no compila sin migrar el builder) y se documenta en la guía; no se reabre.
- **Legado temporal de `calculo_eventos` (T4–T8).** Durante cinco tareas conviven las firmas antiguas
  (envoltorios) y las nuevas; ninguna tarea cierra con el árbol sin compilar y T9 borra el legado. Si T9
  se aplazara, quedaría una segunda definición de franja: es el riesgo que el `grep` de cierre evita.
- **Specs históricos:** las supersesiones de 009/008/005/010 se anotan al cerrar (§7.2), no en esta
  fase; solo el planner edita `spec.md`.
- **Cambiar D con turnos ya guardados** no se bloquea en la edición; la generación rechaza y explica.
  Es la lectura de RF-2/RF-4 (decisión 10).
- **Un preset JSON con 5** se rechaza al cargarlo (V2), aunque un preset en memoria con 5 no bloquee la
  construcción. Son dos caminos distintos de la spec (decisión 5 y §5).
- **La fila por defecto se escribe al crear el esquema**, no por el servicio; es la única excepción a
  «solo el servicio escribe» y se justifica en §1.2/§3.
- **La CLI y la consola** no añaden comandos: ningún RF pide uno. La consola debe seguir compilando y
  pasando (RNF-5); si el usuario quiere un comando `configuracion`, es otro requisito.
- **Supuesto de datos:** el seed es un fixture de persistencia; con D = 35 sigue sin ser generable por
  la regla de tipo de la 010 (un solo laboratorio), y el plan no lo intenta (la 011 no cambia eso).

## 12. Restricciones de la spec (no se tocan)

- El modelo del solver, el contrato 2.0 en su forma, el tope de 40 horas académicas y la interfaz.
- 1 hora académica = 1 bloque = 1 franja; los 80 min de Educación Media quedan fuera.
- No hay migración de bases ni conversión de valores anteriores.
- No se añaden pantallas, comandos ni configuraciones por turno/curso/docente.
