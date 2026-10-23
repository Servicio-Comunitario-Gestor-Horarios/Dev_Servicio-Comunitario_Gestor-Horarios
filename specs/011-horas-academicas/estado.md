# Estado 011

Fase: plan
Aprobaciones: spec 2026-10-23
Base: 5d6b1da77c9da645441a0bebb38e37753f0f7198 (HEAD). **Condición:** el esquema WIP de la 009 está sin commitear; la base efectiva será el commit que incluya la 009 y la 010. El diff del árbol no se imputa a la implementación.
Dudas abiertas: ninguna
Siguiente paso: el planner escribirá `plan.md` y `tasks.md` de la 009 cuando el usuario confirme la base de commit.
Veredicto: pendiente
QA: CAMBIOS NECESARIOS resueltos el 2026-10-23 (H1–H8); spec aprobada por el usuario el 2026-10-23.

## Decisión nueva del usuario (2026-10-23)

- **La configuración global manda** sobre la duración de la hora académica; el **turno es un dato
  externo del plantel y se valida**.
- **Default 35 minutos** (hora académica de Educación Básica, Gaceta 42.505; antes 45), rango **30–90**
  para el cambio normativo y variantes regionales.
- **La franja de clase se deriva de D**, no de repartir la ventana del turno. Esto supersede la
  estimación de la 009 y el inventario de turnos de la 008.
- **Invariante de encaje:** `nº franjas × D + tiempo de eventos ≤ ventana`; si no, rechazo con turno, D
  y déficit. El **número de franjas utilizable** se calcula con ventana, eventos y D.
- **Bloques de 80 minutos de Media:** duda diferida, fuera de alcance; hoy 1 hora = 1 bloque = 1 franja.
- **Duración del preset (cerrada).** La duración deja de ser un dato del usuario en el preset: la
  construcción la **sobrescribe siempre** con la de la configuración global. El preset puede seguir
  conteniendo el campo porque comparte formato con el contrato, pero su valor **nunca** se usa; un
  cambio de configuración desde que se guardó el preset usa el valor nuevo, sin error ni aviso.

## Decisiones de modelado para revisar

- **Franjas declaradas frente a utilizables.** El turno conserva su **número de franjas declarado**
  (dato del plantel) y se valida contra el **máximo utilizable**
  `floor((ventana − eventos) / D)`. El máximo es diagnóstico; la generación usa el declarado, que no
  puede superarlo. La alternativa (derivar siempre el número y dejar de declararlo) cambiaría el modelo
  de turnos y el contrato; no se elige.
- **Configuración y seed.** La configuración **no es dato de dominio** para la política de base vacía:
  la siembra la asegura en 35 de forma **idempotente** (una fila), no la cuenta como dato de dominio,
  no bloquea una base recién creada y no duplica filas.

## Hallazgos del QA resueltos

- **H1:** default 35, franja = D y supersesión de la estimación de la 009 y del inventario de 40 de la
  008; ya no conviven tres duraciones normativas.
- **H2:** resuelto por la decisión del usuario: la duración deja de ser un dato del preset; la
  construcción la **sobrescribe siempre** con la de la configuración y no hay aviso.
- **H3:** la validación del contrato se estrecha a 30–90; supersede V2 de la 005/010 y usa el mismo
  punto único que la configuración.
- **H4:** la configuración no es dato de dominio del seed; se asegura de forma idempotente, sin
  duplicar ni bloquear la siembra.
- **H5:** estado de entrega y verificación declarado; las pruebas del constructor con 45 y del seed con
  40 se reescriben a 35 y a la invariante, y la construcción sin interfaz debe pasar.
- **H6:** «un solo punto» se acota a horas académicas → minutos; la conversión de rangos a franjas del
  constructor sigue; la terminología es del backend y el `horasAula` del front es deuda.
- **H7:** seed verificable: valor efectivo 35 y recuento de configuraciones 1; sin «recuento o resumen»
  ni afirmaciones sobre generaciones no persistidas.
- **H8:** la 011 **extiende** la definición de huella de la 004 para incluir D.

## Supersesiones que declara

- **009:** definición de «duración estimada de la franja de clase» y cálculo
  `(ventana − eventos) / nº franjas` (RF-3 y RF-11); la franja pasa a durar D y el sobrante no se
  reparte.
- **008:** inventario de 8 franjas de 40 minutos y rejilla `8 × 40 + 10 = 330`; los turnos se revalidan
  con D = 35.
- **005/010:** V2 de `duracion_minutos` solo exige `> 0`; la 011 la estrecha a 30–90.

## Notas

- **Orden de implementación.** Después de la 010: la 010 deja el contrato en 2.0; la 011 solo cambia la
  procedencia y el rango del campo de duración.
- **Tope de 40.** No se toca; si el usuario quisiera cambiarlo, sería otro requisito.
- **No hay plan ni tareas todavía.** Esta fase termina con la aprobación de la spec.

## Tamaño previsto

Nueve RF agrupables en **≤10 tareas**: configuración y servicios, punto único, franja y encaje del
turno, contrato, huella, esquema/versión, seed y guía de API.
