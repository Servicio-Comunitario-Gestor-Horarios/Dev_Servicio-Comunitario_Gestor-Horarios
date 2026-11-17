# Conjunto de datos del QA — réplica del centro real

Datos **exactos** para cargar a mano la réplica del Liceo Nacional Robert Serra
(planes **31059** y **31060**, año escolar 2026/2027) sobre una base vacía.
Este es el conjunto con el que se ejecuta la revisión manual de
[`revision-manual-qa.md`](revision-manual-qa.md): **no** se entrega ninguna base
ni preset precargado.

> **Volumen:** 2 turnos, 3 eventos, 2 tipos de aula, 13 aulas, 85 materias, 2 planes
> con 95 materias-año, 33 docentes, 272 asignaciones docente–materia, 165
> disponibilidades y 24 cursos. Es una carga larga (varias sesiones): **carga un
> bloque, verifícalo y cierra la aplicación antes de pasar al siguiente**. Si algo
> no cuadra, repórtalo antes de seguir: los errores se propagan y luego no se sabe
> de dónde vienen.

## Orden obligatorio de carga

```
B0  Turnos y eventos        (consola, fuera de la interfaz)
B1  Tipos de aula           (interfaz)
B2  Aulas                   (interfaz)
B3  Materias                (interfaz)
B4  Planes de estudio       (interfaz)
B5  Docentes                (interfaz)
B6  Materias por docente    (interfaz)
B7  Disponibilidad          (interfaz)
B8  Cursos / años-sección   (interfaz)
B9  Parámetros del generador(interfaz)
```

Cada bloque termina con sus **comprobaciones**. No sigas si alguna falla.

---

## B0 — Turnos y eventos (con la consola, antes de abrir la aplicación)

La interfaz no permite crear turnos ni recesos. Se crean con la consola incluida
en la entrega (`gestor-horarios-consola.exe`). **Cierra la aplicación** y ejecuta
desde la carpeta del paquete:

```bat
set DB=%APPDATA%\LiceoRobertSerra\GestorHorarios\gestor_horarios.db

gestor-horarios-consola --db "%DB%" ^
  --cmd "turnos crear Mañana 07:00 12:40 8" ^
  --cmd "turnos crear Tarde 12:40 18:10 8" ^
  --cmd "turnos evento-agregar Mañana Receso 2 10 08:20 08:30 1" ^
  --cmd "turnos evento-agregar Mañana \"Acto cívico\" 2 10 08:30 08:40 2" ^
  --cmd "turnos evento-agregar Tarde Receso 2 10 14:00 14:10 1" ^
  --cmd "turnos listar" ^
  --cmd "turnos eventos Mañana" ^
  --cmd "turnos eventos Tarde"
```

| Turno | Inicio | Fin | Franjas de 40 min |
|---|---|---|---|
| Mañana | 07:00 | 12:40 | 8 |
| Tarde | 12:40 | 18:10 | 8 |

| Turno | Evento | Después de la franja | Duración | Horario | Orden |
|---|---|---|---|---|---|
| Mañana | Receso | 2 | 10 min | 08:20–08:30 | 1 |
| Mañana | Acto cívico | 2 | 10 min | 08:30–08:40 | 2 |
| Tarde | Receso | 2 | 10 min | 14:00–14:10 | 1 |

**Comprobaciones B0**

- `turnos listar` devuelve exactamente **2** filas: `Mañana 07:00 12:40 8` y `Tarde 12:40 18:10 8`.
- `turnos eventos` devuelve **2** para Mañana (Receso y Acto cívico, ambos tras la franja 2) y **1** para Tarde.
- Si al abrir la aplicación **Generación** dice «0 franjas/día» o el resumen de parámetros no muestra la duración, B0 no se aplicó: revisa la ruta `%DB%`.

---

## B1 — Tipos de aula (2)

| Tipo de aula | Nota |
|---|---|
| Aula de estudio | Tipo general: ya existe; **no se puede renombrar ni borrar** |
| Cancha | Créalo con `Nuevo tipo de aula` (se usa en Educación Física) |

**Comprobaciones B1:** el desplegable de tipo de aula de Aulas y de Materias ofrece
exactamente esos dos; intentar borrar «Aula de estudio» falla con un mensaje que lo
explica.

---

## B2 — Aulas (13)

| # | Nombre | Capacidad | Tipo de aula | Edificio | Piso |
|---|---|---|---|---|---|
| 1 | Aula 01 | 44 | Aula de estudio | A | Planta baja |
| 2 | Aula 02 | 44 | Aula de estudio | A | Planta baja |
| 3 | Aula 03 | 44 | Aula de estudio | A | Planta baja |
| 4 | Aula 04 | 44 | Aula de estudio | A | Piso 1 |
| 5 | Aula 05 | 44 | Aula de estudio | B | Planta baja |
| 6 | Aula 06 | 44 | Aula de estudio | B | Planta baja |
| 7 | Aula 07 | 44 | Aula de estudio | B | Planta baja |
| 8 | Aula 08 | 44 | Aula de estudio | B | Piso 1 |
| 9 | Aula 09 | 44 | Aula de estudio | C | Planta baja |
| 10 | Aula 10 | 44 | Aula de estudio | C | Planta baja |
| 11 | Aula 11 | 44 | Aula de estudio | C | Piso 1 |
| 12 | Aula 12 | 44 | Aula de estudio | C | Piso 1 |
| 13 | Cancha | 88 | Cancha | Patio | Descubierto |

**Comprobaciones B2**

- Inicio → **Aulas = 13**; la tabla lista 13 filas.
- **12** de tipo `Aula de estudio` (capacidad 44) y **1** de tipo `Cancha` (capacidad **88**).
- Capacidades: 12 aulas de 44 y la cancha de 88; ninguna con capacidad 0.
- El motor admite **2 grupos simultáneos** en la cancha (regla del centro, no editable).

---

## B3 — Materias (85)

**80** de tipo `Aula de estudio` y **5** de tipo
`Cancha` (las cinco de Educación Física).

| # | Nombre de la materia | Tipo de aula |
|---|---|---|
| 1 | Castellano 1º | Aula de estudio |
| 2 | Inglés y otras lenguas extranjeras 1º | Aula de estudio |
| 3 | Matemáticas 1º | Aula de estudio |
| 4 | Educación Física 1º | Cancha |
| 5 | Arte y Patrimonio 1º | Aula de estudio |
| 6 | Ciencias Naturales 1º | Aula de estudio |
| 7 | Geografía, Historia y Ciudadanía 1º | Aula de estudio |
| 8 | Orientación y Convivencia 1º | Aula de estudio |
| 9 | Lengua y Literatura 1º | Aula de estudio |
| 10 | Idiomas 1º | Aula de estudio |
| 11 | Biología, Ambiente y Tecnología 1º | Aula de estudio |
| 12 | Física 1º | Aula de estudio |
| 13 | Química 1º | Aula de estudio |
| 14 | Geografía, Historia y Soberanía Nacional 1º | Aula de estudio |
| 15 | Orientación Vocacional 1º | Aula de estudio |
| 16 | Innovación Tecnológica y Productividad 1º | Aula de estudio |
| 17 | Castellano 2º | Aula de estudio |
| 18 | Inglés y otras lenguas extranjeras 2º | Aula de estudio |
| 19 | Matemáticas 2º | Aula de estudio |
| 20 | Educación Física 2º | Cancha |
| 21 | Arte y Patrimonio 2º | Aula de estudio |
| 22 | Ciencias Naturales 2º | Aula de estudio |
| 23 | Geografía, Historia y Ciudadanía 2º | Aula de estudio |
| 24 | Orientación y Convivencia 2º | Aula de estudio |
| 25 | Lengua y Literatura 2º | Aula de estudio |
| 26 | Idiomas 2º | Aula de estudio |
| 27 | Biología, Ambiente y Tecnología 2º | Aula de estudio |
| 28 | Física 2º | Aula de estudio |
| 29 | Química 2º | Aula de estudio |
| 30 | Geografía, Historia y Soberanía Nacional 2º | Aula de estudio |
| 31 | Orientación Vocacional 2º | Aula de estudio |
| 32 | Innovación Tecnológica y Productividad 2º | Aula de estudio |
| 33 | Castellano 3º | Aula de estudio |
| 34 | Inglés y otras lenguas extranjeras 3º | Aula de estudio |
| 35 | Matemáticas 3º | Aula de estudio |
| 36 | Educación Física 3º | Cancha |
| 37 | Física 3º | Aula de estudio |
| 38 | Química 3º | Aula de estudio |
| 39 | Biología 3º | Aula de estudio |
| 40 | Geografía, Historia y Ciudadanía 3º | Aula de estudio |
| 41 | Orientación y Convivencia 3º | Aula de estudio |
| 42 | Lengua y Literatura 3º | Aula de estudio |
| 43 | Idiomas 3º | Aula de estudio |
| 44 | Biología, Ambiente y Tecnología 3º | Aula de estudio |
| 45 | Geografía, Historia y Soberanía Nacional 3º | Aula de estudio |
| 46 | Orientación Vocacional 3º | Aula de estudio |
| 47 | Innovación Tecnológica y Productividad 3º | Aula de estudio |
| 48 | Castellano 4º | Aula de estudio |
| 49 | Inglés y otras lenguas extranjeras 4º | Aula de estudio |
| 50 | Matemáticas 4º | Aula de estudio |
| 51 | Educación Física 4º | Cancha |
| 52 | Física 4º | Aula de estudio |
| 53 | Química 4º | Aula de estudio |
| 54 | Biología 4º | Aula de estudio |
| 55 | Geografía, Historia y Ciudadanía 4º | Aula de estudio |
| 56 | Formación para la Soberanía Nacional 4º | Aula de estudio |
| 57 | Orientación y Convivencia 4º | Aula de estudio |
| 58 | Lengua y Literatura 4º | Aula de estudio |
| 59 | Idiomas 4º | Aula de estudio |
| 60 | Biología, Ambiente y Tecnología 4º | Aula de estudio |
| 61 | Geografía, Historia y Soberanía Nacional 4º | Aula de estudio |
| 62 | Orientación Vocacional 4º | Aula de estudio |
| 63 | Innovación Tecnológica y Productividad 4º | Aula de estudio |
| 64 | Castellano 5º | Aula de estudio |
| 65 | Inglés y otras lenguas extranjeras 5º | Aula de estudio |
| 66 | Matemáticas 5º | Aula de estudio |
| 67 | Educación Física 5º | Cancha |
| 68 | Física 5º | Aula de estudio |
| 69 | Química 5º | Aula de estudio |
| 70 | Biología 5º | Aula de estudio |
| 71 | Geografía, Historia y Ciudadanía 5º | Aula de estudio |
| 72 | Formación para la Soberanía Nacional 5º | Aula de estudio |
| 73 | Orientación y Convivencia 5º | Aula de estudio |
| 74 | Lengua y Literatura 5º | Aula de estudio |
| 75 | Idiomas 5º | Aula de estudio |
| 76 | Biología, Ambiente y Tecnología 5º | Aula de estudio |
| 77 | Geografía, Historia y Soberanía Nacional 5º | Aula de estudio |
| 78 | Orientación Vocacional 5º | Aula de estudio |
| 79 | Innovación Tecnológica y Productividad 5º | Aula de estudio |
| 80 | Participación en los Grupos de Creación, Recreación y Producción 1º | Aula de estudio |
| 81 | Participación en los Grupos de Creación, Recreación y Producción 2º | Aula de estudio |
| 82 | Participación en los Grupos de Creación, Recreación y Producción 3º | Aula de estudio |
| 83 | Participación en los Grupos de Creación, Recreación y Producción 4º | Aula de estudio |
| 84 | Participación en los Grupos de Creación, Recreación y Producción 5º | Aula de estudio |
| 85 | Ciencias de la Tierra 5º | Aula de estudio |

**Materias sin carga (6):** existen en el catálogo pero **no** van en
ningún plan. Son a propósito: sirven para docentes habilitados sin materia que
impartir.

- Participación en los Grupos de Creación, Recreación y Producción 1º
- Participación en los Grupos de Creación, Recreación y Producción 2º
- Participación en los Grupos de Creación, Recreación y Producción 3º
- Participación en los Grupos de Creación, Recreación y Producción 4º
- Participación en los Grupos de Creación, Recreación y Producción 5º
- Ciencias de la Tierra 5º

**Comprobaciones B3**

- Inicio → **Materias = 85**.
- Las cinco materias `Educación Física 1º` … `Educación Física 5º` son de tipo `Cancha`; el resto, `Aula de estudio`.
- Ninguna materia con nombre repetido.

---

## B4 — Planes de estudio (2) y sus materias (95 filas)

| Código | Nombre | Descripción |
|---|---|---|
| 31059 | Media General | Media General |
| 31060 | Ciencias y Tecnología | Ciencias y Tecnología |

En `+ Registrar Plan de Estudio` desmarca **«Plan global»** para poder repartir las
materias por año (cada fila de abajo indica el año de esa materia).

### Plan 31059 — Media General

| Año | Materia | Horas/semana |
|---|---|---|
| 1.º | Castellano 1º | 4 |
| 1.º | Inglés y otras lenguas extranjeras 1º | 4 |
| 1.º | Matemáticas 1º | 4 |
| 1.º | Educación Física 1º | 4 |
| 1.º | Arte y Patrimonio 1º | 4 |
| 1.º | Ciencias Naturales 1º | 6 |
| 1.º | Geografía, Historia y Ciudadanía 1º | 6 |
| 1.º | Orientación y Convivencia 1º | 2 |
| | **Total 1.º** | **34** |
| 2.º | Castellano 2º | 4 |
| 2.º | Inglés y otras lenguas extranjeras 2º | 4 |
| 2.º | Matemáticas 2º | 4 |
| 2.º | Educación Física 2º | 4 |
| 2.º | Arte y Patrimonio 2º | 4 |
| 2.º | Ciencias Naturales 2º | 6 |
| 2.º | Geografía, Historia y Ciudadanía 2º | 6 |
| 2.º | Orientación y Convivencia 2º | 2 |
| | **Total 2.º** | **34** |
| 3.º | Castellano 3º | 4 |
| 3.º | Inglés y otras lenguas extranjeras 3º | 4 |
| 3.º | Matemáticas 3º | 4 |
| 3.º | Educación Física 3º | 4 |
| 3.º | Física 3º | 4 |
| 3.º | Química 3º | 4 |
| 3.º | Biología 3º | 4 |
| 3.º | Geografía, Historia y Ciudadanía 3º | 4 |
| 3.º | Orientación y Convivencia 3º | 2 |
| | **Total 3.º** | **34** |
| 4.º | Castellano 4º | 4 |
| 4.º | Inglés y otras lenguas extranjeras 4º | 4 |
| 4.º | Matemáticas 4º | 4 |
| 4.º | Educación Física 4º | 4 |
| 4.º | Física 4º | 4 |
| 4.º | Química 4º | 4 |
| 4.º | Biología 4º | 4 |
| 4.º | Geografía, Historia y Ciudadanía 4º | 4 |
| 4.º | Formación para la Soberanía Nacional 4º | 2 |
| 4.º | Orientación y Convivencia 4º | 2 |
| | **Total 4.º** | **36** |
| 5.º | Castellano 5º | 3 |
| 5.º | Inglés y otras lenguas extranjeras 5º | 3 |
| 5.º | Matemáticas 5º | 4 |
| 5.º | Educación Física 5º | 4 |
| 5.º | Física 5º | 4 |
| 5.º | Química 5º | 4 |
| 5.º | Biología 5º | 4 |
| 5.º | Geografía, Historia y Ciudadanía 5º | 4 |
| 5.º | Formación para la Soberanía Nacional 5º | 2 |
| 5.º | Orientación y Convivencia 5º | 2 |
| | **Total 5.º** | **34** |

| Año | 1.º | 2.º | 3.º | 4.º | 5.º |
|---|---|---|---|---|---|
| Horas | 34 | 34 | 34 | 36 | 34 |

### Plan 31060 — Ciencias y Tecnología

| Año | Materia | Horas/semana |
|---|---|---|
| 1.º | Matemáticas 1º | 4 |
| 1.º | Educación Física 1º | 4 |
| 1.º | Lengua y Literatura 1º | 4 |
| 1.º | Idiomas 1º | 4 |
| 1.º | Biología, Ambiente y Tecnología 1º | 4 |
| 1.º | Física 1º | 2 |
| 1.º | Química 1º | 2 |
| 1.º | Geografía, Historia y Soberanía Nacional 1º | 6 |
| 1.º | Orientación Vocacional 1º | 4 |
| 1.º | Innovación Tecnológica y Productividad 1º | 6 |
| | **Total 1.º** | **40** |
| 2.º | Matemáticas 2º | 4 |
| 2.º | Educación Física 2º | 4 |
| 2.º | Lengua y Literatura 2º | 4 |
| 2.º | Idiomas 2º | 4 |
| 2.º | Biología, Ambiente y Tecnología 2º | 4 |
| 2.º | Física 2º | 2 |
| 2.º | Química 2º | 2 |
| 2.º | Geografía, Historia y Soberanía Nacional 2º | 6 |
| 2.º | Orientación Vocacional 2º | 4 |
| 2.º | Innovación Tecnológica y Productividad 2º | 6 |
| | **Total 2.º** | **40** |
| 3.º | Matemáticas 3º | 4 |
| 3.º | Educación Física 3º | 4 |
| 3.º | Física 3º | 4 |
| 3.º | Química 3º | 4 |
| 3.º | Lengua y Literatura 3º | 4 |
| 3.º | Idiomas 3º | 4 |
| 3.º | Biología, Ambiente y Tecnología 3º | 4 |
| 3.º | Geografía, Historia y Soberanía Nacional 3º | 4 |
| 3.º | Orientación Vocacional 3º | 2 |
| 3.º | Innovación Tecnológica y Productividad 3º | 6 |
| | **Total 3.º** | **40** |
| 4.º | Matemáticas 4º | 4 |
| 4.º | Educación Física 4º | 4 |
| 4.º | Física 4º | 4 |
| 4.º | Química 4º | 4 |
| 4.º | Lengua y Literatura 4º | 4 |
| 4.º | Idiomas 4º | 4 |
| 4.º | Biología, Ambiente y Tecnología 4º | 4 |
| 4.º | Geografía, Historia y Soberanía Nacional 4º | 4 |
| 4.º | Orientación Vocacional 4º | 2 |
| 4.º | Innovación Tecnológica y Productividad 4º | 6 |
| | **Total 4.º** | **40** |
| 5.º | Matemáticas 5º | 4 |
| 5.º | Educación Física 5º | 4 |
| 5.º | Física 5º | 4 |
| 5.º | Química 5º | 4 |
| 5.º | Lengua y Literatura 5º | 4 |
| 5.º | Idiomas 5º | 4 |
| 5.º | Biología, Ambiente y Tecnología 5º | 4 |
| 5.º | Geografía, Historia y Soberanía Nacional 5º | 4 |
| 5.º | Orientación Vocacional 5º | 2 |
| 5.º | Innovación Tecnológica y Productividad 5º | 6 |
| | **Total 5.º** | **40** |

| Año | 1.º | 2.º | 3.º | 4.º | 5.º |
|---|---|---|---|---|---|
| Horas | 40 | 40 | 40 | 40 | 40 |

**Comprobaciones B4**

- Plan **31059**: totales por año **34 / 34 / 34 / 36 / 34** h.
- Plan **31060**: totales por año **40 / 40 / 40 / 40 / 40** h.
- Todas las sumas por año son **pares** (se planifican en bloques de 2 franjas).
- Un plan **no** puede tener la misma materia en dos años: el plan 31059 y el 31060 comparten 16 materias (Matemáticas 1.º–5.º, Educación Física 1.º–5.º, Física 3.º–5.º, Química 3.º–5.º) y cada una aparece **una sola vez por plan**.
- Hay **95** materias-año en total (45 en 31059 + 50 en 31060).

---

## B5 — Docentes (33)

La interfaz exige **cédula numérica**. El centro original identifica al profesorado
como `P001`…`P033`; al cargarlo a mano usa la cédula de la tabla
(`10000001`…`10000033`) y **respeta el nombre, el correo y el teléfono**.

| # | Cédula | Nombre | Email | Teléfono | Turno |
|---|---|---|---|---|---|
| 1 | 10000001 | Ana García | p001@centro-de-prueba.edu.ve | +58 400-000-0001 | Mañana |
| 2 | 10000002 | José García | p002@centro-de-prueba.edu.ve | +58 400-000-0002 | Mañana |
| 3 | 10000003 | María García | p003@centro-de-prueba.edu.ve | +58 400-000-0003 | Mañana |
| 4 | 10000004 | Jesús García | p004@centro-de-prueba.edu.ve | +58 400-000-0004 | Mañana |
| 5 | 10000005 | Lucía García | p005@centro-de-prueba.edu.ve | +58 400-000-0005 | Mañana |
| 6 | 10000006 | Carlos García | p006@centro-de-prueba.edu.ve | +58 400-000-0006 | Mañana |
| 7 | 10000007 | Andrés García | p007@centro-de-prueba.edu.ve | +58 400-000-0007 | Mañana |
| 8 | 10000008 | Dolores García | p008@centro-de-prueba.edu.ve | +58 400-000-0008 | Mañana |
| 9 | 10000009 | Víctor García | p009@centro-de-prueba.edu.ve | +58 400-000-0009 | Mañana |
| 10 | 10000010 | Inés García | p010@centro-de-prueba.edu.ve | +58 400-000-0010 | Mañana |
| 11 | 10000011 | Ramón García | p011@centro-de-prueba.edu.ve | +58 400-000-0011 | Mañana |
| 12 | 10000012 | Ángela García | p012@centro-de-prueba.edu.ve | +58 400-000-0012 | Mañana |
| 13 | 10000013 | Félix García | p013@centro-de-prueba.edu.ve | +58 400-000-0013 | Mañana |
| 14 | 10000014 | Pilar García | p014@centro-de-prueba.edu.ve | +58 400-000-0014 | Mañana |
| 15 | 10000015 | Raúl García | p015@centro-de-prueba.edu.ve | +58 400-000-0015 | Mañana |
| 16 | 10000016 | Catalina García | p016@centro-de-prueba.edu.ve | +58 400-000-0016 | Tarde |
| 17 | 10000017 | Tomás García | p017@centro-de-prueba.edu.ve | +58 400-000-0017 | Tarde |
| 18 | 10000018 | Mercedes García | p018@centro-de-prueba.edu.ve | +58 400-000-0018 | Tarde |
| 19 | 10000019 | Gonzalo García | p019@centro-de-prueba.edu.ve | +58 400-000-0019 | Tarde |
| 20 | 10000020 | Úrsula García | p020@centro-de-prueba.edu.ve | +58 400-000-0020 | Tarde |
| 21 | 10000021 | Ana Ñáñez | p021@centro-de-prueba.edu.ve | +58 400-000-0021 | Tarde |
| 22 | 10000022 | José Ñáñez | p022@centro-de-prueba.edu.ve | +58 400-000-0022 | Tarde |
| 23 | 10000023 | María Ñáñez | p023@centro-de-prueba.edu.ve | +58 400-000-0023 | Tarde |
| 24 | 10000024 | Jesús Ñáñez | p024@centro-de-prueba.edu.ve | +58 400-000-0024 | Tarde |
| 25 | 10000025 | Lucía Ñáñez | p025@centro-de-prueba.edu.ve | +58 400-000-0025 | Tarde |
| 26 | 10000026 | Carlos Ñáñez | p026@centro-de-prueba.edu.ve | +58 400-000-0026 | Tarde |
| 27 | 10000027 | Andrés Ñáñez | p027@centro-de-prueba.edu.ve | +58 400-000-0027 | Tarde |
| 28 | 10000028 | Dolores Ñáñez | p028@centro-de-prueba.edu.ve | +58 400-000-0028 | Tarde |
| 29 | 10000029 | Víctor Ñáñez | p029@centro-de-prueba.edu.ve | +58 400-000-0029 | Tarde |
| 30 | 10000030 | Inés Ñáñez | p030@centro-de-prueba.edu.ve | +58 400-000-0030 | Tarde |
| 31 | 10000031 | Ramón Ñáñez | p031@centro-de-prueba.edu.ve | +58 400-000-0031 | Mañana |
| 32 | 10000032 | Ángela Ñáñez | p032@centro-de-prueba.edu.ve | +58 400-000-0032 | Tarde |
| 33 | 10000033 | Félix Ñáñez | p033@centro-de-prueba.edu.ve | +58 400-000-0033 | Tarde |

**Comprobaciones B5**

- Inicio → **Docentes = 33**.
- **16** son del turno Mañana y **17** del turno Tarde.
- Ningún error de cédula duplicada, email inválido ni teléfono rechazado.
- Cada docente aparece con el turno correcto en el panel derecho.

---

## B6 — Materias por docente (272 asignaciones)

En la pestaña del docente, `Gestionar titulares` → marca **todas** las materias de su
fila y guarda. `Años` y `Plan` no se fijan aquí.

| Cédula | Docente | Materias titulares |
|---|---|---|
| 10000001 | Ana García | Matemáticas 1º · Química 1º · Matemáticas 2º · Química 2º · Matemáticas 3º · Química 3º · Matemáticas 4º · Química 4º · Matemáticas 5º · Química 5º |
| 10000002 | José García | Castellano 1º · Orientación y Convivencia 1º · Castellano 2º · Orientación y Convivencia 2º · Castellano 3º · Orientación y Convivencia 3º · Castellano 4º · Orientación y Convivencia 4º · Castellano 5º · Orientación y Convivencia 5º |
| 10000003 | María García | Ciencias Naturales 1º · Física 1º · Ciencias Naturales 2º · Física 2º · Física 3º · Física 4º · Física 5º |
| 10000004 | Jesús García | Ciencias Naturales 1º · Química 1º · Ciencias Naturales 2º · Química 2º · Química 3º · Química 4º · Química 5º |
| 10000005 | Lucía García | Geografía, Historia y Ciudadanía 1º · Orientación y Convivencia 1º · Geografía, Historia y Ciudadanía 2º · Orientación y Convivencia 2º · Geografía, Historia y Ciudadanía 3º · Orientación y Convivencia 3º · Geografía, Historia y Ciudadanía 4º · Formación para la Soberanía Nacional 4º · Orientación y Convivencia 4º · Geografía, Historia y Ciudadanía 5º · Formación para la Soberanía Nacional 5º · Orientación y Convivencia 5º |
| 10000006 | Carlos García | Geografía, Historia y Ciudadanía 1º · Geografía, Historia y Ciudadanía 2º · Geografía, Historia y Ciudadanía 3º · Geografía, Historia y Ciudadanía 4º · Geografía, Historia y Ciudadanía 5º |
| 10000007 | Andrés García | Castellano 1º · Castellano 2º · Castellano 3º · Castellano 4º · Castellano 5º |
| 10000008 | Dolores García | Educación Física 1º · Orientación y Convivencia 1º · Educación Física 2º · Orientación y Convivencia 2º · Educación Física 3º · Orientación y Convivencia 3º · Educación Física 4º · Orientación y Convivencia 4º · Educación Física 5º · Orientación y Convivencia 5º |
| 10000009 | Víctor García | Matemáticas 1º · Orientación y Convivencia 1º · Matemáticas 2º · Orientación y Convivencia 2º · Matemáticas 3º · Orientación y Convivencia 3º · Matemáticas 4º · Orientación y Convivencia 4º · Matemáticas 5º · Orientación y Convivencia 5º |
| 10000010 | Inés García | Arte y Patrimonio 1º · Geografía, Historia y Ciudadanía 1º · Arte y Patrimonio 2º · Geografía, Historia y Ciudadanía 2º · Geografía, Historia y Ciudadanía 3º · Geografía, Historia y Ciudadanía 4º · Geografía, Historia y Ciudadanía 5º |
| 10000011 | Ramón García | Castellano 1º · Educación Física 1º · Orientación y Convivencia 1º · Castellano 2º · Educación Física 2º · Orientación y Convivencia 2º · Castellano 3º · Educación Física 3º · Orientación y Convivencia 3º · Castellano 4º · Educación Física 4º · Orientación y Convivencia 4º · Castellano 5º · Educación Física 5º · Orientación y Convivencia 5º |
| 10000012 | Ángela García | Física 1º · Física 2º · Física 3º · Biología 3º · Física 4º · Biología 4º · Física 5º · Biología 5º |
| 10000013 | Félix García | Castellano 1º · Inglés y otras lenguas extranjeras 1º · Castellano 2º · Inglés y otras lenguas extranjeras 2º · Castellano 3º · Inglés y otras lenguas extranjeras 3º · Biología 3º · Castellano 4º · Inglés y otras lenguas extranjeras 4º · Biología 4º · Castellano 5º · Inglés y otras lenguas extranjeras 5º · Biología 5º |
| 10000014 | Pilar García | Inglés y otras lenguas extranjeras 1º · Orientación y Convivencia 1º · Inglés y otras lenguas extranjeras 2º · Orientación y Convivencia 2º · Inglés y otras lenguas extranjeras 3º · Orientación y Convivencia 3º · Inglés y otras lenguas extranjeras 4º · Orientación y Convivencia 4º · Inglés y otras lenguas extranjeras 5º · Orientación y Convivencia 5º |
| 10000015 | Raúl García | Inglés y otras lenguas extranjeras 1º · Orientación y Convivencia 1º · Inglés y otras lenguas extranjeras 2º · Orientación y Convivencia 2º · Inglés y otras lenguas extranjeras 3º · Biología 3º · Orientación y Convivencia 3º · Inglés y otras lenguas extranjeras 4º · Biología 4º · Orientación y Convivencia 4º · Inglés y otras lenguas extranjeras 5º · Biología 5º · Orientación y Convivencia 5º |
| 10000016 | Catalina García | Orientación y Convivencia 1º · Orientación Vocacional 1º · Orientación y Convivencia 2º · Orientación Vocacional 2º · Orientación y Convivencia 3º · Orientación Vocacional 3º · Orientación y Convivencia 4º · Orientación Vocacional 4º · Orientación y Convivencia 5º · Orientación Vocacional 5º |
| 10000017 | Tomás García | Física 1º · Química 1º · Física 2º · Química 2º · Física 3º · Química 3º · Física 4º · Química 4º · Física 5º · Química 5º |
| 10000018 | Mercedes García | Educación Física 1º · Educación Física 2º · Educación Física 3º · Educación Física 4º · Educación Física 5º |
| 10000019 | Gonzalo García | Geografía, Historia y Ciudadanía 1º · Geografía, Historia y Soberanía Nacional 1º · Geografía, Historia y Ciudadanía 2º · Geografía, Historia y Soberanía Nacional 2º · Geografía, Historia y Ciudadanía 3º · Geografía, Historia y Soberanía Nacional 3º · Geografía, Historia y Ciudadanía 4º · Geografía, Historia y Soberanía Nacional 4º · Geografía, Historia y Ciudadanía 5º · Geografía, Historia y Soberanía Nacional 5º |
| 10000020 | Úrsula García | Ciencias Naturales 1º · Geografía, Historia y Ciudadanía 1º · Orientación Vocacional 1º · Ciencias Naturales 2º · Geografía, Historia y Ciudadanía 2º · Orientación Vocacional 2º · Geografía, Historia y Ciudadanía 3º · Orientación Vocacional 3º · Geografía, Historia y Ciudadanía 4º · Orientación Vocacional 4º · Geografía, Historia y Ciudadanía 5º · Orientación Vocacional 5º |
| 10000021 | Ana Ñáñez | Matemáticas 1º · Matemáticas 2º · Matemáticas 3º · Matemáticas 4º · Matemáticas 5º |
| 10000022 | José Ñáñez | Arte y Patrimonio 1º · Geografía, Historia y Ciudadanía 1º · Geografía, Historia y Soberanía Nacional 1º · Arte y Patrimonio 2º · Geografía, Historia y Ciudadanía 2º · Geografía, Historia y Soberanía Nacional 2º · Geografía, Historia y Ciudadanía 3º · Geografía, Historia y Soberanía Nacional 3º · Geografía, Historia y Ciudadanía 4º · Geografía, Historia y Soberanía Nacional 4º · Geografía, Historia y Ciudadanía 5º · Geografía, Historia y Soberanía Nacional 5º |
| 10000023 | María Ñáñez | Lengua y Literatura 1º · Lengua y Literatura 2º · Lengua y Literatura 3º · Lengua y Literatura 4º · Lengua y Literatura 5º |
| 10000024 | Jesús Ñáñez | Educación Física 1º · Innovación Tecnológica y Productividad 1º · Educación Física 2º · Innovación Tecnológica y Productividad 2º · Educación Física 3º · Innovación Tecnológica y Productividad 3º · Educación Física 4º · Innovación Tecnológica y Productividad 4º · Educación Física 5º · Innovación Tecnológica y Productividad 5º |
| 10000025 | Lucía Ñáñez | Innovación Tecnológica y Productividad 1º · Innovación Tecnológica y Productividad 2º · Innovación Tecnológica y Productividad 3º · Innovación Tecnológica y Productividad 4º · Innovación Tecnológica y Productividad 5º |
| 10000026 | Carlos Ñáñez | Orientación Vocacional 1º · Orientación Vocacional 2º · Orientación Vocacional 3º · Orientación Vocacional 4º · Orientación Vocacional 5º |
| 10000027 | Andrés Ñáñez | Castellano 1º · Lengua y Literatura 1º · Castellano 2º · Lengua y Literatura 2º · Castellano 3º · Lengua y Literatura 3º · Castellano 4º · Lengua y Literatura 4º · Castellano 5º · Lengua y Literatura 5º |
| 10000028 | Dolores Ñáñez | Inglés y otras lenguas extranjeras 1º · Inglés y otras lenguas extranjeras 2º · Inglés y otras lenguas extranjeras 3º · Inglés y otras lenguas extranjeras 4º · Inglés y otras lenguas extranjeras 5º |
| 10000029 | Víctor Ñáñez | Biología, Ambiente y Tecnología 1º · Biología, Ambiente y Tecnología 2º · Biología, Ambiente y Tecnología 3º · Biología, Ambiente y Tecnología 4º · Biología, Ambiente y Tecnología 5º |
| 10000030 | Inés Ñáñez | Matemáticas 1º · Física 1º · Matemáticas 2º · Física 2º · Matemáticas 3º · Física 3º · Matemáticas 4º · Física 4º · Matemáticas 5º · Física 5º |
| 10000031 | Ramón Ñáñez | Participación en los Grupos de Creación, Recreación y Producción 1º · Participación en los Grupos de Creación, Recreación y Producción 2º · Participación en los Grupos de Creación, Recreación y Producción 3º · Participación en los Grupos de Creación, Recreación y Producción 4º · Participación en los Grupos de Creación, Recreación y Producción 5º |
| 10000032 | Ángela Ñáñez | Ciencias de la Tierra 5º |
| 10000033 | Félix Ñáñez | Idiomas 1º · Idiomas 2º · Idiomas 3º · Idiomas 4º · Idiomas 5º |

**Comprobaciones B6**

- La suma de materias asignadas por docente es **272**.
- Cada materia-año de cada plan tiene **al menos un docente habilitado en el turno** de sus cursos; si falta alguna, la generación lo dirá identificando área, año y turno.
- Algunos docentes tienen **6 materias sin carga** (Participación 1.º–5.º y Ciencias de la Tierra 5.º): es correcto, no tienen clases que impartir.
- **No hay suplentes** en este conjunto: se añaden a mano en el caso `QA-NEG-03`.

---

## B7 — Disponibilidad (165 franjas-día = 33 docentes × 5 días)

Cada docente está disponible **todo su turno, de lunes a viernes**. En la pestaña
**Disponibilidad**, con `Marcar todo disponible` y ajustando al turno:

| Turno | Días | Ventana | Franjas por día | Total por docente |
|---|---|---|---|---|
| Mañana | lunes a viernes | 07:00–12:40 | 8 | 40 |
| Tarde | lunes a viernes | 12:40–18:10 | 8 | 40 |

Las franjas del **turno contrario** quedan como **No disponible** (un docente de
mañana no da clase por la tarde).

**Comprobaciones B7**

- 33 docentes × 5 días = **165** registros de disponibilidad.
- Ningún docente queda con 0 franjas disponibles (eso es el caso `QA-NEG-02`, que se provoca a propósito).
- Reinicia la aplicación: la rejilla conserva lo marcado.

---

## B8 — Cursos / años y secciones (24)

Se crean en `Gestionar años y secciones…`. El **turno** se asigna en la tabla de
Generación (o ya viene en el curso); el **plan** y el **nº de estudiantes** son
parámetros del generador, no del curso.

| Año | Sección | Turno | Plan | Nº estudiantes |
|---|---|---|---|---|
| 1.º | A | Mañana | 31059 | 39 |
| 1.º | B | Mañana | 31059 | 39 |
| 1.º | C | Tarde | 31060 | 37 |
| 1.º | D | Tarde | 31060 | 36 |
| 1.º | E | Tarde | 31060 | 36 |
| 2.º | A | Tarde | 31059 | 37 |
| 2.º | B | Tarde | 31059 | 35 |
| 2.º | C | Tarde | 31059 | 34 |
| 2.º | D | Tarde | 31060 | 32 |
| 2.º | E | Tarde | 31060 | 33 |
| 3.º | A | Mañana | 31059 | 39 |
| 3.º | B | Mañana | 31059 | 35 |
| 3.º | C | Mañana | 31059 | 34 |
| 3.º | D | Tarde | 31060 | 37 |
| 3.º | E | Tarde | 31060 | 35 |
| 4.º | A | Mañana | 31059 | 44 |
| 4.º | B | Mañana | 31059 | 44 |
| 4.º | C | Mañana | 31059 | 41 |
| 4.º | D | Tarde | 31060 | 32 |
| 4.º | E | Tarde | 31060 | 32 |
| 5.º | A | Mañana | 31059 | 35 |
| 5.º | B | Mañana | 31059 | 42 |
| 5.º | C | Mañana | 31059 | 42 |
| 5.º | D | Mañana | 31059 | 39 |

**Comprobaciones B8**

- **24** cursos: 12 en Mañana y 12 en Tarde.
- Planes: **15** cursos con 31059 y **9** con 31060.
- Matrícula total: **889** estudiantes (mínimo 32, máximo 44).
- Ningún año+sección repetido (el par `4.º D` no existe en 31059: es `4.º E` en 31060).

---

## B9 — Parámetros del generador

En **Configuración → Avanzado**, antes de generar:

| Parámetro | Valor |
|---|---|
| Duración de la hora académica | **40 min** |
| Días a planificar | **5** |
| Año máximo permitido | **Hasta 5to año (liceos regulares)** |
| Usar el aula fija asignada a cada curso | **Activado** |

Después, en **Generación de horario**, comprueba que:

- El resumen dice **«5 días · 8 franjas/día · 40 min por franja · aula fija por curso»** (o el texto equivalente).
- Los 24 cursos aparecen con su turno; asigna el **plan** y el **nº de estudiantes** de la tabla B8.
- Pulsa `Recalcular horas sugeridas por docente` y verifica: **ningún docente por encima de 36 h de aula** y la **suma de horas de aula de un turno = clases de ese turno** (Mañana **414**, Tarde **462**, total **876**).

---

## Verificación final del conjunto

| Concepto | Esperado | ¿OK? |
|---|---|---|
| Turnos | 2 (Mañana, Tarde) | |
| Eventos | 3 (2 Mañana, 1 Tarde) | |
| Tipos de aula | 2 | |
| Aulas | 13 (12 estudio + 1 cancha) | |
| Materias | 85 (80 estudio + 5 cancha) | |
| Planes | 2 (31059, 31060) | |
| Materias-año de los planes | 95 (45 + 50) | |
| Docentes | 33 (16 mañana + 17 tarde) | |
| Asignaciones docente–materia | 272 | |
| Disponibilidades | 165 (33 × 5) | |
| Cursos | 24 (12 + 12) | |
| Matrícula | 889 | |
| Clases al generar | 876 (414 + 462), 96 de EF | |
| Resultado | «Generable: sí» y generación con éxito | |

Si algún total no cuadra, **no sigas**: identifica el bloque y repite su carga
desde el punto de verificación anterior.
