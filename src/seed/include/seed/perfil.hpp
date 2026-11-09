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
    int anio = 0;
    QString area;              ///< área de formación exacta del catálogo (nombre completo)
    bool sinCarga = false;     ///< materia de las áreas excluidas de todo plan (excepción de D-4)
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
    QString tipo;              ///< tipo del catálogo de tipos de aula (RF-5)
};

/// Evento del turno (RF-7): suceso con nombre que ocupa tiempo tras la k-ésima franja.
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
    QVector<EventoDef> eventos; ///< eventos del turno, en el orden en que se declaran
};

/// Curso del centro: año, sección, turno y aula de referencia (lo persistido) más el plan
/// y la matrícula, que viven solo en el perfil (RF-4).
struct CursoDef {
    int anio = 0;
    QString seccion;
    QString aula;
    QString plan;              ///< plan del curso en el perfil; no se persiste
    QString turno;             ///< turno del curso; se persiste en `Cursos.turno`
    int matricula = 0;         ///< alumnos del grupo (32–44); no se persiste
};

/// Conjunto de datos de prueba del centro (Liceo Nacional Robert Serra, 2026/2027).
struct Perfil {
    QStringList          tiposAula; ///< catálogo declarado: 1 («Cancha»); el general lo pone el esquema
    QVector<TurnoDef>    turnos;   ///< 2 (Mañana y Tarde)
    QVector<AulaDef>     aulas;    ///< 13 (12 generales y una cancha)
    QVector<MateriaAnio> materias; ///< 85 (79 con carga y 6 sin carga)
    QVector<PlanDef>     planes;   ///< 2 (31059 y 31060)
    QVector<CursoDef>    cursos;   ///< 24 (12 en cada turno)
    int duracionMinutos = 0;       ///< duración de la hora académica (configuración): 40
};

/// Recuentos por entidad del conjunto aplicado (RF-8). Se leen de la base tras el commit
/// (más la duración y su fila de configuración), no del perfil, para que el informe
/// verifique lo escrito (plan §7).
struct Recuentos {
    int turnos = 0;            ///< filas de Turnos (2)
    int eventos = 0;           ///< filas de Eventos (3)
    int tiposAula = 0;         ///< filas de TiposAula (2: general y cancha)
    int aulas = 0;             ///< filas de Aulas (13)
    int materias = 0;          ///< filas de Materias (85)
    int planes = 0;            ///< filas de PlanEstudio (2)
    int planMaterias = 0;      ///< filas de PlanEstudio_Materia (95)
    int cursos = 0;            ///< filas de Cursos (24)
    int docentes = 0;          ///< filas de Profesores (32)
    int asignaciones = 0;      ///< filas de Profesor_Materia (272)
    int disponibilidades = 0;  ///< filas de Disponibilidad_Profesor (160)
    int preferencias = 0;      ///< filas de Preferencia_Profesor (0: el conjunto no las siembra)
    int duracionMinutos = 0;   ///< duración efectiva de la hora académica (40)
    int configuraciones = 0;   ///< filas de ConfiguracionHoraria (1)
};

/// Identidad: 1 hora académica = 1 bloque (sin conversión ni redondeo). Pura.
int bloques(int horas);

/// Suma de horas académicas de un plan en un año; -1 si no tiene ese año. Pura.
int horasDeAnio(const PlanDef& plan, int anio);

/// Suma de bloques (coinciden con las horas) de un plan en un año; -1 si no tiene ese año. Pura.
int bloquesDeAnio(const PlanDef& plan, int anio);

/// Tipo de aula que pide una materia por su área: Educación Física exige «Cancha»; el resto,
/// el tipo general del esquema (RF-3). Pura.
QString tipoEspecializadoDeMateria(const QString& nombre);

/// Tipo final de una materia: el especializado de su área si el conjunto tiene al menos un
/// aula de ese tipo; en caso contrario, el tipo general (RF-3). Pura.
QString tipoFinalDeMateria(const QString& nombre, const QSet<QString>& tiposDeAulas);

/// Horas académicas de un curso: suma de las cargas del plan del curso en su año; -1 si el
/// perfil no declara ese plan (plan §5.1). Pura.
int horasDeCurso(const Perfil& perfil, const CursoDef& curso);

/// Demanda total del centro: Σ horasDeCurso de los 24 cursos (876); -1 si un curso declara
/// un plan inexistente. Pura.
int demandaTotal(const Perfil& perfil);

/// Demanda de un turno: Σ horasDeCurso de sus cursos (mañana 414, tarde 462); -1 si un curso
/// declara un plan inexistente o el turno no existe. Pura.
int demandaPorTurno(const Perfil& perfil, const QString& turno);

/// ¿Tiene el área algún curso en el turno dado? (Regla de asignación de docentes, plan §5.3.)
/// Pura.
bool areaConCursosEnTurno(const Perfil& perfil, const QString& area, const QString& turno);

/// Demanda de un área en un turno: Σ horas de las materias del área en los planes de los
/// cursos de ese turno (Educación Física: 48 por turno). -1 si el turno no existe o un curso
/// declara un plan inexistente. Pura.
int demandaDeAreaPorTurno(const Perfil& perfil, const QString& area, const QString& turno);

/// Motivo por el que el conjunto **no se declara generable** (RF-9, plan §5.5): la Educación
/// Física de cada turno necesita más clases que franjas de cancha con aforo 1. Devuelve el
/// primer turno que no cabe, con sus cifras, o "" si el conjunto cabría. No es validación:
/// la aplicación no falla por esto. Pura.
QString motivoNoGenerable(const Perfil& perfil);

/// Perfil por defecto, construido una sola vez e inmutable. No lee reloj, disco ni configuración.
const Perfil& perfilPorDefecto();

/// Valida la coherencia del perfil (planes, catálogo, cursos, aulas, turnos y duración) y deja
/// en `error` el primer problema encontrado, en español e identificando plan/año/curso/área y
/// el valor recibido. Pura.
bool validarPerfil(const Perfil& perfil, QString* error);

} // namespace seed
