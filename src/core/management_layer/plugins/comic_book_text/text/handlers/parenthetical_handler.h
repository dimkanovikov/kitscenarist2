#pragma once

#include "standard_key_handler.h"

#include <QString>


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке ремарки
 */
class ParentheticalHandler : public StandardKeyHandler
{
public:
    explicit ParentheticalHandler(Ui::ComicBookTextEdit* _editor);

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = nullptr) override;
    void handleTab(QKeyEvent* _event = nullptr) override;
    void handleOther(QKeyEvent* _event = nullptr) override;
    /** @} */
};

} // namespace KeyProcessingLayer
