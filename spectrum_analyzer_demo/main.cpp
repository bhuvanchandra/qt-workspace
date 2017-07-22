#include "mainwindow.h"
#include <QApplication>

#include <signal.h>
#include <unistd.h>

void catchUnixSignals(const std::vector<int>& quitSignals,
                      const std::vector<int>& ignoreSignals = std::vector<int>()) {

    auto handler = [](int sig) ->void {
        printf("\nquit the application (user request signal = %d).\n", sig);
        QCoreApplication::quit();
    };

    // all these signals will be ignored.
    for ( int sig : ignoreSignals )
        signal(sig, SIG_IGN);

    // each of these signals calls the handler (quits the QCoreApplication).
    for ( int sig : quitSignals )
        signal(sig, handler);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP, SIGKILL});
    MainWindow w;
    w.show();

    return a.exec();
}
