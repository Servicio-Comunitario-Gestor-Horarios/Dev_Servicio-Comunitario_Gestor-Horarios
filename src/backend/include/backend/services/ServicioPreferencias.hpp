#pragma once

/**
 * @file ServicioPreferencias.hpp
 * @brief Servicio CRUD de las preferencias horarias del docente (RF-1, RF-2, RF-3).
 *
 * Persiste preferencias (docente, día 0–6, rango y sentido preferida/evitada) sobre
 * `Preferencia_Profesor`. La validación de día y rango es la compartida
 * (`franja_horaria::errorDeFranja`) y el sentido y el solape vienen del modelo puro
 * (`preferencias`). No participa en la generación: solo se guarda y se expone.
 *
 * Las reglas de negocio: docente existente, sin duplicado exacto y sin solapes de
 * sentidos opuestos (los del mismo sentido y los rangos contiguos se permiten). Cada
 * rechazo cita el valor recibido y la acción disponible (RNF-1).
 */

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "backend/data/franja_horaria.hpp"
#include "backend/resultado.hpp"
#include "backend/services/preferencias.hpp"

/// Preferencia horaria persistida: identidad, docente, franja y sentido.
struct PreferenciaDTO {
    int id = 0;                  ///< id en la tabla Preferencia_Profesor
    QString idProfesor;          ///< docente al que pertenece
    FranjaHoraria franja;        ///< día 0–6 y rango inicio < fin
    SentidoPreferencia sentido = SentidoPreferencia::Preferida;
};

/// Servicio CRUD sobre la tabla `Preferencia_Profesor`.
class ServicioPreferencias {
public:
    explicit ServicioPreferencias(QSqlDatabase& db);

    /// Alta de una preferencia. Valida docente, franja, sentido, duplicado y solape
    /// opuesto; devuelve la preferencia con su id.
    Resultado<PreferenciaDTO> agregar(const QString& idProfesor, const FranjaHoraria& franja,
                                      SentidoPreferencia sentido);

    /// Preferencias del docente ordenadas por día, inicio, fin e id.
    /// Un docente sin preferencias (o inexistente) devuelve un vector vacío, no error.
    Resultado<QVector<PreferenciaDTO>> listar(const QString& idProfesor) const;

    /// Edición por identidad. El docente no cambia; el duplicado y el solape se
    /// comprueban contra el **día nuevo**, excluyendo la propia fila. Si no encaja, no
    /// escribe y conserva el valor anterior.
    Resultado<PreferenciaDTO> actualizar(int id, const FranjaHoraria& franja,
                                         SentidoPreferencia sentido);

    /// Baja por identidad; error explícito si no existe.
    Resultado<bool> eliminar(int id);

    /// Borra solo las preferencias del docente y devuelve cuántas borró (0 es válido).
    Resultado<int> limpiar(const QString& idProfesor);

private:
    QSqlDatabase& m_db;
};
