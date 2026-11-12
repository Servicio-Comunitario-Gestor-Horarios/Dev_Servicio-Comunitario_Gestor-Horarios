#include <QTest>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioCargaHoraria.hpp>
#include <backend/services/ServicioMaterias.hpp>
#include <backend/services/ServicioPlanesEstudio.hpp>
#include <backend/services/ServicioTiposAula.hpp>

namespace {

/// Id de la fila del tipo general que deja el esquema en una base nueva (RF-1).
int idTipoGeneral(QSqlDatabase& db)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id FROM TiposAula WHERE clave = 'aula de estudio'"));
    if (!query.exec() || !query.next())
        return -1;
    return query.value(0).toInt();
}

} // namespace

class TestServicioPlanesEstudio : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QString dbPath = m_tempDir->filePath("test_planes.db");
        QVERIFY(m_dbManager->initialize(dbPath));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioPlanesEstudio(m_dbManager->database()));

        m_tipos.reset(new ServicioTiposAula(m_dbManager->database()));
        m_idTipoAula = idTipoGeneral(m_dbManager->database());
        QVERIFY(m_idTipoAula > 0);
    }

    void cleanup() {
        m_tipos.reset();
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Tests ──────────────────────────────────────────────────────────────

    void crearPlan_exitoso() {
        auto resultado = m_servicio->crearPlan("PLAN-001", "Plan de Ciencias", "Ciencias Naturales");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.codigo, "PLAN-001");
        QCOMPARE(resultado.valor.nombre, "Plan de Ciencias");
        QCOMPARE(resultado.valor.descripcion, "Ciencias Naturales");
    }

    void crearPlan_sinDescripcion() {
        auto resultado = m_servicio->crearPlan("PLAN-002", "Plan de Letras");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.codigo, "PLAN-002");
        QCOMPARE(resultado.valor.nombre, "Plan de Letras");
        QVERIFY(resultado.valor.descripcion.isEmpty());
    }

    void crearPlan_codigoVacio_falla() {
        auto resultado = m_servicio->crearPlan("", "Plan");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede estar vacío"));
    }

    void crearPlan_nombreVacio_falla() {
        auto resultado = m_servicio->crearPlan("PLAN-003", "");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede estar vacío"));
    }

    void crearPlan_codigoDuplicado_falla() {
        auto r1 = m_servicio->crearPlan("PLAN-004", "Plan A");
        QVERIFY(r1.ok);

        auto r2 = m_servicio->crearPlan("PLAN-004", "Plan B");
        QVERIFY(!r2.ok);
        QVERIFY(r2.mensajeError.contains("Ya existe un plan"));
    }

    void obtenerPlan_exitoso() {
        auto creado = m_servicio->crearPlan("PLAN-005", "Plan Test", "Descripción Test");
        QVERIFY(creado.ok);

        auto obtenido = m_servicio->obtenerPlan("PLAN-005");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.codigo, "PLAN-005");
        QCOMPARE(obtenido.valor.nombre, "Plan Test");
        QCOMPARE(obtenido.valor.descripcion, "Descripción Test");
    }

    void obtenerPlan_inexistente_falla() {
        auto resultado = m_servicio->obtenerPlan("PLAN-999");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void listarPlanes() {
        m_servicio->crearPlan("Z-PLAN", "Plan Z");
        m_servicio->crearPlan("A-PLAN", "Plan A");
        m_servicio->crearPlan("M-PLAN", "Plan M");

        auto lista = m_servicio->listarPlanes().valor;
        QCOMPARE(lista.size(), 3);

        // Ordenado por código
        QCOMPARE(lista[0].codigo, "A-PLAN");
        QCOMPARE(lista[1].codigo, "M-PLAN");
        QCOMPARE(lista[2].codigo, "Z-PLAN");
    }

    void actualizarPlan_exitoso() {
        auto creado = m_servicio->crearPlan("PLAN-006", "Plan Original");
        QVERIFY(creado.ok);

        auto actualizado = m_servicio->actualizarPlan(
            "PLAN-006", "Plan Actualizado", "Nueva descripción"
        );

        QVERIFY(actualizado.ok);
        QCOMPARE(actualizado.valor.nombre, "Plan Actualizado");
        QCOMPARE(actualizado.valor.descripcion, "Nueva descripción");
    }

    void actualizarPlan_inexistente_falla() {
        auto resultado = m_servicio->actualizarPlan("PLAN-999", "Plan");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void eliminarPlan_exitoso() {
        auto creado = m_servicio->crearPlan("PLAN-007", "Plan a Eliminar");
        QVERIFY(creado.ok);

        bool eliminado = m_servicio->eliminarPlan("PLAN-007");
        QVERIFY(eliminado);

        auto obtenido = m_servicio->obtenerPlan("PLAN-007");
        QVERIFY(!obtenido.ok);
    }

    void eliminarPlan_conMaterias_falla() {
        auto plan = m_servicio->crearPlan("PLAN-008", "Plan con Materias");
        QVERIFY(plan.ok);

        // Crear una materia y asociarla al plan
        QSqlQuery query(m_dbManager->database());
        query.prepare("INSERT INTO Materias (nombre, id_tipo_aula) "
                      "VALUES ('Materia Test', :idTipo)");
        query.bindValue(":idTipo", m_idTipoAula);
        QVERIFY(query.exec());
        int materiaId = query.lastInsertId().toInt();

        query.prepare(
            "INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas) "
            "VALUES ('PLAN-008', :id, 1, 4)"
        );
        query.bindValue(":id", materiaId);
        QVERIFY(query.exec());

        bool eliminado = m_servicio->eliminarPlan("PLAN-008");
        QVERIFY(!eliminado); // No debe eliminar porque tiene materias asociadas
    }

    void eliminarPlan_inexistente_falla() {
        bool eliminado = m_servicio->eliminarPlan("PLAN-999");
        QVERIFY(!eliminado);
    }

    void eliminarPlan_codigoVacio_falla() {
        bool eliminado = m_servicio->eliminarPlan("");
        QVERIFY(!eliminado);
    }

    void obtenerPlan_codigoLargo_falla() {
        QString codigoLargo(21, 'X');
        auto resultado = m_servicio->obtenerPlan(codigoLargo);
        QVERIFY(!resultado.ok);
    }

    // ── Plan completo (Issue #97) ────────────────────────────────────────

    void crearPlanCompleto_exitoso() {
        const int mat1 = crearMateria("Matemática");
        const int mat2 = crearMateria("Física");

        QVector<PlanMateriaDTO> materias;
        materias.append(PlanMateriaDTO{mat1, 1, 3});
        materias.append(PlanMateriaDTO{mat2, 1, 2});

        auto resultado = m_servicio->crearPlanCompleto(
            "2001", "Plan Numérico", "Descripción", materias);

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.codigo, "2001");
        QCOMPARE(resultado.valor.nombre, "Plan Numérico");

        const auto carga = cargaDe("2001");
        QCOMPARE(carga.size(), 2);
        QCOMPARE(carga[0].horas + carga[1].horas, 5);
    }

    void crearPlanCompleto_codigoNoNumerico_falla() {
        const int mat1 = crearMateria("Historia");
        QVector<PlanMateriaDTO> materias;
        materias.append(PlanMateriaDTO{mat1, 1, 2});

        auto resultado = m_servicio->crearPlanCompleto(
            "PLAN-X", "Plan con letras", QString(), materias);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("solo números"));
    }

    void crearPlanCompleto_sinMaterias_falla() {
        auto resultado = m_servicio->crearPlanCompleto(
            "2002", "Plan vacío", QString(), {});

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("al menos una materia"));
    }

    void crearPlanCompleto_materiaDuplicada_falla() {
        const int mat1 = crearMateria("Biología");
        QVector<PlanMateriaDTO> materias;
        materias.append(PlanMateriaDTO{mat1, 1, 2});
        materias.append(PlanMateriaDTO{mat1, 2, 2});

        auto resultado = m_servicio->crearPlanCompleto(
            "2003", "Plan duplicado", QString(), materias);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("repetir"));
    }

    void crearPlanCompleto_excedeHoras_falla() {
        const int mat1 = crearMateria("Arte");
        const int mat2 = crearMateria("Música");
        QVector<PlanMateriaDTO> materias;
        materias.append(PlanMateriaDTO{mat1, 1, 30});
        materias.append(PlanMateriaDTO{mat2, 1, 20});

        auto resultado = m_servicio->crearPlanCompleto(
            "2004", "Plan sobrecargado", QString(), materias);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("máximo"));
    }

    void crearPlanCompleto_nombreDuplicado_falla() {
        const int mat1 = crearMateria("Ética");
        QVector<PlanMateriaDTO> materias;
        materias.append(PlanMateriaDTO{mat1, 1, 2});

        auto primero = m_servicio->crearPlanCompleto(
            "2005", "Nombre repetido", QString(), materias);
        QVERIFY(primero.ok);

        auto segundo = m_servicio->crearPlanCompleto(
            "2006", "nombre repetido", QString(), materias);
        QVERIFY(!segundo.ok);
        QVERIFY(segundo.mensajeError.contains("Ya existe"));
    }

    void actualizarPlanCompleto_reemplazaMaterias() {
        const int mat1 = crearMateria("Lengua");
        const int mat2 = crearMateria("Literatura");

        QVector<PlanMateriaDTO> materias;
        materias.append(PlanMateriaDTO{mat1, 1, 2});
        QVERIFY(m_servicio->crearPlanCompleto(
            "2007", "Plan editable", QString(), materias).ok);

        QVector<PlanMateriaDTO> nuevas;
        nuevas.append(PlanMateriaDTO{mat2, 1, 4});

        auto resultado = m_servicio->actualizarPlanCompleto(
            "2007", "Plan editado", "Otra", nuevas);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Plan editado");

        const auto carga = cargaDe("2007");
        QCOMPARE(carga.size(), 1);
        QCOMPARE(carga[0].idMateria, mat2);
        QCOMPARE(carga[0].horas, 4);
    }

private:
    QVector<CargaHorariaDTO> cargaDe(const QString& codigo) {
        ServicioCargaHoraria servicio(m_dbManager->database());
        return servicio.listarCargaPorPlan(codigo);
    }

    int crearMateria(const QString& nombre) {
        ServicioMaterias servicio(m_dbManager->database());
        const auto resultado = servicio.crearMateria(nombre, QString(), m_idTipoAula);
        return resultado.ok ? resultado.valor.id : -1;
    }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioPlanesEstudio> m_servicio;
    std::unique_ptr<ServicioTiposAula> m_tipos;
    int m_idTipoAula = -1;
};

QTEST_MAIN(TestServicioPlanesEstudio)
#include "test_servicio_planes_estudio.moc"
