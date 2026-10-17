# Plan 008 - Datos de prueba del centro (Plan 31059)

Base: f94acba455a04ae80364c82e1f07ae16d9f156ec

> **Alcance.** Implementa la spec 008 (`Estado: aprobada (2026-10-17)`). Solo puebla la **base de
> datos**; no genera presets del solver. Las decisiones del QUÉ están en la spec y no se reabren aquí.

## 1. Forma de la entrega

- **Biblioteca `seed`** (`src/seed`, estático, sin OR-Tools) con el perfil, las variantes de docentes,
  la cobertura y el aplicador. La enlazan el ejecutable y los tests.
- **Ejecutable `gestor-horarios-seed`** (`src/seed/src/main.cpp`): herramienta de desarrollo con la CLI
  de §7.
- **Datos del perfil en C++** (tablas constantes y generación determinista), **no** en JSON ni en
  recursos Qt: son datos fijos, se validan con funciones puras y no hace falta parsear nada en tiempo
  de ejecución.
- **Sin dependencias nuevas**: `seed` enlaza `datos` (que arrastra `backend` + Qt6 Core/Sql). Ni OR-Tools
  ni Qt Widgets.

## 2. Archivos y responsabilidades

| Ruta | Responsabilidad |
|---|---|
| `src/seed/CMakeLists.txt` | Biblioteca `seed` + ejecutable `gestor-horarios-seed`; enlaza `datos`; `PROJECT_VERSION` para `--version`. |
| `src/seed/include/seed/perfil.hpp` | Tipos del perfil, `bloques()`, totales por año, `cargaMateria()`, `cargaAsignada()`, `demandaTotal()`, `perfilPorDefecto()`, `validarPerfil()`, `recuentosEsperados()`. |
| `src/seed/include/seed/docentes.hpp` | `VarianteDocentes`, `DocenteDef`, `docentesDeVariante()`, `coberturaDeVariante()`, `docentesCumplenCarga()`, `nombreVariante()`/`varianteDesdeTexto()`. |
| `src/seed/include/seed/sembrador.hpp` | `OpcionesAplicacion`, `ResultadoAplicacion`, `aplicar()`, `rutaRespaldoLibre()`, `resumenTexto()`. |
| `src/seed/src/perfil.cpp` | Catálogo de 57 materias, 4 planes, 16 aulas, 2 turnos, 13 cursos; validación del perfil. |
| `src/seed/src/docentes.cpp` | Generación determinista de las 4 variantes y comprobación de cobertura. |
| `src/seed/src/sembrador.cpp` | Apertura, detección de datos, respaldo, transacción, inserciones (cargas por SQL directo), borrados y recuentos. |
| `src/seed/src/main.cpp` | CLI, salida por consola y códigos de salida. |
| `src/CMakeLists.txt` | `add_subdirectory(seed)` dentro de `if(BUILD_BACKEND)`, tras `datos`, bajo `if(BUILD_SEED)`. |
| `CMakeLists.txt` (raíz) | `option(BUILD_SEED "Build the test-data seeder (requires BUILD_BACKEND)" ON)`. |
| `test/CMakeLists.txt` | `add_qtest(test_seed_perfil ...)` y `add_qtest(test_seed_aplicacion ...)`, dentro de `if(TARGET backend)` y `if(TARGET seed)`. |
| `test/backend/test_seed_perfil.cpp` | Pruebas puras del perfil, planes y cobertura. |
| `test/backend/test_seed_aplicacion.cpp` | Pruebas de aplicación sobre bases temporales (`QTemporaryDir`). |
| `docs/guias/datos-prueba.md` | Guía de uso: CLI, variantes, recuentos, seguridad y fuera de alcance. |

**Por qué no se toca la consola existente:** la consola es un REPL de CRUD; el seed tiene su propio
ciclo (validar perfil, respaldar, transaccionar, informar). Añadirlo como subcomando mezclaría dos
responsabilidades y obligaría a que la consola cargara el perfil. *Alternativa descartada:* subcomando
`seed` en `gestor-horarios-consola`.

## 3. Estructuras del perfil (`seed/perfil.hpp`)

```cpp
namespace seed {

struct MateriaAnio { QString nombre; int anio; };                  // catálogo (57)
struct CargaPlan   { QString materia; int anio; int horas; };      // enlace plan-materia (210)
struct PlanDef     { QString codigo, nombre, areaDesarrollo; QVector<CargaPlan> cargas; };
struct AulaDef     { QString nombre; int capacidad; QString edificio, piso; };
struct TurnoDef    { QString nombre; QTime inicio, fin; int slots;
                     int recesoDespuesDeSlot; QTime recesoInicio, recesoFin; };
struct CursoDef    { int anio; QString seccion; QString aula; };   // sin turno (NULL)
struct Perfil {
    QVector<TurnoDef>     turnos;      // 2
    QVector<AulaDef>      aulas;       // 16
    QVector<MateriaAnio>  materias;    // 57, orden: año 1..5, luego orden del 31059
    QVector<PlanDef>      planes;      // 4
    QVector<CursoDef>     cursos;      // 13
};

/// Identidad: 1 hora académica = 1 bloque. Pura.
int bloques(int horas);
/// Suma de horas (y de bloques, que coinciden) de un plan en un año; -1 si no tiene ese año.
int horasDeAnio(const PlanDef&, int anio);
int bloquesDeAnio(const PlanDef&, int anio);

// ── Carga potencial (RF-6, RF-7) ──────────────────────────────────────────
/// Secciones del año: 3 en 1.º–3.º y 2 en 4.º–5.º.
int seccionesDeAnio(int anio);
/// Horas máximas de una materia entre los planes que la incluyen.
int horasMaximas(const Perfil&, const QString& materia);
/// ¿Es una de las 6 materias nuevas de un plan derivado? (no añaden demanda). Pura.
bool esMateriaNueva(const QString& nombre);
/// Carga de una materia-año = horasMaximas × secciones; **0** para las 6 materias nuevas
/// (sustituyen horas base y no añaden demanda). Pura.
int cargaMateria(const Perfil&, const QString& materia);
/// Σ cargaMateria de las materias de un docente. Pura.
int cargaAsignada(const Perfil&, const QVector<QString>& materias);
/// Demanda total = 13 cursos × 40 = 520; coincide con Σ cargaMateria del catálogo. Pura.
int demandaTotal(const Perfil&);

const Perfil& perfilPorDefecto();

struct Recuentos { int turnos, recesos, aulas, materias, planes, planMaterias,
                   cursos, docentes, asignaciones, disponibilidades; };
Recuentos recuentosEsperados(const Perfil&, VarianteDocentes);

bool validarPerfil(const Perfil&, QString* error);   // §5
}
```

`perfilPorDefecto()` devuelve una referencia a un objeto construido una sola vez (`static const`),
inmutable; no lee reloj, disco ni configuración.

## 4. Datos del perfil

### 4.1 Turnos (2) y recesos (2)

| Nombre | inicio | fin | slots | receso tras slot | receso |
|---|---|---|---|---|---|
| `Mañana` | 07:00 | 12:30 | 8 | 2 | 08:20–08:30 |
| `Tarde` | 12:40 | 18:10 | 8 | 2 | 14:00–14:10 |

`fin` es la campana (incluye el receso). La rejilla aritmética de la 005 termina en 12:20/18:00; el
desfase de 10 min está documentado en la spec y **no se corrige**.

**Convención de `despues_de_slot`.** Se escribe **2** con indexación **1-based** (el receso cae tras la
2.ª franja). Verificación: la franja 2 de la mañana termina a las **08:20** y la de la tarde a las
**14:00**, que coinciden con el `inicio` del receso; en `test_seed_aplicacion` se lee
`SELECT turno, despues_de_slot, inicio, fin FROM Recesos` y se compara con `(2, 08:20, 08:30)` y
`(2, 14:00, 14:10)`. Si la 004/005 confirmaran una indexación 0-based, se escribiría **1** y el test
seguiría exigiendo las mismas horas.

### 4.2 Catálogo de materias (57) y planes (4)

- Catálogo = **51 materias-año** del 31059 + **6 nuevas**:

| Materia nueva | Año | Plan |
|---|---|---|
| Matemática Aplicada | 4 | EXA |
| Física Aplicada | 5 | EXA |
| Química Aplicada | 4 | NAT |
| Ciencias de la Tierra Aplicadas | 5 | NAT |
| Geografía Económica | 4 | SOC |
| Historia Contemporánea | 5 | SOC |

- Nombre de cada materia del 31059: `<Área> <año>º` (por ejemplo `Castellano 1º`, `Física 3º`,
  `Ciencias de la Tierra 5º`). El catálogo se ordena por año y, dentro del año, por el orden de áreas de
  la tabla del 31059.
- **Orden fijo de las 6 materias nuevas (requisito de determinismo, RF-11):** cada una se añade **al
  final de su año**, después de todas las materias del 31059 de ese año. El orden dentro del año es el
  de la tabla de arriba filtrado por año: en 4.º, `Matemática Aplicada`, `Química Aplicada`,
  `Geografía Económica`; en 5.º, `Física Aplicada`, `Ciencias de la Tierra Aplicadas`,
  `Historia Contemporánea`. Con eso, los índices 0-based del catálogo son: 0–8 (1.º), 9–17 (2.º),
  18–27 (3.º), 28–41 (4.º, con las nuevas en **39, 40 y 41**) y 42–56 (5.º, con las nuevas en
  **54, 55 y 56**). De ahí salen el mapeo `i, i+19, i+38` de los titulares de la variante de
  suplentes y los índices `0, 7, 14, 21, 28, 35, 42, 49` de los docentes parciales.
- **Plan GEN** (`General`, área de desarrollo «General»): exactamente la tabla adaptada de la spec.

  | Área | 1.º | 2.º | 3.º | 4.º | 5.º |
  |---|---|---|---|---|---|
  | Castellano | 4 | 4 | 4 | 4 | 4 |
  | Inglés | 5 | 5 | 5 | 5 | 3 |
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
  | Participación en Grupos… | 5 | 5 | 5 | 5 | 5 |

  Totales GEN: **40/40/40/40/40 horas académicas** (1 hora = 1 bloque); 9/9/10/11/12 materias.

- **Planes derivados**: copian GEN y aplican exactamente estos cambios (el resto de áreas y años queda
  igual). Ningún año supera 40 horas académicas.

  | Plan | Año | Cambios (h académicas) | Total |
  |---|---|---|---|
  | EXA | 4.º | +Matemática Aplicada 3; EF 5→4; Castellano 4→3; Grupos 5→4 | 40 |
  | EXA | 5.º | +Física Aplicada 3; EF 5→4; GHC 3→2; Grupos 5→4 | 40 |
  | NAT | 4.º | +Química Aplicada 3; Castellano 4→3; GHC 3→2; Grupos 5→4 | 40 |
  | NAT | 5.º | +Ciencias de la Tierra Aplicadas 3; Castellano 4→3; GHC 3→2; Grupos 5→4 | 40 |
  | SOC | 4.º | +Geografía Económica 3; EF 5→4; Matemáticas 4→3; Grupos 5→4 | 40 |
  | SOC | 5.º | +Historia Contemporánea 3; EF 5→4; Matemáticas 4→3; Grupos 5→4 | 40 |

  Totales por plan (h académicas): **GEN 40/40/40/40/40 · EXA 40/40/40/40/40 ·
  NAT 40/40/40/40/40 · SOC 40/40/40/40/40**. Las 6 materias nuevas pesan **3 h** cada una y se
  compensan con recortes de 1 h en tres áreas base del mismo año, de modo que el año sigue sumando 40.
  Los tres derivados difieren entre sí también en **qué áreas base recortan** (EXA: EF/Castellano/Grupos
  en 4.º y EF/GHC/Grupos en 5.º; NAT: Castellano/GHC/Grupos; SOC: EF/Matemáticas/Grupos).

### 4.3 Aulas (16) y cursos (13)

- `Aula 01` … `Aula 13`: capacidad 34–40, edificios `A`/`B`, pisos `Planta baja`/`Piso 1`.
- `Aula 14` (reserva, 32, edificio `B`), `Laboratorio de Ciencias` (36, `C`, `Planta baja`),
  `Taller de Informática` (30, `C`, `Piso 1`).
- Cursos, en este orden: `1ºA 1ºB 1ºC 2ºA 2ºB 2ºC 3ºA 3ºB 3ºC 4ºA 4ºB 5ºA 5ºB`. Cada curso recibe, en
  ese orden, `Aula 01`…`Aula 13` (**distinta por curso**). **`turno` = NULL en los 13.**

### 4.4 Docentes (`seed/docentes.hpp`)

```cpp
enum class VarianteDocentes { Holgada, Suplentes, SinProfesorado, SinDisponibilidad };
struct FranjaDef    { int dia; QTime inicio, fin; };   // dia 1=lunes … 5=viernes
struct DocenteDef   { QString id, nombre, email, telefono;
                      QStringList materias; QVector<FranjaDef> disponibilidad; int horasAula; };
QVector<DocenteDef> docentesDeVariante(VarianteDocentes);
bool coberturaDeVariante(const Perfil&, VarianteDocentes, QString* error);
QString nombreVariante(VarianteDocentes);
std::optional<VarianteDocentes> varianteDesdeTexto(const QString&);   // acepta con guiones
```

- **Nombres ficticios**: combinación determinista de una lista de 20 nombres y 20 apellidos (con
  acentos y eñes) en `docentes.cpp`; `id` `P001`…; correo `pNNN@centro-de-prueba.edu.ve`; teléfono
  `+58 400-000-0000`. No se usan datos de personas reales.
- **Disponibilidad completa**: lunes a viernes `07:00–18:00` (**5 filas por docente**; empieza y acaba
  en fronteras de la rejilla de ambos turnos y contiene bloques completos de los dos).
- **Carga de una materia-año**: `cargaMateria(m) = horasMaximas(m) × seccionesDeAnio(m.anio)` (3
  secciones en 1.º–3.º, 2 en 4.º–5.º) para las **51 materias del 31059**, y **0** para las **6 materias
  nuevas** (sus horas sustituyen horas de áreas base del mismo plan y año y ya están contadas en el
  máximo de esas áreas). `cargaAsignada(docente) = Σ_m cargaMateria(m)`. `demandaTotal()` = **520**
  (13 cursos × 40), que coincide con la suma de `cargaMateria` de las 57 materias. No se usa ninguna
  otra base para la banda de capacidad.
- **Holgada (65)**: `P001`…`P057` con **una materia** cada uno (catálogo en orden) y
  `horasAula = 30`. La carga va de **0 a 18** (las 6 materias nuevas aportan 0): **0 %–60 % del tope**,
  es decir **≤ 60 %**; el máximo, 18 (`GHC 1.º` = 6 × 3), es exactamente el 60 %. `P058`…`P065`:
  un docente **parcial** por cada materia de índice 0, 7, 14, 21, 28, 35, 42 y 49 (0-based), también
  con `horasAula = 30`; `P058`, `P060`, `P062` y `P064` con disponibilidad solo de mañana
  `07:00–12:20` y `P059`, `P061`, `P063` y `P065` solo de tarde `12:40–18:00` (**5 filas cada uno**
  también: 65 × 5 = 325).
- **Suplentes (43)**: `P001`…`P019` titulares con **tres materias** cada uno (índices `i`, `i+19`,
  `i+38` para `i = 0..18`; cubren las 57) y
  `horasAula = ceil(cargaAsignada / 1,15)` (redondeo hacia arriba). Cada titular queda en **~110–115 %
  de su tope** (`cargaAsignada ≥ 0.8 × horasAula`), es decir, **cerca del tope y sin excederlo mucho**:
  modela el desbordamiento que obliga a recurrir a los suplentes y cumple el ≥ 80 % de RF-7.
  `P020`…`P043`: 24 suplentes **sin materias**, disponibilidad completa y `horasAula = 8`.
  Cargas de los 19 titulares: `37, 27, 27, 27, 29, 30, 41, 22, 38, 26, 31, 24, 31, 24, 25, 34, 12,
  19, 16` (**Σ = 520**); topes: `33, 24, 24, 24, 26, 27, 36, 20, 34, 23, 27, 21, 27, 21, 22, 30, 11,
  17, 14` (**Σ = 461**). Frente a `demandaTotal = 520`, los topes quedan al **88,65 %** (**11,35 % por
  debajo**, dentro del 10–15 % de RF-7) y los ratios carga/tope van de **109 % a 114,8 %**; los 24
  suplentes aportan capacidad de sobra (192 h).
- **Sin profesorado (64)**: la holgada **sin `P023`** (materia índice 22 = `Física 3º`), de modo que esa
  materia-año queda sin ningún docente en ambos turnos (320 disponibilidades = 64 × 5).
- **Sin disponibilidad (65)**: la holgada con la disponibilidad de `P023` sustituida por lunes a
  viernes `19:00–21:00` (**cero solape** con los dos turnos); siguen siendo 5 filas.
- `horasAula` no se persiste (vive en el preset); el perfil lo declara para las comprobaciones de
  cobertura/carga de las pruebas. Todos los años de todos los planes suman **exactamente 40** horas
  académicas, que es el límite que `ServicioCargaHoraria` acepta (`excedeLimiteHoras` rechaza solo
  `> 40`); las cargas se insertan por SQL directo en todo caso.

## 5. Validación del perfil (pura)

`validarPerfil` comprueba, en este orden y con mensaje en español que identifica plan/año/materia:

1. 2 turnos (mañana/tarde) con 8 slots, `inicio`/`fin` de §4.1 y receso tras el slot 2 con horas de
   §4.1; 16 aulas con nombre único; 13 cursos con `anio` 1–5, sección de un carácter, pareja
   año+sección única y aula existente y **distinta** entre cursos.
2. Catálogo: 57 materias, nombre único, `anio` 1–5, sin huérfanas (cada una usada por ≥1 plan).
3. Plan GEN: coincide **exactamente** con la tabla de §4.2 (horas y años), 51 cargas, 9/9/10/11/12.
4. Cada plan: sin materias repetidas ni años vacíos; `Σ_m horas_m ≤ 40` por año; progresión
   (Arte/CN solo 1–2; Física/Química/Biología solo 3–5; CT solo 5; Soberanía solo 4–5).
5. Derivados: ≥2 materias nuevas de su eje, todas en 4.º–5.º, usadas **solo** por ese plan; ≥2 áreas
   base con horas distintas de GEN; total declarado = suma (se recalcula, no se declara aparte).
6. Recuentos esperados: 2 recesos, 57 materias, 4 planes, 210 plan-materias, 13 cursos.
7. Carga de las variantes (`docentesCumplenCarga`, pura):
   - holgada: para cada docente, `cargaAsignada(d) ≤ 0.6 × horasAula(d)` (con `horasAula = 30` y
     cargas de 0 a 18; el máximo, 18, es exactamente el 60 %);
   - suplentes: para cada titular, `cargaAsignada(d) ≥ 0.8 × horasAula(d)` y ratio carga/tope entre
     1,00 y 1,15; `Σ horasAula(titulares)` entre el **85 % y el 90 %** de `demandaTotal()` (10–15 % por
     debajo; 461 de 520 = 88,65 %);
   - sin profesorado / sin disponibilidad: heredan las relaciones de la holgada (la variante no cambia
     la carga, solo la cobertura).

`coberturaDeVariante` comprueba, para **cada** materia-año del catálogo: (a) hay ≥1 docente asignado;
(b) para **cada uno** de los dos turnos hay ≥1 docente de esa materia con un rango que contiene un
bloque completo del turno (fronteras `inicio + k×40`). Devuelve la primera materia-año que falla.

## 6. Aplicador (`seed/sembrador.hpp`)

```cpp
enum class EstadoAplicacion {
    Ok, UsoInvalido, BaseConDatos, VarianteDesconocida, ConfirmacionRequerida,
    RespaldoFallido, EsquemaNoEsperado, FalloApertura, FalloEscritura, PerfilIncoherente
};
struct OpcionesAplicacion {
    QString rutaDb;                                  // obligatoria
    VarianteDocentes variante = VarianteDocentes::Holgada;
    bool reemplazarTodo = false;
    bool soloDocentes   = false;
    bool confirmado     = false;                     // --si
    QDateTime ahora;                                 // nombre del respaldo; vacío = reloj
    bool simularFalloTrasDocentes = false;           // solo tests (atomicidad)
};
struct ResultadoAplicacion {
    EstadoAplicacion estado = EstadoAplicacion::FalloApertura;
    QString detalle;
    QString rutaRespaldo;
    Recuentos recuentos;
    bool ok() const { return estado == EstadoAplicacion::Ok; }
};
ResultadoAplicacion aplicar(const OpcionesAplicacion&);
QString resumenTexto(const Perfil&, VarianteDocentes, const Recuentos&);   // informe de §8
```

### 6.1 Pseudocódigo de `aplicar`

```
1. si rutaDb vacía → UsoInvalido.
2. perfil = perfilPorDefecto(); si !validarPerfil → PerfilIncoherente (no toca la base).
3. variante: si no es válida → VarianteDesconocida.
4. si reemplazarTodo && soloDocentes → UsoInvalido ("elige una sola operación").
5. ctx.abrir(rutaDb) con ContextoBaseDatos:
     - no existe → la crea con el esquema (OkCreada);
     - existe con esquema esperado → OkAbierta;
     - FalloEsquema/FalloApertura → EsquemaNoEsperado/FalloApertura (sin escribir).
6. db = ctx.conexion(). Primero respaldo y comprobaciones: `VACUUM INTO` no puede ejecutarse dentro de
   una transacción.
7. hayDatos = contarFilasDeDominio(db) > 0.
8. si soloDocentes:
     - si !confirmado → ConfirmacionRequerida (antes de respaldar).
     - respaldar (ruta única, §6.2); si falla → RespaldoFallido.
     - transacción: borrar docentes (§6.3), insertar docentes de la variante, commit.
     - si falla: rollback → FalloEscritura (la variante anterior queda intacta).
9. si hayDatos && !reemplazarTodo → BaseConDatos (no escribe nada).
10. si reemplazarTodo: si !confirmado → ConfirmacionRequerida; respaldar; si falla → RespaldoFallido.
11. transacción:
      si reemplazarTodo → borrar TODO el dominio en orden §6.3.
      insertar turnos+recesos, aulas, materias, planes (+210 cargas por SQL directo), cursos,
      docentes+materias+disponibilidad.
      comprobar recuentos contra recuentosEsperados(); si difieren → rollback → FalloEscritura.
      commit.
12. rellenar ResultadoAplicacion.recuentos con los recuentos leídos de la base y estado = Ok.
```

- **Atomicidad**: una única transacción SQLite sobre la conexión del `ContextoBaseDatos`; los servicios
  de alta (`crearTurno`, `crearAula`, `crearMateria`, `crearPlan`, `crearCurso`, `crearProfesor`,
  `asignarMateria`, `agregarDisponibilidad`, `agregarReceso`) usan esa misma conexión y **no** abren
  transacción propia, así que sus `INSERT` entran en la del seed. Cualquier `Resultado::ok == false`,
  comprobación de recuentos o el fallo simulado → `db.rollback()`.
- **Detección de datos**: suma de `COUNT(*)` de `Aulas`, `Profesores`, `Materias`, `PlanEstudio`,
  `PlanEstudio_Materia`, `Turnos`, `Recesos`, `Cursos`, `Profesor_Materia`, `Disponibilidad_Profesor`.
- No se usa `Respaldo::descartarBaseYCrearNueva` (borra el archivo y pierde la conexión); se respalda y
  se vacía por SQL dentro de la transacción.

### 6.2 Respaldo

`Respaldo::construirNombreRespaldo(ruta, ahora)` + `Respaldo::crearRespaldo(db, ruta)`; si el destino
existe, se prueba `<ruta>-2`, `-3`… hasta 99 (función pura `rutaRespaldoLibre(ruta, ahora)`). El respaldo
se hace **antes** de la transacción y su ruta se devuelve en `ResultadoAplicacion::rutaRespaldo`. Si
falla, se aborta sin borrar nada.

### 6.3 Borrados (orden FK-seguro, `foreign_keys = ON`)

Total: `Disponibilidad_Profesor` → `Profesor_Materia` → `Cursos` → `PlanEstudio_Materia` →
`PlanEstudio` → `Materias` → `Recesos` → `Turnos` → `Aulas` → `Profesores`.
Solo docentes: `Disponibilidad_Profesor` → `Profesor_Materia` → `Profesores`.

### 6.4 Inserciones

Servicios existentes sobre la conexión, **salvo `PlanEstudio_Materia`, que va por SQL directo** (ver
abajo) para no abrir una transacción anidada:

1. `ServicioTurnosRecesos::crearTurno(nombre, inicio, fin, 8)` y
   `agregarReceso(turno, despuesDeSlot = 2, duracion = 10, inicio, fin)` con
   `("Mañana", 08:20, 08:30)` y `("Tarde", 14:00, 14:10)` (2 + 2).
2. `ServicioAula::crearAula` (16, guardando `nombre → id`).
3. `ServicioMaterias::crearMateria` (57, guardando `nombre → id`; `requisitos` vacío).
4. `ServicioPlanesEstudio::crearPlan` (4) y, para las **210** cargas,
   `INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas)` **por SQL directo**
   dentro de la transacción del seed (preparado una vez, `bindValue` por fila).
5. `ServicioCursos::crearCurso(anio, seccion, /*turno*/ QString(), idAula)` (13) → `turno` NULL.
6. `ServicioProfesor::crearProfesor` (N), `asignarMateria(id, idMateria)` y
   `agregarDisponibilidad(id, FranjaHoraria{dia, inicio, fin})` (5 por docente).

**Métodos prohibidos dentro de la transacción del seed** (abren y confirman su **propia** transacción;
QSQLITE no anida): `ServicioCargaHoraria::asignarCarga`, `ServicioCargaHoraria::actualizarCarga`,
`ServicioProfesor::eliminarProfesor`, `ServicioTurnosRecesos::eliminarTurno`,
`NucleoDatos::eliminarConCascada`. Los borrados del seed son SQL directo (§6.3); el alta de cargas
también (§6.4, punto 4).

Cada `Resultado` se comprueba; el primer fallo aborta con `FalloEscritura` y el mensaje del servicio.

## 7. CLI (`gestor-horarios-seed`)

```
gestor-horarios-seed --db <ruta> [--variante <holgada|suplentes|sin-profesorado|sin-disponibilidad>]
                     [--reemplazar | --solo-docentes] [--si] [--ayuda] [--version]
```

| Opción | Efecto |
|---|---|
| `--db <ruta>` | **Obligatoria**. Base destino (se crea si no existe). |
| `--variante <v>` | Variante de docentes; por defecto `holgada`. |
| `--reemplazar` | Reemplazo total (exige `--si` y deja respaldo). |
| `--solo-docentes` | Recarga solo docentes/materias asignadas/disponibilidad (exige `--si` y respaldo). |
| `--si` | Confirma la operación destructiva (sin terminal interactiva). |
| `--ayuda`, `--version` | Ayuda y versión (código 0). |

**Sin `--reemplazar`/`--solo-docentes`**: si la base tiene datos → error «la base ya tiene datos» y
código 2, sin escribir.

**Salida en éxito** (§8): recuentos por entidad, totales por plan y año (horas académicas = bloques), ruta del
respaldo si lo hubo, y `ok`. Todo en español.

**Códigos de salida**: `0` ok · `1` uso/argumentos · `2` base con datos · `3` confirmación requerida ·
`4` respaldo fallido · `5` apertura/esquema · `6` variante desconocida · `7` escritura/atomicidad ·
`8` perfil incoherente.

## 8. Informe (`resumenTexto`)

Texto plano determinista, sin fechas ni rutas temporales, con:

```
Base: <ruta>
Variante: holgada
Turnos: 2   Recesos: 2   Aulas: 16   Materias: 57   Planes: 4   Materias de plan: 210
Cursos: 13 (sin turno)   Docentes: 65   Materias asignadas: 65   Disponibilidades: 325
Materias por año: GEN 9/9/10/11/12 · EXA 9/9/10/12/13 · NAT 9/9/10/12/13 · SOC 9/9/10/12/13
GEN  General            40/40/40/40/40 h académicas
EXA  Ciencias Exactas   40/40/40/40/40 h académicas
NAT  Ciencias Naturales 40/40/40/40/40 h académicas
SOC  Ciencias Sociales  40/40/40/40/40 h académicas
Respaldo: <ruta o «no aplica»>
```

`Recuentos` se obtiene contando en la base tras el commit (`SELECT COUNT(*)`), no del perfil, para que
el informe verifique lo escrito. La **variante** viaja como parámetro de `resumenTexto` y se imprime en
la segunda línea (no forma parte de `Recuentos`, que cuenta filas).

## 9. Decisiones y alternativas descartadas

| Decisión | Alternativa descartada | Motivo |
|---|---|---|
| Biblioteca `seed` + ejecutable propio | Subcomando en `gestor-horarios-consola` | Separa CRUD interactivo de una operación transaccional con respaldo; los tests enlazan la biblioteca sin CLI. |
| Perfil en C++ | JSON embebido en `.qrc` | Datos fijos y validables en compilación/pruebas; JSON añade parseo y errores en ejecución sin beneficio. |
| Servicios de dominio para insertar | SQL directo para todo | Reutiliza validaciones y auditoría existentes; el SQL directo se limita a borrados, recuentos y `PlanEstudio_Materia`. |
| `PlanEstudio_Materia` por SQL directo | `ServicioCargaHoraria::asignarCarga` | `asignarCarga` abre y confirma **su propia** transacción (`ServicioCargaHoraria.cpp:104-129`); anidarla con la del seed falla en QSQLITE. |
| Borrados por SQL directo en orden FK | `eliminarProfesor` / `eliminarTurno` / `eliminarConCascada` | También abren transacción propia; usarlos dentro de la del seed rompería la atomicidad. |
| Respaldo + vaciado en transacción | `descartarBaseYCrearNueva` | Esa función borra el archivo (pierde la conexión) y no permite rollback del reemplazo. |
| Una transacción para todo el reemplazo | Borrar y luego insertar por partes | Sin transacción, un fallo deja la base a medias (RF-10). |
| Datos derivados calculados (docentes por regla) | Tabla literal de 65 docentes | Menos código, determinista y verificable por cobertura; los nombres salen de listas fijas. |
| `horasAula` declarado en el perfil | Persistirlo en la base | La 005 lo sitúa en el preset; se declara solo para comprobar carga/cobertura. |
| `simularFalloTrasDocentes` | Probar la atomicidad con bloqueos externos | Seam mínimo y determinista para el rollback; no se usa desde la CLI. |

## 10. Pruebas (tests primero)

Registro en `test/CMakeLists.txt`, dentro de `if(TARGET backend)` y `if(TARGET seed)`:

```cmake
add_qtest(test_seed_perfil backend/test_seed_perfil.cpp seed)
add_qtest(test_seed_aplicacion backend/test_seed_aplicacion.cpp seed Qt6::Sql)
```

### `test_seed_perfil.cpp` (Qt Test)

- `planGeneralReproduceTablaAdaptada`: 51 cargas, 9/9/10/11/12, horas **40/40/40/40/40** (1 hora =
  1 bloque).
- `planesDerivadosCumplenReglas`: 53 cargas cada uno, ≥2 nuevas en 4.º–5.º, ≥2 áreas variadas,
  **Σ horas ≤ 40** por año y totales de §4.2 (40 en todos).
- `catalogoSinHuerfanas`: 57 materias, nombres únicos, todas usadas, ninguna repetida por plan.
- `progresionDel31059`: años de Arte, CN, Física, Química, Biología, CT y Soberanía.
- `cursosAulasYTurnos`: 13 cursos, sin turno, aulas distintas, 2 turnos con 8 slots y recesos exactos.
- `variantesValidasCubrenAmbosTurnos`: holgada y suplentes → `coberturaDeVariante == true`.
- `varianteSinProfesoradoNoCubre`: falla y menciona `Física 3º`.
- `varianteSinDisponibilidadNoCubre`: falla y menciona `Física 3º`.
- `recuentosEsperados`: 65/65/325, 43/57/215, 64/64/320, 65/65/325.
- `cargaMateriaYDemanda`: `seccionesDeAnio` 3/3/3/2/2; `cargaMateria("Castellano 1º") == 12`,
  `("Matemáticas 3º") == 12`, `("Ciencias de la Tierra 5º") == 4`,
  `("Matemática Aplicada") == 0` (materia nueva, no añade demanda) y `demandaTotal() == 520`; la suma de
  `cargaMateria` de las 57 materias es 520 y las cargas por materia van de **0 a 18**.
- `catalogoOrdenFijo`: las materias nuevas ocupan los índices **39, 40, 41** (`Matemática Aplicada`,
  `Química Aplicada`, `Geografía Económica`) y **54, 55, 56** (`Física Aplicada`,
  `Ciencias de la Tierra Aplicadas`, `Historia Contemporánea`); `materias[22] == "Física 3º"`.
- `holgadaCumpleTope`: para cada docente de la holgada, `cargaAsignada ≤ 0.6 × horasAula` (con tope 30
  y carga máxima 18, que es exactamente el 60 %).
- `suplentesCumplenTopeYCapacidad`: cada titular cumple `cargaAsignada ≥ 0.8 × horasAula` y
  `1.00 ≤ cargaAsignada / horasAula ≤ 1.15`; el test **calcula** `Σ horasAula(titulares)` y comprueba
  que **vale 461** y que cae en `[0.85, 0.90] × demandaTotal()` (461 / 520 → 88,65 %, un 11,35 % por
  debajo). La tarea no depende del literal: la verificación es el rango.
- `limiteExactoDeCuarenta`: `horasDeAnio` de los **20** pares plan-año (4 planes × 5 años) es 40 y
  `40 > MAX_HORAS...` es falso (el límite se acepta; el servicio rechaza solo `> 40`).
- `validarPerfilRechazaCorrupciones`: un plan con año > 40 horas académicas, una materia huérfana y un
  aula repetida → `false` con mensaje.

### `test_seed_aplicacion.cpp` (Qt Test + `QTemporaryDir`)

Los casos se construyen por partes: T5a deja verdes los de turnos/recesos, aulas, materias, planes,
cargas y cursos; T5b amplía el de recuentos y añade docentes, determinismo y rollback.

- `creaBaseNuevaYPuebla`: `Estado::Ok`, recuentos por tabla, 13 cursos sin turno, 210 filas de
  `PlanEstudio_Materia` y `SELECT turno, despues_de_slot, inicio, fin FROM Recesos` igual a
  `(Mañana, 2, 08:20, 08:30)` y `(Tarde, 2, 14:00, 14:10)`.
- `baseConDatosFallaSinEscribir`: segunda aplicación → `BaseConDatos` y recuentos intactos.
- `reemplazoTotalConRespaldo`: `--reemplazar`+confirmado → `Ok`, archivo de respaldo existe y es válido
  (`Respaldo::validarRespaldo`), recuentos correctos.
- `reemplazoTotalSinConfirmacion`: `ConfirmacionRequerida` y sin respaldo ni cambios.
- `soloDocentesConservaElResto`: cambia variante → docentes 43, planes/cursos/aulas/turnos/210 cargas
  idénticos (comparar filas antes/después).
- `varianteDesconocida` / `rutaVacia`: estados `VarianteDesconocida`/`UsoInvalido` sin tocar la base.
- `atomicidadAnteFallo`: `simularFalloTrasDocentes` con reemplazo → `FalloEscritura` y la base conserva
  **exactamente** los datos previos (incluidas las 210 cargas), lo que demuestra que las cargas por SQL
  directo entran en la misma transacción.
- `determinismo`: dos bases nuevas con la misma variante → mismas filas (nombres, ids, relaciones y las
  210 cargas), salvo `fecha_creacion`/`fecha_modificacion`.
- `tiempoDeAplicacion`: aplicar en base nueva < 15 s (RNF-1).

## 11. Verificación

**Entorno de referencia (Docker, con OR-Tools):**

```bash
docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"
```

**Solo los tests del seed (más rápido):**

```bash
docker exec gestor-dev bash -lc "cd /workspace && cmake --build build && ctest --test-dir build -R 'test_seed' --output-on-failure"
```

**Host sin OR-Tools:** no se puede configurar desde cero (`BUILD_BACKEND=ON` exige OR-Tools). Se
**reutiliza** el árbol `build/` ya configurado:

```bash
cmake --build build --target test_seed_perfil test_seed_aplicacion gestor-horarios-seed
ctest --test-dir build -R 'test_seed' --output-on-failure
```

Si `build/` no está configurado, usar el comando de Docker. `dev-frontend` **no** compila el seed (es
intencional): el módulo vive dentro de `if(BUILD_BACKEND)`.

> **Nota sobre el total de la suite.** El número de pruebas previas (44) puede variar si
> `find_package(GTest)` no encuentra Google Test: `test/CMakeLists.txt` omite los `add_gtest` en ese
> caso. El criterio es «suite completa en verde» comparada con el total que reporte `ctest` en el mismo
> entorno antes del cambio, más los 2 tests nuevos.

**CLI (verificación manual concreta):**

```bash
rm -f /tmp/opencode/seed-demo.db*
./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-demo.db
# → ok, recuentos 2/2/16/57/4/210/13/65/65/325 y totales por plan
./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-demo.db; echo $?
# → error «la base ya tiene datos», código 2
./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-demo.db --variante suplentes --solo-docentes --si
# → ok, docentes 43, resto intacto, respaldo impreso
./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-demo.db --variante inexistente; echo $?
# → código 6
```

**Por tarea:**

| Tarea | Comando | Resultado esperado |
|---|---|---|
| T1 | `ctest --test-dir build -R test_seed_perfil --output-on-failure` | Casos del plan General y `bloques()` verdes. |
| T2 | ídem | Casos de derivados, catálogo y validación verdes. |
| T3 | ídem | Casos de turnos, recesos, aulas y cursos verdes. |
| T4 | ídem | Casos de las 4 variantes y cobertura verdes. |
| T5a | `ctest --test-dir build -R test_seed_aplicacion --output-on-failure` | Apertura, detección de datos, turnos/recesos, aulas, materias, planes, 210 cargas y cursos verdes. |
| T5b | ídem | Docentes, enlaces, recuentos, determinismo y commit/rollback verdes. |
| T6 | ídem | Respaldo, `--solo-docentes`, confirmación y atomicidad verdes. |
| T7 | Comando CLI de arriba | Salida y códigos 0/2/6 como se indica; `--ayuda` lista las opciones. |
| T8 | `ctest ... -R 'test_seed'` + CLI | Informe con recuentos, materias por año y totales por plan correctos; < 15 s. |
| T9 | `ctest --preset full` | Suite completa en verde (total previo + 2) y guía creada. |

## 12. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|---|---|---|
| RF-1 catálogo materia-año | T1, T2 | `test_seed_perfil`: `catalogoSinHuerfanas` |
| RF-2 plan General | T1 | `planGeneralReproduceTablaAdaptada` |
| RF-3 planes derivados | T2 | `planesDerivadosCumplenReglas` |
| RF-4 cursos/turnos | T3, T5a | `cursosAulasYTurnos` + `test_seed_aplicacion.creaBaseNuevaYPuebla` (construido en T5a) |
| RF-5 aulas | T3, T5a | `cursosAulasYTurnos` (aulas distintas y 16) + `creaBaseNuevaYPuebla` (16 filas) |
| RF-6 variante holgada | T4 | `variantesValidasCubrenAmbosTurnos`, `holgadaCumpleTope`, `recuentosEsperados` |
| RF-7 variante suplentes | T4 | `variantesValidasCubrenAmbosTurnos`, `suplentesCumplenTopeYCapacidad`, `recuentosEsperados` |
| RF-8 variantes inválidas | T4 | `varianteSinProfesoradoNoCubre`, `varianteSinDisponibilidadNoCubre` |
| RF-9 conmutación de variante | T6 | `soloDocentesConservaElResto` |
| RF-10 aplicación y seguridad | T5a, T5b, T6 | `creaBaseNuevaYPuebla` (recesos y 210 cargas), `baseConDatosFallaSinEscribir`, `reemplazoTotalConRespaldo`, `reemplazoTotalSinConfirmacion` |
| RF-11 determinismo | T5b | `determinismo` (+ funciones puras sin reloj) |
| RF-12 informe y reutilización | T8 | `resumenTexto` en CLI (recuentos, materias por año y totales) y recuentos leídos de la base |
| RF-13 documentación | T9 | `docs/guias/datos-prueba.md` y `--ayuda` |
| RNF-1 tiempo | T8 | `tiempoDeAplicacion` (< 15 s) |
| RNF-2 sin dependencias ni esquema | T1 | `cmake --preset full` configura; no hay tablas/columnas nuevas |
| RNF-3 mensajes en español | T5a, T5b, T6 | Casos de error comparan el mensaje |
| RNF-4 datos ficticios | T4, T9 | Nombres de listas ficticias; revisión en la guía |
| RNF-5 estabilidad salvo auditoría | T5b | `determinismo` ignora `fecha_creacion`/`fecha_modificacion` |
| RNF-6 atomicidad y respaldo | T6 | `atomicidadAnteFallo` (210 cargas intactas), `reemplazoTotalConRespaldo` |
| RNF-7 trazabilidad | Todas | Esta tabla + `journal.md` por tarea |

## 13. Riesgos y notas para el implementador

- **`despues_de_slot`**: se escribe 2 (1-based) y se verifica leyendo `Recesos` en
  `test_seed_aplicacion` (08:20–08:30 y 14:00–14:10). Si la 004/005 confirmaran 0-based, se usa 1 y el
  test exige las mismas horas. La rejilla de bloques no depende de ello.
- **`fin` del turno** (12:30/18:10) incluye el receso; la rejilla aritmética termina en 12:20/18:00. No
  “corregir” el `fin` ni desplazar bloques.
- **Transacciones anidadas**: `asignarCarga`, `actualizarCarga`, `eliminarProfesor`, `eliminarTurno` y
  `eliminarConCascada` abren su propia transacción. **No llamarlos** dentro de la del seed: las cargas y
  los borrados son SQL directo.
- **`MAX_HORAS_SEMANALES_POR_ANIO`** (40) solo lo comprueba `ServicioCargaHoraria`; al insertar por SQL
  directo no se ejecuta, pero **ningún año supera 40** y el límite exacto 40 se acepta (el servicio
  rechaza solo `> 40`).
- **Carga de docentes**: `cargaAsignada` usa la **hora máxima por materia** y las secciones del año; las
  6 materias nuevas aportan 0. Si se toca una tabla de plan o el orden del catálogo, hay que recalcular
  `demandaTotal` (520) y `Σ topes` de suplentes (461).
- **`ServicioProfesor::agregarDisponibilidad`** valida día 0–6 e `inicio < fin`; los rangos
  `07:00–18:00` y `19:00–21:00` son válidos.
- **Nombres con acentos**: los `listar*` ordenan con `QCollator`; el perfil debe incluir eñes y tildes
  (por ejemplo «Ñáñez», «Jesús»).
- **Cursos sin turno**: `crearCurso(anio, seccion, QString(), idAula)` deja `turno` NULL; no usar el
  nombre del turno.
- No se actualiza `MEMORY.md` en las tareas: se hace al cerrar la spec, con `VEREDICTO: APROBADO`.
