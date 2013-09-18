#include "buffer.h"
#include "util.h"

#include <SciLexer.h>

#include <QDebug>
#include <QDropEvent>
#include <QFontDatabase>
#include <QTextStream>
#include <QUrl>

#include <cmath>

Buffer::Buffer(QWidget *parent) :
        ScintillaEdit(parent), m_encoding("UTF-8") {
    // Use Unicode code page
    setCodePage(SC_CP_UTF8);
    // Track the scroll width
    setScrollWidth(1);
    setScrollWidthTracking(true);
    // Set a monospaced font
    QFontDatabase fontDb;
#ifdef Q_OS_WIN
    if (fontDb.families(QFontDatabase::Any).contains("Consolas")) {
        styleSetFont(STYLE_DEFAULT, "Consolas");
    } else {
        styleSetFont(STYLE_DEFAULT, "Courier New");
    }
#elif Q_OS_MAC
    if (fontDb.families(QFontDatabase::Any).contains("Menlo")) {
        styleSetFont(STYLE_DEFAULT, "Menlo");
    } else {
        styleSetFont(STYLE_DEFAULT, "Monaco");
    }
#else
    styleSetFont(STYLE_DEFAULT, "Monospace");
#endif
    // Set font size.
    styleSetSize(STYLE_DEFAULT, 10);
    // View whitespace
    setViewWS(SCWS_VISIBLEALWAYS);
    setWhitespaceFore(true, convertColor(Qt::lightGray));
    // Set up identation
    setTabWidth(4);
    setUseTabs(false);
    setIndent(4);
    setIndentationGuides(SC_IV_LOOKBOTH);
    // Set long line indicator
    setEdgeMode(EDGE_LINE);
    setEdgeColumn(80);
    // Setup the margins
    setShowLineNumbers(true);
    setShowIconMargin(false);
    setShowFoldMargin(true);
    setMarginMaskN(Fold, SC_MASK_FOLDERS);
    setMarginSensitiveN(Fold, true);
    markerDefine(SC_MARKNUM_FOLDEROPEN, SC_MARK_ARROWDOWN);
    markerDefine(SC_MARKNUM_FOLDER, SC_MARK_ARROW);
    markerDefine(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    markerDefine(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    markerDefine(SC_MARKNUM_FOLDEREND, SC_MARK_ARROW);
    markerDefine(SC_MARKNUM_FOLDEROPENMID, SC_MARK_ARROWDOWN);
    markerDefine(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

    connect(this, SIGNAL(linesAdded(int)), this, SLOT(onLinesAdded(int)));
    connect(this, SIGNAL(marginClicked(int,int,int)), this,
            SLOT(onMarginClicked(int,int,int)));
}

Buffer::~Buffer() {

}

void Buffer::clear() {
    // Clear the file name and the editor
    clearAll();
    setFileInfo(QFileInfo(""));
    setSavePoint();
}

bool Buffer::open(const QString &fileName) {
    // Open the file
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QTextStream input(&file);
    input.setCodec(m_encoding);
    QString content = input.readAll();
    setText(content.toUtf8());
    file.close();

    // File opened succesfully
    setFileInfo(QFileInfo(fileName));
    emptyUndoBuffer();
    setSavePoint();

    return true;
}

bool Buffer::save(const QString &fileName) {
    // Save the file
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    // Save the text to a file.
    QTextStream output(&file);
    QByteArray content = getText(textLength() + 1);
    output.setCodec(m_encoding);
    output << QString::fromUtf8(content);
    output.flush();
    file.close();

    // File saved
    setFileInfo(QFileInfo(fileName));
    setSavePoint();

    return true;
}

QFileInfo Buffer::fileInfo() const {
    return m_fileInfo;
}

QByteArray Buffer::encoding() const {
    return m_encoding;
}

void Buffer::setEncoding(const QByteArray& encoding) {
    if (m_encoding != encoding) {
        m_encoding = encoding;
        emit encodingChanged(encoding);
    }
}

bool Buffer::showLineNumbers() const {
    return m_showLineNumbers;
}

void Buffer::setShowLineNumbers(bool showLineNumbers) {
    m_showLineNumbers = showLineNumbers;
    if (m_showLineNumbers) {
        setMarginWidthN(Line, getLineMarginWidth());
    } else {
        setMarginWidthN(Line, 0);
    }
}

bool Buffer::showIconMargin() const {
    return m_showIconMargin;
}

void Buffer::setShowIconMargin(bool showIconMargin) {
    m_showIconMargin = showIconMargin;
    setMarginWidthN(Icon, m_showIconMargin ? 16 : 0);
}

bool Buffer::showFoldMargin() const {
    return m_showFoldMargin;
}

void Buffer::setShowFoldMargin(bool showFoldMargin) {
    m_showFoldMargin = showFoldMargin;
    setMarginWidthN(Fold, m_showFoldMargin ? 16 : 0);
}

bool Buffer::find(const QString& findText, int flags, bool forward,
                  bool wrap, bool *searchWrapped) {
    if (findText.isEmpty()) {
        return false;
    }
    *searchWrapped = false;
    // Perform the search
    setSearchFlags(flags);
    setTargetStart(forward ? currentPos() : currentPos() - 1);
    setTargetEnd(forward ? length() : 0);
    QByteArray findArray = findText.toUtf8();
    int findPos = searchInTarget(findArray.length(), findArray);
    // If the search should wrap, perform the search again.
    if (findPos == -1 && wrap) {
        setTargetStart(forward ? 0 : length());
        setTargetEnd(forward ? currentPos() : currentPos() - 1);
        findPos = searchInTarget(findArray.length(), findArray);
        if (searchWrapped != NULL) {
            *searchWrapped = true;
        }
    }
    if (findPos != -1)  {
        setSel(targetStart(), targetEnd());
        scrollRange(targetStart(), targetEnd());
    }
    return findPos != -1;
}

void Buffer::onLinesAdded(int) {
    if (m_showLineNumbers) {
        setMarginWidthN(Line, getLineMarginWidth());
    }
}

void Buffer::onMarginClicked(int position, int, int margin) {
    if (margin == Fold) {
        toggleFold(lineFromPosition(position));
    }
}

void Buffer::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls()) {
        // If the user is dropping URLs, emit a signal
        QList<QUrl> urls = event->mimeData()->urls();
        emit urlsDropped(urls);
    } else {
        // Do the default action
        ScintillaEditBase::dropEvent(event);
    }
 }

void Buffer::setFileInfo(const QFileInfo& fileInfo) {
    if (m_fileInfo != fileInfo) {
        m_fileInfo = fileInfo;
        // Set up the lexer for the buffer
        setupLexer();
        emit fileInfoChanged(fileInfo);
    }
}

void Buffer::setupLexer() {
    Language lang;

    for (size_t i = 0; i < G_LANGUAGE_COUNT; i++) {
        Language currentLang = G_AVAILABLE_LANGUAGES[i];
        QStringList extensions = currentLang.patterns.split(' ');
        for (int i = 0; i < extensions.size(); ++i) {
            QRegExp re(extensions.at(i));
            re.setPatternSyntax(QRegExp::Wildcard);
            if (re.exactMatch(m_fileInfo.fileName())) {
                lang = currentLang;
                goto outer;
            }
        }
    }

outer:
    if (lang.lexer) {
        setLexer(lang.lexer);
        setKeyWords(0, lang.keywords.toLatin1());
        setProperty("fold", "1");
        setProperty("fold.compact", "0");
    } else {
        setLexer(SCLEX_NULL);
        setKeyWords(0, "");
        setProperty("fold", "0");
    }
}

int Buffer::getLineMarginWidth() {
    int width = ((int) std::log10(lineCount())) + 1;
    QString text;
    text.fill('9', width).prepend('_');

    return textWidth(STYLE_LINENUMBER, text.toLatin1());
}