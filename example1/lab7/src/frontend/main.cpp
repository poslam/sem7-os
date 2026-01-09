#include <QApplication>

#include "MainWindow.h"
#include "backend.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Start embedded backend (simulator mode) so GUI can talk to http://localhost:8080.
    start_backend(true);

    MainWindow w;
    w.showFullScreen();

    const int rc = app.exec();
    stop_backend();
    return rc;
}
