#include "seed/docentes.hpp"

namespace seed {

namespace {

/// Nombres ficticios con acentos y eñes (RF-6, RNF-4): combinación determinista de 20 nombres
/// y 20 apellidos. No corresponden a personas reales.
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

/// Identificador legible y estable (RF-11): P001, P002, …
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

/// Disponibilidad completa: lunes a viernes 07:00–18:00 (5 filas; cubre bloques de ambos turnos).
QVector<FranjaDef> disponibilidadCompleta() {
    QVector<FranjaDef> franjas;
    for (int dia = 1; dia <= 5; ++dia) {
        franjas.append(FranjaDef{dia, QTime(7, 0), QTime(18, 0)});
    }
    return franjas;
}

/// Solo mañana: 07:00–12:20 (rejilla aritmética de la mañana, 8 bloques).
QVector<FranjaDef> disponibilidadManana() {
    QVector<FranjaDef> franjas;
    for (int dia = 1; dia <= 5; ++dia) {
        franjas.append(FranjaDef{dia, QTime(7, 0), QTime(12, 20)});
    }
    return franjas;
}

/// Solo tarde: 12:40–18:00 (rejilla aritmética de la tarde, 8 bloques).
QVector<FranjaDef> disponibilidadTarde() {
    QVector<FranjaDef> franjas;
    for (int dia = 1; dia <= 5; ++dia) {
        franjas.append(FranjaDef{dia, QTime(12, 40), QTime(18, 0)});
    }
    return franjas;
}

/// Fuera de ambos turnos: 19:00–21:00 (solape nulo, RF-8).
QVector<FranjaDef> disponibilidadNoche() {
    QVector<FranjaDef> franjas;
    for (int dia = 1; dia <= 5; ++dia) {
        franjas.append(FranjaDef{dia, QTime(19, 0), QTime(21, 0)});
    }
    return franjas;
}

/// Tope de un titular de suplentes: `ceil(carga / 1,15)` en aritmética entera
/// (`ceil(carga × 100 / 115)`), sin coma flotante.
int topeSuplentes(int carga) {
    return (carga * 100 + 114) / 115;
}

/// Docente con identidad/contacto por índice, materias y tope dados, disponibilidad completa.
DocenteDef docenteBase(int indice, const QStringList& materias, int horasAula) {
    DocenteDef d;
    d.id = idDocente(indice);
    d.nombre = nombreDocente(indice);
    d.email = emailDocente(indice);
    d.telefono = telefonoDocente(indice);
    d.materias = materias;
    d.disponibilidad = disponibilidadCompleta();
    d.horasAula = horasAula;
    return d;
}

/// Índices del catálogo de los 8 docentes parciales de la holgada (plan §4.4).
constexpr int kParciales[] = {0, 7, 14, 21, 28, 35, 42, 49};

/// ¿Contiene algún rango de `disponibilidad` un bloque completo del turno (fronteras
/// `inicio + k×40`)? El receso no desplaza los bloques (RF-4).
bool solapaBloqueCompleto(const QVector<FranjaDef>& disponibilidad, const TurnoDef& turno) {
    for (int k = 0; k < turno.numSlots; ++k) {
        const QTime inicio = turno.inicio.addSecs(k * 40 * 60);
        const QTime fin = inicio.addSecs(40 * 60);
        for (const FranjaDef& franja : disponibilidad) {
            if (franja.inicio <= inicio && franja.fin >= fin) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

QVector<DocenteDef> docentesDeVariante(const Perfil& perfil, VarianteDocentes variante) {
    switch (variante) {
    case VarianteDocentes::Holgada:
    case VarianteDocentes::SinProfesorado:
    case VarianteDocentes::SinDisponibilidad: {
        QVector<DocenteDef> docentes;
        // P001…P057: una materia cada uno (catálogo en orden), tope 30.
        for (int i = 0; i < perfil.materias.size(); ++i) {
            docentes.append(docenteBase(i, QStringList{perfil.materias[i].nombre}, 30));
        }
        // P058…P065: parciales (materias con doble titular), disponibilidad de un solo turno.
        for (int k = 0; k < 8; ++k) {
            const int indice = perfil.materias.size() + k;
            DocenteDef d =
                docenteBase(indice, QStringList{perfil.materias[kParciales[k]].nombre}, 30);
            d.disponibilidad = (k % 2 == 0) ? disponibilidadManana() : disponibilidadTarde();
            docentes.append(d);
        }
        if (variante == VarianteDocentes::SinProfesorado) {
            docentes.remove(22);  // P023 = «Física 3º» queda sin docente (RF-8).
        } else if (variante == VarianteDocentes::SinDisponibilidad) {
            docentes[22].disponibilidad = disponibilidadNoche();  // solape nulo en ambos turnos.
        }
        return docentes;
    }
    case VarianteDocentes::Suplentes: {
        QVector<DocenteDef> docentes;
        const int nTitulares = perfil.materias.size() / 3;  // 19 (57 materias, 3 por titular).
        // P001…P019: titulares con tres materias (i, i+19, i+38); tope = ceil(carga / 1,15).
        for (int i = 0; i < nTitulares; ++i) {
            const QStringList materias{
                perfil.materias[i].nombre,
                perfil.materias[i + nTitulares].nombre,
                perfil.materias[i + 2 * nTitulares].nombre,
            };
            const int carga = cargaAsignada(perfil, materias);
            docentes.append(docenteBase(i, materias, topeSuplentes(carga)));
        }
        // P020…P043: 24 suplentes sin materias, tope 8.
        for (int i = nTitulares; i < nTitulares + 24; ++i) {
            docentes.append(docenteBase(i, QStringList{}, 8));
        }
        return docentes;
    }
    }
    return {};
}

Recuentos recuentosEsperados(const Perfil& perfil, VarianteDocentes variante) {
    Recuentos r;
    r.turnos       = perfil.turnos.size();
    r.eventos      = 0;
    for (const TurnoDef& turno : perfil.turnos) {
        r.eventos += turno.eventos.size();  // un evento «Receso» por turno (RF-10).
    }
    r.tiposAula    = perfil.tiposAula.size();
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

    const QVector<DocenteDef> docentes = docentesDeVariante(perfil, variante);
    r.docentes         = docentes.size();
    r.asignaciones     = 0;
    r.disponibilidades = 0;
    for (const DocenteDef& docente : docentes) {
        r.asignaciones += docente.materias.size();
        r.disponibilidades += docente.disponibilidad.size();
    }
    return r;
}

bool coberturaDeVariante(const Perfil& perfil, VarianteDocentes variante, QString* error) {
    const auto falla = [&](const QString& mensaje) {
        if (error) {
            *error = mensaje;
        }
        return false;
    };

    const QVector<DocenteDef> docentes = docentesDeVariante(perfil, variante);
    for (const MateriaAnio& materia : perfil.materias) {
        QVector<const DocenteDef*> asignados;
        for (const DocenteDef& docente : docentes) {
            if (docente.materias.contains(materia.nombre)) {
                asignados.append(&docente);
            }
        }
        if (asignados.isEmpty()) {
            return falla(QStringLiteral("La materia «%1» no tiene ningún docente asignado.")
                             .arg(materia.nombre));
        }
        for (const TurnoDef& turno : perfil.turnos) {
            bool cubierto = false;
            for (const DocenteDef* docente : asignados) {
                if (solapaBloqueCompleto(docente->disponibilidad, turno)) {
                    cubierto = true;
                    break;
                }
            }
            if (!cubierto) {
                return falla(
                    QStringLiteral("La materia «%1» no tiene ningún docente con disponibilidad que "
                                   "solape una franja completa del turno «%2».")
                        .arg(materia.nombre, turno.nombre));
            }
        }
    }
    return true;
}

bool docentesCumplenCarga(const Perfil& perfil, VarianteDocentes variante, QString* error) {
    const auto falla = [&](const QString& mensaje) {
        if (error) {
            *error = mensaje;
        }
        return false;
    };

    const QVector<DocenteDef> docentes = docentesDeVariante(perfil, variante);

    if (variante == VarianteDocentes::Suplentes) {
        int sumaTopes = 0;
        for (const DocenteDef& docente : docentes) {
            if (docente.materias.isEmpty()) {
                continue;  // suplente sin materias: no entra en la carga de los titulares.
            }
            const int carga = cargaAsignada(perfil, docente.materias);
            if (carga * 10 < 8 * docente.horasAula) {
                return falla(QStringLiteral("El titular %1 no alcanza el 80 por ciento de su tope "
                                            "(carga %2, tope %3).")
                                 .arg(docente.id)
                                 .arg(carga)
                                 .arg(docente.horasAula));
            }
            if (carga < docente.horasAula || carga * 100 > 115 * docente.horasAula) {
                return falla(QStringLiteral("El ratio carga/tope del titular %1 (%2/%3) sale del "
                                            "intervalo [1,00, 1,15].")
                                 .arg(docente.id)
                                 .arg(carga)
                                 .arg(docente.horasAula));
            }
            sumaTopes += docente.horasAula;
        }
        const int demanda = demandaTotal(perfil);
        if (sumaTopes * 100 < 85 * demanda || sumaTopes * 100 > 90 * demanda) {
            return falla(QStringLiteral("La suma de topes de los titulares (%1) no queda entre el "
                                        "85 y el 90 por ciento de la demanda (%2).")
                             .arg(sumaTopes)
                             .arg(demanda));
        }
        return true;
    }

    // Holgada y las inválidas (heredan la relación de carga de la holgada, RF-6/RF-8).
    for (const DocenteDef& docente : docentes) {
        const int carga = cargaAsignada(perfil, docente.materias);
        if (carga * 10 > 6 * docente.horasAula) {
            return falla(QStringLiteral("El docente %1 supera el 60 por ciento de su tope "
                                        "(carga %2, tope %3).")
                             .arg(docente.id)
                             .arg(carga)
                             .arg(docente.horasAula));
        }
    }
    return true;
}

QString nombreVariante(VarianteDocentes variante) {
    switch (variante) {
    case VarianteDocentes::Holgada:
        return QStringLiteral("holgada");
    case VarianteDocentes::Suplentes:
        return QStringLiteral("suplentes");
    case VarianteDocentes::SinProfesorado:
        return QStringLiteral("sin profesorado");
    case VarianteDocentes::SinDisponibilidad:
        return QStringLiteral("sin disponibilidad");
    }
    return {};
}

std::optional<VarianteDocentes> varianteDesdeTexto(const QString& texto) {
    const QString t = texto.trimmed().toLower();
    if (t == QStringLiteral("holgada")) {
        return VarianteDocentes::Holgada;
    }
    if (t == QStringLiteral("suplentes")) {
        return VarianteDocentes::Suplentes;
    }
    if (t == QStringLiteral("sin-profesorado") || t == QStringLiteral("sin profesorado")) {
        return VarianteDocentes::SinProfesorado;
    }
    if (t == QStringLiteral("sin-disponibilidad") || t == QStringLiteral("sin disponibilidad")) {
        return VarianteDocentes::SinDisponibilidad;
    }
    return std::nullopt;
}

} // namespace seed
