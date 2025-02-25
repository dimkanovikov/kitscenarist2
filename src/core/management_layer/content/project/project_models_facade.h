#pragma once

#include <QObject>
#include <QUuid>

namespace BusinessLayer {
class AbstractImageWrapper;
class AbstractModel;
class StructureModel;
class StructureModelItem;
} // namespace BusinessLayer

namespace Domain {
class DocumentObject;
enum class DocumentObjectType;
} // namespace Domain


namespace ManagementLayer {

/**
 * @brief Фасад для работы с моделями документов проекта
 */
class ProjectModelsFacade : public QObject
{
    Q_OBJECT

public:
    explicit ProjectModelsFacade(BusinessLayer::StructureModel* _projectStructureModel,
                                 BusinessLayer::AbstractImageWrapper* _imageWrapper,
                                 QObject* _parent = nullptr);
    ~ProjectModelsFacade() override;

    /**
     * @brief Сбросить все загруженные модели
     */
    void clear();

    /**
     * @brief Получить модель для заданного документа
     */
    BusinessLayer::AbstractModel* modelFor(const QUuid& _uuid);
    BusinessLayer::AbstractModel* modelFor(Domain::DocumentObjectType _type);
    BusinessLayer::AbstractModel* modelFor(Domain::DocumentObject* _document);

    /**
     * @brief Получить список всех моделей заданного типа
     */
    QVector<BusinessLayer::AbstractModel*> modelsFor(Domain::DocumentObjectType _type);

    /**
     * @brief Удалить модель для заданного документа
     */
    void removeModelFor(Domain::DocumentObject* _document);

    /**
     * @brief Получить список загруженных моделей документов
     */
    QVector<BusinessLayer::AbstractModel*> loadedModels() const;
    QVector<BusinessLayer::AbstractModel*> loadedModelsFor(Domain::DocumentObjectType _type) const;

    /**
     * @brief Получить список загруженных документов
     */
    QVector<Domain::DocumentObject*> loadedDocuments() const;

signals:
    /**
     * @brief Изменилось название модели
     */
    void modelNameChanged(BusinessLayer::AbstractModel* _model, const QString& _name);

    /**
     * @brief Изменился цвет модели
     */
    void modelColorChanged(BusinessLayer::AbstractModel* _model, const QColor& _color);

    /**
     * @brief Изменился контент модели
     */
    void modelContentChanged(BusinessLayer::AbstractModel* _model, const QByteArray& _undo,
                             const QByteArray& _redo);

    /**
     * @brief Запрос на отмену последнего действия в модели
     */
    void modelUndoRequested(BusinessLayer::AbstractModel* _model, int _undoStep);

    /**
     * @brief Запрос на удаление модели
     */
    void modelRemoveRequested(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Изменилось название проекта
     */
    void projectNameChanged(const QString& _name);

    /**
     * @brief Изменилось короткое описание проекта
     */
    void projectLoglineChanged(const QString& _logline);

    /**
     * @brief Изменилась обложка проекта
     */
    void projectCoverChanged(const QPixmap& _cover);

    /**
     * @brief Пользователь хочет изменить список соавторов в проекте
     */
    void projectCollaboratorInviteRequested(const QString& _email, const QColor& _color, int _role,
                                            const QHash<QUuid, int>& _permissions);
    void projectCollaboratorUpdateRequested(const QString& _email, const QColor& _color, int _role,
                                            const QHash<QUuid, int>& _permissions);
    void projectCollaboratorRemoveRequested(const QString& _email);

    /**
     * @brief Сменилась видимость элемента сценария
     */
    /** @{ */
    void screenplayTitlePageVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void screenplaySynopsisVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void screenplayTreatmentVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void screenplayTextVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void screenplayStatisticsVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    /** @} */

    /**
     * @brief Сменилась видимость элемента сериала
     */
    /** @{ */
    void screenplaySeriesTitlePageVisibilityChanged(BusinessLayer::AbstractModel* _model,
                                                    bool _visible);
    void screenplaySeriesSynopsisVisibilityChanged(BusinessLayer::AbstractModel* _model,
                                                   bool _visible);
    void screenplaySeriesTreatmentVisibilityChanged(BusinessLayer::AbstractModel* _model,
                                                    bool _visible);
    void screenplaySeriesTextVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void screenplaySeriesStatisticsVisibilityChanged(BusinessLayer::AbstractModel* _model,
                                                     bool _visible);
    /** @} */

    /**
     * @brief Сменилась видимость элемента комикса
     */
    /** @{ */
    void comicBookTitlePageVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void comicBookSynopsisVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void comicBookTextVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void comicBookStatisticsVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    /** @} */

    /**
     * @brief Сменилась видимость элемента аудиопостановки
     */
    /** @{ */
    void audioplayTitlePageVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void audioplaySynopsisVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void audioplayTextVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void audioplayStatisticsVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    /** @} */

    /**
     * @brief Сменилась видимость элемента аудиопостановки
     */
    /** @{ */
    void stageplayTitlePageVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void stageplaySynopsisVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void stageplayTextVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void stageplayStatisticsVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    /** @} */

    /**
     * @brief Сменилась видимость элемента сценария
     */
    /** @{ */
    void novelTitlePageVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void novelSynopsisVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void novelOutlineVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void novelTextVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    void novelStatisticsVisibilityChanged(BusinessLayer::AbstractModel* _model, bool _visible);
    /** @} */

    /**
     * @brief Необходимо создать персонажа с заданным именем
     */
    void createCharacterRequested(const QString& _name, const QByteArray& _content);

    /**
     * @brief Изменилось имя персонажа
     */
    void characterNameChanged(BusinessLayer::AbstractModel* _character, const QString& _oldName,
                              const QString& _newName);

    /**
     * @brief Запрос на обновление списка всех реплик персонажа
     */
    void characterDialoguesUpdateRequested(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Неоходимо создать локацию с заданным именем
     */
    void createLocationRequested(const QString& _name, const QByteArray& _content);

    /**
     * @brief Изменилось название локации
     */
    void locationNameChanged(BusinessLayer::AbstractModel* _location, const QString& _oldName,
                             const QString& _newName);

    /**
     * @brief Запрос на обновление списка всех сцен локации
     */
    void locationScenesUpdateRequested(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Неоходимо создать мир с заданным именем
     */
    void createWorldRequested(const QString& _name, const QByteArray& _content);

    /**
     * @brief Запрос на обновление списка действующих лиц титульной страницы
     */
    void titlePageCharactersUpdateRequested(BusinessLayer::AbstractModel* _titlePage);

    /**
     * @brief Необходимо очистить корзинку
     */
    void emptyRecycleBinRequested();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
