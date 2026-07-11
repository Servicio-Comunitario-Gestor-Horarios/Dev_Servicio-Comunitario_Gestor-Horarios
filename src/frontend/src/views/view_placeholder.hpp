#ifndef VIEW_PLACEHOLDER_HPP
#define VIEW_PLACEHOLDER_HPP

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class ViewPlaceholder : public QWidget
{
    Q_OBJECT
public:
    explicit ViewPlaceholder(const QString &title, QWidget *parent = nullptr);
};

#endif // VIEW_PLACEHOLDER_HPP