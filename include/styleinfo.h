#ifndef STYLEINFO_H
#define STYLEINFO_H

#include <QString>

/**
 * Contains information about a style for a language.
 */
struct StyleDescription {
    /**
     * Creates the style description.
     *
     * @param stl The Scintilla identifier of the style.
     * @param desc he description for the style.
     */
    StyleDescription(unsigned char stl, QString desc);

    /** The Scintilla identifier of the style */
    unsigned char style;

    /** The description for the style. */
    QString description;
};

class StyleInfo {
public:
    static StyleInfo fromString(const QString& string);

    StyleInfo(int foregroundColor = -1, int backgroundColor = -1,
              bool bold = false, bool italic = false, bool underline = false,
              bool eolFilled = false);

    /**
     * Returns the foreground color in the format expected by Scintilla. If -1,
     * then this color has not been set.
     *
     * @return The foreground color in the format expected by Scintilla.
     */
    int foregroundColor() const;

    void setForegroundColor(int foregroundColor);

    /**
     * Returns the background color in the format expected by Scintilla. If -1,
     * then this color has not been set.
     *
     * @return The background color in the format expected by Scintilla.
     */
    int backgroundColor() const;

    void setBackgroundColor(int backgroundColor);

    /**
     * Returns true if the style should be bolded.
     *
     * @return true if the style should be bolded.
     */
    bool bold() const;

    void setBold(bool bold);

    /**
     * Returns true if the style should be italicized.
     *
     * @return true if the style should be italicized.
     */
    bool italic() const;

    void setItalic(bool italic);

    /**
     * Returns true if the style should be underlined.
     *
     * @return true if the style should be underlined.
     */
    bool underline() const;

    void setUnderline(bool underline);

    /**
     * Returns true if the remainder of the line up to the right edge of the
     * window is filled with the background colour set for the last character.
     *
     * @return true if the remainder of the line up to the right edge of the
     * window is filled with the background colour set for the last character.
     */
    bool eolFilled() const;

    void setEolFilled(bool eolFilled);

private:
    /** Holds the foreground color. */
    int m_foregroundColor;
    /** Holds the background color. */
    int m_backgroundColor;
    /** true for bold style. */
    bool m_bold;
    /** true for italic style. */
    bool m_italic;
    /** true for underline style. */
    bool m_underline;
    /** true if the remainder of the line up to the right edge of the window is
     * filled with the background colour set for the last character. */
    bool m_eolFilled;
};

#endif // STYLEINFO_H
