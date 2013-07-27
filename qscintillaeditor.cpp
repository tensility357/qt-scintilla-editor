#include <ScintillaEdit.h>

#include <QApplication>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

#include "qscintillaeditor.h"
#include "ui_qscintillaeditor.h"

QScintillaEditor::QScintillaEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QScintillaEditor),
    nameSet(false) {
    ui->setupUi(this);

    edit = new ScintillaEdit(this);
    setCentralWidget(edit);
}

QScintillaEditor::~QScintillaEditor() {
    delete ui;
}

void QScintillaEditor::on_actionNew_triggered() {
    if (checkModifiedAndSave()) {
        // Clear the file name and the editor
        edit->clearAll();
        edit->setSavePoint();
        nameSet = false;
        fileInfo.setFile("");
    }
}

void QScintillaEditor::on_actionOpen_triggered()
{
    if (checkModifiedAndSave()) {
        // Display the open file dialog
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
        if (!fileName.isEmpty()) {
            // Save the file
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::critical(this,  QApplication::applicationName(),
                    tr("The file cannot be opened."));
                return;
            }
            QTextStream in(&file);
            QString content = in.readAll();
            edit->setText(content.toAscii());
            file.close();

            // File saved succesfully
            edit->setSavePoint();
            nameSet = true;
            fileInfo.setFile(fileName);
        }
    }
}

bool QScintillaEditor::checkModifiedAndSave() {
    // If the file has been modified, promt the user to save the changes
    if (edit->modify()) {
        // Ask the user if the file should be saved
        QString message = QString(tr("File %1 has been modified"))
                .arg(nameSet ? tr("Untitled") : fileInfo.fileName());
        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
            QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        // Get the user response
        int ret = msgBox.exec();
        switch (ret) {
        case QMessageBox::Save:
            // Try to save the file
            return saveFile();
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
            // User canceled, do not clear
            return false;
        default:
            // Should never be reached
            break;
        }
    }

    return true;
}

bool QScintillaEditor::saveFile() {
    // Get a file name if there is none
    if (!nameSet) {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"));
        if (fileName.isEmpty()) {
            return false;
        }
        fileInfo.setFile(fileName);
        nameSet = true;
    }

    // Save the file
    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::WriteOnly)) {
        // Cannot write file, display an error message
        QMessageBox::critical(this, QApplication::applicationName(),
            tr("The file cannot be saved"));
        return false;
    } else {
        // Save the text to a file.
        QTextStream stream(&file);
        QByteArray content = edit->getText(edit->textLength() + 1);
        stream << QString::fromAscii(content);
        stream.flush();
        file.close();

        // Set a savepoint
        edit->setSavePoint();
    }

    return true;
}

