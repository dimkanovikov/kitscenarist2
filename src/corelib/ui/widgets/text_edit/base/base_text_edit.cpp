#include "base_text_edit.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QKeyEvent>
#include <QLocale>
#include <QRegularExpression>
#include <QStyleHints>
#include <QTextBlock>

namespace {

/**
 * @brief Расположить курсор так, чтобы извлечь форматирование для последующего инвертирования
 */
QTextCursor cursorForFormatInversion(const QTextCursor& _cursor)
{
    if (!_cursor.hasSelection()) {
        return _cursor;
    }

    auto cursor = _cursor;
    //
    // Максимум, потому что позиция в начале выделения имеет формат предыдущего символа
    //
    cursor.setPosition(std::max(_cursor.selectionStart(), _cursor.selectionEnd()));
    //
    // Если конец выделения попал в блок, в котором нет форматов, идём назад, в поисках блока,
    // обладающего форматированием, и тогда мы сможем из него извлечь необходимую информацию
    // для последующего инвертирования
    //
    const auto startPosition = std::min(_cursor.selectionStart(), _cursor.selectionEnd());
    while (cursor.block().textFormats().isEmpty() && cursor.position() > startPosition) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
    }
    return cursor;
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
/** @} */

} // namespace


class BaseTextEdit::Implementation
{
public:
    explicit Implementation(BaseTextEdit* _q);

    /**
     * @brief Перенастроить редактор в соответствии с актуальной дизайн системой
     */
    void reconfigure();

    /**
     * @brief Выделить блок при тройном клике
     */
    bool selectBlockOnTripleClick(QMouseEvent* _event);

    /**
     * @brief Обновить выравнивание в текущем блоке
     */
    void updateCurrentBlockAlignment();


    BaseTextEdit* q = nullptr;

    bool capitalizeWords = true;
    bool correctDoubleCapitals = true;
    bool replaceThreeDots = false;
    bool smartQuotes = false;
    bool compressSpaces = false;

    /**
     * @brief Количеств
     */
    int mouseClicks = 0;

    /**
     * @brief Время последнего клика мышки, мс
     */
    qint64 lastMouseClickTime = 0;

    /**
     * @brief Изначальное выравнивание блока
     */
    QHash<int, Qt::Alignment> blockNumberToSourceAlignment;
};

BaseTextEdit::Implementation::Implementation(BaseTextEdit* _q)
    : q(_q)
{
}

void BaseTextEdit::Implementation::reconfigure()
{
    q->setCursorWidth(Ui::DesignSystem::layout().px2());
}

bool BaseTextEdit::Implementation::selectBlockOnTripleClick(QMouseEvent* _event)
{
    if (_event->button() == Qt::RightButton) {
        return false;
    }

    const qint64 curentMouseClickTime = QDateTime::currentMSecsSinceEpoch();
    const qint64 timeDelta = curentMouseClickTime - lastMouseClickTime;
    if (timeDelta <= (QApplication::styleHints()->mouseDoubleClickInterval())) {
        mouseClicks += 1;
    } else {
        mouseClicks = 1;
    }
    lastMouseClickTime = curentMouseClickTime;

    //
    // Тройной клик обрабатываем самостоятельно
    //
    if (mouseClicks > 2) {
        mouseClicks = 1;
        QTextCursor cursor = q->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        q->setTextCursor(cursor);
        _event->accept();
        return true;
    }

    return false;
}

void BaseTextEdit::Implementation::updateCurrentBlockAlignment()
{
    //
    // Корректировки выполняются только для RTL, т.к. кьют не умеет рисовать курсор в правильном
    // изначальном положении для RTL
    //
    if (q->isLeftToRight()) {
        return;
    }

    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    auto cursor = q->textCursor();
    cursor.beginEditBlock();
    do {
        if (cursor.block().text().isEmpty()) {
            if (!blockNumberToSourceAlignment.contains(cursor.blockNumber())) {
                blockNumberToSourceAlignment[cursor.blockNumber()]
                    = cursor.blockFormat().alignment();

                auto blockFormat = cursor.blockFormat();
                blockFormat.setAlignment(q->isLeftToRight() ? Qt::AlignLeft : Qt::AlignRight);
                cursor.setBlockFormat(blockFormat);
            }
            continue;
        }

        auto blockFormat = cursor.blockFormat();
        blockFormat.setAlignment(blockNumberToSourceAlignment.take(cursor.blockNumber()));
        cursor.setBlockFormat(blockFormat);
    } while (cursor.movePosition(QTextCursor::NextBlock));
    cursor.endEditBlock();
}


// ****


BaseTextEdit::BaseTextEdit(QWidget* _parent)
    : CompleterTextEdit(_parent)
    , d(new Implementation(this))
{
    d->reconfigure();

    connect(this, &BaseTextEdit::textChanged, this, [this] { d->updateCurrentBlockAlignment(); });
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
    TextHelper::updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::setTextItalic(bool _italic)
{
    auto buildFormat = [_italic](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontItalic(_italic);
        return format;
    };
    TextHelper::updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::setTextUnderline(bool _underline)
{
    auto buildFormat = [_underline](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontUnderline(_underline);
        return format;
    };
    TextHelper::updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::setTextStrikethrough(bool _strikethrough)
{
    auto buildFormat = [_strikethrough](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontStrikeOut(_strikethrough);
        return format;
    };
    TextHelper::updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::invertTextBold()
{
    setTextBold(!cursorForFormatInversion(textCursor()).charFormat().font().bold());
}

void BaseTextEdit::invertTextItalic()
{
    setTextItalic(!cursorForFormatInversion(textCursor()).charFormat().font().italic());
}

void BaseTextEdit::invertTextUnderline()
{
    setTextUnderline(!cursorForFormatInversion(textCursor()).charFormat().font().underline());
}

void BaseTextEdit::invertTextStrikethrough()
{
    setTextStrikethrough(!cursorForFormatInversion(textCursor()).charFormat().font().strikeOut());
}

void BaseTextEdit::setTextFont(const QFont& _font)
{
    auto buildFormat = [_font](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFont(_font);
        return format;
    };
    TextHelper::updateSelectionFormatting(textCursor(), buildFormat);
}

void BaseTextEdit::setTextAlignment(Qt::Alignment _alignment)
{
    auto cursor = textCursor();
    const int startPosition = std::min(cursor.selectionStart(), cursor.selectionEnd());
    const int lastPosition = std::max(cursor.selectionStart(), cursor.selectionEnd());
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

    cursor.setPosition(lastPosition);
    setTextCursor(cursor);
}

ContextMenu* BaseTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    auto menu = CompleterTextEdit::createContextMenu(_position, _parent);

    auto formattingAction = new QAction;
    formattingAction->setText(tr("Formatting"));
    formattingAction->setIconText(u8"\U000F0284");
    {
        auto boldAction = new QAction(formattingAction);
        boldAction->setText(tr("Bold"));
        boldAction->setWhatsThis(
            QKeySequence(QKeySequence::Bold).toString(QKeySequence::NativeText));
        boldAction->setEnabled(textCursor().hasSelection());
        connect(boldAction, &QAction::triggered, this, &BaseTextEdit::invertTextBold);
        //
        auto italicAction = new QAction(formattingAction);
        italicAction->setText(tr("Italic"));
        italicAction->setWhatsThis(
            QKeySequence(QKeySequence::Italic).toString(QKeySequence::NativeText));
        italicAction->setEnabled(textCursor().hasSelection());
        connect(italicAction, &QAction::triggered, this, &BaseTextEdit::invertTextItalic);
        //
        auto underlineAction = new QAction(formattingAction);
        underlineAction->setText(tr("Underline"));
        underlineAction->setWhatsThis(
            QKeySequence(QKeySequence::Underline).toString(QKeySequence::NativeText));
        underlineAction->setEnabled(textCursor().hasSelection());
        connect(underlineAction, &QAction::triggered, this, &BaseTextEdit::invertTextUnderline);

        auto strikethroughAction = new QAction(formattingAction);
        strikethroughAction->setText(tr("Strikethrough"));
        strikethroughAction->setWhatsThis(
            QKeySequence("Shift+Ctrl+X").toString(QKeySequence::NativeText));
        strikethroughAction->setEnabled(textCursor().hasSelection());
        connect(strikethroughAction, &QAction::triggered, this,
                &BaseTextEdit::invertTextStrikethrough);

        auto alignLeftAction = new QAction(formattingAction);
        alignLeftAction->setSeparator(true);
        alignLeftAction->setText(tr("Align left"));
        alignLeftAction->setWhatsThis(QKeySequence("Ctrl+L").toString(QKeySequence::NativeText));
        connect(alignLeftAction, &QAction::triggered, this,
                [this] { setTextAlignment(Qt::AlignLeft); });
        //
        auto alignCenterAction = new QAction(formattingAction);
        alignCenterAction->setText(tr("Align center"));
        alignCenterAction->setWhatsThis(QKeySequence("Ctrl+E").toString(QKeySequence::NativeText));
        connect(alignCenterAction, &QAction::triggered, this,
                [this] { setTextAlignment(Qt::AlignHCenter); });
        //
        auto alignRightAction = new QAction(formattingAction);
        alignRightAction->setText(tr("Align right"));
        alignRightAction->setWhatsThis(QKeySequence("Ctrl+R").toString(QKeySequence::NativeText));
        connect(alignRightAction, &QAction::triggered, this,
                [this] { setTextAlignment(Qt::AlignRight); });
        //
        auto alignJustifyAction = new QAction(formattingAction);
        alignJustifyAction->setText(tr("Align justify"));
        alignJustifyAction->setWhatsThis(QKeySequence("Ctrl+J").toString(QKeySequence::NativeText));
        connect(alignJustifyAction, &QAction::triggered, this,
                [this] { setTextAlignment(Qt::AlignJustify); });
    }

    //
    // В зависимости от того, есть ли меню проверки орфографии добавляем пункт меню форматирования
    //
    auto actions = menu->actions().toVector();
    if (isMispelledWordUnderCursor(_position)) {
        actions.insert(1, formattingAction);
    } else {
        actions.first()->setSeparator(true);
        actions.prepend(formattingAction);
    }
    menu->setActions(actions);

    return menu;
}

bool BaseTextEdit::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
        d->reconfigure();
        updateGeometry();
        update();
        return true;
    }

    case QEvent::LayoutDirectionChange: {
        d->updateCurrentBlockAlignment();
        Q_FALLTHROUGH();
    }

    default: {
        return CompleterTextEdit::event(_event);
    }
    }
}

void BaseTextEdit::mousePressEvent(QMouseEvent* _event)
{
    const auto isHandled = d->selectBlockOnTripleClick(_event);
    if (isHandled) {
        return;
    }

    CompleterTextEdit::mousePressEvent(_event);
}

void BaseTextEdit::mouseDoubleClickEvent(QMouseEvent* _event)
{
    const auto isHandled = d->selectBlockOnTripleClick(_event);
    if (isHandled) {
        return;
    }

    CompleterTextEdit::mouseDoubleClickEvent(_event);
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
    // ... сделать текст перечёркнутым
    //
    else if (_event->key() == Qt::Key_X && _event->modifiers().testFlag(Qt::ShiftModifier)
             && _event->modifiers().testFlag(Qt::ControlModifier)) {
        invertTextStrikethrough();
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
    } else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_J) {
        setTextAlignment(Qt::AlignJustify);
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

        if (QString selectedText = cursor.selectedText(); !selectedText.isEmpty()) {
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
    }
    // ... перевод курсора к следующему символу
    //
    else if (_event == QKeySequence::MoveToNextChar) {
        const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
        auto cursor = textCursor();
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            cursor.movePosition(QTextCursor::NextCharacter,
                                isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        } else {
            cursor.movePosition(QTextCursor::PreviousCharacter,
                                isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        }
        setTextCursor(cursor);
    }
    //
    // ... перевод курсора к предыдущему символу
    //
    else if (_event == QKeySequence::MoveToPreviousChar) {
        const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
        auto cursor = textCursor();
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            cursor.movePosition(QTextCursor::PreviousCharacter,
                                isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        } else {
            cursor.movePosition(QTextCursor::NextCharacter,
                                isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        }
        setTextCursor(cursor);
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
    } else if (_event == QKeySequence::SelectAll) {
        auto cursor = textCursor();
        cursor.select(QTextCursor::Document);
        setTextCursor(cursor);
    }
    //
    // ... вставим перенос строки внутри абзаца
    //
    else if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
             && _event->modifiers().testFlag(Qt::ShiftModifier)) {
        textCursor().insertText(QChar(QChar::LineSeparator));
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
        const auto isPreviousCharacterUpper = cursorBackwardText.length() > 3
            && cursorBackwardText[cursorBackwardText.length() - 4].isLetter()
            && cursorBackwardText[cursorBackwardText.length() - 4]
                == TextHelper::smartToUpper(cursorBackwardText[cursorBackwardText.length() - 4]);
        const auto right3Characters = cursorBackwardText.right(3).simplified();

        //
        // Если две из трёх последних букв находятся в верхнем регистре, то это наш случай
        //
        if (!right3Characters.contains(" ") && right3Characters.length() == 3
            && right3Characters != TextHelper::smartToUpper(right3Characters)
            && right3Characters.left(2) == TextHelper::smartToUpper(right3Characters.left(2))
            && right3Characters[0].isLetter() && right3Characters[1].isLetter()
            && _eventText != TextHelper::smartToUpper(_eventText) && !isPreviousCharacterUpper) {
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

    //
    // Если была попытка ввести несколько пробелов подряд, или пробел в начале строки,
    // удаляем этот лишний пробел
    //
    if (d->compressSpaces && (cursorBackwardText == " " || cursorBackwardText.endsWith("  "))) {
        cursor.deletePreviousChar();

        return true;
    }

    return false;
}

void BaseTextEdit::doSetTextCursor(const QTextCursor& _cursor)
{
    QTextCursor cursor = _cursor;
    const auto sourceSelectionStart = _cursor.selectionStart() != _cursor.position()
        ? _cursor.selectionStart()
        : _cursor.selectionEnd();
    const auto sourceSelectionEnd = _cursor.selectionStart() != _cursor.position()
        ? _cursor.selectionEnd()
        : _cursor.selectionStart();
    const bool isSelectionForward = sourceSelectionStart < sourceSelectionEnd;
    cursor.setPosition(sourceSelectionStart);
    while ((isSelectionForward ? !cursor.atEnd() : !cursor.atStart())
           && (!cursor.block().isVisible()
               || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor))) {
        const auto isCursorMoved = cursor.movePosition(
            isSelectionForward ? QTextCursor::NextBlock : QTextCursor::PreviousBlock);
        if (isCursorMoved) {
            cursor.movePosition(isSelectionForward ? QTextCursor::StartOfBlock
                                                   : QTextCursor::EndOfBlock);
        } else {
            cursor.movePosition(isSelectionForward ? QTextCursor::EndOfBlock
                                                   : QTextCursor::StartOfBlock);
        }
    }
    const int selectionsStart = cursor.position();

    //
    // Если естьв ыделение, определим и завершающую позицию курсора
    //
    if (_cursor.hasSelection()) {
        cursor.setPosition(sourceSelectionEnd);
        while ((isSelectionForward ? !cursor.atStart() : !cursor.atEnd())
               && (!cursor.block().isVisible()
                   || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor))) {
            const auto isCursorMoved = cursor.movePosition(
                isSelectionForward ? QTextCursor::PreviousBlock : QTextCursor::NextBlock);
            if (isCursorMoved) {
                cursor.movePosition(isSelectionForward ? QTextCursor::EndOfBlock
                                                       : QTextCursor::StartOfBlock);
            } else {
                cursor.movePosition(isSelectionForward ? QTextCursor::StartOfBlock
                                                       : QTextCursor::EndOfBlock);
            }
        }
        const int selectionEnd = cursor.position();
        cursor.setPosition(selectionsStart);
        cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);
    }
    //
    // А если выделения нет, то проверим кейс, когда пользователь нажимает влево находять в описании
    // самого первого бита истории
    //
    else {
        if (!cursor.block().isVisible()) {
            while (
                !cursor.atEnd()
                && (!cursor.block().isVisible()
                    || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor))) {
                const auto isCursorMoved = cursor.movePosition(QTextCursor::NextBlock);
                if (isCursorMoved) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                } else {
                    cursor.movePosition(QTextCursor::EndOfBlock);
                }
            }
        }
    }

    CompleterTextEdit::doSetTextCursor(cursor);
}
