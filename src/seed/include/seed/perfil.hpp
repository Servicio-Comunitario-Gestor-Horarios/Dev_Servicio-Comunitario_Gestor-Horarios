#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QVector>

namespace seed {

/// Entrada del catálogo: un área de formación en un año concreto («Castellano 1º»).
struct MateriaAnio {
    QString nombre;
    int anio;
};

/// Enlace plan-materia: una materia-año con su carga semanal en horas académicas.
struct CargaPlan {
    QString materia;
    int anio;
    int horas;
};

/// Plan de estudios: código/nombre/área y sus materias-año con carga.
struct PlanDef {
    QString codigo;
    QString nombre;
    QString areaDesarrollo;
    QVector<CargaPlan> cargas;
};

struct AulaDef {
    QString nombre;
    int capacidad;
    QString edificio;
    QString piso;
    QString tipo;              ///< tipo del catálogo de tipos de aula (RF-6, RF-10)
};

/// Evento del turno (RF-1, RF-10): suceso con nombre que ocupa tiempo tras la k-ésima franja.
struct EventoDef {
    QString nombre;            ///< nombre del suceso («Receso», «Acto cívico»…)
    int despuesDeSlot = 0;     ///< posición 1..numSlots (después de la k-ésima franja)
    int orden = 0;             ///< orden dentro de la posición (>= 1)
    int duracion = 0;          ///< duración en minutos (> 0)
    QTime inicio;              ///< hora de inicio; inválida = evento sin horario
    QTime fin;                 ///< hora de fin; inválida = evento sin horario
};

struct TurnoDef {
    QString nombre;
    QTime inicio;
    QTime fin;
    int numSlots;              ///< franjas del turno (8); «slots» es keyword de Qt
    int recesoDespuesDeSlot;   ///< compatibilidad: espejo del evento «Receso» (se retira en T8)
    QTime recesoInicio;
    QTime recesoFin;
    QVector<EventoDef> eventos; ///< eventos del turno, en el orden en que se declaran
};

/// Curso de un año y sección, sin turno (el turno lo aporta el preset).
struct CursoDef {
    int anio;
    QString seccion;
    QString aula;
};

/// Conjunto de datos de prueba del centro.
struct Perfil {
    QStringList          tiposAula; ///< catálogo de tipos: 3 («Aula de estudio», «Laboratorio», «Taller»)
    QVector<TurnoDef>    turnos;   ///< 2 (se rellena en 008-T3)
    QVector<AulaDef>     aulas;    ///< 16 (se rellena en 008-T3)
    QVector<MateriaAnio> materias; ///< 51 del 31059 (57 con las nuevas en 008-T2)
    QVector<PlanDef>     planes;   ///< 4 (General en T1; derivados en 008-T2)
    QVector<CursoDef>    cursos;   ///< 13 (se rellena en 008-T3)
};

/// Recuentos por entidad del conjunto aplicado (RF-12, RF-10 de la 009). Se leen de la base
/// tras el commit, no del perfil, para que el informe verifique lo escrito (plan §8).
struct Recuentos {
    int turnos = 0;            ///< filas de Turnos (2)
    int eventos = 0;           ///< filas de Eventos (2)
    int tiposAula = 0;         ///< filas de TiposAula (3)
    int recesos = 0;           ///< compatibilidad: filas de Recesos (2); se retira en T8
    int aulas = 0;             ///< filas de Aulas (16)
    int materias = 0;          ///< filas de Materias (57)
    int planes = 0;            ///< filas de PlanEstudio (4)
    int planMaterias = 0;      ///< filas de PlanEstudio_Materia (210)
    int cursos = 0;            ///< filas de Cursos (13)
    int docentes = 0;          ///< filas de Profesores
    int asignaciones = 0;      ///< filas de Profesor_Materia
    int disponibilidades = 0;  ///< filas de Disponibilidad_Profesor
};

/// Identidad: 1 hora académica = 1 bloque (sin conversión ni redondeo). Pura.
int bloques(int horas);

/// Suma de horas académicas de un plan en un año; -1 si no tiene ese año. Pura.
int horasDeAnio(const PlanDef& plan, int anio);

/// Suma de bloques (coinciden con las horas) de un plan en un año; -1 si no tiene ese año. Pura.
int bloquesDeAnio(const PlanDef& plan, int anio);

/// Secciones de un año: 3 en 1.º–3.º y 2 en 4.º–5.º. Pura.
int seccionesDeAnio(int anio);

/// ¿Es una de las 6 materias nuevas de un plan derivado? (no son áreas del 31059 y no
/// añaden demanda, RF-3). Pura.
bool esMateriaNueva(const QString& nombre);

/// Tipo de aula que pide una materia por su área (prefijo, no nombre exacto): ciencias
/// experimentales (Física, Química, Biología, Ciencias Naturales y Ciencias de la Tierra,
/// incluidas las «Aplicada») → «Laboratorio»; informática/taller → «Taller»; Educación
/// Física → «Cancha»; el resto → «Aula de estudio» (RF-10). Pura.
QString tipoEspecializadoDeMateria(const QString& nombre);

/// Tipo final de una materia: el especializado de su área si el conjunto tiene al menos un
/// aula de ese tipo; en caso contrario, «Aula de estudio» (N1, RF-10). Pura.
QString tipoFinalDeMateria(const QString& nombre, const QSet<QString>& tiposDeAulas);

/// Horas máximas de una materia entre los planes que la incluyen (0 si no la incluye ninguno).
/// Pura.
int horasMaximas(const Perfil& perfil, const QString& materia);

/// Carga de una materia-año = horasMaximas × secciones del año; **0** para las 6 materias
/// nuevas (sustituyen horas base y no añaden demanda, RF-3). Pura.
int cargaMateria(const Perfil& perfil, const QString& materia);

/// Σ cargaMateria de las materias de un docente. Pura.
int cargaAsignada(const Perfil& perfil, const QStringList& materias);

/// Demanda total = 13 cursos × 40 = 520; coincide con Σ cargaMateria del catálogo. Pura.
int demandaTotal(const Perfil& perfil);

/// Perfil por defecto, construido una sola vez e inmutable. No lee reloj, disco ni configuración.
const Perfil& perfilPorDefecto();

/// Valida la coherencia del perfil (catálogo, planes y aulas) y deja en `error` el primer
/// problema encontrado, en español e identificando plan/año/materia. Pura.
bool validarPerfil(const Perfil& perfil, QString* error);

} // namespace seed
