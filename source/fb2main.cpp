#include <QtGui>
#include <QtDebug>

#include "fb2main.h"
#include "fb2doc.h"

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerxml.h>

MainWindow::MainWindow()
{
    init();
    createText();
    setCurrentFile("");
}

MainWindow::MainWindow(const QString &filename)
{
    init();
    createQsci();
    loadXML(filename);
    setCurrentFile(filename);
}

MainWindow::MainWindow(const QString &filename, QTextDocument * document)
{
    init();
    createText();
    if (!document) document = loadFB2(filename);
    setCurrentFile(filename, document);
}

bool MainWindow::loadXML(const QString &filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            qsciEdit->clear();
            return qsciEdit->read(&file);
        }
    }
    return false;
}

Fb2MainDocument * MainWindow::loadFB2(const QString &filename)
{
    if (filename.isEmpty()) return NULL;

    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << tr("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString());
        return NULL;
    }

    return Fb2MainDocument::load(file);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::newFile()
{
    MainWindow *other = new MainWindow;
    other->move(x() + 40, y() + 40);
    other->show();
}

void MainWindow::open()
{
    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.isEmpty()) return;

    MainWindow * existing = findMainWindow(filename);
    if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
    }

    if (textEdit) {
        QTextDocument * document = loadFB2(filename);
        if (!document) return;

        if (isUntitled && textEdit->document()->isEmpty() && !isWindowModified()) {
            setCurrentFile(filename, document);
        } else {
            MainWindow * other = new MainWindow(filename, document);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    } else if (qsciEdit) {
        if (isUntitled && !isWindowModified()) {
            loadXML(filename);
            setCurrentFile(filename);
        } else {
            MainWindow * other = new MainWindow(filename);
            other->move(x() + 40, y() + 40);
            other->show();
        }

    }

}

bool MainWindow::save()
{
    if (isUntitled) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), curFile);
    if (fileName.isEmpty()) return false;
    return saveFile(fileName);
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About SDI"),
            tr("The <b>SDI</b> example demonstrates how to write single "
               "document interface applications using Qt."));
}

void MainWindow::documentWasModified()
{
    QFileInfo info = windowFilePath();
    QString title = info.fileName();
    title += QString("[*]") += QString(" - ") += qApp->applicationName();
    setWindowTitle(title);
    setWindowModified(true);
}

void MainWindow::init()
{
    setAttribute(Qt::WA_DeleteOnClose);

    isUntitled = true;

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    readSettings();

    setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::createText()
{
    textEdit = new QTextEdit;
    textEdit->setAcceptRichText(true);
    setCentralWidget(textEdit);

    connect(cutAct, SIGNAL(triggered()), textEdit, SLOT(cut()));
    connect(copyAct, SIGNAL(triggered()), textEdit, SLOT(copy()));
    connect(pasteAct, SIGNAL(triggered()), textEdit, SLOT(paste()));

    connect(textEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    connect(textEdit, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
}

void MainWindow::createQsci()
{
    //  http://qtcoder.blogspot.com/2010/10/qscintills.html
    //  http://www.riverbankcomputing.co.uk/static/Docs/QScintilla2/classQsciScintilla.html

    qsciEdit = new QsciScintilla;
    qsciEdit->setUtf8(true);
    qsciEdit->setCaretLineVisible(true);
    qsciEdit->setCaretLineBackgroundColor(QColor("gainsboro"));
    qsciEdit->setWrapMode(QsciScintilla::WrapWord);

    qsciEdit->setEolMode(QsciScintilla::EolWindows);

    qsciEdit->setAutoIndent(true);
    qsciEdit->setIndentationGuides(true);

    qsciEdit->setAutoCompletionSource(QsciScintilla::AcsAll);
    qsciEdit->setAutoCompletionCaseSensitivity(true);
    qsciEdit->setAutoCompletionReplaceWord(true);
    qsciEdit->setAutoCompletionShowSingle(true);
    qsciEdit->setAutoCompletionThreshold(2);

    qsciEdit->setMarginsBackgroundColor(QColor("gainsboro"));
    qsciEdit->setMarginWidth(0, 0);
    qsciEdit->setMarginLineNumbers(1, true);
    qsciEdit->setMarginWidth(1, QString("1000"));
    qsciEdit->setFolding(QsciScintilla::BoxedFoldStyle, 2);

    qsciEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    qsciEdit->setMatchedBraceBackgroundColor(Qt::yellow);
    qsciEdit->setUnmatchedBraceForegroundColor(Qt::blue);

    QFont font("Courier", 10);
    font.setStyleHint(QFont::TypeWriter);

    QsciLexerXML * lexer = new QsciLexerXML;
    lexer->setFont(font, -1);

    qsciEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    qsciEdit->setLexer(lexer);

    setCentralWidget(qsciEdit);
    qsciEdit->setFocus();

    //    connect(qsciEdit, SIGNAL(textChanged()), this, SLOT(documentWasModified()));
    //    connect(qsciEdit, SIGNAL(cursorPositionChanged(int, int)), this, SLOT(cursorMoved(int, int)));
}

void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    closeAct = new QAction(tr("&Close"), this);
    closeAct->setShortcut(tr("Ctrl+W"));
    closeAct->setStatusTip(tr("Close this window"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));

    textAct = new QAction(tr("&Text"), this);
    textAct->setCheckable(true);
    connect(textAct, SIGNAL(triggered()), this, SLOT(viewText()));

    qsciAct = new QAction(tr("&XML"), this);
    qsciAct->setCheckable(true);
    connect(qsciAct, SIGNAL(triggered()), this, SLOT(viewQsci()));

    QActionGroup * viewGroup = new QActionGroup(this);
    viewGroup->addAction(textAct);
    viewGroup->addAction(qsciAct);
    textAct->setChecked(true);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(closeAct);
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    editMenu = menuBar()->addMenu(tr("&View"));
    editMenu->addAction(textAct);
    editMenu->addAction(qsciAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings;
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    move(pos);
    resize(size);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

bool MainWindow::maybeSave()
{
    if (textEdit && textEdit->document()->isModified()) {
	QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("SDI"),
                     tr("The document has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard
		     | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

bool MainWindow::saveFile(const QString &fileName)
{
    return false;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("SDI"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textEdit->toPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &filename, QTextDocument * document)
{
    static int sequenceNumber = 1;

    QString title;
    isUntitled = filename.isEmpty();
    if (isUntitled) {
        curFile = tr("book%1.fb2").arg(sequenceNumber++);
        title = curFile;
    } else {
        QFileInfo info = filename;
        curFile = info.canonicalFilePath();
        title = info.fileName();
    }
    title += QString(" - ") += qApp->applicationName();

    if (document) {
        textEdit->setDocument(document);
        textEdit->document()->setModified(false);
        connect(textEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    }

    setWindowModified(false);
    setWindowFilePath(curFile);
    setWindowTitle(title);
}

MainWindow *MainWindow::findMainWindow(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QWidget *widget, qApp->topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin && mainWin->curFile == canonicalFilePath)
            return mainWin;
    }
    return 0;
}

void MainWindow::viewQsci()
{
    if (centralWidget() == qsciEdit) return;
    if (textEdit) { delete textEdit; textEdit = NULL; }
    createQsci();
}

void MainWindow::viewText()
{
    if (centralWidget() == textEdit) return;
    if (qsciEdit) { delete qsciEdit; qsciEdit = NULL; }
    createText();
}
