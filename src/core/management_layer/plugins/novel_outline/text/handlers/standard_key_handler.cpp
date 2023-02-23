#include "standard_key_handler.h"

#include "../novel_outline_edit.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/novel_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::NovelOutlineEdit;


namespace KeyProcessingLayer {

namespace {
/**
 * @brief Получить тип перехода/смены в зависимости от заданных параметров
 */
static TextParagraphType actionFor(bool _tab, bool _jump, TextParagraphType _blockType)
{
    const QString settingsKey
        = QString("%1/styles-%2/from-%3-by-%4")
              .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
                   (_jump ? "jumping" : "changing"), BusinessLayer::toString(_blockType),
                   (_tab ? "tab" : "enter"));
    const auto typeString = settingsValue(settingsKey).toString();
    return BusinessLayer::textParagraphTypeFromString(typeString);
}

/**
 * @brief Вспомогательные константы для использования с функцией actionFor
 */
/** @{ */
const bool kTab = true;
const bool kEnter = false;
const bool kJump = true;
const bool kChange = false;
/** @} */
} // namespace


StandardKeyHandler::StandardKeyHandler(Ui::NovelOutlineEdit* _editor)
    : AbstractKeyHandler(_editor)
{
}

TextParagraphType StandardKeyHandler::jumpForTab(TextParagraphType _blockType)
{
    return actionFor(kTab, kJump, _blockType);
}

TextParagraphType StandardKeyHandler::jumpForEnter(TextParagraphType _blockType)
{
    return actionFor(kEnter, kJump, _blockType);
}

TextParagraphType StandardKeyHandler::changeForTab(TextParagraphType _blockType)
{
    return actionFor(kTab, kChange, _blockType);
}

TextParagraphType StandardKeyHandler::changeForEnter(TextParagraphType _blockType)
{
    return actionFor(kEnter, kChange, _blockType);
}

void StandardKeyHandler::handleDelete(QKeyEvent* _event)
{
    if (!editor()->isReadOnly()) {
        //
        // Удаление
        //
        removeCharacters(false);

        //
        // Покажем подсказку, если это возможно
        //
        handleOther(_event);
    }
}

void StandardKeyHandler::handleBackspace(QKeyEvent* _event)
{
    if (!editor()->isReadOnly()) {
        //
        // Удаление
        //
        removeCharacters(true);

        //
        // Покажем подсказку, если это возможно
        //
        handleOther(_event);
    }
}

void StandardKeyHandler::handleEscape(QKeyEvent*)
{
    editor()->closeCompleter();
}

void StandardKeyHandler::handleUp(QKeyEvent* _event)
{
    if (editor()->isCompleterVisible()) {
        return;
    }

    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
    const QTextCursor::MoveMode cursorMoveMode
        = isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    QTextCursor cursor = editor()->textCursor();

    //
    // Исходная позиция курсора
    //
    int initCursorPosition = cursor.position();

    //
    // Рассчитаем количество символов от края
    //
    int marginFromLineStart = 0;
    {
        int currentLineYCoordinate = editor()->cursorRect(cursor).y();
        while (!cursor.atStart() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                break;
            }
        }
        marginFromLineStart = initCursorPosition - cursor.position() - (cursor.atStart() ? 0 : 1);
    }

    //
    // В данный момент курсор либо в начале документа, либо поднялся к концу предыдущей строки
    //

    if (!cursor.atStart()) {
        //
        // Если мы поднялись на строку вверх, но попали в невидимый блок, перейдём к предыдущему
        // видимому
        //
        const QTextBlock firstDocumentBlock = cursor.document()->firstBlock();
        while (cursor.block() != firstDocumentBlock
               && (!cursor.block().isVisible()
                   || TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                   || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor))) {
            cursor.movePosition(QTextCursor::PreviousBlock, cursorMoveMode);
            cursor.movePosition(QTextCursor::EndOfBlock, cursorMoveMode);
        }

        //
        // Сместим курсор в предыдущей строке на то кол-во символов, на которое он был смещён прежде
        //
        {
            int currentLineEndPosition = cursor.position();
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atStart()
                   && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                    break;
                }
            }

            //
            // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую
            // строку
            //
            if (!cursor.atStart()) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
            }

            int currentLineStartPosition = cursor.position();
            if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode,
                                    marginFromLineStart);
            } else {
                cursor.setPosition(currentLineEndPosition, cursorMoveMode);
            }
        }
    }

    editor()->setTextCursor(cursor);
}

void StandardKeyHandler::handleDown(QKeyEvent* _event)
{
    if (editor()->isCompleterVisible()) {
        return;
    }

    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
    const QTextCursor::MoveMode cursorMoveMode
        = isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    QTextCursor cursor = editor()->textCursor();

    //
    // Исходная позиция курсора
    //
    int initCursorPosition = cursor.position();

    //
    // Рассчитаем количество символов от края
    //
    int marginFromLineStart = 0;
    {
        int currentLineYCoordinate = editor()->cursorRect(cursor).y();
        while (!cursor.atStart() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                break;
            }
        }
        marginFromLineStart = initCursorPosition - cursor.position() - (cursor.atStart() ? 0 : 1);
    }

    //
    // Вернём курсор в исходное положение
    //
    cursor.setPosition(initCursorPosition, cursorMoveMode);

    //
    // Сместим курсор к следующей строке или к концу документа
    //
    {
        int currentLineYCoordinate = editor()->cursorRect(cursor).y();
        while (!cursor.atEnd() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            if (!cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode)) {
                break;
            }
        }
    }

    //
    // В данный момент курсор либо в конце документа, либо перешёл к началу следующей строки
    //

    if (!cursor.atEnd()) {
        //
        // Если мы опустились на строку вниз, но попали в невидимый блок, перейдём к следующему
        // видимому
        //
        while (!cursor.atEnd()
               && (!cursor.block().isVisible()
                   || TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                   || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor))) {
            cursor.movePosition(QTextCursor::NextBlock, cursorMoveMode);
            cursor.movePosition(QTextCursor::EndOfBlock, cursorMoveMode);
        }

        //
        // Сместим курсор в следующей строке на то кол-во символов, на которое он был смещён прежде
        //
        {
            int currentLineStartPosition = cursor.position();
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atEnd() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                if (!cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode)) {
                    break;
                }
            }

            //
            // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую
            // строку
            //
            if (!cursor.atEnd()) {
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
                while (
                    !cursor.atStart()
                    && (!cursor.block().isVisible()
                        || TextBlockStyle::forBlock(cursor.block())
                            == TextParagraphType::PageSplitter
                        || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                        || cursor.blockFormat().boolProperty(
                            PageTextEdit::PropertyDontShowCursor))) {
                    if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                        break;
                    }
                }
            }

            int currentLineEndPosition = cursor.position();
            if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                const int moveRepeats
                    = currentLineEndPosition - currentLineStartPosition - marginFromLineStart;
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode, moveRepeats);
            } else {
                cursor.setPosition(currentLineEndPosition, cursorMoveMode);
            }
        }
    }

    editor()->setTextCursor(cursor);
}

void StandardKeyHandler::handlePageUp(QKeyEvent* _event)
{
    QTextCursor cursor = editor()->textCursor();
    cursor.beginEditBlock();

    for (int line = 0; line < 20; ++line) {
        handleUp(_event);
    }

    cursor.endEditBlock();
}

void StandardKeyHandler::handlePageDown(QKeyEvent* _event)
{
    QTextCursor cursor = editor()->textCursor();
    cursor.beginEditBlock();

    for (int line = 0; line < 20; ++line) {
        handleDown(_event);
    }

    cursor.endEditBlock();
}

void StandardKeyHandler::handleOther(QKeyEvent*)
{
    if (editor()->isCompleterVisible()) {
        editor()->closeCompleter();
    }
}


// **** private ****


void StandardKeyHandler::removeCharacters(bool _backward)
{
    BusinessLayer::TextCursor cursor = editor()->textCursor();

    //
    // Бит нельзя удалить, если не стёрт весь текст его заголовка
    //
    if (!cursor.hasSelection()
        && TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::BeatHeading
        && !cursor.block().text().isEmpty()
        && ((cursor.positionInBlock() == 0 && _backward)
            || (cursor.positionInBlock() == cursor.block().text().length() && !_backward))) {
        return;
    }

    //
    // Если пользователь хочет удалить бит (удаляя заголовок бита), то нужно также удалить и всё
    // содержимое удаляемого бита, соответственно нужно выделить всё содержимое бита
    //
    const QSet<TextParagraphType> beatBorders = {
        TextParagraphType::PartHeading,    TextParagraphType::PartFooter,
        TextParagraphType::ChapterHeading, TextParagraphType::ChapterFooter,
        TextParagraphType::SceneHeading,   TextParagraphType::BeatHeading,
    };
    if (!cursor.atStart() && !cursor.hasSelection()
        && TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::BeatHeading
        && cursor.block().text().isEmpty() && cursor.positionInBlock() == 0 && _backward) {
        //
        // Отводим курсор на символ назад, куда будет происходить удаление и потом выделяем весь
        // текст бита, следающий за удаляемым заголовком
        //
        cursor.movePosition(QTextCursor::PreviousCharacter);
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        do {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        } while (!cursor.atEnd()
                 && !beatBorders.contains(TextBlockStyle::forBlock(cursor.block())));
        if (beatBorders.contains(TextBlockStyle::forBlock(cursor.block()))) {
            cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        }
        editor()->setTextCursorForced(cursor);
    } else if (!cursor.atEnd() && !cursor.hasSelection()
               && TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::BeatHeading
               && cursor.block().text().isEmpty()
               && cursor.positionInBlock() == cursor.block().text().length() && !_backward) {
        //
        // Отводим курсор на символ назад (если не в самом начале документа), куда будет происходить
        // удаление и потом выделяем весь текст бита, следающий за удаляемым заголовком. Если
        // удаляется бит, стоящий в самом начале документа, то по возможности выделение будет
        // расширено до первого символа в блоке идущем после бита
        //
        const bool beatStartsFromBeginningOfDocument = cursor.atStart();
        if (!beatStartsFromBeginningOfDocument) {
            cursor.movePosition(QTextCursor::PreviousCharacter);
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }
        do {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        } while (!cursor.atEnd()
                 && !beatBorders.contains(TextBlockStyle::forBlock(cursor.block())));
        if (beatBorders.contains(TextBlockStyle::forBlock(cursor.block()))) {
            if (beatStartsFromBeginningOfDocument) {
                cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            } else {
                cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            }
        }
        editor()->setTextCursorForced(cursor);
    }

    cursor.removeCharacters(_backward, editor());

    //
    // Если прям всё удалилось, то применим оставшемуся параграфу стиль заголовка сцены
    //
    if (cursor.document()->isEmpty()) {
        editor()->setCurrentParagraphType(TextParagraphType::SceneHeading);
    }
}

} // namespace KeyProcessingLayer
