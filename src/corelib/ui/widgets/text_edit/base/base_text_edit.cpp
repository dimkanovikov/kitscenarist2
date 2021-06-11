#include "base_text_edit.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QLocale>
#include <QRegularExpression>
#include <QTextBlock>

namespace {
/**
 * @brief Обновить форматированние выделенного блока в соответствии с заданным функтором
 */
template<typename Func>
void updateSelectionFormatting(QTextCursor _cursor, Func updateFormat)
{
    if (!_cursor.hasSelection()) {
        return;
    }

    const auto positionInterval = std::minmax(_cursor.selectionStart(), _cursor.selectionEnd());
    int position = positionInterval.first;
    const int lastPosition = positionInterval.second;
    while (position < lastPosition) {
        const auto block = _cursor.document()->findBlock(position);
        for (const auto& format : block.textFormats()) {
            const auto formatStart = block.position() + format.start;
            const auto formatEnd = formatStart + format.length;
            if (position >= formatEnd) {
                continue;
            }
            if (formatStart >= lastPosition) {
                break;
            }

            _cursor.setPosition(std::max(formatStart, position));
            _cursor.setPosition(std::min(formatEnd, lastPosition), QTextCursor::KeepAnchor);

            const auto newFormat = updateFormat(format.format);
            _cursor.mergeCharFormat(newFormat);

            _cursor.clearSelection();
            _cursor.movePosition(QTextCursor::NextCharacter);
            position = _cursor.position();
        }
    }
}

/**
 * @brief Оканчивается ли строка сокращением
 */
bool stringEndsWithAbbrev(const QString& _text)
{
    Q_UNUSED(_text);

    //
    // TODO: Добавить словарь сокращений. Возможно его можно вытащить из ханспела
    //

    return false;
}

/**
 * @brief Функции для получения корректных кавычек в зависимости от локали приложения
 */
/** @{ */
QString localeQuote(bool _open)
{
    switch (QLocale().language()) {
    default: {
        return "\"";
    }

    case QLocale::Russian:
    case QLocale::Spanish:
    case QLocale::French: {
        if (_open) {
            return "«";
        } else {
            return "»";
        }
    }

    case QLocale::English: {
        if (_open) {
            return "“";
        } else {
            return "”";
        }
    }
    }
}
QString localOpenQuote()
{
    return localeQuote(true);
}
QString localCloseQuote()
{
    return localeQuote(false);
}
/** @{ */
} // namespace


class BaseTextEdit::Implementation
{
public:
    void reconfigure(BaseTextEdit* _textEdit);

    bool capitalizeWords = true;
    bool correctDoubleCapitals = true;
    bool replaceThreeDots = false;
    bool smartQuotes = false;
};

void BaseTextEdit::Implementation::reconfigure(BaseTextEdit* _textEdit)
{
    _textEdit->setCursorWidth(Ui::DesignSystem::layout().px2());
}


// ****


BaseTextEdit::BaseTextEdit(QWidget* _parent)
    : CompleterTextEdit(_parent)
    , d(new Implementation)
{
    d->reconfigure(this);
}

BaseTextEdit::~BaseTextEdit() = default;

void BaseTextEdit::setCapitalizeWords(bool _capitalize)
{
    d->capitalizeWords = _capitalize;
}

bool BaseTextEdit::capitalizeWords() const
{
    return d->capitalizeWords;
}

void BaseTextEdit::setCorrectDoubleCapitals(bool _correct)
{
    d->correctDoubleCapitals = _correct;
}

bool BaseTextEdit::correctDoubleCapitals() const
{
    return d->correctDoubleCapitals;
}

void BaseTextEdit::setTextBold(bool _bold)
{
    auto buildFormat = [_bold](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontWeight(_bold ? QFont::Bold : QFont::Normal);
        return format;
    };
    updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::setTextItalic(bool _italic)
{
    auto buildFormat = [_italic](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontItalic(_italic);
        return format;
    };
    updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::setTextUnderline(bool _underline)
{
    auto buildFormat = [_underline](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontUnderline(_underline);
        return format;
    };
    updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::invertTextBold()
{
    auto cursor = textCursor();
    cursor.setPosition(std::max(cursor.selectionStart(), cursor.selectionEnd()));
    setTextBold(!cursor.charFormat().font().bold());
}

void BaseTextEdit::invertTextItalic()
{
    auto cursor = textCursor();
    cursor.setPosition(std::max(cursor.selectionStart(), cursor.selectionEnd()));
    setTextItalic(!cursor.charFormat().font().italic());
}

void BaseTextEdit::invertTextUnderline()
{
    auto cursor = textCursor();
    cursor.setPosition(std::max(cursor.selectionStart(), cursor.selectionEnd()));
    setTextUnderline(!cursor.charFormat().font().underline());
}

void BaseTextEdit::setTextFont(const QFont& _font)
{
    auto buildFormat = [_font](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFont(_font);
        return format;
    };
    updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::setTextAlignment(Qt::Alignment _alignment)
{
    auto cursor = textCursor();
    const auto positionInterval = std::minmax(cursor.selectionStart(), cursor.selectionEnd());
    const int startPosition = positionInterval.first;
    const int lastPosition = positionInterval.second;
    cursor.setPosition(startPosition);
    do {
        auto blockFormat = cursor.blockFormat();
        if (blockFormat.alignment() != _alignment) {
            blockFormat.setAlignment(_alignment);
            cursor.setBlockFormat(blockFormat);
        }

        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    } while (!cursor.atEnd() && cursor.position() < lastPosition);

    cursor.setPosition(positionInterval.second);
    setTextCursor(cursor);
}

bool BaseTextEdit::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
        d->reconfigure(this);
        updateGeometry();
        update();
        return true;
    }

    default: {
        return CompleterTextEdit::event(_event);
    }
    }
}

bool BaseTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    bool isEventHandled = true;

    //
    // Переопределяем
    //

    //
    // Для форматирования нужно брать значение в стиле у последнего символа из выделения,
    // т.к. charFormat() отдаёт значение стиля предыдущего перед курсором символа
    //
    // ... сделать текст полужирным
    //
    if (_event == QKeySequence::Bold) {
        invertTextBold();
    }
    //
    // ... сделать текст курсивом
    //
    else if (_event == QKeySequence::Italic) {
        invertTextItalic();
    }
    //
    // ... сделать текст подчёркнутым
    //
    else if (_event == QKeySequence::Underline) {
        invertTextUnderline();
    }
    //
    // Выравнивание
    //
    else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_L) {
        setTextAlignment(Qt::AlignLeft);
    } else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_E) {
        setTextAlignment(Qt::AlignHCenter);
    } else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_R) {
        setTextAlignment(Qt::AlignRight);
    }
    //
    // Поднятие/опускание регистра букв
    // Работает в три шага:
    // 1. ВСЕ ЗАГЛАВНЫЕ
    // 2. Первая заглавная
    // 3. все строчные
    //
    else if (_event->modifiers().testFlag(Qt::ControlModifier)
             && (_event->key() == Qt::Key_Up || _event->key() == Qt::Key_Down)) {
        //
        // Нужно ли убирать выделение после операции
        //
        bool clearSelection = false;
        //
        // Если выделения нет, работаем со словом под курсором
        //
        QTextCursor cursor = textCursor();
        const int sourcePosition = cursor.position();
        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
            clearSelection = true;
        }

        QString selectedText = cursor.selectedText();
        const QChar firstChar = selectedText.at(0);
        const bool firstToUpper = TextHelper::smartToUpper(firstChar) != firstChar;
        const bool textInUpper = (selectedText.length() > 1)
            && (TextHelper::smartToUpper(selectedText) == selectedText);
        const int fromPosition = qMin(cursor.selectionStart(), cursor.selectionEnd());
        const int toPosition = qMax(cursor.selectionStart(), cursor.selectionEnd());
        const bool toUpper = _event->key() == Qt::Key_Up;
        for (int position = fromPosition; position < toPosition; ++position) {
            cursor.setPosition(position);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            selectedText = cursor.selectedText();
            if (toUpper) {
                //
                // Поднимаем для всего текста, или только для первого символа
                //
                if (!firstToUpper || (firstToUpper && position == fromPosition)) {
                    cursor.insertText(TextHelper::smartToUpper(selectedText));
                }
            } else {
                //
                // Опускаем для всего текста, или для всех символов, кроме первого
                //
                if (!textInUpper || (textInUpper && position != fromPosition)) {
                    cursor.insertText(TextHelper::smartToLower(selectedText));
                }
            }
        }

        if (clearSelection) {
            cursor.setPosition(sourcePosition);
        } else {
            cursor.setPosition(fromPosition);
            cursor.setPosition(toPosition, QTextCursor::KeepAnchor);
        }
        setTextCursor(cursor);
    }
    // ... перевод курсора к следующему символу
    //
    else if (_event == QKeySequence::MoveToNextChar) {
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            moveCursor(QTextCursor::NextCharacter);
        } else {
            moveCursor(QTextCursor::PreviousCharacter);
        }
    }
    //
    // ... перевод курсора к предыдущему символу
    //
    else if (_event == QKeySequence::MoveToPreviousChar) {
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            moveCursor(QTextCursor::PreviousCharacter);
        } else {
            moveCursor(QTextCursor::NextCharacter);
        }
    }
    //
    // ... перевод курсора к концу строки
    //
    else if (_event == QKeySequence::MoveToEndOfLine || _event == QKeySequence::SelectEndOfLine) {
        QTextCursor cursor = textCursor();
        const int startY = cursorRect(cursor).y();
        const QTextCursor::MoveMode keepAncor = _event->modifiers().testFlag(Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;
        while (!cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter, keepAncor);
            if (cursorRect(cursor).y() > startY) {
                cursor.movePosition(QTextCursor::PreviousCharacter, keepAncor);
                setTextCursor(cursor);
                break;
            }
        }
        setTextCursor(cursor);
    }
    //
    // ... перевод курсора к началу строки
    //
    else if (_event == QKeySequence::MoveToStartOfLine
             || _event == QKeySequence::SelectStartOfLine) {
        QTextCursor cursor = textCursor();
        const int startY = cursorRect(cursor).y();
        const QTextCursor::MoveMode keepAncor = _event->modifiers().testFlag(Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;
        while (!cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, keepAncor);
            if (cursorRect(cursor).y() < startY) {
                cursor.movePosition(QTextCursor::NextCharacter, keepAncor);
                setTextCursor(cursor);
                break;
            }
        }
        setTextCursor(cursor);
    }
#ifdef Q_OS_MAC
    //
    // Особая комбинация для вставки точки независимо от раскладки
    //
    else if (_event->modifiers().testFlag(Qt::MetaModifier)
             && _event->modifiers().testFlag(Qt::AltModifier)
             && (_event->key() == Qt::Key_Period || _event->key() == 1070)) {
        insertPlainText(".");
    }
    //
    // Особая комбинация для вставки запятой независимо от раскладки
    //
    else if (_event->modifiers().testFlag(Qt::MetaModifier)
             && _event->modifiers().testFlag(Qt::AltModifier)
             && (_event->key() == Qt::Key_Comma || _event->key() == 1041)) {
        insertPlainText(",");
    }
#endif
    //
    // Оставляем необработанным
    //
    else {
        isEventHandled = false;
    }

    return isEventHandled;
}

bool BaseTextEdit::updateEnteredText(const QString& _eventText)
{
    if (_eventText.isEmpty()) {
        return false;
    }

    //
    // Получим значения
    //
    // ... курсора
    QTextCursor cursor = textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());

    //
    // Определяем необходимость установки верхнего регистра для первого символа блока
    //
    if (d->capitalizeWords && cursorBackwardText != " " && cursorBackwardText == _eventText
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сформируем правильное представление строки
        //
        QString correctedText = _eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursor(cursor);

        return true;
    }

    //
    // Если перед нами конец предложения
    // и не сокращение
    // и после курсора нет текста (для ремарки допустима скобка)
    //
    const QString endOfSentancePattern = QString("([.]|[?]|[!]|[…]) %1$").arg(_eventText);
    if (d->capitalizeWords && cursorBackwardText.contains(QRegularExpression(endOfSentancePattern))
        && !stringEndsWithAbbrev(cursorBackwardText) && cursorForwardText.isEmpty()
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сделаем первую букву заглавной
        //
        QString correctedText = _eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursor(cursor);

        return true;
    }

    //
    // Исправляем проблему ДВойных ЗАглавных
    //
    if (d->correctDoubleCapitals) {
        QString right3Characters = cursorBackwardText.right(3).simplified();

        //
        // Если две из трёх последних букв находятся в верхнем регистре, то это наш случай
        //
        if (!right3Characters.contains(" ") && right3Characters.length() == 3
            && right3Characters != TextHelper::smartToUpper(right3Characters)
            && right3Characters.left(2) == TextHelper::smartToUpper(right3Characters.left(2))
            && right3Characters.left(2).at(0).isLetter()
            && right3Characters.left(2).at(1).isLetter()
            && _eventText != TextHelper::smartToUpper(_eventText)) {
            //
            // Сделаем предпоследнюю букву строчной
            //
            QString correctedText = right3Characters;
            correctedText[correctedText.length() - 2]
                = correctedText[correctedText.length() - 2].toLower();

            //
            // Стираем предыдущий введённый текст
            //
            for (int repeats = 0; repeats < correctedText.length(); ++repeats) {
                cursor.deletePreviousChar();
            }

            //
            // Выводим необходимый
            //
            cursor.insertText(correctedText);
            setTextCursor(cursor);

            return true;
        }
    }

    //
    // Заменяем три точки символом многоточия
    //
    if (d->replaceThreeDots && _eventText == "." && cursorBackwardText.endsWith("...")) {
        //
        // Три последних символа
        //
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 3);
        cursor.insertText("…");

        return true;
    }

    //
    // Корректируем кавычки
    //
    if (d->smartQuotes && _eventText == "\"" && localOpenQuote() != "\"") {
        //
        // Выделим введённый символ
        //
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
        //
        // Определим предшествующий текст
        //
        QTextCursor cursorCopy = cursor;
        cursorCopy.setPosition(cursor.selectionStart());
        cursorCopy.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);

        if (cursorCopy.selectedText().isEmpty()
            || QStringList({ " ", "(" }).contains(cursorCopy.selectedText().right(1))) {
            cursor.insertText(localOpenQuote());
        } else {
            cursor.insertText(localCloseQuote());
        }

        return true;
    }

    return false;
}
