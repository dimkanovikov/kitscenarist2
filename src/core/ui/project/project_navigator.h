#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Навигатор по структуре проекта
 */
class ProjectNavigator : public StackWidget
{
    Q_OBJECT

public:
    explicit ProjectNavigator(QWidget* _parent = nullptr);
    ~ProjectNavigator() override;

    /**
     * @brief Задать модель документов проекта
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Задать модель контекстного меню навигатора
     */
    void setContextMenuActions(const QVector<QAction*>& _actions);

    /**
     * @brief Сохранить состояние
     */
    QVariant saveState() const;

    /**
     * @brief Восстановить состояние
     */
    void restoreState(const QVariant& _state);

    /**
     * @brief Выделить элемент с заданным индексом в дереве
     */
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Получить индекс выделенного элемента в дереве
     */
    QModelIndex currentIndex() const;

    /**
     * @brief Отобразить навигатор проекта
     */
    void showProjectNavigator();

    /**
     * @brief Показан ли в данный момент навигатор проекта
     */
    bool isProjectNavigatorShown() const;

signals:
    /**
     * @brief Пользователь выбрал заданный элемент структуры
     */
    void itemSelected(const QModelIndex& _index);

    /**
     * @brief Пользователь сделал двойной клик на элементе структуры
     */
    void itemDoubleClicked(const QModelIndex& _index);

    /**
     * @brief Пользователь хочет открыть контекстное меню и поэтому нужно обновить контекстное меню
     */
    void contextMenuUpdateRequested(const QModelIndex& _index);

    /**
     * @brief Пользователь нажал кнопку добавления документа
     */
    void addDocumentClicked();

protected:
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
