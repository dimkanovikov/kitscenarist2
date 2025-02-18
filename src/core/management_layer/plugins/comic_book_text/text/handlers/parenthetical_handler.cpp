#include "parenthetical_handler.h"

#include "../comic_book_text_edit.h"

#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ComicBookTextEdit;


namespace KeyProcessingLayer {

ParentheticalHandler::ParentheticalHandler(ComicBookTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void ParentheticalHandler::handleEnter(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());
    // ... префикс и постфикс стиля
    const auto& style
        = editor()->comicBookTemplate().paragraphStyle(TextParagraphType::Parenthetical);
    QString stylePrefix = style.prefix();
    QString stylePostfix = style.postfix();


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Ни чего не делаем
        //
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(TextParagraphType::Parenthetical);
        } else {
            //! Нет выделения

            if ((cursorBackwardText.isEmpty() && cursorForwardText.isEmpty())
                || (cursorBackwardText + cursorForwardText == stylePrefix + stylePostfix)) {
                //! Текст пуст

                //
                // Ни чего не делаем
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::Parenthetical));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty() || cursorBackwardText == stylePrefix) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty() || cursorForwardText == stylePostfix) {
                    //! В конце блока

                    //
                    // Перейдём к блоку реплики
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock);
                    editor()->setTextCursor(cursor);
                    editor()->addParagraph(jumpForEnter(TextParagraphType::Parenthetical));
                } else {
                    //! Внутри блока

                    //
                    // Переместим обрамление в правильное место
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock);
                    if (cursorForwardText.endsWith(stylePostfix)) {
                        for (int deleteReplays = stylePostfix.length(); deleteReplays > 0;
                             --deleteReplays) {
                            cursor.deletePreviousChar();
                        }
                    }
                    cursor = editor()->textCursor();

                    //
                    // Перейдём к блоку реплики
                    //
                    editor()->setTextCursor(cursor);
                    editor()->addParagraph(TextParagraphType::Dialogue);
                }
            }
        }
    }
}

void ParentheticalHandler::handleTab(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());
    // ... префикс и постфикс стиля
    const auto& style
        = editor()->comicBookTemplate().paragraphStyle(TextParagraphType::Parenthetical);
    QString stylePrefix = style.prefix();
    QString stylePostfix = style.postfix();


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Ни чего не делаем
        //
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Ни чего не делаем
            //
        } else {
            //! Нет выделения

            if ((cursorBackwardText.isEmpty() && cursorForwardText.isEmpty())
                || (cursorBackwardText + cursorForwardText == stylePrefix + stylePostfix)) {
                //! Текст пуст

                //
                // Меняем стиль на реплику
                //
                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::Parenthetical));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty() || cursorBackwardText == stylePrefix) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty() || cursorForwardText == stylePostfix) {
                    //! В конце блока

                    //
                    // Вставляем блок реплики
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock);
                    editor()->setTextCursor(cursor);
                    editor()->addParagraph(jumpForTab(TextParagraphType::Parenthetical));
                } else {
                    //! Внутри блока

                    //
                    // Ни чего не делаем
                    //
                }
            }
        }
    }
}

void ParentheticalHandler::handleOther(QKeyEvent* _event)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());

    //
    // Была нажата открывающая скобка
    //
    if (_event != 0 && _event->text() == "(") {
        //
        // Удаляем введённую скобку
        //
        cursor.deletePreviousChar();

        //
        // BUG: Если курсор в начале документа, то не прорисовывается текст
        //
    }
    //
    // Была нажата закрывающая скобка
    //
    else if (_event != 0 && _event->text() == ")") {
        //
        // Во всех случаях удаляем введённую скобку
        //
        cursor.deletePreviousChar();

        //
        // Если в блоке, в котором есть текст
        //
        if (!currentBlock.text().isEmpty()) {
            //
            // Переходим к блоку реплики
            //
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addParagraph(TextParagraphType::Dialogue);
        }
    }
    //
    // В противном случае, обрабатываем в базовом классе
    //
    else {
        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer
