#include "view_placeholder.hpp"

ViewPlaceholder::ViewPlaceholder(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(title, this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 24px; color: #64748b;");
    layout->addWidget(label);
}