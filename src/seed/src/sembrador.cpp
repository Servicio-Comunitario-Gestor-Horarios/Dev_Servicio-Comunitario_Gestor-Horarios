#include "seed/sembrador.hpp"

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

#include "backend/database/apertura_base_datos.hpp"
#include "backend/database/respaldo.hpp"
#include "backend/services/NucleoDatos.hpp"
#include "datos/ContextoBaseDatos.hpp"

namespace seed {

namespace {

/// ¿La variante es una de las cuatro definidas? (RF-9: rechazar variantes inexistentes.)
bool varianteValida(VarianteDocentes variante) {
    switch (variante) {
        case VarianteDocentes::Holgada:
        case VarianteDocentes::Suplentes:
        case VarianteDocentes::SinProfesorado:
        case VarianteDocentes::SinDisponibilidad:
            return true;
    }
    return false;
}

/// Las 11 tablas de dominio del esquema v3 (RF-10, detección de datos). `TiposAula` y
/// `Eventos` son datos de dominio desde la spec 009; `Recesos` ya no existe.
const char* const kTablasDominio[] = {
    "TiposAula", "Aulas", "Profesores", "Materias", "PlanEstudio", "PlanEstudio_Materia",
    "Turnos", "Eventos", "Cursos", "Profesor_Materia", "Disponibilidad_Profesor",
};

/// Número de filas de una tabla; -1 si la consulta falla.
int contarFilas(QSqlDatabase& db, const QString& tabla) {
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabla))) {
        return -1;
    }
    return q.next() ? q.value(0).toInt() : -1;
}

/// Suma de filas de dominio; > 0 si la base ya tiene datos. -1 si no es legible.
int contarFilasDeDominio(QSqlDatabase& db) {
    int total = 0;
    for (const char* tabla : kTablasDominio) {
        const int n = contarFilas(db, QString::fromLatin1(tabla));
        if (n < 0) {
            return -1;
        }
        total += n;
    }
    return total;
}

/// Recuentos leídos de la base (RF-12): se cuenta tras el commit, no desde el perfil.
Recuentos leerRecuentos(QSqlDatabase& db) {
    Recuentos r;
    r.turnos           = contarFilas(db, QStringLiteral("Turnos"));
    r.eventos          = contarFilas(db, QStringLiteral("Eventos"));
    r.tiposAula        = contarFilas(db, QStringLiteral("TiposAula"));
    r.aulas            = contarFilas(db, QStringLiteral("Aulas"));
    r.materias         = contarFilas(db, QStringLiteral("Materias"));
    r.planes           = contarFilas(db, QStringLiteral("PlanEstudio"));
    r.planMaterias     = contarFilas(db, QStringLiteral("PlanEstudio_Materia"));
    r.cursos           = contarFilas(db, QStringLiteral("Cursos"));
    r.docentes         = contarFilas(db, QStringLiteral("Profesores"));
    r.asignaciones     = contarFilas(db, QStringLiteral("Profesor_Materia"));
    r.disponibilidades = contarFilas(db, QStringLiteral("Disponibilidad_Profesor"));
    return r;
}

/// ¿Los recuentos leídos coinciden con los esperados para la variante? (RF-12).
bool recuentosCoinciden(const Recuentos& actuales, const Recuentos& esperados) {
    return actuales.turnos == esperados.turnos && actuales.eventos == esperados.eventos &&
           actuales.tiposAula == esperados.tiposAula && actuales.aulas == esperados.aulas &&
           actuales.materias == esperados.materias && actuales.planes == esperados.planes &&
           actuales.planMaterias == esperados.planMaterias &&
           actuales.cursos == esperados.cursos && actuales.docentes == esperados.docentes &&
           actuales.asignaciones == esperados.asignaciones &&
           actuales.disponibilidades == esperados.disponibilidades;
}

ResultadoAplicacion fallo(EstadoAplicacion estado, QString detalle) {
    ResultadoAplicacion resultado;
    resultado.estado = estado;
    resultado.detalle = std::move(detalle);
    return resultado;
}

} // namespace

QString rutaRespaldoLibre(const QString& rutaDb, const QDateTime& ahora) {
    const QString base = Respaldo::construirNombreRespaldo(rutaDb, ahora);
    if (!QFile::exists(base)) {
        return base;
    }
    for (int n = 2; n <= 99; ++n) {
        const QString candidato = QStringLiteral("%1-%2").arg(base).arg(n);
        if (!QFile::exists(candidato)) {
            return candidato;
        }
    }
    return QString();
}

QString resumenTexto(const Perfil& perfil, VarianteDocentes variante, const Recuentos& r) {
    QStringList lineas;

    // Variante (parámetro, plan §8): no forma parte de `Recuentos`, que cuenta filas.
    lineas << QStringLiteral("Variante: %1").arg(nombreVariante(variante));

    // Recuentos por entidad, leídos de la base tras el commit (RF-12): el informe verifica
    // lo escrito, no lo declarado en el perfil.
    lineas << QStringLiteral("Turnos: %1   Eventos: %2   Aulas: %3   Materias: %4   "
                             "Planes: %5   Materias de plan: %6")
                  .arg(r.turnos)
                  .arg(r.eventos)
                  .arg(r.aulas)
                  .arg(r.materias)
                  .arg(r.planes)
                  .arg(r.planMaterias);
    lineas << QStringLiteral("Cursos: %1 (sin turno)   Docentes: %2   "
                             "Materias asignadas: %3   Disponibilidades: %4   "
                             "Tipos de aula: %5")
                  .arg(r.cursos)
                  .arg(r.docentes)
                  .arg(r.asignaciones)
                  .arg(r.disponibilidades)
                  .arg(r.tiposAula);

    // Materias por año de cada plan (RF-12): se calcula del perfil (validado) y contrasta con
    // lo declarado en la spec (GEN 9/9/10/11/12; derivados 9/9/10/12/13).
    QStringList materiasPorPlan;
    for (const PlanDef& plan : perfil.planes) {
        QStringList porAnio;
        for (int anio = 1; anio <= 5; ++anio) {
            int contador = 0;
            for (const CargaPlan& carga : plan.cargas) {
                if (carga.anio == anio) {
                    ++contador;
                }
            }
            porAnio << QString::number(contador);
        }
        materiasPorPlan << QStringLiteral("%1 %2").arg(plan.codigo, porAnio.join(QStringLiteral("/")));
    }
    lineas << QStringLiteral("Materias por año: %1")
                  .arg(materiasPorPlan.join(QStringLiteral(" · ")));

    // Totales semanales por plan y año, en horas académicas (= bloques, RF-12). El nombre del
    // plan se alinea a la izquierda para una lectura estable y legible.
    int anchoNombre = 0;
    for (const PlanDef& plan : perfil.planes) {
        if (plan.nombre.size() > anchoNombre) {
            anchoNombre = plan.nombre.size();
        }
    }
    for (const PlanDef& plan : perfil.planes) {
        QStringList porAnio;
        for (int anio = 1; anio <= 5; ++anio) {
            porAnio << QString::number(horasDeAnio(plan, anio));
        }
        lineas << QStringLiteral("%1  %2%3 h académicas")
                      .arg(plan.codigo,
                           plan.nombre.leftJustified(anchoNombre + 1, QChar(' ')),
                           porAnio.join(QStringLiteral("/")));
    }

    return lineas.join(QStringLiteral("\n"));
}

ResultadoAplicacion aplicar(const OpcionesAplicacion& opciones) {
    // 1. La base de destino es obligatoria (RF-10): nunca se actúa sin indicarla.
    if (opciones.rutaDb.trimmed().isEmpty()) {
        return fallo(EstadoAplicacion::UsoInvalido,
                     QStringLiteral("Debe indicar la base de destino con «--db <ruta>»."));
    }

    // 2. El perfil debe ser coherente antes de tocar la base (RF-2/RF-3, RNF-3).
    const Perfil& perfil = perfilPorDefecto();
    QString errorPerfil;
    if (!validarPerfil(perfil, &errorPerfil)) {
        return fallo(EstadoAplicacion::PerfilIncoherente, errorPerfil);
    }

    // 3. Variante de docentes válida (RF-9).
    if (!varianteValida(opciones.variante)) {
        return fallo(EstadoAplicacion::VarianteDesconocida,
                     QStringLiteral("Variante de docentes desconocida."));
    }

    // 3bis. La variante pedida cumple las restricciones de carga del perfil (plan §5.7).
    //    Comprobación pura sobre datos ya validados: con el perfil canónico es cierta para
    //    las cuatro variantes, pero protege ante una tabla de carga mal editada.
    QString errorCarga;
    if (!docentesCumplenCarga(perfil, opciones.variante, &errorCarga)) {
        return fallo(EstadoAplicacion::PerfilIncoherente, errorCarga);
    }

    // 4. Operaciones contradictorias.
    if (opciones.reemplazarTodo && opciones.soloDocentes) {
        return fallo(EstadoAplicacion::UsoInvalido,
                     QStringLiteral("Elija una sola operación: «--reemplazar» o «--solo-docentes»."));
    }

    // 5. Apertura o creación de la base con el esquema esperado (RF-10).
    ContextoBaseDatos contexto;
    const AperturaBaseDatos::Resultado apertura = contexto.abrir(opciones.rutaDb);
    if (!apertura.ok()) {
        if (apertura.estado == AperturaBaseDatos::Estado::FalloEsquema) {
            return fallo(EstadoAplicacion::EsquemaNoEsperado, apertura.detalle);
        }
        return fallo(EstadoAplicacion::FalloApertura, apertura.detalle);
    }

    QSqlDatabase& db = contexto.conexion();

    // 6. Detección de datos de dominio (RF-10). Si la base ya tiene datos y no se pide una
    //    operación destructiva, se falla sin escribir.
    const int filasDominio = contarFilasDeDominio(db);
    if (filasDominio < 0) {
        return fallo(EstadoAplicacion::FalloApertura,
                     QStringLiteral("No se pudo comprobar si la base ya tiene datos."));
    }
    const bool hayDatos = filasDominio > 0;
    const bool destructiva = opciones.reemplazarTodo || opciones.soloDocentes;
    if (hayDatos && !destructiva) {
        return fallo(EstadoAplicacion::BaseConDatos,
                     QStringLiteral("La base ya tiene datos; use «--reemplazar» para reemplazarlos."));
    }

    // 7. Confirmación y respaldo previo (RF-9, RF-10, RNF-6). Toda operación destructiva exige
    //    confirmación explícita y deja respaldo obligatorio antes de borrar nada. El respaldo se
    //    crea ANTES de la transacción (`VACUUM INTO` no puede ejecutarse dentro de una).
    QString rutaRespaldo;
    if (destructiva) {
        if (!opciones.confirmado) {
            return fallo(EstadoAplicacion::ConfirmacionRequerida,
                         QStringLiteral("La operación es destructiva: confírmela con «--si»."));
        }
        if (hayDatos) {
            const QDateTime ahora =
                opciones.ahora.isValid() ? opciones.ahora : QDateTime::currentDateTime();
            rutaRespaldo = rutaRespaldoLibre(opciones.rutaDb, ahora);
            if (rutaRespaldo.isEmpty()) {
                return fallo(EstadoAplicacion::RespaldoFallido,
                             QStringLiteral("No se pudo elegir una ruta libre para el respaldo."));
            }
            QString errorRespaldo;
            if (!Respaldo::crearRespaldo(db, rutaRespaldo, &errorRespaldo)) {
                return fallo(EstadoAplicacion::RespaldoFallido, errorRespaldo);
            }
        }
    }

    // 8. Transacción única SQLite (RF-10, RNF-6): o se escriben todos los datos, o ninguno.
    if (!db.transaction()) {
        return fallo(EstadoAplicacion::FalloEscritura,
                     QStringLiteral("No se pudo iniciar la transacción de escritura."));
    }

    NucleoDatos& nucleo = contexto.nucleo();
    const auto abortar = [&db](const QString& detalle) -> ResultadoAplicacion {
        db.rollback();
        return fallo(EstadoAplicacion::FalloEscritura, detalle);
    };

    // Borrado por SQL directo en orden FK-seguro (plan §6.3). Los métodos con transacción
    // propia (`eliminarProfesor`, `eliminarTurno`, `eliminarConCascada`) están prohibidos aquí.
    const auto borrarTabla = [&db](const QString& tabla) -> bool {
        QSqlQuery q(db);
        return q.exec(QStringLiteral("DELETE FROM %1").arg(tabla));
    };

    // Mapa materia → id, construido desde la base (RF-1): se rellena con las materias recién
    // insertadas en el reemplazo total, o se lee del catálogo ya sembrado en la conmutación.
    QHash<QString, int> idMateria;
    const auto leerIdMaterias = [&db, &idMateria]() -> bool {
        idMateria.clear();
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral("SELECT id, nombre FROM Materias"))) {
            return false;
        }
        while (q.next()) {
            idMateria.insert(q.value(1).toString(), q.value(0).toInt());
        }
        return true;
    };

    // Inserción de docentes (RF-6/RF-7/RF-8) por SQL directo dentro de la transacción: tres
    // `QSqlQuery` preparados una vez y `bindValue` por fila. No usa `ServicioProfesor` para
    // mantener el alta determinista y sin transacción anidada (plan §6.4). Devuelve false y
    // rellena `error` si falla (ya revirtió).
    const auto insertarDocentes = [&](QString* error) -> bool {
        const QVector<DocenteDef> docentes = docentesDeVariante(perfil, opciones.variante);

        QSqlQuery insertProfesor(db);
        insertProfesor.prepare(
            "INSERT INTO Profesores (id, nombre, email, telefono) "
            "VALUES (:id, :nombre, :email, :telefono)");
        QSqlQuery insertAsignacion(db);
        insertAsignacion.prepare(
            "INSERT INTO Profesor_Materia (id_Profesor, id_Materia) "
            "VALUES (:idProfesor, :idMateria)");
        QSqlQuery insertDisponibilidad(db);
        insertDisponibilidad.prepare(
            "INSERT INTO Disponibilidad_Profesor (id_Profesor, dia, hora_inicio, hora_fin) "
            "VALUES (:idProfesor, :dia, :horaInicio, :horaFin)");

        for (const DocenteDef& docente : docentes) {
            insertProfesor.bindValue(QStringLiteral(":id"), docente.id);
            insertProfesor.bindValue(QStringLiteral(":nombre"), docente.nombre);
            insertProfesor.bindValue(QStringLiteral(":email"), docente.email);
            insertProfesor.bindValue(QStringLiteral(":telefono"), docente.telefono);
            if (!insertProfesor.exec()) {
                *error = QStringLiteral("No se pudo insertar el docente %1: %2")
                             .arg(docente.id, insertProfesor.lastError().text());
                return false;
            }

            for (const QString& materia : docente.materias) {
                insertAsignacion.bindValue(QStringLiteral(":idProfesor"), docente.id);
                insertAsignacion.bindValue(QStringLiteral(":idMateria"),
                                           idMateria.value(materia, -1));
                if (!insertAsignacion.exec()) {
                    *error = QStringLiteral("No se pudo asignar la materia «%1» al docente %2: %3")
                                 .arg(materia, docente.id, insertAsignacion.lastError().text());
                    return false;
                }
            }

            for (const FranjaDef& franja : docente.disponibilidad) {
                insertDisponibilidad.bindValue(QStringLiteral(":idProfesor"), docente.id);
                insertDisponibilidad.bindValue(QStringLiteral(":dia"),
                                               QString::number(franja.dia));
                insertDisponibilidad.bindValue(QStringLiteral(":horaInicio"),
                                               franja.inicio.toString(QStringLiteral("HH:mm")));
                insertDisponibilidad.bindValue(QStringLiteral(":horaFin"),
                                               franja.fin.toString(QStringLiteral("HH:mm")));
                if (!insertDisponibilidad.exec()) {
                    *error =
                        QStringLiteral("No se pudo insertar la disponibilidad del docente %1: %2")
                            .arg(docente.id, insertDisponibilidad.lastError().text());
                    return false;
                }
            }
        }

        // Fallo simulado tras los docentes (solo tests, atomicidad RF-10/RNF-6): revierte todo.
        if (opciones.simularFalloTrasDocentes) {
            *error = QStringLiteral("Fallo simulado tras insertar los docentes.");
            return false;
        }

        return true;
    };

    // ── Conmutación de docentes (RF-9): solo docentes + materias asignadas + disponibilidad ──
    if (opciones.soloDocentes) {
        if (!borrarTabla(QStringLiteral("Disponibilidad_Profesor")) ||
            !borrarTabla(QStringLiteral("Profesor_Materia")) ||
            !borrarTabla(QStringLiteral("Profesores"))) {
            return abortar(
                QStringLiteral("No se pudieron borrar los docentes para la conmutación de variante."));
        }
        if (!leerIdMaterias()) {
            return abortar(
                QStringLiteral("No se pudieron leer las materias para la conmutación de docentes."));
        }
        QString error;
        if (!insertarDocentes(&error)) {
            return abortar(error);
        }
    } else {
        // ── Alta completa (base nueva o reemplazo total) ──────────────────────────────
        if (opciones.reemplazarTodo) {
            // Orden FK-seguro (plan §4.5): Eventos antes que Turnos; Cursos antes que Turnos
            // y Aulas; PlanEstudio_Materia/Profesor_Materia/Disponibilidad_Profesor antes de
            // sus padres; Aulas y Materias antes que TiposAula; Profesores al final.
            for (const char* tabla :
                 {"Eventos", "Disponibilidad_Profesor", "Profesor_Materia", "Cursos",
                  "PlanEstudio_Materia", "PlanEstudio", "Materias", "Turnos", "Aulas",
                  "TiposAula", "Profesores"}) {
                if (!borrarTabla(QString::fromLatin1(tabla))) {
                    return abortar(QStringLiteral(
                        "No se pudieron borrar los datos previos para el reemplazo total."));
                }
            }
        }

        // Tipos de aula (RF-5/RF-10): el catálogo va antes que las aulas y las materias,
        // que lo referencian. Se guarda nombre → id para resolver el tipo de cada fila.
        QHash<QString, int> idTipoAula;
        for (const QString& tipo : perfil.tiposAula) {
            const auto creado = nucleo.crearTipoAula(tipo);
            if (!creado.ok) {
                return abortar(creado.mensajeError);
            }
            idTipoAula.insert(tipo, creado.valor.id);
        }

        // Turnos y sus eventos (RF-1/RF-4): 2 turnos, cada uno con su evento «Receso» de
        // 10 min tras la franja 2. El evento es la única fuente del receso (sin compatibilidad).
        for (const TurnoDef& turno : perfil.turnos) {
            const auto creado =
                nucleo.crearTurno(turno.nombre, turno.inicio, turno.fin, turno.numSlots);
            if (!creado.ok) {
                return abortar(creado.mensajeError);
            }
            for (const EventoDef& evento : turno.eventos) {
                const auto agregado = nucleo.agregarEvento(
                    turno.nombre, evento.nombre, evento.despuesDeSlot, evento.duracion,
                    evento.inicio, evento.fin, evento.orden);
                if (!agregado.ok) {
                    return abortar(agregado.mensajeError);
                }
            }
        }

        // Aulas (RF-5), con su tipo y guardando nombre → id para asignar el aula de cada curso.
        QHash<QString, int> idAula;
        for (const AulaDef& aula : perfil.aulas) {
            const auto creada = nucleo.crearAula(aula.nombre, aula.capacidad, aula.edificio,
                                                 aula.piso, idTipoAula.value(aula.tipo, -1));
            if (!creada.ok) {
                return abortar(creada.mensajeError);
            }
            idAula.insert(aula.nombre, creada.valor.id);
        }

        // Materias (RF-1/RF-7), con el tipo que pide su área (N1: si el conjunto no tiene
        // aula de ese tipo, genérico). Se guarda nombre → id para enlazar cargas y docentes.
        QSet<QString> tiposDeAulas;
        for (const AulaDef& aula : perfil.aulas) {
            tiposDeAulas.insert(aula.tipo);
        }
        for (const MateriaAnio& materia : perfil.materias) {
            const QString tipo = tipoFinalDeMateria(materia.nombre, tiposDeAulas);
            const auto creada =
                nucleo.crearMateria(materia.nombre, QString(), idTipoAula.value(tipo, -1));
            if (!creada.ok) {
                return abortar(creada.mensajeError);
            }
            idMateria.insert(materia.nombre, creada.valor.id);
        }

        // Planes (RF-2/RF-3) y las 210 cargas por SQL directo. Nunca `asignarCarga`
        // (abre su propia transacción; plan §6.4).
        QSqlQuery insertCarga(db);
        insertCarga.prepare(
            "INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas) "
            "VALUES (:codigo, :idMateria, :curso, :horas)");
        for (const PlanDef& plan : perfil.planes) {
            const auto creado = nucleo.crearPlan(plan.codigo, plan.nombre, plan.areaDesarrollo);
            if (!creado.ok) {
                return abortar(creado.mensajeError);
            }
            for (const CargaPlan& carga : plan.cargas) {
                insertCarga.bindValue(":codigo", plan.codigo);
                insertCarga.bindValue(":idMateria", idMateria.value(carga.materia, -1));
                insertCarga.bindValue(":curso", carga.anio);
                insertCarga.bindValue(":horas", carga.horas);
                if (!insertCarga.exec()) {
                    return abortar(
                        QStringLiteral("No se pudo insertar la carga de «%1» en el plan %2: %3")
                            .arg(carga.materia, plan.codigo, insertCarga.lastError().text()));
                }
            }
        }

        // Cursos (RF-4): sin turno (NULL) y cada uno con su aula propia (RF-5).
        for (const CursoDef& curso : perfil.cursos) {
            const auto creado =
                nucleo.crearCurso(curso.anio, curso.seccion, QString(), idAula.value(curso.aula, -1));
            if (!creado.ok) {
                return abortar(creado.mensajeError);
            }
        }

        QString error;
        if (!insertarDocentes(&error)) {
            return abortar(error);
        }
    }

    // Comprobación de recuentos completos contra el perfil (RF-12): si lo escrito no coincide
    // con lo esperado, se revierte y no queda nada a medias (RF-10).
    const Recuentos esperados = recuentosEsperados(perfil, opciones.variante);
    const Recuentos actuales = leerRecuentos(db);
    if (!recuentosCoinciden(actuales, esperados)) {
        return abortar(QStringLiteral("Los recuentos escritos no coinciden con el perfil."));
    }

    if (!db.commit()) {
        db.rollback();
        return fallo(EstadoAplicacion::FalloEscritura,
                     QStringLiteral("No se pudo confirmar la escritura: %1")
                         .arg(db.lastError().text()));
    }

    ResultadoAplicacion resultado;
    resultado.estado = EstadoAplicacion::Ok;
    resultado.rutaRespaldo = rutaRespaldo;
    resultado.recuentos = leerRecuentos(db);
    return resultado;
}

} // namespace seed
