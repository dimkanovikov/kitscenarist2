#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Панель инструментов редактора текста
 */
class ScreenplayTitlePageEditToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ScreenplayTitlePageEditToolbar(QWidget* _parent = nullptr);
    ~ScreenplayTitlePageEditToolbar() override;

    /**
     * @brief Задать текущий шрифт у курсора
     */
    void setCurrentFont(const QFont& _font);

signals:
    void undoPressed();
    void redoPressed();
    void fontChanged(const QFont& _font);

protected:
    /**
     * @brief Скрываем попап, когда фокус ушёл из виджета
     */
    void focusOutEvent(QFocusEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
