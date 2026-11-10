#include "seed/perfil.hpp"

#include <QHash>
#include <QSet>

#include "backend/services/calculo_eventos.hpp"
#include "backend/services/duracion_horaria.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "common/aforo_aula.hpp"
#include "common/tipo_general.hpp"

namespace seed {

int bloques(int horas) {
    // Rectificación (2026-10-17, constitución §7 / spec 005-T11): la hora académica
    // es un bloque, no una hora de reloj. La conversión es la IDENTIDAD:
    // bloques(horas) = horas. La duración real de la franja es la de la configuración
    // global (40 min en este conjunto, spec 011/017) y no interviene en la conversión.
    return horas;
}

int horasDeAnio(const PlanDef& plan, int anio) {
    int total = 0;
    bool tieneAnio = false;
    for (const CargaPlan& carga : plan.cargas) {
        if (carga.anio == anio) {
            total += carga.horas;
            tieneAnio = true;
        }
    }
    return tieneAnio ? total : -1;
}

int bloquesDeAnio(const PlanDef& plan, int anio) {
    // bloques() es la identidad, así que bloques(-1) = -1 conserva el «sin año».
    return bloques(horasDeAnio(plan, anio));
}

namespace {

/// Área de formación de un plan con sus horas académicas semanales por año
/// (índice 0 = 1.º, … 4 = 5.º; -1 = no se cursa en ese año).
struct AreaPlan {
    const char* nombre;
    int horas[5];
};

/// Las 12 áreas del 31059 (Media General) en su orden oficial, tras excluir de los planes
/// «Participación en los Grupos…» y «Ciencias de la Tierra» (D-4, plan §4.1). Suman
/// 34/34/34/36/34 horas y 8/8/9/10/10 áreas por año.
const AreaPlan kAreas31059[] = {
    {"Castellano", {4, 4, 4, 4, 3}},
    {"Inglés y otras lenguas extranjeras", {4, 4, 4, 4, 3}},
    {"Matemáticas", {4, 4, 4, 4, 4}},
    {"Educación Física", {4, 4, 4, 4, 4}},
    {"Arte y Patrimonio", {4, 4, -1, -1, -1}},
    {"Ciencias Naturales", {6, 6, -1, -1, -1}},
    {"Física", {-1, -1, 4, 4, 4}},
    {"Química", {-1, -1, 4, 4, 4}},
    {"Biología", {-1, -1, 4, 4, 4}},
    {"Geografía, Historia y Ciudadanía", {6, 6, 4, 4, 4}},
    {"Formación para la Soberanía Nacional", {-1, -1, -1, 2, 2}},
    {"Orientación y Convivencia", {2, 2, 2, 2, 2}},
};

/// Las 10 áreas del 31060 (Ciencias y Tecnología): 40 horas académicas cada año y
/// 10 áreas por año (plan §4.1).
const AreaPlan kAreas31060[] = {
    {"Lengua y Literatura", {4, 4, 4, 4, 4}},
    {"Idiomas", {4, 4, 4, 4, 4}},
    {"Matemáticas", {4, 4, 4, 4, 4}},
    {"Educación Física", {4, 4, 4, 4, 4}},
    {"Biología, Ambiente y Tecnología", {4, 4, 4, 4, 4}},
    {"Física", {2, 2, 4, 4, 4}},
    {"Química", {2, 2, 4, 4, 4}},
    {"Geografía, Historia y Soberanía Nacional", {6, 6, 4, 4, 4}},
    {"Orientación Vocacional", {4, 4, 2, 2, 2}},
    {"Innovación Tecnológica y Productividad", {6, 6, 6, 6, 6}},
};

/// Las dos áreas excluidas de todo plan (D-4): sus materias quedan en el catálogo sin carga.
const char* const kAreaParticipacion =
    "Participación en los Grupos de Creación, Recreación y Producción";
const char* const kAreaCienciasTierra = "Ciencias de la Tierra";

/// Materia sin carga (excepción de D-4): «Participación…» de 1.º a 5.º y «Ciencias de la
/// Tierra» de 5.º. No pertenecen a ningún plan; se siembran para probar docentes habilitados
/// sin materia que impartir.
struct MateriaSinCargaDef {
    const char* area;
    int anio;
};

const MateriaSinCargaDef kMateriasSinCarga[] = {
    {kAreaParticipacion, 1},
    {kAreaParticipacion, 2},
    {kAreaParticipacion, 3},
    {kAreaParticipacion, 4},
    {kAreaParticipacion, 5},
    {kAreaCienciasTierra, 5},
};

/// Nombre de la materia-año: «<Área> <año>º» (por ejemplo «Castellano 1º», «Física 3º»).
QString nombreMateria(const char* area, int anio) {
    return QStringLiteral("%1 %2º").arg(QString::fromUtf8(area)).arg(anio);
}

QString nombreMateria(const QString& area, int anio) {
    return QStringLiteral("%1 %2º").arg(area).arg(anio);
}

/// Horas canónicas de un área en un año dentro de una tabla de plan; -1 si el área no está
/// en el plan o no se cursa ese año. Es la constante independiente del `Perfil` que se
/// valida: `validarPerfil` compara cada plan contra ella (plan §6.6).
int horasCanonicas(const AreaPlan* tabla, int cantidad, const QString& area, int anio) {
    for (int i = 0; i < cantidad; ++i) {
        if (area == QString::fromUtf8(tabla[i].nombre)) {
            return tabla[i].horas[anio - 1];
        }
    }
    return -1;
}

/// Curso del centro (plan §4.3): plan, año, sección, turno, aula de referencia y matrícula.
struct CursoTabla {
    const char* plan;
    int anio;
    const char* seccion;
    const char* turno;
    const char* aula;
    int matricula;
};

/// Los 24 cursos: 12 de la mañana (31059) y 12 de la tarde (31059 2.º y 31060). Las letras
/// de 31060 son libres para que cada pareja año+sección exista una sola vez (D-12).
const CursoTabla kCursos[] = {
    // Mañana — 31059
    {"31059", 1, "A", "Mañana", "Aula 01", 39},
    {"31059", 1, "B", "Mañana", "Aula 02", 39},
    {"31059", 3, "A", "Mañana", "Aula 03", 39},
    {"31059", 3, "B", "Mañana", "Aula 04", 35},
    {"31059", 3, "C", "Mañana", "Aula 05", 34},
    {"31059", 4, "A", "Mañana", "Aula 12", 44},
    {"31059", 4, "B", "Mañana", "Aula 11", 44},
    {"31059", 4, "C", "Mañana", "Aula 10", 41},
    {"31059", 5, "A", "Mañana", "Aula 09", 35},
    {"31059", 5, "B", "Mañana", "Aula 08", 42},
    {"31059", 5, "C", "Mañana", "Aula 07", 42},
    {"31059", 5, "D", "Mañana", "Aula 06", 39},
    // Tarde — 31059 (2.º) y 31060
    {"31059", 2, "A", "Tarde", "Aula 01", 37},
    {"31059", 2, "B", "Tarde", "Aula 02", 35},
    {"31059", 2, "C", "Tarde", "Aula 03", 34},
    {"31060", 1, "C", "Tarde", "Aula 10", 37},
    {"31060", 1, "D", "Tarde", "Aula 11", 36},
    {"31060", 1, "E", "Tarde", "Aula 12", 36},
    {"31060", 2, "D", "Tarde", "Aula 04", 32},
    {"31060", 2, "E", "Tarde", "Aula 05", 33},
    {"31060", 3, "D", "Tarde", "Aula 07", 37},
    {"31060", 3, "E", "Tarde", "Aula 06", 35},
    {"31060", 4, "D", "Tarde", "Aula 09", 32},
    {"31060", 4, "E", "Tarde", "Aula 08", 32},
};

/// Aula del centro (plan §4.4): las 12 de uso general (capacidad 44) y la cancha (88).
struct AulaTabla {
    const char* nombre;
    int capacidad;
    const char* edificio;
    const char* piso;
    bool cancha;   // false = tipo general del esquema
};

const AulaTabla kAulas[] = {
    {"Aula 01", 44, "A", "Planta baja", false},
    {"Aula 02", 44, "A", "Planta baja", false},
    {"Aula 03", 44, "A", "Planta baja", false},
    {"Aula 04", 44, "A", "Piso 1", false},
    {"Aula 05", 44, "B", "Planta baja", false},
    {"Aula 06", 44, "B", "Planta baja", false},
    {"Aula 07", 44, "B", "Planta baja", false},
    {"Aula 08", 44, "B", "Piso 1", false},
    {"Aula 09", 44, "C", "Planta baja", false},
    {"Aula 10", 44, "C", "Planta baja", false},
    {"Aula 11", 44, "C", "Piso 1", false},
    {"Aula 12", 44, "C", "Piso 1", false},
    {"Cancha", 88, "Patio", "Descubierto", true},
};

/// Evento del turno (plan §4.5): nombre, posición, orden, duración y horario en reloj.
EventoDef evento(const char* nombre, int despuesDeSlot, int orden, int duracion,
                 const QTime& inicio, const QTime& fin) {
    return EventoDef{QString::fromUtf8(nombre), despuesDeSlot, orden, duracion, inicio, fin};
}

/// Regla de progresión del 31059 (RF-2): un área restringida a un rango de años.
struct ReglaProgresion {
    const char* area;
    int anioMin;
    int anioMax;
};

const ReglaProgresion kProgresion[] = {
    {"Arte y Patrimonio", 1, 2},
    {"Ciencias Naturales", 1, 2},
    {"Física", 3, 5},
    {"Química", 3, 5},
    {"Biología", 3, 5},
    {"Formación para la Soberanía Nacional", 4, 5},
};

/// Área de una materia del catálogo; vacía si no está en él. Pura.
QString areaDe(const Perfil& perfil, const QString& materia) {
    for (const MateriaAnio& m : perfil.materias) {
        if (m.nombre == materia) {
            return m.area;
        }
    }
    return QString();
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

/// Turno del perfil con ese nombre, o nullptr.
const TurnoDef* turnoPorNombre(const Perfil& perfil, const QString& nombre) {
    for (const TurnoDef& turno : perfil.turnos) {
        if (turno.nombre == nombre) {
            return &turno;
        }
    }
    return nullptr;
}

/// Nombre legible del curso para los mensajes («1ºA»).
QString nombreCurso(const CursoDef& curso) {
    return QStringLiteral("%1º%2").arg(curso.anio).arg(curso.seccion);
}

} // namespace

const Perfil& perfilPorDefecto() {
    static const Perfil perfil = [] {
        Perfil p;

        // Catálogo declarado de tipos de aula (RF-3, RF-5): solo la cancha; el tipo general
        // lo garantiza la creación de la base.
        p.tiposAula = {aforo_aula::tipoCancha()};

        // Duración de la hora académica (RF-7): 40 min, valor que el sembrador escribe en la
        // configuración global.
        p.duracionMinutos = 40;

        // Planes 31059 y 31060 con sus cargas reales (RF-2).
        PlanDef mediaGeneral;
        mediaGeneral.codigo = QStringLiteral("31059");
        mediaGeneral.nombre = QStringLiteral("Media General");
        mediaGeneral.areaDesarrollo = QStringLiteral("Media General");

        PlanDef cienciasTecnologia;
        cienciasTecnologia.codigo = QStringLiteral("31060");
        cienciasTecnologia.nombre = QStringLiteral("Ciencias y Tecnología");
        cienciasTecnologia.areaDesarrollo = QStringLiteral("Ciencias y Tecnología");

        // Catálogo y cargas en una sola pasada determinista (plan §4.2): por año (1..5) y,
        // dentro de cada año, primero las áreas del 31059 y después las del 31060. Si una
        // materia-año ya existe (compartida: Matemáticas, Educación Física, Física y
        // Química), no se duplica el catálogo: solo se añade la carga del otro plan.
        QSet<QString> nombresCatalogo;
        for (int anio = 1; anio <= 5; ++anio) {
            for (const AreaPlan& area : kAreas31059) {
                const int horas = area.horas[anio - 1];
                if (horas < 0) {
                    continue;
                }
                const QString nombre = nombreMateria(area.nombre, anio);
                if (!nombresCatalogo.contains(nombre)) {
                    p.materias.append(MateriaAnio{nombre, anio, QString::fromUtf8(area.nombre), false});
                    nombresCatalogo.insert(nombre);
                }
                mediaGeneral.cargas.append(CargaPlan{nombre, anio, horas});
            }
            for (const AreaPlan& area : kAreas31060) {
                const int horas = area.horas[anio - 1];
                if (horas < 0) {
                    continue;
                }
                const QString nombre = nombreMateria(area.nombre, anio);
                if (!nombresCatalogo.contains(nombre)) {
                    p.materias.append(MateriaAnio{nombre, anio, QString::fromUtf8(area.nombre), false});
                    nombresCatalogo.insert(nombre);
                }
                cienciasTecnologia.cargas.append(CargaPlan{nombre, anio, horas});
            }
        }

        // Las 6 materias sin carga de D-4, al final del catálogo y sin enlace a ningún plan.
        for (const MateriaSinCargaDef& sinCarga : kMateriasSinCarga) {
            const QString area = QString::fromUtf8(sinCarga.area);
            p.materias.append(
                MateriaAnio{nombreMateria(area, sinCarga.anio), sinCarga.anio, area, true});
        }

        p.planes.append(mediaGeneral);
        p.planes.append(cienciasTecnologia);

        // Turnos (RF-7, plan §4.5): 8 franjas de 40 min; la mañana cierra a las 12:40 con
        // receso y acto cívico (8 × 40 + 20 = 340) y la tarde a las 18:10 con su receso
        // (8 × 40 + 10 = 330).
        {
            TurnoDef manana;
            manana.nombre = QStringLiteral("Mañana");
            manana.inicio = QTime(7, 0);
            manana.fin = QTime(12, 40);
            manana.numSlots = 8;
            manana.eventos.append(evento("Receso", 2, 1, 10, QTime(8, 20), QTime(8, 30)));
            manana.eventos.append(evento("Acto cívico", 2, 2, 10, QTime(8, 30), QTime(8, 40)));
            p.turnos.append(manana);

            TurnoDef tarde;
            tarde.nombre = QStringLiteral("Tarde");
            tarde.inicio = QTime(12, 40);
            tarde.fin = QTime(18, 10);
            tarde.numSlots = 8;
            tarde.eventos.append(evento("Receso", 2, 1, 10, QTime(14, 0), QTime(14, 10)));
            p.turnos.append(tarde);
        }

        // Aulas (RF-5): 12 de uso general de capacidad 44 y una cancha de 88.
        for (const AulaTabla& aula : kAulas) {
            p.aulas.append(AulaDef{QString::fromUtf8(aula.nombre), aula.capacidad,
                                   QString::fromUtf8(aula.edificio),
                                   QString::fromUtf8(aula.piso),
                                   aula.cancha ? aforo_aula::tipoCancha()
                                               : tipo_general::nombre()});
        }

        // Cursos (RF-4, plan §4.3): 24, con su plan y su turno en el perfil y aula de
        // referencia de tipo general.
        for (const CursoTabla& curso : kCursos) {
            p.cursos.append(CursoDef{curso.anio, QString::fromUtf8(curso.seccion),
                                     QString::fromUtf8(curso.aula),
                                     QString::fromUtf8(curso.plan),
                                     QString::fromUtf8(curso.turno), curso.matricula});
        }

        return p;
    }();
    return perfil;
}

QString tipoEspecializadoDeMateria(const QString& nombre) {
    // Educación Física se imparte en la cancha (RF-3): se devuelve el nombre visible canónico
    // del tipo. El reconocimiento del tipo declarado se hace por clave en `tipoFinalDeMateria`.
    if (nombre.startsWith(QStringLiteral("Educación Física "))) {
        return aforo_aula::tipoCancha();
    }
    return tipo_general::nombre();
}

QString tipoFinalDeMateria(const QString& nombre, const QSet<QString>& tiposDeAulas) {
    // 018 (RF-2): el tipo se resuelve por CLAVE normalizada y se devuelve el nombre declarado
    // (puede ser «CANCHA», «cancha »…), para que el sembrador siga casando `idTipoAula` por
    // nombre. Si el conjunto no declara un aula de ese tipo, la materia cae al tipo general.
    const QString clave = tipo_general::claveNormalizada(tipoEspecializadoDeMateria(nombre));
    for (const QString& declarado : tiposDeAulas) {
        if (tipo_general::claveNormalizada(declarado) == clave) {
            return declarado;
        }
    }
    // El tipo general lo aporta el esquema aunque el perfil no lo declare.
    return tipo_general::nombre();
}

CapacidadCancha capacidadCancha(const Perfil& perfil) {
    CapacidadCancha capacidad;
    for (const AulaDef& aula : perfil.aulas) {
        if (tipo_general::claveNormalizada(aula.tipo) == aforo_aula::claveCancha()) {
            ++capacidad.aulas;
        }
    }
    capacidad.aforo = aforo_aula::aforoDeClave(aforo_aula::claveCancha());

    const int dias = 5;
    capacidad.franjasPorTurno =
        perfil.turnos.isEmpty() ? 0 : dias * perfil.turnos[0].numSlots;
    capacidad.plazasPorTurno =
        capacidad.aulas * capacidad.franjasPorTurno * capacidad.aforo;
    capacidad.plazasSemana = capacidad.plazasPorTurno * perfil.turnos.size();

    if (!perfil.turnos.isEmpty()) {
        const TurnoDef& turnoDef = perfil.turnos[0];
        TurnoConfig turno;
        turno.nombre = turnoDef.nombre;
        for (int s = 0; s < turnoDef.numSlots; ++s) {
            turno.slot.append(s);
        }
        turno.inicio = turnoDef.inicio.toString(QStringLiteral("HH:mm"));
        turno.fin = turnoDef.fin.toString(QStringLiteral("HH:mm"));

        QVector<EventoConfig> eventos;
        for (const EventoDef& eventoDef : turnoDef.eventos) {
            EventoConfig evento;
            evento.nombre = eventoDef.nombre;
            evento.turno = turnoDef.nombre;
            evento.despues_de_slot = eventoDef.despuesDeSlot;
            evento.orden = eventoDef.orden;
            evento.duracion = eventoDef.duracion;
            evento.inicio = eventoDef.inicio.toString(QStringLiteral("HH:mm"));
            evento.fin = eventoDef.fin.toString(QStringLiteral("HH:mm"));
            eventos.append(evento);
        }

        // Las posiciones de par NO se recalculan aquí: se delegan en el contrato.
        capacidad.posicionesParPorTurno = dias * posicionesDePar(turno, eventos);
        capacidad.clasesEfPorTurno = demandaDeAreaPorTurno(
            perfil, QStringLiteral("Educación Física"), turnoDef.nombre);
    }
    capacidad.plazasParPorTurno =
        capacidad.aulas * capacidad.posicionesParPorTurno * capacidad.aforo;
    capacidad.paresEfPorTurno = capacidad.clasesEfPorTurno / 2;

    for (const TurnoDef& turno : perfil.turnos) {
        capacidad.clasesEfSemana +=
            demandaDeAreaPorTurno(perfil, QStringLiteral("Educación Física"), turno.nombre);
    }
    return capacidad;
}

int horasDeCurso(const Perfil& perfil, const CursoDef& curso) {
    const PlanDef* plan = planPorCodigo(perfil, curso.plan);
    if (plan == nullptr) {
        return -1;
    }
    return horasDeAnio(*plan, curso.anio);
}

int demandaTotal(const Perfil& perfil) {
    int total = 0;
    for (const CursoDef& curso : perfil.cursos) {
        const int horas = horasDeCurso(perfil, curso);
        if (horas < 0) {
            return -1;
        }
        total += horas;
    }
    return total;
}

int demandaPorTurno(const Perfil& perfil, const QString& turno) {
    if (turnoPorNombre(perfil, turno) == nullptr) {
        return -1;
    }
    int total = 0;
    for (const CursoDef& curso : perfil.cursos) {
        if (curso.turno != turno) {
            continue;
        }
        const int horas = horasDeCurso(perfil, curso);
        if (horas < 0) {
            return -1;
        }
        total += horas;
    }
    return total;
}

bool areaConCursosEnTurno(const Perfil& perfil, const QString& area, const QString& turno) {
    for (const CursoDef& curso : perfil.cursos) {
        if (curso.turno != turno) {
            continue;
        }
        const PlanDef* plan = planPorCodigo(perfil, curso.plan);
        if (plan == nullptr) {
            continue;
        }
        for (const CargaPlan& carga : plan->cargas) {
            if (carga.anio == curso.anio && areaDe(perfil, carga.materia) == area) {
                return true;
            }
        }
    }
    return false;
}

int demandaDeAreaPorTurno(const Perfil& perfil, const QString& area, const QString& turno) {
    if (turnoPorNombre(perfil, turno) == nullptr) {
        return -1;
    }
    int total = 0;
    for (const CursoDef& curso : perfil.cursos) {
        if (curso.turno != turno) {
            continue;
        }
        const PlanDef* plan = planPorCodigo(perfil, curso.plan);
        if (plan == nullptr) {
            return -1;
        }
        for (const CargaPlan& carga : plan->cargas) {
            if (carga.anio == curso.anio && areaDe(perfil, carga.materia) == area) {
                total += carga.horas;
            }
        }
    }
    return total;
}

QString motivoNoGenerable(const Perfil& perfil) {
    // La Educación Física se imparte en la cancha (D-5): sin ella no hay dónde ubicarla.
    const CapacidadCancha capacidad = capacidadCancha(perfil);
    if (capacidad.aulas == 0) {
        return QStringLiteral("el centro no tiene ninguna aula de tipo «%1» para la "
                              "Educación Física.")
            .arg(aforo_aula::tipoCancha());
    }

    // Con aforo 2, la cancha ofrece `franjasPorTurno × aforo` plazas por turno: en el centro,
    // 40 × 2 = 80 frente a las 48 clases de Educación Física (018, RF-8).
    for (const TurnoDef& turno : perfil.turnos) {
        const int plazas = capacidad.aulas * 5 * turno.numSlots * capacidad.aforo;
        const int demanda = demandaDeAreaPorTurno(perfil, QStringLiteral("Educación Física"),
                                                  turno.nombre);
        if (demanda > plazas) {
            return QStringLiteral("«%1»: %2 clases de Educación Física frente a %3 plazas de "
                                  "cancha (%4 aulas × %5 franjas × aforo %6).")
                .arg(turno.nombre)
                .arg(demanda)
                .arg(plazas)
                .arg(capacidad.aulas)
                .arg(5 * turno.numSlots)
                .arg(capacidad.aforo);
        }
    }
    return QString();
}

bool validarPerfil(const Perfil& perfil, QString* error) {
    const auto falla = [&](const QString& mensaje) {
        if (error) {
            *error = mensaje;
        }
        return false;
    };

    // 1. Duración de la hora académica (RF-7): rango único del punto de duración horaria.
    {
        const QString errorRango = duracion_horaria::errorDeRango(perfil.duracionMinutos);
        if (!errorRango.isEmpty()) {
            return falla(QStringLiteral("Duración de la hora académica inválida: %1")
                             .arg(errorRango));
        }
    }

    // 2. Turnos (RF-7): exactamente 2 (mañana y tarde), 8 franjas y sus ventanas reales.
    {
        const struct {
            const char* nombre;
            int hIni, mIni, hFin, mFin;
        } kTurnos[] = {
            {"Mañana", 7, 0, 12, 40},
            {"Tarde", 12, 40, 18, 10},
        };
        if (perfil.turnos.size() != 2) {
            return falla(QStringLiteral("El perfil debe tener 2 turnos (tiene %1).")
                             .arg(perfil.turnos.size()));
        }
        for (int i = 0; i < 2; ++i) {
            const TurnoDef& turno = perfil.turnos[i];
            if (turno.nombre != QString::fromUtf8(kTurnos[i].nombre)) {
                return falla(QStringLiteral("El turno %1 debería llamarse «%2» (tiene «%3»).")
                                 .arg(i + 1)
                                 .arg(QString::fromUtf8(kTurnos[i].nombre), turno.nombre));
            }
            if (turno.numSlots != 8) {
                return falla(QStringLiteral("El turno «%1» tiene %2 franjas (se esperan 8).")
                                 .arg(turno.nombre)
                                 .arg(turno.numSlots));
            }
            if (turno.inicio != QTime(kTurnos[i].hIni, kTurnos[i].mIni) ||
                turno.fin != QTime(kTurnos[i].hFin, kTurnos[i].mFin)) {
                return falla(QStringLiteral("El turno «%1» tiene la ventana %2–%3 "
                                            "(se esperaba %4–%5).")
                                 .arg(turno.nombre)
                                 .arg(turno.inicio.toString(QStringLiteral("HH:mm")),
                                      turno.fin.toString(QStringLiteral("HH:mm")),
                                      QTime(kTurnos[i].hIni, kTurnos[i].mIni)
                                          .toString(QStringLiteral("HH:mm")),
                                      QTime(kTurnos[i].hFin, kTurnos[i].mFin)
                                          .toString(QStringLiteral("HH:mm"))));
            }
        }
    }

    // 3. Eventos de los turnos (RF-7): nombre, posición, orden único por posición, duración
    //    y, si declara horario, par completo, diferencia coherente y contención en la ventana
    //    de su posición. El encaje se comprueba con la duración **del perfil**, no con la de
    //    fábrica, para que un turno de 40 min no herede la validación de 35.
    for (const TurnoDef& turno : perfil.turnos) {
        TurnoDTO turnoDto;
        turnoDto.nombre = turno.nombre;
        turnoDto.inicio = turno.inicio;
        turnoDto.fin = turno.fin;
        turnoDto.numSlots = turno.numSlots;
        for (const EventoDef& evento : turno.eventos) {
            EventoDTO dto;
            dto.nombre = evento.nombre;
            dto.despuesDeSlot = evento.despuesDeSlot;
            dto.orden = evento.orden;
            dto.duracion = evento.duracion;
            dto.inicio = evento.inicio;
            dto.fin = evento.fin;
            turnoDto.eventos.append(dto);
        }

        QSet<QString> ordenesOcupados;
        for (int i = 0; i < turno.eventos.size(); ++i) {
            const EventoDef& evento = turno.eventos[i];
            if (evento.nombre.trimmed().isEmpty()) {
                return falla(QStringLiteral("El turno «%1» tiene un evento sin nombre.")
                                 .arg(turno.nombre));
            }
            if (evento.despuesDeSlot < 1 || evento.despuesDeSlot > turno.numSlots) {
                return falla(QStringLiteral("El evento «%1» del turno «%2» tiene la posición %3 "
                                            "fuera de 1..%4.")
                                 .arg(evento.nombre, turno.nombre)
                                 .arg(evento.despuesDeSlot)
                                 .arg(turno.numSlots));
            }
            if (evento.duracion <= 0) {
                return falla(QStringLiteral("El evento «%1» del turno «%2» tiene una duración no "
                                            "positiva (%3 min).")
                                 .arg(evento.nombre, turno.nombre)
                                 .arg(evento.duracion));
            }
            if (evento.orden < 1) {
                return falla(QStringLiteral("El evento «%1» del turno «%2» tiene un orden inválido (%3).")
                                 .arg(evento.nombre, turno.nombre)
                                 .arg(evento.orden));
            }
            const QString clave =
                QStringLiteral("%1#%2").arg(evento.despuesDeSlot).arg(evento.orden);
            if (ordenesOcupados.contains(clave)) {
                return falla(QStringLiteral("El turno «%1» repite el orden %2 en la posición %3.")
                                 .arg(turno.nombre)
                                 .arg(evento.orden)
                                 .arg(evento.despuesDeSlot));
            }
            ordenesOcupados.insert(clave);

            const bool tieneInicio = evento.inicio.isValid();
            const bool tieneFin = evento.fin.isValid();
            if (tieneInicio != tieneFin) {
                return falla(QStringLiteral("El evento «%1» del turno «%2» declara solo una de "
                                            "sus horas.")
                                 .arg(evento.nombre, turno.nombre));
            }
            if (!tieneInicio) {
                continue;   // sin horario no hay ventana que comprobar
            }
            if (evento.fin <= evento.inicio) {
                return falla(QStringLiteral("El evento «%1» del turno «%2» termina antes de "
                                            "empezar (%3–%4).")
                                 .arg(evento.nombre, turno.nombre)
                                 .arg(evento.inicio.toString(QStringLiteral("HH:mm")))
                                 .arg(evento.fin.toString(QStringLiteral("HH:mm"))));
            }
            if (evento.inicio.secsTo(evento.fin) != evento.duracion * 60) {
                return falla(QStringLiteral("La duración del evento «%1» (%2 min) del turno «%3» "
                                            "no coincide con la diferencia entre sus horas (%4 min).")
                                 .arg(evento.nombre)
                                 .arg(evento.duracion)
                                 .arg(turno.nombre)
                                 .arg(evento.inicio.secsTo(evento.fin) / 60));
            }
            if (!eventoDentroDeVentana(turnoDto.eventos[i], turnoDto, evento.despuesDeSlot,
                                       perfil.duracionMinutos)) {
                const QPair<QTime, QTime> ventana =
                    ventanaDePosicion(turnoDto, evento.despuesDeSlot, perfil.duracionMinutos);
                return falla(QStringLiteral("El evento «%1» del turno «%2» (%3–%4) queda fuera de "
                                            "la ventana de la posición %5 (%6–%7).")
                                 .arg(evento.nombre, turno.nombre)
                                 .arg(evento.inicio.toString(QStringLiteral("HH:mm")))
                                 .arg(evento.fin.toString(QStringLiteral("HH:mm")))
                                 .arg(evento.despuesDeSlot)
                                 .arg(ventana.first.toString(QStringLiteral("HH:mm")))
                                 .arg(ventana.second.toString(QStringLiteral("HH:mm"))));
            }
        }

        const QString errorEncaje = errorDeEncaje(turnoDto, perfil.duracionMinutos);
        if (!errorEncaje.isEmpty()) {
            return falla(errorEncaje);
        }
    }

    // 4. Aulas (RF-5): exactamente 13, nombre único, 12 generales de capacidad ≥ 44 y una
    //    cancha de capacidad ≥ 88.
    {
        // 018 (RF-2, D-3): la cancha declarada se identifica por su **nombre** y su tipo se
        // clasifica por **clave**. Una cancha cuyo tipo no se normalice a `cancha` se rechaza
        // de forma explícita (tipo recibido, clave esperada y acción), sin aplicar el aforo 1
        // en silencio.
        const AulaDef* canchaDeclarada = nullptr;
        for (const AulaDef& aula : perfil.aulas) {
            if (aula.nombre == aforo_aula::tipoCancha()) {
                canchaDeclarada = &aula;
                break;
            }
        }
        if (canchaDeclarada == nullptr) {
            return falla(QStringLiteral("El perfil debe declarar el aula «%1».")
                             .arg(aforo_aula::tipoCancha()));
        }
        const QString claveDeclarada = tipo_general::claveNormalizada(canchaDeclarada->tipo);
        if (claveDeclarada != aforo_aula::claveCancha()) {
            return falla(QStringLiteral("La cancha «%1» declara el tipo «%2» (clave «%3»); la "
                                        "clave esperada es «%4»: el tipo de la cancha debe "
                                        "llamarse «%5». Corrija el tipo del aula o del catálogo.")
                             .arg(canchaDeclarada->nombre)
                             .arg(canchaDeclarada->tipo)
                             .arg(claveDeclarada)
                             .arg(aforo_aula::claveCancha())
                             .arg(aforo_aula::tipoCancha()));
        }

        QSet<QString> nombres;
        int generales = 0;
        int canchas = 0;
        for (const AulaDef& aula : perfil.aulas) {
            if (aula.nombre.isEmpty()) {
                return falla(QStringLiteral("El perfil contiene un aula sin nombre."));
            }
            if (nombres.contains(aula.nombre)) {
                return falla(QStringLiteral("El aula «%1» está duplicada.").arg(aula.nombre));
            }
            nombres.insert(aula.nombre);
            const QString clave = tipo_general::claveNormalizada(aula.tipo);
            if (clave == tipo_general::clave()) {
                ++generales;
                if (aula.capacidad < 44) {
                    return falla(QStringLiteral("El aula «%1» tiene capacidad %2 (las generales "
                                                "deben cubrir el grupo mayor, 44).")
                                     .arg(aula.nombre)
                                     .arg(aula.capacidad));
                }
            } else if (clave == aforo_aula::claveCancha()) {
                ++canchas;
                if (aula.capacidad < 88) {
                    return falla(QStringLiteral("La cancha «%1» tiene capacidad %2 (debe cubrir "
                                                "dos grupos completos, 88).")
                                     .arg(aula.nombre)
                                     .arg(aula.capacidad));
                }
            }
        }
        if (perfil.aulas.size() != 13) {
            return falla(QStringLiteral("El perfil debe tener 13 aulas (tiene %1).")
                             .arg(perfil.aulas.size()));
        }
        if (generales != 12) {
            return falla(QStringLiteral("El perfil debe tener 12 aulas de uso general (tiene %1).")
                             .arg(generales));
        }
        if (canchas != 1) {
            return falla(QStringLiteral("El perfil debe tener una cancha (tiene %1).").arg(canchas));
        }
    }

    // 5. Tipos de aula (RF-3, RF-5): catálogo declarado no vacío y sin duplicados, cada aula
    //    con un tipo del catálogo declarado o el general del esquema y cada materia con un
    //    tipo requerido del que haya al menos un aula sembrada.
    {
        if (perfil.tiposAula.isEmpty()) {
            return falla(QStringLiteral("El perfil no declara tipos de aula."));
        }
        QSet<QString> catalogo;
        for (const QString& tipo : perfil.tiposAula) {
            const QString nombre = tipo.trimmed();
            if (nombre.isEmpty()) {
                return falla(
                    QStringLiteral("El catálogo de tipos de aula contiene un tipo sin nombre."));
            }
            if (catalogo.contains(nombre)) {
                return falla(QStringLiteral("El tipo de aula «%1» está duplicado en el catálogo.")
                                 .arg(nombre));
            }
            catalogo.insert(nombre);
        }

        // El general lo aporta la creación de la base, no el perfil: sus aulas pueden
        // referenciarlo aunque no figure en el catálogo declarado.
        catalogo.insert(tipo_general::nombre());

        QSet<QString> tiposDeAulas;
        for (const AulaDef& aula : perfil.aulas) {
            if (!catalogo.contains(aula.tipo)) {
                return falla(QStringLiteral("El aula «%1» tiene el tipo «%2», que no está en el "
                                            "catálogo.")
                                 .arg(aula.nombre, aula.tipo));
            }
            tiposDeAulas.insert(aula.tipo);
        }

        for (const MateriaAnio& materia : perfil.materias) {
            const QString tipo = tipoFinalDeMateria(materia.nombre, tiposDeAulas);
            if (!tiposDeAulas.contains(tipo)) {
                return falla(QStringLiteral("La materia «%1» requiere el tipo de aula «%2», del "
                                            "que no hay ninguna aula sembrada.")
                                 .arg(materia.nombre, tipo));
            }
        }
    }

    // 6. Catálogo (RF-3, D-4): nombres únicos, año válido, área conocida, `sinCarga`
    //    coherente con los enlaces a planes y exactamente las 6 materias sin carga.
    {
        QSet<QString> nombres;
        int sinCarga = 0;
        for (const MateriaAnio& materia : perfil.materias) {
            if (materia.nombre.isEmpty()) {
                return falla(QStringLiteral("El catálogo contiene una materia sin nombre."));
            }
            if (materia.anio < 1 || materia.anio > 5) {
                return falla(QStringLiteral("La materia «%1» tiene un año inválido (%2).")
                                 .arg(materia.nombre)
                                 .arg(materia.anio));
            }
            if (materia.area.isEmpty()) {
                return falla(QStringLiteral("La materia «%1» no declara su área de formación.")
                                 .arg(materia.nombre));
            }
            if (nombres.contains(materia.nombre)) {
                return falla(QStringLiteral("La materia «%1» está duplicada en el catálogo.")
                                 .arg(materia.nombre));
            }
            nombres.insert(materia.nombre);

            int enlaces = 0;
            for (const PlanDef& plan : perfil.planes) {
                for (const CargaPlan& carga : plan.cargas) {
                    if (carga.materia == materia.nombre) {
                        ++enlaces;
                    }
                }
            }
            if (materia.sinCarga) {
                ++sinCarga;
                if (enlaces > 0) {
                    return falla(QStringLiteral("La materia sin carga «%1» aparece en un plan.")
                                     .arg(materia.nombre));
                }
            } else {
                if (enlaces == 0) {
                    return falla(
                        QStringLiteral("La materia «%1» del catálogo no aparece en ningún plan.")
                            .arg(materia.nombre));
                }
                if (horasCanonicas(kAreas31059, 12, materia.area, materia.anio) < 0 &&
                    horasCanonicas(kAreas31060, 10, materia.area, materia.anio) < 0) {
                    return falla(QStringLiteral("La materia «%1» declara el área «%2», que "
                                                "ningún plan incluye en %3º.")
                                     .arg(materia.nombre, materia.area)
                                     .arg(materia.anio));
                }
            }
        }
        if (sinCarga != 6) {
            return falla(QStringLiteral("El catálogo debe tener 6 materias sin carga (tiene %1).")
                             .arg(sinCarga));
        }
        // Las 6 sin carga son exactamente las de D-4 (área y año).
        for (const MateriaSinCargaDef& esperada : kMateriasSinCarga) {
            const QString nombre =
                nombreMateria(QString::fromUtf8(esperada.area), esperada.anio);
            if (!nombres.contains(nombre)) {
                return falla(QStringLiteral("Falta la materia sin carga «%1».").arg(nombre));
            }
        }
    }

    // 7. Cada plan (RF-2): código conocido, sin materias repetidas, sin años vacíos, horas no
    //    negativas, total par y ≤ 40, y tablas canónicas de áreas, horas y cargas por año.
    {
        const int kAreasPorAnio31059[5] = {8, 8, 9, 10, 10};
        const int kHorasPorAnio31059[5] = {34, 34, 34, 36, 34};
        const int kAreasPorAnio31060[5] = {10, 10, 10, 10, 10};
        const int kHorasPorAnio31060[5] = {40, 40, 40, 40, 40};

        for (const PlanDef& plan : perfil.planes) {
            const bool es31059 = plan.codigo == QStringLiteral("31059");
            const bool es31060 = plan.codigo == QStringLiteral("31060");
            if (!es31059 && !es31060) {
                return falla(QStringLiteral("El perfil declara el plan «%1», que no es del "
                                            "centro (31059 o 31060).")
                                 .arg(plan.codigo));
            }
            const AreaPlan* tabla = es31059 ? kAreas31059 : kAreas31060;
            const int cantidadAreas = es31059 ? 12 : 10;
            const int* areasPorAnio = es31059 ? kAreasPorAnio31059 : kAreasPorAnio31060;
            const int* horasPorAnio = es31059 ? kHorasPorAnio31059 : kHorasPorAnio31060;
            const int cargasEsperadas = es31059 ? 45 : 50;

            QSet<QString> vistas;
            for (const CargaPlan& carga : plan.cargas) {
                if (carga.horas < 0) {
                    return falla(QStringLiteral("La materia «%1» del plan %2 tiene horas negativas.")
                                     .arg(carga.materia, plan.codigo));
                }
                if (vistas.contains(carga.materia)) {
                    return falla(QStringLiteral("El plan %1 repite la materia «%2».")
                                     .arg(plan.codigo, carga.materia));
                }
                vistas.insert(carga.materia);
            }

            for (int anio = 1; anio <= 5; ++anio) {
                const int total = horasDeAnio(plan, anio);
                if (total < 0) {
                    return falla(QStringLiteral("El plan %1 deja el año %2º sin materias.")
                                     .arg(plan.codigo)
                                     .arg(anio));
                }
                if (total > 40) {
                    return falla(
                        QStringLiteral("El plan %1 supera 40 horas académicas en %2º (%3).")
                            .arg(plan.codigo)
                            .arg(anio)
                            .arg(total));
                }
                if (total % 2 != 0) {
                    return falla(QStringLiteral("El plan %1 suma %2 horas académicas en %3º; "
                                                "debe ser un número par.")
                                     .arg(plan.codigo)
                                     .arg(total)
                                     .arg(anio));
                }
                int contador = 0;
                for (const CargaPlan& carga : plan.cargas) {
                    if (carga.anio == anio) {
                        ++contador;
                    }
                }
                if (contador != areasPorAnio[anio - 1]) {
                    return falla(QStringLiteral("Plan %1, año %2: %3 materias; se esperaban %4.")
                                     .arg(plan.codigo)
                                     .arg(anio)
                                     .arg(contador)
                                     .arg(areasPorAnio[anio - 1]));
                }
                if (total != horasPorAnio[anio - 1]) {
                    return falla(QStringLiteral("Plan %1, año %2: suma %3 h académicas; se "
                                                "esperaban %4 (diferencia %5).")
                                     .arg(plan.codigo)
                                     .arg(anio)
                                     .arg(total)
                                     .arg(horasPorAnio[anio - 1])
                                     .arg(total - horasPorAnio[anio - 1]));
                }
            }

            if (plan.cargas.size() != cargasEsperadas) {
                return falla(QStringLiteral("El plan %1 tiene %2 cargas; se esperaban %3.")
                                 .arg(plan.codigo)
                                 .arg(plan.cargas.size())
                                 .arg(cargasEsperadas));
            }

            for (const CargaPlan& carga : plan.cargas) {
                const QString area = areaDe(perfil, carga.materia);
                if (area.isEmpty()) {
                    return falla(QStringLiteral("El plan %1 incluye la materia «%2», que no está "
                                                "en el catálogo.")
                                     .arg(plan.codigo, carga.materia));
                }
                const int canonica = horasCanonicas(tabla, cantidadAreas, area, carga.anio);
                if (canonica < 0) {
                    return falla(QStringLiteral("El plan %1 no incluye el área «%2» en %3º.")
                                     .arg(plan.codigo, area)
                                     .arg(carga.anio));
                }
                if (carga.horas != canonica) {
                    return falla(QStringLiteral("Plan %1: la materia «%2» tiene %3 h académicas; "
                                                "se esperaban %4.")
                                     .arg(plan.codigo, carga.materia)
                                     .arg(carga.horas)
                                     .arg(canonica));
                }
            }
        }

        if (perfil.planes.size() != 2) {
            return falla(QStringLiteral("El perfil tiene %1 planes; se esperaban 2.")
                             .arg(perfil.planes.size()));
        }
        if (perfil.materias.size() != 85) {
            return falla(
                QStringLiteral("El perfil tiene %1 materias en el catálogo; se esperaban 85.")
                    .arg(perfil.materias.size()));
        }
        int totalCargas = 0;
        for (const PlanDef& plan : perfil.planes) {
            totalCargas += plan.cargas.size();
        }
        if (totalCargas != 95) {
            return falla(
                QStringLiteral("El perfil tiene %1 cargas plan-materia; se esperaban 95.")
                    .arg(totalCargas));
        }
    }

    // 8. Progresión del 31059 (RF-2): las áreas restringidas solo en sus años. El 31060 no
    //    restringe (sus 10 áreas se cursan en los cinco años).
    for (const PlanDef& plan : perfil.planes) {
        if (plan.codigo != QStringLiteral("31059")) {
            continue;
        }
        for (const CargaPlan& carga : plan.cargas) {
            const QString area = areaDe(perfil, carga.materia);
            for (const ReglaProgresion& regla : kProgresion) {
                if (area == QString::fromUtf8(regla.area) &&
                    (carga.anio < regla.anioMin || carga.anio > regla.anioMax)) {
                    return falla(QStringLiteral("La materia «%1» del plan %2 rompe la progresión "
                                                "del 31059 (año %3 fuera de %4–%5).")
                                     .arg(carga.materia, plan.codigo)
                                     .arg(carga.anio)
                                     .arg(regla.anioMin)
                                     .arg(regla.anioMax));
                }
            }
        }
    }

    // 9. Cursos (RF-4, RF-5): 24, año 1–5, sección de un carácter en mayúscula, pareja
    //    año+sección única, plan y turno declarados, aula existente y de tipo general, sin
    //    repetir aula dentro de un turno, 12 por turno y matrícula entre 32 y 44.
    {
        QSet<QString> nombresAula;
        for (const AulaDef& aula : perfil.aulas) {
            nombresAula.insert(aula.nombre);
        }
        QSet<QString> parejas;
        QHash<QString, QSet<QString>> aulasPorTurno;
        QHash<QString, int> cursosPorTurno;
        for (const CursoDef& curso : perfil.cursos) {
            const QString nombre = nombreCurso(curso);
            if (curso.anio < 1 || curso.anio > 5) {
                return falla(QStringLiteral("El curso %1 tiene un año inválido.").arg(nombre));
            }
            if (curso.seccion.size() != 1 || !curso.seccion.at(0).isUpper()) {
                return falla(QStringLiteral("El curso %1 tiene una sección inválida («%2»).")
                                 .arg(nombre, curso.seccion));
            }
            const QString pareja = QStringLiteral("%1%2").arg(curso.anio).arg(curso.seccion);
            if (parejas.contains(pareja)) {
                return falla(QStringLiteral("La pareja año+sección «%1» se repite entre cursos.")
                                 .arg(pareja));
            }
            parejas.insert(pareja);

            const PlanDef* plan = planPorCodigo(perfil, curso.plan);
            if (plan == nullptr) {
                return falla(QStringLiteral("El curso %1 declara el plan «%2», que no existe.")
                                 .arg(nombre, curso.plan));
            }
            if (turnoPorNombre(perfil, curso.turno) == nullptr) {
                return falla(QStringLiteral("El curso %1 declara el turno «%2», que no existe.")
                                 .arg(nombre, curso.turno));
            }

            bool esGeneral = false;
            for (const AulaDef& aula : perfil.aulas) {
                if (aula.nombre == curso.aula && aula.tipo == tipo_general::nombre()) {
                    esGeneral = true;
                    break;
                }
            }
            if (!nombresAula.contains(curso.aula)) {
                return falla(QStringLiteral("El curso %1 tiene un aula inexistente («%2»).")
                                 .arg(nombre, curso.aula));
            }
            if (!esGeneral) {
                return falla(QStringLiteral("El curso %1 tiene el aula «%2», que no es de uso "
                                            "general.")
                                 .arg(nombre, curso.aula));
            }
            if (aulasPorTurno[curso.turno].contains(curso.aula)) {
                return falla(QStringLiteral("El aula «%1» se repite entre cursos del turno «%2».")
                                 .arg(curso.aula, curso.turno));
            }
            aulasPorTurno[curso.turno].insert(curso.aula);
            ++cursosPorTurno[curso.turno];

            if (horasDeAnio(*plan, curso.anio) <= 0) {
                return falla(QStringLiteral("El curso %1 no tiene materias en el plan %2.")
                                 .arg(nombre, curso.plan));
            }
            if (curso.matricula < 32 || curso.matricula > 44) {
                return falla(QStringLiteral("El curso %1 tiene matrícula %2 (debe estar entre 32 "
                                            "y 44).")
                                 .arg(nombre)
                                 .arg(curso.matricula));
            }
        }
        if (perfil.cursos.size() != 24) {
            return falla(QStringLiteral("El perfil debe tener 24 cursos (tiene %1).")
                             .arg(perfil.cursos.size()));
        }
        for (const TurnoDef& turno : perfil.turnos) {
            if (cursosPorTurno.value(turno.nombre) != 12) {
                return falla(QStringLiteral("El turno «%1» tiene %2 cursos (se esperan 12).")
                                 .arg(turno.nombre)
                                 .arg(cursosPorTurno.value(turno.nombre)));
            }
        }
    }

    return true;
}

} // namespace seed
