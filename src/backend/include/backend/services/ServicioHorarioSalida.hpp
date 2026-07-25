#pragma once

#include <QString>
#include <QVector>
#include <backend/resultado.hpp>
#include <backend/data/horario_salida.hpp>

/**
 * @brief Servicio CRUD para horarios de salida (archivos JSON del solver).
 *
 * Opera sobre archivos .json en un directorio de salida.
 * Mantiene un horario cargado en memoria para operaciones de asignación.
 */
class ServicioHorarioSalida {
public:
    explicit ServicioHorarioSalida(const QString& directorioSalida);

    // ─── Archivos ──────────────────────────────────────────────────────────

    QVector<QString> listarArchivos() const;

    // ─── Carga / Guardado ─────────────────────────────────────────────────

    Resultado<HorarioSalida> cargarHorario(const QString& nombreArchivo);
    Resultado<bool> guardarHorario(const HorarioSalida& horario, const QString& nombreArchivo) const;

    // ─── Consultas ─────────────────────────────────────────────────────────

    Resultado<CursoOutput> obtenerCurso(const QString& nombre) const;
    Resultado<QStringList> listarCursos() const;

    // ─── CRUD de asignaciones ──────────────────────────────────────────────

    Resultado<AsignacionOutput> agregarAsignacion(const QString& curso, int dia,
                                                  const AsignacionOutput& asignacion);
    Resultado<bool> eliminarAsignacion(const QString& curso, int dia, int slot);
    Resultado<AsignacionOutput> modificarAsignacion(const QString& curso, int dia, int slot,
                                                    const AsignacionOutput& nueva);

private:
    QString m_directorio;
    HorarioSalida m_horario;

    /** @brief Ruta completa de un archivo de salida. */
    QString rutaArchivo(const QString& nombreArchivo) const;

    /** @brief Busca el puntero al día dentro de un curso, o nullptr. */
    DiaOutput* buscarDia(const QString& curso, int dia);

    /** @brief Busca una asignación por slot dentro de un día, o nullptr. */
    AsignacionOutput* buscarAsignacion(const QString& curso, int dia, int slot);
};
