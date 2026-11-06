#pragma once

/**
 * @file schedule_grid_dialog.hpp
 * @brief Base compartida por los diálogos de horarios de profesores y alumnos.
 *
 * Aporta la rejilla, el arrastrar/soltar de bloques, la detección de conflictos
 * entre todos los horarios y el diálogo estilizado Cancelar/Intercambiar.
 *
 * La rejilla se dibuja con los datos que dejó la generación en el documento
 * (`_cuadricula`): días, franjas de cada turno, duración del bloque y recesos
 * (posición y duración). Un curso solo ve las franjas de su turno; un docente,
 * las de todos. Las clases que ocupan varias franjas seguidas se pintan como un
 * único bloque. Si el documento no describe su rejilla, se usa la plantilla
 * histórica (véase `schedule_model.hpp`).
 *
 * Las subclases sólo definen la navegación (docentes o años/secciones) y el
 * `currentRef()` que indica qué horario se está editando.
 */

#include <QDialog>
#include <QJsonObject>
#include <QPoint>
#include <QString>

#include "schedule_model.hpp"

class QLabel;
class QFrame;
class QScrollArea;
class QGridLayout;
class QVBoxLayout;
class QTimer;
class QWidget;
class QMouseEvent;

class ScheduleGridDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScheduleGridDialog(const QJsonObject& rootData, bool showCourse,
                                QWidget* parent = nullptr);
    ~ScheduleGridDialog() override = default;

signals:
    /** Emite el documento completo (teachers + years) tras una edición. */
    void scheduleModified(const QJsonObject& updatedData);

public:
    /**
     * Mueve (o intercambia) la clase que empieza en (fromDay, fromSlot) a
     * (toDay, toSlot) del horario actual. Las clases de varias franjas seguidas
     * viajan enteras y, si el destino solapa su propio tramo, se desplazan sin
     * perder ninguna clase. Es el mismo camino que usa el arrastrar/soltar,
     * expuesto para pruebas y para accesibilidad.
     */
    void moveBlock(int fromDay, int fromSlot, int toDay, int toSlot);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

    // Utilidades para las subclases
    QWidget* buildGridCard(QWidget* parent);
    QWidget* buildLegendCard(QWidget* parent);
    /** Vuelve a pintar la leyenda con las materias del horario que se está viendo. */
    void     rebuildLegend();
    void     applyStyles();
    void     rebuildGrid();
    void     resetGridScroll();
    int      blockCount() const;
    int      visualBlockCount() const;
    QString  currentTurno() const;
    void     emitModified() { emit scheduleModified(m_data); }

    // Contrato de las subclases
    virtual ScheduleModel::Ref currentRef() const = 0;
    virtual QString            displayName() const = 0;
    virtual void               onScheduleChanged() {}

    QJsonObject m_data;
    bool m_showCourse = true;

    QLabel* m_eyebrowLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;

private:
    QFrame* createBlockCell(int day, int slot, const ScheduleModel::Block& block,
                            int span, const QString& rangoHoras);
    void    startDrag(QFrame* cell, const QPoint& pos);

    /** Programa un recálculo de la rejilla cuando cambia el tamaño visible. */
    void scheduleSizeRebuild();
    /** Reconstruye la rejilla solo si el tamaño de las celdas debe cambiar. */
    void rebuildForViewportIfNeeded();
    /** Tamaño de celda que llena el área visible; false si aún no hay tamaño. */
    bool computeCellSizes(int& dayW, int& rowH) const;

    QScrollArea* m_gridScroll = nullptr;
    QWidget*     m_gridHost = nullptr;
    QGridLayout* m_scheduleGrid = nullptr;

    QScrollArea* m_legendScroll = nullptr;
    QWidget*     m_legendHost = nullptr;
    QVBoxLayout* m_legendLayout = nullptr;

    QTimer* m_sizeTimer = nullptr;
    int m_dayColW = 150;
    int m_rowH = 74;
    int m_numDias = 5;
    int m_teachingRows = 0;
    int m_recessRows = 0;
    int m_lastViewportW = -1;
    int m_lastViewportH = -1;

    QPoint  m_pressPos;
    QFrame* m_pressCell = nullptr;
    bool    m_maybeDrag = false;
    bool    m_entranceDone = false;
};
