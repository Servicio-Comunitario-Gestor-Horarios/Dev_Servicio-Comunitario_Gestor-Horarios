#include "fixtures/centro_sin_ef.hpp"

#include <initializer_list>

namespace {

struct MateriaHoras {
    const char* nombre;
    int horas;
};

int agregarMateria(SolverConfig& config, const QString& nombre) {
    config.materias.append(MateriaSolverConfig{nombre, QStringLiteral("Aula común")});
    return config.materias.size() - 1;
}

void agregarAnio(SolverConfig& config, PlanSolverConfig& plan, int anio,
                 std::initializer_list<MateriaHoras> materias) {
    for (const MateriaHoras& materia : materias) {
        const int indice = agregarMateria(config, QString::fromUtf8(materia.nombre));
        plan.materias.append(PlanMateriaConfig{indice, anio, materia.horas});
    }
}

} // namespace

SolverConfig fixtureCentroSinEf() {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = true;
    config.dimensiones = {30, 0, 12, 24, 5, 16};
    config.franja_horaria = {40, 8};

    QVector<int> manana;
    QVector<int> tarde;
    for (int slot = 0; slot < 8; ++slot) manana.append(slot);
    for (int slot = 8; slot < 16; ++slot) tarde.append(slot);
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), manana, QStringLiteral("07:00"),
                    QStringLiteral("12:30")},
        TurnoConfig{QStringLiteral("tarde"), tarde, QStringLiteral("12:40"),
                    QStringLiteral("18:10")},
    };
    config.eventos = {
        EventoConfig{QStringLiteral("Receso"), QStringLiteral("manana"), 2, 1, 10, QString(),
                     QString()},
        EventoConfig{QStringLiteral("Receso"), QStringLiteral("tarde"), 2, 1, 10, QString(),
                     QString()},
    };

    for (int aula = 0; aula < 12; ++aula)
        config.aulas.append(AulaSolverConfig{
            QStringLiteral("Aula %1").arg(aula), QStringLiteral("Aula común"), 30});

    PlanSolverConfig plan31059{QStringLiteral("31059"), {}};
    agregarAnio(config, plan31059, 1,
                {{"Castellano", 4}, {"Inglés", 4}, {"Matemáticas", 4}, {"Arte", 4},
                 {"Cs. Naturales", 6}, {"GHC", 6}, {"Orientación", 2}, {"Grupos", 4}});
    agregarAnio(config, plan31059, 2,
                {{"Castellano", 4}, {"Inglés", 4}, {"Matemáticas", 4}, {"Arte", 4},
                 {"Cs. Naturales", 6}, {"GHC", 6}, {"Orientación", 2}, {"Grupos", 4}});
    agregarAnio(config, plan31059, 3,
                {{"Castellano", 4}, {"Inglés", 4}, {"Matemáticas", 4}, {"Física", 4},
                 {"Química", 4}, {"Biología", 4}, {"GHC", 4}, {"Orientación", 2},
                 {"Grupos", 4}});
    agregarAnio(config, plan31059, 4,
                {{"Castellano", 4}, {"Inglés", 4}, {"Matemáticas", 4}, {"Física", 4},
                 {"Química", 4}, {"Biología", 4}, {"GHC", 4}, {"FSN", 2},
                 {"Orientación", 2}, {"Grupos", 4}});
    agregarAnio(config, plan31059, 5,
                {{"Castellano", 3}, {"Inglés", 3}, {"Matemáticas", 4}, {"Física", 4},
                 {"Química", 4}, {"Biología", 4}, {"Cs. de la Tierra", 2}, {"GHC", 4},
                 {"FSN", 2}, {"Orientación", 2}, {"Grupos", 4}});

    PlanSolverConfig plan31060{QStringLiteral("31060"), {}};
    for (int anio = 1; anio <= 2; ++anio)
        agregarAnio(config, plan31060, anio,
                    {{"Lengua", 4}, {"Idiomas", 4}, {"Matemáticas", 4},
                     {"Biología/Ambiente", 4}, {"Física", 2}, {"Química", 2}, {"GHC", 6},
                     {"Orientación Vocacional", 4}, {"ITP", 6}});
    for (int anio = 3; anio <= 4; ++anio)
        agregarAnio(config, plan31060, anio,
                    {{"Lengua", 4}, {"Idiomas", 4}, {"Matemáticas", 4}, {"Biología", 4},
                     {"Física", 4}, {"Química", 4}, {"GHC", 4},
                     {"Orientación Vocacional", 2}, {"ITP", 6}});

    config.planes = {plan31059, plan31060};

    struct CursoFixture {
        int anio;
        const char* seccion;
        const char* turno;
        const char* plan;
        int aula;
    };
    const QVector<CursoFixture> cursos = {
        {1, "A", "manana", "31059", 0},  {1, "B", "manana", "31059", 1},
        {3, "A", "manana", "31059", 2},  {3, "B", "manana", "31059", 3},
        {3, "C", "manana", "31059", 4},  {4, "A", "manana", "31059", 5},
        {4, "B", "manana", "31059", 6},  {4, "C", "manana", "31059", 7},
        {5, "A", "manana", "31059", 8},  {5, "B", "manana", "31059", 9},
        {5, "C", "manana", "31059", 10}, {5, "D", "manana", "31059", 11},
        {2, "A", "tarde", "31059", 0},   {2, "B", "tarde", "31059", 1},
        {2, "C", "tarde", "31059", 2},
        {1, "A60", "tarde", "31060", 3}, {1, "B60", "tarde", "31060", 4},
        {1, "C60", "tarde", "31060", 5}, {2, "A60", "tarde", "31060", 6},
        {2, "B60", "tarde", "31060", 7}, {3, "A60", "tarde", "31060", 8},
        {3, "B60", "tarde", "31060", 9}, {4, "A60", "tarde", "31060", 10},
        {4, "B60", "tarde", "31060", 11},
    };
    for (const CursoFixture& curso : cursos)
        config.cursos.append(CursoSolverConfig{
            curso.anio, QString::fromUtf8(curso.seccion), QString::fromUtf8(curso.turno), 20,
            QString::fromUtf8(curso.plan), curso.aula});

    QVector<int> slotsTodos;
    for (int slot = 0; slot < 16; ++slot) slotsTodos.append(slot);
    QVector<Disponibilidad> disponibilidad;
    for (int dia = 0; dia < 5; ++dia)
        disponibilidad.append(Disponibilidad{dia, slotsTodos});

    // 30 docentes: cada entrada del catálogo (plan, año, materia) se asigna a un titular por
    // round-robin. El reparto reparte las entradas de forma determinista.
    for (int profesor = 0; profesor < 30; ++profesor) {
        ProfesorSolverConfig prof;
        prof.nombre = QStringLiteral("Prof %1").arg(profesor + 1);
        prof.horas_requeridas = -1;
        prof.horas_aula = 40;
        for (int materia = 0; materia < config.materias.size(); ++materia)
            if (materia % 30 == profesor) prof.materias_asignadas.append(materia);
        prof.disponibilidad = disponibilidad;
        config.profesores.append(prof);
    }

    config.dimensiones.num_materias = config.materias.size();
    return config;
}
