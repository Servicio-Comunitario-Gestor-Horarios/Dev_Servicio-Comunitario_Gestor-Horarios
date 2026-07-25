#include <gtest/gtest.h>
#include "backend/services/ServicioExportacion.hpp"

using namespace backend;
using namespace backend::services;

class ServicioExportacionTest : public ::testing::Test {
protected:
    SolverConfig config;
    HorarioSalida horario;

    void SetUp() override {
        // Configurar entidades ficticias en SolverConfig
        MateriaConfig mat0; mat0.nombre = "Matematica";
        MateriaConfig mat1; mat1.nombre = "Fisica, Avanzada"; // Con coma para probar escape
        config.materias = {mat0, mat1};

        ProfesorConfig prof0; prof0.nombre = "Prof. Garcia";
        ProfesorConfig prof1; prof1.nombre = "Prof. Martinez";
        config.profesores = {prof0, prof1};

        AulaConfig aula0; aula0.nombre = "Aula 101";
        config.aulas = {aula0};

        // Configurar HorarioSalida ficticio
        CursoOutput curso1A;
        curso1A.turno = "manana";
        
        DiaOutput dia0;
        dia0.dia = 0; // Lunes
        dia0.asignaciones.push_back({0, 0, 0, 0}); // Slot 0: Mat, Garcia, Aula 101
        dia0.asignaciones.push_back({1, 1, 1, 0}); // Slot 1: Fis, Martinez, Aula 101

        curso1A.dias.push_back(dia0);
        horario.horarios["1A"] = curso1A;
    }
};

TEST_F(ServicioExportacionTest, ExportarJsonExitoso) {
    QJsonObject json = ServicioExportacion::exportarJson(horario);
    EXPECT_TRUE(json.contains("horarios"));
    EXPECT_TRUE(json["horarios"].toObject().contains("1A"));
}

TEST_F(ServicioExportacionTest, ExportarCsvCursosValido) {
    auto res = ServicioExportacion::exportarCsvCursos(horario, config);
    ASSERT_TRUE(res.esExitoso());

    QString csv = res.obtenerValor();
    EXPECT_TRUE(csv.contains("Curso: 1A (Turno: manana)"));
    EXPECT_TRUE(csv.contains("Dia,Slot 0,Slot 1,Slot 2,Slot 3,Slot 4,Slot 5"));
    EXPECT_TRUE(csv.contains("Matematica (Prof. Garcia / Aula 101)"));
    // Verificar escapado de comas en la materia
    EXPECT_TRUE(csv.contains("\"Fisica, Avanzada (Prof. Martinez / Aula 101)\""));
}

TEST_F(ServicioExportacionTest, ExportarCsvProfesoresValido) {
    auto res = ServicioExportacion::exportarCsvProfesores(horario, config);
    ASSERT_TRUE(res.esExitoso());

    QString csv = res.obtenerValor();
    EXPECT_TRUE(csv.contains("Profesor: Prof. Garcia"));
    EXPECT_TRUE(csv.contains("Profesor: Prof. Martinez"));
    EXPECT_TRUE(csv.contains("1A - Matematica"));
    EXPECT_TRUE(csv.contains("1A - \"Fisica, Avanzada\""));
}

TEST_F(ServicioExportacionTest, ErrorIndiceFueraDeRango) {
    // Insertar un índice inválido en las asignaciones
    horario.horarios["1A"].dias[0].asignaciones.push_back({2, 99, 0, 0}); // Materia 99 no existe

    auto resCurso = ServicioExportacion::exportarCsvCursos(horario, config);
    EXPECT_FALSE(resCurso.esExitoso());
    EXPECT_TRUE(resCurso.obtenerError().contains("out-of-bounds") || 
                resCurso.obtenerError().contains("fuera de rango"));

    auto resProf = ServicioExportacion::exportarCsvProfesores(horario, config);
    EXPECT_FALSE(resProf.esExitoso());
}