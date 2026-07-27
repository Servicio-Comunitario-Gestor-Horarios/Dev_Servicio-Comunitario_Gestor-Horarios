#include "backend/services/ServicioExportacion.hpp"
#include <QMap>
#include <QStringList>

namespace backend::services {

QString ServicioExportacion::escaparCsv(const QString& campo) {
    if (campo.contains(',') || campo.contains('"') || campo.contains('\n') || campo.contains('\r')) {
        QString escapado = campo;
        escapado.replace('"', "\"\"");
        return QString("\"%1\"").arg(escapado);
    }
    return campo;
}

QString ServicioExportacion::obtenerNombreDia(int dia) {
    static const QStringList dias = {"Lunes", "Martes", "Miercoles", "Jueves", "Viernes"};
    if (dia >= 0 && dia < dias.size()) {
        return dias[dia];
    }
    return "Desconocido";
}

Resultado<QString> ServicioExportacion::exportarCsvCursos(
    const HorarioSalida& horario,
    const SolverConfig& config) 
{
    QString resultadoStr;

    bool primerCurso = true;

    for (auto itCurso = horario.horarios.constBegin(); itCurso != horario.horarios.constEnd(); ++itCurso) {
        const QString& nombreCurso = itCurso.key();
        const CursoOutput& curso = itCurso.value();

        if (!primerCurso) {
            resultadoStr += "\n";
        }
        primerCurso = false;

        // Encabezado de la sección del curso
        resultadoStr += escaparCsv(QString("Curso: %1 (Turno: %2)").arg(nombreCurso, curso.turno)) + "\n";

        // Determinar slots visibles según turno
        int inicioSlot = 0;
        int finSlot = 5;
        if (curso.turno.trimmed().toLower() == "tarde") {
            inicioSlot = 6;
            finSlot = 11;
        }

        // Construir fila de encabezados de slots
        QStringList headers;
        headers << "Dia";
        for (int slot = inicioSlot; slot <= finSlot; ++slot) {
            headers << QString("Slot %1").arg(slot);
        }
        resultadoStr += headers.join(',') + "\n";

        // Mapear asignaciones rápidamente: mapa[dia][slot] = AsignacionOutput
        QMap<int, QMap<int, AsignacionOutput>> asignacionesMapa;
        for (const DiaOutput& diaObj : curso.dias) {
            for (const AsignacionOutput& asig : diaObj.asignaciones) {
                asignacionesMapa[diaObj.dia][asig.slot] = asig;
            }
        }

        // Iterar días laborables (0 al 4: Lunes a Viernes)
        for (int dia = 0; dia < 5; ++dia) {
            QStringList fila;
            fila << obtenerNombreDia(dia);

            for (int slot = inicioSlot; slot <= finSlot; ++slot) {
                if (asignacionesMapa.contains(dia) && asignacionesMapa[dia].contains(slot)) {
                    const AsignacionOutput& asig = asignacionesMapa[dia][slot];

                    if (asig.materia < 0 || asig.materia >= config.materias.size()) {
                        return Resultado<QString>::error(QString("Índice de materia fuera de rango: %1").arg(asig.materia));
                    }
                    if (asig.profesor < 0 || asig.profesor >= config.profesores.size()) {
                        return Resultado<QString>::error(QString("Índice de profesor fuera de rango: %1").arg(asig.profesor));
                    }
                    if (asig.aula < 0 || asig.aula >= config.aulas.size()) {
                        return Resultado<QString>::error(QString("Índice de aula fuera de rango: %1").arg(asig.aula));
                    }

                    const QString& nombreMat = config.materias[asig.materia].nombre;
                    const QString& nombreProf = config.profesores[asig.profesor].nombre;
                    const QString& nombreAula = config.aulas[asig.aula].nombre;

                    QString celda = QString("%1 (%2 / %3)").arg(nombreMat, nombreProf, nombreAula);
                    fila << escaparCsv(celda);
                } else {
                    fila << "";
                }
            }
            resultadoStr += fila.join(',') + "\n";
        }
    }

    return Resultado<QString>::exito(resultadoStr);
}

Resultado<QString> ServicioExportacion::exportarCsvProfesores(
    const HorarioSalida& horario,
    const SolverConfig& config) 
{
    // Estructura interna de agrupación: mapaProfesorAsignaciones[indiceProfesor][dia][slot] = "Curso - Materia"
    QMap<int, QMap<int, QMap<int, QString>>> mapaProfesorAsignaciones;

    for (auto itCurso = horario.horarios.constBegin(); itCurso != horario.horarios.constEnd(); ++itCurso) {
        const QString& nombreCurso = itCurso.key();
        const CursoOutput& curso = itCurso.value();

        for (const DiaOutput& diaObj : curso.dias) {
            for (const AsignacionOutput& asig : diaObj.asignaciones) {
                // Validación de límites
                if (asig.profesor < 0 || asig.profesor >= config.profesores.size()) {
                    return Resultado<QString>::error(QString("Índice de profesor fuera de rango: %1").arg(asig.profesor));
                }
                if (asig.materia < 0 || asig.materia >= config.materias.size()) {
                    return Resultado<QString>::error(QString("Índice de materia fuera de rango: %1").arg(asig.materia));
                }

                const QString& nombreMat = config.materias[asig.materia].nombre;
                QString celda = QString("%1 - %2").arg(nombreCurso, nombreMat);

                mapaProfesorAsignaciones[asig.profesor][diaObj.dia][asig.slot] = celda;
            }
        }
    }

    QString resultadoStr;
    bool primerProfesor = true;

    // Se iteran todos los profesores declarados en la configuración
    for (int profIdx = 0; profIdx < config.profesores.size(); ++profIdx) {
        if (!primerProfesor) {
            resultadoStr += "\n";
        }
        primerProfesor = false;

        const QString& nombreProfesor = config.profesores[profIdx].nombre;
        resultadoStr += escaparCsv(QString("Profesor: %1").arg(nombreProfesor)) + "\n";

        // Los profesores cubren el rango completo de slots (0 al 11)
        QStringList headers;
        headers << "Dia";
        for (int slot = 0; slot <= 11; ++slot) {
            headers << QString("Slot %1").arg(slot);
        }
        resultadoStr += headers.join(',') + "\n";

        const auto& mapaDias = mapaProfesorAsignaciones[profIdx];

        for (int dia = 0; dia < 5; ++dia) {
            QStringList fila;
            fila << obtenerNombreDia(dia);

            for (int slot = 0; slot <= 11; ++slot) {
                if (mapaDias.contains(dia) && mapaDias[dia].contains(slot)) {
                    fila << escaparCsv(mapaDias[dia][slot]);
                } else {
                    fila << "";
                }
            }
            resultadoStr += fila.join(',') + "\n";
        }
    }

    return Resultado<QString>::exito(resultadoStr);
}

QJsonObject ServicioExportacion::exportarJson(const HorarioSalida& horario) {
    return horario.toJson();
}

} // namespace backend::services
