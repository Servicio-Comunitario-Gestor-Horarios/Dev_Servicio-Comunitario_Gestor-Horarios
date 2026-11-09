#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QPair>
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
#include <seed/docentes.hpp>
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

/// Id de la fila del tipo general que deja el esquema en una base nueva (015 RF-1).
int idTipoGeneral(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id FROM TiposAula WHERE clave = 'aula de estudio'"));
    if (!q.exec() || !q.next()) {
        return -1;
    }
    return q.value(0).toInt();
}

/// Número de filas del catálogo cuya clave es la del tipo general.
int contarTipoGeneral(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM TiposAula WHERE clave = 'aula de estudio'"));
    if (!q.exec() || !q.next()) {
        return -1;
    }
    return q.value(0).toInt();
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

    /// Inserta un docente y una preferencia directamente (la siembra no las crea) para
    /// comprobar que el reemplazo total respeta la FK y deja la tabla vacía.
    void preinsertarPreferencia(const QString& ruta)
    {
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());

        QSqlQuery docente(db);
        QVERIFY2(docente.exec("INSERT INTO Profesores (id, nombre, email) "
                              "VALUES ('P-PREF', 'Docente Pref', 'pref@centro.edu')"),
                 qPrintable(docente.lastError().text()));

        QSqlQuery preferencia(db);
        QVERIFY2(preferencia.exec(
                     "INSERT INTO Preferencia_Profesor "
                     "(id_Profesor, dia, hora_inicio, hora_fin, sentido) "
                     "VALUES ('P-PREF', '1', '08:00', '10:00', 'preferida')"),
                 qPrintable(preferencia.lastError().text()));

        cerrarInspeccion(db);
    }

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

        // Recuentos del conjunto (plan §4.8): 85/95/24/13/3/2 y 32 docentes con 272
        // asignaciones y 160 disponibilidades (32 × 5).
        QCOMPARE(resultado.recuentos.turnos, 2);
        QCOMPARE(resultado.recuentos.eventos, 3);
        QCOMPARE(resultado.recuentos.tiposAula, 2);
        QCOMPARE(resultado.recuentos.aulas, 13);
        QCOMPARE(resultado.recuentos.materias, 85);
        QCOMPARE(resultado.recuentos.planes, 2);
        QCOMPARE(resultado.recuentos.planMaterias, 95);
        QCOMPARE(resultado.recuentos.cursos, 24);
        QCOMPARE(resultado.recuentos.docentes, 32);
        QCOMPARE(resultado.recuentos.asignaciones, 272);
        QCOMPARE(resultado.recuentos.disponibilidades, 160);

        // El conjunto no siembra preferencias (spec 012): recuento 0 y tabla vacía.
        QCOMPARE(resultado.recuentos.preferencias, 0);

        // La configuración global queda en 40 min con una sola fila (RF-7).
        QCOMPARE(resultado.recuentos.duracionMinutos, 40);
        QCOMPARE(resultado.recuentos.configuraciones, 1);

        // Inspección directa de la base.
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());

        // Turnos (RF-7): 8 franjas de 40 min y ventanas 07:00–12:40 / 12:40–18:10.
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
            QCOMPARE(filasTurnos[0], QStringList({"Mañana", "07:00", "12:40", "8"}));
            QCOMPARE(filasTurnos[1], QStringList({"Tarde", "12:40", "18:10", "8"}));
        }

        // Configuración global (RF-7): una sola fila `(1, 40)`, idempotente.
        QCOMPARE(contar(db, QStringLiteral("ConfiguracionHoraria")), 1);
        QCOMPARE(contar(db, QStringLiteral("Preferencia_Profesor")), 0);
        {
            QSqlQuery config(db);
            QVERIFY(config.exec("SELECT id, duracion_minutos FROM ConfiguracionHoraria"));
            QVERIFY(config.next());
            QCOMPARE(config.value(0).toInt(), 1);
            QCOMPARE(config.value(1).toInt(), 40);
            QVERIFY(!config.next());
        }

        // Tipos de aula (RF-3/RF-5): catálogo de 2 (general + cancha) y todas las aulas y
        // materias con un tipo existente (nunca NULL ni huérfano).
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 2);
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
        {
            const QStringList catalogo =
                filasDe(db, QStringLiteral("SELECT nombre, clave FROM TiposAula"));
            QVERIFY(catalogo.contains(QStringLiteral("Aula de estudio|aula de estudio")));
            QVERIFY(catalogo.contains(QStringLiteral("Cancha|cancha")));
        }

        // Eventos exactos (RF-7): receso y acto cívico en la mañana y receso en la tarde.
        QCOMPARE(contar(db, QStringLiteral("Eventos")), 3);
        QSqlQuery eventos(db);
        QVERIFY(eventos.exec(
            "SELECT turno, nombre, despues_de_slot, orden, duracion, inicio, fin "
            "FROM Eventos ORDER BY turno, despues_de_slot, orden"));
        QVector<QStringList> filas;
        while (eventos.next()) {
            QStringList fila;
            for (int i = 0; i < 7; ++i) {
                fila.append(eventos.value(i).toString());
            }
            filas.append(fila);
        }
        QCOMPARE(filas.size(), 3);
        QCOMPARE(filas[0], QStringList({"Mañana", "Receso", "2", "1", "10", "08:20", "08:30"}));
        QCOMPARE(filas[1], QStringList({"Mañana", "Acto cívico", "2", "2", "10", "08:30", "08:40"}));
        QCOMPARE(filas[2], QStringList({"Tarde", "Receso", "2", "1", "10", "14:00", "14:10"}));

        // 24 cursos con turno (12 y 12) y aula de referencia de uso general; las 12 aulas
        // se comparten entre turnos.
        QSqlQuery cursos(db);
        QVERIFY(cursos.exec("SELECT anio, seccion, turno, aula FROM Cursos"));
        int cursosTotal = 0;
        int manana = 0;
        int tarde = 0;
        int sinTurno = 0;
        QSet<int> aulasManana;
        QSet<int> aulasTarde;
        QSet<int> aulasTotales;
        while (cursos.next()) {
            ++cursosTotal;
            const QString turno = cursos.value("turno").toString();
            if (turno.isEmpty()) {
                ++sinTurno;
            } else if (turno == QStringLiteral("Mañana")) {
                ++manana;
                aulasManana.insert(cursos.value("aula").toInt());
            } else {
                ++tarde;
                aulasTarde.insert(cursos.value("aula").toInt());
            }
            const QVariant aula = cursos.value("aula");
            QVERIFY(!aula.isNull());
            aulasTotales.insert(aula.toInt());
        }
        QCOMPARE(cursosTotal, 24);
        QCOMPARE(sinTurno, 0);
        QCOMPARE(manana, 12);
        QCOMPARE(tarde, 12);
        QCOMPARE(aulasManana.size(), 12);
        QCOMPARE(aulasTarde.size(), 12);
        QCOMPARE(aulasTotales.size(), 12);

        cerrarInspeccion(db);
    }

    void aplicar_sobreBaseNueva_noInformaBaseConDatos()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        // La base recién creada solo trae esquema, configuración y la fila del general: no es
        // «base con datos» (015 RF-5).
        QCOMPARE(AperturaBaseDatos::abrir(ruta).estado, AperturaBaseDatos::Estado::OkCreada);

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion resultado = aplicar(opciones);

        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);
        QCOMPARE(resultado.recuentos.tiposAula, 2);
    }

    void aplicar_reutilizaElTipoGeneral()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        QCOMPARE(AperturaBaseDatos::abrir(ruta).estado, AperturaBaseDatos::Estado::OkCreada);

        int idAntes = -1;
        {
            QSqlDatabase db = abrirInspeccion(ruta, m_contador);
            QVERIFY(db.isOpen());
            idAntes = idTipoGeneral(db);
            cerrarInspeccion(db);
        }
        QVERIFY(idAntes > 0);

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion resultado = aplicar(opciones);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));

        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());

        // Misma fila, mismo id: la siembra no crea un segundo tipo general.
        QCOMPARE(idTipoGeneral(db), idAntes);
        QCOMPARE(contarTipoGeneral(db), 1);
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 2); // general + Cancha

        const QStringList catalogo =
            filasDe(db, QStringLiteral("SELECT nombre, clave FROM TiposAula"));
        QCOMPARE(catalogo.size(), 2);
        QVERIFY(catalogo.contains(QStringLiteral("Cancha|cancha")));

        cerrarInspeccion(db);
    }

    void reemplazar_conservaElTipoGeneral()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        QVERIFY2(aplicar(opciones).ok(), "siembra inicial");

        int idAntes = -1;
        {
            QSqlDatabase db = abrirInspeccion(ruta, m_contador);
            QVERIFY(db.isOpen());
            idAntes = idTipoGeneral(db);
            cerrarInspeccion(db);
        }
        QVERIFY(idAntes > 0);

        OpcionesAplicacion reemplazo;
        reemplazo.rutaDb = ruta;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = true;
        reemplazo.ahora = QDateTime(QDate(2026, 11, 5), QTime(12, 0, 0));
        const ResultadoAplicacion resultado = aplicar(reemplazo);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.recuentos.tiposAula, 2);

        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());

        // El reemplazo conserva la identidad (id) del general y no lo duplica.
        QCOMPARE(idTipoGeneral(db), idAntes);
        QCOMPARE(contarTipoGeneral(db), 1);
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 2);

        cerrarInspeccion(db);
    }

    void aplicar_dosVeces_mismosDatosYRecuentos()
    {
        // Comparaciones por clave natural: no dependen de ids ni de fechas.
        const struct {
            const char* consulta;
            const char* sql;
        } kComparaciones[] = {
            {"TiposAula", "SELECT nombre, clave FROM TiposAula"},
            {"Turnos", "SELECT nombre, inicio, fin, slots FROM Turnos"},
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
             "SELECT p.codigo_PlanEstudio, m.nombre, p.curso, p.horas "
             "FROM PlanEstudio_Materia p JOIN Materias m ON m.id = p.id_Materia"},
            {"Cursos",
             "SELECT c.anio, c.seccion, c.turno, a.nombre "
             "FROM Cursos c LEFT JOIN Aulas a ON a.id = c.aula"},
            {"Profesores", "SELECT id, nombre, email, telefono FROM Profesores"},
            {"Profesor_Materia",
             "SELECT pm.id_Profesor, m.nombre "
             "FROM Profesor_Materia pm JOIN Materias m ON m.id = pm.id_Materia"},
            {"Disponibilidad_Profesor",
             "SELECT id_Profesor, dia, hora_inicio, hora_fin FROM Disponibilidad_Profesor"},
            {"Preferencia_Profesor",
             "SELECT id_Profesor, dia, hora_inicio, hora_fin, sentido FROM Preferencia_Profesor"},
        };
        const auto exigirMismosDatos = [&](QSqlDatabase& a, QSqlDatabase& b) {
            for (const auto& c : kComparaciones) {
                QVERIFY2(filasDe(a, QString::fromLatin1(c.sql))
                             == filasDe(b, QString::fromLatin1(c.sql)),
                         qPrintable(QStringLiteral("La tabla %1 difiere entre bases.")
                                        .arg(QString::fromLatin1(c.consulta))));
            }
        };
        const auto exigirMismosRecuentos = [](const Recuentos& a, const Recuentos& b) {
            QCOMPARE(a.turnos, b.turnos);
            QCOMPARE(a.eventos, b.eventos);
            QCOMPARE(a.tiposAula, b.tiposAula);
            QCOMPARE(a.aulas, b.aulas);
            QCOMPARE(a.materias, b.materias);
            QCOMPARE(a.planes, b.planes);
            QCOMPARE(a.planMaterias, b.planMaterias);
            QCOMPARE(a.cursos, b.cursos);
            QCOMPARE(a.docentes, b.docentes);
            QCOMPARE(a.asignaciones, b.asignaciones);
            QCOMPARE(a.disponibilidades, b.disponibilidades);
            QCOMPARE(a.preferencias, b.preferencias);
            QCOMPARE(a.duracionMinutos, b.duracionMinutos);
            QCOMPARE(a.configuraciones, b.configuraciones);
        };

        // 1) Dos bases vacías: mismos datos y recuentos.
        QTemporaryDir dirA;
        QTemporaryDir dirB;
        QVERIFY(dirA.isValid());
        QVERIFY(dirB.isValid());

        OpcionesAplicacion opcionesA;
        opcionesA.rutaDb = dirA.filePath(QStringLiteral("centro.db"));
        OpcionesAplicacion opcionesB;
        opcionesB.rutaDb = dirB.filePath(QStringLiteral("centro.db"));

        const ResultadoAplicacion primera = aplicar(opcionesA);
        const ResultadoAplicacion segunda = aplicar(opcionesB);
        QVERIFY2(primera.ok(), qPrintable(primera.detalle));
        QVERIFY2(segunda.ok(), qPrintable(segunda.detalle));
        exigirMismosRecuentos(primera.recuentos, segunda.recuentos);

        QSqlDatabase a = abrirInspeccion(opcionesA.rutaDb, m_contador);
        QSqlDatabase b = abrirInspeccion(opcionesB.rutaDb, m_contador);
        QVERIFY(a.isOpen());
        QVERIFY(b.isOpen());
        exigirMismosDatos(a, b);
        cerrarInspeccion(a);
        cerrarInspeccion(b);

        // 2) Reemplazo total sobre una base ya sembrada: mismos datos y recuentos que una
        //    siembra nueva.
        OpcionesAplicacion reemplazo = opcionesA;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = true;
        reemplazo.ahora = QDateTime(QDate(2026, 11, 5), QTime(12, 0, 0));
        const ResultadoAplicacion trasReemplazo = aplicar(reemplazo);
        QVERIFY2(trasReemplazo.ok(), qPrintable(trasReemplazo.detalle));
        exigirMismosRecuentos(trasReemplazo.recuentos, segunda.recuentos);

        QSqlDatabase aReemplazada = abrirInspeccion(opcionesA.rutaDb, m_contador);
        QSqlDatabase bNueva = abrirInspeccion(opcionesB.rutaDb, m_contador);
        QVERIFY(aReemplazada.isOpen());
        QVERIFY(bNueva.isOpen());
        exigirMismosDatos(aReemplazada, bNueva);
        cerrarInspeccion(aReemplazada);
        cerrarInspeccion(bNueva);
    }

    void aplicar_recuentoTiposEsDos()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion resultado = aplicar(opciones);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));

        QCOMPARE(resultado.recuentos.tiposAula, 2);

        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 2);
        QCOMPARE(contarTipoGeneral(db), 1);

        const QStringList catalogo =
            filasDe(db, QStringLiteral("SELECT nombre, clave FROM TiposAula"));
        QVERIFY(catalogo.contains(QStringLiteral("Aula de estudio|aula de estudio")));
        QVERIFY(catalogo.contains(QStringLiteral("Cancha|cancha")));

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

        // Segunda aplicación sobre la base ya poblada: falla sin escribir (RF-1).
        const ResultadoAplicacion segunda = aplicar(opciones);
        QCOMPARE(segunda.estado, EstadoAplicacion::BaseConDatos);
        QVERIFY(!segunda.ok());
        QVERIFY(!segunda.detalle.isEmpty());

        // Los datos quedan intactos (ni duplicados ni borrados).
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("Turnos")), 2);
        QCOMPARE(contar(db, QStringLiteral("Eventos")), 3);
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 2);
        QCOMPARE(contar(db, QStringLiteral("Aulas")), 13);
        QCOMPARE(contar(db, QStringLiteral("Materias")), 85);
        QCOMPARE(contar(db, QStringLiteral("PlanEstudio_Materia")), 95);
        QCOMPARE(contar(db, QStringLiteral("Cursos")), 24);
        QCOMPARE(contar(db, QStringLiteral("Profesores")), 32);
        QCOMPARE(contar(db, QStringLiteral("Profesor_Materia")), 272);
        QCOMPARE(contar(db, QStringLiteral("Disponibilidad_Profesor")), 160);
        cerrarInspeccion(db);
    }

    void rutaVaciaEsUsoInvalido()
    {
        // Sin «--db» no se actúa (RF-1): la base de destino es obligatoria.
        OpcionesAplicacion opciones;  // rutaDb vacía
        const ResultadoAplicacion resultado = aplicar(opciones);
        QCOMPARE(resultado.estado, EstadoAplicacion::UsoInvalido);
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
        // las 95 cargas) salvo id/fecha_creacion/fecha_modificacion (RNF-4).
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

        // Nada a medias (RF-1): la base nueva queda sin datos de dominio. La única fila que
        // sobrevive es el tipo general que deja la creación del esquema (015 RF-1).
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        const char* const kTablas[] = {
            "TiposAula", "Aulas", "Profesores", "Materias", "PlanEstudio", "PlanEstudio_Materia",
            "Turnos", "Eventos", "Cursos", "Profesor_Materia", "Disponibilidad_Profesor",
            "Preferencia_Profesor",
        };
        for (const char* tabla : kTablas) {
            const QString nombre = QString::fromLatin1(tabla);
            const int esperado = nombre == QStringLiteral("TiposAula") ? 1 : 0;
            QCOMPARE(contar(db, nombre), esperado);
        }
        // La fila de configuración la deja la creación del esquema (fuera de la transacción)
        // y el rollback la devuelve a 35: sigue habiendo una sola fila (RF-7).
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

        // El respaldo existe y es una base válida (RF-1).
        QVERIFY(!resultado.rutaRespaldo.isEmpty());
        QVERIFY(QFile::exists(resultado.rutaRespaldo));
        QString errorRespaldo;
        QVERIFY2(Respaldo::validarRespaldo(resultado.rutaRespaldo, &errorRespaldo),
                 qPrintable(errorRespaldo));

        // Recuentos correctos tras el reemplazo (plan §4.8).
        QCOMPARE(resultado.recuentos.docentes, 32);
        QCOMPARE(resultado.recuentos.asignaciones, 272);
        QCOMPARE(resultado.recuentos.disponibilidades, 160);
        QCOMPARE(resultado.recuentos.planMaterias, 95);
        QCOMPARE(resultado.recuentos.eventos, 3);
        QCOMPARE(resultado.recuentos.tiposAula, 2);

        // El reemplazo no duplica ni borra la configuración: sigue habiendo una fila con 40.
        QCOMPARE(resultado.recuentos.duracionMinutos, 40);
        QCOMPARE(resultado.recuentos.configuraciones, 1);
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("ConfiguracionHoraria")), 1);
        QSqlQuery config(db);
        QVERIFY(config.exec("SELECT duracion_minutos FROM ConfiguracionHoraria"));
        QVERIFY(config.next());
        QCOMPARE(config.value(0).toInt(), 40);
        cerrarInspeccion(db);
    }

    void reemplazoTotalConCursosAsignados()
    {
        // El vaciado del reemplazo va en orden FK-seguro: los cursos apuntan a un turno y a un
        // aula (RF-4), de modo que Cursos debe borrarse antes que Turnos y Aulas, y Eventos
        // antes que Turnos, o SQLite rechaza el DELETE por la FK.
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
            QCOMPARE(contar(asignacion, QStringLiteral("Cursos")), 24);
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
        QCOMPARE(resultado.recuentos.cursos, 24);
        QCOMPARE(resultado.recuentos.eventos, 3);
        QCOMPARE(resultado.recuentos.tiposAula, 2);

        // La base queda repoblada como una siembra nueva: 24 cursos con turno, 12 y 12.
        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("Cursos")), 24);
        QCOMPARE(contar(db, QStringLiteral("Eventos")), 3);
        QCOMPARE(contar(db, QStringLiteral("TiposAula")), 2);
        QSqlQuery cursos(db);
        QVERIFY(cursos.exec("SELECT turno, COUNT(*) FROM Cursos GROUP BY turno ORDER BY turno"));
        QVector<QPair<QString, int>> porTurno;
        while (cursos.next()) {
            porTurno.append({cursos.value(0).toString(), cursos.value(1).toInt()});
        }
        QCOMPARE(porTurno.size(), 2);
        QCOMPARE(porTurno[0].first, QStringLiteral("Mañana"));
        QCOMPARE(porTurno[0].second, 12);
        QCOMPARE(porTurno[1].first, QStringLiteral("Tarde"));
        QCOMPARE(porTurno[1].second, 12);
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
        QCOMPARE(contar(db, QStringLiteral("Profesores")), 32);
        QCOMPARE(contar(db, QStringLiteral("PlanEstudio_Materia")), 95);
        cerrarInspeccion(db);
    }

    void baseRecienCreadaSoloConConfiguracionSeSiembraSinReemplazar()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        // Una base recién creada solo tiene el esquema y su fila de configuración: la
        // configuración no es dato de dominio y no bloquea la siembra (RF-7).
        QCOMPARE(AperturaBaseDatos::abrir(ruta).estado, AperturaBaseDatos::Estado::OkCreada);

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion resultado = aplicar(opciones);

        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);
        QCOMPARE(resultado.recuentos.duracionMinutos, 40);
        QCOMPARE(resultado.recuentos.configuraciones, 1);
        QCOMPARE(resultado.recuentos.preferencias, 0);
        QCOMPARE(resultado.recuentos.turnos, 2);
    }

    void resumenTexto_incluyeLaHoraAcademicaYSuRecuento()
    {
        Recuentos recuentos;
        recuentos.duracionMinutos = 40;
        recuentos.configuraciones = 1;
        recuentos.cursos = 24;
        const QString resumen = resumenTexto(perfilPorDefecto(), recuentos);

        QVERIFY2(resumen.contains(QStringLiteral("40 min")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("configuraciones: 1")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("Preferencias: 0")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("Cursos: 24")), qPrintable(resumen));
    }

    void resumenTexto_informaClasesMatriculaCapacidadYNoGenerable()
    {
        const Perfil& perfil = perfilPorDefecto();
        const QString resumen = resumenTexto(perfil, recuentosEsperados(perfil));

        // Identidad y aritmética del centro (RF-8, RNF-7).
        QVERIFY2(resumen.contains(QStringLiteral("Centro: Liceo Nacional Robert Serra (2026/2027)")),
                 qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("Clases: 876 (EF 96; 48 por turno)")),
                 qPrintable(resumen));

        // Clases por plan y por año (RF-8): 31059 516 (68+102+102+108+136) y 31060 360
        // (120+80+80+80; su año sin cursos no se imprime). Se derivan de los datos.
        QVERIFY2(resumen.contains(QStringLiteral("Clases por plan: 31059 516 · 31060 360")),
                 qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("Clases por año: 31059 68/102/102/108/136 · "
                                                 "31060 120/80/80/80")),
                 qPrintable(resumen));

        QVERIFY2(resumen.contains(QStringLiteral("Matrícula: 889 (579 + 310)")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("Capacidad docente: 576 por turno (16 × 36)")),
                 qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("Demanda: 414 Mañana · 462 Tarde")),
                 qPrintable(resumen));

        // «No generable» con la causa y la acción (RF-9); no es un rechazo de la aplicación.
        QVERIFY2(resumen.contains(QStringLiteral("Generable: no — ")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("48 clases de Educación Física")),
                 qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("40 franjas de cancha")), qPrintable(resumen));
    }

    void informeTrasAplicarNoFallaYMuestraElMotivo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        const ResultadoAplicacion resultado = aplicar(opciones);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));

        // El informe sale de los recuentos leídos de la base y no convierte el «no generable»
        // en un fallo de la aplicación (RF-9).
        const QString resumen = resumenTexto(perfilPorDefecto(), resultado.recuentos);
        QVERIFY2(resumen.contains(QStringLiteral("Generable: no — ")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("876")), qPrintable(resumen));
        QVERIFY2(resumen.contains(QStringLiteral("889")), qPrintable(resumen));
        QCOMPARE(resultado.estado, EstadoAplicacion::Ok);
    }

    void reemplazoTotal_conPreferenciaPreinsertada_borraSinFallarPorFK()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        OpcionesAplicacion opciones;
        opciones.rutaDb = ruta;
        QVERIFY2(aplicar(opciones).ok(), "siembra inicial");
        preinsertarPreferencia(ruta);

        OpcionesAplicacion reemplazo;
        reemplazo.rutaDb = ruta;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = true;
        reemplazo.ahora = QDateTime(QDate(2026, 10, 29), QTime(12, 0, 0));

        const ResultadoAplicacion resultado = aplicar(reemplazo);
        QVERIFY2(resultado.ok(), qPrintable(resultado.detalle));
        QCOMPARE(resultado.recuentos.preferencias, 0);

        QSqlDatabase db = abrirInspeccion(ruta, m_contador);
        QVERIFY(db.isOpen());
        QCOMPARE(contar(db, QStringLiteral("Preferencia_Profesor")), 0);
        QCOMPARE(contar(db, QStringLiteral("Profesores")), 32);
        cerrarInspeccion(db);
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
        QCOMPARE(contar(antes, QStringLiteral("Profesores")), 32);
        QCOMPARE(contar(antes, QStringLiteral("PlanEstudio_Materia")), 95);
        cerrarInspeccion(antes);

        // Reemplazo total que falla a mitad: la base previa queda intacta (rollback).
        OpcionesAplicacion reemplazo;
        reemplazo.rutaDb = ruta;
        reemplazo.reemplazarTodo = true;
        reemplazo.confirmado = true;
        reemplazo.simularFalloTrasDocentes = true;
        reemplazo.ahora = QDateTime(QDate(2026, 10, 17), QTime(12, 0, 0));

        const ResultadoAplicacion resultado = aplicar(reemplazo);
        QCOMPARE(resultado.estado, EstadoAplicacion::FalloEscritura);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());

        QSqlDatabase despues = abrirInspeccion(ruta, m_contador);
        QVERIFY(despues.isOpen());
        QCOMPARE(contar(despues, QStringLiteral("Profesores")), 32);
        QCOMPARE(contar(despues, QStringLiteral("PlanEstudio_Materia")), 95);
        const QStringList planMateriasDespues =
            filasDe(despues, "SELECT codigo_PlanEstudio, id_Materia, curso, horas "
                             "FROM PlanEstudio_Materia");
        const QStringList profesoresDespues =
            filasDe(despues, "SELECT id, nombre, email, telefono FROM Profesores");
        QVERIFY2(planMateriasDespues == planMateriasAntes,
                 "Las 95 cargas cambiaron tras el fallo en reemplazo.");
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
