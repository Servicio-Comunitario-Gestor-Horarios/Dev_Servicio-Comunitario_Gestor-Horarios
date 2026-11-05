# Journal 013 - Preferencias del docente en la generación

Registro de la implementación de las tareas del plan 013. Una entrada por tarea ejecutada; las entradas
son **aditivas** (no se reescriben las anteriores).

- **Base:** `bb1767af343c108b20de2b2b4de7ebe1d7658c2f` **+ la 015 y la 016 aplicadas en el árbol de
  trabajo (sin commits)**. HEAD sigue en `bb1767a`. **Si la 015/016 se commitean antes de empezar, el
  `Base` pasa a ser ese sha de cierre.**
- **Fecha de apertura:** 2026-11-04.
- **Estado:** implementación **no iniciada**; las diez secciones de abajo están vacías a la espera de la
  indicación del usuario.
- **Aprobaciones:** spec **2026-11-04** · plan/tareas **2026-11-04** (10 tareas; el exceso de duración de
  **T1/T5/T6** sobre la guía de 20–30 min fue **aceptado por el usuario**, sin dividir la spec y con
  RF-8 dentro de la 013).
- **Alcance de la entrega:** contrato 2.0 con la sección opcional `preferencias`, proyección reloj → franja
  y filtro de día, huella con las preferencias intervinientes, objetivo en dos fases (S1/S2 fijos, 2a
  preferencia y 2b S3), estado por preferencia en el resultado del backend y documentación. **Sin** cambios
  de esquema (vigente **v6**, el de la 015; la 013 **no** lo cambia), del contrato de salida, de las
  constantes S1/S2/S3 ni del seed (que sigue con 0 preferencias y ya no declara «Aula de estudio»).

```text
Comandos de referencia
  backend:      cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
  suite completa: cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

---

## T1 - Preferencia proyectada en el contrato, V19 y preset sin la sección

**Fecha:** _(pendiente)_
**Requisito:** RF-2, RF-7, RNF-1. **Plan:** §1.1, §1.1.1 y §4-T1.

### Rojo (tests primero)

_(pendiente)_

### Verde

_(pendiente)_

### Archivos tocados

_(pendiente)_

### Hallazgos / desviaciones

_(pendiente)_

---

## T2 - Rejilla compartida, proyección reloj → franja y filtro de día (puro)

**Fecha:** _(pendiente)_
**Requisito:** RF-3, RNF-1. **Plan:** §1.2 y §4-T2.

### Rojo (tests primero)

_(pendiente)_

### Verde

_(pendiente)_

### Archivos tocados

_(pendiente)_

### Hallazgos / desviaciones

_(pendiente)_

---

## T3 - Construcción de la entrada: preferencias desde los datos, preset ignorado y orden con V19

**Fecha:** _(pendiente)_
**Requisito:** RF-2, RF-3, RF-7, RNF-4. **Plan:** §1.3 y §4-T3.

### Rojo (tests primero)

_(pendiente)_

### Verde

_(pendiente)_

### Archivos tocados

_(pendiente)_

### Hallazgos / desviaciones

_(pendiente)_

---

## T4 - Huella con intervinientes y cableado mínimo del frontend

**Fecha:** _(pendiente)_
**Requisito:** RF-6, RNF-6. **Plan:** §1.4, §1.6 y §4-T4.

### Rojo (tests primero)

_(pendiente)_

### Verde

_(pendiente)_

### Archivos tocados

_(pendiente)_

### Hallazgos / desviaciones

_(pendiente)_

---

## T5 - Objetivo: fase 1 de referencia y fase 2 con anclaje, deduplicación y fallback

**Fecha:** _(pendiente)_
**Requisito:** RF-1, RF-4, RF-5. **Plan:** §1.5 y §4-T5. **Estimación aceptada:** ~45–60 min.

### Rojo (tests primero)

_(pendiente)_

### Verde

_(pendiente)_

### Archivos tocados

_(pendiente)_

### Hallazgos / desviaciones

_(pendiente)_

---

## T6 - Jerarquía preferencia > S3 (fase 2b), presupuesto y medición

**Fecha:** _(pendiente)_
**Requisito:** RF-5, RNF-2, RNF-3. **Plan:** §1.5, §3 y §4-T6.

### Rojo (tests primero)

_(pendiente)_

### Verde

_(pendiente)_

### Medición (RNF-3)

- Referencia (sin preferencias): _(pendiente)_
- Con preferencias (tres fases): _(pendiente)_
- Diferencia: _(pendiente)_ — umbral: < 60 s y ≤ +50 %.

### Archivos tocados

_(pendiente)_

### Hallazgos / desviaciones

_(pendiente)_

---

## T7 - Estado por preferencia y cumplimiento en el resultado del backend

**Fecha:** _(pendiente)_
**Requisito:** RF-8, RNF-1. **Plan:** §1.7 y §4-T7.

### Rojo (tests primero)

_(pendiente)_

### Verde

_(pendiente)_

### Archivos tocados

_(pendiente)_

### Hallazgos / desviaciones

_(pendiente)_

---

## T8 - Documentación del contrato y de la constitución

**Fecha:** _(pendiente)_
**Requisito:** RF-9. **Plan:** §1.8 y §4-T8.

### Cambios

_(pendiente)_

### Verificación

_(pendiente)_

---

## T9 - Supersesiones, acotación de la equivalencia y riesgo documental

**Fecha:** _(pendiente)_
**Requisito:** RF-9, RNF-4. **Plan:** §1.8 y §4-T9.

### Cambios

_(pendiente)_

### Verificación

_(pendiente)_

---

## T10 - Regresión global y suite completa

**Fecha:** _(pendiente)_
**Requisito:** RNF-4, RNF-5, RNF-7. **Plan:** §4-T10.

### Regresión

_(pendiente)_

### Suite

_(pendiente)_

### Cierre

_(pendiente)_
