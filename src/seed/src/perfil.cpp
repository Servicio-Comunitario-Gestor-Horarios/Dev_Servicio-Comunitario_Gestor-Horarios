#include "seed/perfil.hpp"

#include <QSet>

#include "backend/services/calculo_eventos.hpp"

namespace seed {

int bloques(int horas) {
    // Rectificación (2026-10-17, constitución §7 / spec 005-T11): la hora académica
    // es un bloque, no una hora de reloj. La conversión es la IDENTIDAD:
    // bloques(horas) = horas. Los 40 minutos son la duración real de la franja y no
    // intervienen en la conversión.
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

int seccionesDeAnio(int anio) {
    // 3 secciones en 1.º–3.º y 2 en 4.º–5.º (RF-4).
    return (anio >= 1 && anio <= 3) ? 3 : 2;
}

namespace {

/// Tipos de aula del conjunto (RF-5, RF-10): el genérico y los dos especializados que usan
/// las aulas. «Cancha» no entra en el catálogo: el conjunto no tiene ninguna.
const char* const kTipoGenerico = "Aula de estudio";
const char* const kTipoLaboratorio = "Laboratorio";
const char* const kTipoTaller = "Taller";

/// Regla de tipo de materia por área/prefijo (RF-10, N1), no por nombre exacto: así
/// «Física Aplicada», «Química Aplicada» y «Ciencias de la Tierra Aplicadas» caen en
/// laboratorio. La fila de informática/taller es inocua: el perfil no tiene esas materias.
struct ReglaTipoMateria {
    const char* prefijo;
    const char* tipo;
};

const ReglaTipoMateria kReglasTipoMateria[] = {
    {"Ciencias Naturales", kTipoLaboratorio},
    {"Ciencias de la Tierra", kTipoLaboratorio},
    {"Física", kTipoLaboratorio},
    {"Química", kTipoLaboratorio},
    {"Biología", kTipoLaboratorio},
    {"Informática", kTipoTaller},
    {"Taller", kTipoTaller},
    {"Educación Física", "Cancha"},
};

/// Evento «Receso» sembrado (RF-10): posición 2, orden 1, 10 min y su horario.
EventoDef eventoReceso(const QTime& inicio, const QTime& fin) {
    return EventoDef{QStringLiteral("Receso"), 2, 1, 10, inicio, fin};
}

/// Un área de formación del 31059 con sus horas académicas semanales por año
/// (índice 0 = 1.º, … 4 = 5.º; -1 = no se cursa en ese año).
struct Area31059 {
    const char* nombre;
    int horas[5];
};

/// Las 14 áreas del plan 31059 en su orden oficial, con la tabla adaptada a
/// 40 horas académicas por año (RF-2, plan §4.2).
const Area31059 kAreas31059[] = {
    {"Castellano", {4, 4, 4, 4, 4}},
    {"Inglés y otras lenguas extranjeras", {5, 5, 5, 5, 3}},
    {"Matemáticas", {4, 4, 4, 4, 4}},
    {"Educación Física", {5, 5, 5, 5, 5}},
    {"Arte y Patrimonio", {4, 4, -1, -1, -1}},
    {"Ciencias Naturales", {5, 5, -1, -1, -1}},
    {"Física", {-1, -1, 4, 4, 4}},
    {"Química", {-1, -1, 3, 3, 3}},
    {"Biología", {-1, -1, 3, 3, 3}},
    {"Ciencias de la Tierra", {-1, -1, -1, -1, 2}},
    {"Geografía, Historia y Ciudadanía", {6, 6, 5, 3, 3}},
    {"Formación para la Soberanía Nacional", {-1, -1, -1, 2, 2}},
    {"Orientación y Convivencia", {2, 2, 2, 2, 2}},
    {"Participación en Grupos de Creación, Recreación y Producción", {5, 5, 5, 5, 5}},
};

/// Nombre de la materia-año: «<Área> <año>º» (por ejemplo «Castellano 1º», «Física 3º»).
QString nombreMateria(const Area31059& area, int anio) {
    return QStringLiteral("%1 %2º").arg(QString::fromUtf8(area.nombre)).arg(anio);
}

/// Materia-año canónica del plan General (tabla adaptada de §4.2) para un nombre dado:
/// año y horas académicas. Devuelve {-1, -1} si el nombre no es del 31059. Es la constante
/// independiente del `Perfil` que se valida: `validarPerfil` compara el plan General contra
/// ella, no contra el propio perfil.
struct CargaCanonica {
    int anio;
    int horas;
};

CargaCanonica cargaCanonica31059(const QString& nombre) {
    for (const Area31059& area : kAreas31059) {
        for (int anio = 1; anio <= 5; ++anio) {
            const int horas = area.horas[anio - 1];
            if (horas >= 0 && nombreMateria(area, anio) == nombre) {
                return CargaCanonica{anio, horas};
            }
        }
    }
    return CargaCanonica{-1, -1};
}

/// Materia nueva de un plan derivado (RF-3, plan §4.2): nombre, año y plan que la usa.
struct MateriaNuevaDef {
    const char* nombre;
    int anio;
    const char* plan;
};

/// Las 6 materias nuevas, en el orden fijo del plan §4.2 (determinismo, RF-11). Dentro de
/// cada año siguen el orden de la tabla del plan: 4.º Matemática Aplicada → Química Aplicada
/// → Geografía Económica; 5.º Física Aplicada → Ciencias de la Tierra Aplicadas → Historia
/// Contemporánea.
const MateriaNuevaDef kMateriasNuevas[] = {
    {"Matemática Aplicada", 4, "EXA"},
    {"Física Aplicada", 5, "EXA"},
    {"Química Aplicada", 4, "NAT"},
    {"Ciencias de la Tierra Aplicadas", 5, "NAT"},
    {"Geografía Económica", 4, "SOC"},
    {"Historia Contemporánea", 5, "SOC"},
};

/// Devuelve la definición de la materia nueva con ese nombre, o nullptr si no lo es.
const MateriaNuevaDef* materiaNueva(const QString& nombre) {
    for (const MateriaNuevaDef& nueva : kMateriasNuevas) {
        if (nombre == QString::fromUtf8(nueva.nombre)) {
            return &nueva;
        }
    }
    return nullptr;
}

/// Recorte de horas de un área base en un plan derivado (plan §4.2). Cada derivado añade su
/// materia nueva de 3 h y recorta 1 h en tres áreas base del mismo año, para que el año siga
/// sumando 40 horas académicas.
struct RecorteHoras {
    const char* plan;
    const char* materia;   // nombre completo de la materia-año
    int horas;             // nuevo valor
};

const RecorteHoras kRecortes[] = {
    // EXA: 4.º EF 5→4, Castellano 4→3, Grupos 5→4; 5.º EF 5→4, GHC 3→2, Grupos 5→4.
    {"EXA", "Educación Física 4º", 4},
    {"EXA", "Castellano 4º", 3},
    {"EXA", "Participación en Grupos de Creación, Recreación y Producción 4º", 4},
    {"EXA", "Educación Física 5º", 4},
    {"EXA", "Geografía, Historia y Ciudadanía 5º", 2},
    {"EXA", "Participación en Grupos de Creación, Recreación y Producción 5º", 4},
    // NAT: 4.º Castellano 4→3, GHC 3→2, Grupos 5→4; 5.º Castellano 4→3, GHC 3→2, Grupos 5→4.
    {"NAT", "Castellano 4º", 3},
    {"NAT", "Geografía, Historia y Ciudadanía 4º", 2},
    {"NAT", "Participación en Grupos de Creación, Recreación y Producción 4º", 4},
    {"NAT", "Castellano 5º", 3},
    {"NAT", "Geografía, Historia y Ciudadanía 5º", 2},
    {"NAT", "Participación en Grupos de Creación, Recreación y Producción 5º", 4},
    // SOC: 4.º EF 5→4, Matemáticas 4→3, Grupos 5→4; 5.º EF 5→4, Matemáticas 4→3, Grupos 5→4.
    {"SOC", "Educación Física 4º", 4},
    {"SOC", "Matemáticas 4º", 3},
    {"SOC", "Participación en Grupos de Creación, Recreación y Producción 4º", 4},
    {"SOC", "Educación Física 5º", 4},
    {"SOC", "Matemáticas 5º", 3},
    {"SOC", "Participación en Grupos de Creación, Recreación y Producción 5º", 4},
};

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
    {"Ciencias de la Tierra", 5, 5},
    {"Formación para la Soberanía Nacional", 4, 5},
};

} // namespace

const Perfil& perfilPorDefecto() {
    static const Perfil perfil = [] {
        Perfil p;

        // Catálogo de tipos de aula (009-T7, RF-5/RF-10): 3, en el orden en que las aulas los
        // usan. El sembrador los inserta antes que las aulas y las materias (T8).
        p.tiposAula = {QString::fromUtf8(kTipoGenerico),
                       QString::fromUtf8(kTipoLaboratorio),
                       QString::fromUtf8(kTipoTaller)};

        PlanDef general;
        general.codigo = QStringLiteral("GEN");
        general.nombre = QStringLiteral("General");
        general.areaDesarrollo = QStringLiteral("General");

        // Catálogo y plan General en una sola pasada determinista: por año (1..5) y, dentro
        // de cada año, por el orden de áreas del 31059. Al final de cada año se añaden las
        // materias nuevas de los derivados (008-T2), en el orden fijo del plan §4.2.
        for (int anio = 1; anio <= 5; ++anio) {
            for (const Area31059& area : kAreas31059) {
                const int horas = area.horas[anio - 1];
                if (horas < 0) {
                    continue;
                }
                const QString nombre = nombreMateria(area, anio);
                p.materias.append(MateriaAnio{nombre, anio});
                general.cargas.append(CargaPlan{nombre, anio, horas});
            }
            for (const MateriaNuevaDef& nueva : kMateriasNuevas) {
                if (nueva.anio == anio) {
                    p.materias.append(MateriaAnio{QString::fromUtf8(nueva.nombre), anio});
                }
            }
        }

        p.planes.append(general);

        // Planes derivados (008-T2): copian GEN y aplican los recortes y las materias nuevas
        // de §4.2, de modo que cada año suma exactamente 40 horas académicas.
        const struct {
            const char* codigo;
            const char* nombre;
            const char* area;
        } kDerivados[] = {
            {"EXA", "Ciencias Exactas", "Ciencias Exactas"},
            {"NAT", "Ciencias Naturales", "Ciencias Naturales"},
            {"SOC", "Ciencias Sociales", "Ciencias Sociales"},
        };

        for (const auto& d : kDerivados) {
            PlanDef plan;
            plan.codigo = QString::fromUtf8(d.codigo);
            plan.nombre = QString::fromUtf8(d.nombre);
            plan.areaDesarrollo = QString::fromUtf8(d.area);
            plan.cargas = general.cargas;

            for (const RecorteHoras& recorte : kRecortes) {
                if (QString::fromUtf8(recorte.plan) != plan.codigo) {
                    continue;
                }
                for (CargaPlan& carga : plan.cargas) {
                    if (carga.materia == QString::fromUtf8(recorte.materia)) {
                        carga.horas = recorte.horas;
                        break;
                    }
                }
            }
            for (const MateriaNuevaDef& nueva : kMateriasNuevas) {
                if (QString::fromUtf8(nueva.plan) != plan.codigo) {
                    continue;
                }
                plan.cargas.append(CargaPlan{QString::fromUtf8(nueva.nombre), nueva.anio, 3});
            }

            p.planes.append(plan);
        }

        // Turnos (008-T3, RF-4, plan §4.1; 009-T7, RF-10): mañana y tarde, 8 franjas de 40 min
        // y un evento «Receso» de 10 min tras la franja 2 (08:20–08:30 / 14:00–14:10). El `fin`
        // es la campana (incluye el receso): la rejilla aritmética termina en 12:20/18:00 y el
        // desfase de 10 min se asume (spec 008). Los campos `receso*` son el espejo de
        // compatibilidad del evento y se retiran en T8.
        {
            TurnoDef manana;
            manana.nombre = QStringLiteral("Mañana");
            manana.inicio = QTime(7, 0);
            manana.fin = QTime(12, 30);
            manana.numSlots = 8;
            manana.eventos.append(eventoReceso(QTime(8, 20), QTime(8, 30)));
            manana.recesoDespuesDeSlot = manana.eventos.first().despuesDeSlot;
            manana.recesoInicio = manana.eventos.first().inicio;
            manana.recesoFin = manana.eventos.first().fin;
            p.turnos.append(manana);

            TurnoDef tarde;
            tarde.nombre = QStringLiteral("Tarde");
            tarde.inicio = QTime(12, 40);
            tarde.fin = QTime(18, 10);
            tarde.numSlots = 8;
            tarde.eventos.append(eventoReceso(QTime(14, 0), QTime(14, 10)));
            tarde.recesoDespuesDeSlot = tarde.eventos.first().despuesDeSlot;
            tarde.recesoInicio = tarde.eventos.first().inicio;
            tarde.recesoFin = tarde.eventos.first().fin;
            p.turnos.append(tarde);
        }

        // Aulas (008-T3, RF-5, plan §4.3; 009-T7, RF-10): 16, nombre único, con variedad de
        // capacidad, edificio y piso; Aula 14 es de reserva y hay un laboratorio y un taller,
        // cada uno con su tipo; las demás son «Aula de estudio».
        {
            const struct {
                const char* nombre;
                int capacidad;
                const char* edificio;
                const char* piso;
                const char* tipo;
            } kAulas[] = {
                {"Aula 01", 40, "A", "Planta baja", kTipoGenerico},
                {"Aula 02", 40, "A", "Planta baja", kTipoGenerico},
                {"Aula 03", 38, "A", "Planta baja", kTipoGenerico},
                {"Aula 04", 38, "A", "Piso 1", kTipoGenerico},
                {"Aula 05", 36, "A", "Piso 1", kTipoGenerico},
                {"Aula 06", 36, "A", "Piso 1", kTipoGenerico},
                {"Aula 07", 34, "B", "Planta baja", kTipoGenerico},
                {"Aula 08", 34, "B", "Planta baja", kTipoGenerico},
                {"Aula 09", 36, "B", "Piso 1", kTipoGenerico},
                {"Aula 10", 38, "B", "Piso 1", kTipoGenerico},
                {"Aula 11", 40, "B", "Piso 1", kTipoGenerico},
                {"Aula 12", 40, "B", "Piso 1", kTipoGenerico},
                {"Aula 13", 34, "A", "Piso 1", kTipoGenerico},
                {"Aula 14", 32, "B", "Planta baja", kTipoGenerico},            // reserva
                {"Laboratorio de Ciencias", 36, "C", "Planta baja", kTipoLaboratorio},
                {"Taller de Informática", 30, "C", "Piso 1", kTipoTaller},
            };
            for (const auto& a : kAulas) {
                p.aulas.append(AulaDef{QString::fromUtf8(a.nombre), a.capacidad,
                                       QString::fromUtf8(a.edificio),
                                       QString::fromUtf8(a.piso),
                                       QString::fromUtf8(a.tipo)});
            }
        }

        // Cursos (008-T3, RF-4, plan §4.3): 13, de 1.º a 5.º con 3/3/3/2/2 secciones, cada uno
        // con un aula distinta (Aula 01…Aula 13) y SIN turno (el turno lo aporta el preset).
        {
            const struct {
                int anio;
                const char* seccion;
            } kCursos[] = {
                {1, "A"}, {1, "B"}, {1, "C"},
                {2, "A"}, {2, "B"}, {2, "C"},
                {3, "A"}, {3, "B"}, {3, "C"},
                {4, "A"}, {4, "B"},
                {5, "A"}, {5, "B"},
            };
            for (int i = 0; i < 13; ++i) {
                p.cursos.append(CursoDef{kCursos[i].anio,
                                         QString::fromUtf8(kCursos[i].seccion),
                                         QStringLiteral("Aula %1").arg(i + 1, 2, 10, QChar('0'))});
            }
        }

        return p;
    }();
    return perfil;
}

bool esMateriaNueva(const QString& nombre) {
    return materiaNueva(nombre) != nullptr;
}

QString tipoEspecializadoDeMateria(const QString& nombre) {
    for (const ReglaTipoMateria& regla : kReglasTipoMateria) {
        const QString prefijo = QString::fromUtf8(regla.prefijo) + QLatin1Char(' ');
        if (nombre.startsWith(prefijo)) {
            return QString::fromUtf8(regla.tipo);
        }
    }
    return QString::fromUtf8(kTipoGenerico);
}

QString tipoFinalDeMateria(const QString& nombre, const QSet<QString>& tiposDeAulas) {
    const QString especializado = tipoEspecializadoDeMateria(nombre);
    return tiposDeAulas.contains(especializado) ? especializado
                                                : QString::fromUtf8(kTipoGenerico);
}

int horasMaximas(const Perfil& perfil, const QString& materia) {
    int maximo = 0;
    for (const PlanDef& plan : perfil.planes) {
        for (const CargaPlan& carga : plan.cargas) {
            if (carga.materia == materia && carga.horas > maximo) {
                maximo = carga.horas;
            }
        }
    }
    return maximo;
}

int cargaMateria(const Perfil& perfil, const QString& materia) {
    if (esMateriaNueva(materia)) {
        return 0;  // las 6 materias nuevas sustituyen horas base y no añaden demanda (RF-3).
    }
    int anio = -1;
    for (const MateriaAnio& m : perfil.materias) {
        if (m.nombre == materia) {
            anio = m.anio;
            break;
        }
    }
    if (anio < 0) {
        return 0;
    }
    return horasMaximas(perfil, materia) * seccionesDeAnio(anio);
}

int cargaAsignada(const Perfil& perfil, const QStringList& materias) {
    int total = 0;
    for (const QString& materia : materias) {
        total += cargaMateria(perfil, materia);
    }
    return total;
}

int demandaTotal(const Perfil& perfil) {
    int total = 0;
    for (const MateriaAnio& materia : perfil.materias) {
        total += cargaMateria(perfil, materia.nombre);
    }
    return total;
}

bool validarPerfil(const Perfil& perfil, QString* error) {
    const auto falla = [&](const QString& mensaje) {
        if (error) {
            *error = mensaje;
        }
        return false;
    };

    // 0. Turnos (RF-4): exactamente 2 (mañana y tarde), 8 franjas, receso de 10 min tras la
    //    franja 2 con las horas de §4.1.
    {
        const struct {
            const char* nombre;
            int hIni, mIni, hFin, mFin;
            int hRecesoIni, mRecesoIni, hRecesoFin, mRecesoFin;
        } kTurnos[] = {
            {"Mañana", 7, 0, 12, 30, 8, 20, 8, 30},
            {"Tarde", 12, 40, 18, 10, 14, 0, 14, 10},
        };
        if (perfil.turnos.size() != 2) {
            return falla(QStringLiteral("El perfil debe tener 2 turnos (tiene %1).")
                             .arg(perfil.turnos.size()));
        }
        for (int i = 0; i < 2; ++i) {
            const TurnoDef& turno = perfil.turnos[i];
            if (turno.nombre != QString::fromUtf8(kTurnos[i].nombre)) {
                return falla(QStringLiteral("El turno %1 debería llamarse «%2».")
                                 .arg(i + 1)
                                 .arg(QString::fromUtf8(kTurnos[i].nombre)));
            }
            if (turno.numSlots != 8) {
                return falla(QStringLiteral("El turno «%1» tiene %2 franjas (se esperan 8).")
                                 .arg(turno.nombre)
                                 .arg(turno.numSlots));
            }
            if (turno.inicio != QTime(kTurnos[i].hIni, kTurnos[i].mIni) ||
                turno.fin != QTime(kTurnos[i].hFin, kTurnos[i].mFin)) {
                return falla(QStringLiteral("El turno «%1» tiene una ventana de campana distinta de %2–%3.")
                                 .arg(turno.nombre)
                                 .arg(turno.inicio.toString(QStringLiteral("HH:mm")))
                                 .arg(turno.fin.toString(QStringLiteral("HH:mm"))));
            }
            if (turno.recesoDespuesDeSlot != 2) {
                return falla(QStringLiteral("El receso del turno «%1» no cae tras la franja 2 (%2).")
                                 .arg(turno.nombre)
                                 .arg(turno.recesoDespuesDeSlot));
            }
            if (turno.recesoInicio != QTime(kTurnos[i].hRecesoIni, kTurnos[i].mRecesoIni) ||
                turno.recesoFin != QTime(kTurnos[i].hRecesoFin, kTurnos[i].mRecesoFin)) {
                return falla(QStringLiteral("El receso del turno «%1» no es %2–%3.")
                                 .arg(turno.nombre)
                                 .arg(turno.recesoInicio.toString(QStringLiteral("HH:mm")))
                                 .arg(turno.recesoFin.toString(QStringLiteral("HH:mm"))));
            }
        }
    }

    // 0bis. Eventos de los turnos (009-T7, RF-1, RF-3, RF-10): nombre, posición, orden único
    //    por posición, duración y, si declaran horario, par completo, diferencia coherente y
    //    contención en la ventana de su posición. La ventana y el par se comprueban con las
    //    funciones puras del servicio (`calculo_eventos`), para que el perfil y la base no
    //    diverjan.
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
            if (!eventoDentroDeVentana(turnoDto.eventos[i], turnoDto, evento.despuesDeSlot)) {
                const QPair<QTime, QTime> ventana =
                    ventanaDePosicion(turnoDto, evento.despuesDeSlot);
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
    }

    // Aulas (RF-5): exactamente 16, con nombre único.
    {
        QSet<QString> nombres;
        for (const AulaDef& aula : perfil.aulas) {
            if (aula.nombre.isEmpty()) {
                return falla(QStringLiteral("El perfil contiene un aula sin nombre."));
            }
            if (nombres.contains(aula.nombre)) {
                return falla(QStringLiteral("El aula «%1» está duplicada.").arg(aula.nombre));
            }
            nombres.insert(aula.nombre);
        }
        if (perfil.aulas.size() != 16) {
            return falla(QStringLiteral("El perfil debe tener 16 aulas (tiene %1).")
                             .arg(perfil.aulas.size()));
        }
    }

    // 0ter. Tipos de aula (009-T7, RF-5, RF-6, RF-10): catálogo no vacío y sin duplicados, cada
    //    aula con un tipo del catálogo y cada materia con un tipo requerido que tenga al menos
    //    un aula sembrada (N1). Así no se siembra una materia imposible de ubicar.
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

    // 1. Catálogo: nombres únicos, año válido y sin huérfanas.
    {
        QSet<QString> nombres;
        for (const MateriaAnio& materia : perfil.materias) {
            if (materia.nombre.isEmpty()) {
                return falla(QStringLiteral("El catálogo contiene una materia sin nombre."));
            }
            if (materia.anio < 1 || materia.anio > 5) {
                return falla(QStringLiteral("La materia «%1» tiene un año inválido (%2).")
                                 .arg(materia.nombre)
                                 .arg(materia.anio));
            }
            if (nombres.contains(materia.nombre)) {
                return falla(QStringLiteral("La materia «%1» está duplicada en el catálogo.")
                                 .arg(materia.nombre));
            }
            nombres.insert(materia.nombre);
        }
        for (const MateriaAnio& materia : perfil.materias) {
            bool usada = false;
            for (const PlanDef& plan : perfil.planes) {
                for (const CargaPlan& carga : plan.cargas) {
                    if (carga.materia == materia.nombre) {
                        usada = true;
                        break;
                    }
                }
                if (usada) {
                    break;
                }
            }
            if (!usada) {
                return falla(QStringLiteral("La materia «%1» del catálogo no aparece en ningún plan.")
                                 .arg(materia.nombre));
            }
        }
    }

    // 2. Progresión del 31059 (RF-2): áreas restringidas a sus años, en todos los planes.
    for (const PlanDef& plan : perfil.planes) {
        for (const CargaPlan& carga : plan.cargas) {
            if (esMateriaNueva(carga.materia)) {
                continue;
            }
            for (const ReglaProgresion& regla : kProgresion) {
                const QString prefijo = QString::fromUtf8(regla.area) + QStringLiteral(" ");
                if (carga.materia.startsWith(prefijo) &&
                    (carga.anio < regla.anioMin || carga.anio > regla.anioMax)) {
                    return falla(
                        QStringLiteral("La materia «%1» del plan %2 rompe la progresión del 31059 (año %3 fuera de %4–%5).")
                            .arg(carga.materia)
                            .arg(plan.codigo)
                            .arg(carga.anio)
                            .arg(regla.anioMin)
                            .arg(regla.anioMax));
                }
            }
        }
    }

    // 3. Cada plan: sin materias repetidas, sin años vacíos y sin horas negativas;
    //    Σ horas ≤ 40 en cada año (RF-2, RF-3).
    for (const PlanDef& plan : perfil.planes) {
        QSet<QString> vistas;
        for (const CargaPlan& carga : plan.cargas) {
            if (carga.horas < 0) {
                return falla(QStringLiteral("La materia «%1» del plan %2 tiene horas negativas.")
                                 .arg(carga.materia)
                                 .arg(plan.codigo));
            }
            if (vistas.contains(carga.materia)) {
                return falla(QStringLiteral("El plan %1 repite la materia «%2».")
                                 .arg(plan.codigo)
                                 .arg(carga.materia));
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
                return falla(QStringLiteral("El plan %1 supera 40 horas académicas en %2º (%3).")
                                 .arg(plan.codigo)
                                 .arg(anio)
                                 .arg(total));
            }
        }
    }

    // 3bis. Plan General (RF-2, plan §5.3): coincide exactamente con la tabla adaptada de
    //    §4.2 — 9/9/10/11/12 materias y Σ 40 h académicas por año, 51 cargas y horas por
    //    materia idénticas —, comparando contra la constante canónica (no contra el perfil).
    {
        const PlanDef* general = nullptr;
        for (const PlanDef& plan : perfil.planes) {
            if (plan.codigo == QStringLiteral("GEN")) {
                general = &plan;
                break;
            }
        }
        if (!general) {
            return falla(QStringLiteral("Falta el plan General (GEN)."));
        }

        const int materiasPorAnio[5] = {9, 9, 10, 11, 12};
        for (int anio = 1; anio <= 5; ++anio) {
            int contador = 0;
            for (const CargaPlan& carga : general->cargas) {
                if (carga.anio == anio) {
                    ++contador;
                }
            }
            if (contador != materiasPorAnio[anio - 1]) {
                return falla(QStringLiteral("Plan General, año %1: %2 materias; se esperaban %3.")
                                 .arg(anio)
                                 .arg(contador)
                                 .arg(materiasPorAnio[anio - 1]));
            }
            const int total = horasDeAnio(*general, anio);
            if (total != 40) {
                return falla(QStringLiteral("Plan General, año %1: suma %2 h académicas; "
                                            "se esperaban 40 (diferencia %3).")
                                 .arg(anio)
                                 .arg(total)
                                 .arg(total - 40));
            }
        }

        if (general->cargas.size() != 51) {
            return falla(QStringLiteral("El plan General tiene %1 cargas; se esperaban 51.")
                             .arg(general->cargas.size()));
        }

        for (const CargaPlan& carga : general->cargas) {
            const CargaCanonica canonica = cargaCanonica31059(carga.materia);
            if (canonica.anio < 0) {
                return falla(
                    QStringLiteral("Plan General: la materia «%1» no es del catálogo del 31059.")
                        .arg(carga.materia));
            }
            if (carga.anio != canonica.anio) {
                return falla(QStringLiteral("Plan General: la materia «%1» está en el año %2; "
                                            "se esperaba %3.")
                                 .arg(carga.materia)
                                 .arg(carga.anio)
                                 .arg(canonica.anio));
            }
            if (carga.horas != canonica.horas) {
                return falla(QStringLiteral("Plan General: la materia «%1» tiene %2 h académicas; "
                                            "se esperaban %3.")
                                 .arg(carga.materia)
                                 .arg(carga.horas)
                                 .arg(canonica.horas));
            }
        }
    }

    // 4. Derivados (RF-3): ≥2 materias nuevas de su eje en 4.º–5.º, usadas solo por su plan,
    //    y ≥2 áreas base con horas distintas de GEN.
    {
        const PlanDef* gen = nullptr;
        for (const PlanDef& plan : perfil.planes) {
            if (plan.codigo == QStringLiteral("GEN")) {
                gen = &plan;
                break;
            }
        }
        if (!gen) {
            return falla(QStringLiteral("Falta el plan General (GEN)."));
        }

        for (const PlanDef& plan : perfil.planes) {
            if (plan.codigo == QStringLiteral("GEN")) {
                continue;
            }
            int nuevas = 0;
            int variaciones = 0;
            for (const CargaPlan& carga : plan.cargas) {
                const MateriaNuevaDef* nueva = materiaNueva(carga.materia);
                if (nueva) {
                    if (carga.anio < 4) {
                        return falla(
                            QStringLiteral("La materia nueva «%1» del plan %2 está en un año anterior a 4.º.")
                                .arg(carga.materia)
                                .arg(plan.codigo));
                    }
                    if (QString::fromUtf8(nueva->plan) != plan.codigo) {
                        return falla(QStringLiteral("La materia nueva «%1» pertenece a %2, no a %3.")
                                         .arg(carga.materia)
                                         .arg(QString::fromUtf8(nueva->plan))
                                         .arg(plan.codigo));
                    }
                    ++nuevas;
                } else {
                    for (const CargaPlan& cargaGen : gen->cargas) {
                        if (cargaGen.materia == carga.materia && cargaGen.horas != carga.horas) {
                            ++variaciones;
                            break;
                        }
                    }
                }
            }
            if (nuevas < 2) {
                return falla(QStringLiteral("El plan %1 añade menos de 2 materias nuevas de su área.")
                                 .arg(plan.codigo));
            }
            if (variaciones < 2) {
                return falla(QStringLiteral("El plan %1 no varía las horas de al menos 2 áreas base.")
                                 .arg(plan.codigo));
            }
        }

        // Cada materia nueva se usa exactamente una vez y en su plan (RF-1, RF-3).
        for (const MateriaNuevaDef& nueva : kMateriasNuevas) {
            int usos = 0;
            for (const PlanDef& plan : perfil.planes) {
                for (const CargaPlan& carga : plan.cargas) {
                    if (carga.materia == QString::fromUtf8(nueva.nombre)) {
                        ++usos;
                    }
                }
            }
            if (usos == 0) {
                return falla(QStringLiteral("La materia nueva «%1» no aparece en su plan %2.")
                                 .arg(QString::fromUtf8(nueva.nombre))
                                 .arg(QString::fromUtf8(nueva.plan)));
            }
            if (usos > 1) {
                return falla(QStringLiteral("La materia nueva «%1» se usa en más de un plan.")
                                 .arg(QString::fromUtf8(nueva.nombre)));
            }
        }
    }

    // 5. Cursos (RF-4, RF-5): 13 cursos sin turno, año 1–5, sección de un carácter en
    //    mayúscula, pareja año+sección única y aula existente y distinta.
    {
        QSet<QString> nombresAula;
        for (const AulaDef& aula : perfil.aulas) {
            nombresAula.insert(aula.nombre);
        }
        QSet<QString> parejas;
        QSet<QString> aulasUsadas;
        for (const CursoDef& curso : perfil.cursos) {
            if (aulasUsadas.contains(curso.aula)) {
                return falla(QStringLiteral("El aula «%1» se repite entre cursos.")
                                 .arg(curso.aula));
            }
            if (curso.aula.isEmpty() || !nombresAula.contains(curso.aula)) {
                return falla(QStringLiteral("El curso %1º%2 tiene un aula inexistente («%3»).")
                                 .arg(curso.anio)
                                 .arg(curso.seccion)
                                 .arg(curso.aula));
            }
            if (curso.anio < 1 || curso.anio > 5) {
                return falla(QStringLiteral("El curso %1º%2 tiene un año inválido.")
                                 .arg(curso.anio)
                                 .arg(curso.seccion));
            }
            if (curso.seccion.size() != 1 || !curso.seccion.at(0).isUpper()) {
                return falla(QStringLiteral("El curso %1º%2 tiene una sección inválida.")
                                 .arg(curso.anio)
                                 .arg(curso.seccion));
            }
            const QString pareja = QStringLiteral("%1%2").arg(curso.anio).arg(curso.seccion);
            if (parejas.contains(pareja)) {
                return falla(QStringLiteral("La pareja año+sección «%1» se repite entre cursos.")
                                 .arg(pareja));
            }
            parejas.insert(pareja);
            aulasUsadas.insert(curso.aula);
        }
        if (perfil.cursos.size() != 13) {
            return falla(QStringLiteral("El perfil debe tener 13 cursos (tiene %1).")
                             .arg(perfil.cursos.size()));
        }
    }

    // 6. Recuentos esperados (plan §5.6, RF-1/RF-2): 57 materias, 4 planes y 210
    //    plan-materias. Los 2 turnos/2 recesos, 16 aulas y 13 cursos ya se comprueban
    //    arriba; aquí se cierra el conteo del catálogo y de los enlaces plan-materia.
    {
        if (perfil.materias.size() != 57) {
            return falla(
                QStringLiteral("El perfil tiene %1 materias en el catálogo; se esperaban 57.")
                    .arg(perfil.materias.size()));
        }
        if (perfil.planes.size() != 4) {
            return falla(QStringLiteral("El perfil tiene %1 planes; se esperaban 4.")
                             .arg(perfil.planes.size()));
        }
        int totalCargas = 0;
        for (const PlanDef& plan : perfil.planes) {
            totalCargas += plan.cargas.size();
        }
        if (totalCargas != 210) {
            return falla(
                QStringLiteral("El perfil tiene %1 materias de plan en total; se esperaban 210.")
                    .arg(totalCargas));
        }
    }

    return true;
}

} // namespace seed
