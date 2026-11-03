#include "../mainwindow.hpp"

#include "../widgets/convergence_widget.hpp"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextCursor>
#include <QLabel>
#include <QSpinBox>
#include <QPlainTextEdit>

void MainWindow::runSolverSingle()
{
    if (configFile.isEmpty())
    {
        appendOutput(
            "ERROR: Select a working directory and load a .cfg file first.\n");
        showSection("Working Directory");
        return;
    }

    if (process->state() != QProcess::NotRunning)
    {
        appendOutput("SU2 is already running.\n");
        return;
    }

    process->setWorkingDirectory(caseDirectory);
    // convergenceWidget->clearValues();

    const QString configName = QFileInfo(configFile).fileName();

    appendOutput("Running: SU2_CFD " + configName + "\n");
    process->start("SU2_CFD", {configName});
}

void MainWindow::runSolverMPI()
{
    if (configFile.isEmpty())
    {
        appendOutput(
            "ERROR: Select a working directory and load a .cfg file first.\n");
        showSection("Working Directory");
        return;
    }

    if (process->state() != QProcess::NotRunning)
    {
        appendOutput("SU2 is already running.\n");
        return;
    }

    const int ranks = mpiRanks ? mpiRanks->value() : 4;

    process->setWorkingDirectory(caseDirectory);
    // convergenceWidget->clearValues();

    const QString configName = QFileInfo(configFile).fileName();

    const QStringList args = {
        "-np", QString::number(ranks), "SU2_CFD", configName};

    appendOutput("Running: mpirun " + args.join(' ') + "\n");
    process->start("mpirun", args);
}

void MainWindow::stopSolver()
{
    if (process->state() == QProcess::NotRunning)
    {
        appendOutput("SU2 is not running.\n");
        return;
    }

    appendOutput("Stopping SU2...\n");
    process->terminate();
}

void MainWindow::readStandardOutput()
{
    const QString output = QString::fromLocal8Bit(
        process->readAllStandardOutput());

    appendOutput(output);
    parseConvergence(output);
}

void MainWindow::readStandardError()
{
    appendOutput(QString::fromLocal8Bit(
        process->readAllStandardError()));
}

void MainWindow::appendOutput(const QString &text)
{
    if (!terminal)
        return;

    terminal->moveCursor(QTextCursor::End);
    terminal->insertPlainText(text);
    terminal->moveCursor(QTextCursor::End);
}

void MainWindow::parseConvergence(const QString &text)
{
    static const QRegularExpression number(
        R"(([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[Ee][-+]?\d+)?))");

    const QStringList lines = text.split('\n');

    for (const QString &line : lines)
    {
        if (!line.contains("RMS_", Qt::CaseInsensitive))
            continue;

        QRegularExpressionMatchIterator it =
            number.globalMatch(line);

        double last = -1.0;

        while (it.hasNext())
            last = it.next().captured(1).toDouble();

        // if (last > 0.0)
        //     convergenceWidget->addValue(last);
    }
}

void MainWindow::processFinished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    appendOutput(
        QString("\nSU2 finished. Exit code: %1\n")
            .arg(exitCode));

    if (exitStatus == QProcess::CrashExit)
        appendOutput("Process crashed.\n");
}
