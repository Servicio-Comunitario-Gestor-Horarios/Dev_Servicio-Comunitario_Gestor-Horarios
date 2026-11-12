#include "backend/database/version_esquema.hpp"

#include "backend/database/migracion.hpp"
#include "backend/database/tipo_general.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QVector>

#include "common/tipo_general.hpp"

namespace
{
    /// Tablas que debe tener el esquema único.
    const QStringList TABLAS_ESPERADAS = {
        QStringLiteral("ConfiguracionHoraria"),
        QStringLiteral("Aulas"),        QStringLiteral("Profesores"),
        QStringLiteral("Materias"),     QStringLiteral("Profesor_Materia"),
        QStringLiteral("Disponibilidad_Profesor"), QStringLiteral("PlanEstudio"),
        QStringLiteral("PlanEstudio_Materia"),     QStringLiteral("Turnos"),
        QStringLiteral("TiposAula"),    QStringLiteral("Eventos"),
        QStringLiteral("Preferencia_Profesor"),
        QStringLiteral("Cursos")
    };

    /// Índices que debe tener el esquema, por nombre (no se exigen los `sqlite_autoindex_*`).
    const QStringList INDICES_ESPERADOS = {
        QStringLiteral("TiposAula_clave")
    };

    /// Disparadores que debe tener el esquema, por nombre (su cuerpo no es identidad).
    const QStringList DISPARADORES_ESPERADOS = {
        QStringLiteral("TiposAula_general_no_borrar"),
        QStringLiteral("TiposAula_general_no_renombrar")
    };


    /// Tablas que NO deben existir (retiradas en el modelo nuevo).
    const QStringList TABLAS_RETIRADAS = {
        QStringLiteral("Curso_Materia"), QStringLiteral("Recesos")
    };

    /// Columnas que debe tener la tabla `Cursos`.
    const QStringList COLUMNAS_CURSOS = {
        QStringLiteral("anio"), QStringLiteral("seccion"),
        QStringLiteral("turno"), QStringLiteral("aula")
    };

    /// Columnas que debe tener la tabla `Aulas` (el tipo es obligatorio).
    const QStringList COLUMNAS_AULAS = {
        QStringLiteral("nombre"), QStringLiteral("capacidad"),
        QStringLiteral("edificio"), QStringLiteral("piso"),
        QStringLiteral("id_tipo_aula")
    };

    /// Columnas que debe tener la tabla `Materias` (el tipo es obligatorio).
    const QStringList COLUMNAS_MATERIAS = {
        QStringLiteral("nombre"), QStringLiteral("requisitos"),
        QStringLiteral("id_tipo_aula")
    };

    /// Columnas que debe tener la tabla `Eventos` (identidad y orden propios).
    const QStringList COLUMNAS_EVENTOS = {
        QStringLiteral("id"), QStringLiteral("nombre"), QStringLiteral("turno"),
        QStringLiteral("despues_de_slot"), QStringLiteral("orden"),
        QStringLiteral("duracion"), QStringLiteral("inicio"), QStringLiteral("fin")
    };

    /// Columnas que debe tener la tabla `TiposAula` (clave normalizada almacenada).
    const QStringList COLUMNAS_TIPOS = {
        QStringLiteral("id"), QStringLiteral("nombre"), QStringLiteral("clave")
    };

    /// Columnas que debe tener la tabla `ConfiguracionHoraria` (una sola fila).
    const QStringList COLUMNAS_CONFIGURACION = {
        QStringLiteral("id"), QStringLiteral("duracion_minutos")
    };

    /// Columnas que debe tener la tabla `Preferencia_Profesor` (sentido propio).
    const QStringList COLUMNAS_PREFERENCIAS = {
        QStringLiteral("id"), QStringLiteral("id_Profesor"), QStringLiteral("dia"),
        QStringLiteral("hora_inicio"), QStringLiteral("hora_fin"), QStringLiteral("sentido")
    };

    /// Tablas del catálogo con sus columnas obligatorias.
    struct TablaConColumnas
    {
        const char* tabla;
        const QStringList* columnas;
    };

    const TablaConColumnas TABLAS_CON_COLUMNAS[] = {
        { "ConfiguracionHoraria", &COLUMNAS_CONFIGURACION },
        { "Preferencia_Profesor", &COLUMNAS_PREFERENCIAS },
        { "Cursos", &COLUMNAS_CURSOS },
        { "Aulas", &COLUMNAS_AULAS },
        { "Materias", &COLUMNAS_MATERIAS },
        { "Eventos", &COLUMNAS_EVENTOS },
        { "TiposAula", &COLUMNAS_TIPOS },
    };

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
        // una constante interna, por lo que la interpolación no supone inyección.
        if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tabla)))
            return false;

        while (query.next())
            if (query.value(1).toString().compare(columna, Qt::CaseInsensitive) == 0)
                return true;

        return false;
    }

    /// ¿Existe el objeto `nombre` de tipo `tipo` (`index`/`trigger`) en `sqlite_master`?
    bool objetoExiste(QSqlDatabase& db, const QString& tipo, const QString& nombre)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type = :tipo AND name = :nombre"));
        query.bindValue(QStringLiteral(":tipo"), tipo);
        query.bindValue(QStringLiteral(":nombre"), nombre);
        return query.exec() && query.next();
    }
} // namespace

VersionEsquema::EstadoApertura VersionEsquema::decidirApertura(bool existe,
                                                               int versionArchivo,
                                                               int versionEsperada)
{
    if (!existe)
        return EstadoApertura::Crear;

    if (versionArchivo == versionEsperada)
        return EstadoApertura::Abrir;

    // Ausente (<=0), anterior o posterior: todas se tratan como esquema no esperado.
    return EstadoApertura::FalloEsquema;
}

int VersionEsquema::leerVersionEsquema(QSqlDatabase& db)
{
    QSqlQuery query(db);

    if (!query.exec(QStringLiteral("PRAGMA user_version")))
    {
        qCritical() << "No se pudo leer la version de esquema:" << query.lastError().text();
        return -1;
    }

    if (!query.next())
        return -1;

    return query.value(0).toInt();
}

bool VersionEsquema::fijarVersionEsquema(QSqlDatabase& db, int version)
{
    QSqlQuery query(db);

    // `PRAGMA user_version` no admite parámetros enlazados; `version` es un int,
    // por lo que la interpolación no supone riesgo de inyección.
    if (!query.exec(QStringLiteral("PRAGMA user_version = %1").arg(version)))
    {
        qCritical() << "No se pudo fijar la version de esquema:" << query.lastError().text();
        return false;
    }

    return true;
}

bool VersionEsquema::crearEsquema(QSqlDatabase& db)
{
    if (!db.isOpen())
    {
        qCritical() << "No se puede crear el esquema: la base de datos no esta abierta.";
        return false;
    }

    if (!db.transaction())
    {
        qCritical() << "No se pudo iniciar la transaccion de creacion del esquema:"
                    << db.lastError().text();
        return false;
    }

    const bool ok = Migracion::aplicarPaso(db)
                 && fijarVersionEsquema(db, VERSION_ESQUEMA_ACTUAL);

    if (!ok)
    {
        db.rollback();
        qCritical() << "La creación del esquema falló y se revirtió: no quedó esquema parcial. "
                       "El archivo puede descartarse y crearse de nuevo (con respaldo "
                       "obligatorio).";
        return false;
    }

    if (!db.commit())
    {
        db.rollback();
        qCritical() << "No se pudo confirmar la creacion del esquema:" << db.lastError().text();
        return false;
    }

    return true;
}

bool VersionEsquema::esquemaEsperado(QSqlDatabase& db, QString* motivo)
{
    // P5: toda la comprobación —tablas/columnas, objetos y fila— en una sola instantánea.
    // Si ya hay una transacción abierta (`transaction()` devuelve false), se usa esa y no se
    // cierra; la instantánea la da la externa. La comprobación no escribe nunca.
    const bool transaccionPropia = db.transaction();

    const bool ok = [&db, motivo]() -> bool
    {
        QStringList faltantes;

        for (const QString& tabla : TABLAS_ESPERADAS)
            if (!tablaExiste(db, tabla))
                faltantes << tabla;

        // Columnas que faltan, agrupadas por tabla (el motivo nombra tabla y columna).
        QStringList piezasColumnas;
        for (const TablaConColumnas& entrada : TABLAS_CON_COLUMNAS)
        {
            const QString tabla = QString::fromLatin1(entrada.tabla);
            if (!tablaExiste(db, tabla))
                continue; // ya figura como tabla faltante

            QStringList columnasFaltantes;
            for (const QString& columna : *entrada.columnas)
                if (!columnaExiste(db, tabla, columna))
                    columnasFaltantes << columna;

            if (!columnasFaltantes.isEmpty())
                piezasColumnas << QStringLiteral("faltan las columnas «%1» de la tabla «%2»")
                                      .arg(columnasFaltantes.join(QStringLiteral("», «")), tabla);
        }

        QStringList retiradasPresentes;
        for (const QString& retirada : TABLAS_RETIRADAS)
            if (tablaExiste(db, retirada))
                retiradasPresentes << retirada;

        // Objetos exigidos por nombre: índice único del catálogo y los dos disparadores.
        // No se comparan cuerpos ni se exigen los `sqlite_autoindex_*`; los objetos de más
        // se toleran (salvo las tablas retiradas, ya comprobadas).
        QStringList objetosFaltantes;
        for (const QString& indice : INDICES_ESPERADOS)
            if (!objetoExiste(db, QStringLiteral("index"), indice))
                objetosFaltantes << QStringLiteral("índice %1").arg(indice);
        for (const QString& disparador : DISPARADORES_ESPERADOS)
            if (!objetoExiste(db, QStringLiteral("trigger"), disparador))
                objetosFaltantes << QStringLiteral("disparador %1").arg(disparador);

        // Fila del tipo general: solo se evalúa si la tabla está y tiene `nombre` y `clave`.
        // Si falta una columna ya se informa de ella y evaluar la fila taparía el motivo real.
        bool filaValida = true;
        QString motivoFila;
        if (tablaExiste(db, QStringLiteral("TiposAula"))
            && columnaExiste(db, QStringLiteral("TiposAula"), QStringLiteral("nombre"))
            && columnaExiste(db, QStringLiteral("TiposAula"), QStringLiteral("clave")))
        {
            QSqlQuery query(db);
            if (!query.exec(QStringLiteral("SELECT id, nombre, clave FROM TiposAula ORDER BY id")))
            {
                filaValida = false;
                motivoFila = QStringLiteral("no se pudo leer el catálogo de tipos de aula");
            }
            else
            {
                QVector<tipo_general::Fila> filas;
                while (query.next())
                    filas.append(tipo_general::Fila{query.value(0).toInt(),
                                                    query.value(1).toString(),
                                                    query.value(2).toString()});

                filaValida = tipo_general::evaluar(filas, &motivoFila)
                             == tipo_general::EstadoFila::Valida;
            }
        }

        const bool correcto = faltantes.isEmpty() && piezasColumnas.isEmpty()
                              && retiradasPresentes.isEmpty() && objetosFaltantes.isEmpty()
                              && filaValida;
        if (correcto)
        {
            if (motivo)
                motivo->clear();
            return true;
        }

        if (motivo)
        {
            QStringList piezas;

            if (!faltantes.isEmpty())
                piezas << QStringLiteral("faltan las tablas «%1»")
                              .arg(faltantes.join(QStringLiteral("», «")));

            piezas << piezasColumnas;

            if (!retiradasPresentes.isEmpty())
                piezas << QStringLiteral("existe la tabla retirada «%1»")
                              .arg(retiradasPresentes.join(QStringLiteral("», «")));

            if (!objetosFaltantes.isEmpty())
                piezas << QStringLiteral("faltan los objetos «%1»")
                              .arg(objetosFaltantes.join(QStringLiteral("», «")));

            if (!filaValida)
                piezas << (motivoFila.isEmpty()
                               ? QStringLiteral("la fila del tipo general no es válida")
                               : motivoFila);

            *motivo = piezas.join(QStringLiteral("; "));
        }

        return false;
    }();

    if (transaccionPropia && !db.commit())
    {
        if (motivo)
            *motivo = QStringLiteral("no se pudo cerrar la comprobación del esquema");
        return false;
    }

    return ok;
}
