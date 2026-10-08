## Problema
Actualmente el login requiere **usuario y contraseña** (campos: Usuario + Contraseña). Para un sistema de uso interno en el liceo, esto añade fricción innecesaria.

## Solución Propuesta
Simplificar a **solo contraseña única** para acceder a la aplicación:
- Eliminar campo "Usuario"
- Contraseña fija: `123456` (hardcoded por ahora, configurable en futuro)
- Mantener todo el diseño visual actual: iconos vectoriales, toggle visibilidad (👁️/👁️‍🗨️), estados visuales error/éxito, QSS consistente con el resto del proyecto

## Criterios de Aceptación
- [ ] Login muestra solo un campo "Contraseña" con placeholder `••••••••`
- [ ] Icono de candado (🔐) en lugar de usuario + candado
- [ ] Toggle visibilidad de contraseña funciona (ojo abierto/cerrado)
- [ ] Contraseña correcta (`123456`) → mensaje "✓ Acceso concedido" + auto-cierre a los 300ms
- [ ] Contraseña incorrecta → mensaje "✗ Contraseña incorrecta" + campo en rojo + limpieza + focus
- [ ] Campo vacío → mensaje "✗ Por favor, ingresa la contraseña" + campo en rojo
- [ ] Enter en el campo dispara login
- [ ] Diseño homogéneo: misma paleta (`#1e3a8a`, `#ef4444`, `#10b981`), sombras, bordes redondeados, tipografía

## Área Afectada
area-frontend

## Asignado Sugerido
Dani

## Sprint
Sprint 5

## Contexto Técnico
- Archivos a modificar: `src/frontend/src/logindialog.h`, `src/frontend/src/logindialog.cpp`
- Patrón a seguir: `logindialog.cpp` actual (iconos vectoriales lambda, QSS inline, `setProperty("error", true)` + unpolish/polish)
- Build verificación: `FRONTEND_STANDALONE=ON` y `BUILD_BACKEND=OFF`