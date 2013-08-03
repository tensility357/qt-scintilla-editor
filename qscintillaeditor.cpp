#include <ScintillaEdit.h>

#include <QtGlobal>

#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "buffer.h"
#include "findreplacedialog.h"
#include "qscintillaeditor.h"
#include "ui_qscintillaeditor.h"

QScintillaEditor::QScintillaEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QScintillaEditor),
    wasMaximized(false),
    findDlg(0) {
    ui->setupUi(this);

    edit = new Buffer(parent);
    setCentralWidget(edit);

    connect(edit, SIGNAL(savePointChanged(bool)), this,
        SLOT(savePointChanged(bool)));
    connect(edit, SIGNAL(updateUi()), this, SLOT(updateUi()));
}

QScintillaEditor::~QScintillaEditor() {
    delete ui;
}

void QScintillaEditor::on_actionNew_triggered() {
    QScintillaEditor *w = new QScintillaEditor;
    w->show();
}

void QScintillaEditor::on_actionOpen_triggered() {
    if (checkModifiedAndSave()) {
        // Display the open file dialog
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
        if (!fileName.isEmpty()) {
            if (!edit->open(fileName)) {
                QMessageBox::critical(this, tr("Open File Error"),
                    tr("The file cannot be opened."));
            }
        }
    }
}

void QScintillaEditor::on_actionSave_triggered() {
    saveFile();
}

void QScintillaEditor::on_actionSaveAs_triggered() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"));
    if (!fileName.isEmpty()) {
        saveFile(fileName);
    }
}

void QScintillaEditor::on_actionClose_triggered() {
    close();
}

void QScintillaEditor::on_actionUndo_triggered() {
    edit->undo();
    ui->actionRedo->setEnabled(edit->canRedo());
}

void QScintillaEditor::on_actionRedo_triggered() {
    edit->redo();
    ui->actionRedo->setEnabled(edit->canRedo());
}

void QScintillaEditor::on_actionCut_triggered() {
    edit->cut();
}

void QScintillaEditor::on_actionCopy_triggered() {
    edit->copy();
}

void QScintillaEditor::on_actionPaste_triggered() {
    edit->paste();
}

void QScintillaEditor::on_actionFind_triggered() {
    if (!findDlg) {
        findDlg = new FindReplaceDialog(this);
        connect(findDlg, SIGNAL(findPressed()), this, SLOT(find()));
    }
    findDlg->setType(FindReplaceDialog::Find);

    findDlg->show();
    findDlg->raise();
    findDlg->activateWindow();
}

void QScintillaEditor::on_actionFindNext_triggered() {
    find();
}

void QScintillaEditor::on_actionGoTo_triggered() {
    int lineCount = edit->lineCount();
    bool ok;
    int line = QInputDialog::getInt(this, tr("Line number"), tr("Go to line"),
        1, 1, lineCount + 1, 1, &ok);
    if (ok) {
        edit->gotoLine(line - 1);
    }
}

void QScintillaEditor::on_actionSelectAll_triggered() {
    edit->selectAll();
}

void QScintillaEditor::on_actionToolBar_triggered() {
    ui->mainToolBar->setVisible(ui->actionToolBar->isChecked());
}

void QScintillaEditor::on_actionStatusBar_triggered() {
    ui->statusBar->setVisible(ui->actionStatusBar->isChecked());
}

void QScintillaEditor::on_actionFullscreen_triggered() {
    if (isFullScreen()) {
        if (wasMaximized) {
            showMaximized();
        } else {
            showNormal();
        }
    } else {
        wasMaximized = isMaximized();
        showFullScreen();
    }
}

void QScintillaEditor::on_actionZoomIn_triggered() {
    edit->zoomIn();
    if (edit->zoom() == 20) {
        ui->actionZoomIn->setEnabled(false);
    }
    ui->actionZoomOut->setEnabled(true);
}

void QScintillaEditor::on_actionZoomOut_triggered() {
    edit->zoomOut();
    if (edit->zoom() == -10) {
        ui->actionZoomOut->setEnabled(false);
    }
    ui->actionZoomIn->setEnabled(true);
}

void QScintillaEditor::on_actionResetZoom_triggered() {
    edit->setZoom(0);
    ui->actionZoomOut->setEnabled(true);
    ui->actionZoomIn->setEnabled(true);
}

void QScintillaEditor::on_actionWhitespace_triggered() {
    if (edit->viewWS() == SCWS_INVISIBLE) {
        edit->setViewWS(SCWS_VISIBLEALWAYS);
    } else {
        edit->setViewWS(SCWS_INVISIBLE);
    }
}

void QScintillaEditor::on_actionWordWrap_triggered() {
    edit->setWrapMode(ui->actionWordWrap->isChecked() ? 1 : 0);
}

void QScintillaEditor::on_actionFont_triggered() {
    // Read the current font
    QString family = edit->styleFont(STYLE_DEFAULT);
    int pointSize = edit->styleSize(STYLE_DEFAULT);
    bool bold = edit->styleBold(STYLE_DEFAULT);
    bool italic = edit->styleItalic(STYLE_DEFAULT);
    QFont initial(family, pointSize,
        bold ? QFont::Bold : QFont::Normal, italic);
    // Show the font dialog
    bool ok;
    QFont font = QFontDialog::getFont(&ok, initial);
    if (ok) {
        edit->styleSetFont(STYLE_DEFAULT, font.family().toLatin1());
        edit->styleSetSize(STYLE_DEFAULT, font.pointSize());
        edit->styleSetBold(STYLE_DEFAULT, font.bold());
        edit->styleSetItalic(STYLE_DEFAULT, font.italic());
    }
}

void QScintillaEditor::find() {
    QString findText = findDlg->findText();
    if (!findText.isEmpty()) {
        // Set the search flags
        int flags = 0;
        if (findDlg->matchCase()) {
            flags |= SCFIND_MATCHCASE;
        }
        if (findDlg->wholeWord()) {
            flags |= SCFIND_WHOLEWORD;
        }
        if (findDlg->regularExpression()) {
            flags |= SCFIND_REGEXP;
        }
        edit->setSearchFlags(flags);
        // Perform the search
        bool forward = findDlg->directionForward();
        edit->setTargetStart(forward ? edit->currentPos() :
                                       edit->currentPos() - 1);
        edit->setTargetEnd(forward ? edit->length() : 0);
        QByteArray findArray = findText.toUtf8();
        int findPos = edit->searchInTarget(findArray.length(), findArray);
        // If the search should wrap, perform the search again.
        bool searchWrapped = false;
        if (findPos == -1 && findDlg->wrapSearch()) {
            edit->setTargetStart(forward ? 0 : edit->length());
            edit->setTargetEnd(forward ? edit->currentPos() :
                                         edit->currentPos() - 1);
            findPos = edit->searchInTarget(findArray.length(), findArray);
            searchWrapped = true;
        }
        if (findPos == -1) {
            ui->statusBar->showMessage(tr("The text was not found."));
        } else {
            edit->setSel(edit->targetStart(), edit->targetEnd());
            edit->scrollRange(edit->targetStart(), edit->targetEnd());
            ui->statusBar->showMessage(searchWrapped ? tr("Search wrapped") :
                                                       tr(""));
        }
    }
}

void QScintillaEditor::savePointChanged(bool dirty) {
    ui->actionSave->setEnabled(dirty);
    ui->actionUndo->setEnabled(dirty);
}

void QScintillaEditor::updateUi() {
    ui->actionCut->setEnabled(!edit->selectionEmpty());
    ui->actionCopy->setEnabled(!edit->selectionEmpty());
}

bool QScintillaEditor::checkModifiedAndSave() {
    // If the file has been modified, promt the user to save the changes
    if (edit->modify()) {
        // Ask the user if the file should be saved
        QString message = QString(tr("File '%1' has been modified"))
                .arg(edit->displayName());
        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setInformativeText(tr("Do you want to save your changes?"));
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
            // Discard the file contents
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

bool QScintillaEditor::saveFile(const QString &fileName) {
    // Get a file name if there is none
    QString newFileName;
    if (!fileName.isEmpty()) { // Save with the provided file name.
        newFileName = fileName;
    } else {
        newFileName = edit->filePath();

        if (newFileName.isEmpty()) {
            QString selectedFileName = QFileDialog::getSaveFileName(this,
                tr("Save File"));
            if (selectedFileName.isEmpty()) {
                return false;
            }
            newFileName = selectedFileName;
        }
    }

    if (!edit->save(newFileName)) {
        // Cannot write file, display an error message
        QMessageBox::critical(this, tr("Save File Error"),
            tr("The file cannot be saved"));
        return false;
    }

    return true;
}

void QScintillaEditor::closeEvent(QCloseEvent *event) {
    if (!checkModifiedAndSave()) {
        event->ignore();
    }
}
