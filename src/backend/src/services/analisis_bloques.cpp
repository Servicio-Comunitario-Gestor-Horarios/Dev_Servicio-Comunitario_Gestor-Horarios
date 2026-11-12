#include "backend/services/analisis_bloques.hpp"

#include "backend/solver/config/bloques.hpp"
#include "backend/solver/config/contrato_solver.hpp"

#include <algorithm>

namespace {

int indiceCurso(const SolverConfig& config, const QString& clave) {
    for (int i = 0; i < config.cursos.size(); ++i)
        if (claveCurso(config.cursos[i].anio, config.cursos[i].seccion) == clave) return i;
    return -1;
}

int diaDominio(int diaGeneracion) { return (diaGeneracion + 1) % 7; }

} // namespace

AnalisisBloques analizarBloques(const HorarioSalida& horario, const SolverConfig& config) {
    AnalisisBloques analisis;

    for (auto itCurso = horario.horarios.constBegin(); itCurso != horario.horarios.constEnd();
         ++itCurso) {
        const QString& clave = itCurso.key();
        const int curso = indiceCurso(config, clave);
        if (curso < 0) continue;

        const CursoOutput& salida = itCurso.value();

        QSet<FranjaSemana> ocupadas;
        QMap<int, QSet<FranjaSemana>> porMateria;
        QMap<FranjaSemana, int> materiaPorFranja;
        QMap<FranjaSemana, int> aulaPorFranja;
        for (const DiaOutput& dia : salida.dias) {
            for (const AsignacionOutput& asignacion : dia.asignaciones) {
                const FranjaSemana franja{dia.dia, asignacion.slot};
                ocupadas.insert(franja);
                porMateria[asignacion.materia].insert(franja);
                materiaPorFranja.insert(franja, asignacion.materia);
                aulaPorFranja.insert(franja, asignacion.aula);
            }
        }

        CursoSalida agregado;
        agregado.curso = clave;

        // Bloques de dos horas del curso (pares desde el inicio de cada tramo).
        const QVector<rejilla::Par> pares =
            rejilla::paresDeCurso(config, curso, ocupadas);
        for (const rejilla::Par& par : pares) {
            const FranjaSemana inicio{par.dia, par.inicio};
            const FranjaSemana fin{par.dia, par.inicio + 1};
            BloqueSalida bloque;
            bloque.curso = clave;
            bloque.diaDominio = diaDominio(par.dia);
            bloque.slotInicio = par.inicio;
            bloque.slotFin = par.inicio + 1;
            bloque.materiaA = materiaPorFranja.value(inicio, -1);
            bloque.materiaB = materiaPorFranja.value(fin, -1);
            bloque.compartido = bloque.materiaA != bloque.materiaB;
            bloque.aula = aulaPorFranja.value(inicio, -1);
            if (bloque.compartido) {
                ++analisis.bloquesCompartidos;
                ++agregado.bloquesCompartidos;
            } else {
                ++analisis.bloquesSimples;
                ++agregado.bloquesSimples;
            }
            analisis.bloques.append(bloque);
        }

        agregado.bloquesDeUno = rejilla::bloquesDeUno(config, curso, ocupadas);
        agregado.desvio = rejilla::desvioDeCurso(config, curso, porMateria);
        agregado.tope = rejilla::cotaDeCurso(config, curso);
        analisis.bloquesDeUno += agregado.bloquesDeUno;
        analisis.desvio += agregado.desvio;
        analisis.tope += agregado.tope;

        for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso])) {
            const int materia = mc.materiaIDx;
            const int horas = bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);
            const QSet<FranjaSemana> franjas = porMateria.value(materia);

            MateriaSalida salidaMateria;
            salidaMateria.curso = clave;
            salidaMateria.materia = materia;
            salidaMateria.resto = horas % 2;
            salidaMateria.franjasDescubiertas =
                rejilla::franjasDescubiertasDeMateria(config, curso, franjas);
            const QVector<int> dias = rejilla::diasDeMateria(config, curso, materia, franjas);
            for (int dia : dias) salidaMateria.diasDominio.append(diaDominio(dia));
            salidaMateria.incumplimientosSeparacion =
                rejilla::incumplimientosSeparacion(dias);

            // Días de dominio implicados en pares consecutivos (se informan, no son conflicto).
            QSet<int> diasIncumplidos;
            for (int i = 1; i < dias.size(); ++i) {
                if (dias[i] - dias[i - 1] == 1) {
                    diasIncumplidos.insert(diaDominio(dias[i - 1]));
                    diasIncumplidos.insert(diaDominio(dias[i]));
                }
            }
            QVector<int> diasIncumplidosOrdenados(diasIncumplidos.begin(),
                                                  diasIncumplidos.end());
            std::sort(diasIncumplidosOrdenados.begin(), diasIncumplidosOrdenados.end());
            salidaMateria.diasIncumplidosSeparacion = diasIncumplidosOrdenados;

            // Apariciones: bloques (simples o compartidos) en los que participa.
            for (const rejilla::Par& par : pares) {
                const int a = materiaPorFranja.value(FranjaSemana{par.dia, par.inicio}, -1);
                const int b = materiaPorFranja.value(FranjaSemana{par.dia, par.inicio + 1}, -1);
                if (a != materia && b != materia) continue;
                AparicionSalida aparicion;
                aparicion.diaDominio = diaDominio(par.dia);
                aparicion.slotInicio = par.inicio;
                aparicion.slotFin = par.inicio + 1;
                salidaMateria.franjas.append(aparicion);
            }
            salidaMateria.apariciones = salidaMateria.franjas.size();

            agregado.incumplimientosSeparacion += salidaMateria.incumplimientosSeparacion;
            analisis.incumplimientosSeparacion += salidaMateria.incumplimientosSeparacion;
            analisis.materias.append(salidaMateria);
        }

        // Huecos: tramos de franjas libres por día (el turno del curso, ordenado).
        QVector<int> franjasTurno = slotsTurno(config, config.cursos[curso].turno);
        std::sort(franjasTurno.begin(), franjasTurno.end());
        for (int dia = 0; dia < config.dimensiones.num_dias; ++dia) {
            int i = 0;
            while (i < franjasTurno.size()) {
                if (ocupadas.contains(FranjaSemana{dia, franjasTurno[i]})) {
                    ++i;
                    continue;
                }
                const int inicio = i;
                while (i < franjasTurno.size() &&
                       !ocupadas.contains(FranjaSemana{dia, franjasTurno[i]})) ++i;
                const int fin = i - 1;

                HuecoSalida hueco;
                hueco.curso = clave;
                hueco.diaDominio = diaDominio(dia);
                hueco.slotInicio = franjasTurno[inicio];
                hueco.slotFin = franjasTurno[fin];
                hueco.esViernes = (dia == 4);
                analisis.huecos.append(hueco);

                const int franjasHueco = fin - inicio + 1;
                agregado.huecos += franjasHueco;
                bool hayClaseDespues = false;
                for (int j = fin + 1; j < franjasTurno.size(); ++j)
                    if (ocupadas.contains(FranjaSemana{dia, franjasTurno[j]})) {
                        hayClaseDespues = true;
                        break;
                    }
                if (hayClaseDespues) analisis.huecosNoFinal += franjasHueco;
                if (dia != 4) {
                    analisis.huecosNoViernes += franjasHueco;
                    agregado.huecosNoViernes += franjasHueco;
                }
            }
        }

        analisis.cursos.append(agregado);
    }

    return analisis;
}
