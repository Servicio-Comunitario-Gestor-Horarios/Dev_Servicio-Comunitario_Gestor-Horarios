# Revisión manual de QA — Gestor de Horarios

Guía paso a paso para revisar una entrega del **Gestor de Horarios** (aplicación de
escritorio Qt + OR-Tools) sin conocimientos de programación: qué se carga, en qué
orden, qué se verifica de esos datos, qué se prueba después y cómo se reporta lo que
falla.

- **Versión revisada:** anótala con `gestor-horarios.exe --version`
- **Entrega:** ZIP portable (`GestorHorarios-<versión>-win64-portable`), que debe
  incluir **`gestor-horarios.exe`** y **`gestor-horarios-consola.exe`**
- **Datos:** **no** vienen precargados. La base se construye a mano durante la
  revisión con el conjunto de
  [`dataset-centro-real.md`](dataset-centro-real.md) (réplica del centro real)
- **Documentos de apoyo:** `packaging/LEEME.txt`, `docs/guias/consola-datos.md`

---

## 0. Cómo usar esta guía

1. Prepara el entorno (§1).
2. **Prueba de humo** de instalación (§2). Si falla, para aquí.
3. **Paso 0: turnos y recesos** con la consola (§3). Sin esto la aplicación no puede
   planificar nada.
4. **Carga manual del conjunto** bloque a bloque, verificando cada bloque (§4).
5. **Verificación integral del conjunto** cargado (§5): si no cuadra, no sigas.
6. **Pruebas funcionales por área** contra esos datos (§6).
7. **Casos de borde y negativos**, provocados sobre los datos ya cargados (§7).
8. **Matriz de combinaciones** (§8) y **persistencia** (§9).
9. Reporta cada fallo con la plantilla de §11 y cierra la ronda con §12.

Cada caso tiene identificador estable (`QA-XXX-nn`), prioridad, pasos, esperado y "qué
mirar". Si algo no coincide con lo esperado **o no está claro**, regístralo: la
ambigüedad también es un hallazgo.

| Prioridad | Significado |
|---|---|
| **P0** | Ruta crítica: sin esto la aplicación no sirve |
| **P1** | Funcionalidad principal de un área |
| **P2** | Secundaria: mensajes, estética, comodidad |

> **Regla de oro:** un fallo se reporta **cuando se encuentra**, no al final. La carga
> del conjunto es larga; si esperas al final, no sabrás qué bloque lo provocó.

---

## 1. Entorno y preparación

### 1.1 Requisitos

- Windows 10/11 de 64 bits. **No** hace falta instalar nada ni permisos de
  administrador: Qt, OR-Tools y el runtime de Visual C++ viajan en el paquete.
- Descomprime el **ZIP completo** en una carpeta local (por ejemplo `C:\QA\gestor\`) y
  trabaja desde ahí. **No** ejecutes el `.exe` desde dentro del ZIP ni desde una unidad
  de red: las DLL deben quedar junto al ejecutable.

### 1.2 Qué hay y dónde vive

| Elemento | Ruta |
|---|---|
| Aplicación | `<carpeta del paquete>\gestor-horarios.exe` |
| Consola (crear turnos y recesos) | `<carpeta del paquete>\gestor-horarios-consola.exe` |
| Base de datos del usuario | `%APPDATA%\LiceoRobertSerra\GestorHorarios\gestor_horarios.db` |
| Horarios guardados (JSON) | `%APPDATA%\LiceoRobertSerra\GestorHorarios\schedules\` |
| Preferencias (tema, días, duración, aula fija, año máximo) | Registro, `HKCU\Software\LiceoRobertSerra\GestorHorarios` |

### 1.3 Empezar desde cero (una y otra vez)

1. Cierra la aplicación y la consola.
2. Renombra `%APPDATA%\LiceoRobertSerra` a `LiceoRobertSerra.bak`.
3. Abre la aplicación: crea una base nueva **vacía** (0 docentes, 0 aulas, 0 materias)
   y ese es el punto de partida de la revisión.

> La base nueva **no trae turnos ni recesos**: por eso el §3 no es opcional. Es una
> limitación conocida de esta versión, no un fallo que haya que reportar de nuevo: no
> existe pantalla para gestionarlos y se crean con la consola.

### 1.4 Organización recomendada del trabajo

- **Sesión 1:** §2, §3 y bloques B1–B4 (tipos, aulas, materias, planes).
- **Sesión 2:** B5–B7 (docentes, materias por docente, disponibilidad).
- **Sesión 3:** B8–B9, §5 y generación.
- **Sesión 4:** §6, §7, §8 y §9 (funcional, negativos, matriz, persistencia).

Después de **cada bloque**: cierra y vuelve a abrir la aplicación, comprueba el bloque
y anota el resultado. Guarda una copia del `.db` al terminar cada sesión
(`gestor_horarios_sesion1.db`, etc.): si algo se estropea, no repites la carga entera.

### 1.5 Nombres de evidencia

`QA-GEN-03_paso2.png`, `B5_docentes_ok.png`, `ronda1_centro.db`. Que el nombre diga
qué caso y qué bloque.

---

## 2. Prueba de humo (P0, 10 minutos)

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-INS-01** | Descomprimir y arrancar | Doble clic en `gestor-horarios.exe` | Abre **maximizada**, sin instalación ni permisos. No aparece consola detrás |
| **QA-INS-02** | Versión | `gestor-horarios.exe --version` | Imprime `Gestor-Horarios v<versión>`; coincide con la del ZIP |
| **QA-INS-03** | Ayuda | `gestor-horarios.exe --help` | `Uso: gestor-horarios [--backend] [--version] [--help]` |
| **QA-INS-04** | Backend headless | `gestor-horarios.exe --backend` | Arranca sin interfaz, sin errores de DLL. Se cierra con `Ctrl+C` |
| **QA-INS-05** | La consola viene en la entrega | `gestor-horarios-consola.exe --version` | Responde con su versión. **Si no existe el archivo, repórtalo como S1**: sin consola no se pueden crear turnos |
| **QA-INS-06** | Ayuda de la consola | `gestor-horarios-consola.exe --ayuda` | Lista los comandos (`turnos crear`, `eventos`, `docentes`, `aulas`, `materias`, `planes`, `cursos`…) |
| **QA-INS-07** | Primer arranque limpio | Con `%APPDATA%\LiceoRobertSerra` inexistente, abre la app | Arranca sin errores y crea la base; Inicio muestra 0 docentes / 0 aulas / 0 materias |
| **QA-INS-08** | Instancia única | Con la app abierta, doble clic otra vez | No se abre otra ventana: enfoca la existente o avisa. En el Administrador de tareas, un solo proceso de UI |
| **QA-INS-09** | Sin las DLL | Mueve solo el `.exe` a otra carpeta y ejecútalo | Falla por DLL ausente (esperado). Devuélvelo a su sitio |
| **QA-INS-10** | Pantalla pequeña / escalado | Prueba al 100 %, 125 % y 150 % de escalado | Nada queda cortado ni fuera de pantalla; la ventana se adapta |

---

## 3. Paso 0 — Turnos y recesos con la consola (bloque B0)

**Con la aplicación cerrada.** Detalle y comandos exactos: bloque **B0** de
[`dataset-centro-real.md`](dataset-centro-real.md).

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-B0-01** | Crear los dos turnos | Ejecuta los `turnos crear` del anexo | `turnos listar` devuelve **Mañana 07:00–12:40 (8 franjas)** y **Tarde 12:40–18:10 (8 franjas)** |
| **QA-B0-02** | Crear los recesos y el acto cívico | Ejecuta los `turnos evento-agregar` | Mañana: Receso (08:20–08:30) y Acto cívico (08:30–08:40) tras la franja 2; Tarde: Receso (14:00–14:10) |
| **QA-B0-03** | Turno duplicado | Vuelve a ejecutar `turnos crear Mañana …` | Falla con un mensaje claro (clave duplicada); no duplica filas |
| **QA-B0-04** | La app ve los turnos | Abre la aplicación → **Generación** | El resumen de parámetros muestra **16 franjas/día** (8 de cada turno) y **40 min** por franja. Ya no aparece 0 |
| **QA-B0-05** | Turno sin receso | Crea un turno de prueba y bórralo con `turnos eliminar` | El turno desaparece; la aplicación sigue arrancando sin errores |

---

## 4. Carga manual del conjunto (bloques B1–B9)

Carga **en el orden del anexo**. Al final de cada bloque hay comprobaciones: si una
falla, no continúes.

| Bloque | Qué cargas | Dónde (pantalla) | Comprobación clave |
|---|---|---|---|
| **B1** | 2 tipos de aula (`Aula de estudio`, `Cancha`) | Aulas → `Nuevo tipo de aula` | Ambos en los desplegables |
| **B2** | 13 aulas (12 estudio + 1 cancha, capacidad 2) | Aulas → `+ Registrar Aula` | Inicio: **Aulas = 13** |
| **B3** | 85 materias (80 estudio + 5 cancha) | Asignaturas → `+ Registrar Asignatura` | Inicio: **Materias = 85** |
| **B4** | 2 planes (31059, 31060) con 95 materias-año | Asignaturas → `+ Registrar Plan de Estudio` | Sumas por año: 34/34/34/36/34 y 40×5 |
| **B5** | 33 docentes | Docentes → `+ Registrar Docente` | Inicio: **Docentes = 33** |
| **B6** | 272 materias titulares | Docentes → pestaña del docente → `Gestionar titulares` | Suma de asignaciones = 272 |
| **B7** | 165 disponibilidades (33 × 5 días) | Docentes → `Disponibilidad` | Ningún docente con 0 franjas |
| **B8** | 24 cursos | Generación → `Gestionar años y secciones…` | 12 mañana + 12 tarde |
| **B9** | 40 min · 5 días · 5.º año · aula fija ON | Configuración → Avanzado | Resumen de parámetros correcto |

Casos específicos de la carga (los demás son la propia tabla del anexo):

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-CAR-01** | Alta de aula | Nombre `Aula 01`, capacidad 44, tipo `Aula de estudio`, edificio `A`, piso `Planta baja` | Se guarda y se ve en la tabla con todos los datos |
| **QA-CAR-02** | Aula sin nombre | Guarda sin nombre | «El campo Nombre es obligatorio.» |
| **QA-CAR-03** | Aula sin tipo | Guarda sin seleccionar tipo | «Debe seleccionar un tipo de aula del catálogo.» |
| **QA-CAR-04** | Tipo duplicado | Crea `Cancha` dos veces (y `CANCHA`) | Rechaza o reutiliza el existente; nunca dos tipos equivalentes |
| **QA-CAR-05** | Tipo general protegido | Intenta borrar o renombrar `Aula de estudio` | Falla con un mensaje que explica que es el tipo general del centro |
| **QA-CAR-06** | Materia de cancha | Crea `Educación Física 1º` con tipo `Cancha` | Guarda; la materia queda marcada con su tipo |
| **QA-CAR-07** | Plan con código no numérico | Código `ABC1` | «El código del plan debe ser numérico y único.» |
| **QA-CAR-08** | Plan duplicado | Crea dos planes con código `31059` | Rechaza el segundo |
| **QA-CAR-09** | Plan por años | Desmarca «Plan global» y reparte materias por año | El reparto se conserva al reabrir el plan |
| **QA-CAR-10** | Materia con 0 horas | Intenta dejar una materia del plan a 0 h | Se rechaza o se guarda como 0 según el diseño: **anótalo**, la generación no debe perder clases sin avisar |
| **QA-CAR-11** | Cédula no numérica | Docente con cédula `P001` | «La cédula debe contener únicamente números (sin puntos, espacios ni símbolos).» |
| **QA-CAR-12** | Cédula duplicada | Repite una cédula ya usada | Mensaje con el docente que ya la tiene |
| **QA-CAR-13** | Email inválido | `ana.garcia`, `ana@centro` | Rechaza y marca el campo |
| **QA-CAR-14** | Errores agrupados | Deja nombre y email mal y guarda | Se listan **todos** los errores de una vez |
| **QA-CAR-15** | Curso duplicado | Crea `4.º` sección `D` en 31059 (no existe) y luego otra vez | El primero se crea; el segundo se rechaza |
| **QA-CAR-16** | Sección inválida | Sección `DD` o `4` | «Selecciona una sección del abecedario.» |
| **QA-CAR-17** | Límite de años | Con «Hasta 5to año», intenta crear 6.º | No se permite; cambia a 6.º en Configuración y ahora sí |
| **QA-CAR-18** | Persistencia por bloque | Al terminar cada bloque, cierra y reabre | Los contadores de Inicio y las tablas conservan lo cargado |
| **QA-CAR-19** | Carga a medias | Empieza B3 (materias) y cierra a la mitad | No se pierde lo ya guardado; al reabrir se puede continuar |
| **QA-CAR-20** | Rendimiento de las listas | Con 85 materias y 33 docentes, desplázate y ordena | La tabla responde sin tirones; los desplegables se abren rápido |

---

## 5. Verificación integral del conjunto cargado

Con B1–B9 completos y la aplicación reabierta:

| ID | Verificación | Esperado |
|---|---|---|
| **QA-VER-01** | Contadores de Inicio | Docentes **33** · Aulas **13** · Materias **85** |
| **QA-VER-02** | Planes | 2 planes; 31059 con 45 materias-año y 31060 con 50 |
| **QA-VER-03** | Sumas por año | 31059: 34/34/34/36/34 · 31060: 40/40/40/40/40 (todas pares) |
| **QA-VER-04** | Cursos | 24 (12 mañana / 12 tarde); planes 15×31059 y 9×31060; matrícula 889 |
| **QA-VER-05** | Materias por docente | Suma total 272; ninguna materia-año de un plan sin docente habilitado en su turno |
| **QA-VER-06** | Disponibilidad | 33 × 5 días; cada docente con las 8 franjas de su turno |
| **QA-VER-07** | Horas sugeridas | `Recalcular horas sugeridas por docente`: ningún docente > **36 h de aula**; suma por turno **414** (mañana) y **462** (tarde) |
| **QA-VER-08** | Resumen del generador | «5 días · 40 min · aula fija por curso» y los 24 cursos con turno, plan y estudiantes |

**Si algo no cuadra:** identifica el bloque, borra **solo ese bloque** y vuelve a
cargarlo (o restaura la copia de la sesión anterior). Reporta el desvío con el total
que esperabas y el que obtuviste. **No generes** con el conjunto incompleto: los
errores se confunden con fallos del motor.

---

## 6. Pruebas funcionales por área

### 6.1 Inicio

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-INI-01** | Totales | Entra en Inicio con el conjunto cargado | 33 / 13 / 85; el logo del liceo se ve |
| **QA-INI-02** | Alta y vuelta | Registra un aula más, vuelve a Inicio | El total sube **sin reiniciar**; bórrala y vuelve a bajar |
| **QA-INI-03** | Tema | Cambia a modo oscuro y vuelve a Inicio | Textos legibles; el logo no desaparece |

### 6.2 Docentes

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-DOC-01** | Editar | Cambia el nombre de un docente y guarda | Tabla y detalles al día; se conserva al reabrir |
| **QA-DOC-02** | Eliminar con datos | Elimina un docente con materias y disponibilidad | Pide confirmación; desaparece de la tabla y de los contadores; sus materias dejan de estar asignadas |
| **QA-DOC-03** | Rejilla de disponibilidad | Marca y desmarca franjas con clic y con arrastre | Cambian de color según la leyenda (Disponible / No disponible), sin huecos ni parpadeos |
| **QA-DOC-04** | Marcar todo disponible | Pulsa `Marcar todo disponible` y luego desmarca una franja | Se marcan las del turno; la desmarcada queda diferenciada |
| **QA-DOC-05** | Horas preferidas | Marca franjas como **Preferida** y otras como **Evitada** | Se distinguen de «sin preferencia» y se conservan al reiniciar |
| **QA-DOC-06** | Titulares | `Gestionar titulares` de un docente: quita una materia | La materia desaparece de su lista y deja de contar en las 272 |
| **QA-DOC-07** | Suplentes | `Gestionar suplentes`: asigna una materia que ya dicta otro | Se guarda como suplencia; comprueba qué pasa si se intenta poner la misma materia como titular y suplente |
| **QA-DOC-08** | Sin materias | Deja un docente sin titulares | Aviso de que no podrá usarse en la generación; al generar no recibe clases |
| **QA-DOC-09** | Turno/plan/año opcionales | Edita un docente y cambia Turno, Plan y Año preferencial | Se guardan como valores iniciales; no alteran sus materias |
| **QA-DOC-10** | Búsqueda | Si la tabla tiene buscador, filtra por cédula, nombre y email | Filtra en caliente; sin coincidencias queda vacía sin error |

### 6.3 Aulas y Asignaturas

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-AUL-01** | Editar aula | Cambia capacidad y edificio | Se refleja en la tabla |
| **QA-AUL-02** | Eliminar aula | Elimina `Aula 13` | Pide confirmación; desaparece del listado |
| **QA-AUL-03** | Aula en uso | Elimina un aula asignada como aula fija de un curso | Aviso o bloqueo explicado; no debe dejar cursos apuntando a un aula inexistente |
| **QA-ASG-01** | Editar materia | Cambia el tipo de `Educación Física 1º` de `Cancha` a `Aula de estudio` | Se guarda; al generar, esas clases dejan de pedir la cancha (**anota** el resultado) |
| **QA-ASG-02** | Eliminar materia | Elimina una materia que está en un plan y asignada a un docente | Aviso del impacto; no quedan referencias rotas |
| **QA-ASG-03** | Eliminar plan en uso | Elimina el plan 31060 | Pide confirmación y avisa; los cursos afectados se señalan en Generación |
| **QA-ASG-04** | Buscador de materias | En el plan, busca `Matem` en «Materias disponibles» | Filtra correctamente con acentos y mayúsculas |

### 6.4 Generación de horario

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-GEN-01** | Carga inicial | Entra en Generación | 24 cursos con turno; docentes con sus horas; «Listo para generar.» |
| **QA-GEN-02** | Recargar desde la base | Añade un docente y pulsa `Recargar desde la base` | Aparece sin reiniciar |
| **QA-GEN-03** | Curso sin turno | Pon un curso en «— Sin turno —» y genera | «El curso «X» no tiene un turno asignado.» |
| **QA-GEN-04** | Curso sin plan | Quita el plan de un curso y genera | «El curso «X» no tiene un plan de estudio asignado.» |
| **QA-GEN-05** | Curso con 0 estudiantes | Pon 0 y genera | «El curso «X» debe tener más de 0 estudiantes.» |
| **QA-GEN-06** | Sin cursos | Base recién creada → Generación | «No hay cursos para planificar.»; no se cae |
| **QA-GEN-07** | Horas sugeridas | `Recalcular horas sugeridas por docente` | Cambian de forma coherente con los planes; mensaje de confirmación |
| **QA-GEN-08** | Años y secciones | Crea `4.º D` y elimina `5.º D` | El árbol y la tabla de generación se actualizan |
| **QA-GEN-09** | Generación completa | Con el conjunto verificado, pulsa **Generar horarios** | «Generando horarios…»; termina con éxito y **salta a Visualización**. Referencia del centro completo: **≤ 300 s** (281 s medidos) |
| **QA-GEN-10** | Guardado del JSON | Mira `%APPDATA%\...\schedules\` | `horario_AAAAMMDD_HHMMSS.json` con el horario; el mensaje indica la ruta |
| **QA-GEN-11** | Generar dos veces | Cambia el nº de estudiantes de un curso y regenera | No reutiliza resultados obsoletos; el segundo resultado refleja el cambio |
| **QA-GEN-12** | Doble clic | Pulsa Generar dos veces rápido | Una sola generación; el botón se deshabilita o ignora el segundo clic |
| **QA-GEN-13** | Cerrar generando | Genera y cierra antes de terminar | Sin errores ni procesos huérfanos en el Administrador de tareas |
| **QA-GEN-14** | Guardar preset | `Guardar preset…` con el conjunto completo | Guarda el `.json` y confirma |
| **QA-GEN-15** | Cargar el propio preset | `Cargar preset…` con el que acabas de guardar | Restaura turnos, planes, estudiantes y suplentes; el resumen coincide con el de antes de guardar |
| **QA-GEN-16** | Preset corrupto | Edita el `.json` guardado: rompe una llave o cambia un tipo | «No se pudo cargar el preset» con el motivo; la pantalla no queda a medias |
| **QA-GEN-17** | Preset con datos de otro centro | En el `.json`, cambia un nombre de docente o de materia por uno inexistente | Aviso explícito de qué falta; no mezcla datos |

### 6.5 Visualización

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-VIS-01** | Rejilla por curso | Revisa los 24 cursos | Sin huecos donde debería haber clase ni dos materias en la misma franja; recesos y acto cívico en su sitio |
| **QA-VIS-02** | Leyenda | Compara leyenda y colores | Coinciden; todas las materias visibles |
| **QA-VIS-03** | Horarios de profesores | `🎓 Horarios de Profesores`, navegando con `Siguiente →` | Se ven los 33 docentes con su carga; sin solapes |
| **QA-VIS-04** | Horarios de estudiantes | `👥 Horarios de Estudiantes`, cambia de sección | Cada curso con su horario; los selectores responden |
| **QA-VIS-05** | Cargar desde archivo | `📁 Cargar Horarios desde Archivo` con el JSON generado | Se navega igual que el recién generado |
| **QA-VIS-06** | Archivo inválido | Carga un `.json` que no es un horario | Aviso claro; la vista anterior no se rompe |
| **QA-VIS-07** | Mover una clase | Arrastra una clase a una franja libre | Se mueve; los bloques de dos franjas se mueven enteros |
| **QA-VIS-08** | Movimiento imposible | Suelta sobre una franja ocupada o donde el bloque se partiría | «Conflicto de horario» o «El destino partiría una clase», con qué hacer |
| **QA-VIS-09** | Persistencia del movimiento | Mueve, cierra y vuelve a cargar | Se conserva **o** se descarta con aviso; nunca a medias |
| **QA-VIS-10** | Regenerar | Genera otra vez y mira Visualización | Se refresca con el horario nuevo, sin restos del anterior |
| **QA-VIS-11** | Cancha | Busca las clases de Educación Física en los 24 cursos | Nunca hay más de **dos** cursos en la cancha en la misma franja (aula de capacidad 88, 2 grupos simultáneos) |

### 6.6 Configuración

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-CFG-01** | Modo oscuro | Activa y acepta | Toda la app cambia; se conserva al reiniciar |
| **QA-CFG-02** | Descartar | Cambia tema, duración y días, pulsa **Descartar** | Nada se guarda |
| **QA-CFG-03** | Duración | 40 → 35 min y genera | El resumen y el horario respetan la duración nueva |
| **QA-CFG-04** | Días | 1 y luego 7 días | La rejilla muestra exactamente esos días; aparecen sábado y domingo |
| **QA-CFG-05** | Aula fija | Desactiva «Usar el aula fija…» y genera | El motor reparte aulas; el resumen lo indica y no asigna dos cursos al mismo aula |
| **QA-CFG-06** | Extremos | Duración 30 y 90 min | Se aceptan; si no encaja en la ventana del turno, aviso comprensible |

---

## 7. Casos de borde y negativos (sobre los datos ya cargados)

No hace falta una base nueva: se provocan **mutando el conjunto** y se deshacen al
terminar (o se restaura la copia del `.db`).

| ID | Escenario | Cómo provocarlo | Esperado |
|---|---|---|---|
| **QA-NEG-01** | Sin profesorado | Elimina los docentes del turno Tarde | Al generar: rechazo identificando materia, año y turno sin docente habilitado; no genera a medias |
| **QA-NEG-02** | Docente sin disponibilidad | Deja a un docente con 0 franjas disponibles | Rechazo o resultado parcial **explicado**; no se cuelga |
| **QA-NEG-03** | Suplencias | Marca a un docente como suplente de una materia de otro | Se guarda; la generación usa la suplencia si falta el titular (compruébalo quitando al titular) |
| **QA-NEG-04** | Exceso de capacidad | Sube a 40 los cursos de un turno o infla horas en el plan | Aviso de capacidad horaria excedida indicando curso/materia; no genera un horario imposible |
| **QA-NEG-05** | Cancha insuficiente | Sube las horas de Educación Física en los planes para que no quepan | Aviso si la cancha no da abasto (2 grupos por franja); nunca más de dos cursos a la vez en ella |
| **QA-NEG-06** | Tope de horas | Haz que un docente necesite más de 36 h de aula | Aviso o reparto que respeta el tope; nunca asignaciones por encima sin explicación |
| **QA-NEG-07** | Base de otra versión | Sustituye el `.db` por una base antigua | «La base de datos es de la versión X y esta aplicación espera la Y», con qué hacer |
| **QA-NEG-08** | Base corrupta | Con la app cerrada, sustituye el `.db` por un `.txt` renombrado | Mensaje de que no se pudo comprobar la base; no se cierra en silencio |
| **QA-NEG-09** | Sin permisos de escritura | Pon `%APPDATA%\LiceoRobertSerra` en solo lectura | Aviso de que no puede guardar; sin cuelgues |
| **QA-NEG-10** | Curso sin aula fija | Desactiva el aula fija y genera | Reparto automático correcto; ningún aula con dos cursos a la vez |
| **QA-NEG-11** | Muchos datos | Duplica cursos hasta 40 y genera | La app responde; si tarda, muestra progreso |
| **QA-NEG-12** | Cierre con cambios | Edita un formulario y cierra con la X | Avisa o descarta de forma explícita |
| **QA-NEG-13** | Todo vacío | Base nueva sin NADA (ni turnos) → Generación | «No hay cursos para planificar.» y, si falta la configuración horaria, aviso claro de que no hay franjas |

---

## 8. Matriz de combinaciones

Cubre al menos una vez cada valor y las parejas marcadas con ★.

| Dimensión | Valores |
|---|---|
| Turno | Mañana ★, Tarde ★, ambos a la vez ★ |
| Plan | 31059 ★, 31060 ★ |
| Días | 1 ★, 5 ★, 7 ★ |
| Duración | 30 ★, 35 ★, 40 ★ (el del centro), 90 ★ |
| Aula fija | ON ★, OFF ★ |
| Año máximo | 5.º ★, 6.º ★ |
| Tema | Claro ★, Oscuro ★ |
| Estudiantes por curso | 1 ★, 37 (real) ★, 999 ★ |
| Disponibilidad | Completa ★, parcial ★, vacía ★ |
| Preferencias | Sin ★, preferidas ★, evitadas ★ |
| Suplencias | Sin ★, con ★ |
| Tipo de aula | Estudio ★, Cancha ★ |
| Escalado Windows | 100 % ★, 125 %, 150 % ★ |
| Ventana | Maximizada ★, restaurada pequeña ★ |

Combinaciones cruzadas mínimas:

1. Mañana + 31059 + 5 días + 40 min + aula fija ON + claro.
2. Tarde + 31060 + 7 días + 35 min + aula fija OFF + oscuro.
3. Ambos turnos + 90 min + oscuro + 150 % de escalado.
4. Un curso, 1 estudiante, 1 día, con un docente de disponibilidad vacía.
5. Cancha con dos cursos de Educación Física a la misma hora.

---

## 9. Persistencia, reinicio y actualización

| ID | Caso | Pasos | Esperado |
|---|---|---|---|
| **QA-PER-01** | Reinicio | Cierra y abre tras la carga completa | Todo sigue: 33 / 13 / 85, planes, cursos y disponibilidad |
| **QA-PER-02** | Otro usuario de Windows | Abre la app con otra cuenta | Datos separados; el otro usuario ve una base vacía |
| **QA-PER-03** | Copia de seguridad | Renombra la carpeta y vuelve a ponerla | Recupera los datos sin intervención |
| **QA-PER-04** | Cierre forzado | Mata el proceso y reabre | La base no se corrompe; como mucho se pierde lo último no guardado |
| **QA-PER-05** | Actualización de la entrega | Sustituye el ZIP por una versión nueva **sin** tocar `%APPDATA%` | Arranca con los datos previos y migra el esquema avisando |
| **QA-PER-06** | Preset como copia de parámetros | Guarda un preset, restaura una copia del `.db` anterior y carga el preset | Los parámetros del generador vuelven a los del preset; los datos de la base no cambian |

---

## 10. Fuera de alcance

- Instalador, firma del binario y SmartScreen (la entrega es un ZIP portable).
- Rendimiento con más de ~40 cursos.
- Accesibilidad completa y traducciones (la interfaz es solo en español).
- Impresión o exportación a PDF (si no existe el botón, no es un fallo de esta ronda).
- Linux/macOS: la entrega es Windows x64.
- **Gestión de turnos desde la interfaz**: hoy no existe; se usan la consola (§3) y
  esta limitación ya está registrada.

---

## 11. Cómo reportar

### 11.1 Severidad

| Nivel | Criterio | Ejemplo |
|---|---|---|
| **S1 — Bloqueante** | Impide usar la app o hace perder datos | No arranca; se cierra al generar; pierde la base |
| **S2 — Grave** | Una función principal no hace lo que debe, sin avisar | Horario que ignora la disponibilidad; preset que no guarda; totales falsos |
| **S3 — Menor** | Molesto, con rodeo | Mensaje confuso; falta un aviso |
| **S4 — Cosmético** | Estética o texto | Acento, alineación, icono |

### 11.2 Qué adjuntar siempre

1. **Versión**: `gestor-horarios.exe --version`.
2. **Entorno**: Windows, escalado, si la base era nueva o la del conjunto completo.
3. **Pasos numerados** con los valores exactos (cédula, nombre del curso, preset).
4. **Esperado** y **obtenido**, con el **mensaje literal** entre comillas.
5. **Frecuencia**: siempre / a veces (¿cuántas de cuántas?).
6. **Evidencias**: captura o vídeo, `.db` (o la copia de la sesión), el preset `.json`
   y el `horario_*.json` generado, y si aplica el ZIP de
   `%APPDATA%\LiceoRobertSerra\GestorHorarios` **con la app cerrada**.
7. **¿Se reproduce desde cero?** Si solo pasa con el conjunto completo, adjunta la base.

### 11.3 Plantilla

```markdown
## QA-<ÁREA>-<nn> — <resumen en una línea>

**Severidad:** S1/S2/S3/S4   **Prioridad:** P0/P1/P2   **Frecuencia:** siempre / 3 de 5
**Versión:** Gestor-Horarios vX.Y.Z   **Entrega:** ZIP portable
**Entorno:** Windows 11, 125 %, base = conjunto centro real (bloque B1–B9 completo)

### Pasos
1. ...
2. ...

### Esperado
...

### Obtenido
... (mensaje literal: «...»)

### Evidencias
- captura, gestor_horarios.db, preset.json, horario_....json

### ¿Se reproduce desde cero?
Sí / No (adjunto la base)

### Notas
...
```

### 11.4 Ejemplos

- ❌ «No funciona la generación.» → no reproducible.
- ✅ «Con el conjunto completo, turno Tarde + plan 31060 + 7 días + aula fija OFF, al
  pulsar Generar aparece «La base de datos no es válida» y no genera; con 5 días sí
  genera. Adjunto base y preset. Siempre.»
- ❌ «Se ve raro.» → ✅ «En modo oscuro, la leyenda de colores de Horarios de Profesores
  queda texto oscuro sobre fondo oscuro (ilegible).»

---

## 12. Cierre de la ronda

### 12.1 Criterios de salida

- Bloques **B0–B9** cargados y verificados (§5) sin desvíos.
- **QA-INS-01, -02, -05, -07** y **QA-GEN-09, -10, -11** en verde.
- Ningún **S1** abierto.
- Ningún **S2** abierto en Docentes, Generación y Visualización.
- Matriz (§8) con todas las filas cubiertas al menos una vez.
- Persistencia (§9) verificada con un ciclo completo.

### 12.2 Resumen de la ronda

| Campo | Valor |
|---|---|
| Versión / commit revisado | |
| Fecha y revisor | |
| Entorno (SO, escalado, usuario) | |
| Bloques cargados y verificados | |
| Casos ejecutados / total | |
| S1 / S2 / S3 / S4 abiertos | |
| Veredicto | Apto / Apto con reservas / No apto |
| Observaciones generales | |

### 12.3 Registro de casos

| ID | Estado (OK / FALLO / BLOQUEADO / NO PROBADO) | Severidad | Issue | Notas |
|---|---|---|---|---|
| QA-INS-01 | | | | |
| QA-B0-01 | | | | |
| QA-VER-01 | | | | |
| QA-DOC-01 | | | | |
| QA-AUL-01 | | | | |
| QA-ASG-01 | | | | |
| QA-GEN-09 | | | | |
| QA-VIS-01 | | | | |
| QA-CFG-01 | | | | |
| QA-NEG-01 | | | | |
| QA-PER-01 | | | | |

---

## Anexo A — Chuleta de comandos

```bat
:: Versión y ayuda
gestor-horarios.exe --version
gestor-horarios.exe --help
gestor-horarios-consola.exe --version
gestor-horarios-consola.exe --ayuda

:: Backend sin interfaz
gestor-horarios.exe --backend

:: Ruta de la base que usa la aplicación
set DB=%APPDATA%\LiceoRobertSerra\GestorHorarios\gestor_horarios.db

:: Empezar de cero (con la aplicación cerrada)
ren "%APPDATA%\LiceoRobertSerra" LiceoRobertSerra.bak

:: Abrir la carpeta de datos
explorer "%APPDATA%\LiceoRobertSerra\GestorHorarios"
```

## Anexo B — Mensajes esperados (no son fallos)

| Mensaje | Cuándo |
|---|---|
| «Generación requiere Release» | Al generar con un binario Debug: usa la entrega Release |
| «Instancia ya en ejecución» | Segundo arranque: comportamiento correcto |
| «No hay cursos para planificar.» | Base sin cursos |
| «El curso «X» no tiene un turno asignado.» | Curso sin turno: es una validación, no un error |
| «La base de datos es de la versión X…» | Base de otra versión: indica qué hacer |
| 0 franjas/día en el resumen | Falta el paso 0 (turnos y recesos, §3) |
