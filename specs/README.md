# specs/ — Convención de especificaciones

Cada spec vive en su propia carpeta:

```
specs/
└── NNN-nombre/
    ├── spec.md     # QUÉ y POR QUÉ
    ├── plan.md     # CÓMO
    └── tasks.md    # Tareas ejecutables
```

- **`NNN`**: número de 3 dígitos, correlativo e incremental (`001-`, `002-`, `003-`, …).
- **`nombre`**: en kebab-case, corto y descriptivo (`001-carga-materias`).

## Archivos

### `spec.md`
Estado (`borrador` | `aprobada` | `implementada`), contexto y objetivo, usuarios, historias de usuario, requisitos funcionales en EARS (CUANDO / SI / MIENTRAS / EL SISTEMA), requisitos no funcionales, casos límite, fuera de alcance, criterios de finalización y dudas abiertas. **Solo el QUÉ y el POR QUÉ**: sin stack, arquitectura ni nombres de archivos.

### `plan.md`
Archivos y responsabilidades · funciones puras (con `hoy` como parámetro) · algoritmo en pseudocódigo · interfaz · decisiones justificadas con su alternativa descartada · estrategia de tests con el comando de tests del proyecto (`ctest --preset full`). Indica qué RF cubre cada parte.

### `tasks.md`
```
- [ ] **Tn. <Descripción>.** RF-x, RF-y
    - Hecho cuando: <comprobación verificable>.
```
Máximo 20-30 min por tarea, en orden de dependencia. Más de 10 tareas ⇒ dividir la spec.

## Reglas de flujo

1. Ninguna fase avanza sin aprobación explícita del usuario.
2. La spec manda: fuera de la spec, no se implementa.
3. Cambios de alcance: primero `spec.md`, luego `plan.md`/`tasks.md`, después el código.
4. Implementación: **una tarea cada vez** — tests rojo → código → tests verde → marcar → parar.
5. Al cerrar la fase, actualizar `MEMORY.md` y el campo Estado de la spec.
