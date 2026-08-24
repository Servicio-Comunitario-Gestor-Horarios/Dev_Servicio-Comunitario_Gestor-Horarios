// qa-audit Phase 6 fallback probe: widget-level offscreen UI evidence.
// External audit tool — links prebuilt /workspace/build static libs; NO product code edited.
// Build (in gestor-dev container):
//   g++ -std=c++17 -fPIC -DQT_NO_DEBUG probe-main.cpp \
//     -I/workspace/src/frontend/include -I/workspace/src/frontend/src \
//     -I/usr/include/x86_64-linux-gnu/qt6 \
//     -I/usr/include/x86_64-linux-gnu/qt6/QtCore \
//     -I/usr/include/x86_64-linux-gnu/qt6/QtGui \
//     -I/usr/include/x86_64-linux-gnu/qt6/QtWidgets \
//     -I/usr/include/x86_64-linux-gnu/qt6/QtNetwork \
//     /workspace/build/src/frontend/libfrontend.a \
//     /workspace/build/src/middleware/libmiddleware.a \
//     -lQt6Widgets -lQt6Gui -lQt6Core -lQt6Network -o /tmp/probe_ui
// Run: QT_QPA_PLATFORM=offscreen /tmp/probe_ui
#include <QApplication>
#include <QEventLoop>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QDebug>

#include "logindialog.h"
#include "views/main_window.hpp"
#include "forms/teacher_form_dialog.hpp"
#include "forms/classroom_form_dialog.hpp"
#include "forms/subject_form_dialog.hpp"

using gestor::frontend::forms::TeacherFormDialog;
using gestor::frontend::forms::ClassroomFormDialog;
using gestor::frontend::forms::SubjectFormDialog;

static const char *OUT = "/workspace/docs/auditoria/evidencia";

static void shot(QWidget &w, const QString &name)
{
    QApplication::processEvents();
    const QPixmap pm = w.grab();
    const QString path = QString("%1/ui-%2.png").arg(OUT, name);
    qInfo().noquote() << "PROBE_SHOT" << name
                      << "| title=" << w.windowTitle()
                      << "| pixmap=" << pm.width() << "x" << pm.height()
                      << "| saved=" << pm.save(path);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // ── 1. LoginDialog: render + fake-auth accept path ────────────────
    LoginDialog login;
    login.show();
    shot(login, "login-dialog");

    const auto edits = login.findChildren<QLineEdit *>();
    qInfo() << "LOGIN lineedits found:" << edits.size();
    if (edits.size() >= 2) {
        edits[0]->setText("audit-probe");
        edits[1]->setText("probe-pass");
    }
    bool clicked = false;
    if (auto btn = login.findChild<QPushButton *>()) {
        btn->click();
        clicked = true;
    }
    QEventLoop loopAuth;
    QTimer::singleShot(700, &loopAuth, &QEventLoop::quit); // >300ms fake-auth delay
    loopAuth.exec();
    const bool accepted = (login.result() == QDialog::Accepted);
    qInfo() << "LOGIN clicked=" << clicked << "accepted=" << accepted;

    // ── 2. MainWindow: sidebar navigation + per-view renders ──────────
    MainWindow w;
    w.show();
    shot(w, "view0-dashboard-inicio");

    struct Step { const char *slot; const char *png; };
    const Step steps[] = {
        {"mostrarDocentes",      "view1-docentes"},
        {"mostrarAulas",         "view2-aulas"},
        {"mostrarAsignaturas",   "view3-asignaturas"},
        {"mostrarGeneracion",    "view4-generacion"},
        {"mostrarVisualizacion", "view5-visualizacion"},
    };
    QStackedWidget *stack = w.findChild<QStackedWidget *>();
    for (const Step &s : steps) {
        const bool ok = QMetaObject::invokeMethod(&w, s.slot, Qt::DirectConnection);
        QApplication::processEvents();
        qInfo() << "NAV invoke" << s.slot << "ok=" << ok
                << "currentIndex=" << (stack ? stack->currentIndex() : -1);
        shot(w, s.png);
    }

    // Sidebar inventory
    const auto buttons = w.findChildren<QPushButton *>();
    QStringList texts;
    for (const QPushButton *b : buttons)
        texts << b->text();
    qInfo().noquote() << "SIDEBAR_BUTTONS" << texts.join(" || ");

    // Table inventory (data wiring evidence; lists never fetch -> expect 0 rows)
    const auto tables = w.findChildren<QTableWidget *>();
    qInfo() << "TABLES count:" << tables.size();
    int i = 0;
    for (const QTableWidget *t : tables)
        qInfo().noquote() << QString("TABLE[%1] rows=%2 cols=%3")
                                 .arg(i++).arg(t->rowCount()).arg(t->columnCount());

    // ── 3. Form dialogs: constructibility + render ────────────────────
    TeacherFormDialog tf;
    tf.show();
    shot(tf, "dialog-teacher-form");
    ClassroomFormDialog cf;
    cf.show();
    shot(cf, "dialog-classroom-form");
    SubjectFormDialog sf;
    sf.show();
    shot(sf, "dialog-subject-form");

    qInfo() << "PROBE_SUMMARY login_accepted=" << accepted
            << "views_probed=" << (1 + int(sizeof(steps)/sizeof(Step)))
            << "dialogs_probed=3";
    return accepted ? 0 : 1;
}
