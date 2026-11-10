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
#include "common/tipo_general.hpp"
#include "datos/ContextoBaseDatos.hpp"
#include "seed/docentes.hpp"

namespace seed {

namespace {

/// Las 12 tablas de dominio del esquema v6 (RF-1, detección de datos). `TiposAula`,
/// `Eventos` y `Preferencia_Profesor` son datos de dominio; `Recesos` ya no existe. La
/// configuración global (`ConfiguracionHoraria`) **no** es dato de dominio (spec 011,
/// RF-8): no bloquea una base recién creada. La fila del tipo general que deja el esquema
/// tampoco cuenta como dato (015 RF-1/RF-5): se excluye del recuento de `TiposAula`.
const char* const kTablasDominio[] = {
    "TiposAula", "Aulas", "Profesores", "Materias", "PlanEstudio", "PlanEstudio_Materia",
    "Turnos", "Eventos", "Cursos", "Profesor_Materia", "Disponibilidad_Profesor",
    "Preferencia_Profesor",
};

/// Número de filas de una tabla; -1 si la consulta falla.
int contarFilas(QSqlDatabase& db, const QString& tabla) {
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabla))) {
        return -1;
    }
    return q.next() ? q.value(0).toInt() : -1;
}

/// Filas de `TiposAula` que no son el tipo general: la fila que garantiza el esquema
/// (015 RF-1) no convierte una base recién creada en «base con datos». -1 si falla.
int contarTiposAulaDeDominio(QSqlDatabase& db) {
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM TiposAula WHERE clave <> :clave"));
    q.bindValue(QStringLiteral(":clave"), tipo_general::clave());
    if (!q.exec() || !q.next()) {
        return -1;
    }
    return q.value(0).toInt();
}

/// Suma de filas de dominio; > 0 si la base ya tiene datos. -1 si no es legible.
int contarFilasDeDominio(QSqlDatabase& db) {
    int total = 0;
    for (const char* tabla : kTablasDominio) {
        const QString nombre = QString::fromLatin1(tabla);
        const int n = nombre == QStringLiteral("TiposAula")
                          ? contarTiposAulaDeDominio(db)
                          : contarFilas(db, nombre);
        if (n < 0) {
            return -1;
        }
        total += n;
    }
    return total;
}

/// Valor efectivo y recuento de la configuración global leídos de la base (RF-7). La
/// duración queda en 0 si no hay fila legible.
void leerConfiguracionSembrada(QSqlDatabase& db, int* duracionMinutos, int* configuraciones) {
    *configuraciones = contarFilas(db, QStringLiteral("ConfiguracionHoraria"));
    *duracionMinutos = 0;

    QSqlQuery q(db);
    if (q.exec(QStringLiteral(
            "SELECT duracion_minutos FROM ConfiguracionHoraria WHERE id = 1")) &&
        q.next()) {
        *duracionMinutos = q.value(0).toInt();
    }
}

/// Recuentos leídos de la base (RF-8): se cuenta tras el commit, no desde el perfil. Incluye
/// la duración efectiva y su fila de configuración, que también se comparan.
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
    r.preferencias     = contarFilas(db, QStringLiteral("Preferencia_Profesor"));
    leerConfiguracionSembrada(db, &r.duracionMinutos, &r.configuraciones);
    return r;
}

/// ¿Los recuentos leídos coinciden con los esperados del perfil? (RF-8).
bool recuentosCoinciden(const Recuentos& actuales, const Recuentos& esperados) {
    return actuales.turnos == esperados.turnos && actuales.eventos == esperados.eventos &&
           actuales.tiposAula == esperados.tiposAula && actuales.aulas == esperados.aulas &&
           actuales.materias == esperados.materias && actuales.planes == esperados.planes &&
           actuales.planMaterias == esperados.planMaterias &&
           actuales.cursos == esperados.cursos && actuales.docentes == esperados.docentes &&
           actuales.asignaciones == esperados.asignaciones &&
           actuales.disponibilidades == esperados.disponibilidades &&
           actuales.preferencias == esperados.preferencias &&
           actuales.duracionMinutos == esperados.duracionMinutos &&
           actuales.configuraciones == esperados.configuraciones;
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

QString resumenTexto(const Perfil& perfil, const Recuentos& r) {
    QStringList lineas;

    // Centro del conjunto (RF-8). El nombre es ficticio para el conjunto derivado; el
    // documento del centro sigue siendo la fuente versionada.
    lineas << QStringLiteral("Centro: Liceo Nacional Robert Serra (2026/2027)");

    // Configuración global de la hora académica (RF-7): valor efectivo y su recuento.
    lineas << QStringLiteral("Hora académica: %1 min (configuraciones: %2)")
                  .arg(r.duracionMinutos)
                  .arg(r.configuraciones);

    // Recuentos por entidad, leídos de la base tras el commit (RF-8): el informe verifica
    // lo escrito, no lo declarado en el perfil.
    lineas << QStringLiteral("Turnos: %1   Eventos: %2   Aulas: %3   Materias: %4   "
                             "Planes: %5   Materias de plan: %6")
                  .arg(r.turnos)
                  .arg(r.eventos)
                  .arg(r.aulas)
                  .arg(r.materias)
                  .arg(r.planes)
                  .arg(r.planMaterias);
    lineas << QStringLiteral("Cursos: %1 (12 Mañana · 12 Tarde)   Docentes: %2   "
                             "Materias asignadas: %3   Disponibilidades: %4   "
                             "Tipos de aula: %5")
                  .arg(r.cursos)
                  .arg(r.docentes)
                  .arg(r.asignaciones)
                  .arg(r.disponibilidades)
                  .arg(r.tiposAula);

    // Preferencias horarias del docente (spec 012): el conjunto no siembra ninguna.
    lineas << QStringLiteral("Preferencias: %1").arg(r.preferencias);

    // Clases del centro (RF-8): Σ horas de los 24 cursos y las de Educación Física, que van a
    // la cancha (4 h × 24 cursos = 96; 48 por turno).
    const int clases = demandaTotal(perfil);
    int efManana = 0;
    int efTarde = 0;
    if (perfil.turnos.size() >= 1) {
        efManana = demandaDeAreaPorTurno(perfil, QStringLiteral("Educación Física"),
                                         perfil.turnos[0].nombre);
    }
    if (perfil.turnos.size() >= 2) {
        efTarde = demandaDeAreaPorTurno(perfil, QStringLiteral("Educación Física"),
                                        perfil.turnos[1].nombre);
    }
    const int clasesEf = efManana + efTarde;
    const QString efPorTurno = efManana == efTarde
                                   ? QString::number(efManana)
                                   : QStringLiteral("%1/%2").arg(efManana).arg(efTarde);
    lineas << QStringLiteral("Clases: %1 (EF %2; %3 por turno)")
                  .arg(clases)
                  .arg(clasesEf)
                  .arg(efPorTurno);

    // Clases por plan y por año (RF-8): Σ horas de los cursos de cada plan, global y por año.
    // Se deriva de los datos (no hay cifras escritas a mano); un año sin cursos del plan no se
    // imprime, como fija la spec (el 5.º de 31060 → cuatro valores).
    QStringList clasesPorPlan;
    QStringList clasesPorAnio;
    for (const PlanDef& plan : perfil.planes) {
        int totalPlan = 0;
        QVector<int> horasPorAnio(5, 0);
        int ultimoAnioConCursos = 0;
        for (const CursoDef& curso : perfil.cursos) {
            if (curso.plan != plan.codigo) {
                continue;
            }
            const int horas = horasDeCurso(perfil, curso);
            totalPlan += horas;
            horasPorAnio[curso.anio - 1] += horas;
            if (curso.anio > ultimoAnioConCursos) {
                ultimoAnioConCursos = curso.anio;
            }
        }
        clasesPorPlan << QStringLiteral("%1 %2").arg(plan.codigo).arg(totalPlan);

        QStringList porAnio;
        for (int anio = 1; anio <= ultimoAnioConCursos; ++anio) {
            porAnio << QString::number(horasPorAnio[anio - 1]);
        }
        clasesPorAnio << QStringLiteral("%1 %2").arg(plan.codigo, porAnio.join(QChar('/')));
    }
    lineas << QStringLiteral("Clases por plan: %1")
                  .arg(clasesPorPlan.join(QStringLiteral(" · ")));
    lineas << QStringLiteral("Clases por año: %1")
                  .arg(clasesPorAnio.join(QStringLiteral(" · ")));

    // Materias por año de cada plan (RF-8): se calcula del perfil (validado) y contrasta con
    // lo declarado en la spec (31059 8/8/9/10/10; 31060 10/10/10/10/10).
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

    // Totales semanales por plan y año, en horas académicas (= bloques, RF-8). El nombre del
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

    // Matrícula del centro (RF-4): vive en el perfil y no se persiste (889 = 579 + 310).
    int matricula = 0;
    int matricula31059 = 0;
    int matricula31060 = 0;
    for (const CursoDef& curso : perfil.cursos) {
        matricula += curso.matricula;
        if (curso.plan == QStringLiteral("31059")) {
            matricula31059 += curso.matricula;
        } else if (curso.plan == QStringLiteral("31060")) {
            matricula31060 += curso.matricula;
        }
    }
    lineas << QStringLiteral("Matrícula: %1 (%2 + %3)")
                  .arg(matricula)
                  .arg(matricula31059)
                  .arg(matricula31060);

    // Capacidad docente y demanda por turno (RF-8, 018): 16 docentes × 36 h en la mañana y 17 ×
    // 36 en la tarde (576 + 612 = 1188) frente a la demanda de clases de cada turno.
    const QVector<DocenteDef> docentes = docentesDelCentro(perfil);
    int docentesManana = 0;
    int docentesTarde = 0;
    int topeHorasAula = 0;
    for (const DocenteDef& docente : docentes) {
        if (perfil.turnos.size() >= 1 && docente.turno == perfil.turnos[0].nombre) {
            ++docentesManana;
        }
        if (perfil.turnos.size() >= 2 && docente.turno == perfil.turnos[1].nombre) {
            ++docentesTarde;
        }
        topeHorasAula = docente.horasAula;
    }
    const int capacidadManana = docentesManana * topeHorasAula;
    const int capacidadTarde = docentesTarde * topeHorasAula;
    const QString demandaManana =
        perfil.turnos.size() >= 1
            ? QString::number(demandaPorTurno(perfil, perfil.turnos[0].nombre))
            : QStringLiteral("0");
    const QString demandaTarde =
        perfil.turnos.size() >= 2
            ? QString::number(demandaPorTurno(perfil, perfil.turnos[1].nombre))
            : QStringLiteral("0");
    lineas << QStringLiteral("Capacidad docente: %1 (%2 + %3)   Demanda: %4 Mañana · "
                             "%5 Tarde")
                  .arg(capacidadManana + capacidadTarde)
                  .arg(capacidadManana)
                  .arg(capacidadTarde)
                  .arg(demandaManana, demandaTarde);

    // Aforo simultáneo de la cancha (018, RF-8): capacidad física (88 alumnos) y aforo de
    // cursos (2) son cifras distintas. Las posiciones de par salen del contrato.
    const CapacidadCancha cancha = capacidadCancha(perfil);
    lineas << QStringLiteral("Cancha: %1 %2 · %3 franjas por turno · aforo %4 → %5 plazas por "
                             "turno · %6 en la semana")
                  .arg(cancha.aulas)
                  .arg(cancha.aulas == 1 ? QStringLiteral("aula") : QStringLiteral("aulas"))
                  .arg(cancha.franjasPorTurno)
                  .arg(cancha.aforo)
                  .arg(cancha.plazasPorTurno)
                  .arg(cancha.plazasSemana);
    lineas << QStringLiteral("Educación Física: %1 clases por turno · %2 en la semana → %3 pares "
                             "por turno · %4 en la semana")
                  .arg(cancha.clasesEfPorTurno)
                  .arg(cancha.clasesEfSemana)
                  .arg(cancha.paresEfPorTurno)
                  .arg(cancha.clasesEfSemana / 2);
    lineas << QStringLiteral("Pares de cancha: %1 posiciones por turno · %2 plazas de par → %3 ≤ "
                             "%4 por turno")
                  .arg(cancha.posicionesParPorTurno)
                  .arg(cancha.plazasParPorTurno)
                  .arg(cancha.paresEfPorTurno)
                  .arg(cancha.plazasParPorTurno);

    // «Generable» (018, RF-8): el centro ya cabe con el aforo 2; sin motivo no se informa causa.
    const QString motivo = motivoNoGenerable(perfil);
    lineas << (motivo.isEmpty() ? QStringLiteral("Generable: sí")
                                : QStringLiteral("Generable: no — %1").arg(motivo));

    return lineas.join(QStringLiteral("\n"));
}

ResultadoAplicacion aplicar(const OpcionesAplicacion& opciones) {
    // 1. La base de destino es obligatoria (RF-1): nunca se actúa sin indicarla.
    if (opciones.rutaDb.trimmed().isEmpty()) {
        return fallo(EstadoAplicacion::UsoInvalido,
                     QStringLiteral("Debe indicar la base de destino con «--db <ruta>»."));
    }

    // 2. El perfil debe ser coherente antes de tocar la base (RF-2…RF-5, RNF-5).
    const Perfil& perfil = perfilPorDefecto();
    QString errorPerfil;
    if (!validarPerfil(perfil, &errorPerfil)) {
        return fallo(EstadoAplicacion::PerfilIncoherente, errorPerfil);
    }

    // 3. La plantilla de docentes (33) debe cumplir tope, turnos, disponibilidad y
    //    asignaciones, y cubrir todas las materias de todos los cursos en su turno (RF-6).
    const QVector<DocenteDef> docentes = docentesDelCentro(perfil);
    QString errorDocentes;
    if (!validarDocentes(perfil, docentes, &errorDocentes)) {
        return fallo(EstadoAplicacion::PerfilIncoherente, errorDocentes);
    }
    if (!coberturaDelCentro(perfil, docentes, &errorDocentes)) {
        return fallo(EstadoAplicacion::PerfilIncoherente, errorDocentes);
    }

    // 4. Apertura o creación de la base con el esquema esperado (RF-1).
    ContextoBaseDatos contexto;
    const AperturaBaseDatos::Resultado apertura = contexto.abrir(opciones.rutaDb);
    if (!apertura.ok()) {
        if (apertura.estado == AperturaBaseDatos::Estado::FalloEsquema) {
            return fallo(EstadoAplicacion::EsquemaNoEsperado, apertura.detalle);
        }
        return fallo(EstadoAplicacion::FalloApertura, apertura.detalle);
    }

    QSqlDatabase& db = contexto.conexion();

    // 5. Detección de datos de dominio (RF-1). Si la base ya tiene datos y no se pide una
    //    operación destructiva, se falla sin escribir.
    const int filasDominio = contarFilasDeDominio(db);
    if (filasDominio < 0) {
        return fallo(EstadoAplicacion::FalloApertura,
                     QStringLiteral("No se pudo comprobar si la base ya tiene datos."));
    }
    const bool hayDatos = filasDominio > 0;
    if (hayDatos && !opciones.reemplazarTodo) {
        return fallo(EstadoAplicacion::BaseConDatos,
                     QStringLiteral("La base ya tiene datos; use «--reemplazar» para reemplazarlos."));
    }

    // 6. Confirmación y respaldo previo (RF-1). El reemplazo total exige confirmación
    //    explícita y deja respaldo obligatorio antes de borrar nada. El respaldo se crea
    //    ANTES de la transacción (`VACUUM INTO` no puede ejecutarse dentro de una).
    QString rutaRespaldo;
    if (opciones.reemplazarTodo) {
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

    // 7. Transacción única SQLite (RF-1): o se escriben todos los datos, o ninguno.
    if (!db.transaction()) {
        return fallo(EstadoAplicacion::FalloEscritura,
                     QStringLiteral("No se pudo iniciar la transacción de escritura."));
    }

    NucleoDatos& nucleo = contexto.nucleo();
    const auto abortar = [&db](const QString& detalle) -> ResultadoAplicacion {
        db.rollback();
        return fallo(EstadoAplicacion::FalloEscritura, detalle);
    };

    // La configuración global no es dato de dominio (RF-7): se asegura en la duración del
    // perfil (40) dentro de la transacción y antes de crear turnos, porque el servicio de
    // turnos valida el encaje con la duración vigente. Es idempotente (una sola fila) y
    // participa del rollback si algo falla después.
    {
        const auto configuracion =
            nucleo.guardarConfiguracionHoraria(perfil.duracionMinutos);
        if (!configuracion.ok) {
            return abortar(configuracion.mensajeError);
        }
    }

    // Borrado por SQL directo en orden FK-seguro. Los métodos con transacción propia
    // (`eliminarProfesor`, `eliminarTurno`, `eliminarConCascada`) están prohibidos aquí.
    const auto borrarTabla = [&db](const QString& tabla) -> bool {
        QSqlQuery q(db);
        return q.exec(QStringLiteral("DELETE FROM %1").arg(tabla));
    };

    // Mapa materia → id, construido desde las materias recién insertadas.
    QHash<QString, int> idMateria;

    // Inserción de docentes (RF-6) por SQL directo dentro de la transacción: tres
    // `QSqlQuery` preparados una vez y `bindValue` por fila. No usa `ServicioProfesor` para
    // mantener el alta determinista y sin transacción anidada. Devuelve false y rellena
    // `error` si falla (ya revirtió).
    const auto insertarDocentes = [&](QString* error) -> bool {
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

        // Fallo simulado tras los docentes (solo tests, atomicidad): revierte todo.
        if (opciones.simularFalloTrasDocentes) {
            *error = QStringLiteral("Fallo simulado tras insertar los docentes.");
            return false;
        }

        return true;
    };

    if (opciones.reemplazarTodo) {
        // Orden FK-seguro: Eventos antes que Turnos; Cursos antes que Turnos y Aulas;
        // Preferencia_Profesor/PlanEstudio_Materia/Profesor_Materia/Disponibilidad_Profesor
        // antes de sus padres; Aulas y Materias antes que TiposAula; Profesores al final.
        for (const char* tabla :
             {"Eventos", "Preferencia_Profesor", "Disponibilidad_Profesor",
              "Profesor_Materia", "Cursos", "PlanEstudio_Materia", "PlanEstudio",
              "Materias", "Turnos", "Aulas", "TiposAula", "Profesores"}) {
            const QString nombre = QString::fromLatin1(tabla);
            bool borrado = false;
            if (nombre == QStringLiteral("TiposAula")) {
                // El tipo general está protegido contra el borrado (015 RF-3, disparador del
                // esquema): el reemplazo borra los especializados y lo conserva.
                QSqlQuery borrarTipos(db);
                borrarTipos.prepare(QStringLiteral(
                    "DELETE FROM TiposAula WHERE clave <> :clave"));
                borrarTipos.bindValue(QStringLiteral(":clave"), tipo_general::clave());
                borrado = borrarTipos.exec();
            } else {
                borrado = borrarTabla(nombre);
            }
            if (!borrado) {
                return abortar(QStringLiteral(
                    "No se pudieron borrar los datos previos para el reemplazo total."));
            }
        }
    }

    // Tipos de aula (RF-3/RF-5): el catálogo declarado se inserta antes que las aulas y las
    // materias, que lo referencian. El tipo general lo garantiza el esquema (015 RF-1/RF-5)
    // y aquí solo se resuelve su id.
    QHash<QString, int> idTipoAula;
    for (const QString& tipo : perfil.tiposAula) {
        const auto creado = nucleo.crearTipoAula(tipo);
        if (!creado.ok) {
            return abortar(creado.mensajeError);
        }
        idTipoAula.insert(tipo, creado.valor.id);
    }
    {
        QSqlQuery general(db);
        general.prepare(QStringLiteral("SELECT id FROM TiposAula WHERE clave = :clave"));
        general.bindValue(QStringLiteral(":clave"), tipo_general::clave());
        if (!general.exec() || !general.next()) {
            return abortar(QStringLiteral(
                "La base no tiene el tipo de aula general; no se puede sembrar el centro."));
        }
        idTipoAula.insert(tipo_general::nombre(), general.value(0).toInt());
    }

    // Turnos y sus eventos (RF-7): 2 turnos de 8 franjas con receso en ambos y acto cívico
    // en la mañana (3 eventos).
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

    // Materias (RF-3), con el tipo que pide su área (Educación Física → cancha; el resto,
    // general). Se guarda nombre → id para enlazar cargas y docentes.
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

    // Planes (RF-2) y sus 95 cargas por SQL directo. Nunca `asignarCarga` (abre su propia
    // transacción).
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

    // Cursos (RF-4): 24, con su turno y su aula de referencia (el plan no se persiste).
    for (const CursoDef& curso : perfil.cursos) {
        const auto creado = nucleo.crearCurso(curso.anio, curso.seccion, curso.turno,
                                              idAula.value(curso.aula, -1));
        if (!creado.ok) {
            return abortar(creado.mensajeError);
        }
    }

    QString error;
    if (!insertarDocentes(&error)) {
        return abortar(error);
    }

    // Comprobación de recuentos completos contra el perfil (RF-8), duración y configuración
    // incluidas: si lo escrito no coincide con lo esperado, se revierte y no queda nada a
    // medias (RF-1).
    const Recuentos esperados = recuentosEsperados(perfil);
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
