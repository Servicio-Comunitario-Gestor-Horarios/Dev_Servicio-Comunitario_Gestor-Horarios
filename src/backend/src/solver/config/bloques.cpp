#include "backend/solver/config/bloques.hpp"

#include <algorithm>

namespace rejilla {
namespace {

/// Horas (franjas) de una materia-curso según su plan.
int horasDeMateria(const SolverConfig& config, int curso, int materia) {
    if (curso < 0 || curso >= config.cursos.size()) return 0;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
        if (mc.materiaIDx == materia)
            return bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);
    return 0;
}

/// Horas semanales totales del curso.
int horasDeCurso(const SolverConfig& config, int curso) {
    if (curso < 0 || curso >= config.cursos.size()) return 0;
    int total = 0;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
        total += bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);
    return total;
}

} // namespace

QVector<QVector<int>> segmentosDelCurso(const SolverConfig& config, int curso) {
    if (curso < 0 || curso >= config.cursos.size()) return {};
    const QString& nombreTurno = config.cursos[curso].turno;
    for (const TurnoConfig& turno : config.turnos.turnos)
        if (turno.nombre == nombreTurno)
            return segmentosTurno(turno, config.eventos);
    return {};
}

QVector<Tramo> tramosDeCurso(const SolverConfig& config, int curso,
                             const QSet<FranjaSemana>& ocupadas) {
    QVector<Tramo> tramos;
    if (curso < 0 || curso >= config.cursos.size()) return tramos;

    const QVector<QVector<int>> segmentos = segmentosDelCurso(config, curso);
    for (int dia = 0; dia < config.dimensiones.num_dias; ++dia) {
        for (int segmento = 0; segmento < segmentos.size(); ++segmento) {
            const QVector<int>& franjas = segmentos[segmento];
            int k = 0;
            while (k < franjas.size()) {
                if (!ocupadas.contains(FranjaSemana{dia, franjas[k]})) {
                    ++k;
                    continue;
                }
                const int inicio = k;
                while (k < franjas.size() &&
                       ocupadas.contains(FranjaSemana{dia, franjas[k]})) {
                    ++k;
                }
                tramos.append(Tramo{dia, segmento, franjas[inicio], k - inicio});
            }
        }
    }
    return tramos;
}

QVector<Par> paresDeCurso(const SolverConfig& config, int curso,
                          const QSet<FranjaSemana>& ocupadas) {
    QVector<Par> pares;
    for (const Tramo& tramo : tramosDeCurso(config, curso, ocupadas))
        for (int k = 0; k + 1 < tramo.longitud; k += 2)
            pares.append(Par{tramo.dia, tramo.segmento, tramo.inicio + k});
    return pares;
}

int franjasDescubiertasDeMateria(const SolverConfig& config, int curso,
                                 const QSet<FranjaSemana>& ocupadasMateria) {
    if (curso < 0 || curso >= config.cursos.size()) return 0;

    const QVector<QVector<int>> segmentos = segmentosDelCurso(config, curso);
    int descubiertas = 0;
    for (int dia = 0; dia < config.dimensiones.num_dias; ++dia) {
        for (const QVector<int>& franjas : segmentos) {
            int k = 0;
            while (k < franjas.size()) {
                if (!ocupadasMateria.contains(FranjaSemana{dia, franjas[k]})) {
                    ++k;
                    continue;
                }
                int longitud = 0;
                while (k < franjas.size() &&
                       ocupadasMateria.contains(FranjaSemana{dia, franjas[k]})) {
                    ++longitud;
                    ++k;
                }
                // Emparejado desde el inicio del tramo: sobra la última si es impar.
                descubiertas += longitud % 2;
            }
        }
    }
    return descubiertas;
}

int franjasDescubiertasDeCurso(const SolverConfig& config, int curso,
                               const QMap<int, QSet<FranjaSemana>>& ocupadasPorMateria) {
    if (curso < 0 || curso >= config.cursos.size()) return 0;

    int total = 0;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
        total += franjasDescubiertasDeMateria(
            config, curso, ocupadasPorMateria.value(mc.materiaIDx));
    return total;
}

int desvioDeMateria(const SolverConfig& config, int curso, int materia,
                    const QSet<FranjaSemana>& ocupadasMateria) {
    const int tope = horasDeMateria(config, curso, materia) % 2;
    const int descubiertas =
        franjasDescubiertasDeMateria(config, curso, ocupadasMateria);
    return std::max(0, descubiertas - tope);
}

int desvioDeCurso(const SolverConfig& config, int curso,
                  const QMap<int, QSet<FranjaSemana>>& ocupadasPorMateria) {
    if (curso < 0 || curso >= config.cursos.size()) return 0;

    int desvio = 0;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
        desvio += desvioDeMateria(config, curso, mc.materiaIDx,
                                  ocupadasPorMateria.value(mc.materiaIDx));
    return desvio;
}

QVector<int> diasDeMateria(const SolverConfig& config, int curso, int materia,
                           const QSet<FranjaSemana>& ocupadasMateria) {
    if (curso < 0 || curso >= config.cursos.size()) return {};

    bool existe = false;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
        if (mc.materiaIDx == materia) {
            existe = true;
            break;
        }
    if (!existe) return {};

    QSet<int> dias;
    for (const FranjaSemana& franja : ocupadasMateria)
        if (franja.dia >= 0 && franja.dia < config.dimensiones.num_dias)
            dias.insert(franja.dia);

    QVector<int> ordenados(dias.begin(), dias.end());
    std::sort(ordenados.begin(), ordenados.end());
    return ordenados;
}

int incumplimientosSeparacion(const QVector<int>& dias) {
    QVector<int> ordenados = dias;
    std::sort(ordenados.begin(), ordenados.end());
    ordenados.erase(std::unique(ordenados.begin(), ordenados.end()), ordenados.end());

    int incumplimientos = 0;
    for (int i = 1; i < ordenados.size(); ++i)
        if (ordenados[i] - ordenados[i - 1] == 1) ++incumplimientos;
    return incumplimientos;
}

int bloquesDeUno(const SolverConfig& config, int curso,
                 const QSet<FranjaSemana>& ocupadas) {    int sueltas = 0;
    for (const Tramo& tramo : tramosDeCurso(config, curso, ocupadas))
        sueltas += tramo.longitud % 2;
    return sueltas;
}

int aparicionesEsperadas(int horas) {
    if (horas <= 0) return 0;
    return (horas + 1) / 2;
}

int cotaDeCurso(const SolverConfig& config, int curso) {
    if (curso < 0 || curso >= config.cursos.size()) return 0;

    int cota = 0;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
        cota += bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos) % 2;
    return cota;
}

bool cumpleTopeDeCurso(const SolverConfig& config, int curso,
                       const QMap<int, QSet<FranjaSemana>>& ocupadasPorMateria) {
    return franjasDescubiertasDeCurso(config, curso, ocupadasPorMateria) <=
           cotaDeCurso(config, curso);
}

bool causaCincoDeCurso(const SolverConfig& config, int curso, QString* motivo) {
    if (motivo) motivo->clear();
    if (curso < 0 || curso >= config.cursos.size()) return false;

    const CursoSolverConfig& cursoCfg = config.cursos[curso];
    const QString clave = claveCurso(cursoCfg.anio, cursoCfg.seccion);

    // 1) Total semanal impar: no se cubre con bloques de dos.
    const int total = horasDeCurso(config, curso);
    if (total % 2 != 0) {
        if (motivo)
            *motivo = QStringLiteral(
                          "Causa (5) conflicto de bloques: el curso %1 suma %2 horas semanales "
                          "(impar) y no se puede repartir en bloques de dos.")
                          .arg(clave)
                          .arg(total);
        return true;
    }

    // 2) Más apariciones que días en alguna materia (con 5 días, h ≥ 11).
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, cursoCfg)) {
        const int horas = bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);
        const int apariciones = aparicionesEsperadas(horas);
        if (apariciones > config.dimensiones.num_dias) {
            const QString nombre =
                (mc.materiaIDx >= 0 && mc.materiaIDx < config.materias.size())
                    ? config.materias[mc.materiaIDx].nombre
                    : QString::number(mc.materiaIDx);
            if (motivo)
                *motivo = QStringLiteral(
                              "Causa (5) conflicto de bloques: la materia «%1» del curso %2 "
                              "necesita %3 apariciones y la semana solo tiene %4 días.")
                              .arg(nombre)
                              .arg(clave)
                              .arg(apariciones)
                              .arg(config.dimensiones.num_dias);
            return true;
        }
    }

    return false;
}

PesosBlandos pesosBlandos(const SolverConfig& config) {
    int maxHuecos = 0;
    for (int curso = 0; curso < config.cursos.size(); ++curso)
        maxHuecos += std::max(0, franjasSemanales(config, config.cursos[curso]) -
                                    horasDeCurso(config, curso));
    return PesosBlandos{maxHuecos + 1, 1};
}
} // namespace rejilla
