#include <limits> // std::numeric_limits
#include <cmath> // ceil, nearbyint

#include "simpletimer.h"
#include "mainwindow.h"

#include <QMessageBox>
#include <QTime>

#ifdef LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR
    #include <QWinTaskbarButton>
    #include <QWinTaskbarProgress>
#endif /* LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR */

SimpleTimer::SimpleTimer(const Ui::MainWindow * const ui, MainWindow * const mainwindow) : myTimer(this), myProgressBarUpdateTimer(this)
{
    running = false;

    myTimer.setSingleShot(true); // timer only fires once
    connect(&myTimer, &QTimer::timeout, this, &SimpleTimer::stopStuff); // call our "stopStuff" func when timer expires
    connect(&myTimer, &QTimer::timeout, this, &SimpleTimer::timerFired); // call our "timerFired" func when timer expires

    myProgressBarUpdateTimer.setSingleShot(false); // fire repeatedly
    myProgressBarUpdateTimer.setInterval(1000); // fire once per second
    connect(&myProgressBarUpdateTimer, &QTimer::timeout, this, &SimpleTimer::updateProgressBar); // on every "tick" update the progress bar

    // get some pointers to ui elements
    theMainWindow = mainwindow;
    theLineEdit = ui->lineEdit;
    thePushButton = ui->pushButton;
    theComboBox = ui->comboBox;
    theProgressBar = ui->progressBar;
}

// update the progress bar (every second), called by our myProgressBarUpdateTimer, also update the tray icon tooltip (if tray icon is visible)
void SimpleTimer::updateProgressBar() const
{
    // progress bar value
    const double percent = 100.0 * myTimer.remainingTime() / myTimer.interval();
    const int value = static_cast<int>(nearbyint(percent));
    theProgressBar->setValue(value);

#ifdef LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR
    wintaskprogress->setValue(value);
#endif /* LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR */

    // label text
    if (myTimer.remainingTime() > 60000) { // >1min
        myFactorString = "min";

        if (myTimer.remainingTime() > 60000 * 5) { // >5min
            myRemainingTimeString.setNum(ceil(myTimer.remainingTime() / 60000.), 'f', 0); // for "big minutes" we just use the minute (always round up to full minutes)
        } else { // <=5min and >1min
            myRemainingTimeString.setNum(myTimer.remainingTime() / 60000., 'f', 1); // for "small minutes" we use one number after the decimal delimiter (rounds to next 0.1 minute). "showpoint" forces the decimal point.
        }
    } else {
        myFactorString = "sec";
        myRemainingTimeString.setNum(myTimer.remainingTime() / 1000., 'f', 0); // round to full seconds
    }

    theProgressBar->setFormat(myRemainingTimeString + myFactorString);

    // update tray icon tooltip
    if (theMainWindow->myTray->isVisible()) {
        theMainWindow->myTray->setToolTip(theMainWindow->windowTitle() + ": " + myRemainingTimeString + myFactorString);
    }
}

void SimpleTimer::startStuff()
{
    running = true;

    thePushButton->setText(tr("Stop"));
    theLineEdit->setDisabled(true);
    theComboBox->setDisabled(true);
    theProgressBar->setEnabled(true);

#ifdef LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR
    // Init the qwintaskbarbutton. From the documentation:
    // QWidget::windowHandle() returns a valid instance of a QWindow only after the widget has been shown. It is therefore recommended to delay the initialization of the QWinTaskbarButton instances until QWidget::showEvent().
    if (wintasbarbutton.window() == Q_NULLPTR) {
        wintasbarbutton.setWindow(theMainWindow->windowHandle());
        wintaskprogress = wintasbarbutton.progress();
    }

    wintaskprogress->show();
#endif /* LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR */

    myTimer.start();
    myProgressBarUpdateTimer.start();
    updateProgressBar(); // ProgressBarUpdateTimer does not run until 1sec after we start our stuff, so do a manual update here
}

void SimpleTimer::stopStuff()
{
    myTimer.stop();
    myProgressBarUpdateTimer.stop();

#ifdef LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR
    wintaskprogress->hide();
#endif /* LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR */

    running = false;

    thePushButton->setText(tr("Start"));
    theLineEdit->setDisabled(false);
    theComboBox->setDisabled(false);
    theProgressBar->setEnabled(false);
    theProgressBar->setValue(0);
    theProgressBar->setFormat("");
}

void SimpleTimer::timerFired() const
{
    theMainWindow->myTray->setToolTip(""); // remove tray tooltip
    theMainWindow->showNormal(); // restore window

    // actual warning that timer fired for user:
    QMessageBox msg(QMessageBox::Warning, tr("Attention"), tr("Alarm: ") + theMainWindow->windowTitle(), QMessageBox::Ok, thePushButton);
    msg.setWindowModality(Qt::WindowModal);
    msg.exec();
}

unsigned long SimpleTimer::getConversionFactor(const int currentIndex)
{
    unsigned long factor = 0; // factor to convert input value to ms

    // Check which conversion factor user has selected
    switch (static_cast<conversion_factor>(currentIndex)) {
	case conversion_factor::ms:
        factor = 1;
        break;
	case conversion_factor::sec:
        factor = 1000;
        break;
	case conversion_factor::min:
        factor = 1000 * 60;
        break;
	case conversion_factor::h:
        factor = 1000 * 60 * 60;
        break;
    }

    return factor;
}

void SimpleTimer::startStopTimer()
{
    const QString inputString = theLineEdit->text().replace(',', '.'); // holds the user input
    static const QRegularExpression regex = QRegularExpression("^(\\d{1,2}):(\\d{1,2})$");
    const QStringList captures = regex.match(inputString).capturedTexts();

    // If running: Stop timer. Else: Start timer.
    if (running) {
        // Check if user input is a time of day or period of time
        if (captures.length() != 3) {
            // set text of LineEdit to current Timer value
            const int remainingTime = myTimer.remainingTime();
            const unsigned long factor = getConversionFactor(theComboBox->currentIndex()); // factor to convert input value to ms
            theLineEdit->setText(QString::number(static_cast<double>(remainingTime) / factor));
        }

        stopStuff();
    } else {
        int newInterval;

        // Check if user input is a time of day or period of time
        if (captures.length() == 3) {
            const QTime timeInput(captures.at(1).toInt(), captures.at(2).toInt());

            if (!timeInput.isValid()) {
                QMessageBox::warning(thePushButton->parentWidget(), tr("Attention"), tr("Invalid input time (format: HH:MM)."));
                return;
            }

            newInterval = -timeInput.msecsTo(QTime::currentTime());

            // if timeInput is tomorrow (newInterval is negative) we need to do "24hours + newInterval"
            if (newInterval < 0) {
                newInterval += 24 * 60 * 60 * 1000;
            }
        } else {
            const unsigned long factor = getConversionFactor(theComboBox->currentIndex()); // factor to convert input value to ms

            // Convert user input
            bool conversionOkay;
            const double input = inputString.toDouble(&conversionOkay); // try to convert user input QString to double

            // Test if conversion was okay (see http://doc.qt.io/qt-5/qstring.html#toDouble) [note: if not ok, then input=0]. QTimer uses int (msec), so make sure we are in the limit of that, also check for negative numbers.
            if (!conversionOkay || static_cast<double>(input * factor) > std::numeric_limits<int>::max() || input <= 0) {
                QMessageBox::warning(thePushButton->parentWidget(), tr("Attention"), tr("Invalid input! Must be a positive number, which can't be too big (max 596h)."));
                return;
            }

            newInterval = static_cast<int>(input * factor);
        }

        myTimer.setInterval(newInterval); // convert input to msec and start the (single shot) timer
        startStuff();
    }
}
