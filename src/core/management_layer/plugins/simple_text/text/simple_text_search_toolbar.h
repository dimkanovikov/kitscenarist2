#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui {

/**
 * @brief Панель поиска по тексту
 */
class SimpleTextSearchToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit SimpleTextSearchToolbar(QWidget* _parent = nullptr);
    ~SimpleTextSearchToolbar() override;

    /**
     * @brief Возвращаем фокус после поиска
     */
    void refocus();

    /**
     * @brief Текст для поиска
     */
    QString searchText() const;

    /**
     * @brief Нужно ли учитывать регистр при поиске
     */
    bool isCaseSensitive() const;

    /**
     * @brief Текст на который нужно заменить
     */
    QString replaceText() const;

signals:
    /**
     * @brief Пользователь хочет закрыть панель поиска
     */
    void closePressed();

    /**
     * @brief Пользоутель хочет сфокусироваться на текста
     */
    void focusTextRequested();

    /**
     * @brief Пользователь хочет найти заданный текст
     */
    void findTextRequested();

    /**
     * @brief Пользователь хочет посмотреть на следующее совпадение
     */
    void findNextRequested();

    /**
     * @brief Пользователь хочет посмотреть на предыдущее совпадение
     */
    void findPreviousRequested();

    /**
     * @brief Пользователь хочет заменить текущий найденный текст на заданный
     */
    void replaceOnePressed();

    /**
     * @brief Пользователь хочет заменить все вхождения поискового запроса на заданный
     */
    void replaceAllPressed();

protected:
    /**
     * @brief Ловим событие изменения размера родительского виджета, чтобы скорректировать свои
     * размеры
     *        + события потери фокуса для скрытия попапа
     *        + события нажатия ENTER в поле поиска
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Корректируем цвета вложенных виджетов
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

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
