#pragma once

#include <QAbstractListModel>


namespace BusinessLayer {

class ScreenplayTextModel;


/**
 * @brief Модель комментариев к тексту сценария
 */
class ScreenplayTextCommentsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        ReviewMarkAuthorEmailRole = Qt::UserRole + 1,
        ReviewMarkCreationDateRole,
        ReviewMarkCommentRole,
        ReviewMarkColorRole,
        ReviewMarkIsDoneRole,
        ReviewMarkCommentsRole
    };

public:
    explicit ScreenplayTextCommentsModel(QObject* _parent = nullptr);
    ~ScreenplayTextCommentsModel() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model);

    /**
     * @brief Получить индекс элемента из модели сценария, в котором находится заданный комментарий,
     *        а так же его смещение внутри блока
     */
    struct PositionHint {
        QModelIndex index;
        int blockPosition;
    };
    PositionHint mapToScreenplay(const QModelIndex& _index);

    /**
     * @brief Получить индекс заметки из индекса элемента модели сценария и позиции в абзаце
     */
    QModelIndex mapFromScreenplay(const QModelIndex& _index, int _positionInBlock);

    /**
     * @brief Пометить заданные элементы выполнеными
     */
    void markAsDone(const QModelIndexList& _indexes);

    /**
     * @brief Пометить заданные элементы невыполнеными
     */
    void markAsUndone(const QModelIndexList& _indexes);

    /**
     * @brief Добавить комментарий к редакторской заметке
     */
    void addComment(const QModelIndex& _index, const QString& _comment);

    /**
     * @brief Удалить выбранные элементы
     */
    void remove(const QModelIndexList& _indexes);

    /**
     * @brief Реализация модели списка
     */
    int rowCount(const QModelIndex& _parent = {}) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
