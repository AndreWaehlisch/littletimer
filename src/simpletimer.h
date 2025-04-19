#ifndef SIMPLETIMER_H
#define SIMPLETIMER_H

#include <QObject>
#include <QString>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QTimer>

#include "ui_mainwindow.h"

#ifdef LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR
    #include <QWinTaskbarButton>
    #include <QWinTaskbarProgress>
#endif /* LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR */

class MainWindow;

enum class conversion_factor { ms, sec, min, h };

class SimpleTimer : public QObject
{
    Q_OBJECT

private:
    mutable QString myFactorString; // factor string for progressbar label ("min"/"sec")
    mutable QString myRemainingTimeString; // remaining time string for progressbar label ("6" in "6min")
    MainWindow *theMainWindow;
    QPushButton *thePushButton;
    QLineEdit *theLineEdit;
    QComboBox *theComboBox;
    QProgressBar *theProgressBar;
    QTimer myTimer; // when user requests 6min countdown, this timer deals with that
    QTimer myProgressBarUpdateTimer; // this timer fires every second and updates the progress bar
#ifdef LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR
    QWinTaskbarButton wintasbarbutton;
    QWinTaskbarProgress *wintaskprogress;
#endif /* LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR */
    void startStuff(); // does stuff when timer is started (e.g. disable button)
    unsigned long getConversionFactor(const int currentIndex);

public:
    bool running; // if the timer is currently running
    SimpleTimer(const Ui::MainWindow * const ui, MainWindow * const mainwindow); // constructor

public slots:
    void stopStuff(); // does stuff when timer is stopped  (e.g. enable button)
    void timerFired() const; // does stuff to inform user that timer has ended
    void updateProgressBar() const; // update theProgressBar continuously
    void startStopTimer(); // start or stop the timer, depending if "running" is true or false
};

#endif // SIMPLETIMER_H
