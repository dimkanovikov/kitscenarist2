#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

#include <corelib_global.h>


/**
 * @brief Виджет для отображения панелей выбора кастомного цвета
 */
class CORE_LIBRARY_EXPORT ColorPicker : public StackWidget
{
    Q_OBJECT

public:
    explicit ColorPicker(QWidget* _parent = nullptr);
    ~ColorPicker() override;

signals:
    /**
     * @brief Пользователь выбрал цвет
     */
    void colorSelected(const QColor& _color);

    /**
     * @brief Пользователь хочет добавить заданный цвет
     */
    void addColorPressed(const QColor& _color);

    /**
     * @brief Пользователь передумал добавлять цвет
     */
    void cancelPressed();

protected:
    void updateTranslations() override;

    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
