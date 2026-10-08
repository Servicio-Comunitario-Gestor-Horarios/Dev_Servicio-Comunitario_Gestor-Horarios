#pragma once
#include <QCheckBox>
#include <QPainter>

class ToggleSwitch : public QCheckBox {
    Q_OBJECT
public:
    explicit ToggleSwitch(QWidget *parent = nullptr) : QCheckBox(parent) {
        setCursor(Qt::PointingHandCursor);
        setFixedSize(46, 24); // Tamaño ajustado a la maqueta
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QRect r = rect();

        if (isChecked()) {
            painter.setBrush(QColor("#243c8a")); // Azul oscuro de la maqueta
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r, 12, 12);
            painter.setBrush(Qt::white);
            painter.drawEllipse(r.right() - 22, 2, 20, 20); // Círculo a la derecha
        } else {
            painter.setBrush(QColor("#cbd5e1")); // Gris claro inactivo
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r, 12, 12);
            painter.setBrush(Qt::white);
            painter.drawEllipse(2, 2, 20, 20); // Círculo a la izquierda
        }
    }
};