#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QVariant>
#include <QVector>

#include <algorithm>

#include <backend/database/apertura_base_datos.hpp>
#include <backend/database/respaldo.hpp>
#include <seed/sembrador.hpp>

using namespace seed;

namespace {

/// Abre una conexión de solo lectura para inspeccionar una base ya creada (sin el contexto).
QSqlDatabase abrirInspeccion(const QString& ruta, int& contador)
{
    const QString nombre =
        QStringLiteral("test_seed_aplicacion_inspeccion_%1").arg(++contador);
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombre);
    db.setDatabaseName(ruta);
    db.open();
    return db;
}

void cerrarInspeccion(QSqlDatabase& db)
{
    const QString nombre = db.connectionName();
    if (db.isOpen()) {
        db.close();
    }
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(nombre);
}

int contar(QSqlDatabase& db, const QString& tabla)
{
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabla));
    return q.next() ? q.value(0).toInt() : -1;
}

/// Vuelca una consulta como texto estable (una cadena por fila, columnas unidas por «|»),
/// ordenada alfabéticamente, para comparar dos bases ignorando ids y marcas de auditoría.
/// Si la consulta falla, devuelve una única entrada «__ERROR__ …» para que la comparación
/// falle de forma visible.
QStringList filasDe(QSqlDatabase& db, const QString& sql)
{
    QStringList filas;
    QSqlQuery q(db);
    if (!q.exec(sql)) {
        filas.append(QStringLiteral("__ERROR__ %1").arg(q.lastError().text()));
        return filas;
    }
    while (q.next()) {
        QStringList valores;
        const int columnas = q.record().count();
        for (int i = 0; i < columnas; ++i) {
            valores.append(q.value(i).toString());
        }
        filas.append(valores.join(QStringLiteral("|")));
    }
    std::sort(filas.begin(), filas.end());
    return filas;
}

} // namespace

class TestSeedAplicacion : public QObject {
    Q_OBJECT

private:
    int m_contador = 0;

private slots:
    void creaBaseNuevaYPuebla()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;

        const ResultadoAplicacion resultado = aplicar(opciones);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);

        // Recuentos completos del conjunto (RF-12): la variante por defecto es la holgada
        // (65 docentes, 65 asignaciones y 325 disponibilidades = 65 × 5).
        QCOMPARE(resultado.recuentos.turnos, 2);
        QCOMPARE(resultado.recuentos.eventos, 2);
        QCOMPARE(resultado.recuentos.tiposAula, 3);
        QCOMPARE(resultado.recuentos.aulas, 16);
        QCOMPARE(resultado.recuentos.materias, 57);
        QCOMPARE(resultado.recuentos.planes, 4);
        QCOMPARE(resultado.recuentos.planMaterias, 210);
        QCOMPARE(resultado.recuentos.cursos, 13);
        QCOMPARE(resultado.recuentos.docentes, 65);
        QCOMPARE(resultado.recuentos.asignaciones, 65);
        QCOMPARE(resultado.recuentos.disponibilidades, 325);

        // El conjunto no siembra preferencias (RF-7): recuento 0 y tabla vacía.
        QCOMPARE(resultado.recuentos.preferencias, 0);

        // La configuración global se asegura en 35 con una sola fila (RF-8) y el resultado
        // informa el valor efectivo y su recuento.
        QCOMPARE(resultado.duracionMinutos, 35);
        QCOMPARE(resultado.configuraciones, 1);

        // Inspección directa de la base.
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());

        // Turnos (RF-4, RF-8): 8 franjas de 35 min y ventana 07:00–11:50 / 12:40–17:30
        // (8 × 35 + los 10 min del receso = 290).
        {
            QSqlQuery turnos(db);
            QVERIFY2(turnos.exec("SELECT nombre, inicio, fin, slots FROM Turnos ORDER BY nombre"),
                     qPrintable(turnos.lastError().text()));
            QVector<QStringList> filasTurnos;
            while (turnos.next()) {
                filasTurnos.append({turnos.value(0).toString(), turnos.value(1).toString(),
                                    turnos.value(2).toString(), turnos.value(3).toString()});
            }
            QCOMPARE(filasTurnos.size(), 2);
            QCOMPARE(filasTurnos[0], QStringList({"Mañana", "07:00", "11:50", "8"}));
            QCOMPARE(filasTurnos[1], QStringList({"Tarde", "12:40", "17:30", "8"}));
        }

        // Configuración global (RF-8): una sola fila `(1, 35)`, idempotente.
        QCOMPARE(contar(db, QStringLiteral("ConfiguracionHoraria")), 1);
        QCOMPARE(contar(db, QStringLiteral("Preferencia_Profesor")), 0);
        {
            QSqlQuery config(db);
            QVERIFY(config.exec("SELECT id, duracion_minutos FROM ConfiguracionHoraria"));
            QVERIFY(config.next());
            QCOMPARE(config.value(0).toInt(), 1);
            QCOMPARE(config.value(1).toInt(), 35);
            QVERIFY(!config.next());
        }

        // Tipos de aula (RF-5/RF-10): catálogo de 3 y todas las aulas y materias con un tipo
        // existente (nunca NULL ni huérfano).
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 3);
        for (const char* tabla : {"Aulas", "Materias"}) {
            const QString nombre = QString::fromLatin1(tabla);
            QSqlQuery conTipo(db);
            QVERIFY2(conTipo.exec(QStringLiteral("SELECT COUNT(*) FROM %1 x "
                                                 "JOIN TiposAula t ON t.id = x.id_tipo_aula")
                                      .arg(nombre)),
                     qPrintable(conTipo.lastError().text()));
            QVERIFY(conTipo.next());
            QCOMPARE(conTipo.value(0).toInt(), contar(db, nombre));
        }

        // Eventos exactos (RF-1): un «Receso» por turno, posición 2, orden 1, 10 min.
        QCOMPARE(contar(db, QStringLiteral("Eventos")), 2);
        QSqlQuery eventos(db);
        QVERIFY(eventos.exec(
            "SELECT turno, nombre, despues_de_slot, orden, duracion, inicio, fin "
            "FROM Eventos ORDER BY turno"));
        QVector<QStringList> filas;
        while (eventos.next()) {
            QStringList fila;
            for (int i = 0; i < 7; ++i) {
                fila.append(eventos.value(i).toString());
            }
            filas.append(fila);
        }
        QCOMPARE(filas.size(), 2);
        const QStringList eventoManana{QStringLiteral("Mañana"), QStringLiteral("Receso"),
                                       QStringLiteral("2"),      QStringLiteral("1"),
                                       QStringLiteral("10"),     QStringLiteral("08:10"),
                                       QStringLiteral("08:20")};
        const QStringList eventoTarde{QStringLiteral("Tarde"), QStringLiteral("Receso"),
                                      QStringLiteral("2"),     QStringLiteral("1"),
                                      QStringLiteral("10"),    QStringLiteral("13:50"),
                                      QStringLiteral("14:00")};
        QCOMPARE(filas[0], eventoManana);
        QCOMPARE(filas[1], eventoTarde);

        // 13 cursos con turno NULL y un aula distinta cada uno (RF-4, RF-5).
        QSqlQuery cursos(db);
        QVERIFY(cursos.exec("SELECT anio, seccion, turno, aula FROM Cursos"));
        int cursosTotal = 0;
        int cursosSinTurno = 0;
        QSet<int> aulas;
        while (cursos.next()) {
            ++cursosTotal;
            if (cursos.value("turno").isNull()) {
                ++cursosSinTurno;
            }
            const QVariant aula = cursos.value("aula");
            QVERIFY(!aula.isNull());
            aulas.insert(aula.toInt());
        }
        QCOMPARE(cursosTotal, 13);
        QCOMPARE(cursosSinTurno, 13);
        QCOMPARE(aulas.size(), 13);

        cerrarInspeccion(db);
    }

    void baseConDatosFallaSinEscribir()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;

        const ResultadoAplicacion primera = aplicar(opciones);
        QVERIFY2(primera.ok(), qPrintable(primera.detalle));

        // Segunda aplicación sobre la base ya poblada: falla sin escribir (RF-10).
        const ResultadoAplicacion segunda = aplicar(opciones);
        QCOMPARE(segunda.estado, EstadoAplicacion::BaseConDatos);
        QVERIFY(!segunda.ok());
        QVERIFY(!segunda.detalle.isEmpty());

        // Los datos quedan intactos (ni duplicados ni borrados).
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("Turnos")), 2);
        QCOMPARE(contar(db, QStringLiteral("Eventos")), 2);
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 3);
        QCOMPARE(contar(db, QStringLiteral("Aulas")), 16);
        QCOMPARE(contar(db, QStringLiteral("Materias")), 57);
        QCOMPARE(contar(db, QStringLiteral("PlanEstudio_Materia")), 210);
        QCOMPARE(contar(db, QStringLiteral("Cursos")), 13);
        QCOMPARE(contar(db, QStringLiteral("Profesores")), 65);
        QCOMPARE(contar(db, QStringLiteral("Profesor_Materia")), 65);
        QCOMPARE(contar(db, QStringLiteral("Disponibilidad_Profesor")), 325);
        cerrarInspeccion(db);
    }

    void rutaVaciaEsUsoInvalido()
    {
        // Sin «--db» no se actúa (RF-10): la base de destino es obligatoria.
        OpcionesAplicacion opciones;  // rutaDb vacía
        const ResultadoAplicacion resultado = aplicar(opciones);
        QCOMPARE(resultado.estado, EstadoAplicacion::UsoInvalido);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
    }

    void varianteDesconocidaEsRechazada()
    {
        // Una variante fuera del enum se rechaza sin tocar la base (RF-9). En la CLI la
        // detecta el parseo (`varianteDesdeTexto`); aquí se ejercita la guarda de `aplicar()`.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        OpcionesAplicacion opciones;
        opciones.rutaDb = dir.filePath(QStringLiteral("centro.db"));
        opciones.variante = static_cast<VarianteDocentes>(999);  // fuera de las cuatro válidas

        const ResultadoAplicacion resultado = aplicar(opciones);
        QCOMPARE(resultado.estado, EstadoAplicacion::VarianteDesconocida);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
    }

    void determinismo()
    {
        QTemporaryDir dirA;
        QTemporaryDir dirB;
        QVERIFY(dirA.isValid());
        QVERIFY(dirB.isValid());
        const QString rutaA = dirA.filePath(QStringLiteral("centro.db"));
        const QString rutaB = dirB.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = rutaA;
        const ResultadoAplicacion primera = aplicar(opciones);
        QVERIFY2(primera.ok(), qPrintable(primera.detalle));
        opciones.rutaDb = rutaB;
        const ResultadoAplicacion segunda = aplicar(opciones);
        QVERIFY2(segunda.ok(), qPrintable(segunda.detalle));

        QSqlDatabase a = abrirInspeccion(rutaA, m_contador);
        QSqlDatabase b = abrirInspeccion(rutaB, m_contador);
        QVERIFY(a.isOpen());
        QVERIFY(b.isOpen());

        // Dos aplicaciones del mismo perfil dejan las mismas filas (nombres, ids, relaciones y
        // las 210 cargas) salvo id/fecha_creacion/fecha_modificacion (RF-11, RNF-5).
        const struct {
            const char* tabla;
            const char* sql;
        } kComparaciones[] = {
            {"Turnos", "SELECT nombre, inicio, fin, slots FROM Turnos"},
            {"ConfiguracionHoraria", "SELECT id, duracion_minutos FROM ConfiguracionHoraria"},
            {"Preferencia_Profesor",
             "SELECT id_Profesor, dia, hora_inicio, hora_fin, sentido FROM Preferencia_Profesor"},
            {"TiposAula", "SELECT nombre, clave FROM TiposAula"},
            {"Eventos",
             "SELECT turno, nombre, despues_de_slot, orden, duracion, inicio, fin FROM Eventos"},
            {"Aulas",
             "SELECT a.nombre, a.capacidad, a.edificio, a.piso, t.nombre "
             "FROM Aulas a JOIN TiposAula t ON t.id = a.id_tipo_aula"},
            {"Materias",
             "SELECT m.nombre, m.requisitos, t.nombre "
             "FROM Materias m JOIN TiposAula t ON t.id = m.id_tipo_aula"},
            {"PlanEstudio", "SELECT codigo, nombre, descripcion FROM PlanEstudio"},
            {"PlanEstudio_Materia",
             "SELECT codigo_PlanEstudio, id_Materia, curso, horas FROM PlanEstudio_Materia"},
            {"Cursos", "SELECT anio, seccion, turno, aula FROM Cursos"},
            {"Profesores", "SELECT id, nombre, email, telefono FROM Profesores"},
            {"Profesor_Materia", "SELECT id_Profesor, id_Materia FROM Profesor_Materia"},
            {"Disponibilidad_Profesor",
             "SELECT id_Profesor, dia, hora_inicio, hora_fin FROM Disponibilidad_Profesor"},
        };
        for (const auto& c : kComparaciones) {
            const QString sql = QString::fromLatin1(c.sql);
            const QStringList filasA = filasDe(a, sql);
            const QStringList filasB = filasDe(b, sql);
            QVERIFY2(filasA == filasB,
                     qPrintable(QStringLiteral("La tabla %1 difiere entre bases.")
                                    .arg(QString::fromLatin1(c.tabla))));
        }

        cerrarInspeccion(a);
        cerrarInspeccion(b);
    }

    void rollbackAnteFalloTrasDocentes()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        opciones.simularFalloTrasDocentes = true;

        const ResultadoAplicacion resultado = aplicar(opciones);
        QCOMPARE(resultado.estado, EstadoAplicacion::FalloEscritura);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());

        // Nada a medias (RF-10/RNF-6): la base nueva queda con 0 filas en las 12 tablas de
        // dominio del esquema v5.
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        const char* const kTablas[] = {
            "TiposAula", "Aulas", "Profesores", "Materias", "PlanEstudio", "PlanEstudio_Materia",
            "Turnos", "Eventos", "Cursos", "Profesor_Materia", "Disponibilidad_Profesor",
            "Preferencia_Profesor",
        };
        for (const char* tabla : kTablas) {
            QCOMPARE(contar(db, QString::fromLatin1(tabla)), 0);
        }
        // La fila de configuración la deja la creación del esquema (fuera de la transacción)
        // y el rollback no la duplica: sigue habiendo una con 35 (RF-2, RF-8).
        QCOMPARE(contar(db, QStringLiteral("ConfiguracionHoraria")), 1);
        {
            QSqlQuery config(db);
            QVERIFY(config.exec("SELECT duracion_minutos FROM ConfiguracionHoraria"));
            QVERIFY(config.next());
            QCOMPARE(config.value(0).toInt(), 35);
        }
        cerrarInspeccion(db);
    }

    void rutaRespaldoLibre_eligeRutaDisponible()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath(QStringLiteral("centro.db"));
        const QDateTime ahora(QDate(2026, 10, 17), QTime(12, 0, 0));

        // Sin colisiones: la ruta base (con la marca temporal) queda libre.
        const QString base = Respaldo::construirNombreRespaldo(rutaDb, ahora);
        QCOMPARE(rutaRespaldoLibre(rutaDb, ahora), base);

        // Si la base está ocupada, se prueba «-2», «-3»… (función pura, sin tocar la base).
        const QString libre2 = base + QStringLiteral("-2");
        const QString libre3 = base + QStringLiteral("-3");
        {
            QFile a(base);
            QVERIFY(a.open(QIODevice::WriteOnly));
            a.write("ocupado");
            a.close();
        }
        QCOMPARE(rutaRespaldoLibre(rutaDb, ahora), libre2);
        {
            QFile b(libre2);
            QVERIFY(b.open(QIODevice::WriteOnly));
            b.write("ocupado");
            b.close();
        }
        QCOMPARE(rutaRespaldoLibre(rutaDb, ahora), libre3);
    }

    void reemplazoTotalConRespaldo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion inicial = aplicar(opciones);
        QVERIFY2(inicial.ok(), qPrintable(inicial.detalle));

        // Reemplazo total con confirmación: deja respaldo previo válido y repuebla.
        OpcionesAplicacion reemplazo;
        reemplazo.rutaDb = ruta;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = true;
        reemplazo.ahora = QDateTime(QDate(2026, 10, 17), QTime(12, 0, 0));

        const ResultadoAplicacion resultado = aplicar(reemplazo);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);

        // El respaldo existe y es una base válida (RF-10/RNF-6).
        QVERIFY(!resultado.rutaRespaldo.isEmpty());
        QVERIFY(QFile::exists(resultado.rutaRespaldo));
        QString errorRespaldo;
        QVERIFY2(Respaldo::validarRespaldo(resultado.rutaRespaldo, &errorRespaldo),
                 qPrintable(errorRespaldo));

        // Recuentos correctos tras el reemplazo (misma variante holgada por defecto).
        QCOMPARE(resultado.recuentos.docentes, 65);
        QCOMPARE(resultado.recuentos.asignaciones, 65);
        QCOMPARE(resultado.recuentos.disponibilidades, 325);
        QCOMPARE(resultado.recuentos.planMaterias, 210);
        QCOMPARE(resultado.recuentos.eventos, 2);
        QCOMPARE(resultado.recuentos.tiposAula, 3);

        // El reemplazo no duplica ni borra la configuración: sigue habiendo una fila con 35.
        QCOMPARE(resultado.duracionMinutos, 35);
        QCOMPARE(resultado.configuraciones, 1);
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("ConfiguracionHoraria")), 1);
        QSqlQuery config(db);
        QVERIFY(config.exec("SELECT duracion_minutos FROM ConfiguracionHoraria"));
        QVERIFY(config.next());
        QCOMPARE(config.value(0).toInt(), 35);
        cerrarInspeccion(db);
    }

    void reemplazoTotalConCursosAsignados()
    {
        // El vaciado del reemplazo va en orden FK-seguro: los cursos pueden apuntar a un
        // turno y a un aula (RF-10), de modo que Cursos debe borrarse antes que Turnos y
        // Aulas, y Eventos antes que Turnos, o SQLite rechaza el DELETE por la FK.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion inicial = aplicar(opciones);
        QVERIFY2(inicial.ok(), qPrintable(inicial.detalle));

        // Asignación directa de turno y aula a todos los cursos, como haría la interfaz.
        {
            QSqlDatabase asignacion = abrirInspeccion(ruta, m_contador);
            QVERIFY(asignacion.isOpen());
            QSqlQuery query(asignacion);
            QVERIFY2(query.exec("UPDATE Cursos SET turno = 'Mañana', "
                                "aula = (SELECT MIN(id) FROM Aulas)"),
                     qPrintable(query.lastError().text()));
            QCOMPARE(contar(asignacion, QStringLiteral("Cursos")), 13);
            cerrarInspeccion(asignacion);
        }

        OpcionesAplicacion reemplazo;
        reemplazo.rutaDb = ruta;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = true;
        reemplazo.ahora = QDateTime(QDate(2026, 10, 17), QTime(12, 0, 0));

        const ResultadoAplicacion resultado = aplicar(reemplazo);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);
        QCOMPARE(resultado.recuentos.cursos, 13);
        QCOMPARE(resultado.recuentos.eventos, 2);
        QCOMPARE(resultado.recuentos.tiposAula, 3);

        // La base queda repoblada como una siembra nueva: cursos sin turno, eventos y tipos.
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("Cursos")), 13);
        QCOMPARE(contar(db, QStringLiteral("Eventos")), 2);
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 3);
        QSqlQuery cursos(db);
        QVERIFY(cursos.exec("SELECT COUNT(*) FROM Cursos WHERE turno IS NULL"));
        QVERIFY(cursos.next());
        QCOMPARE(cursos.value(0).toInt(), 13);
        cerrarInspeccion(db);
    }

    void reemplazoTotalSinConfirmacion()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion inicial = aplicar(opciones);
        QVERIFY2(inicial.ok(), qPrintable(inicial.detalle));

        OpcionesAplicacion reemplazo;
        reemplazo.rutaDb = ruta;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = false; // sin --si
        reemplazo.ahora = QDateTime(QDate(2026, 10, 17), QTime(12, 0, 0));

        const ResultadoAplicacion resultado = aplicar(reemplazo);
        QCOMPARE(resultado.estado, EstadoAplicacion::ConfirmacionRequerida);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
        // Sin respaldo ni cambios: la base conserva los datos previos.
        QVERIFY(resultado.rutaRespaldo.isEmpty());
        QVERIFY(!QFile::exists(Respaldo::construirNombreRespaldo(
            ruta, QDateTime(QDate(2026, 10, 17), QTime(12, 0, 0)))));

        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("Profesores")), 65);
        QCOMPARE(contar(db, QStringLiteral("PlanEstudio_Materia")), 210);
        cerrarInspeccion(db);
    }

    void soloDocentesConservaElResto()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion inicial = aplicar(opciones);
        QVERIFY2(inicial.ok(), qPrintable(inicial.detalle));

        // Fotografía del estado previo de todo lo que no son docentes.
        QSqlDatabase antes = abrirInspeccion(ruta, m_contador);
        QVERIFY(antes.isOpen());
        const struct {
            const char* tabla;
            const char* sql;
        } kNoDocentes[] = {
            {"Turnos", "SELECT nombre, inicio, fin, slots FROM Turnos"},
            {"TiposAula", "SELECT nombre, clave FROM TiposAula"},
            {"Eventos",
             "SELECT turno, nombre, despues_de_slot, orden, duracion, inicio, fin FROM Eventos"},
            {"Aulas",
             "SELECT a.nombre, a.capacidad, a.edificio, a.piso, t.nombre "
             "FROM Aulas a JOIN TiposAula t ON t.id = a.id_tipo_aula"},
            {"Materias",
             "SELECT m.nombre, m.requisitos, t.nombre "
             "FROM Materias m JOIN TiposAula t ON t.id = m.id_tipo_aula"},
            {"PlanEstudio", "SELECT codigo, nombre, descripcion FROM PlanEstudio"},
            {"PlanEstudio_Materia",
             "SELECT codigo_PlanEstudio, id_Materia, curso, horas FROM PlanEstudio_Materia"},
            {"Cursos", "SELECT anio, seccion, turno, aula FROM Cursos"},
        };
        QStringList antesFilas[8];
        for (int i = 0; i < 8; ++i) {
            antesFilas[i] = filasDe(antes, QString::fromLatin1(kNoDocentes[i].sql));
        }
        QCOMPARE(contar(antes, QStringLiteral("Profesores")), 65);
        cerrarInspeccion(antes);

        // Conmutación a la variante de suplentes: solo docentes/materias/disponibilidad.
        OpcionesAplicacion conmutacion;
        conmutacion.rutaDb = ruta;
        conmutacion.soloDocentes = true;
        conmutacion.confirmado = true;
        conmutacion.variante = VarianteDocentes::Suplentes;
        conmutacion.ahora = QDateTime(QDate(2026, 10, 17), QTime(12, 0, 0));

        const ResultadoAplicacion resultado = aplicar(conmutacion);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);
        QCOMPARE(resultado.recuentos.docentes, 43);
        QCOMPARE(resultado.recuentos.asignaciones, 57);
        QCOMPARE(resultado.recuentos.disponibilidades, 215);

        // La conmutación también asegura la configuración: una fila con 35, sin duplicar.
        QCOMPARE(resultado.duracionMinutos, 35);
        QCOMPARE(resultado.configuraciones, 1);

        QSqlDatabase despues = abrirInspeccion(ruta, m_contador);
        QVERIFY(despues.isOpen());
        // El resto de tablas queda idéntico (incluidas las 210 cargas, los tipos y los eventos).
        for (int i = 0; i < 8; ++i) {
            const QStringList filas = filasDe(despues, QString::fromLatin1(kNoDocentes[i].sql));
            QVERIFY2(filas == antesFilas[i],
                     qPrintable(QStringLiteral("La tabla %1 cambió durante «--solo-docentes».")
                                    .arg(QString::fromLatin1(kNoDocentes[i].tabla))));
        }
        QCOMPARE(contar(despues, QStringLiteral("PlanEstudio_Materia")), 210);
        QCOMPARE(contar(despues, QStringLiteral("Profesores")), 43);
        QCOMPARE(contar(despues, QStringLiteral("ConfiguracionHoraria")), 1);
        {
            QSqlQuery config(despues);
            QVERIFY(config.exec("SELECT duracion_minutos FROM ConfiguracionHoraria"));
            QVERIFY(config.next());
            QCOMPARE(config.value(0).toInt(), 35);
        }
        cerrarInspeccion(despues);
    }

    void baseRecienCreadaSoloConConfiguracionSeSiembraSinReemplazar()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        // Una base recién creada solo tiene el esquema y su fila de configuración: la
        // configuración no es dato de dominio y no bloquea la siembra (RF-8).
        QCOMPARE(AperturaBaseDatos::abrir(ruta).estado, AperturaBaseDatos::Estado::OkCreada);

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion resultado = aplicar(opciones);

        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);
        QCOMPARE(resultado.duracionMinutos, 35);
        QCOMPARE(resultado.configuraciones, 1);
        QCOMPARE(resultado.recuentos.preferencias, 0);
        QCOMPARE(resultado.recuentos.turnos, 2);
    }

    void resumenTexto_incluyeLaHoraAcademicaYSuRecuento()
    {
        const QString resumen = resumenTexto(perfilPorDefecto(), VarianteDocentes::Holgada,
                                             Recuentos{}, 35, 1);

        QVERIFY2(resumen.contains(QStringLiteral("35 min")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("configuraciones: 1")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("Preferencias: 0")), qPrintable(resumen));
    }

    void atomicidadAnteFalloEnReemplazo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion inicial = aplicar(opciones);
        QVERIFY2(inicial.ok(), qPrintable(inicial.detalle));

        QSqlDatabase antes = abrirInspeccion(ruta, m_contador);
        QVERIFY(antes.isOpen());
        const QStringList planMateriasAntes =
            filasDe(antes, "SELECT codigo_PlanEstudio, id_Materia, curso, horas "
                           "FROM PlanEstudio_Materia");
        const QStringList profesoresAntes =
            filasDe(antes, "SELECT id, nombre, email, telefono FROM Profesores");
        QCOMPARE(contar(antes, QStringLiteral("Profesores")), 65);
        QCOMPARE(contar(antes, QStringLiteral("PlanEstudio_Materia")), 210);
        cerrarInspeccion(antes);

        // Reemplazo total que falla a mitad: la base previa queda intacta (rollback).
        OpcionesAplicacion reemplazo;
        reemplazo.rutaDb = ruta;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = true;
        reemplazo.variante = VarianteDocentes::Suplentes;
        reemplazo.simularFalloTrasDocentes = true;
        reemplazo.ahora = QDateTime(QDate(2026, 10, 17), QTime(12, 0, 0));

        const ResultadoAplicacion resultado = aplicar(reemplazo);
        QCOMPARE(resultado.estado, EstadoAplicacion::FalloEscritura);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());

        QSqlDatabase despues = abrirInspeccion(ruta, m_contador);
        QVERIFY(despues.isOpen());
        QCOMPARE(contar(despues, QStringLiteral("Profesores")), 65);
        QCOMPARE(contar(despues, QStringLiteral("PlanEstudio_Materia")), 210);
        const QStringList planMateriasDespues =
            filasDe(despues, "SELECT codigo_PlanEstudio, id_Materia, curso, horas "
                             "FROM PlanEstudio_Materia");
        const QStringList profesoresDespues =
            filasDe(despues, "SELECT id, nombre, email, telefono FROM Profesores");
        QVERIFY2(planMateriasDespues == planMateriasAntes,
                 "Las 210 cargas cambiaron tras el fallo en reemplazo.");
        QVERIFY2(profesoresDespues == profesoresAntes,
                 "Los docentes cambiaron tras el fallo en reemplazo.");
        cerrarInspeccion(despues);
    }

    void tiempoDeAplicacion()
    {
        // RNF-1: aplicar el perfil por defecto sobre una base vacía termina en menos de 15 s.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;

        QElapsedTimer cronometro;
        cronometro.start();
        const ResultadoAplicacion resultado = aplicar(opciones);
        const qint64 transcurrido = cronometro.elapsed();

        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QVERIFY2(transcurrido < 15000,
                 qPrintable(QStringLiteral("La aplicación sobre una base nueva tardó %1 ms "
                                           "(límite: 15000 ms).")
                                .arg(transcurrido)));
    }
};

QTEST_MAIN(TestSeedAplicacion)
#include "test_seed_aplicacion.moc"
