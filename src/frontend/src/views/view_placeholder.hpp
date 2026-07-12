/**
 * @file view_placeholder.hpp
 * @brief Widget de placeholder para vistas pendientes
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class ViewPlaceholder : public QWidget
{
    Q_OBJECT
public:
    explicit ViewPlaceholder(const QString &title, QWidget *parent = nullptr);
};
