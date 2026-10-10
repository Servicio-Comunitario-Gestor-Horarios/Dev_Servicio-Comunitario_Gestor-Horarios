#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QTime>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioProfesor.hpp>

#include <backend/services/ServicioMaterias.hpp>

class TestServicioProfesor : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QString dbPath = m_tempDir->filePath("test_profesor.db");
        QVERIFY(m_dbManager->initialize(dbPath));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioProfesor(m_dbManager->database()));
        m_servicioMaterias.reset(new ServicioMaterias(m_dbManager->database()));
    }

    void cleanup() {
        m_servicioMaterias.reset();
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Helpers ────────────────────────────────────────────────────────────

    FranjaHoraria franja(int dia, int hIni, int mIni, int hFin, int mFin) {
        FranjaHoraria f;
        f.dia    = dia;
        f.inicio = QTime(hIni, mIni);
        f.fin    = QTime(hFin, mFin);
        return f;
    }

    // ─── crearProfesor ──────────────────────────────────────────────────────

    void crearProfesor_exitoso() {
        auto resultado = m_servicio->crearProfesor(
            "P-001", "Dr. García", "garcia@uni.edu", "555-1234");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.id,       QString("P-001"));
        QCOMPARE(resultado.valor.nombre,   QString("Dr. García"));
        QCOMPARE(resultado.valor.email,    QString("garcia@uni.edu"));
        QCOMPARE(resultado.valor.telefono, QString("555-1234"));
        QVERIFY(resultado.valor.disponibilidad.isEmpty());
        QVERIFY(resultado.valor.materias.isEmpty());
    }

    void crearProfesor_sinTelefono() {
        auto resultado = m_servicio->crearProfesor(
            "P-002", "Dra. López", "lopez@uni.edu");

        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.telefono.isEmpty());
    }

    void crearProfesor_idVacio_falla() {
        auto resultado = m_servicio->crearProfesor(
            "", "Dr. García", "garcia@uni.edu");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("ID"));
    }

    void crearProfesor_nombreVacio_falla() {
        auto resultado = m_servicio->crearProfesor(
            "P-003", "   ", "x@uni.edu");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("nombre"));
    }

    void crearProfesor_emailVacio_falla() {
        auto resultado = m_servicio->crearProfesor("P-004", "Dr. X", "");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("email"));
    }

    void crearProfesor_emailInvalido_falla() {
        auto resultado = m_servicio->crearProfesor(
            "P-005", "Dr. X", "sin_arroba_punto");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("formato"));
    }

    void crearProfesor_idDuplicado_falla() {
        QVERIFY(m_servicio->crearProfesor(
            "P-100", "Dr. Uno", "uno@uni.edu").ok);

        auto r2 = m_servicio->crearProfesor(
            "P-100", "Dr. Dos", "dos@uni.edu");
        QVERIFY(!r2.ok);
        QCOMPARE(r2.codigoError, -2);
    }

    void crearProfesor_nombreDuplicado_falla() {
        QVERIFY(m_servicio->crearProfesor(
            "P-200", "Dr. Repetido", "a@uni.edu").ok);

        auto r2 = m_servicio->crearProfesor(
            "P-201", "Dr. Repetido", "b@uni.edu");
        QVERIFY(!r2.ok);
        QCOMPARE(r2.codigoError, -2);
        QVERIFY(r2.mensajeError.contains("nombre"));
    }

    void crearProfesor_emailDuplicado_falla() {
        QVERIFY(m_servicio->crearProfesor(
            "P-300", "Dr. A", "mismo@uni.edu").ok);

        auto r2 = m_servicio->crearProfesor(
            "P-301", "Dr. B", "mismo@uni.edu");
        QVERIFY(!r2.ok);
        QCOMPARE(r2.codigoError, -2);
        QVERIFY(r2.mensajeError.contains("email"));
    }

    // ─── obtenerProfesor ────────────────────────────────────────────────────

    void obtenerProfesor_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-400", "Dr. Pérez", "perez@uni.edu", "555-0001").ok);

        auto obtenido = m_servicio->obtenerProfesor("P-400");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.nombre,   QString("Dr. Pérez"));
        QCOMPARE(obtenido.valor.email,    QString("perez@uni.edu"));
        QCOMPARE(obtenido.valor.telefono, QString("555-0001"));
    }

    void obtenerProfesor_inexistente_falla() {
        auto resultado = m_servicio->obtenerProfesor("NO-EXISTE");
        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.codigoError, -3);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void obtenerProfesor_idVacio_falla() {
        auto resultado = m_servicio->obtenerProfesor("   ");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("inválido"));
    }

    // ─── listarProfesores ───────────────────────────────────────────────────

    void listarProfesores_ordenadoPorNombre() {
        QVERIFY(m_servicio->crearProfesor("P-A", "Zapata",  "z@uni.edu").ok);
        QVERIFY(m_servicio->crearProfesor("P-B", "Álvarez", "a@uni.edu").ok);
        QVERIFY(m_servicio->crearProfesor("P-C", "Martínez","m@uni.edu").ok);

        auto lista = m_servicio->listarProfesores().valor;
        QCOMPARE(lista.size(), 3);
        QCOMPARE(lista[0].nombre, QString("Álvarez"));
        QCOMPARE(lista[1].nombre, QString("Martínez"));
        QCOMPARE(lista[2].nombre, QString("Zapata"));
    }

    void listarProfesores_vacio() {
        auto lista = m_servicio->listarProfesores().valor;
        QVERIFY(lista.isEmpty());
    }

    // ─── actualizarProfesor ─────────────────────────────────────────────────

    void actualizarProfesor_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-500", "Nombre Viejo", "viejo@uni.edu", "111").ok);

        auto actualizado = m_servicio->actualizarProfesor(
            "P-500", "Nombre Nuevo", "nuevo@uni.edu", "222");

        QVERIFY(actualizado.ok);
        QCOMPARE(actualizado.valor.nombre,   QString("Nombre Nuevo"));
        QCOMPARE(actualizado.valor.email,    QString("nuevo@uni.edu"));
        QCOMPARE(actualizado.valor.telefono, QString("222"));
    }

    void actualizarProfesor_inexistente_falla() {
        auto resultado = m_servicio->actualizarProfesor(
            "P-999", "X", "x@uni.edu", "000");
        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.codigoError, -3);
    }

    void actualizarProfesor_nombreDuplicado_falla() {
        QVERIFY(m_servicio->crearProfesor("P-600", "Dr. Uno", "u@uni.edu").ok);
        QVERIFY(m_servicio->crearProfesor("P-601", "Dr. Dos", "d@uni.edu").ok);

        auto resultado = m_servicio->actualizarProfesor(
            "P-601", "Dr. Uno", "d@uni.edu", "");
        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.codigoError, -2);
    }

    void actualizarProfesor_emailDuplicado_falla() {
        QVERIFY(m_servicio->crearProfesor("P-700", "Dr. A", "a@uni.edu").ok);
        QVERIFY(m_servicio->crearProfesor("P-701", "Dr. B", "b@uni.edu").ok);

        auto resultado = m_servicio->actualizarProfesor(
            "P-701", "Dr. B", "a@uni.edu", "");
        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.codigoError, -2);
    }

    // ─── eliminarProfesor ───────────────────────────────────────────────────

    void eliminarProfesor_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-800", "Dr. Borrar", "b@uni.edu").ok);

        QVERIFY(m_servicio->eliminarProfesor("P-800"));

        auto obtenido = m_servicio->obtenerProfesor("P-800");
        QVERIFY(!obtenido.ok);
    }

    void eliminarProfesor_inexistente_falla() {
        QVERIFY(!m_servicio->eliminarProfesor("P-999"));
    }

    void eliminarProfesor_idVacio_falla() {
        QVERIFY(!m_servicio->eliminarProfesor(""));
        QVERIFY(!m_servicio->eliminarProfesor("   "));
    }

    void eliminarProfesor_borraRelaciones() {
        QVERIFY(m_servicio->crearProfesor(
            "P-900", "Dr. Cascada", "c@uni.edu").ok);
        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-900", franja(1, 8, 0, 10, 0)).ok);

        auto materia = m_servicioMaterias->crearMateria("Materia Cascada");
        QVERIFY(materia.ok);
        QVERIFY(m_servicio->asignarMateria("P-900", materia.valor.id));

        QVERIFY(m_servicio->eliminarProfesor("P-900"));

        // Verificar directamente en la BD que no queden huérfanos
        QSqlQuery q(m_dbManager->database());

        q.prepare("SELECT COUNT(*) FROM Disponibilidad_Profesor WHERE id_Profesor = :id");
        q.bindValue(":id", "P-900");
        QVERIFY(q.exec() && q.next());
        QCOMPARE(q.value(0).toInt(), 0);

        q.prepare("SELECT COUNT(*) FROM Profesor_Materia WHERE id_Profesor = :id");
        q.bindValue(":id", "P-900");
        QVERIFY(q.exec() && q.next());
        QCOMPARE(q.value(0).toInt(), 0);
    }

    // ─── agregarDisponibilidad ──────────────────────────────────────────────

    void agregarDisponibilidad_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-1000", "Dr. Disp", "d@uni.edu").ok);

        auto resultado = m_servicio->agregarDisponibilidad(
            "P-1000", franja(1, 8, 0, 10, 30));

        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.id > 0);
        QCOMPARE(resultado.valor.franja.dia, 1);
        QCOMPARE(resultado.valor.franja.inicio, QTime(8, 0));
        QCOMPARE(resultado.valor.franja.fin,    QTime(10, 30));
    }

    void agregarDisponibilidad_profesorInexistente_falla() {
        auto resultado = m_servicio->agregarDisponibilidad(
            "NO-EXISTE", franja(1, 8, 0, 10, 0));
        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.codigoError, -3);
    }

    void agregarDisponibilidad_idVacio_falla() {
        auto resultado = m_servicio->agregarDisponibilidad(
            "", franja(1, 8, 0, 10, 0));
        QVERIFY(!resultado.ok);
    }

    void agregarDisponibilidad_diaInvalido_falla() {
        QVERIFY(m_servicio->crearProfesor("P-1100", "Dr. X", "x@uni.edu").ok);

        auto r1 = m_servicio->agregarDisponibilidad(
            "P-1100", franja(-1, 8, 0, 10, 0));
        QVERIFY(!r1.ok);
        QVERIFY(r1.mensajeError.contains("día"));

        auto r2 = m_servicio->agregarDisponibilidad(
            "P-1100", franja(7, 8, 0, 10, 0));
        QVERIFY(!r2.ok);
        QVERIFY(r2.mensajeError.contains("día"));
    }

    void agregarDisponibilidad_inicioMayorQueFin_falla() {
        QVERIFY(m_servicio->crearProfesor("P-1200", "Dr. X", "x@uni.edu").ok);

        auto resultado = m_servicio->agregarDisponibilidad(
            "P-1200", franja(1, 12, 0, 8, 0));
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("anterior"));
    }

    void agregarDisponibilidad_inicioIgualAFin_falla() {
        QVERIFY(m_servicio->crearProfesor("P-1300", "Dr. X", "x@uni.edu").ok);

        auto resultado = m_servicio->agregarDisponibilidad(
            "P-1300", franja(1, 10, 0, 10, 0));
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("anterior"));
    }

    void agregarDisponibilidad_multiplesFranjas() {
        QVERIFY(m_servicio->crearProfesor(
            "P-1400", "Dr. Multi", "m@uni.edu").ok);

        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-1400", franja(1, 8, 0, 10, 0)).ok);
        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-1400", franja(1, 14, 0, 16, 0)).ok);
        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-1400", franja(3, 9, 0, 11, 0)).ok);

        auto obtenido = m_servicio->obtenerProfesor("P-1400");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.disponibilidad.size(), 3);
        // Ordenado por día, hora_inicio
        QCOMPARE(obtenido.valor.disponibilidad[0].franja.dia, 1);
        QCOMPARE(obtenido.valor.disponibilidad[0].franja.inicio, QTime(8, 0));
        QCOMPARE(obtenido.valor.disponibilidad[1].franja.inicio, QTime(14, 0));
        QCOMPARE(obtenido.valor.disponibilidad[2].franja.dia, 3);
    }

    // ─── eliminarDisponibilidad / limpiarDisponibilidad ────────────────────

    void eliminarDisponibilidad_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-1500", "Dr. X", "x@uni.edu").ok);

        auto creada = m_servicio->agregarDisponibilidad(
            "P-1500", franja(1, 8, 0, 10, 0));
        QVERIFY(creada.ok);

        QVERIFY(m_servicio->eliminarDisponibilidad(creada.valor.id));

        auto obtenido = m_servicio->obtenerProfesor("P-1500");
        QVERIFY(obtenido.ok);
        QVERIFY(obtenido.valor.disponibilidad.isEmpty());
    }

    void eliminarDisponibilidad_idInvalido_falla() {
        QVERIFY(!m_servicio->eliminarDisponibilidad(0));
        QVERIFY(!m_servicio->eliminarDisponibilidad(-5));
    }

    void eliminarDisponibilidad_inexistente_falla() {
        QVERIFY(!m_servicio->eliminarDisponibilidad(9999));
    }

    void limpiarDisponibilidad_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-1600", "Dr. X", "x@uni.edu").ok);
        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-1600", franja(1, 8, 0, 10, 0)).ok);
        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-1600", franja(2, 8, 0, 10, 0)).ok);

        QVERIFY(m_servicio->limpiarDisponibilidad("P-1600"));

        auto obtenido = m_servicio->obtenerProfesor("P-1600");
        QVERIFY(obtenido.ok);
        QVERIFY(obtenido.valor.disponibilidad.isEmpty());
    }

    void limpiarDisponibilidad_idVacio_falla() {
        QVERIFY(!m_servicio->limpiarDisponibilidad(""));
    }

    // ─── asignarMateria / quitarMateria / limpiarMaterias ──────────────────

    void asignarMateria_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-1700", "Dr. X", "x@uni.edu").ok);

        auto materia = m_servicioMaterias->crearMateria("Álgebra");
        QVERIFY(materia.ok);

        QVERIFY(m_servicio->asignarMateria("P-1700", materia.valor.id));

        auto obtenido = m_servicio->obtenerProfesor("P-1700");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.materias.size(), 1);
        QCOMPARE(obtenido.valor.materias[0].id, materia.valor.id);
        QCOMPARE(obtenido.valor.materias[0].nombre, QString("Álgebra"));
    }

    void asignarMateria_idempotente() {
        QVERIFY(m_servicio->crearProfesor(
            "P-1800", "Dr. X", "x@uni.edu").ok);

        auto materia = m_servicioMaterias->crearMateria("Física");
        QVERIFY(materia.ok);

        QVERIFY(m_servicio->asignarMateria("P-1800", materia.valor.id));
        QVERIFY(m_servicio->asignarMateria("P-1800", materia.valor.id));

        auto obtenido = m_servicio->obtenerProfesor("P-1800");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.materias.size(), 1); // No duplicado
    }

    void asignarMateria_parametrosInvalidos_falla() {
        auto materia = m_servicioMaterias->crearMateria("Química");
        QVERIFY(materia.ok);

        QVERIFY(!m_servicio->asignarMateria("", materia.valor.id));
        QVERIFY(!m_servicio->asignarMateria("P-1900", 0));
        QVERIFY(!m_servicio->asignarMateria("P-1900", -1));
    }

    void asignarMateriaPorNombre_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-2000", "Dr. X", "x@uni.edu").ok);

        auto materia = m_servicioMaterias->crearMateria("Cálculo");
        QVERIFY(materia.ok);

        QVERIFY(m_servicio->asignarMateriaPorNombre("P-2000", "Cálculo"));

        auto obtenido = m_servicio->obtenerProfesor("P-2000");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.materias.size(), 1);
        QCOMPARE(obtenido.valor.materias[0].nombre, QString("Cálculo"));
    }

    void asignarMateriaPorNombre_inexistente_falla() {
        QVERIFY(m_servicio->crearProfesor(
            "P-2100", "Dr. X", "x@uni.edu").ok);

        QVERIFY(!m_servicio->asignarMateriaPorNombre("P-2100", "NoExiste"));
    }

    void asignarMateriaPorNombre_parametrosInvalidos_falla() {
        QVERIFY(!m_servicio->asignarMateriaPorNombre("", "X"));
        QVERIFY(!m_servicio->asignarMateriaPorNombre("P-X", ""));
        QVERIFY(!m_servicio->asignarMateriaPorNombre("P-X", "   "));
    }

    void quitarMateria_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-2200", "Dr. X", "x@uni.edu").ok);

        auto m1 = m_servicioMaterias->crearMateria("Mat A");
        auto m2 = m_servicioMaterias->crearMateria("Mat B");
        QVERIFY(m1.ok && m2.ok);

        QVERIFY(m_servicio->asignarMateria("P-2200", m1.valor.id));
        QVERIFY(m_servicio->asignarMateria("P-2200", m2.valor.id));

        QVERIFY(m_servicio->quitarMateria("P-2200", m1.valor.id));

        auto obtenido = m_servicio->obtenerProfesor("P-2200");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.materias.size(), 1);
        QCOMPARE(obtenido.valor.materias[0].id, m2.valor.id);
    }

    void quitarMateria_noAsignada_falla() {
        QVERIFY(m_servicio->crearProfesor(
            "P-2300", "Dr. X", "x@uni.edu").ok);

        auto materia = m_servicioMaterias->crearMateria("Sin Asignar");
        QVERIFY(materia.ok);

        QVERIFY(!m_servicio->quitarMateria("P-2300", materia.valor.id));
    }

    void quitarMateria_parametrosInvalidos_falla() {
        QVERIFY(!m_servicio->quitarMateria("", 1));
        QVERIFY(!m_servicio->quitarMateria("P-X", 0));
    }

    void limpiarMaterias_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-2400", "Dr. X", "x@uni.edu").ok);

        auto m1 = m_servicioMaterias->crearMateria("A");
        auto m2 = m_servicioMaterias->crearMateria("B");
        QVERIFY(m1.ok && m2.ok);

        QVERIFY(m_servicio->asignarMateria("P-2400", m1.valor.id));
        QVERIFY(m_servicio->asignarMateria("P-2400", m2.valor.id));

        QVERIFY(m_servicio->limpiarMaterias("P-2400"));

        auto obtenido = m_servicio->obtenerProfesor("P-2400");
        QVERIFY(obtenido.ok);
        QVERIFY(obtenido.valor.materias.isEmpty());
    }

    void limpiarMaterias_idVacio_falla() {
        QVERIFY(!m_servicio->limpiarMaterias(""));
    }

    // ─── toProfesor (DTO → value object) ───────────────────────────────────

    void toProfesor_convierteCorrectamente() {
        ProfesorDTO dto;
        dto.id       = "P-X";
        dto.nombre   = "Dr. Test";
        dto.email    = "t@uni.edu";
        dto.telefono = "555";

        DisponibilidadDTO d;
        d.id = 1;
        d.franja = franja(1, 8, 0, 10, 0);
        dto.disponibilidad.append(d);

        MateriaAsignadaDTO m;
        m.id = 10;
        m.nombre = "Matemáticas";
        dto.materias.append(m);

        Profesor p = dto.toProfesor();

        QCOMPARE(p.nombre, QString("Dr. Test"));
        QCOMPARE(p.disponibilidad.size(), 1);
        QCOMPARE(p.disponibilidad[0].dia, 1);
        QCOMPARE(p.materias.size(), 1);
        QCOMPARE(p.materias[0], QString("Matemáticas"));
        // El value object NO lleva id/email/telefono
    }

    // ─── Extracción para solver ────────────────────────────────────────────

    void obtenerTodosParaSolver() {
        QVERIFY(m_servicio->crearProfesor(
            "P-2500", "Dr. A", "a@uni.edu").ok);
        QVERIFY(m_servicio->crearProfesor(
            "P-2501", "Dr. B", "b@uni.edu").ok);
        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-2500", franja(1, 8, 0, 10, 0)).ok);

        auto materia = m_servicioMaterias->crearMateria("Mat");
        QVERIFY(materia.ok);
        QVERIFY(m_servicio->asignarMateria("P-2500", materia.valor.id));

        auto profesores = m_servicio->obtenerTodosParaSolver();
        QCOMPARE(profesores.size(), 2);

        // Ordenados por nombre: A antes que B
        QCOMPARE(profesores[0].nombre, QString("Dr. A"));
        QCOMPARE(profesores[0].disponibilidad.size(), 1);
        QCOMPARE(profesores[0].materias.size(), 1);
        QCOMPARE(profesores[1].nombre, QString("Dr. B"));
        QVERIFY(profesores[1].disponibilidad.isEmpty());
        QVERIFY(profesores[1].materias.isEmpty());
    }

    void obtenerTodosParaSolver_vacio() {
        auto profesores = m_servicio->obtenerTodosParaSolver();
        QVERIFY(profesores.isEmpty());
    }

    void obtenerProfesorParaSolver_exitoso() {
        QVERIFY(m_servicio->crearProfesor(
            "P-2600", "Dr. Solver", "s@uni.edu").ok);
        QVERIFY(m_servicio->agregarDisponibilidad(
            "P-2600", franja(2, 14, 0, 18, 0)).ok);

        auto resultado = m_servicio->obtenerProfesorParaSolver("P-2600");
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, QString("Dr. Solver"));
        QCOMPARE(resultado.valor.disponibilidad.size(), 1);
        QCOMPARE(resultado.valor.disponibilidad[0].dia, 2);
    }

    void obtenerProfesorParaSolver_inexistente_falla() {
        auto resultado = m_servicio->obtenerProfesorParaSolver("NO-EXISTE");
        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.codigoError, -3);
    }

private:
    std::unique_ptr<QTemporaryDir>       m_tempDir;
    std::unique_ptr<DatabaseManager>     m_dbManager;
    std::unique_ptr<ServicioProfesor>    m_servicio;
    std::unique_ptr<ServicioMaterias>    m_servicioMaterias;
};

QTEST_MAIN(TestServicioProfesor)
#include "test_servicio_profesor.moc"
