#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке реплики
 */
class DialogHandler : public StandardKeyHandler
{
public:
    explicit DialogHandler(Ui::ScreenplayTextEdit* _editor);

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
