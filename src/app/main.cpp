#include <QCoreApplication>
#include <QApplication>
#include <cstring>
#include <cstdio>

int run_frontend_app(int argc, char *argv[]);
int run_backend_app(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--backend") == 0) {
            QCoreApplication app(argc, argv);
            return run_backend_app(argc, argv);
        }
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("Gestor-Horarios v%s\n", PROJECT_VERSION);
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0) {
            std::printf("Uso: gestor-horarios [--backend] [--version] [--help]\n");
            return 0;
        }
    }
    QApplication app(argc, argv);
    return run_frontend_app(argc, argv);
}
