#pragma once

#include "abstract_key_handler.h"

class QString;

namespace BusinessLayer {
enum class TextParagraphType;
}


namespace KeyProcessingLayer {

/**
 * @brief Реализация стандартного обработчика
 */
class StandardKeyHandler : public AbstractKeyHandler
{
public:
    explicit StandardKeyHandler(Ui::ComicBookTextEdit* _editor);

protected:
    /**
     * @brief Получить стиль блока к которому переходить
     */
    /** @{ */
    static BusinessLayer::TextParagraphType jumpForTab(BusinessLayer::TextParagraphType _blockType);
    static BusinessLayer::TextParagraphType jumpForEnter(
        BusinessLayer::TextParagraphType _blockType);
    BusinessLayer::TextParagraphType changeForTab(BusinessLayer::TextParagraphType _blockType);
    BusinessLayer::TextParagraphType changeForEnter(BusinessLayer::TextParagraphType _blockType);
    /** @} */

    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleDelete(QKeyEvent* _event = 0);
    void handleBackspace(QKeyEvent* _event = 0);
    void handleEscape(QKeyEvent* _event = 0);
    void handleUp(QKeyEvent* _event = 0);
    void handleDown(QKeyEvent* _event = 0);
    void handlePageUp(QKeyEvent* _event = 0);
    void handlePageDown(QKeyEvent* _event = 0);
    void handleOther(QKeyEvent* _event = 0);
    /** @} */

private:
    /**
     * @brief Переместить курсор вверх или вниз
     */
    void moveCursorUpDown(bool _up, bool _isShiftPressed);

    /**
     * @brief Удалить символы
     */
    void removeCharacters(bool _backward);
};

} // namespace KeyProcessingLayer
