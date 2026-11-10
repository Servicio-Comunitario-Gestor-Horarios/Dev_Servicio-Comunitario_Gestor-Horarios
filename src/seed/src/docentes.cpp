#include "seed/docentes.hpp"

#include <QHash>
#include <QSet>

namespace seed {

namespace {

/// Nombres ficticios con acentos y eñes (RNF-6): combinación determinista de 20 nombres y
/// 20 apellidos. No corresponden a personas reales del centro.
const char* const kNombres[] = {
    "Ana",     "José",   "María",   "Jesús",   "Lucía",   "Carlos",  "Andrés",
    "Dolores", "Víctor", "Inés",    "Ramón",   "Ángela",  "Félix",   "Pilar",
    "Raúl",    "Catalina", "Tomás", "Mercedes", "Gonzalo", "Úrsula",
};
const char* const kApellidos[] = {
    "García",   "Ñáñez",     "Pérez",     "Muñoz",     "López",      "Vásquez",
    "Rodríguez", "Giménez",  "Hernández", "Benítez",   "Álvarez",    "Cárdenas",
    "Domínguez", "Ibáñez",   "Núñez",     "Téllez",    "Frías",      "Alarcón",
    "Berríos",   "Córdova",
};
constexpr int kNumNombres = 20;
constexpr int kNumApellidos = 20;

/// Tope de horas académicas de aula de todos los docentes (D-3): las 40 h del centro
/// incluyen actividades «fuera de aula» y se descuentan.
constexpr int kTopeHorasAula = 36;

/// Tamaño y total de asignaciones del conjunto (plan §4.6/§4.7, ampliado por la 018).
constexpr int kNumDocentes = 33;
constexpr int kAsignacionesEsperadas = 272;

/// Docentes por turno (018, T4b): la tarde suma el docente de apoyo de idiomas (P033), porque
/// P028 (Inglés e Idiomas) acumulaba 48 clases frente a las 40 franjas de un turno.
constexpr int kDocentesManana = 16;
constexpr int kDocentesTarde = 17;

/// Identificador legible y estable: P001, P002, …
QString idDocente(int indice) {
    return QStringLiteral("P%1").arg(indice + 1, 3, 10, QChar('0'));
}

QString nombreDocente(int indice) {
    return QStringLiteral("%1 %2")
        .arg(QString::fromUtf8(kNombres[indice % kNumNombres]))
        .arg(QString::fromUtf8(kApellidos[(indice / kNumNombres) % kNumApellidos]));
}

QString emailDocente(int indice) {
    return QStringLiteral("p%1@centro-de-prueba.edu.ve").arg(indice + 1, 3, 10, QChar('0'));
}

QString telefonoDocente(int indice) {
    return QStringLiteral("+58 400-000-%1").arg(indice + 1, 4, 10, QChar('0'));
}

/// Una posición del documento del centro (plan §4.6): turno, áreas declaradas con el nombre
/// exacto del catálogo y si es uno de los 2 docentes añadidos de D-4.
struct PosicionDocente {
    const char* turno;
    int numAreas;
    const char* areas[4];
    bool anadido;
};

const char* const kManana = "Mañana";
const char* const kTarde = "Tarde";

const PosicionDocente kPosiciones[] = {
    // ── Turno de la mañana: 15 posiciones del centro (P001…P015) ──────────────────────
    {kManana, 2, {"Matemáticas", "Química"}, false},
    {kManana, 2, {"Castellano", "Orientación y Convivencia"}, false},
    {kManana, 2, {"Física", "Ciencias Naturales"}, false},
    {kManana, 2, {"Química", "Ciencias Naturales"}, false},
    {kManana, 3,
     {"Geografía, Historia y Ciudadanía", "Formación para la Soberanía Nacional",
      "Orientación y Convivencia"},
     false},
    {kManana, 1, {"Geografía, Historia y Ciudadanía"}, false},
    {kManana, 1, {"Castellano"}, false},
    {kManana, 2, {"Educación Física", "Orientación y Convivencia"}, false},
    {kManana, 2, {"Matemáticas", "Orientación y Convivencia"}, false},
    {kManana, 2, {"Arte y Patrimonio", "Geografía, Historia y Ciudadanía"}, false},
    {kManana, 3, {"Educación Física", "Castellano", "Orientación y Convivencia"}, false},
    {kManana, 3, {"Biología", "Biología, Ambiente y Tecnología", "Física"}, false},
    {kManana, 4,
     {"Inglés y otras lenguas extranjeras", "Castellano", "Biología",
      "Biología, Ambiente y Tecnología"},
     false},
    {kManana, 2, {"Inglés y otras lenguas extranjeras", "Orientación y Convivencia"}, false},
    {kManana, 4,
     {"Biología", "Biología, Ambiente y Tecnología", "Inglés y otras lenguas extranjeras",
      "Orientación y Convivencia"},
     false},

    // ── Turno de la tarde: 15 posiciones del centro (P016…P030) ───────────────────────
    {kTarde, 2, {"Orientación y Convivencia", "Orientación Vocacional"}, false},
    {kTarde, 2, {"Química", "Física"}, false},
    {kTarde, 1, {"Educación Física"}, false},
    {kTarde, 2, {"Geografía, Historia y Ciudadanía", "Geografía, Historia y Soberanía Nacional"},
     false},
    {kTarde, 3,
     {"Ciencias Naturales", "Geografía, Historia y Ciudadanía", "Orientación Vocacional"},
     false},
    {kTarde, 1, {"Matemáticas"}, false},
    {kTarde, 3,
     {"Geografía, Historia y Ciudadanía", "Arte y Patrimonio",
      "Geografía, Historia y Soberanía Nacional"},
     false},
    {kTarde, 1, {"Lengua y Literatura"}, false},
    {kTarde, 2, {"Educación Física", "Innovación Tecnológica y Productividad"}, false},
    {kTarde, 1, {"Innovación Tecnológica y Productividad"}, false},
    {kTarde, 1, {"Orientación Vocacional"}, false},
    {kTarde, 2, {"Castellano", "Lengua y Literatura"}, false},
    {kTarde, 1, {"Inglés y otras lenguas extranjeras"}, false},
    {kTarde, 2, {"Biología", "Biología, Ambiente y Tecnología"}, false},
    {kTarde, 2, {"Matemáticas", "Física"}, false},

    // ── Docentes añadidos para las materias sin carga de D-4 (P031 y P032) ─────────────
    {kManana, 1, {"Participación en los Grupos de Creación, Recreación y Producción"}, true},
    {kTarde, 1, {"Ciencias de la Tierra"}, true},

    // ── Docente de apoyo de idiomas (018, T4b: P033) ───────────────────────────────────
    // P028 se queda con Inglés (12 clases de 31059 2º) y este docente recibe Idiomas (36
    // clases de 31060); juntos no superan las 40 franjas del turno ni las 36 h de aula.
    {kTarde, 1, {"Idiomas"}, false},
};

/// Turno del perfil con ese nombre, o nullptr.
const TurnoDef* turnoPorNombre(const Perfil& perfil, const QString& nombre) {
    for (const TurnoDef& turno : perfil.turnos) {
        if (turno.nombre == nombre) {
            return &turno;
        }
    }
    return nullptr;
}

/// Materias-año del catálogo que recibe una posición (plan §5.4): todas las del área, en el
/// orden del catálogo, comparando el nombre del área de forma **exacta** (sin expandir
/// «Biología» a «Biología, Ambiente y Tecnología») y solo si el área tiene cursos en el turno
/// del docente. Las posiciones añadidas (D-4) no dependen del turno: sus materias no tienen
/// carga en ningún plan.
QStringList materiasDeDocente(const Perfil& perfil, const QStringList& areas,
                              const QString& turno, bool anadido) {
    QStringList salida;
    for (const MateriaAnio& materia : perfil.materias) {
        if (!areas.contains(materia.area)) {
            continue;
        }
        if (anadido || areaConCursosEnTurno(perfil, materia.area, turno)) {
            salida << materia.nombre;
        }
    }
    return salida;
}

/// Disponibilidad del turno (RF-6, RF-7): un rango por día, de lunes a viernes, con la
/// ventana completa del turno en reloj (07:00–12:40 o 12:40–18:10).
QVector<FranjaDef> disponibilidadDelTurno(const TurnoDef& turno) {
    QVector<FranjaDef> franjas;
    for (int dia = 1; dia <= 5; ++dia) {
        franjas.append(FranjaDef{dia, turno.inicio, turno.fin});
    }
    return franjas;
}

/// Plan del perfil con ese código, o nullptr.
const PlanDef* planPorCodigo(const Perfil& perfil, const QString& codigo) {
    for (const PlanDef& plan : perfil.planes) {
        if (plan.codigo == codigo) {
            return &plan;
        }
    }
    return nullptr;
}

/// Estado de un docente cargado para los mensajes de error.
QString descripcion(const DocenteDef& docente) {
    return QStringLiteral("%1 («%2»)").arg(docente.id, docente.nombre);
}

} // namespace

QVector<DocenteDef> docentesDelCentro(const Perfil& perfil) {
    QVector<DocenteDef> docentes;
    docentes.reserve(kNumDocentes);

    int indice = 0;
    for (const PosicionDocente& posicion : kPosiciones) {
        QStringList areas;
        areas.reserve(posicion.numAreas);
        for (int i = 0; i < posicion.numAreas; ++i) {
            areas << QString::fromUtf8(posicion.areas[i]);
        }

        DocenteDef docente;
        docente.id = idDocente(indice);
        docente.nombre = nombreDocente(indice);
        docente.email = emailDocente(indice);
        docente.telefono = telefonoDocente(indice);
        docente.turno = QString::fromUtf8(posicion.turno);
        docente.materias = materiasDeDocente(perfil, areas, docente.turno, posicion.anadido);

        if (const TurnoDef* turno = turnoPorNombre(perfil, docente.turno)) {
            docente.disponibilidad = disponibilidadDelTurno(*turno);
        }
        docente.horasAula = kTopeHorasAula;

        docentes.append(docente);
        ++indice;
    }
    return docentes;
}

Recuentos recuentosEsperados(const Perfil& perfil) {
    Recuentos r;
    r.turnos       = perfil.turnos.size();
    r.eventos      = 0;
    for (const TurnoDef& turno : perfil.turnos) {
        r.eventos += turno.eventos.size();
    }
    r.tiposAula    = perfil.tiposAula.size() + 1; // + el tipo general que garantiza el esquema
    r.aulas        = perfil.aulas.size();
    r.materias     = perfil.materias.size();
    r.planes       = perfil.planes.size();
    r.planMaterias = 0;
    for (const PlanDef& plan : perfil.planes) {
        r.planMaterias += plan.cargas.size();
    }
    r.cursos = perfil.cursos.size();

    // El conjunto no siembra preferencias (spec 012, RF-7): el recuento esperado es 0.
    r.preferencias = 0;

    const QVector<DocenteDef> docentes = docentesDelCentro(perfil);
    r.docentes         = docentes.size();
    r.asignaciones     = 0;
    r.disponibilidades = 0;
    for (const DocenteDef& docente : docentes) {
        r.asignaciones += docente.materias.size();
        r.disponibilidades += docente.disponibilidad.size();
    }

    // Duración de la hora académica y su fila de configuración (RF-7): la declara el perfil
    // y el sembrador la escribe en `ConfiguracionHoraria` (una fila).
    r.duracionMinutos = perfil.duracionMinutos;
    r.configuraciones = 1;
    return r;
}

bool validarDocentes(const Perfil& perfil, const QVector<DocenteDef>& docentes, QString* error) {
    const auto falla = [&](const QString& mensaje) {
        if (error) {
            *error = mensaje;
        }
        return false;
    };

    if (docentes.size() != kNumDocentes) {
        return falla(QStringLiteral("El conjunto debe tener 33 docentes (tiene %1).")
                         .arg(docentes.size()));
    }

    QSet<QString> materiasCatalogo;
    for (const MateriaAnio& materia : perfil.materias) {
        materiasCatalogo.insert(materia.nombre);
    }

    QSet<QString> ids;
    QSet<QString> nombres;
    QSet<QString> emails;
    QHash<QString, int> porTurno;
    int asignaciones = 0;

    for (const DocenteDef& docente : docentes) {
        if (docente.id.trimmed().isEmpty() || docente.nombre.trimmed().isEmpty() ||
            docente.email.trimmed().isEmpty()) {
            return falla(QStringLiteral("Un docente del conjunto no declara identidad completa."));
        }
        if (ids.contains(docente.id)) {
            return falla(QStringLiteral("El identificador de docente «%1» está duplicado.")
                             .arg(docente.id));
        }
        ids.insert(docente.id);
        if (nombres.contains(docente.nombre)) {
            return falla(QStringLiteral("El docente «%1» está duplicado por nombre.")
                             .arg(docente.nombre));
        }
        nombres.insert(docente.nombre);
        if (emails.contains(docente.email)) {
            return falla(QStringLiteral("El correo «%1» del docente %2 está duplicado.")
                             .arg(docente.email, docente.id));
        }
        emails.insert(docente.email);

        if (docente.horasAula != kTopeHorasAula) {
            return falla(QStringLiteral("El docente %1 declara %2 horas de aula; el tope del "
                                        "centro es %3.")
                             .arg(descripcion(docente))
                             .arg(docente.horasAula)
                             .arg(kTopeHorasAula));
        }

        const TurnoDef* turno = turnoPorNombre(perfil, docente.turno);
        if (turno == nullptr) {
            return falla(QStringLiteral("El docente %1 declara el turno «%2», que no existe.")
                             .arg(descripcion(docente), docente.turno));
        }
        ++porTurno[docente.turno];

        if (docente.disponibilidad.size() != 5) {
            return falla(QStringLiteral("El docente %1 tiene %2 filas de disponibilidad (se "
                                        "esperan 5, una por día).")
                             .arg(descripcion(docente))
                             .arg(docente.disponibilidad.size()));
        }
        QSet<int> dias;
        for (const FranjaDef& franja : docente.disponibilidad) {
            if (franja.dia < 1 || franja.dia > 5) {
                return falla(QStringLiteral("El docente %1 tiene disponibilidad en el día %2 "
                                            "(debe estar entre 1 y 5).")
                                 .arg(descripcion(docente))
                                 .arg(franja.dia));
            }
            if (dias.contains(franja.dia)) {
                return falla(QStringLiteral("El docente %1 repite el día %2 en su "
                                            "disponibilidad.")
                                 .arg(descripcion(docente))
                                 .arg(franja.dia));
            }
            dias.insert(franja.dia);
            if (franja.inicio >= franja.fin) {
                return falla(QStringLiteral("La franja del día %1 del docente %2 no declara un "
                                            "rango válido.")
                                 .arg(franja.dia)
                                 .arg(descripcion(docente)));
            }
            if (franja.inicio < turno->inicio || franja.fin > turno->fin) {
                return falla(QStringLiteral("La disponibilidad del día %1 del docente %2 "
                                            "(%3–%4) sale de la ventana del turno «%5» (%6–%7).")
                                 .arg(franja.dia)
                                 .arg(descripcion(docente))
                                 .arg(franja.inicio.toString(QStringLiteral("HH:mm")),
                                      franja.fin.toString(QStringLiteral("HH:mm")),
                                      turno->nombre,
                                      turno->inicio.toString(QStringLiteral("HH:mm")),
                                      turno->fin.toString(QStringLiteral("HH:mm"))));
            }
        }
        if (dias.size() != 5) {
            return falla(QStringLiteral("El docente %1 no cubre los cinco días de lunes a "
                                        "viernes.")
                             .arg(descripcion(docente)));
        }

        QSet<QString> vistas;
        for (const QString& materia : docente.materias) {
            if (!materiasCatalogo.contains(materia)) {
                return falla(QStringLiteral("El docente %1 tiene asignada la materia «%2», que "
                                            "no está en el catálogo.")
                                 .arg(descripcion(docente), materia));
            }
            if (vistas.contains(materia)) {
                return falla(QStringLiteral("El docente %1 repite la materia «%2».")
                                 .arg(descripcion(docente), materia));
            }
            vistas.insert(materia);
            ++asignaciones;
        }
    }

    // 15 posiciones del centro más 1 añadido en cada turno (D-13, D-4) y el apoyo de idiomas
    // de la tarde (018, T4b): 16 en la mañana y 17 en la tarde.
    for (const TurnoDef& turno : perfil.turnos) {
        int esperados = 0;
        if (turno.nombre == QString::fromUtf8(kManana)) {
            esperados = kDocentesManana;
        } else if (turno.nombre == QString::fromUtf8(kTarde)) {
            esperados = kDocentesTarde;
        }
        if (porTurno.value(turno.nombre) != esperados) {
            return falla(QStringLiteral("El turno «%1» tiene %2 docentes (se esperan %3).")
                             .arg(turno.nombre)
                             .arg(porTurno.value(turno.nombre))
                             .arg(esperados));
        }
    }

    if (asignaciones != kAsignacionesEsperadas) {
        return falla(QStringLiteral("El conjunto tiene %1 materias asignadas; se esperaban %2.")
                         .arg(asignaciones)
                         .arg(kAsignacionesEsperadas));
    }
    return true;
}

bool coberturaDelCentro(const Perfil& perfil, const QVector<DocenteDef>& docentes,
                        QString* error) {
    const auto falla = [&](const QString& mensaje) {
        if (error) {
            *error = mensaje;
        }
        return false;
    };

    // Cada materia del catálogo (incluidas las 6 sin carga) tiene al menos un docente.
    for (const MateriaAnio& materia : perfil.materias) {
        bool hay = false;
        for (const DocenteDef& docente : docentes) {
            if (docente.materias.contains(materia.nombre)) {
                hay = true;
                break;
            }
        }
        if (!hay) {
            return falla(QStringLiteral("La materia «%1» no tiene ningún docente asignado.")
                             .arg(materia.nombre));
        }
    }

    // Cada materia de cada curso tiene docente habilitado en el turno del curso (RF-6).
    for (const CursoDef& curso : perfil.cursos) {
        const PlanDef* plan = planPorCodigo(perfil, curso.plan);
        if (plan == nullptr) {
            return falla(QStringLiteral("El curso %1º%2 declara el plan «%3», que no existe.")
                             .arg(curso.anio)
                             .arg(curso.seccion)
                             .arg(curso.plan));
        }
        for (const CargaPlan& carga : plan->cargas) {
            if (carga.anio != curso.anio) {
                continue;
            }
            bool cubierto = false;
            for (const DocenteDef& docente : docentes) {
                if (docente.turno == curso.turno &&
                    docente.materias.contains(carga.materia)) {
                    cubierto = true;
                    break;
                }
            }
            if (!cubierto) {
                return falla(QStringLiteral("La materia «%1» del curso %2º%3 no tiene docente "
                                            "habilitado en el turno «%4».")
                                 .arg(carga.materia)
                                 .arg(curso.anio)
                                 .arg(curso.seccion)
                                 .arg(curso.turno));
            }
        }
    }
    return true;
}

} // namespace seed
