#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Представление приложения
 */
class ApplicationView : public Widget
{
    Q_OBJECT

public:
    explicit ApplicationView(QWidget* _parent = nullptr);
    ~ApplicationView() override;

    /**
     * @brief Сохранить состояние
     */
    QVariantMap saveState() const;

    /**
     * @brief Восстановить состояние
     */
    void restoreState(const QVariantMap& _state);

    /**
     * @brief Показать заданный контент
     */
    void showContent(QWidget* _toolbar, QWidget* _navigator, QWidget* _view);

    /**
     * @brief Установить панель инструментов для работы с аккаунтом
     */
    void setAccountBar(Widget* _accountBar);

signals:
    /**
     * @brief Запрос на закрытие приложения
     */
    void closeRequested();

protected:
    /**
     * @brief Корректируем расположение виджета личного кабинета внутри виджета представления
     */
    bool eventFilter(QObject* _target, QEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы вместо реального закрытия испустить сигнал о данном намерении
     */
    void closeEvent(QCloseEvent* _event) override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
