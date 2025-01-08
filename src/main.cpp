#include "mainwindow.h"

// from https://forum.qt.io/topic/133694/using-alwaysactivatewindow-to-gain-foreground-in-win10-using-qt6-2
// see also: https://doc.qt.io/qt-5/qwindowswindowfunctions.html#WindowActivationBehavior-enum
#ifdef LITTLETIMER_DO_WIN_BRINGTOFRONT
    #include <private/qguiapplication_p.h>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv); // create the QApplication (there can only be a single one)
    app.setQuitOnLastWindowClosed(false);  // if all windows (timers) are hidden because they are minimized to tray and then one timer is closed, then the app would close without this

#ifdef LITTLETIMER_DO_WIN_BRINGTOFRONT
    if (auto inf = qApp->nativeInterface<QNativeInterface::Private::QWindowsApplication>()) {
        inf->setWindowActivationBehavior(QNativeInterface::Private::QWindowsApplication::AlwaysActivateWindow);
    }
#endif

    MainWindow::theIcon = QIcon(":/hourglass.ico");

    const QStringList arguments = app.arguments();

    // create the first default timer window (gets auto deleted)
    MainWindow *w = new MainWindow();
    w->show();
    if (arguments.size() > 1)
        w->setWindowTitle(arguments.mid(1).join(' ')); // build the window title from command line arguments (except the 0th, i.e, the program name)

    return app.exec();
}
