#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTest>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include <backend/database/version_esquema.hpp>

using VersionEsquema::EstadoApertura;

namespace {

bool tablaExiste(QSqlDatabase& db, const QString& tabla)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT name FROM sqlite_master "
                                 "WHERE type='table' AND name=:nombre"));
    query.bindValue(QStringLiteral(":nombre"), tabla);
    return query.exec() && query.next();
}

bool columnaExiste(QSqlDatabase& db, const QString& tabla, const QString& columna)
{
    QSqlQuery query(db);
    // `PRAGMA table_info` no admite parámetros enlazados; el nombre de tabla es
    // una constante del test.
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tabla)))
        return false;

    while (query.next())
        if (query.value(1).toString().compare(columna, Qt::CaseInsensitive) == 0)
            return true;

    return false;
}

/// Huella SHA-256 del catálogo real de la base: tablas, índices y disparadores con su
/// DDL tal y como los registró SQLite, en orden estable (por tipo y nombre) y con el
/// espacio normalizado. Cualquier cambio de esquema (tabla, columna, restricción,
/// índice o disparador) cambia la huella.
QByteArray huellaEsquema(QSqlDatabase& db)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT name, sql FROM sqlite_master "
                                  "WHERE type IN ('table','index','trigger') "
                                  "AND name NOT LIKE 'sqlite_%' "
                                  "ORDER BY type, name")))
        return QByteArray();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (query.next())
    {
        QString sql = query.value(1).toString();
        sql.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
        hash.addData((query.value(0).toString() + QLatin1Char('|') + sql.trimmed()
                      + QLatin1Char('\n')).toUtf8());
    }
    return hash.result().toHex();
}

} // namespace

/// Tests de la decisión de apertura y de la creación del esquema único (RF-6).
class TestVersionEsquema : public QObject
{
    Q_OBJECT

private:
    QString m_nombreConexion;

    QSqlDatabase abrirBaseEnMemoria()
    {
        m_nombreConexion = QStringLiteral("test_version_esquema");
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_nombreConexion);
        db.setDatabaseName(QStringLiteral(":memory:"));
        db.open();
        return db;
    }

    void cerrarBase(QSqlDatabase& db)
    {
        if (db.isOpen())
            db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_nombreConexion);
    }

    /// Inserta un tipo de aula escribiendo la clave directamente en la tabla,
    /// como haría una escritura ajena al servicio: el índice único debe frenarla.
    bool insertarTipo(QSqlDatabase& db, const QString& nombre, const QString& clave)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO TiposAula (nombre, clave) "
                                     "VALUES (:nombre, :clave)"));
        query.bindValue(QStringLiteral(":nombre"), nombre);
        query.bindValue(QStringLiteral(":clave"), clave);
        return query.exec();
    }

    /// Inserta un turno mínimo para poder colgar eventos de él.
    bool insertarTurno(QSqlDatabase& db, const QString& nombre)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO Turnos (nombre, inicio, fin, slots) "
                                     "VALUES (:nombre, '07:00', '12:00', 8)"));
        query.bindValue(QStringLiteral(":nombre"), nombre);
        return query.exec();
    }

    bool insertarEvento(QSqlDatabase& db, const QString& nombre, const QString& turno,
                        int posicion, int orden, int duracion)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO Eventos "
                                     "(nombre, turno, despues_de_slot, orden, duracion) "
                                     "VALUES (:nombre, :turno, :posicion, :orden, :duracion)"));
        query.bindValue(QStringLiteral(":nombre"), nombre);
        query.bindValue(QStringLiteral(":turno"), turno);
        query.bindValue(QStringLiteral(":posicion"), posicion);
        query.bindValue(QStringLiteral(":orden"), orden);
        query.bindValue(QStringLiteral(":duracion"), duracion);
        return query.exec();
    }

    bool insertarEventoConHoras(QSqlDatabase& db, const QString& turno, int posicion, int orden,
                                const QVariant& inicio, const QVariant& fin)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO Eventos "
                                     "(nombre, turno, despues_de_slot, orden, duracion, inicio, fin) "
                                     "VALUES ('Evento', :turno, :posicion, :orden, 10, :inicio, :fin)"));
        query.bindValue(QStringLiteral(":turno"), turno);
        query.bindValue(QStringLiteral(":posicion"), posicion);
        query.bindValue(QStringLiteral(":orden"), orden);
        query.bindValue(QStringLiteral(":inicio"), inicio);
        query.bindValue(QStringLiteral(":fin"), fin);
        return query.exec();
    }

    /// Inserta un docente mínimo para poder colgarle preferencias.
    bool insertarProfesor(QSqlDatabase& db, const QString& id)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO Profesores (id, nombre, email) "
                                     "VALUES (:id, :nombre, :email)"));
        query.bindValue(QStringLiteral(":id"), id);
        query.bindValue(QStringLiteral(":nombre"), QStringLiteral("Docente ") + id);
        query.bindValue(QStringLiteral(":email"), id + QStringLiteral("@centro.edu"));
        return query.exec();
    }

    /// Inserta una preferencia escribiendo directamente la tabla (como una escritura
    /// ajena al servicio): el `CHECK`/`UNIQUE` debe frenarla.
    bool insertarPreferencia(QSqlDatabase& db, const QString& idProfesor, const QString& dia,
                             const QString& inicio, const QString& fin, const QString& sentido)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO Preferencia_Profesor "
                                     "(id_Profesor, dia, hora_inicio, hora_fin, sentido) "
                                     "VALUES (:profesor, :dia, :inicio, :fin, :sentido)"));
        query.bindValue(QStringLiteral(":profesor"), idProfesor);
        query.bindValue(QStringLiteral(":dia"), dia);
        query.bindValue(QStringLiteral(":inicio"), inicio);
        query.bindValue(QStringLiteral(":fin"), fin);
        query.bindValue(QStringLiteral(":sentido"), sentido);
        return query.exec();
    }

private slots:

    void versionActual_esSeis()
    {
        QCOMPARE(VersionEsquema::VERSION_ESQUEMA_ACTUAL, 6);
    }

    // ─── decidirApertura ────────────────────────────────────────────────────

    void decidirApertura_inexistente_debeCrear()
    {
        QCOMPARE(VersionEsquema::decidirApertura(false, 0, VersionEsquema::VERSION_ESQUEMA_ACTUAL),
                 EstadoApertura::Crear);
    }

    void decidirApertura_versionIgual_debeAbrir()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, VersionEsquema::VERSION_ESQUEMA_ACTUAL,
                                                 VersionEsquema::VERSION_ESQUEMA_ACTUAL),
                 EstadoApertura::Abrir);
    }

    void decidirApertura_versionAusente_debeFallarEsquema()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 0, VersionEsquema::VERSION_ESQUEMA_ACTUAL),
                 EstadoApertura::FalloEsquema);
    }

    void decidirApertura_versionAnterior_debeFallarEsquema()
    {
        // La 011 sube la marca a 4: una base v3 (sin `ConfiguracionHoraria`) no se migra.
        QCOMPARE(VersionEsquema::decidirApertura(true, 3, VersionEsquema::VERSION_ESQUEMA_ACTUAL),
                 EstadoApertura::FalloEsquema);
    }

    void decidirApertura_versionPosterior_debeFallarEsquema()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true,
                                                 VersionEsquema::VERSION_ESQUEMA_ACTUAL + 1,
                                                 VersionEsquema::VERSION_ESQUEMA_ACTUAL),
                 EstadoApertura::FalloEsquema);
    }

    // ─── crearEsquema ───────────────────────────────────────────────────────

    void crearEsquema_creaCatalogoYFijaVersion()
    {
        QSqlDatabase db = abrirBaseEnMemoria();

        QCOMPARE(VersionEsquema::leerVersionEsquema(db), 0);

        QVERIFY(VersionEsquema::crearEsquema(db));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), VersionEsquema::VERSION_ESQUEMA_ACTUAL);
        QVERIFY(tablaExiste(db, QStringLiteral("Aulas")));
        QVERIFY(tablaExiste(db, QStringLiteral("Profesores")));
        QVERIFY(tablaExiste(db, QStringLiteral("Cursos")));
        QVERIFY(tablaExiste(db, QStringLiteral("TiposAula")));
        QVERIFY(tablaExiste(db, QStringLiteral("Eventos")));
        QVERIFY(tablaExiste(db, QStringLiteral("ConfiguracionHoraria")));
        QVERIFY(tablaExiste(db, QStringLiteral("Preferencia_Profesor")));
        QVERIFY(!tablaExiste(db, QStringLiteral("Curso_Materia")));
        QVERIFY(!tablaExiste(db, QStringLiteral("Recesos")));

        // Idempotente: repetir no falla ni cambia la versión.
        QVERIFY(VersionEsquema::crearEsquema(db));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        cerrarBase(db);
    }

    void crearEsquema_dejaElTipoGeneral()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // Toda base nueva queda con exactamente una fila del tipo general (RF-1).
        QSqlQuery query(db);
        QVERIFY2(query.exec(QStringLiteral("SELECT nombre, clave FROM TiposAula")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Aula de estudio"));
        QCOMPARE(query.value(1).toString(), QStringLiteral("aula de estudio"));
        QVERIFY(!query.next()); // exactamente una fila

        cerrarBase(db);
    }

    void crearEsquema_esIdempotente()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery primera(db);
        QVERIFY(primera.exec(QStringLiteral("SELECT id, nombre, clave FROM TiposAula")));
        QVERIFY(primera.next());
        const int idAntes = primera.value(0).toInt();
        const QString nombreAntes = primera.value(1).toString();
        const QString claveAntes = primera.value(2).toString();

        // Repetir el paso que garantiza el tipo general no lo duplica ni lo cambia.
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery cuenta(db);
        QVERIFY(cuenta.exec(QStringLiteral("SELECT COUNT(*) FROM TiposAula")));
        QVERIFY(cuenta.next());
        QCOMPARE(cuenta.value(0).toInt(), 1);

        QSqlQuery despues(db);
        QVERIFY(despues.exec(QStringLiteral("SELECT id, nombre, clave FROM TiposAula")));
        QVERIFY(despues.next());
        QCOMPARE(despues.value(0).toInt(), idAntes);
        QCOMPARE(despues.value(1).toString(), nombreAntes);
        QCOMPARE(despues.value(2).toString(), claveAntes);
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        cerrarBase(db);
    }

    void crearEsquema_fallido_noDejaEsquemaParcial()
    {
        QSqlDatabase db = abrirBaseEnMemoria();

        // Fuerza un fallo POSTERIOR al alta del tipo general y a los dos disparadores:
        // `ConfiguracionHoraria` preexistente sin `duracion_minutos` hace fallar el
        // `INSERT OR IGNORE` de la migración. La tabla preexistente queda (se creó fuera de
        // la transacción), pero todo lo escrito dentro se revierte (RF-1).
        QSqlQuery previa(db);
        QVERIFY(previa.exec(QStringLiteral(
            "CREATE TABLE ConfiguracionHoraria (id INTEGER PRIMARY KEY CHECK(id = 1))")));

        QVERIFY(!VersionEsquema::crearEsquema(db));

        // La marca no se fija y no queda ninguna tabla del esquema…
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), 0);
        QVERIFY(tablaExiste(db, QStringLiteral("ConfiguracionHoraria"))); // la preexistente
        QVERIFY(!tablaExiste(db, QStringLiteral("TiposAula")));
        QVERIFY(!tablaExiste(db, QStringLiteral("Aulas")));
        QVERIFY(!tablaExiste(db, QStringLiteral("Cursos")));

        // …ni la fila canónica ni los dos disparadores.
        QSqlQuery fila(db);
        QVERIFY(!fila.exec(QStringLiteral("SELECT 1 FROM TiposAula LIMIT 1"))); // tabla revertida

        QSqlQuery disparadores(db);
        QVERIFY(disparadores.exec(QStringLiteral(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'trigger' AND name IN "
            "('TiposAula_general_no_borrar', 'TiposAula_general_no_renombrar')")));
        QVERIFY(disparadores.next());
        QCOMPARE(disparadores.value(0).toInt(), 0);

        cerrarBase(db);
    }

    void crearEsquema_dejaConfiguracionHorariaCon35YUnaSolaFila()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery query(db);
        QVERIFY2(query.exec(QStringLiteral(
                     "SELECT id, duracion_minutos FROM ConfiguracionHoraria")),
                 qPrintable(query.lastError().text()));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
        QCOMPARE(query.value(1).toInt(), 35); // valor por defecto (RF-2)
        QVERIFY(!query.next());               // una sola configuración global (RF-1)

        // Idempotente: volver a crear el esquema no duplica la fila ni cambia el valor.
        QVERIFY(VersionEsquema::crearEsquema(db));
        QSqlQuery cuenta(db);
        QVERIFY(cuenta.exec(QStringLiteral(
            "SELECT COUNT(*), MAX(duracion_minutos) FROM ConfiguracionHoraria")));
        QVERIFY(cuenta.next());
        QCOMPARE(cuenta.value(0).toInt(), 1);
        QCOMPARE(cuenta.value(1).toInt(), 35);

        cerrarBase(db);
    }

    void configuracionHoraria_check_rechazaSegundaFilaYValoresFueraDeRango()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // Una sola configuración por base: `CHECK(id = 1)`.
        QSqlQuery segunda(db);
        QVERIFY(!segunda.exec(QStringLiteral(
            "INSERT INTO ConfiguracionHoraria (id, duracion_minutos) VALUES (2, 35)")));

        // El rango 30–90 es la última barrera para escrituras directas (RF-1).
        QSqlQuery baja(db);
        QVERIFY(!baja.exec(QStringLiteral(
            "UPDATE ConfiguracionHoraria SET duracion_minutos = 29 WHERE id = 1")));

        QSqlQuery alta(db);
        QVERIFY(!alta.exec(QStringLiteral(
            "UPDATE ConfiguracionHoraria SET duracion_minutos = 91 WHERE id = 1")));

        // Los rechazos no tocan el valor guardado.
        QSqlQuery lectura(db);
        QVERIFY(lectura.exec(QStringLiteral(
            "SELECT duracion_minutos FROM ConfiguracionHoraria WHERE id = 1")));
        QVERIFY(lectura.next());
        QCOMPARE(lectura.value(0).toInt(), 35);

        cerrarBase(db);
    }

    // ─── esquemaEsperado ────────────────────────────────────────────────────

    void esquemaEsperado_aceptaEsquemaCorrecto()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(VersionEsquema::esquemaEsperado(db));
        cerrarBase(db);
    }

    void esquemaEsperado_exigeElIndiceYLosDosDisparadores()
    {
        // Cada objeto exigido se comprueba por nombre: quitarlo invalida la base y el
        // motivo lo nombra (RF-6).
        {
            QSqlDatabase db = abrirBaseEnMemoria();
            QVERIFY(VersionEsquema::crearEsquema(db));
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("DROP INDEX TiposAula_clave")));

            QString motivo;
            QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
            QVERIFY(motivo.contains(QStringLiteral("TiposAula_clave")));
            cerrarBase(db);
        }
        {
            QSqlDatabase db = abrirBaseEnMemoria();
            QVERIFY(VersionEsquema::crearEsquema(db));
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar")));

            QString motivo;
            QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
            QVERIFY(motivo.contains(QStringLiteral("TiposAula_general_no_borrar")));
            cerrarBase(db);
        }
        {
            QSqlDatabase db = abrirBaseEnMemoria();
            QVERIFY(VersionEsquema::crearEsquema(db));
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_renombrar")));

            QString motivo;
            QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
            QVERIFY(motivo.contains(QStringLiteral("TiposAula_general_no_renombrar")));
            cerrarBase(db);
        }
    }

    void esquemaEsperado_exigeExactamenteUnaFila()
    {
        // P10: el disparador de borrado impide quitar la fila; se quita el disparador en el
        // montaje (base en memoria y por caso; no se restaura).

        // Caso 1: 0 filas.
        {
            QSqlDatabase db = abrirBaseEnMemoria();
            QVERIFY(VersionEsquema::crearEsquema(db));
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar")));
            QVERIFY(q.exec(QStringLiteral("DELETE FROM TiposAula")));

            QString motivo;
            QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
            QVERIFY(motivo.contains(QStringLiteral("0")));
            QVERIFY(motivo.contains(QStringLiteral("aula de estudio")));
            cerrarBase(db);
        }

        // Caso 2: dos filas con nombre canónico (la segunda con la clave en mayúsculas).
        {
            QSqlDatabase db = abrirBaseEnMemoria();
            QVERIFY(VersionEsquema::crearEsquema(db));
            QVERIFY(insertarTipo(db, QStringLiteral("AULA DE ESTUDIO"),
                                 QStringLiteral("AULA DE ESTUDIO")));

            QString motivo;
            QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
            QVERIFY(motivo.contains(QStringLiteral("2")));
            QVERIFY(motivo.contains(QStringLiteral("aula de estudio")));
            cerrarBase(db);
        }

        // Caso 3: una sola fila canónica con la clave almacenada distinta.
        {
            QSqlDatabase db = abrirBaseEnMemoria();
            QVERIFY(VersionEsquema::crearEsquema(db));
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar")));
            QVERIFY(q.exec(QStringLiteral("DELETE FROM TiposAula")));
            QVERIFY(insertarTipo(db, QStringLiteral("Aula de estudio"),
                                 QStringLiteral("AULA DE ESTUDIO")));

            QString motivo;
            QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
            QVERIFY(motivo.contains(QStringLiteral("1")));
            QVERIFY(motivo.contains(QStringLiteral("AULA DE ESTUDIO")));
            QVERIFY(motivo.contains(QStringLiteral("se esperaba")));
            cerrarBase(db);
        }
    }

    void esquemaEsperado_toleraObjetosDeMas()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // Un índice de más no invalida la base (RF-6).
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE INDEX indice_extra ON Aulas(nombre)")));
        QVERIFY(VersionEsquema::esquemaEsperado(db));

        cerrarBase(db);
    }

    void esquemaEsperado_noDejaTransaccionAbierta()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QString motivo;
        QVERIFY(VersionEsquema::esquemaEsperado(db, &motivo));

        // Si la guardia hubiera abierto una transacción y no la cerrara, esta fallaría.
        QVERIFY(db.transaction());
        QVERIFY(db.commit());

        cerrarBase(db);
    }

    void esquemaEsperado_funcionaDentroDeUnaTransaccion()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // P5: con una transacción ya abierta, la guardia usa esa instantánea y no la cierra.
        QVERIFY(db.transaction());
        QVERIFY(VersionEsquema::esquemaEsperado(db));
        QVERIFY(db.commit());

        cerrarBase(db);
    }

    void esquemaEsperado_exigeLaConfiguracionHorariaYSusColumnas()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // El catálogo incluye la tabla y sus columnas.
        QVERIFY(columnaExiste(db, QStringLiteral("ConfiguracionHoraria"), QStringLiteral("id")));
        QVERIFY(columnaExiste(db, QStringLiteral("ConfiguracionHoraria"),
                              QStringLiteral("duracion_minutos")));
        QVERIFY(VersionEsquema::esquemaEsperado(db));

        // Sin la tabla, el motivo la nombra como faltante.
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("DROP TABLE ConfiguracionHoraria")));
        QString motivo;
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("ConfiguracionHoraria")));

        // Con la tabla pero sin la columna del valor, el motivo nombra la columna.
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE ConfiguracionHoraria (id INTEGER PRIMARY KEY CHECK(id = 1))")));
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("duracion_minutos")));

        cerrarBase(db);
    }

    void esquemaEsperado_rechazaTablaRetirada()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Curso_Materia (id INTEGER)")));

        QVERIFY(!VersionEsquema::esquemaEsperado(db));
        cerrarBase(db);
    }

    void esquemaEsperado_rechazaCursosSinColumnaAula()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("DROP TABLE Cursos")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Cursos ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "anio INTEGER NOT NULL,"
                                      "seccion TEXT NOT NULL,"
                                      "turno TEXT,"
                                      "fecha_creacion TEXT,"
                                      "fecha_modificacion TEXT,"
                                      "UNIQUE(anio, seccion))")));

        QVERIFY(!VersionEsquema::esquemaEsperado(db)); // falta la columna `aula`
        cerrarBase(db);
    }

    void crearEsquema_columnasDeTiposEventosAulasYMaterias()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // TiposAula: nombre visible y clave normalizada almacenada.
        QVERIFY(columnaExiste(db, QStringLiteral("TiposAula"), QStringLiteral("id")));
        QVERIFY(columnaExiste(db, QStringLiteral("TiposAula"), QStringLiteral("nombre")));
        QVERIFY(columnaExiste(db, QStringLiteral("TiposAula"), QStringLiteral("clave")));

        // Eventos: identidad propia y orden persistente dentro de la posición.
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("id")));
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("nombre")));
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("turno")));
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("despues_de_slot")));
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("orden")));
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("duracion")));
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("inicio")));
        QVERIFY(columnaExiste(db, QStringLiteral("Eventos"), QStringLiteral("fin")));

        // La referencia de tipo es obligatoria en aulas y materias.
        QVERIFY(columnaExiste(db, QStringLiteral("Aulas"), QStringLiteral("id_tipo_aula")));
        QVERIFY(columnaExiste(db, QStringLiteral("Materias"), QStringLiteral("id_tipo_aula")));

        cerrarBase(db);
    }

    void esquemaEsperado_rechazaRegistroDeRecesos()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Recesos ("
                                      "turno TEXT, despues_de_slot INTEGER, duracion INTEGER)")));

        QString motivo;
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("Recesos"), Qt::CaseInsensitive));
        cerrarBase(db);
    }

    void esquemaEsperado_rechazaEventosSinOrden()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("DROP TABLE Eventos")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Eventos ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "nombre TEXT NOT NULL,"
                                      "turno TEXT NOT NULL,"
                                      "despues_de_slot INTEGER NOT NULL,"
                                      "duracion INTEGER NOT NULL)")));

        QString motivo;
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("Eventos")));
        QVERIFY(motivo.contains(QStringLiteral("orden"))); // columna que falta
        cerrarBase(db);
    }

    void esquemaEsperado_rechazaTiposAulaSinClave()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("DROP TABLE TiposAula")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE TiposAula ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "nombre TEXT NOT NULL)")));

        QString motivo;
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("TiposAula")));
        QVERIFY(motivo.contains(QStringLiteral("clave"))); // columna que falta
        cerrarBase(db);
    }

    // ─── unicidad normalizada del catálogo de tipos ─────────────────────────

    void tipoAula_claveUnica_rechazaDuplicadoAscii()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QVERIFY(insertarTipo(db, QStringLiteral("Laboratorio"), QStringLiteral("laboratorio")));
        // Misma clave, distinta capitalización: la escritura directa se rechaza.
        QVERIFY(!insertarTipo(db, QStringLiteral("laboratorio"), QStringLiteral("laboratorio")));
        cerrarBase(db);
    }

    void tipoAula_claveUnica_rechazaDuplicadoAcentuado()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // «Física».toCaseFolded() == «física» == «FÍSICA».toCaseFolded().
        QVERIFY(insertarTipo(db, QStringLiteral("Física"), QStringLiteral("física")));
        QVERIFY(!insertarTipo(db, QStringLiteral("FÍSICA"), QStringLiteral("física")));
        cerrarBase(db);
    }

    void tipoAula_check_rechazaNombreConEspaciosYClaveVacia()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QVERIFY(!insertarTipo(db, QStringLiteral(" Laboratorio"), QStringLiteral(" laboratorio")));
        QVERIFY(!insertarTipo(db, QStringLiteral("Laboratorio"), QString()));
        cerrarBase(db);
    }

    // ─── protección del tipo general: borrado (015 T2) ─────────────────────

    void proteccion_borradoDirecto_delGeneral_esRechazado()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // Escritura directa que no pasa por el servicio: el disparador la rechaza (RF-3).
        QSqlQuery borrado(db);
        QVERIFY(!borrado.exec(QStringLiteral(
            "DELETE FROM TiposAula WHERE clave = 'aula de estudio'")));
        QVERIFY(borrado.lastError().text().contains(
            QStringLiteral("No se puede eliminar el tipo de aula general del centro")));

        // La fila sigue existiendo: no se puede dejar el centro sin el tipo general.
        QSqlQuery cuenta(db);
        QVERIFY(cuenta.exec(QStringLiteral(
            "SELECT COUNT(*) FROM TiposAula WHERE clave = 'aula de estudio'")));
        QVERIFY(cuenta.next());
        QCOMPARE(cuenta.value(0).toInt(), 1);

        cerrarBase(db);
    }

    void proteccion_borradoDeUnEspecializado_sePermite()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(insertarTipo(db, QStringLiteral("Laboratorio"), QStringLiteral("laboratorio")));

        // Control positivo: un tipo especializado sin dependientes se borra con normalidad.
        QSqlQuery borrado(db);
        QVERIFY2(borrado.exec(QStringLiteral(
                     "DELETE FROM TiposAula WHERE clave = 'laboratorio'")),
                 qPrintable(borrado.lastError().text()));

        QSqlQuery cuenta(db);
        QVERIFY(cuenta.exec(QStringLiteral("SELECT COUNT(*) FROM TiposAula")));
        QVERIFY(cuenta.next());
        QCOMPARE(cuenta.value(0).toInt(), 1); // solo queda el tipo general

        cerrarBase(db);
    }

    // ─── protección del tipo general: renombrado (015 T3) ──────────────────

    void proteccion_renombradoDirecto_delGeneral_esRechazado()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // Cambiar solo el nombre visible: rechazado (RF-4).
        QSqlQuery porNombre(db);
        QVERIFY(!porNombre.exec(QStringLiteral(
            "UPDATE TiposAula SET nombre = 'Aula común' WHERE clave = 'aula de estudio'")));
        QVERIFY(porNombre.lastError().text().contains(
            QStringLiteral("No se puede renombrar el tipo de aula general del centro")));

        // Cambiar solo la clave almacenada: rechazado igualmente.
        QSqlQuery porClave(db);
        QVERIFY(!porClave.exec(QStringLiteral(
            "UPDATE TiposAula SET clave = 'AULA DE ESTUDIO' "
            "WHERE clave = 'aula de estudio'")));
        QVERIFY(porClave.lastError().text().contains(
            QStringLiteral("No se puede renombrar el tipo de aula general del centro")));

        // La fila conserva el nombre y la clave anteriores.
        QSqlQuery fila(db);
        QVERIFY(fila.exec(QStringLiteral(
            "SELECT nombre, clave FROM TiposAula WHERE clave = 'aula de estudio'")));
        QVERIFY(fila.next());
        QCOMPARE(fila.value(0).toString(), QStringLiteral("Aula de estudio"));
        QCOMPARE(fila.value(1).toString(), QStringLiteral("aula de estudio"));
        QVERIFY(!fila.next());

        cerrarBase(db);
    }

    void proteccion_updateSoloDeFecha_delGeneral_sePermite()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // `BEFORE UPDATE OF nombre, clave` no se dispara si solo cambia la fecha.
        QSqlQuery fecha(db);
        QVERIFY2(fecha.exec(QStringLiteral(
                     "UPDATE TiposAula SET fecha_modificacion = '2030-01-01 00:00:00' "
                     "WHERE clave = 'aula de estudio'")),
                 qPrintable(fecha.lastError().text()));

        cerrarBase(db);
    }

    void proteccion_updateSinCambios_sePermite()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        // Asignar el mismo nombre y la misma clave no es un renombrado: `NEW <> OLD` es falso.
        QSqlQuery iguales(db);
        QVERIFY2(iguales.exec(QStringLiteral(
                     "UPDATE TiposAula SET nombre = 'Aula de estudio', "
                     "clave = 'aula de estudio' WHERE clave = 'aula de estudio'")),
                 qPrintable(iguales.lastError().text()));

        cerrarBase(db);
    }

    // ─── eventos: unicidad y restricciones básicas ──────────────────────────

    void eventos_unicoTurnoPosicionOrden_rechazaDuplicado()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(insertarTurno(db, QStringLiteral("Mañana")));

        QVERIFY(insertarEvento(db, QStringLiteral("Receso"), QStringLiteral("Mañana"), 2, 1, 10));
        // Mismo turno, posición y orden: rechazado por UNIQUE.
        QVERIFY(!insertarEvento(db, QStringLiteral("Acto"), QStringLiteral("Mañana"), 2, 1, 10));
        // Misma posición con el orden siguiente: admitido.
        QVERIFY(insertarEvento(db, QStringLiteral("Acto"), QStringLiteral("Mañana"), 2, 2, 10));

        // Cada evento conserva su identidad propia.
        QSqlQuery ids(db);
        QVERIFY(ids.exec(QStringLiteral("SELECT id FROM Eventos ORDER BY orden")));
        QVERIFY(ids.next());
        const int primero = ids.value(0).toInt();
        QVERIFY(ids.next());
        QVERIFY(ids.value(0).toInt() != primero);

        cerrarBase(db);
    }

    void eventos_check_rechazaPosicionCeroOrdenCeroYDuracionCero()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(insertarTurno(db, QStringLiteral("Mañana")));

        QVERIFY(!insertarEvento(db, QStringLiteral("Receso"), QStringLiteral("Mañana"), 0, 1, 10));
        QVERIFY(!insertarEvento(db, QStringLiteral("Receso"), QStringLiteral("Mañana"), 1, 0, 10));
        QVERIFY(!insertarEvento(db, QStringLiteral("Receso"), QStringLiteral("Mañana"), 1, 1, 0));
        cerrarBase(db);
    }

    void eventos_check_exigeElParDeHoras()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(insertarTurno(db, QStringLiteral("Mañana")));

        // Par incompleto: rechazado por CHECK((inicio IS NULL) = (fin IS NULL)).
        QVERIFY(!insertarEventoConHoras(db, QStringLiteral("Mañana"), 1, 1,
                                        QStringLiteral("08:00"), QVariant()));
        QVERIFY(!insertarEventoConHoras(db, QStringLiteral("Mañana"), 1, 2,
                                        QVariant(), QStringLiteral("08:10")));
        // Ambos declarados y ninguno declarado: admitidos.
        QVERIFY(insertarEventoConHoras(db, QStringLiteral("Mañana"), 1, 3,
                                       QStringLiteral("08:00"), QStringLiteral("08:10")));
        QVERIFY(insertarEventoConHoras(db, QStringLiteral("Mañana"), 2, 1,
                                       QVariant(), QVariant()));
        cerrarBase(db);
    }

    // ─── preferencias del docente: columnas, CHECK y UNIQUE ─────────────────

    void crearEsquema_columnasDePreferencias()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QVERIFY(columnaExiste(db, QStringLiteral("Preferencia_Profesor"), QStringLiteral("id")));
        QVERIFY(columnaExiste(db, QStringLiteral("Preferencia_Profesor"),
                              QStringLiteral("id_Profesor")));
        QVERIFY(columnaExiste(db, QStringLiteral("Preferencia_Profesor"), QStringLiteral("dia")));
        QVERIFY(columnaExiste(db, QStringLiteral("Preferencia_Profesor"),
                              QStringLiteral("hora_inicio")));
        QVERIFY(columnaExiste(db, QStringLiteral("Preferencia_Profesor"),
                              QStringLiteral("hora_fin")));
        QVERIFY(columnaExiste(db, QStringLiteral("Preferencia_Profesor"),
                              QStringLiteral("sentido")));
        cerrarBase(db);
    }

    void preferencia_check_rechazaDiaSentidoYHorasInvalidas()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(insertarProfesor(db, QStringLiteral("P1")));

        // Día fuera de 0–6 (se guarda como texto).
        QVERIFY(!insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("-1"),
                                     QStringLiteral("08:00"), QStringLiteral("10:00"),
                                     QStringLiteral("preferida")));
        QVERIFY(!insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("7"),
                                     QStringLiteral("08:00"), QStringLiteral("10:00"),
                                     QStringLiteral("preferida")));

        // Sentido distinto de «preferida»/«evitada».
        QVERIFY(!insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("1"),
                                     QStringLiteral("08:00"), QStringLiteral("10:00"),
                                     QStringLiteral("neutra")));

        // Inicio igual al fin y inicio posterior: `CHECK(hora_inicio < hora_fin)`.
        QVERIFY(!insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("1"),
                                     QStringLiteral("10:00"), QStringLiteral("10:00"),
                                     QStringLiteral("preferida")));
        QVERIFY(!insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("1"),
                                     QStringLiteral("12:00"), QStringLiteral("08:00"),
                                     QStringLiteral("preferida")));

        // Una fila válida sí entra: el rechazo es de las restricciones, no de la tabla.
        QVERIFY(insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("1"),
                                    QStringLiteral("08:00"), QStringLiteral("10:00"),
                                    QStringLiteral("preferida")));

        QSqlQuery cuenta(db);
        QVERIFY(cuenta.exec(QStringLiteral("SELECT COUNT(*) FROM Preferencia_Profesor")));
        QVERIFY(cuenta.next());
        QCOMPARE(cuenta.value(0).toInt(), 1);

        cerrarBase(db);
    }

    void preferencia_unique_rechazaDuplicadoExacto()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(insertarProfesor(db, QStringLiteral("P1")));
        QVERIFY(insertarProfesor(db, QStringLiteral("P2")));

        QVERIFY(insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("2"),
                                    QStringLiteral("08:00"), QStringLiteral("10:00"),
                                    QStringLiteral("evitada")));
        // Duplicado exacto: mismo docente, día, rango y sentido.
        QVERIFY(!insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("2"),
                                     QStringLiteral("08:00"), QStringLiteral("10:00"),
                                     QStringLiteral("evitada")));
        // Mismo rango con otro sentido: el `UNIQUE` incluye `sentido`.
        QVERIFY(insertarPreferencia(db, QStringLiteral("P1"), QStringLiteral("2"),
                                    QStringLiteral("08:00"), QStringLiteral("10:00"),
                                    QStringLiteral("preferida")));
        // Otro docente con el mismo rango y sentido: la identidad incluye al docente.
        QVERIFY(insertarPreferencia(db, QStringLiteral("P2"), QStringLiteral("2"),
                                    QStringLiteral("08:00"), QStringLiteral("10:00"),
                                    QStringLiteral("evitada")));

        QSqlQuery cuenta(db);
        QVERIFY(cuenta.exec(QStringLiteral("SELECT COUNT(*) FROM Preferencia_Profesor")));
        QVERIFY(cuenta.next());
        QCOMPARE(cuenta.value(0).toInt(), 3);

        cerrarBase(db);
    }

    void esquemaEsperado_exigeLasPreferenciasYSusColumnas()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(VersionEsquema::esquemaEsperado(db));

        // Sin la tabla, el motivo la nombra como faltante.
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("DROP TABLE Preferencia_Profesor")));
        QString motivo;
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("Preferencia_Profesor")));

        // Con la tabla pero sin `sentido`, el motivo nombra la columna.
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE Preferencia_Profesor (id INTEGER PRIMARY KEY)")));
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("sentido")));

        cerrarBase(db);
    }

    void esquemaEsperado_explicaElMotivo()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Curso_Materia (id INTEGER)")));

        QString motivo;
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("Curso_Materia"))); // tabla retirada presente

        QVERIFY(q.exec(QStringLiteral("DROP TABLE Curso_Materia")));
        QVERIFY(q.exec(QStringLiteral("DROP TABLE Cursos")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Cursos (id INTEGER PRIMARY KEY)")));

        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("anio")));   // columna que falta
        QVERIFY(motivo.contains(QStringLiteral("aula")));   // columna que falta

        QVERIFY(q.exec(QStringLiteral("DROP TABLE Cursos")));
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.isEmpty()); // catálogo correcto: sin motivo

        cerrarBase(db);
    }

    // ─── huella del esquema ────────────────────────────────────────────────

    void huellaEsquema_noCambiaSinSubirLaMarcaDeVersion()
    {
        // Guardia anti-regresión del incidente real de 2026-10-17: el DDL de
        // `Cursos` cambió (nombre/aula_fija → anio/seccion/turno/aula) sin subir
        // VERSION_ESQUEMA_ACTUAL, y las bases existentes con la marca antigua
        // quedaron indistinguibles por número de versión.
        //
        // SI ESTE TEST FALLA AL CAMBIAR EL ESQUEMA, ACTÚA ASÍ:
        //   1. Sube VERSION_ESQUEMA_ACTUAL en `backend/database/version_esquema.hpp`.
        //   2. Actualiza TABLAS_ESPERADAS/TABLAS_RETIRADAS y las listas COLUMNAS_* de
        //      `version_esquema.cpp`.
        //   3. Ejecuta el test: falla mostrando la huella real del esquema recién
        //      creado; cópiala a HUELLAS_CONOCIDAS más abajo y repite.
        // Cambiar el DDL sin subir la versión vuelve a romper las bases de los
        // usuarios: la marca ya no las distingue y solo el catálogo las rechaza.
        // Las entradas 2–5 son históricas: se calcularon con el resumen de solo tablas.
        // Este test solo compara la entrada de VERSION_ESQUEMA_ACTUAL (v6), calculada con
        // tablas, índices y disparadores.
        static const struct { int version; const char* huella; } HUELLAS_CONOCIDAS[] = {
            { 2, "b7684333c36458b0529308f9130e2a3b105c4cd4086b5f0e470a26ca917eb882" },
            { 3, "ba386397216ac9fb8d427ab4603c09ce3d8342cc5605dda16eecc78a02f1701b" },
            { 4, "f630cc2547e21454b8273fa255124142138e254bfa8b2a580c52bf8b051f7101" },
            { 5, "eebe660c385f9802dc7b0b63abb6e7821db3ad3f2ec35982eeae55e49e9deaa0" },
            // v6: huella del esquema con tablas, índice y disparadores (incluye el cuerpo
            // del disparador: es guardia de release, no forma parte de la validez).
            { 6, "15d99b8628a43a09542db1e0a443c15f52302ba893a6af107445370d9fe7ac3a" },
        };

        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        const QByteArray huella = huellaEsquema(db);
        QVERIFY(!huella.isEmpty());

        const char* registrada = nullptr;
        for (const auto& entrada : HUELLAS_CONOCIDAS)
            if (entrada.version == VersionEsquema::VERSION_ESQUEMA_ACTUAL)
                registrada = entrada.huella;

        QVERIFY2(registrada,
                 "No hay huella registrada para VERSION_ESQUEMA_ACTUAL en "
                 "HUELLAS_CONOCIDAS (test_version_esquema.cpp): añade la huella del "
                 "esquema recién creado.");

        QCOMPARE(QString::fromLatin1(huella), QString::fromLatin1(registrada));

        // La marca y el catálogo deben viajar juntos: una base recién creada es
        // válida para la apertura con el mismo esquema congelado aquí.
        QVERIFY(VersionEsquema::esquemaEsperado(db));

        cerrarBase(db);
    }
};

QTEST_MAIN(TestVersionEsquema)
#include "test_version_esquema.moc"
