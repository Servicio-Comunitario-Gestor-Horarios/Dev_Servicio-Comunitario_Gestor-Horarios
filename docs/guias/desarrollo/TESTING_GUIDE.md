# Testing Guide — Gestor-Horarios

## Índice

1. [¿Qué es un framework de testing?](#1-que-es-un-framework-de-testing)
2. [Ecosistema de tests](#2-ecosistema-de-tests)
3. [QTest — tests nativos Qt](#3-qtest--tests-nativos-qt)
4. [Google Test — tests de lógica general](#4-google-test--tests-de-logica-general)
5. [Convenciones y buenas prácticas](#5-convenciones-y-buenas-practicas)
6. [Flujo de trabajo](#6-flujo-de-trabajo)
7. [Referencia rápida](#7-referencia-rapida)

---

## 1. ¿Qué es un framework de testing?

Un **framework de testing** es una biblioteca que te permite escribir pruebas automáticas
para verificar que tu código funciona correctamente. En lugar de compilar, ejecutar
y revisar a mano cada vez, escribes la prueba una vez y la ejecutas con un solo comando.

### ¿Por qué necesitamos uno?

```cpp
// Sin framework: tienes que hacer todo a mano
int main() {
    FranjaHoraria f;
    f.dia = 1;
    // ... configurar, ejecutar, imprimir, comparar manualmente
    QJsonObject obj = f.toJson();
    if (obj["dia"].toInt() != 1) {
        std::cerr << "Error: dia no coincide\n";
        return 1;
    }
    return 0;
}

// Con framework (QTest):
class TestFranjaHoraria : public QObject {
    Q_OBJECT
    void testToJson() {
        FranjaHoraria f;
        f.dia = 1;
        QJsonObject obj = f.toJson();
        QCOMPARE(obj["dia"].toInt(), 1);  // ← se verifica solo
    }
};
```

### Beneficios

| Sin framework | Con framework |
|---|---|
| Tienes que compilar y ejecutar a mano | `ctest` ejecuta todo automáticamente |
| Tienes que imprimir y comparar resultados | Las aserciones lo hacen por ti |
| Un error detiene todo | Los tests son independientes |
| No hay reporte estructurado | Reporte claro: cuántos pasaron, cuáles fallaron |
| Difícil organizar cientos de tests | Se agrupan en suites, tags, filtros |

---

## 2. Ecosistema de tests

### 2.1 Estructura de directorios

```
test/
├── CMakeLists.txt          ← Registro de todos los tests
├── backend/                ← Tests del módulo backend
│   ├── test_franja_horaria.cpp
│   ├── test_aula.cpp
│   ├── test_materia.cpp
│   ├── test_profesor.cpp
│   ├── test_horario.cpp
│   └── test_plan_estudio.cpp
├── frontend/               ← Tests del módulo frontend
│   └── (a futuro)
└── middleware/              ← Tests del módulo middleware
    └── (a futuro)
```

### 2.2 Dos frameworks, un propósito

Usamos **dos frameworks** que conviven en el mismo proyecto:

| Framework | Para qué |
|---|---|
| **QTest** | Tests que involucran tipos Qt (`QTime`, `QString`, `QJsonObject`). Ideal para serialización, modelos de datos, y cualquier código que use el framework Qt. |
| **Google Test** | Tests de lógica general que no dependen de Qt. Algoritmos, solver OR-Tools, reglas de negocio, y lógica pura C++17. |

Ambos se ejecutan con el mismo comando (`ctest`) y se configuran desde el mismo
`CMakeLists.txt`.

### 2.3 Cómo se ejecutan

```bash
# Compilar todo (código + tests)
cmake --build build

# Ejecutar todos los tests
ctest --test-dir build/test

# Ejecutar tests que coincidan con un patrón
ctest --test-dir build/test -R franja       # solo tests con "franja"
ctest --test-dir build/test -R backend      # solo tests de backend

# Ejecutar con salida detallada
ctest --test-dir build/test --output-on-failure

# Ejecutar un test directamente (sin ctest)
./build/test/test_backend_franja_horaria

# Ejecutar un test específico dentro de un archivo
./build/test/test_backend_franja_horaria toJson_returnsCorrectFields
```

### 2.4 Cómo se configura un test nuevo

En `test/CMakeLists.txt` hay dos funciones helper:

```cmake
# Para tests con QTest (tipos Qt, Q_OBJECT, AUTOMOC)
add_qtest(NOMBRE_EJECUTABLE
    ruta/al/archivo.cpp
    librerias_a_linkear...
)

# Para tests con Google Test (lógica general)
add_gtest(NOMBRE_EJECUTABLE
    ruta/al/archivo.cpp
    librerias_a_linkear...
)
```

Ejemplo real:

```cmake
add_qtest(test_backend_franja_horaria
    backend/test_franja_horaria.cpp
    backend           # ← linkea la librería backend
)
```

---

## 3. QTest — tests nativos Qt

### 3.1 ¿Qué es?

QTest es el framework de testing que viene con Qt6. Está diseñado para trabajar
de forma natural con tipos Qt (`QString`, `QTime`, `QJsonObject`, `QVector`, etc.).

### 3.2 Estructura de un test

Todo test QTest sigue esta plantilla:

```cpp
#include <QTest>
#include <QJsonObject>
#include <backend/data/mi_struct.hpp>

class TestMiStruct : public QObject {
    Q_OBJECT                          // ← OBLIGATORIO para que funcione QTest

private slots:                        // ← Cada método aquí es un test individual
    void testUnaCosa() {
        // ... preparar, ejecutar, verificar
    }

    void testOtraCosa() {
        // ...
    }
};

QTEST_MAIN(TestMiStruct)              // ← Genera el main() automáticamente
#include "test_mi_struct.moc"         // ← OBLIGATORIO: procesa Q_OBJECT
```

### 3.3 Macros de aserción

| Macro | Qué hace | Falla si… |
|---|---|---|
| `QCOMPARE(actual, esperado)` | Compara dos valores con `==` | No son iguales |
| `QVERIFY(condición)` | Verifica que una condición sea true | Es false |
| `QVERIFY2(condición, mensaje)` | Igual que QVERIFY pero con mensaje personalizado | Es false |
| `QFAIL(mensaje)` | Falla el test inmediatamente | Siempre (se usa en condicionales) |
| `QCOMPARE_EQ(actual, esperado)` | Como QCOMPARE pero permite tipos diferentes | No son iguales |

> **Importante:** `QCOMPARE` usa `operator==` del tipo. Para tipos Qt como `QTime`,
> `QString`, `QJsonObject` funciona directamente sin conversiones.

### 3.4 Ciclo de vida de un test

QTest provee métodos especiales que se ejecutan automáticamente:

```cpp
class TestEjemplo : public QObject {
    Q_OBJECT

private slots:
    // Se ejecuta UNA VEZ antes de todos los tests
    void initTestCase() { /* abrir base de datos, cargar configuración */ }

    // Se ejecuta ANTES de cada test
    void init() { /* crear objetos fresh para cada prueba */ }

    // Los tests propiamente dichos
    void testUno() { /* ... */ }
    void testDos() { /* ... */ }

    // Se ejecuta DESPUÉS de cada test
    void cleanup() { /* limpiar objetos creados en init() */ }

    // Se ejecuta UNA VEZ después de todos los tests
    void cleanupTestCase() { /* cerrar base de datos */ }
};
```

### 3.5 Ejemplo completo

```cpp
#include <QTest>
#include <QJsonObject>
#include <backend/data/aula.hpp>

class TestAula : public QObject {
    Q_OBJECT

private slots:
    void toJson_returnsCorrectFields() {
        Aula a;
        a.nombre = "A-101";
        a.capacidad = 30;
        a.locacion = "Edificio A";

        QJsonObject obj = a.toJson();

        // QCOMPARE funciona directo con QString
        QCOMPARE(obj["nombre"].toString(), QString("A-101"));
        QCOMPARE(obj["capacidad"].toInt(), 30);
    }

    void fromJson_reconstructsObject() {
        QJsonObject obj;
        obj["nombre"] = "B-203";
        obj["capacidad"] = 45;
        obj["locacion"] = "Edificio B";

        Aula a = Aula::fromJson(obj);

        QCOMPARE(a.nombre, QString("B-203"));
        QCOMPARE(a.capacidad, 45);
    }

    void roundtrip_toJsonFromJson_identity() {
        Aula original;
        original.nombre = "Lab-1";
        original.capacidad = 20;
        original.locacion = "Laboratorios";

        // toJson → fromJson debe dar el mismo objeto
        QJsonObject json = original.toJson();
        Aula result = Aula::fromJson(json);

        QCOMPARE(result.nombre, original.nombre);
        QCOMPARE(result.capacidad, original.capacidad);
        QCOMPARE(result.locacion, original.locacion);
    }
};

QTEST_MAIN(TestAula)
#include "test_aula.moc"
```

### 3.6 Salida de QTest

```
********* Start testing of TestAula *********
Config: Using QtTest library 6.4.2, Qt 6.4.2
PASS   : TestAula::initTestCase()
PASS   : TestAula::toJson_returnsCorrectFields()
PASS   : TestAula::fromJson_reconstructsObject()
PASS   : TestAula::roundtrip_toJsonFromJson_identity()
PASS   : TestAula::cleanupTestCase()
Totals: 5 passed, 0 failed, 0 skipped, 0 blacklisted, 12ms
********* Finished testing of TestAula *********
```

Si un test falla, QTest muestra el valor actual vs el esperado y la línea exacta.

### 3.7 Limitaciones de QTest

- ❌ No tiene **mocks** (simular dependencias)
- ❌ Las fixtures (init/cleanup) son por clase, no anidadas
- ❌ Cada archivo genera un ejecutable separado
- ✅ Pero es perfecto para tests que usan tipos Qt

---

## 4. Google Test — tests de lógica general

### 4.1 ¿Qué es?

Google Test (gtest) es el framework de testing más usado en C++. Desarrollado por
Google, es el estándar de la industria para proyectos C++ modernos. Incluye:

- **gtest**: el framework base (aserciones, fixtures, test discovery)
- **gmock**: biblioteca de mocks (simular objetos y verificar interacciones)

### 4.2 Estructura de un test

```cpp
#include <gtest/gtest.h>

// Test simple
TEST(NombreSuite, NombreTest) {
    // ... preparar, ejecutar, verificar
}

// Test con fixture (setup/teardown reutilizable)
class MiFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // se ejecuta antes de cada test
    }

    void TearDown() override {
        // se ejecuta después de cada test
    }
};

TEST_F(MiFixture, TestConFixture) {
    // usa los recursos configurados en SetUp()
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 4.3 Macros de aserción

Google Test tiene dos familias de macros:

**`EXPECT_*`** — no fatales: si fallan, el test continúa (reporta el error pero sigue).

**`ASSERT_*`** — fatales: si fallan, el test se detiene ahí.

| Macro | Verifica | Versión fatal |
|---|---|---|
| `EXPECT_EQ(a, b)` | `a == b` | `ASSERT_EQ(a, b)` |
| `EXPECT_NE(a, b)` | `a != b` | `ASSERT_NE(a, b)` |
| `EXPECT_LT(a, b)` | `a < b` | `ASSERT_LT(a, b)` |
| `EXPECT_GT(a, b)` | `a > b` | `ASSERT_GT(a, b)` |
| `EXPECT_TRUE(cond)` | `cond` es true | `ASSERT_TRUE(cond)` |
| `EXPECT_FALSE(cond)` | `cond` es false | `ASSERT_FALSE(cond)` |
| `EXPECT_STREQ(a, b)` | strings `char*` iguales | `ASSERT_STREQ(a, b)` |
| `EXPECT_THAT(val, matcher)` | val cumple el matcher | `ASSERT_THAT(val, matcher)` |

### 4.4 Matchers (gmock)

Los matchers permiten aserciones más expresivas:

```cpp
#include <gmock/gmock.h>

EXPECT_THAT(vector, SizeIs(3));
EXPECT_THAT(vector, Contains("elemento"));
EXPECT_THAT(nombre, StartsWith("Dr."));
EXPECT_THAT(lista, Each(Gt(0)));    // todos los elementos > 0
EXPECT_THAT(valor, DoubleNear(3.14, 0.01));  // comparación con tolerancia
```

### 4.5 Ejemplo completo

```cpp
#include <gtest/gtest.h>
#include <QJsonObject>
#include <QTime>
#include <backend/data/franja_horaria.hpp>

TEST(FranjaHorariaTest, ToJsonProducesCorrectFields) {
    FranjaHoraria f;
    f.dia = 1;
    f.inicio = QTime(8, 0);
    f.fin = QTime(10, 30);

    QJsonObject obj = f.toJson();

    EXPECT_EQ(obj["dia"].toInt(), 1);
    // Con tipos Qt necesitamos .toString() y .toStdString()
    EXPECT_EQ(obj["inicio"].toString().toStdString(), "08:00");
}

TEST(FranjaHorariaTest, FromJsonReconstructsObject) {
    QJsonObject obj;
    obj["dia"] = 3;
    obj["inicio"] = "09:00";
    obj["fin"] = "11:00";

    FranjaHoraria f = FranjaHoraria::fromJson(obj);

    EXPECT_EQ(f.dia, 3);
    EXPECT_EQ(f.inicio, QTime(9, 0));
}
```

> **Nota:** Con Google Test, los tipos Qt como `QTime` o `QString` requieren
> conversión a tipos estándar para comparar. Usa `.toString()` para `QTime`,
> `.toStdString()` para `QString`.

### 4.6 Fixtures con Google Test

Los fixtures permiten reutilizar configuración entre tests:

```cpp
class SolverFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Crear datos de prueba compartidos
        horario = new Horario();
        // Cargar materias, aulas, etc.
    }

    void TearDown() override {
        delete horario;
    }

    Horario* horario;
    Profesor profe;
};

TEST_F(SolverFixture, AsignacionBasica) {
    auto resultado = solver->asignar(horario);
    EXPECT_TRUE(resultado.valido);
}

TEST_F(SolverFixture, SinProfesoresDisponibles) {
    profe.disponibilidad.clear();
    auto resultado = solver->asignar(horario);
    EXPECT_FALSE(resultado.valido);
}
```

### 4.7 Mocks con gmock

Los mocks te permiten simular objetos que todavía no existen o que son difíciles
de configurar (ej: base de datos, API externa):

```cpp
#include <gmock/gmock.h>

// Interfaz que queremos mockear
class Database {
public:
    virtual ~Database() = default;
    virtual QVector<Materia> getMaterias() = 0;
};

// Mock generado con gmock
class MockDatabase : public Database {
public:
    MOCK_METHOD(QVector<Materia>, getMaterias, (), (override));
};

// Test que usa el mock
TEST(ServiceTest, CargaMaterias) {
    MockDatabase db;
    EXPECT_CALL(db, getMaterias())
        .Times(1)
        .WillOnce(testing::Return(QVector<Materia>{m1, m2}));

    Service s(&db);
    auto resultado = s.cargarMaterias();
    EXPECT_EQ(resultado.size(), 2);
}
```

### 4.8 Salida de Google Test

```
[==========] Running 3 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 3 tests from FranjaHorariaTest
[ RUN      ] FranjaHorariaTest.ToJsonProducesCorrectFields
[       OK ] FranjaHorariaTest.ToJsonProducesCorrectFields (0 ms)
[ RUN      ] FranjaHorariaTest.FromJsonReconstructsObject
[       OK ] FranjaHorariaTest.FromJsonReconstructsObject (0 ms)
[ RUN      ] FranjaHorariaTest.RoundtripIdentity
[       OK ] FranjaHorariaTest.RoundtripIdentity (0 ms)
[----------] 3 tests from FranjaHorariaTest (0 ms total)
[==========] 3 tests ran (0 ms total)
[  PASSED  ] 3 tests.
```

---

## 5. Convenciones y buenas prácticas

### 5.1 Nombres de archivos

```
test_<nombre_struct>().cpp     ← snake_case, en inglés o español
```

Ejemplos:
- `test_franja_horaria.cpp` — test para `FranjaHoraria`
- `test_aula.cpp` — test para `Aula`

### 5.2 Nombres de tests (QTest)

Los nombres de los métodos deben describir **qué** prueban y **en qué escenario**:

```
verbo_escenario_resultadoEsperado
```

Ejemplos:
- `toJson_returnsCorrectFields`
- `fromJson_reconstructsObject`
- `roundtrip_toJsonFromJson_identity`
- `requirimientos_emptyWhenNotPresent`
- `disponibilidad_emptyWhenNotPresent`

### 5.3 Nombres de tests (Google Test)

Con GTest se usan `TEST(Suite, Test)`:

```cpp
TEST(FranjaHorariaTest, ToJsonReturnsCorrectFields)
TEST(FranjaHorariaTest, FromJsonReconstructsObject)
TEST(ProfesorTest, DisponibilidadVacia)
```

La **Suite** agrupa tests relacionados (generalmente el nombre de la struct/clase).
El **Test** describe el escenario en camelCase.

### 5.4 Estructura de cada test (AAA)

Cada test debe seguir el patrón **Arrange-Act-Assert**:

```cpp
void test_algo() {
    // 1. ARRANGE — preparar datos
    FranjaHoraria f;
    f.dia = 1;
    f.inicio = QTime(8, 0);

    // 2. ACT — ejecutar la acción
    QJsonObject obj = f.toJson();

    // 3. ASSERT — verificar el resultado
    QCOMPARE(obj["dia"].toInt(), 1);
}
```

### 5.5 Cobertura mínima por struct

Cada struct con `toJson()`/`fromJson()` debe tener al menos:

| Test | Qué verifica |
|---|---|
| `toJson_returnsCorrectFields` | Que los campos se serializan correctamente |
| `fromJson_reconstructsObject` | Que se reconstruye desde JSON |
| `roundtrip_toJsonFromJson_identity` | Que toJson → fromJson da el mismo objeto |
| (opcional) Casos borde | Campos vacíos, valores por defecto, arrays vacíos |

### 5.6 Independencia

- **Los tests no deben depender unos de otros.** Cada test debe poder ejecutarse
  solo y en cualquier orden.
- No compartas estado mutable entre tests. Usa `init()`/`SetUp()` para crear
  datos frescos en cada test.

### 5.7 ¿QTest o Google Test? — Guía práctica

| Situación | Framework |
|---|---|
| Probar serialización de structs con `QJsonObject` | QTest |
| Probar código con `QTime`, `QString`, `QVector` | QTest |
| Probar lógica pura C++ (sin Qt) | Google Test |
| Necesitas mocks (base de datos, APIs) | Google Test (gmock) |
| Necesitas fixtures complejos con setup/teardown | Google Test |
| Test de algoritmos OR-Tools | Google Test |

---

## 6. Flujo de trabajo

### 6.1 Escribir un test nuevo

1. Crear el archivo en `test/<modulo>/test_<nombre>.cpp`
2. Elegir el framework según la tabla de la sección 5.7
3. Escribir los tests siguiendo las convenciones de la sección 5
4. Agregar el test en `test/CMakeLists.txt` usando `add_qtest()` o `add_gtest()`
5. Compilar: `cmake --build build`
6. Ejecutar: `ctest --test-dir build/test --output-on-failure`

### 6.2 Antes de hacer commit

```bash
# 1. Compilar todo (confirmar que no hay errores de compilación)
cmake --build build

# 2. Ejecutar todos los tests (confirmar que no hay regresiones)
ctest --test-dir build/test

# 3. Si algo falla, arreglarlo antes del commit
```

### 6.3 Depurar un test que falla

```bash
# Ejecutar solo el test que falla (más rápido)
./build/test/test_backend_franja_horaria

# Ejecutar un test específico dentro del archivo
./build/test/test_backend_franja_horaria toJson_returnsCorrectFields

# Con verbose (-v) para más detalle
./build/test/test_backend_franja_horaria -v
```

---

## 7. Referencia rápida

### Comandos

```bash
cmake --build build                           # compilar todo (código + tests)
ctest --test-dir build/test                   # ejecutar todos los tests
ctest --test-dir build/test -R franja         # filtrar por nombre
ctest --test-dir build/test --output-on-failure  # mostrar detalles de fallos
./build/test/test_backend_aula                # ejecutar un test directamente
./build/test/test_backend_aula -v             # ejecutar con verbose
```

### QTest — macros esenciales

```cpp
QCOMPARE(actual, esperado)        // comparación exacta
QVERIFY(condicion)                // condición verdadera
QVERIFY2(condicion, "mensaje")    // condición con mensaje
QFAIL("mensaje")                  // fallo forzado
```

### Google Test — macros esenciales

```cpp
EXPECT_EQ(actual, esperado)       // comparación (no fatal)
ASSERT_EQ(actual, esperado)       // comparación (fatal)
EXPECT_TRUE(cond)                 // condición verdadera
EXPECT_THAT(val, Matcher())       // matcher expresivo
```

### test/CMakeLists.txt — funciones helper

```cmake
add_qtest(nombre path.cpp lib1 lib2)    # registrar test QTest
add_gtest(nombre path.cpp lib1 lib2)    # registrar test Google Test
```
