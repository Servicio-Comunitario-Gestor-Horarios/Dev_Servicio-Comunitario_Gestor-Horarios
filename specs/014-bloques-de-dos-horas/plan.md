# Plan 014 — 014a (motor) y 014b (cambio de requisitos: 8 workers y presupuestos)

Base: **`bf06df67c32fc3dceb13a4a67226c47eb65004b2`** (HEAD, commit «backup» del usuario).

> **Nota de base.** La `spec.md` y el `estado.md` **corregidos** (QA B1–B4) están **en el árbol** respecto de
> ese commit, no dentro de él. El reviewer diferencia contra `bf06df6` y debe incluir la spec/estado del
> árbol como contexto, no como cambios de código.

## 1. Alcance y reparto

- **014a (este plan, T1–T7):** el **motor**. Retirada de S3, modelado CP-SAT de las reglas duras
  (emparejamiento, alineación, misma aula), separación y huecos blandos, jerarquía por fases, causa (5),
  informe del resultado y fixture de aceptación sin EF. **No toca** la constitución, el contrato
  documentado ni los goldens normativos.
- **014b (lote propio, B1–B7; ver §13):** **cambio de requisitos**: 8 workers por defecto, presupuestos
  60/20/10, plazo de 300 s, **huecos al final duros y separación blanda**, causa (5) con 11 h, **informe
  ampliado**, **seed a horas pares**, aceptación/medición y documentación/regresión. **No toca** la lógica
  de bloques.

**Regla de trabajo:** tests primero en cada tarea; una tarea no cierra con el árbol roto.
**Tamaño:** **9 tareas**. T3 y T5 se parten en **T3a/T3b** y **T5a/T5b** (45–60 min declarados) para
que ninguna unidad de revisión mezcle el emparejamiento con la alineación ni la separación con los huecos.
Cabe en el límite de 10.

## 2. Archivos y responsabilidades

| Archivo | Cambio | RF |
|---|---|---|
| `src/backend/include/backend/solver/config/contrato_solver.hpp` | Quitar `PESO_AGRUPACION` y sus `static_assert`; quitar `esInicioSegmento` y `calcularPenalizacionGrupo`; conservar `segmentosTurno`. **No se añaden constantes de peso**: se calculan por instancia en `pesosBlandos` | RF-6, RF-7 |
| `src/backend/src/solver/config/contrato_solver.cpp` | Quitar las implementaciones de `esInicioSegmento` y `calcularPenalizacionGrupo`; conservar `segmentosTurno` y las funciones de capacidad | RF-6 |
| **Nuevo** `src/backend/include/backend/solver/config/bloques.hpp` + `src/backend/src/solver/config/bloques.cpp` | Funciones **puras** de la rejilla: tramos, pares, desvío, cota, apariciones, causa (5), días, huecos y pesos blandos. Sin OR-Tools | RF-1, RF-2, RF-3, RF-4, RF-5, RF-10 |
| `src/backend/include/backend/solver/variables.hpp` + `.cpp` | Quitar `hay`/`isolated`/`longGroup`; añadir `ocupada`, `par`, `materiaEnFranja`, `bloqueSimple`, `apareceDia`, `incumpleSeparacion`, `hueco` (sin `huecoNoFinal`: los huecos al final son duros) | RF-1, RF-2, RF-3, RF-4 |
| **Nuevo** `src/backend/include/backend/solver/restricciones/restricciones_bloques.hpp` + `.cpp` | Restricciones duras del emparejamiento, la alineación y la misma aula | RF-1, RF-2, RF-9 |
| **Se elimina** `src/backend/include/backend/solver/restricciones/restricciones_agrupacion.hpp` + `src/backend/src/solver/restricciones/restricciones_agrupacion.cpp` | S3 fuera | RF-6 |
| `src/backend/include/backend/solver/solver.hpp` + `src/backend/src/solver/solver.cpp` | `PresupuestoFases` (referencia, separación+huecos, preferencias); objetivo S1+S2; expresiones de separación y huecos; fases 1/2a/2b; causa (5); sin S3 | RF-1–RF-10 |
| `src/backend/include/backend/services/ValidadorSalidaSolver.hpp` + `.cpp` | Quitar el análisis S3 (`GrupoAgrupacion`, `grupos`, `penalizacionAgrupacion`); conservar P2/P4 y ampliar P4 con la comprobación de bloques | RF-6, RF-12 |
| **Nuevo** `src/backend/include/backend/services/analisis_bloques.hpp` + `src/backend/src/services/analisis_bloques.cpp` | Informe puro de bloques, materias, desvío, cota, huecos y separación | RF-12 |
| `src/backend/include/backend/services/ServicioGeneracion.hpp` + `.cpp` | Campo aditivo `AnalisisBloques bloques` en `ResultadoGeneracion`; se calcula en `alResponder` | RF-12 |
| `src/backend/CMakeLists.txt` | Añadir los `.cpp` nuevos; quitar `restricciones_agrupacion.cpp` | — |
| **Nuevos** `test/backend/test_bloques.cpp`, `test/backend/test_solver_bloques.cpp`, `test/backend/test_analisis_bloques.cpp` | Tests puros y de modelo | RF-1, RF-2, RF-5, RF-10, RF-12 |
| **Nuevos** `test/backend/fixtures/centro_sin_ef.hpp` + `.cpp`, `test/backend/test_solver_centro_sin_ef.cpp` | Fixture de aceptación (24 cursos, 34/36 h, 848 clases) y benchmark | RF-11, RNF-2 |
| `test/CMakeLists.txt` | Registrar los tests nuevos y la librería del fixture | — |
| Tests existentes | Ver §11 | — |

## 3. Formulación CP-SAT de las reglas duras

### 3.1 Notación

- `x[p][m][c][d][s]` (existente): clase de la materia `m` del curso `c`, día `d`, franja `s`, con el
  docente `p`.
- `T(c)`: franjas del turno del curso; `Seg(c,d)`: sus segmentos contiguos (cortados por eventos).
- `r_m = h_m mod 2` (resto de la materia-curso).

### 3.2 Variables nuevas

| Variable | Clave | Significado |
|---|---|---|
| `ocupada` | `(c,d,s)` | `1` si el curso tiene clase en `(d,s)`; `= Σ_{p,m} x` (0/1 por D4) |
| `par` | `(c,d,s)` | `1` si el par de franjas `(s, s+1)` está ocupado y emparejado; solo si `s, s+1` están en el mismo segmento |
| `materiaEnFranja` | `(m,c,d,s)` | `1` si la materia tiene clase en `(d,s)`; `= Σ_p x` (0/1 por D1/D2/D3) |
| `bloqueSimple` | `(m,c,d,s)` | `1` si el par `(s, s+1)` es **entero** de la materia `m`; solo si existe `materiaEnFranja` en las dos franjas |
| `apareceDia` | `(m,c,d)` | `1` si la materia tiene **un** bloque en el día `d` (el tope diario lo garantiza) |
| `incumpleSeparacion` | `(m,c,d)` | `1` si la materia aparece en `d` y en `d+1` (blando) |
| `hueco` | `(c,d,s)` | `1` si la franja del turno no tiene clase; `= 1 − ocupada` |

`participa[m,c,d,s] = materiaEnFranja[s] + materiaEnFranja[s+1] − bloqueSimple[m,c,d,s]` es una
**expresión lineal** (0/1) que vale `1` si la materia ocupa el par (entera o a medias). No necesita
variable propia.
La **separación** es un término blando sobre `incumpleSeparacion` (R8); los **huecos al final** son una
desigualdad lineal sobre `ocupada` (R9); el **viernes**, una expresión lineal sobre `hueco`.

### 3.3 Restricciones

**(R1) Ocupación.** Para cada `(c,d,s)`:
`ocupada[c,d,s] = Σ_{p,m} x[p,m,c,d,s]`.

**(R2) Emparejamiento exacto (dimer tiling).** Para cada segmento, y para cada franja `s` del segmento:
`ocupada[c,d,s] = par[c,d,s] + par[c,d,s−1]`, donde `par[c,d,s]` solo existe si `s+1` está en el mismo
segmento y `par[c,d,s−1]` solo si `s−1` está en el mismo segmento (en los extremos, el término ausente es
0).

**(R3) Materia en franja.** `materiaEnFranja[m,c,d,s] = Σ_p x[p,m,c,d,s]`.

**(R4) Bloque simple.** `bloqueSimple[m,c,d,s] = min(par[c,d,s], materiaEnFranja[m,c,d,s],
materiaEnFranja[m,c,d,s+1])` con `AddMinEquality`; si falta una de las dos `materiaEnFranja`, el bloque
simple no se crea (es 0).

**(R5) Alineación.** Para cada materia-curso:
`2 · Σ_{pares} bloqueSimple[m, par] ≥ h_m − r_m`.
Equivalente: `Σ bloqueSimple ≥ ⌊h_m/2⌋`. Como la materia tiene `h_m` franjas (D3), el número de franjas
compartidas es `h_m − 2·Σ simple ≤ r_m`.

**(R6) Un bloque por materia y día.** Para cada `(m,c,d)`:
`Σ_{pares del día d} participa[m, par] ≤ 1`.
Consecuencia: la materia aparece como máximo una vez al día (bloque entero o mitad).

**(R7) Misma aula en el bloque.** Para cada par `(c,d,s)` y cada aula `a`:
`par[c,d,s] ⇒ y[c,d,s,a] = y[c,d,s+1,a]` (`AddEquality(...).OnlyEnforceIf(par)`).
La implicación solo se impone cuando `par=1`; si `par=0`, las dos franjas pueden estar ocupadas por
**pares distintos** (un tramo de 4, por ejemplo) y sus aulas **no** tienen por qué coincidir. La igualdad
es exactamente «las dos franjas de un mismo par, una sola aula».

**(R8) Separación (blanda con prioridad alta).**
`apareceDia[m,c,d] = Σ_{pares del día d} participa[m, par]` (0/1 por R6).
`incumpleSeparacion[m,c,d] = min(apareceDia[m,c,d], apareceDia[m,c,d+1])` para `d < nº_días − 1`
(`AddMinEquality`). El mismo día ya es imposible por R6. Es una prioridad **blanda**; se suma al objetivo de
la fase 2a con `PESO_SEPARACION` (domina a los huecos no-viernes).

**(R9) Huecos al final (duro) y viernes (blando).**
`hueco[c,d,s] = 1 − ocupada[c,d,s]`.
Para cada día, la ocupación es monótona: **`ocupada[c,d,s] ≥ ocupada[c,d,s+1]`** para franjas consecutivas
del mismo día (equivalente: las libres son un sufijo). La preferencia blanda es
`Σ_{d ≠ viernes} hueco[c,d,s]` (minimizar los huecos que no caen en viernes).

### 3.4 Por qué no hay bloques de 1 (demostración)

De (R2), toda franja ocupada pertenece a **exactamente un** `par`: `ocupada = par[s] + par[s−1]` y cada
`par` es 0/1. Además, `par[s]=1` obliga a `ocupada[s]=1` (por la igualdad en `s`) y a `ocupada[s+1]=1` (por
la igualdad en `s+1`). Luego las franjas ocupadas se cubren con pares disjuntos de franjas adyacentes. Un
tramo de longitud **impar** no se puede cubrir: su última franja necesitaría un `par` con una franja
posterior libre, lo que pondría `ocupada=1` en una franja libre. Recíprocamente, cualquier patrón de tramos
**pares** admite la partición desde el inicio del tramo, así que la formulación es **exacta** (no prohíbe
soluciones válidas).

### 3.5 Por qué la alineación es exacta

Una materia con `h_m` franjas tiene `h_m/2` pares si es par y `(h_m−1)/2` pares más un resto si es impar.
(R5) exige al menos `⌊h_m/2⌋` bloques simples; como cada bloque simple usa 2 franjas, las franjas que
quedan fuera de un bloque simple son como máximo `h_m − 2⌊h_m/2⌋ = r_m`. Esas franjas son exactamente las
que pueden ir en un bloque compartido. Por tanto: una materia **sin resto** (`r_m=0`) nunca comparte; una
materia impar comparte **como máximo una** franja; y un bloque compartido tiene exactamente dos franjas de
dos materias impares (cada una con su único resto).

### 3.6 Pseudocódigo de lo no obvio

```
crearParYSimples(config, vars):
  para cada curso c:
    para cada día d:
      para cada segmento seg de Seg(c,d):
        para k en 0..len(seg)-2:
          par[c,d,seg[k]] = NewBoolVar()
        para k en 0..len(seg)-1:
          s = seg[k]
          términos = [par[c,d,s] si k+1 < len, par[c,d,s-1] si k > 0]
          AddEquality(ocupada[c,d,s], Sum(términos))
        para k en 0..len(seg)-2:
          s = seg[k]
          para cada materia m con materiaEnFranja en s o s+1:
            si existen ambas:
              simple = NewBoolVar(); AddMinEquality(simple, {par, u[s], u[s+1]})
            // participa = u[s] + u[s+1] − simple (lineal)
          para cada materia m:
            AddLessOrEqual(Sum(participa de los pares del día), 1)
          para cada aula a:
            AddEquality(y[c,d,s,a], y[c,d,s+1,a]).OnlyEnforceIf(par[c,d,s])
```

## 4. Métrica, pesos y fases

### 4.1 Métrica (una sola fórmula)

- **Desvío del curso:** `Σ_m máx(0, franjas descubiertas_m − r_m)`; con (R5) es 0. **No existe un «desvío
  de rejilla»**: el número de bloques de 1 es una **comprobación dura del P4** (conflicto), no un término
  de la métrica. **Tope** = `Σ(h mod 2)`.
- El desvío **no es una prioridad**: las reglas duras lo fijan en 0; se calcula para el informe y para el
  validador de salida.

### 4.2 Pesos blandos (función pura)

`PesosBlandos pesosBlandos(const SolverConfig&)` en `bloques.hpp`:

- `maxHuecos = Σ_c (|T(c)| − h_c)` (huecos máximos posibles).
- `PESO_VIERNES = 1`.
- `PESO_SEPARACION = maxHuecos + 1`.

Así, **una** violación de separación pesa más que **todos** los huecos no-viernes juntos. Los huecos al
final son **duros** y no tienen peso. La preferencia va en la fase 2b.

### 4.3 Fases (3 resoluciones con preferencias; 2 sin ellas)

| Fase | Modelo | Objetivo | Presupuesto |
|---|---|---|---|
| **1. Referencia** | reglas duras + S1 + S2 | `PESO_HORAS·Σe + COSTE_SUPLENTE·Σsuplentes` | 6.0 |
| **2a. Separación y viernes** | duras, con `z*` y `e*` **fijados por igualdad** | `PESO_SEPARACION·Σ incumple + PESO_VIERNES·Σ huecos no-viernes` | 2.0 |
| **2b. Preferencia** | duras, `z*`/`e*` fijados y **el valor de 2a fijado por igualdad** | `expresionPreferencias` (013) | 1.0 |

- **Sin preferencias** se omite 2b y se devuelve 2a (2 resoluciones).
- **Escalera de plazo:** si 2a es `INFEASIBLE`/`UNKNOWN` → referencia; si 2b es `INFEASIBLE`/`UNKNOWN` →
  2a; `MODEL_INVALID` se propaga. La **cota** de la separación/viernes se mide contra la referencia
  obtenida con `PresupuestoFases.separacionHuecos` agotado (el solver devuelve la fase 1).
- **Garantía estructural:** `z*` y `e*` se extraen de la fase 1 y se fijan por igualdad en 2a/2b; el
  reparto de docentes y el exceso son los de la **nueva referencia** (la fase 1 con las reglas de la 014).
- **Coste previsto:** 6.0 + 2.0 + 1.0 = **9.0 s deterministas** (igual que la 013: 6+2+1). La medición
  real se hace en T7; umbral: **≤ 9,4 s de pared** (2× los 4,7 s de la 013 en el mismo host) y **< 60 s**
  en el test. **La 014b lo sustituye** por 8 workers y **60/20/10** con el **plazo de 300 s** (002; centro
  completo **172,6 s**), ver §13.

### 4.4 Por qué fases y no un único objetivo ponderado

Un único objetivo con S1, S2, viernes y preferencia exigiría pesos que dominen el **máximo agregado** de
los términos y haría más difícil la cota inferior del solver; además la preferencia tiene premios
negativos (013). Con fases: S1/S2 se fijan por anclaje (garantía exacta), el **viernes** tiene su fase
(una sola prioridad, sin pesos que calibrar) y la preferencia va al final con su valor anterior fijado.
**Alternativa descartada:** 5 fases lexicográficas (más resoluciones, más
tiempo); **alternativa descartada:** pesos fijos 10/100/1000 (pueden fallar en otra instancia).

## 5. Funciones puras (`bloques.hpp`)

```cpp
namespace bloques {
struct Tramo { int dia; int segmento; int inicio; int longitud; };
struct Par    { int dia; int segmento; int inicio; };   // cubre inicio e inicio+1

QVector<Tramo> tramosDeCurso(const SolverConfig&, int curso,
                             const QSet<FranjaSemana>& ocupadas);
QVector<Par>   paresDeCurso(const SolverConfig&, int curso,
                            const QSet<FranjaSemana>& ocupadas);
int desvioDeCurso(const SolverConfig&, int curso, const QSet<FranjaSemana>& ocupadas);
    // única fórmula: Σ_m máx(0, franjas descubiertas_m − (h_m mod 2)); no suma bloques de 1
int bloquesDeUno(const SolverConfig&, int curso, const QSet<FranjaSemana>& ocupadas);
    // comprobación dura del P4 (0 en una solución factible); NO es un término del desvío
int cotaDeCurso(const SolverConfig&, int curso);            // Σ(h mod 2)
int aparicionesEsperadas(int horas);                        // ⌈h/2⌉
bool causaCincoDeCurso(const SolverConfig&, int curso, QString* motivo);
QVector<int> diasDeMateria(const SolverConfig&, int curso, int materia,
                           const QSet<FranjaSemana>& ocupadas);   // días de generación
int incumplimientosSeparacion(const QVector<int>& dias);
QSet<FranjaSemana> huecosDeCurso(const SolverConfig&, int curso,
                                 const QSet<FranjaSemana>& ocupadas);
bool huecosAlFinal(const SolverConfig&, int curso, int dia,
                   const QSet<FranjaSemana>& ocupadas);        // regla dura
int huecosNoViernes(const SolverConfig&, int curso,
                    const QSet<FranjaSemana>& ocupadas);       // prioridad blanda
struct PesosBlandos { int separacion; int viernes; };
PesosBlandos pesosBlandos(const SolverConfig&);
}
```

- **Día de dominio:** `diaDominio = (diaGeneracion + 1) % 7` (lunes 1 … sábado 6, domingo 0). La
  conversión se hace **solo** en el informe (`analisis_bloques`) y en los textos; las funciones de rejilla
  trabajan con el día de generación (0 = lunes). **Viernes = día de generación 4.**
- `tramosDeCurso` recorre los segmentos de `Seg(c,d)` y agrupa las franjas ocupadas consecutivas.
- `paresDeCurso` empareja **desde el inicio del tramo**; si el tramo es impar, deja la última franja sin
  par. `bloquesDeUno` cuenta esas franjas (comprobación del P4); `desvioDeCurso` **no** las suma.
- `causaCincoDeCurso` devuelve `true` y el texto si el total del curso es impar o alguna materia tiene
  `⌈h/2⌉ > nº_días`.

## 6. Causa (5) y textos

En `primeraCausaNoFactible` (`solver.cpp`), **después** de (4) y antes del desenlace «no atribuible»:

1. **Total impar:** `Causa (5) conflicto de bloques: el curso %1 suma %2 horas semanales (impar) y no se
   puede repartir en bloques de dos.`
2. **Más apariciones que días:** `Causa (5) conflicto de bloques: la materia «%1» del curso %2 necesita %3
   apariciones y la semana solo tiene %4 días.` (con 5 días, 11 h o más).

Un fallo por aula concreta (tipo distinto en un bloque compartido, referencia ocupada) **no** es (5):
se informa la (2) si el conteo de capacidad la explica y, si no, «no atribuible» (010). El **mensaje de
«no atribuible» cita (1)–(5) y el diagnóstico de bloques** (materias con más apariciones que días, restos
sin pareja del mismo tipo, huecos que no caben al final). La **preferencia del viernes** (blanda) nunca es
causa; la **separación** es **blanda** y **no es causa (5)** (se informa); los **huecos al final** son
**duros** y se diagnostican con (3)/(4) o «no atribuible», nunca con (5).

## 7. Informe (`analisis_bloques`)

```cpp
struct BloqueSalida { QString curso; int diaDominio; int slotInicio; int slotFin;
                      int materiaA; int materiaB; bool compartido; int aula; };
struct AparicionSalida { int diaDominio; int slotInicio; int slotFin; };
struct MateriaSalida { QString curso; int materia; int apariciones;
                       QVector<AparicionSalida> franjas;          // una por aparición
                       QVector<int> diasDominio;
                       int franjasDescubiertas; int resto;
                       QVector<int> diasIncumplidosSeparacion; }; // se informa (blando)
struct HuecoSalida { QString curso; int diaDominio; int slotInicio; int slotFin;
                     bool esViernes; };                          // el viernes es preferencia
struct CursoSalida { QString curso; int desvio; int tope; int huecosNoViernes; };
struct AnalisisBloques {
    QVector<BloqueSalida> bloques;
    QVector<MateriaSalida> materias;
    QVector<HuecoSalida> huecos;
    QVector<CursoSalida> cursos;                                  // agregados por curso
    int bloquesSimples = 0, bloquesCompartidos = 0, bloquesDeUno = 0;
    int desvio = 0, tope = 0;
    int incumplimientosSeparacion = 0;
    int huecosNoViernes = 0;
};
AnalisisBloques analizarBloques(const HorarioSalida&, const SolverConfig&);
```

- `desvio` = `Σ_m máx(0, franjas descubiertas_m − r_m)`, **una sola fórmula** (P1). `bloquesDeUno` es un
  **contador de comprobación del P4** (debe ser 0) y **no** suma al desvío.
- **Cada materia lleva su `curso`** y **una `AparicionSalida` por aparición con sus franjas**; la
  separación es **blanda**, así que `diasIncumplidosSeparacion` se informa (puede ser > 0). Los **agregados
  por curso** (`CursoSalida`: desvío, tope, huecos no-viernes) se exponen aparte.
- Se calcula sobre el `HorarioSalida` (no cambia el contrato de salida del solver) y se añade como campo
  **aditivo** `AnalisisBloques bloques` a `ResultadoGeneracion`, en `ServicioGeneracion::alResponder`.
- El validador de salida (P4) se amplía: un **bloque de 1**, una materia con más franjas descubiertas que
  su resto, un bloque con dos aulas distintas o **un hueco que no es sufijo** ⇒ **conflicto**
  (`CalculoFallido`). La **separación** es blanda: se informa, **no** es conflicto. El bloque de 1 es
  conflicto, **no** un término de la métrica.
- Los días del informe son de **dominio** (0 = domingo); **viernes = dominio 5**.

## 8. Fixture de aceptación sin EF

`test/backend/fixtures/centro_sin_ef.{hpp,cpp}` con `SolverConfig fixtureCentroSinEf()`. Datos (de la
tabla confirmada, **sin Educación Física**):

**Planes y horas por año (sin EF):**

| Plan | Año | h | Materias |
|---|---|---|---|
| 31059 | 1º | 34 | Castellano 4, Inglés 4, Matemáticas 4, Arte 4, Cs. Naturales 6, GHC 6, Orientación 2, Grupos 4 |
| 31059 | 2º | 34 | ídem |
| 31059 | 3º | 34 | Castellano 4, Inglés 4, Matemáticas 4, Física 4, Química 4, Biología 4, GHC 4, Orientación 2, Grupos 4 |
| 31059 | 4º | 36 | Castellano 4, Inglés 4, Matemáticas 4, Física 4, Química 4, Biología 4, GHC 4, FSN 2, Orientación 2, Grupos 4 |
| 31059 | 5º | 36 | Castellano 3, Inglés 3, Matemáticas 4, Física 4, Química 4, Biología 4, Cs. de la Tierra 2, GHC 4, FSN 2, Orientación 2, Grupos 4 |
| 31060 | 1º–2º | 36 | Lengua 4, Idiomas 4, Matemáticas 4, Biología/Ambiente 4, Física 2, Química 2, GHC 6, Orientación Vocacional 4, ITP 6 |
| 31060 | 3º–4º | 36 | Lengua 4, Idiomas 4, Matemáticas 4, Biología 4, Física 4, Química 4, GHC 4, Orientación Vocacional 2, ITP 6 |

**Estructura:**

- **24 cursos** con los turnos confirmados (mañana: 31059 1A, 1B, 3A, 3B, 3C, 4A, 4B, 4C, 5A, 5B, 5C, 5D;
  tarde: 31059 2A, 2B, 2C y 31060 1A, 1B, 1C, 2A, 2B, 3A, 3B, 4A, 4B). `plan` y `anio`/`seccion`.
- **12 aulas** de uso general (índices 0..11), compartidas entre turnos; `aula_fija = true`; el `aula` de
  cada curso es su índice en la lista (0..11 en cada turno).
- **30 docentes**: cada materia del catálogo se asigna a un titular de forma determinista (round-robin);
  `horas_aula = 40`, `turno`/`plan`/`anios` vacíos, `disponibilidad` 0..15 todos los días. Capacidad
  30×40 = 1200 ≥ 848.
- **Rejilla:** `num_dias = 5`, `num_slots_dia = 16`, `D = 40`, turnos de 8 franjas (0..7 y 8..15) y un
  evento tras la franja 2 de cada turno (10 min); ventanas 07:00–12:30 / 12:40–18:10.
- **Totales:** 8 cursos de 34 h y 16 de 36 h; **848 clases**; 17/18 bloques; 6/4 huecos. **Huecos
  esperados (resultado medido, no garantía):** 34 h **con dos materias de 6 h** (31059 1º y 2º) → **4 al
  final del viernes + 2 al final de otro día**; 34 h **sin ellas** (31059 3º) → **6 al final del viernes**;
  36 h → **4 al final del viernes**.

**Uso:** `test_solver_centro_sin_ef.cpp` resuelve el fixture y comprueba el desvío 0, 848 clases, el
informe y los valores blandos con su cota (no el óptimo blando). El benchmark mide el tiempo. Se registra
como librería de test (`fixture_centro`) en `test/CMakeLists.txt`.

## 9. Verificación por tarea

Comando base (backend):

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
```

Comando final (con OR-Tools en Release), **excluyendo el test del seed** (014b):

```bash
cmake --preset full-release && cmake --build build-rel --parallel \
  && ctest --preset full-release -E test_generacion_seed
```

Cada tarea ejecuta el comando base; T7 ejecuta además el de `full-release` **con la exclusión**. Un test
nuevo se escribe en **rojo** antes del código y la tarea no se marca hasta que pasa.

## 10. Trazabilidad RF → tarea → verificación

| RF/RNF | Tarea(s) | Verificación |
|---|---|---|
| RF-1 (bloque y emparejamiento) | T2, T3a | `test_bloques` (tramos/pares: 2, 4, 5, 6, 0; impar no emparejable); `test_solver_bloques` (un segmento lleno produce sus pares; un tramo impar no tiene solución) |
| RF-2 (alineación) | T2, T4 | `test_bloques` (3 h → 1 simple + 1 resto; 4 h → 2 simples; 10 h → 5 simples); `test_solver_bloques` (3 h+3 h comparten; 3 h+4 h no; una materia par no comparte) |
| RF-3 (separación **blanda**) | T2, T5a, **B5b** | `test_bloques` (días consecutivos); `test_solver_bloques` (6 h tiende a L/X/V; el incumplimiento se informa y es **≤ la referencia**) |
| RF-4 (huecos al final **duros**; viernes blando) | T2, T5b, **B5b** | `test_bloques` (sufijo; hueco en medio no es solución); `test_solver_centro_sin_ef` (34 h con dos 6 h → 4+2; 34 h sin ellas → 6; 36 h → 4; todos al final) |
| RF-5 (cobertura/paridad/condiciones del 0) | T2, T4 | `test_bloques` (total impar → causa 5; `⌈h/2⌉ > días` → causa 5 (11 h); total par no basta) |
| RF-6 (métrica única/S3 fuera) | T1, T2, T6 | grep ampliado vacío; `test_bloques` (un solo desvío; el bloque de 1 **no** suma); `test_analisis_bloques`; P4 |
| RF-7 (naturaleza/garantía) | T3a, T3b, T4, T5a, T5b, **B5b** | `test_solver_preferencias` (z*/e* de la nueva referencia); `test_solver_centro_sin_ef` (desvío 0) |
| RF-8 (orden blando/013) | T5a, T5b, T6, **B5b** | `test_solver_preferencias` (**S1 > S2 > separación > viernes > preferencia**; sin preferencias no hay 2b; la preferencia no rompe una dura) |
| RF-9 (misma aula) | T3b, T4 | `test_solver_bloques` (par simple con una sola `y`; par de dos materias con una sola `y`); el bloque compartido 3 h+3 h se verifica en T4 |
| RF-10 (causa 5) | T2, T4, **B5b** | `test_bloques` (textos); `test_solver_bloques` (`analizarFallo` con total impar y con **11 h**); **no atribuible cita (1)–(5) y el diagnóstico de bloques**; la separación (blanda) no es causa; aula concreta → (2)/no atribuible |
| RF-11 (alcance) | T2, T4, T7 | `test_bloques` (1 h resto; **11 h causa 5**; 7–10 h informado); el fixture tiene 24 cursos |
| RF-12 (informe/contrato) | T6, **B6** | `test_analisis_bloques` (bloques, **curso por materia**, **apariciones con franjas**, **días incumplidos**, huecos, desvío único, tope, **agregados por curso**); `test_servicio_generacion` (campo aditivo); `test_contrato_generacion` (salida intacta) |
| RF-13 (coherencia documental) | **B3** | Revisión documental y golden en 014b; no entra en 014a |
| **RNF-1** (validez y 8 workers) | T7 (014a), **B1–B4 (014b)** | `test_solver_centro_sin_ef` produce un horario **válido** (no se comparan horarios entre ejecuciones); la búsqueda por defecto usa **8 workers** (B1) |
| **RNF-2** (desempeño) | T7 (014a), **B2 (014b)** | **014a:** < 60 s (objetivo ≤ 9,4 s, superado); **014b: plazo de 300 s** (002) con 8 workers y 60/20/10; centro completo **172,6 s** y test lento **183,58 s**, medidos y registrados |
| **RNF-3** (mensajes) | T2, T6, **B5b/B6** | textos de la causa (5) y etiquetas del informe en español, con curso/materia/día; **«no atribuible» cita (1)–(5) y el diagnóstico de bloques** |
| **RNF-4** (sin regresión) | T3b, T4, T6, **B5b** | `test_solver_horarios` (D1–D13), `test_validador_salida` (P2/P4), `test_contrato_generacion` (contrato 2.0) |
| **RNF-5** (sin interfaz) | T7 | la suite de `backend` pasa sin compilar ni ejecutar la interfaz (T7 no depende del test del seed) |
| **RNF-6** (protección de datos) | T1, T7 | `git diff --stat` no toca `src/backend/src/database` ni el esquema; la suite verde |
| **RNF-7** (trazabilidad) | — | esta tabla y el «Hecho cuando» de cada tarea |
| **RNF-8** (seed a horas pares) | **B7** | `test_seed_perfil` (ninguna materia impar de laboratorio/taller sin pareja del mismo tipo) y el seed valida |

## 11. Tests existentes que cambian (y en qué tarea)

| Test | Cambio | Tarea |
|---|---|---|
| `test/backend/test_contrato_solver.cpp` | Quitar `PESO_AGRUPACION`, `calcularPenalizacionGrupo` y `esInicioSegmento`; conservar `segmentosTurno` | **T1** |
| `test/backend/test_variables_cp_sat.cpp` | Quitar `hay`/`isolated`/`longGroup` | **T1** |
| `test/backend/test_validador_salida.cpp` | Quitar los tests de grupos S3 | **T1** |
| `test/backend/test_solver_horarios.cpp` | Quitar `solver_agrupacion_*` (T1); ajustar los escenarios que el emparejamiento deje infactibles (T3a/T3b) o la alineación (T4) | T1, T3a/T3b, T4 |
| `test/backend/test_solver_preferencias.cpp` | Quitar `presupuesto.agrupacion` y el desempate con S3 | **T1** |
| `test/backend/test_solver_benchmark.cpp` | Quitar las aserciones de S3 | **T1** |
| `test/backend/test_edge_cases.cpp` | Quitar referencias a S3 (T1); revisar los escenarios de solver con totales impares (T4) | T1, T4 |
| `test/backend/test_contrato_generacion.cpp` / `test_servicio_generacion.cpp` | Añadir la comprobación del campo aditivo `bloques` | T6 |
| `test/backend/test_validador_salida.cpp` | Añadir los tests de `analisis_bloques` y P4 ampliado | T6 |
| `test/backend/test_solver_horarios.cpp` | Ajustar los escenarios con total/segmento impar a totales pares y huecos al final | T4 |
| `test/backend/test_solver_preferencias.cpp` | `separacionHuecos` y la nueva escalera 2b→2a→referencia | T5a/T5b |
| `test/backend/test_solver_benchmark.cpp` | El escenario pequeño queda como humo; el de aceptación pasa a `test_solver_centro_sin_ef` | T7 |
| `test/backend/test_solver_preferencias.cpp` | Separación **blanda** (con peso) y huecos al final **duros**; la fase 2a vuelve a incluir separación + viernes | **B5b** |
| `test/backend/test_solver_bloques.cpp` | **11 h** → causa (5); **7–10 h** informado; separación ≤ referencia; huecos al final; el mensaje de «no atribuible» cita (1)–(5) | **B5b** |
| `test/backend/test_solver_centro_sin_ef.cpp` | Aseverar **separación informada ≤ la referencia** y **`huecosNoFinal == 0`**; el viernes se informa | **B5b** |
| `test/backend/test_analisis_bloques.cpp` | Curso por materia, apariciones con franjas, días incumplidos, agregados por curso | **B6** |
| `test/backend/test_validador_salida.cpp` | P4: hueco no sufijo ⇒ conflicto; la separación se informa | **B6** |
| `test/backend/test_seed_perfil.cpp` | **B7**: ninguna materia impar de laboratorio/taller sin pareja del mismo tipo (o todas pares) | **B7** |
| `test/frontend/test_generacion_seed.cpp` | **014b** (el seed sintético de 5 h no cumple la separación); **T7 lo excluye** | 014b |

## 12. Riesgos y umbrales

- **Tiempo de resolución.** Las reglas duras y las variables nuevas estrechan el problema; la fase 1 con
  6.0 puede no encontrar la primera solución en el fixture de 24 cursos. **La 014b sustituye el umbral de
  la 014a (≤ 9,4 s, 2× la 013) por el plazo de 300 s** (002) con **8 workers** y presupuestos **60/20/10**:
  el centro completo tarda **172,6 s** (medido, Release) y queda por debajo del plazo. Si no se cumple, se
  documenta y se propone ajustar el presupuesto o la spec (no se rebaja el criterio en silencio).
- **Tamaño del modelo.** `materiaEnFranja`/`bloqueSimple` por materia y par y las implicaciones de aula por
  par y aula; estimación **~30–40k booleanas** en el fixture. Se **mide en T3a/T3b/T4** (número de
  variables y restricciones del modelo construido, impreso en el test) y no solo el tiempo en T7. Si supera
  ~40k, podar `materiaEnFranja`/`bloqueSimple` a las franjas con `x` y revisar las implicaciones de aula.
- **Escenarios existentes infactibles.** Los tests pequeños con totales impares o segmentos impares sin
  huecos dejan de ser factibles; se ajustan a totales pares y huecos al final (T3a/T4).
- **Huecos al final duros; separación blanda.** Los huecos al final pueden volver infactible el centro si
  la disponibilidad no permite dejar el sufijo; se diagnostica con (3)/(4)/no atribuible (nunca (5) por la
  separación). La **separación**, al ser blanda, no vuelve infactible el horario: se informa. El **seed de
  5 h** se corrige en B7 (horas pares o pareja del mismo tipo).
- **Materias de 11 h o más.** Con separación blanda solo es infactible **11 h o más**
  (`⌈h/2⌉ > nº_días`, causa (5)); **7–10 h** se informa como incumplimiento. Los planes reales no las
  tienen.
- **La preferencia y las fases.** La 2b fija el valor de 2a; los tests que asumían el orden antiguo
  (preferencia > S3) se reescriben. La garantía estructural se mantiene con `z*`/`e*` de la **nueva**
  referencia.
- **Separación blanda y viernes; huecos al final duros.** T7 comprueba el **valor informado de separación
  y su cota** (≤ el de la referencia con la misma `z*`/`e*`) y que **`huecosNoFinal == 0`**; la colocación
  del viernes se registra. No se asume el óptimo blando. La expectativa 4+2 / 6 se documenta como resultado
  medido y, si difiere, se anota su causa.
- **Fixture y docentes.** El fixture usa 30 docentes con disponibilidad amplia y asignación round-robin;
  si alguna materia queda sin titular, V13 falla. Se comprueba en T7 (cada materia tiene al menos un
  docente elegible).
- **Cancha/EF.** La cancha admite **dos cursos a la vez** (160 ≥ 96); el **aforo simultáneo por aula** es
  una **spec futura**. El fixture no usa EF. No hay riesgo de capacidad por la cancha.
- **Frontend.** El contrato de salida no cambia; el campo `bloques` es aditivo. El test del seed y los
  textos de la interfaz se ajustan en 014b; **T7 no depende de ese test** (ver P5).

## 13. 014b — cambio de requisitos (2026-11-06): 8 workers y presupuestos

**Decisión del usuario.** El determinismo **no es esencial**; si el programa resuelve con **más workers**,
es mejor así. Se sustituye la promesa de «mismo horario entre ejecuciones» por **validez** con una búsqueda
de **8 workers**. Además, el **plazo del proceso de cálculo sube de 60 s a 300 s** (002) para conservar la
calidad con 8 workers.

**Medición que lo motiva (journal T7 de la 014a, Release):**

| Configuración | Presupuesto | Resultado | Pared |
|---|---|---|---|
| 1 worker | 60 s det | **UNKNOWN** | 286,5 s |
| **8 workers** | **60 s det (solo fase 1)** | **FEASIBLE: 848 clases, desvío 0, 0 bloques de 1** | **24,3 s** |
| **8 workers, 60/20/10 (completo)** | — | **848 clases, desvío 0, separación 2** | **172,6 s** |
| **Test lento de aceptación (`slow-release`)** | — | **848 clases, desvío 0, `huecosNoFinal == 0`** | **183,58 s** |
| 8 workers, dos ejecuciones | — | **no coinciden** (determinismo NO) | — |
| producto antes de la 014b | 1 worker (6/2/1) | test lento en Debug: 380,8 / 642,5 s (1.153,7 s) | — |

**Diseño.**

- `PresupuestoFases`: **`workers = 8`** por defecto, `referencia = 60.0`, `separacionHuecos = 20.0`,
  `preferencias = 10.0` (tiempos **deterministas**). `random_seed = 0` se conserva.
- `Solver::ejecutar` ya recibe `workers`; solo cambia el **defecto** a 8 y se conserva la guarda `> 0`.
- **Plazo del servicio:** `ServicioGeneracion` pasa su `plazoMs` por defecto de **60 000 a 300 000**
  (002: 300 s); salud y apagado siguen en 5 s. El plazo del **proceso** (002) y los presupuestos internos
  de las fases (014b) son distintos: 172,6 s < 300 s.
- **Justificación de los presupuestos:** con 8 workers, la fase 1 a **60 s det ≈ 24,3 s** de pared
  (medido); el **centro completo con 60/20/10 tarda 172,6 s** (medido, Release) y el **test lento
  183,58 s**, por debajo del **plazo de
  300 s** de la 002. Con el plazo de **60 s** la separación empeoraba de **2 a 90** (el servicio cortaba
  antes de que la 2a/2b mejoraran). No se sube más: 300 s es el techo del proceso y 172,6 s deja margen
  para IPC y para máquinas más lentas.
- **Test de aceptación:** `test_solver_centro_sin_ef` usa `Solver()` **por defecto** (sin
  `PresupuestoFases` explícito) y conserva `LABELS "slow"`; los presets `backend`/`full`/`full-release` lo
  excluyen (`ctest --preset slow` / `slow-release`).
- **Medición:** el centro completo (**848 clases**) con los valores por defecto, en **Release**; se
  registra en `journal.md` el tiempo de pared y la validez (desvío 0, 0 bloques de 1). **Medido: 172,6 s
  el centro completo y 183,58 s el test lento; desvío 0, separación 2, `huecosNoFinal == 0`.**
- **Regresión:** `ctest --preset backend` **61/61**, `ctest --preset full-release -E
  test_generacion_seed` **65/65** y el **test lento** verde.
- **Goldens de horario:** los que comparan un horario exacto se **relajan a validez/invariantes** o se
  fijan con **1 worker explícito solo en ese test**, y se documenta; no se deja una comparación exacta con
  el defecto de 8 workers.
- **Nada funcional depende de 1 worker:** se revisan los tests de salida exacta
  (`test_solver_preferencias` → `sinPreferencias_salidaIdenticaALaActual`, el golden de la 013 y cualquier
  comparación entre ejecuciones). El producto no usa `1` en ningún camino.

**Cambios de la 014b (reglas, informe y seed).**

- **B5b — separación blanda, huecos al final duros.** La separación vuelve a ser **blanda** con
  `incumpleSeparacion = min(apareceDia[d], apareceDia[d+1])` y peso `PESO_SEPARACION` (fase 2a); los
  **huecos al final** siguen **duros** (`ocupada[c,d,s] ≥ ocupada[c,d,s+1]`, sin `huecoNoFinal`); el
  **viernes** es blando. La causa (5) vuelve a **11 h o más** (`⌈h/2⌉ > nº_días`); **7–10 h** se informa.
  Orden blando: **S1 > S2 > separación > viernes > preferencia**. El test de aceptación asevera
  **separación informada ≤ la referencia** y **`huecosNoFinal == 0`**.
- **B6 — informe ampliado.** `MateriaSalida` lleva **curso**, **una `AparicionSalida` por aparición con sus
  franjas** y los **días incumplidos** (se informa); `CursoSalida` expone los **agregados por curso**
  (desvío, tope, huecos no-viernes); P4 detecta **hueco no sufijo** (la separación se informa, no es
  conflicto).
- **B7 — seed sintético a horas pares.** Ninguna materia impar de **laboratorio/taller** sin **pareja del
  mismo tipo**, o todas pares; se corrige el perfil y se valida (`test_seed_perfil`).

**Enmienda constitucional (§7 «Comportamiento»), texto exacto para la 014b:**

> **Enmienda (2026-11-06, spec 014).** Se sustituye «Para la misma entrada el resultado es reproducible»
> por: **para la misma entrada el resultado es válido con las reglas vigentes; la búsqueda usa 8 workers
> (semilla fija) y no se promete el mismo horario entre ejecuciones.** Los diagnósticos y los cálculos
> puros siguen siendo deterministas.

**Enmienda constitucional (§7.2), texto exacto:**

> **Enmienda (2026-11-06, spec 014).** Los **huecos al final del día** pasan a **regla dura**; la
> **separación de un día entre apariciones** y el **viernes** (que los huecos caigan preferentemente en
> viernes) son prioridades **blandas**, en el orden **separación > viernes**. Orden blando completo:
> **S1 > S2 > separación > viernes > preferencia**. La causa **(5)** cubre las materias de **11 h o más**
> (más apariciones que días); **7–10 h** se informa. **S3 eliminada.**

**Supersesiones:** la **005** (RNF-1) y la **013** (RNF-2) ya llevan la nota en sus specs; el **RF-13** de
la 014 la recoge. El contrato documentado (`docs/contrato-solver-2.0.md`) se revisa si cita «1 worker» o
«reproducible».

**Tareas:** B1, B1b, B2, B3, B4, **B5b, B6 y B7** en `tasks.md` (lote 014b, ≤10).
