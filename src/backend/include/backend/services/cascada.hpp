#pragma once

/**
 * @file cascada.hpp
 * @brief Descripción de dependencias para la eliminación en cascada (RF-2).
 *
 * La lógica de "qué depende de qué" es pura y determinista: describe las
 * relaciones del esquema sin tocar la base de datos. `NucleoDatos` usa esa
 * descripción tanto para listar los dependientes (para que el frontend pida
 * confirmación explícita) como para borrarlos en una sola transacción.
 */

#include <QString>
#include <QVector>

/// Un registro dependiente que se eliminaría en cascada (RF-2).
struct Dependencia {
    QString dominio;      ///< dominio lógico del dependiente ("curso", "curso_materia", …)
    qint64 id = 0;        ///< identificador del registro dentro de su tabla
    QString descripcion;  ///< texto en español para el diálogo de confirmación
};

/// Relación directa de un dominio padre con las filas que dependen de él.
struct RelacionCascada {
    QString dominio;        ///< dominio lógico del dependiente
    QString tabla;          ///< tabla física donde viven las filas dependientes
    QString columna;        ///< columna que referencia al dominio padre
    QString columnaId;      ///< columna que aporta el id del dependiente (vacío → rowid)
    QString etiqueta;       ///< columna de texto para describir el dependiente (opcional)
    QString descripcion;    ///< descripción base en español
    bool esDominio = false; ///< el dependiente es a su vez un dominio con cascada propia
};

/// Tabla y columna clave de un dominio raíz.
struct ClaveDominio {
    QString tabla;
    QString columna;
};

/**
 * @brief Función pura: relaciones de dependencia directas de un dominio (RF-2).
 *
 * No consulta la base de datos. Un dominio desconocido devuelve un vector vacío.
 */
QVector<RelacionCascada> dependenciasDe(const QString& dominio);

/// Función pura: indica si `dominio` es un dominio con cascada conocido.
bool dominioValido(const QString& dominio);

/// Función pura: tabla y columna clave de un dominio raíz.
ClaveDominio claveDeDominio(const QString& dominio);
