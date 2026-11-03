#include "../mainwindow.hpp"
#include "../widgets/convergence_widget.hpp"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QListWidget>
#include <QTextStream>
#include <QLabel>
#include <QPlainTextEdit>
#include <QApplication>


void MainWindow::newCase()
{
    if (process->state() != QProcess::NotRunning)
    {
        appendOutput(
            "Cannot create a new case while SU2 is running.\n");
        return;
    }

    caseDirectory.clear();
    configFile.clear();
    meshFile.clear();

    if (convergenceWidget)
        convergenceWidget->clearValues();

    if (terminal)
        terminal->clear();

    clearViewport();
    showSection("Working Directory");
    updateCaseState();
}

void MainWindow::selectWorkingDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        "Select SU2 Case Working Directory",
        caseDirectory.isEmpty()
            ? QDir::homePath()
            : caseDirectory);

    if (directory.isEmpty())
        return;

    showLoading("Loading case...");
    QApplication::processEvents();

    caseDirectory = QDir(directory).absolutePath();
    configFile.clear();
    meshFile.clear();

    if (convergenceWidget)
        convergenceWidget->clearValues();

    clearViewport();

    if (terminal)
        terminal->clear();

    updateCaseFiles();
    updateCaseState();

    hideLoading();
}

void MainWindow::selectConfiguration()
{
    if (caseDirectory.isEmpty())
    {
        showSection("Working Directory");
        return;
    }

    const QString filename = QFileDialog::getOpenFileName(
        this,
        "Select SU2 Configuration",
        caseDirectory,
        "SU2 Configuration (*.cfg)");

    if (filename.isEmpty())
        return;

    configFile = QFileInfo(filename).absoluteFilePath();
    updateMeshFromConfig();
    updateCaseState();
}

void MainWindow::updateCaseFiles()
{
    if (caseDirectory.isEmpty())
        return;

    QDir dir(caseDirectory);

    const QStringList cfgFiles =
        dir.entryList({"*.cfg"}, QDir::Files, QDir::Name);

    if (cfgFiles.isEmpty())
    {
        appendOutput(
            "No .cfg file found in the selected working directory.\n");
        showSection("Working Directory");
        return;
    }

    if (cfgFiles.size() == 1)
    {
        configFile = dir.absoluteFilePath(cfgFiles.first());
        updateMeshFromConfig();
        updateCaseState();
        return;
    }

    bool ok = false;

    const QString selected = QInputDialog::getItem(
        this,
        "Select SU2 Configuration",
        "Configuration:",
        cfgFiles,
        0,
        false,
        &ok);

    if (ok && !selected.isEmpty())
    {
        configFile = dir.absoluteFilePath(selected);
        updateMeshFromConfig();
        updateCaseState();
    }
}

void MainWindow::updateMeshFromConfig()
{
    if (configFile.isEmpty())
        // updateCaseState();
        return;

    const QString meshName = readConfigValue("MESH_FILENAME");

    if (meshName.isEmpty())
    {
        meshFile.clear();
        appendOutput(
            "ERROR: MESH_FILENAME was not found in the CFG file.\n");
        // updateCaseState();
        return;
    }

    meshFile = QDir(caseDirectory).absoluteFilePath(meshName);

    if (!QFileInfo::exists(meshFile))
    {
        appendOutput(
            "ERROR: Mesh file not found: " + meshFile + "\n");
        // updateCaseState();
        return;
    }

    showLoading("Loading");

    QApplication::processEvents();

    loadSU2Mesh(meshFile);

    hideLoading();
}

void MainWindow::updateCaseState()
{
    const bool hasDirectory = !caseDirectory.isEmpty();
    const bool hasConfig = !configFile.isEmpty();
    const bool hasMesh = !meshFile.isEmpty();

    if (directoryEdit)
        directoryEdit->setText(
            hasDirectory ? caseDirectory : QString());

    if (navigationWidget)
    {
        QTreeWidgetItem *setupItem =
            navigationWidget->topLevelItem(0);

        if (setupItem)
        {
            for (int i = 0; i < setupItem->childCount(); ++i)
            {
                QTreeWidgetItem *item =
                    setupItem->child(i);

                if (item->text(0) == "Working Directory")
                {
                    item->setFlags(
                        item->flags() | Qt::ItemIsEnabled);
                }
                else if (item->text(0) == "Case")
                {
                    if (hasDirectory)
                        item->setFlags(
                            item->flags() | Qt::ItemIsEnabled);
                    else
                        item->setFlags(
                            item->flags() & ~Qt::ItemIsEnabled);
                }
                else
                {
                    if (hasConfig)
                        item->setFlags(
                            item->flags() | Qt::ItemIsEnabled);
                    else
                        item->setFlags(
                            item->flags() & ~Qt::ItemIsEnabled);
                }
            }
        }
    }

    // if (!hasDirectory)
    // {
    //     caseStatus->setText(
    //         "No working directory selected");
    // }
    // else if (!hasConfig)
    // {
    //     caseStatus->setText(
    //         "Working directory selected — no configuration loaded");
    // }
    // else if (!hasMesh)
    // {
    //     caseStatus->setText(
    //         "Configuration loaded — mesh not found");
    // }
    // else
    // {
    //     caseStatus->setText(" ");
    // }

    if (!hasDirectory)
        return;

    if (!hasConfig)
        return;

    if (!hasMesh)
        return;

    // Update the Working Directory panel immediately
    if (sectionTitle &&
        sectionTitle->text() == "Working Directory")
    {
        updateWorkingDirectoryPanel();
    }
}

QString MainWindow::readConfigValue(const QString &key) const
{
    if (configFile.isEmpty())
        return {};

    QFile file(configFile);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream stream(&file);

    while (!stream.atEnd())
    {
        const QString line = stream.readLine().trimmed();

        if (line.startsWith('%') || line.isEmpty())
            continue;

        const int eq = line.indexOf('=');
        if (eq < 0)
            continue;

        const QString lhs = line.left(eq).trimmed();

        if (lhs.compare(key, Qt::CaseInsensitive) == 0)
            return line.mid(eq + 1).trimmed();
    }

    return {};
}

QStringList MainWindow::readConfigValues(const QString &key) const
{
    const QString value = readConfigValue(key);

    if (value.isEmpty())
        return {};

    QString cleaned = value;
    cleaned.remove('(');
    cleaned.remove(')');

    return cleaned.split(',', Qt::SkipEmptyParts);
}
