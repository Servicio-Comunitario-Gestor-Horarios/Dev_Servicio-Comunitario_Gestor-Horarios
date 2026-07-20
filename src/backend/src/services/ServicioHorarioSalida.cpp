#include "backend/services/ServicioHorarioSalida.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

// ─── Constructor ───────────────────────────────────────────────────────────

ServicioHorarioSalida::ServicioHorarioSalida(const QString& directorioSalida)
    : m_directorio(directorioSalida) {}

// ─── Helpers ───────────────────────────────────────────────────────────────

QString ServicioHorarioSalida::rutaArchivo(const QString& nombreArchivo) const {
    return m_directorio + "/" + nombreArchivo;
}

DiaOutput* ServicioHorarioSalida::buscarDia(const QString& curso, int dia) {
    if (!m_horario.horarios.contains(curso)) {
        return nullptr;
    }
    auto& dias = m_horario.horarios[curso].dias;
    for (int i = 0; i < dias.size(); ++i) {
        if (dias[i].dia == dia) {
            return &dias[i];
        }
    }
    return nullptr;
}

AsignacionOutput* ServicioHorarioSalida::buscarAsignacion(const QString& curso, int dia, int slot) {
    auto* diaPtr = buscarDia(curso, dia);
    if (!diaPtr) {
        return nullptr;
    }
    for (int i = 0; i < diaPtr->asignaciones.size(); ++i) {
        if (diaPtr->asignaciones[i].slot == slot) {
            return &diaPtr->asignaciones[i];
        }
    }
    return nullptr;
}

// ─── Archivos ──────────────────────────────────────────────────────────────

QVector<QString> ServicioHorarioSalida::listarArchivos() const {
    QVector<QString> archivos;
    QDir directorio(m_directorio);
    if (!directorio.exists()) {
        return archivos;
    }

    QStringList filtros;
    filtros << "*.json";
    QStringList nombres = directorio.entryList(filtros, QDir::Files, QDir::Name);
    for (const auto& nombre : nombres) {
        archivos.append(nombre);
    }
    return archivos;
}

// ─── Carga / Guardado ─────────────────────────────────────────────────────

Resultado<HorarioSalida> ServicioHorarioSalida::cargarHorario(const QString& nombreArchivo) {
    QString ruta = rutaArchivo(nombreArchivo);
    QFile archivo(ruta);
    if (!archivo.exists()) {
        return Resultado<HorarioSalida>::error(
            QString("No se encontró el archivo: %1").arg(ruta));
    }
    if (!archivo.open(QIODevice::ReadOnly)) {
        return Resultado<HorarioSalida>::error(
            QString("No se pudo abrir el archivo: %1").arg(archivo.errorString()));
    }

    QJsonParseError errorParse;
    QJsonDocument doc = QJsonDocument::fromJson(archivo.readAll(), &errorParse);
    archivo.close();

    if (errorParse.error != QJsonParseError::NoError) {
        return Resultado<HorarioSalida>::error(
            QString("Error al parsear JSON: %1").arg(errorParse.errorString()));
    }

    if (!doc.isObject()) {
        return Resultado<HorarioSalida>::error("El documento JSON no es un objeto válido.");
    }

    QJsonObject raiz = doc.object();
    if (!raiz.contains("metadata") || !raiz.contains("horarios")) {
        return Resultado<HorarioSalida>::error(
            "Faltan campos requeridos ('metadata' o 'horarios') en el JSON.");
    }

    m_horario = HorarioSalida::fromJson(raiz);
    return Resultado<HorarioSalida>::exito(m_horario);
}

Resultado<bool> ServicioHorarioSalida::guardarHorario(const HorarioSalida& horario,
                                                      const QString& nombreArchivo) const {
    QDir directorio(m_directorio);
    if (!directorio.exists()) {
        if (!directorio.mkpath(".")) {
            return Resultado<bool>::error(
                QString("No se pudo crear el directorio: %1").arg(m_directorio));
        }
    }

    QString ruta = rutaArchivo(nombreArchivo);
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly)) {
        return Resultado<bool>::error(
            QString("No se pudo escribir el archivo: %1").arg(archivo.errorString()));
    }

    QJsonDocument doc(horario.toJson());
    archivo.write(doc.toJson(QJsonDocument::Indented));
    archivo.close();

    return Resultado<bool>::exito(true);
}

// ─── Consultas ─────────────────────────────────────────────────────────────

Resultado<CursoOutput> ServicioHorarioSalida::obtenerCurso(const QString& nombre) const {
    if (!m_horario.horarios.contains(nombre)) {
        return Resultado<CursoOutput>::error(
            QString("No se encontró el curso: %1").arg(nombre));
    }
    return Resultado<CursoOutput>::exito(m_horario.horarios[nombre]);
}

Resultado<QStringList> ServicioHorarioSalida::listarCursos() const {
    QStringList cursos = m_horario.horarios.keys();
    return Resultado<QStringList>::exito(cursos);
}

// ─── CRUD de asignaciones ──────────────────────────────────────────────────

Resultado<AsignacionOutput> ServicioHorarioSalida::agregarAsignacion(
    const QString& curso, int dia, const AsignacionOutput& asignacion) {

    if (!m_horario.horarios.contains(curso)) {
        return Resultado<AsignacionOutput>::error(
            QString("No se encontró el curso: %1").arg(curso));
    }

    auto* diaPtr = buscarDia(curso, dia);
    if (!diaPtr) {
        // Crear el día si no existe
        DiaOutput nuevoDia;
        nuevoDia.dia = dia;
        m_horario.horarios[curso].dias.append(nuevoDia);
        diaPtr = buscarDia(curso, dia);
    }

    // Verificar que no exista un slot duplicado
    for (const auto& a : diaPtr->asignaciones) {
        if (a.slot == asignacion.slot) {
            return Resultado<AsignacionOutput>::error(
                QString("Ya existe una asignación en el slot %1 del día %2 para el curso %3.")
                    .arg(asignacion.slot).arg(dia).arg(curso));
        }
    }

    diaPtr->asignaciones.append(asignacion);
    return Resultado<AsignacionOutput>::exito(asignacion);
}

Resultado<bool> ServicioHorarioSalida::eliminarAsignacion(const QString& curso, int dia, int slot) {
    auto* diaPtr = buscarDia(curso, dia);
    if (!diaPtr) {
        return Resultado<bool>::error(
            QString("No se encontró el día %1 en el curso %2.").arg(dia).arg(curso));
    }

    for (int i = 0; i < diaPtr->asignaciones.size(); ++i) {
        if (diaPtr->asignaciones[i].slot == slot) {
            diaPtr->asignaciones.remove(i);
            return Resultado<bool>::exito(true);
        }
    }

    return Resultado<bool>::error(
        QString("No se encontró una asignación en el slot %1 del día %2.")
            .arg(slot).arg(dia));
}

Resultado<AsignacionOutput> ServicioHorarioSalida::modificarAsignacion(
    const QString& curso, int dia, int slot, const AsignacionOutput& nueva) {

    auto* ptr = buscarAsignacion(curso, dia, slot);
    if (!ptr) {
        return Resultado<AsignacionOutput>::error(
            QString("No se encontró una asignación en el slot %1 del día %2 en el curso %3.")
                .arg(slot).arg(dia).arg(curso));
    }

    *ptr = nueva;
    return Resultado<AsignacionOutput>::exito(nueva);
}
