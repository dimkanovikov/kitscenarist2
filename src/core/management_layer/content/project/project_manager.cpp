#include "project_manager.h"

#include "include/custom_events.h"
#include "project_models_facade.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/base/title_page_model.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/presentation/presentation_model.h>
#include <business_layer/model/project/project_information_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>
#include <business_layer/model/structure/structure_proxy_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/model/worlds/world_model.h>
#include <business_layer/model/worlds/worlds_model.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/database.h>
#include <data_layer/storage/document_change_storage.h>
#include <data_layer/storage/document_image_storage.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_change_object.h>
#include <domain/document_object.h>
#include <domain/starcloud_api.h>
#include <include/custom_events.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <interfaces/ui/i_document_view.h>
#include <management_layer/content/projects/projects_model_project_item.h>
#include <management_layer/content/projects/projects_model_team_item.h>
#include <management_layer/plugins_builder.h>
#include <ui/abstract_navigator.h>
#include <ui/account/collaborators_tool_bar.h>
#include <ui/design_system/design_system.h>
#include <ui/project/create_document_dialog.h>
#include <ui/project/create_draft_dialog.h>
#include <ui/project/project_navigator.h>
#include <ui/project/project_tool_bar.h>
#include <ui/project/project_view.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <ui/widgets/splitter/splitter.h>
#include <utils/logging.h>
#include <utils/shugar.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QSet>
#include <QShortcut>
#include <QTimer>
#include <QUuid>
#include <QVariantAnimation>


namespace ManagementLayer {

namespace {

const QLatin1String kCurrentViewMimeTypeKey("view-mime-type");
const QLatin1String kCurrentVersionKey("current-version");
constexpr int kCollaboratorsUpdateTimeoutMs = 60 * 1000;

/**
 * @brief Является ли заданный элемент текстовым
 */
bool isTextItem(BusinessLayer::StructureModelItem* _item)
{
    static const QSet<Domain::DocumentObjectType> textItems = {
        Domain::DocumentObjectType::ScreenplayTitlePage,
        Domain::DocumentObjectType::ScreenplaySynopsis,
        Domain::DocumentObjectType::ScreenplayTreatment,
        Domain::DocumentObjectType::ScreenplayText,
        Domain::DocumentObjectType::ComicBookTitlePage,
        Domain::DocumentObjectType::ComicBookSynopsis,
        Domain::DocumentObjectType::ComicBookText,
        Domain::DocumentObjectType::AudioplayTitlePage,
        Domain::DocumentObjectType::AudioplaySynopsis,
        Domain::DocumentObjectType::AudioplayText,
        Domain::DocumentObjectType::StageplayTitlePage,
        Domain::DocumentObjectType::StageplaySynopsis,
        Domain::DocumentObjectType::StageplayText,
        Domain::DocumentObjectType::NovelTitlePage,
        Domain::DocumentObjectType::NovelSynopsis,
        Domain::DocumentObjectType::NovelOutline,
        Domain::DocumentObjectType::NovelText,
        Domain::DocumentObjectType::SimpleText,
    };
    return textItems.contains(_item->type());
}

/**
 * @brief Сформировать ключ настроек проекта
 */
QString projectSettingsKey(const QString& _key)
{
    return DatabaseLayer::Database::currentFile() + "/" + _key;
}

/**
 * @brief Сформировать ключ настроек для документа проекта
 */
QString documentSettingsKey(const QUuid& _documentUuid, const QString& _key)
{
    return projectSettingsKey(_documentUuid.toString() + "/" + _key);
}

/**
 * @brief Можно ли удалять документ
 */
bool canDocumentBeRemoved(BusinessLayer::StructureModelItem* _item)
{
    const QSet<Domain::DocumentObjectType> cantBeRemovedItems = {
        Domain::DocumentObjectType::Project,
        Domain::DocumentObjectType::Characters,
        Domain::DocumentObjectType::Locations,
        Domain::DocumentObjectType::Worlds,
        Domain::DocumentObjectType::ScreenplayTitlePage,
        Domain::DocumentObjectType::ScreenplaySynopsis,
        Domain::DocumentObjectType::ScreenplayTreatment,
        Domain::DocumentObjectType::ScreenplayText,
        Domain::DocumentObjectType::ScreenplayStatistics,
        Domain::DocumentObjectType::ComicBookTitlePage,
        Domain::DocumentObjectType::ComicBookSynopsis,
        Domain::DocumentObjectType::ComicBookText,
        Domain::DocumentObjectType::ComicBookStatistics,
        Domain::DocumentObjectType::AudioplayTitlePage,
        Domain::DocumentObjectType::AudioplaySynopsis,
        Domain::DocumentObjectType::AudioplayText,
        Domain::DocumentObjectType::AudioplayStatistics,
        Domain::DocumentObjectType::StageplayTitlePage,
        Domain::DocumentObjectType::StageplaySynopsis,
        Domain::DocumentObjectType::StageplayText,
        Domain::DocumentObjectType::StageplayStatistics,
        Domain::DocumentObjectType::NovelTitlePage,
        Domain::DocumentObjectType::NovelSynopsis,
        Domain::DocumentObjectType::NovelOutline,
        Domain::DocumentObjectType::NovelText,
        Domain::DocumentObjectType::NovelStatistics,
    };
    return !cantBeRemovedItems.contains(_item->type());
}

} // namespace

class ProjectManager::Implementation
{
public:
    explicit Implementation(ProjectManager* _q, QWidget* _parent,
                            const PluginsBuilder& _pluginsBuilder);

    /**
     * @brief Обновить текст пункта меню разделения экрана
     */
    void updateOptionsText();

    /**
     * @brief Переключить активное представление
     */
    void switchViews(bool _withActivation = true);

    /**
     * @brief Действия контекстного меню
     */
    enum class ContextMenuAction { AddDocument, RemoveDocument, EmptyRecycleBin };

    /**
     * @brief Обновить модель действий контекстного меню навигатора
     */
    void updateNavigatorContextMenu(const QModelIndex& _index);

    /**
     * @brief Установить заданный элемент текущим
     */
    void setCurrentIndex(const QModelIndex& _sourceModelIndex);
    void setCurrentItem(BusinessLayer::StructureModelItem* _item);

    /**
     * @brief Открыть текущий документ в отдельном окне
     */
    void openCurrentDocumentInNewWindow();

    /**
     * @brief Режим редакитрования заданного документа
     */
    DocumentEditingMode documentEditingMode(BusinessLayer::StructureModelItem* _documentItem) const;

    /**
     * @brief Добавить документ в проект
     */
    void addDocument(Domain::DocumentObjectType _type = Domain::DocumentObjectType::Undefined);

    /**
     * @brief Добавить документ в заданный контейнер
     */
    void addDocumentToContainer(Domain::DocumentObjectType _containerType,
                                Domain::DocumentObjectType _documentType,
                                const QString& _documentName, const QByteArray& _content = {});

    /**
     * @brief Задать видимость документа
     */
    void setDocumentVisible(BusinessLayer::AbstractModel* _model, Domain::DocumentObjectType _type,
                            bool _visible);

    /**
     * @brief Получить реальную модель, если в индексе элемент-алиас
     */
    BusinessLayer::StructureModelItem* aliasedItemForIndex(const QModelIndex& _index);

    /**
     * @brief Создать новую версию заданного документа
     */
    void createNewVersion(const QModelIndex& _itemIndex);

    /**
     * @brief Изменить версию
     */
    void editVersion(const QModelIndex& _itemIndex, int _versionIndex);

    /**
     * @brief Удалить версию
     */
    void removeVersion(const QModelIndex& _itemIndex, int _versionIndex);

    /**
     * @brief Удалить документы
     */
    void removeDocuments(const QList<QModelIndex>& _itemsIndexes);
    void removeDocumentImpl(BusinessLayer::StructureModelItem* _item);

    /**
     * @brief Найти всех персонажей
     */
    void findAllCharacters();

    /**
     * @brief Найти все локации
     */
    void findAllLocations();

    /**
     * @brief Очистить корзину
     */
    void emptyRecycleBin();

    /**
     * @brief Находится ли итем в корзине
     */
    bool isInsideRecycleBin(BusinessLayer::StructureModelItem* _item) const;

    /**
     * @brief Отсортировать по алфавиту дочерние элементы
     */
    void sortChildrenAlphabetically();

    /**
     * @brief Получить интерфейс активного редактора документа
     */
    Ui::IDocumentView* activeDocumentView() const;

    /**
     * @brief Получить интерфейс неактивного редактора документа
     */
    Ui::IDocumentView* inactiveDocumentView() const;

    /**
     * @brief Обновить режим редактирования представлений
     */
    void updateViewsEditingMode();

    //
    // Данные
    //

    ProjectManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;
    Ui::ProjectToolBar* toolBar = nullptr;
    Ui::ProjectNavigator* navigator = nullptr;

    /**
     * @brief Виджеты представления текущих редакторов
     */
    struct {
        Splitter* container = nullptr;

        Ui::ProjectView* left = nullptr;
        Ui::ProjectView* right = nullptr;

        //
        // Список представлений, открытых в отдельных окнах
        //
        struct Window {
            Ui::IDocumentView* view = nullptr;
            QModelIndex itemIndex = {};
        };
        QVector<Window> windows = {};

        //
        // Ссылки на активное представление и неактивное
        //
        Ui::ProjectView* active = nullptr;
        Ui::ProjectView* inactive = nullptr;

        //
        // Индексы элементов представлений в дереве
        //
        QModelIndex activeIndex = {};
        QModelIndex inactiveIndex = {};

        //
        // Состояние контейнера перед входом в полноэкранный режим
        //
        QByteArray stateBeforeFullscreen = {};

        //
        // Открытая модель
        //
        QPointer<BusinessLayer::AbstractModel> activeModel = {};
        QPointer<BusinessLayer::AbstractModel> inactiveModel = {};

        //
        // Майм-тип редактора открытой модели
        //
        QString activeViewMimeType = {};
        QString inactiveViewMimeType = {};
    } view;

    /**
     * @brief Панель со список соавторов
     */
    Ui::CollaboratorsToolBar* collaboratorsToolBar = nullptr;

    /**
     * @brief Действие разделения экрана напополам
     */
    QAction* splitScreenAction = nullptr;
    QShortcut* splitScreenShortcut = nullptr;

    /**
     * @brief Модели списка документов
     */
    BusinessLayer::StructureModel* projectStructureModel = nullptr;
    BusinessLayer::StructureProxyModel* projectStructureProxyModel = nullptr;

    /**
     * @brief Хранилище изображений проекта
     */
    DataStorageLayer::DocumentImageStorage documentImageStorage;

    /**
     * @brief Фасад доступа к моделям проекта
     */
    ProjectModelsFacade modelsFacade;

    /**
     * @brief Фабрика для создания плагинов-редакторов к моделям
     */
    const PluginsBuilder& pluginsBuilder;

    /**
     * @brief Путь текущего проекта
     */
    QString projectPath;

    /**
     * @brief Работает ли пользователь с облачным проектом
     */
    bool isProjectRemote = false;

    /**
     * @brief Является ли пользователь владельцем проекта
     */
    bool isProjectOwner = true;

    /**
     * @brief Находится ли проект в команде
     */
    bool isProjectInTeam = false;

    /**
     * @brief Может ли пользователь расшаривать доступ к документам проекта
     */
    bool allowGrantAccessToProject = true;

    /**
     * @brief Текущий режим редактирования документов
     */
    DocumentEditingMode editingMode = DocumentEditingMode::Edit;

    /**
     * @brief Список документов и разрешения на работу с ними
     */
    QHash<QUuid, DocumentEditingMode> editingPermissions;

    /**
     * @brief Список документов и таймеров для полной синхронизации
     */
    mutable QHash<QUuid, QSharedPointer<QTimer>> documentToSyncTimer;

    /**
     * @brief Списки изменений, которые были отправлены на синхронизацию
     */
    mutable QHash<QUuid, QVector<Domain::DocumentChangeObject*>> changesForSync;

    /**
     * @brief Список активных соавторов <уид устройства, курсор>
     */
    QHash<QString, Domain::CursorInfo> collaboratorsCursors;

    /**
     * @brief Дебаунсер для проверки активных соавторов
     */
    Debouncer collaboratorsUpdateDebouncer;

    /**
     * @brief Количество последовательно обновлённых документов
     */
    int mergedDocuments = 0;

    /**
     * @brief Возможность экспортирования для текущего документа
     */
    bool isCurrentDocumentExportAvailable = false;

    /**
     * @brief Последний добавленный элемент
     */
    QModelIndex lastAddedItemIndex;
};

ProjectManager::Implementation::Implementation(ProjectManager* _q, QWidget* _parent,
                                               const PluginsBuilder& _pluginsBuilder)
    : q(_q)
    , topLevelWidget(_parent)
    , toolBar(new Ui::ProjectToolBar(_parent))
    , navigator(new Ui::ProjectNavigator(_parent))
    , view({
          new Splitter(_parent),
          new Ui::ProjectView(_parent),
          new Ui::ProjectView(_parent),
      })
    , collaboratorsToolBar(new Ui::CollaboratorsToolBar(_parent))
    , splitScreenAction(new QAction(_parent))
    , splitScreenShortcut(new QShortcut(_parent))
    , projectStructureModel(new BusinessLayer::StructureModel(navigator))
    , projectStructureProxyModel(new BusinessLayer::StructureProxyModel(projectStructureModel))
    , modelsFacade(projectStructureModel, &documentImageStorage)
    , pluginsBuilder(_pluginsBuilder)
    , collaboratorsUpdateDebouncer(kCollaboratorsUpdateTimeoutMs)
{
    toolBar->hide();
    navigator->hide();
    view.left->installEventFilter(_q);
    view.right->hide();
    view.container->setWidgets(view.left, view.right);
    view.container->setSizes({ 1, 0 });
    view.container->hide();
    view.active = view.left;
    view.inactive = view.right;

    navigator->setModel(projectStructureProxyModel);

    splitScreenAction->setCheckable(true);
    splitScreenAction->setIconText(u8"\U000F10E7");
    splitScreenAction->setWhatsThis(QKeySequence("F2").toString(QKeySequence::NativeText));
    updateOptionsText();
    toolBar->setOptions({ splitScreenAction }, AppBarOptionsLevel::App);
    splitScreenShortcut->setKey(QKeySequence("F2"));
    splitScreenShortcut->setContext(Qt::ApplicationShortcut);
}

void ProjectManager::Implementation::updateOptionsText()
{
    splitScreenAction->setText(splitScreenAction->isChecked() ? tr("Remove split")
                                                              : tr("Split window"));
}

void ProjectManager::Implementation::switchViews(bool _withActivation)
{
    std::swap(view.active, view.inactive);
    if (_withActivation) {
        view.active->setActive(true);
        view.inactive->setActive(false);
    }

    std::swap(view.activeIndex, view.inactiveIndex);
    std::swap(view.activeModel, view.inactiveModel);
    std::swap(view.activeViewMimeType, view.inactiveViewMimeType);

    const auto activeIndex = projectStructureProxyModel->mapFromSource(view.activeIndex);

    if (_withActivation) {
        q->activateView(activeIndex, view.activeViewMimeType);
    }

    //
    // Прежде чем выбрать нужный элемент в навигаторе, заблокируем сигналы, чтобы не было ложных
    // срабатываний и повторной установки плагина редактора
    //
    QSignalBlocker signalBlocker(navigator);
    navigator->setCurrentIndex(activeIndex);
}

void ProjectManager::Implementation::updateNavigatorContextMenu(const QModelIndex& _index)
{
    QVector<QAction*> menuActions;

    const auto currentItemIndex = projectStructureProxyModel->mapToSource(_index);
    const auto currentItem = projectStructureModel->itemForIndex(currentItemIndex);
    const auto selectedIndexes = navigator->selectedIndexes();

    const auto enabled = editingMode == DocumentEditingMode::Edit;

    //
    // Определяем, где находятся элементы: все корзине/все не в корзине
    //
    bool allInsideRecycleBin = true;
    bool allOutsideRecycleBin = true;
    if (!_index.isValid()) {
        allInsideRecycleBin = false;
        allOutsideRecycleBin = false;
    } else {
        for (const auto index : selectedIndexes) {
            const auto currentItemIndex = projectStructureProxyModel->mapToSource(index);
            const auto currentItem = projectStructureModel->itemForIndex(currentItemIndex);
            if (!isInsideRecycleBin(currentItem)) {
                allInsideRecycleBin = false;
            } else {
                allOutsideRecycleBin = false;
            }
        }
    }

    //
    // Если все выделенные элементы находятся в корзине, то их можно только удалить навсегда
    //
    if (allInsideRecycleBin) {
        auto removeDocument = new QAction(tr("Remove permanently"));
        removeDocument->setIconText(u8"\U000f01b4");
        removeDocument->setEnabled(enabled);
        connect(removeDocument, &QAction::triggered, q,
                [this, selectedIndexes] { removeDocuments(selectedIndexes); });
        menuActions.append(removeDocument);
    } else if (allOutsideRecycleBin) {
        //
        // ... если один элемент не в корзине, то формируем список действий для конкретных элементов
        // структуры проекта
        //
        if (selectedIndexes.size() == 1 && _index.isValid()) {
            if (currentItem->type() == Domain::DocumentObjectType::Characters) {
                auto findAllCharacters = new QAction(tr("Find all characters"));
                findAllCharacters->setIconText(u8"\U000F0016");
                findAllCharacters->setEnabled(enabled);
                connect(findAllCharacters, &QAction::triggered, q,
                        [this] { this->findAllCharacters(); });
                menuActions.append(findAllCharacters);
                //
                auto addCharacter = new QAction(tr("Add character"));
                addCharacter->setIconText(u8"\U000F0014");
                connect(addCharacter, &QAction::triggered, q, [this] { addDocument(); });
                menuActions.append(addCharacter);
            } else if (currentItem->type() == Domain::DocumentObjectType::Locations) {
                auto findAllLocations = new QAction(tr("Find all locations"));
                findAllLocations->setIconText(u8"\U000F13B0");
                findAllLocations->setEnabled(enabled);
                connect(findAllLocations, &QAction::triggered, q,
                        [this] { this->findAllLocations(); });
                menuActions.append(findAllLocations);
                //
                auto addLocation = new QAction(tr("Add location"));
                addLocation->setIconText(u8"\U000F0975");
                connect(addLocation, &QAction::triggered, q, [this] { addDocument(); });
                menuActions.append(addLocation);
            } else if (currentItem->type() == Domain::DocumentObjectType::RecycleBin) {
                if (currentItem->hasChildren()) {
                    auto emptyRecycleBin = new QAction(tr("Empty recycle bin"));
                    emptyRecycleBin->setIconText(u8"\U000f05e8");
                    emptyRecycleBin->setEnabled(enabled);
                    connect(emptyRecycleBin, &QAction::triggered, q,
                            [this] { this->emptyRecycleBin(); });
                    menuActions.append(emptyRecycleBin);
                }
            }

            //
            // Персонажей и локации можно отсортировать по алфавиту, если выделен один элемент
            //
            if (currentItem->type() == Domain::DocumentObjectType::Characters
                || currentItem->type() == Domain::DocumentObjectType::Locations) {
                auto sortAlphabetically = new QAction(tr("Sort alphabetically"));
                sortAlphabetically->setIconText(u8"\U000F05BD");
                connect(sortAlphabetically, &QAction::triggered, q,
                        [this] { sortChildrenAlphabetically(); });
                menuActions.append(sortAlphabetically);
            }

            //
            // Для текстовых документов можно создать версию
            //
            if (isTextItem(currentItem)) {
                auto createNewVersion = new QAction(tr("Create new draft"));
                createNewVersion->setSeparator(!menuActions.isEmpty());
                createNewVersion->setIconText(u8"\U000F00FB");
                createNewVersion->setEnabled(enabled);
                connect(createNewVersion, &QAction::triggered, q,
                        [this, currentItemIndex] { this->createNewVersion(currentItemIndex); });
                menuActions.append(createNewVersion);
            }

            //
            // Каждый из элементов можно открыть в своём окне, кроме корзины
            //
            if (currentItem->type() != Domain::DocumentObjectType::RecycleBin) {
                auto openInNewWindow = new QAction(tr("Open in new window"));
                openInNewWindow->setSeparator(!menuActions.isEmpty());
                openInNewWindow->setIconText(u8"\U000F03CC");
                connect(openInNewWindow, &QAction::triggered, q,
                        [this] { openCurrentDocumentInNewWindow(); });
                menuActions.append(openInNewWindow);
            }

            //
            // Для документов, имеющих разрешение на экспорт, можно вызвать экспорт
            //
            if (isCurrentDocumentExportAvailable) {
                auto exportCurrentFileAction = new QAction(tr("Export..."));
                exportCurrentFileAction->setSeparator(true);
                exportCurrentFileAction->setIconText(u8"\U000f0207");
                connect(exportCurrentFileAction, &QAction::triggered, q,
                        &ProjectManager::exportCurrentDocumentRequested);
                menuActions.append(exportCurrentFileAction);
            }

            //
            // Документы облачного проекта можно расшарить
            //
            if (isProjectRemote && allowGrantAccessToProject
                && currentItem->type() != Domain::DocumentObjectType::RecycleBin) {
                QModelIndex documentIndexToShare;
                const auto documentUuidToShare = [currentItem, _index, &documentIndexToShare] {
                    switch (currentItem->type()) {
                    case Domain::DocumentObjectType::Character:
                    case Domain::DocumentObjectType::Location:
                    case Domain::DocumentObjectType::World:
                    case Domain::DocumentObjectType::AudioplayTitlePage:
                    case Domain::DocumentObjectType::AudioplaySynopsis:
                    case Domain::DocumentObjectType::AudioplayText:
                    case Domain::DocumentObjectType::AudioplayStatistics:
                    case Domain::DocumentObjectType::ComicBookTitlePage:
                    case Domain::DocumentObjectType::ComicBookSynopsis:
                    case Domain::DocumentObjectType::ComicBookText:
                    case Domain::DocumentObjectType::ComicBookStatistics:
                    case Domain::DocumentObjectType::NovelTitlePage:
                    case Domain::DocumentObjectType::NovelSynopsis:
                    case Domain::DocumentObjectType::NovelOutline:
                    case Domain::DocumentObjectType::NovelText:
                    case Domain::DocumentObjectType::NovelStatistics:
                    case Domain::DocumentObjectType::ScreenplayTitlePage:
                    case Domain::DocumentObjectType::ScreenplaySynopsis:
                    case Domain::DocumentObjectType::ScreenplayTreatment:
                    case Domain::DocumentObjectType::ScreenplayText:
                    case Domain::DocumentObjectType::ScreenplayStatistics:
                    case Domain::DocumentObjectType::StageplayTitlePage:
                    case Domain::DocumentObjectType::StageplaySynopsis:
                    case Domain::DocumentObjectType::StageplayText:
                    case Domain::DocumentObjectType::StageplayStatistics: {
                        documentIndexToShare = _index.parent();
                        return currentItem->parent()->uuid();
                    }
                    default: {
                        documentIndexToShare = _index;
                        return currentItem->uuid();
                    }
                    }
                }();
                //
                // ... если пользователь владелец, или может изменять этот документ
                //
                if ((isProjectOwner || editingMode == DocumentEditingMode::Edit
                     || editingPermissions.value(documentUuidToShare, DocumentEditingMode::Read)
                         == DocumentEditingMode::Edit)) {
                    auto shareAccess = new QAction(tr("Share access"));
                    shareAccess->setSeparator(!menuActions.isEmpty());
                    shareAccess->setIconText(u8"\U000F0010");
                    connect(shareAccess, &QAction::triggered, q, [this, documentIndexToShare] {
                        auto projectCollaboratorsView = pluginsBuilder.projectCollaboratorsView(
                            modelsFacade.modelFor(Domain::DocumentObjectType::Project));
                        view.left->addEditor(projectCollaboratorsView->asQWidget());

                        QMetaObject::invokeMethod(projectCollaboratorsView->asQWidget(),
                                                  "configureDocumentAccessPressed",
                                                  Q_ARG(QModelIndex, documentIndexToShare));
                    });
                    menuActions.append(shareAccess);
                }
            }
        }
        //
        // ... для одного или нескольких элементов не в корзине и кроме самой корзины
        //
        if (currentItem->type() != Domain::DocumentObjectType::RecycleBin) {
            auto addFolder = new QAction(tr("Add folder"));
            addFolder->setSeparator(!menuActions.isEmpty());
            addFolder->setIconText(u8"\U000F0257");
            addFolder->setEnabled(enabled);
            connect(addFolder, &QAction::triggered, q,
                    [this] { addDocument(Domain::DocumentObjectType::Folder); });
            menuActions.append(addFolder);
            //
            auto addDocument = new QAction(tr("Add document"));
            addDocument->setIconText(u8"\U000F0415");
            addDocument->setEnabled(enabled);
            connect(addDocument, &QAction::triggered, q, [this] { this->addDocument(); });
            menuActions.append(addDocument);

            //
            // Если хотя бы один выделенный элемент можно удалить, добавим действие
            //
            bool addRemoveAction = false;
            for (const auto index : selectedIndexes) {
                const auto currentItemIndex = projectStructureProxyModel->mapToSource(index);
                const auto currentItem = projectStructureModel->itemForIndex(currentItemIndex);
                if (canDocumentBeRemoved(currentItem)) {
                    addRemoveAction = true;
                    break;
                }
            }
            if (addRemoveAction) {
                auto removeDocument = new QAction(
                    selectedIndexes.size() > 1 ? tr("Remove documents") : tr("Remove document"));
                removeDocument->setSeparator(true);
                removeDocument->setIconText(u8"\U000f01b4");
                removeDocument->setEnabled(enabled);
                connect(removeDocument, &QAction::triggered, q,
                        [this, selectedIndexes] { removeDocuments(selectedIndexes); });
                menuActions.append(removeDocument);
            }
        }
    }

    //
    // При клике в любое место можно развернуть или свернуть все элементы
    //
    auto expandAll = new QAction(tr("Expand all"));
    expandAll->setSeparator(!menuActions.isEmpty());
    expandAll->setIconText(u8"\U000F004C");
    connect(expandAll, &QAction::triggered, navigator, &Ui::ProjectNavigator::expandAll);
    menuActions.append(expandAll);

    auto collapseAll = new QAction(tr("Collapse all"));
    collapseAll->setIconText(u8"\U000F0044");
    connect(collapseAll, &QAction::triggered, navigator, &Ui::ProjectNavigator::collapseAll);
    menuActions.append(collapseAll);

    navigator->setContextMenuActions(menuActions);
}

void ProjectManager::Implementation::setCurrentIndex(const QModelIndex& _sourceModelIndex)
{
    const auto mappedIndex = projectStructureProxyModel->mapFromSource(_sourceModelIndex);
    navigator->setCurrentIndex(mappedIndex);
}

void ProjectManager::Implementation::setCurrentItem(BusinessLayer::StructureModelItem* _item)
{
    const auto itemIndex = projectStructureModel->indexForItem(_item);
    setCurrentIndex(itemIndex);
}

void ProjectManager::Implementation::openCurrentDocumentInNewWindow()
{
    if (view.activeModel == nullptr) {
        return;
    }

    Log::info("Activate plugin \"%1\" in new window", view.activeViewMimeType);
    auto windowView = pluginsBuilder.activateWindowView(view.activeViewMimeType, view.activeModel);
    if (windowView == nullptr) {
        return;
    }

    if (auto windowWidget = windowView->asQWidget()) {
        windowWidget->resize(800, 600);
        windowWidget->show();
        //
        // TODO: Ещё сюда нужно писать версию открытого документа, если это не текущая версия
        //
        windowWidget->setWindowTitle(view.activeModel->documentName());

        view.windows.append({ windowView, view.activeIndex });

        const auto item = projectStructureModel->itemForIndex(view.activeIndex);
        windowView->setEditingMode(documentEditingMode(item));
    }
}

DocumentEditingMode ProjectManager::Implementation::documentEditingMode(
    BusinessLayer::StructureModelItem* _documentItem) const
{
    if (_documentItem->isReadOnly() || isInsideRecycleBin(_documentItem)) {
        return DocumentEditingMode::Read;
    }

    if (editingPermissions.isEmpty()) {
        return editingMode;
    }

    auto uuidToCheck = _documentItem->uuid();
    switch (_documentItem->type()) {
    case Domain::DocumentObjectType::ScreenplayTitlePage:
    case Domain::DocumentObjectType::ScreenplaySynopsis:
    case Domain::DocumentObjectType::ScreenplayTreatment:
    case Domain::DocumentObjectType::ScreenplayText:
    case Domain::DocumentObjectType::ScreenplayStatistics:
    case Domain::DocumentObjectType::ComicBookTitlePage:
    case Domain::DocumentObjectType::ComicBookSynopsis:
    case Domain::DocumentObjectType::ComicBookText:
    case Domain::DocumentObjectType::ComicBookStatistics:
    case Domain::DocumentObjectType::AudioplayTitlePage:
    case Domain::DocumentObjectType::AudioplaySynopsis:
    case Domain::DocumentObjectType::AudioplayText:
    case Domain::DocumentObjectType::AudioplayStatistics:
    case Domain::DocumentObjectType::StageplayTitlePage:
    case Domain::DocumentObjectType::StageplaySynopsis:
    case Domain::DocumentObjectType::StageplayText:
    case Domain::DocumentObjectType::StageplayStatistics:
    case Domain::DocumentObjectType::NovelTitlePage:
    case Domain::DocumentObjectType::NovelSynopsis:
    case Domain::DocumentObjectType::NovelOutline:
    case Domain::DocumentObjectType::NovelText:
    case Domain::DocumentObjectType::NovelStatistics:
    case Domain::DocumentObjectType::Character:
    case Domain::DocumentObjectType::Location:
    case Domain::DocumentObjectType::World: {
        uuidToCheck = _documentItem->parent()->uuid();
        break;
    }

    default: {
        break;
    }
    }
    return editingPermissions.value(uuidToCheck, DocumentEditingMode::Read);
}

void ProjectManager::Implementation::addDocument(Domain::DocumentObjectType _type)
{
    const auto currentItemIndex
        = projectStructureProxyModel->mapToSource(navigator->currentIndex());
    const auto currentItem = projectStructureModel->itemForIndex(currentItemIndex);

    auto dialog = new Ui::CreateDocumentDialog(topLevelWidget);
    if (currentItem->type() == Domain::DocumentObjectType::Folder) {
        dialog->setInsertionParent(currentItem->name());
    }
    if (_type != Domain::DocumentObjectType::Undefined) {
        dialog->setDocumentType(_type);
    } else if (currentItem->type() == Domain::DocumentObjectType::Characters) {
        dialog->setDocumentType(Domain::DocumentObjectType::Character);
    } else if (currentItem->type() == Domain::DocumentObjectType::Locations) {
        dialog->setDocumentType(Domain::DocumentObjectType::Location);
    } else if (currentItem->type() == Domain::DocumentObjectType::Worlds) {
        dialog->setDocumentType(Domain::DocumentObjectType::World);
    } else {
        dialog->setDocumentType(dialog->lastSelectedType());
    }

    connect(dialog, &Ui::CreateDocumentDialog::createPressed, navigator,
            [this, sourceType = _type, currentItemIndex, dialog](Domain::DocumentObjectType _type,
                                                                 const QString& _name,
                                                                 const QString& _importFilePath) {
                if (_type == Domain::DocumentObjectType::Character) {
                    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                    auto model = modelsFacade.modelFor(document);
                    auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(model);
                    Q_ASSERT(charactersModel);
                    if (charactersModel->exists(_name)) {
                        dialog->setNameError(tr("Character with this name already exists"));
                        return;
                    }
                } else if (_type == Domain::DocumentObjectType::Location) {
                    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                    auto model = modelsFacade.modelFor(document);
                    auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(model);
                    Q_ASSERT(locationsModel);
                    if (locationsModel->exists(_name)) {
                        dialog->setNameError(tr("Location with this name already exists"));
                        return;
                    }
                } else if (_type == Domain::DocumentObjectType::World) {
                    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Worlds);
                    auto model = modelsFacade.modelFor(document);
                    auto worldsModel = qobject_cast<BusinessLayer::WorldsModel*>(model);
                    Q_ASSERT(worldsModel);
                    if (worldsModel->exists(_name)) {
                        dialog->setNameError(tr("World with this name already exists"));
                        return;
                    }
                }

                //
                // Определим индекс родительского элемента, ищем именно папку
                //
                auto parentIndex
                    = dialog->needInsertIntoParent() ? currentItemIndex : currentItemIndex.parent();
                auto parentItem = projectStructureModel->itemForIndex(parentIndex);
                while (parentIndex.isValid() && parentItem != nullptr
                       && parentItem->type() != Domain::DocumentObjectType::Folder) {
                    parentIndex = parentIndex.parent();
                    parentItem = projectStructureModel->itemForIndex(parentIndex);
                }

                //
                // Определим индекс элемента для выделения
                //
                const auto addedItemIndex = projectStructureModel->addDocument(
                    _type, _name, parentIndex, {}, true, dialog->episodesAmount());
                QModelIndex itemForSelectIndex = addedItemIndex;
                //
                // ... в зависимости от типа, выбираем потенциально желаемый к редактированию
                // документ
                //     сейчас это просто выбор сценария для всех видов бандлов
                //
                constexpr int invalidIndex = -1;
                int childIndex = invalidIndex;
                switch (_type) {
                case Domain::DocumentObjectType::Screenplay:
                case Domain::DocumentObjectType::Novel: {
                    childIndex = 3;
                    break;
                }
                case Domain::DocumentObjectType::Audioplay:
                case Domain::DocumentObjectType::ComicBook:
                case Domain::DocumentObjectType::Stageplay: {
                    childIndex = 2;
                    break;
                }
                default: {
                    break;
                }
                }
                if (childIndex != invalidIndex) {
                    itemForSelectIndex
                        = projectStructureModel->index(childIndex, 0, itemForSelectIndex);
                }
                setCurrentIndex(itemForSelectIndex);

                //
                // Если исходный тип документа задан не был, то сохраним выбранный, чтобы
                // переиспользовать его при следующем вызове диалога добавления документа
                //
                if (sourceType == Domain::DocumentObjectType::Undefined) {
                    dialog->saveSelectedType();
                }

                //
                // Скрываем сам диалог
                //
                dialog->hideDialog();

                //
                // Если был задан файл, посылаем сигнал на импорт
                //
                if (!_importFilePath.isEmpty()) {
                    const auto item = projectStructureModel->itemForIndex(addedItemIndex);
                    emit q->importFileRequested(_importFilePath, item->uuid(), _type);
                }
            });
    connect(dialog, &Ui::CreateDocumentDialog::disappeared, dialog,
            &Ui::CreateDocumentDialog::deleteLater);

    dialog->showDialog();
}

void ProjectManager::Implementation::addDocumentToContainer(
    Domain::DocumentObjectType _containerType, Domain::DocumentObjectType _documentType,
    const QString& _documentName, const QByteArray& _content)
{
    for (int itemRow = 0; itemRow < projectStructureModel->rowCount(); ++itemRow) {
        const auto itemIndex = projectStructureModel->index(itemRow, 0);
        const auto item = projectStructureModel->itemForIndex(itemIndex);
        if (item->type() == _containerType) {
            //
            // Документ будет вставлен в контейнер, только если контейнер доступен для изменения
            //
            const bool isCanBeStored = isProjectOwner || editingMode == DocumentEditingMode::Edit
                || editingPermissions.value(item->uuid(), DocumentEditingMode::Read)
                    == DocumentEditingMode::Edit;
            if (isCanBeStored) {
                projectStructureModel->addDocument(_documentType, _documentName, itemIndex,
                                                   _content);
            }
            break;
        }
    }
}

void ProjectManager::Implementation::setDocumentVisible(BusinessLayer::AbstractModel* _model,
                                                        Domain::DocumentObjectType _type,
                                                        bool _visible)
{
    auto item = projectStructureModel->itemForUuid(_model->document()->uuid());

    //
    // Если нужно настроить видимость самого элемента
    //
    if (item->type() == _type) {
        projectStructureModel->setItemVisible(item, _visible);
        return;
    }

    //
    // Либо настраиваем видимость одного из вложенных элементов
    //
    for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
        auto childItem = item->childAt(childIndex);
        if (childItem->type() == _type) {
            projectStructureModel->setItemVisible(childItem, _visible);
            return;
        }
    }
}

BusinessLayer::StructureModelItem* ProjectManager::Implementation::aliasedItemForIndex(
    const QModelIndex& _index)
{
    auto item = projectStructureModel->itemForIndex(_index);
    if (item->type() != Domain::DocumentObjectType::ScreenplayTreatment
        && item->type() != Domain::DocumentObjectType::NovelOutline) {
        return item;
    }

    auto model = modelsFacade.modelFor(item->uuid());
    if (model == nullptr) {
        return item;
    }

    return projectStructureModel->itemForUuid(model->document()->uuid());
}

void ProjectManager::Implementation::createNewVersion(const QModelIndex& _itemIndex)
{
    auto dialog = new Ui::CreateDraftDialog(topLevelWidget);
    dialog->setVersions(
        [this, _itemIndex] {
            const auto item = aliasedItemForIndex(_itemIndex);
            QStringList versions = { tr("Current draft") };
            for (const auto version : item->versions()) {
                versions.append(version->name());
            }
            return versions;
        }(),
        view.active->currentDraft());
    connect(dialog, &Ui::CreateDraftDialog::savePressed, view.active,
            [this, _itemIndex, dialog](const QString& _name, const QColor& _color,
                                       int _versionIndex, bool _readOnly) {
                dialog->hideDialog();

                const auto item = aliasedItemForIndex(_itemIndex);
                const auto model = modelsFacade.modelFor(
                    _versionIndex == 0 ? item->uuid()
                                       : item->versions().at(_versionIndex - 1)->uuid());
                projectStructureModel->addItemVersion(item, _name, _color, _readOnly,
                                                      model->document()->content());
                view.active->setDocumentDraft(item->versions());
            });
    connect(dialog, &Ui::CreateDraftDialog::disappeared, dialog,
            &Ui::CreateDraftDialog::deleteLater);

    dialog->showDialog();
}

void ProjectManager::Implementation::editVersion(const QModelIndex& _itemIndex, int _versionIndex)
{
    auto dialog = new Ui::CreateDraftDialog(topLevelWidget);
    const auto item = aliasedItemForIndex(_itemIndex);
    const auto version = item->versions().at(_versionIndex);
    dialog->edit(version->name(), version->color(), version->isReadOnly());
    connect(dialog, &Ui::CreateDraftDialog::savePressed, view.active,
            [this, item, _versionIndex, dialog](const QString& _name, const QColor& _color,
                                                int _sourceVersionIndex, bool _readOnly) {
                Q_UNUSED(_sourceVersionIndex)

                dialog->hideDialog();

                projectStructureModel->updateItemVersion(item, _versionIndex, _name, _color,
                                                         _readOnly);
                view.active->setDocumentDraft(item->versions());

                //
                // Пеерзагрузим отображение, чтобы обновить флаг редактируемости версии
                //
                q->showViewForVersion(item->versions().at(_versionIndex));
            });
    connect(dialog, &Ui::CreateDraftDialog::disappeared, dialog,
            &Ui::CreateDraftDialog::deleteLater);

    dialog->showDialog();
}

void ProjectManager::Implementation::removeVersion(const QModelIndex& _itemIndex, int _versionIndex)
{
    auto dialog = new Dialog(view.active->topLevelWidget());
    const auto item = aliasedItemForIndex(_itemIndex);
    const auto version = item->versions().at(_versionIndex);
    constexpr int cancelButtonId = 0;
    constexpr int removeButtonId = 1;
    dialog->showDialog(
        {}, tr("Do you really want to remove document version \"%1\"?").arg(version->name()),
        { { cancelButtonId, tr("No"), Dialog::RejectButton },
          { removeButtonId, tr("Yes, remove"), Dialog::AcceptButton } });
    connect(
        dialog, &Dialog::finished, view.active,
        [this, item, _versionIndex, cancelButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал удалять
            //
            if (_buttonInfo.id == cancelButtonId) {
                return;
            }

            //
            // Если таки хочет, то сперва выберем другой документ для редактирования
            //
            if ((view.active->currentDraft() - 1) == _versionIndex) {
                //
                // ... если удалена последняя версия, то покажем текущую
                //
                if (_versionIndex == 0) {
                    q->showViewForVersion(item);
                }
                //
                // ... если удалена не последняя, то покажем предыдущую
                //
                else {
                    q->showViewForVersion(item->versions().at(_versionIndex - 1));
                }
                //
                // ... уменьшим на один индекс вкладки выбранной версии
                //
                view.active->setCurrentDraft(_versionIndex);
            }
            //
            // ... а потом собственно удалим заданную версию
            //
            const auto documentUuid = item->versions().at(_versionIndex)->uuid();
            auto document
                = DataStorageLayer::StorageFacade::documentStorage()->document(documentUuid);
            modelsFacade.removeModelFor(document);
            DataStorageLayer::StorageFacade::documentStorage()->removeDocument(document);
            projectStructureModel->removeItemVersion(item, _versionIndex);
            view.active->setDocumentDraft(item->versions());
            emit q->documentRemoved(documentUuid);
        });
    connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ProjectManager::Implementation::removeDocuments(const QList<QModelIndex>& _itemsIndexes)
{
    if (_itemsIndexes.isEmpty()) {
        return;
    }

    //
    // Собираем элементы для удаления
    //
    QList<BusinessLayer::StructureModelItem*> itemsList;
    for (const auto index : _itemsIndexes) {
        const auto itemIndex = projectStructureProxyModel->mapToSource(index);

        //
        // Если хотя бы один из родительских элементов уже содержится в списке,
        // то потомка добавлять не нужно, т.к. он и так будет удален
        //
        bool shouldToAdd = true;
        auto parentItemIndex = itemIndex.parent();
        while (parentItemIndex.isValid()) {
            if (itemsList.contains(projectStructureModel->itemForIndex(parentItemIndex))) {
                shouldToAdd = false;
                break;
            }
            parentItemIndex = parentItemIndex.parent();
        }
        if (const auto item = projectStructureModel->itemForIndex(itemIndex);
            shouldToAdd && canDocumentBeRemoved(item)) {
            itemsList.append(item);
        }
    }

    //
    // Если документы не все в корзине, то переносим их в корзину
    //
    bool allInsideRecycleBin = true;
    for (const auto item : itemsList) {
        if (!isInsideRecycleBin(item)) {
            projectStructureModel->moveItemToRecycleBin(item);
            allInsideRecycleBin = false;
        }
    }

    //
    // Из корзины удаляем только если все элементы уже были в ней
    //
    if (!allInsideRecycleBin) {
        return;
    }

    QString question;
    if (itemsList.size() > 1) {
        QString documentsList;
        for (const auto& item : itemsList) {
            documentsList += "\n" + item->name();
        }

        question
            = tr("Do you really want to permanently remove following documents?") + documentsList;
    } else {
        question = tr("Do you really want to permanently remove document: \"%1\"?")
                       .arg(itemsList.first()->name());
    }
    //
    // Спросим действительно ли пользователь хочет удалить документы
    //
    const int kCancelButtonId = 0;
    const int kRemoveButtonId = 1;
    auto dialog = new Dialog(topLevelWidget);
    dialog->showDialog({}, question,
                       { { kCancelButtonId, tr("No"), Dialog::RejectButton },
                         { kRemoveButtonId, tr("Yes, remove"), Dialog::NormalButton } });
    QObject::connect(
        dialog, &Dialog::finished,
        [this, itemsList, kCancelButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал удалять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Если таки хочет, то удаляем документы
            //
            for (const auto item : itemsList) {
                removeDocumentImpl(item);
            }
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ProjectManager::Implementation::removeDocumentImpl(BusinessLayer::StructureModelItem* _item)
{
    if (_item == nullptr) {
        return;
    }

    //
    // Если удаляемый документ открыт в одном из редакторов в данный момент, то закроем его
    //
    if (view.activeModel != nullptr && view.activeModel->document() != nullptr
        && view.activeModel->document()->uuid() == _item->uuid()) {
        view.active->showDefaultPage();
    }
    if (view.inactiveModel != nullptr && view.inactiveModel->document() != nullptr
        && view.inactiveModel->document()->uuid() == _item->uuid()) {
        view.inactive->showDefaultPage();
    }

    //
    // Кроме того, что нужно удалить сам документ, нужно ещё удалить и все вложенные документы
    // NOTE: порядок удаления важен
    //

    //
    // Сформируем модели детей, чтобы они не формировались на этапе удаления, т.к. часть
    // связанных между собой документов уже будет удалена
    //
    for (int index = 0; index < _item->childCount(); ++index) {
        modelsFacade.modelFor(_item->childAt(index)->uuid());
    }

    //
    // Сначала удаляем детей
    //
    for (int index = _item->childCount() - 1; index >= 0; --index) {
        auto child = _item->childAt(index);
        removeDocumentImpl(child);
    }

    //
    // Некоторые документы мы всё таки не удаляем, т.к. они основообразующие
    //
    if (_item->type() == Domain::DocumentObjectType::Project
        || _item->type() == Domain::DocumentObjectType::Characters
        || _item->type() == Domain::DocumentObjectType::Locations
        || _item->type() == Domain::DocumentObjectType::Worlds
        || _item->type() == Domain::DocumentObjectType::RecycleBin) {
        return;
    }

    //
    // Перед удалением сформируем дополнительный список документов, которые должны быть удалены
    //
    QVector<QUuid> documentsToRemove = { _item->uuid() };
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(_item->uuid());
    if (document != nullptr) {
        //
        // ... изображения
        //
        auto model = modelsFacade.modelFor(document);
        switch (document->type()) {
        case Domain::DocumentObjectType::Character: {
            auto character = static_cast<BusinessLayer::CharacterModel*>(model);
            for (const auto& photo : character->photos()) {
                documentsToRemove.append(photo.uuid);
            }
            break;
        }

        case Domain::DocumentObjectType::Location: {
            auto location = static_cast<BusinessLayer::LocationModel*>(model);
            for (const auto& photo : location->photos()) {
                documentsToRemove.append(photo.uuid);
            }
            break;
        }

        case Domain::DocumentObjectType::World: {
            auto world = static_cast<BusinessLayer::WorldModel*>(model);
            for (const auto& photo : world->photos()) {
                documentsToRemove.append(photo.uuid);
            }
            for (const auto& item : world->races()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->floras()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->animals()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->naturalResources()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->climates()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->religions()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->ethics()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->languages()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->castes()) {
                documentsToRemove.append(item.photo.uuid);
            }
            for (const auto& item : world->magicTypes()) {
                documentsToRemove.append(item.photo.uuid);
            }
            break;
        }

        default: {
            break;
        }
        }
        //
        // ... версии
        //
        for (const auto& version : _item->versions()) {
            documentsToRemove.append(version->uuid());
        }
        //
        // ... удалим все невалидные айдишники
        //
        documentsToRemove.removeAll({});
    }
    //
    // ... удаляем из структуры только в том случае, если не происходит изменение структуры во время
    //     синхронизации документа
    //
    if (!projectStructureModel->isChangesApplyingInProcess()) {
        projectStructureModel->removeItem(_item);
    }
    //
    // ... собственно удаляем модель и документ
    //
    for (const auto& documentUuid : std::as_const(documentsToRemove)) {
        document = DataStorageLayer::StorageFacade::documentStorage()->document(documentUuid);
        modelsFacade.removeModelFor(document);
        DataStorageLayer::StorageFacade::documentStorage()->removeDocument(document);
        //
        // ... уведомляем об удалённых документах
        //
        if (!projectStructureModel->isChangesApplyingInProcess()) {
            emit q->documentRemoved(documentUuid);
        }
        //
        // ... либо отписываемся от них, если находимся в процессе перестройки структуры
        //
        else {
            emit q->closeDocumentRequested(documentUuid);
        }
    }
}

void ProjectManager::Implementation::findAllCharacters()
{
    //
    // Найти все модели где могут встречаться персонажи и определить их
    //
    QSet<QString> charactersFromText;
    auto scriptModels = modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
    scriptModels.append(modelsFacade.modelsFor(Domain::DocumentObjectType::ComicBookText));
    scriptModels.append(modelsFacade.modelsFor(Domain::DocumentObjectType::AudioplayText));
    scriptModels.append(modelsFacade.modelsFor(Domain::DocumentObjectType::StageplayText));
    for (auto model : scriptModels) {
        auto audioplay = qobject_cast<BusinessLayer::ScriptTextModel*>(model);
        const auto characters = audioplay->findCharactersFromText();
        charactersFromText.unite({ characters.begin(), characters.end() });
    }
    charactersFromText.remove({});

    //
    // Определить персонажи, которых нет в тексте
    //
    QSet<QString> charactersNotFromText;
    const auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
        modelsFacade.modelFor(Domain::DocumentObjectType::Characters));
    for (int row = 0; row < charactersModel->rowCount(); ++row) {
        const auto characterName = charactersModel->index(row, 0).data().toString();
        if (!charactersFromText.contains(characterName)) {
            charactersNotFromText.insert(characterName);
        }
    }

    //
    // Спросить пользователя, что он хочет сделать с ними
    //
    QString message;
    if (!charactersFromText.isEmpty()) {
        QStringList characters = charactersFromText.values();
        std::sort(characters.begin(), characters.end());
        message.append(
            QString("%1:\n%2.").arg(tr("Characters from the text"), characters.join(", ")));
    }
    if (!charactersNotFromText.isEmpty()) {
        if (!message.isEmpty()) {
            message.append("\n\n");
        }
        QStringList characters = charactersNotFromText.values();
        std::sort(characters.begin(), characters.end());
        message.append(
            QString("%1:\n%2.")
                .arg(tr("Characters that are not found in the text"), characters.join(", ")));
    }
    const int kCancelButtonId = 0;
    const int kKeepFromTextButtonId = 1;
    const int kKeepAllButtonId = 2;
    auto dialog = new Dialog(topLevelWidget);
    //
    // Т.к. персов может быть очень много, расширяем максимальное ограничение по ширине для диалога
    //
    dialog->setContentFixedWidth(topLevelWidget->width() * 0.7);
    const auto placeButtonSideBySide = false;
    dialog->showDialog(
        {}, message,
        { { kKeepFromTextButtonId, tr("Save only characters from the text"), Dialog::NormalButton },
          { kKeepAllButtonId, tr("Save all characters"), Dialog::NormalButton },
          { kCancelButtonId, tr("Change nothing"), Dialog::RejectButton } },
        placeButtonSideBySide);
    QObject::connect(
        dialog, &Dialog::finished, dialog,
        [this, charactersFromText, charactersNotFromText, charactersModel, dialog, kCancelButtonId,
         kKeepFromTextButtonId](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал что-либо менять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Удаляем дубликаты
            // FIXME: выяснить откуда тут могут появляться дубликаты и искоренить проблему, сейчас
            //        тут исправляется только следствие, эта проблема может аффектить и другие места
            //
            {
                QSet<QString> allCharacters;
                QVector<int> duplicatesToRemove;
                for (int row = 0; row < charactersModel->rowCount(); ++row) {
                    const auto characterName = charactersModel->index(row, 0).data().toString();
                    if (allCharacters.contains(characterName)) {
                        duplicatesToRemove.append(row);
                    } else {
                        allCharacters.insert(characterName);
                    }
                }

                for (auto duplicateRow : reversed(duplicatesToRemove)) {
                    const auto characterModel = charactersModel->character(duplicateRow);
                    auto item
                        = projectStructureModel->itemForUuid(characterModel->document()->uuid());
                    removeDocumentImpl(item);
                }
            }

            //
            // Если надо, удалим персонажей, которые не встречаются в тексте
            //
            if (_buttonInfo.id == kKeepFromTextButtonId) {
                for (const auto& characterName : charactersNotFromText) {
                    const auto characterModel = charactersModel->character(characterName);
                    auto item
                        = projectStructureModel->itemForUuid(characterModel->document()->uuid());
                    removeDocumentImpl(item);
                }
            }

            //
            // Сохраняем новых персонажей, которых ещё не было в базе
            //
            for (const auto& characterName : charactersFromText) {
                if (charactersModel->exists(characterName)) {
                    continue;
                }

                addDocumentToContainer(Domain::DocumentObjectType::Characters,
                                       Domain::DocumentObjectType::Character, characterName);
            }
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ProjectManager::Implementation::findAllLocations()
{
    //
    // Найти все модели где могут встречаться локации и определить их
    //
    QSet<QString> locationsFromText;
    auto scriptModels = modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
    scriptModels.append(modelsFacade.modelsFor(Domain::DocumentObjectType::ComicBookText));
    scriptModels.append(modelsFacade.modelsFor(Domain::DocumentObjectType::AudioplayText));
    scriptModels.append(modelsFacade.modelsFor(Domain::DocumentObjectType::StageplayText));
    for (auto model : scriptModels) {
        auto audioplay = qobject_cast<BusinessLayer::ScriptTextModel*>(model);
        const auto characters = audioplay->findLocationsFromText();
        locationsFromText.unite({ characters.begin(), characters.end() });
    }
    locationsFromText.remove({});

    //
    // Определить локации, которых нет в тексте
    //
    QSet<QString> locationsNotFromText;
    const auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(
        modelsFacade.modelFor(Domain::DocumentObjectType::Locations));
    for (int row = 0; row < locationsModel->rowCount(); ++row) {
        const auto locationName = locationsModel->index(row, 0).data().toString();
        if (!locationsFromText.contains(locationName)) {
            locationsNotFromText.insert(locationName);
        }
    }

    //
    // Спросить пользователя, что он хочет сделать с ними
    //
    QString message;
    if (!locationsFromText.isEmpty()) {
        QStringList locations = locationsFromText.values();
        std::sort(locations.begin(), locations.end());
        message.append(
            QString("%1:\n%2.").arg(tr("Locations from the text"), locations.join(", ")));
    }
    if (!locationsNotFromText.isEmpty()) {
        if (!message.isEmpty()) {
            message.append("\n\n");
        }
        QStringList locations = locationsNotFromText.values();
        std::sort(locations.begin(), locations.end());
        message.append(
            QString("%1:\n%2.")
                .arg(tr("Locations that are not found in the text"), locations.join(", ")));
    }
    const int kCancelButtonId = 0;
    const int kKeepFromTextButtonId = 1;
    const int kKeepAllButtonId = 2;
    auto dialog = new Dialog(topLevelWidget);
    //
    // Т.к. локаций может быть очень много, расширяем максимальное ограничение по ширине для диалога
    //
    dialog->setContentFixedWidth(topLevelWidget->width() * 0.7);
    const auto placeButtonSideBySide = false;
    dialog->showDialog(
        {}, message,
        { { kKeepFromTextButtonId, tr("Save only locations from the text"), Dialog::NormalButton },
          { kKeepAllButtonId, tr("Save all locations"), Dialog::NormalButton },
          { kCancelButtonId, tr("Change nothing"), Dialog::RejectButton } },
        placeButtonSideBySide);
    QObject::connect(
        dialog, &Dialog::finished, dialog,
        [this, locationsFromText, locationsNotFromText, locationsModel, dialog, kCancelButtonId,
         kKeepFromTextButtonId](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал что-либо менять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Удаляем дубликаты
            // FIXME: выяснить откуда тут могут появляться дубликаты и искоренить проблему, сейчас
            //        тут исправляется только следствие, эта проблема может аффектить и другие места
            //
            {
                QSet<QString> allCharacters;
                QVector<int> duplicatesToRemove;
                for (int row = 0; row < locationsModel->rowCount(); ++row) {
                    const auto locationName = locationsModel->index(row, 0).data().toString();
                    if (allCharacters.contains(locationName)) {
                        duplicatesToRemove.append(row);
                    } else {
                        allCharacters.insert(locationName);
                    }
                }

                for (auto duplicateRow : reversed(duplicatesToRemove)) {
                    const auto locationModel = locationsModel->location(duplicateRow);
                    auto item
                        = projectStructureModel->itemForUuid(locationModel->document()->uuid());
                    removeDocumentImpl(item);
                }
            }

            //
            // Если надо, удалим локации, которые не встречаются в тексте
            //
            if (_buttonInfo.id == kKeepFromTextButtonId) {
                for (const auto& locationName : locationsNotFromText) {
                    const auto locationModel = locationsModel->location(locationName);
                    auto item
                        = projectStructureModel->itemForUuid(locationModel->document()->uuid());
                    removeDocumentImpl(item);
                }
            }

            //
            // Сохраняем новых локации, которых ещё не было в базе
            //
            for (const auto& locationName : locationsFromText) {
                if (locationsModel->exists(locationName)) {
                    continue;
                }

                addDocumentToContainer(Domain::DocumentObjectType::Locations,
                                       Domain::DocumentObjectType::Location, locationName);
            }
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ProjectManager::Implementation::emptyRecycleBin()
{
    auto recycleBin = projectStructureModel->itemForType(Domain::DocumentObjectType::RecycleBin);
    if (recycleBin == nullptr) {
        return;
    }

    //
    // Спросим действительно ли пользователь хочет очистить корзину
    //
    const int kCancelButtonId = 0;
    const int kEmptyButtonId = 1;
    auto dialog = new Dialog(topLevelWidget);
    dialog->showDialog(
        {}, tr("Do you really want to permanently remove all documents from the recycle bin?"),
        { { kCancelButtonId, tr("No"), Dialog::RejectButton },
          { kEmptyButtonId, tr("Yes, remove"), Dialog::NormalButton } });
    QObject::connect(
        dialog, &Dialog::finished,
        [this, recycleBin, kCancelButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь передумал очищать корзину
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Собственно очищаем корзину
            //
            while (recycleBin->hasChildren()) {
                auto itemToRemove = recycleBin->childAt(0);
                removeDocumentImpl(itemToRemove);
            }

            //
            // Дизейблим кнопку очистки корзины в навигаторе
            //
            navigator->setButtonEnabled(false);
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

bool ProjectManager::Implementation::isInsideRecycleBin(
    BusinessLayer::StructureModelItem* _item) const
{
    if (_item == nullptr) {
        return false;
    }

    for (auto parent = _item->parent(); parent != nullptr; parent = parent->parent()) {
        if (parent->type() == Domain::DocumentObjectType::RecycleBin) {
            return true;
        }
    }
    return false;
}

void ProjectManager::Implementation::sortChildrenAlphabetically()
{
    const auto currentItemIndex
        = projectStructureProxyModel->mapToSource(navigator->currentIndex());
    const auto currentItem = projectStructureModel->itemForIndex(currentItemIndex);
    const std::function<bool(BusinessLayer::AbstractModelItem*, BusinessLayer::AbstractModelItem*)>
        sorter = [](BusinessLayer::AbstractModelItem* _lhs,
                    BusinessLayer::AbstractModelItem* _rhs) {
            return _lhs->data(Qt::DisplayRole).toString() < _rhs->data(Qt::DisplayRole).toString();
        };
    currentItem->sortChildren(sorter);
    projectStructureModel->saveChanges();
}

Ui::IDocumentView* ProjectManager::Implementation::activeDocumentView() const
{
    Ui::IDocumentView* documentView = nullptr;
    auto plugin = pluginsBuilder.plugin(view.activeViewMimeType);
    if (plugin != nullptr) {
        if (view.active == view.left) {
            documentView = plugin->view(view.activeModel);
        } else {
            documentView = plugin->secondaryView(view.activeModel);
        }
    }
    return documentView;
}

Ui::IDocumentView* ProjectManager::Implementation::inactiveDocumentView() const
{
    Ui::IDocumentView* documentView = nullptr;
    auto plugin = pluginsBuilder.plugin(view.inactiveViewMimeType);
    if (plugin != nullptr) {
        if (view.inactive == view.left) {
            documentView = plugin->view(view.inactiveModel);
        } else {
            documentView = plugin->secondaryView(view.inactiveModel);
        }
    }
    return documentView;
}

void ProjectManager::Implementation::updateViewsEditingMode()
{
    if (auto activeView = activeDocumentView(); activeView != nullptr) {
        const auto item = projectStructureModel->itemForIndex(view.activeIndex);
        activeView->setEditingMode(documentEditingMode(item));
    }
    if (auto inactiveView = inactiveDocumentView(); inactiveView != nullptr) {
        const auto item = projectStructureModel->itemForIndex(view.inactiveIndex);
        inactiveView->setEditingMode(documentEditingMode(item));
    }

    for (auto window : view.windows) {
        const auto item = projectStructureModel->itemForIndex(window.itemIndex);
        window.view->setEditingMode(documentEditingMode(item));
    }
}


// ****


ProjectManager::ProjectManager(QObject* _parent, QWidget* _parentWidget,
                               const PluginsBuilder& _pluginsBuilder)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget, _pluginsBuilder))
{
    connect(d->toolBar, &Ui::ProjectToolBar::menuPressed, this, &ProjectManager::menuRequested);
    connect(d->toolBar, &Ui::ProjectToolBar::viewPressed, this, [this](const QString& _mimeType) {
        showView(d->navigator->currentIndex(), _mimeType);
    });

    //
    // Отображаем необходимый редактор при выборе документа в списке
    //
    connect(d->navigator, &Ui::ProjectNavigator::itemSelected, this,
            [this](const QModelIndex& _index) {
                showView(_index);

                //
                // Для корзины и вложенных элементов вместо кнопки добавления документов показываем
                // кнопку очистки корзины
                //
                bool isInRecycleBin = false;
                bool isRecycleBinHasDocuments = false;
                const auto mappedIndex = d->projectStructureProxyModel->mapToSource(_index);
                auto topLevelParent = d->projectStructureModel->itemForIndex(mappedIndex);
                while (topLevelParent != nullptr) {
                    if (topLevelParent->type() == Domain::DocumentObjectType::RecycleBin) {
                        isInRecycleBin = true;
                        isRecycleBinHasDocuments = topLevelParent->hasChildren();
                        break;
                    }
                    topLevelParent = topLevelParent->parent();
                }
                d->navigator->showButton(isInRecycleBin
                                             ? Ui::ProjectNavigator::ActionButton::EmptyRecycleBin
                                             : Ui::ProjectNavigator::ActionButton::AddDocument);
                d->navigator->setButtonEnabled(isInRecycleBin ? isRecycleBinHasDocuments : true);
            });
    //
    // Отображаем навигатор выбранного элемента
    //
    connect(d->navigator, &Ui::ProjectNavigator::itemDoubleClicked, this,
            [this](const QModelIndex& _index) {
                const auto mappedIndex = d->projectStructureProxyModel->mapToSource(_index);
                if (!d->projectStructureModel
                         ->data(mappedIndex,
                                BusinessLayer::StructureModelDataRole::IsNavigatorAvailable)
                         .toBool()) {
                    return;
                }

                showNavigator(_index);
            });
    connect(d->navigator, &Ui::ProjectNavigator::itemNavigationRequested, this,
            [this](const QModelIndex& _index) { showNavigator(_index); });
    connect(d->navigator, &Ui::ProjectNavigator::contextMenuUpdateRequested, this,
            [this](const QModelIndex& _index) { d->updateNavigatorContextMenu(_index); });
    connect(d->navigator, &Ui::ProjectNavigator::addDocumentClicked, this,
            [this] { d->addDocument(); });
    connect(d->navigator, &Ui::ProjectNavigator::emptyRecycleBinClicked, this,
            [this] { d->emptyRecycleBin(); });

    //
    // Разделение экрана на две панели
    //
    connect(d->splitScreenAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsText();
        if (_checked) {
            Log::info("Split screen turned on");

            const bool isFirstSplit = !d->view.inactiveIndex.isValid();
            if (isFirstSplit) {
                d->view.inactiveIndex = d->view.activeIndex;
            }

            d->view.container->setSizes({ 1, 1 });
            d->view.right->show();
            d->switchViews();

            if (isFirstSplit) {
                showView(d->projectStructureProxyModel->mapFromSource(d->view.inactiveIndex),
                         d->view.activeViewMimeType);
            }
        } else {
            Log::info("Split screen turned off");

            d->view.right->hide();
            d->view.container->setSizes({ 1, 0 });
            if (d->view.active == d->view.right) {
                d->switchViews();
            }
        }
    });
    connect(d->splitScreenShortcut, &QShortcut::activated, this, [this] {
        if (!d->view.container->isVisible()) {
            return;
        }

        d->splitScreenAction->toggle();
    });

    //
    // Соединения с моделью структуры проекта
    //
    connect(
        d->projectStructureModel, &BusinessLayer::StructureModel::documentAdded, this,
        [this](const QUuid& _uuid, const QUuid& _parentUuid, Domain::DocumentObjectType _type,
               const QString& _name, const QByteArray& _content) {
            Q_UNUSED(_parentUuid);

            //
            // Если в данный момент накладываются изменения (из облака), то не создаём новые
            // документы и модели для них, они будут получены, когда пользователь выберет
            // конкретный документ в структуре проекта
            //
            if (d->projectStructureModel->isChangesApplyingInProcess()) {
                return;
            }

            //
            // Если же измнеения не накладываются, значит мы в состоянии, когда пользователь
            // локально создаёт новые документы и для них нужно создать соответствующие модели
            //
            auto document
                = DataStorageLayer::StorageFacade::documentStorage()->createDocument(_uuid, _type);
            if (!_content.isNull()) {
                document->setContent(_content);
            }

            auto documentModel = d->modelsFacade.modelFor(document);
            documentModel->setDocumentName(_name);
            if (!_content.isNull()) {
                documentModel->reassignContent();
            }

            switch (_type) {
            case Domain::DocumentObjectType::Character: {
                auto charactersDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                    d->modelsFacade.modelFor(charactersDocument));
                auto characterModel = qobject_cast<BusinessLayer::CharacterModel*>(documentModel);
                charactersModel->addCharacterModel(characterModel);

                break;
            }

            case Domain::DocumentObjectType::Location: {
                auto locationsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(
                    d->modelsFacade.modelFor(locationsDocument));
                auto locationModel = qobject_cast<BusinessLayer::LocationModel*>(documentModel);
                locationsModel->addLocationModel(locationModel);

                break;
            }

            case Domain::DocumentObjectType::World: {
                auto worldsDocument = DataStorageLayer::StorageFacade::documentStorage()->document(
                    Domain::DocumentObjectType::Worlds);
                auto locationsModel = qobject_cast<BusinessLayer::WorldsModel*>(
                    d->modelsFacade.modelFor(worldsDocument));
                auto locationModel = qobject_cast<BusinessLayer::WorldModel*>(documentModel);
                locationsModel->addWorldModel(locationModel);

                break;
            }

            default:
                break;
            }

            documentModel->reassignContent();
            emit uploadDocumentRequested(_uuid, true);
        });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::documentAboutToBeRemoved,
            this, [this](const QUuid& _uuid) {
                //
                // Удаляем документы только в случае, если накладываются изменения из облака, т.к. в
                // этой ситуации мы не знаем наверняка, удалён ли документ или перемещён, а в случае
                // с локальными изменениями - это точно перемещение
                //
                if (!d->projectStructureModel->isChangesApplyingInProcess()) {
                    return;
                }

                const auto item = d->projectStructureModel->itemForUuid(_uuid);
                d->removeDocumentImpl(item);
            });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::contentsChanged, this,
            [this](const QByteArray& _undo, const QByteArray& _redo) {
                handleModelChange(d->projectStructureModel, _undo, _redo);
            });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::dataChanged, this,
            [this](const QModelIndex& _topLeft, const QModelIndex& _bottomRight) {
                if (_topLeft != _bottomRight) {
                    return;
                }

                const auto sourceIndex
                    = d->projectStructureProxyModel->mapToSource(d->navigator->currentIndex());
                if (sourceIndex != _topLeft) {
                    return;
                }

                auto item = d->aliasedItemForIndex(sourceIndex);
                d->view.active->setDocumentDraft(item->versions());
            });
    connect(
        d->projectStructureModel, &BusinessLayer::StructureModel::rowsInserted, this,
        [this](const QModelIndex& _parent, int _first, int _last) {
            const auto insertedInItem = d->projectStructureModel->itemForIndex(_parent);
            if (insertedInItem == nullptr) {
                return;
            }

            //
            // Восстанавливаем персонажей
            //
            if (insertedInItem->type() == Domain::DocumentObjectType::Characters) {
                const auto charactersDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                auto charactersModel = d->modelsFacade.modelFor(charactersDocument);
                auto characters = qobject_cast<BusinessLayer::CharactersModel*>(charactersModel);
                for (int row = _first; row <= _last; ++row) {
                    const auto removedItemIndex = d->projectStructureModel->index(row, 0, _parent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    characters->addCharacterModel(qobject_cast<BusinessLayer::CharacterModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Восстанавливаем локации
            //
            else if (insertedInItem->type() == Domain::DocumentObjectType::Locations) {
                const auto locationsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                auto locationsModel = d->modelsFacade.modelFor(locationsDocument);
                auto locations = qobject_cast<BusinessLayer::LocationsModel*>(locationsModel);
                for (int row = _first; row <= _last; ++row) {
                    const auto removedItemIndex = d->projectStructureModel->index(row, 0, _parent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    locations->addLocationModel(qobject_cast<BusinessLayer::LocationModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Восстанавливаем миры
            //
            else if (insertedInItem->type() == Domain::DocumentObjectType::Worlds) {
                const auto worldsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Worlds);
                auto worldsModel = d->modelsFacade.modelFor(worldsDocument);
                auto worlds = qobject_cast<BusinessLayer::WorldsModel*>(worldsModel);
                for (int row = _first; row <= _last; ++row) {
                    const auto removedItemIndex = d->projectStructureModel->index(row, 0, _parent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    worlds->addWorldModel(qobject_cast<BusinessLayer::WorldModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
        });
    connect(
        d->projectStructureModel, &BusinessLayer::StructureModel::rowsAboutToBeRemoved, this,
        [this](const QModelIndex& _parent, int _first, int _last) {
            const auto removedFromItem = d->projectStructureModel->itemForIndex(_parent);
            if (removedFromItem == nullptr) {
                return;
            }

            //
            // Удаляем персонажа
            //
            if (removedFromItem->type() == Domain::DocumentObjectType::Characters) {
                const auto charactersDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                auto charactersModel = d->modelsFacade.modelFor(charactersDocument);
                auto characters = qobject_cast<BusinessLayer::CharactersModel*>(charactersModel);
                for (int row = _first; row <= _last; ++row) {
                    const auto removedItemIndex = d->projectStructureModel->index(row, 0, _parent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    auto chModel = d->modelsFacade.modelFor(removedItem->uuid());
                    characters->removeCharacterModel(
                        qobject_cast<BusinessLayer::CharacterModel*>(chModel));
                }
            }
            //
            // Удаляем локации
            //
            else if (removedFromItem->type() == Domain::DocumentObjectType::Locations) {
                const auto locationsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                auto locationsModel = d->modelsFacade.modelFor(locationsDocument);
                auto locations = qobject_cast<BusinessLayer::LocationsModel*>(locationsModel);
                for (int row = _first; row <= _last; ++row) {
                    const auto removedItemIndex = d->projectStructureModel->index(row, 0, _parent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    locations->removeLocationModel(qobject_cast<BusinessLayer::LocationModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Удаляем миры
            //
            else if (removedFromItem->type() == Domain::DocumentObjectType::Worlds) {
                const auto worldsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Worlds);
                auto worldsModel = d->modelsFacade.modelFor(worldsDocument);
                auto worlds = qobject_cast<BusinessLayer::WorldsModel*>(worldsModel);
                for (int row = _first; row <= _last; ++row) {
                    const auto removedItemIndex = d->projectStructureModel->index(row, 0, _parent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    worlds->removeWorldModel(qobject_cast<BusinessLayer::WorldModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
        });

    connect(d->projectStructureModel, &BusinessLayer::StructureModel::rowsMoved, this,
            [this](const QModelIndex& _sourceParent, int _sourceStart, int _sourceEnd,
                   const QModelIndex& _destinationParent) {
                const auto sourceParentItem = d->projectStructureModel->itemForIndex(_sourceParent);
                const auto destinationItem
                    = d->projectStructureModel->itemForIndex(_destinationParent);
                if (sourceParentItem == nullptr || destinationItem == nullptr) {
                    return;
                }
                //
                // Обновляем режим редактирования представлений при перемещении в корзину или из неё
                //
                if (destinationItem->type() == Domain::DocumentObjectType::RecycleBin
                    || sourceParentItem->type() == Domain::DocumentObjectType::RecycleBin) {
                    d->updateViewsEditingMode();
                }
            });

    connect(
        d->projectStructureModel, &BusinessLayer::StructureModel::rowsAboutToBeMoved, this,
        [this](const QModelIndex& _sourceParent, int _sourceStart, int _sourceEnd,
               const QModelIndex& _destinationParent, int _destination) {
            const auto sourceParentItem = d->projectStructureModel->itemForIndex(_sourceParent);
            const auto destinationItem = d->projectStructureModel->itemForIndex(_destinationParent);
            if (sourceParentItem == nullptr || destinationItem == nullptr) {
                return;
            }

            //
            // Удаляем персонажа
            //
            if (sourceParentItem->type() == Domain::DocumentObjectType::Characters
                && destinationItem->type() == Domain::DocumentObjectType::RecycleBin) {
                const auto charactersDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                auto charactersModel = d->modelsFacade.modelFor(charactersDocument);
                auto characters = qobject_cast<BusinessLayer::CharactersModel*>(charactersModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    characters->removeCharacterModel(qobject_cast<BusinessLayer::CharacterModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Удаляем локации
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::Locations
                     && destinationItem->type() == Domain::DocumentObjectType::RecycleBin) {
                const auto locationsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                auto locationsModel = d->modelsFacade.modelFor(locationsDocument);
                auto locations = qobject_cast<BusinessLayer::LocationsModel*>(locationsModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    locations->removeLocationModel(qobject_cast<BusinessLayer::LocationModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Удаляем мир
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::Worlds
                     && destinationItem->type() == Domain::DocumentObjectType::RecycleBin) {
                const auto worldsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Worlds);
                auto worldsModel = d->modelsFacade.modelFor(worldsDocument);
                auto worlds = qobject_cast<BusinessLayer::WorldsModel*>(worldsModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    worlds->removeWorldModel(qobject_cast<BusinessLayer::WorldModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Восстанавливаем персонажей
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::RecycleBin
                     && destinationItem->type() == Domain::DocumentObjectType::Characters) {
                const auto charactersDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Characters);
                auto charactersModel = d->modelsFacade.modelFor(charactersDocument);
                auto characters = qobject_cast<BusinessLayer::CharactersModel*>(charactersModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    characters->addCharacterModel(qobject_cast<BusinessLayer::CharacterModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Восстанавливаем локации
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::RecycleBin
                     && destinationItem->type() == Domain::DocumentObjectType::Locations) {
                const auto locationsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Locations);
                auto locationsModel = d->modelsFacade.modelFor(locationsDocument);
                auto locations = qobject_cast<BusinessLayer::LocationsModel*>(locationsModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    locations->addLocationModel(qobject_cast<BusinessLayer::LocationModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Восстанавливаем миры
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::RecycleBin
                     && destinationItem->type() == Domain::DocumentObjectType::Worlds) {
                const auto worldsDocument
                    = DataStorageLayer::StorageFacade::documentStorage()->document(
                        Domain::DocumentObjectType::Worlds);
                auto worldsModel = d->modelsFacade.modelFor(worldsDocument);
                auto worlds = qobject_cast<BusinessLayer::WorldsModel*>(worldsModel);
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto removedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto removedItem
                        = d->projectStructureModel->itemForIndex(removedItemIndex);
                    worlds->addWorldModel(qobject_cast<BusinessLayer::WorldModel*>(
                        d->modelsFacade.modelFor(removedItem->uuid())));
                }
            }
            //
            // Перемещаем персонажей
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::Characters
                     && destinationItem->type() == Domain::DocumentObjectType::Characters) {
                auto characters = qobject_cast<BusinessLayer::CharactersModel*>(
                    d->modelsFacade.modelFor(sourceParentItem->type()));
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto movedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto movedItem = d->projectStructureModel->itemForIndex(movedItemIndex);
                    const auto character = qobject_cast<BusinessLayer::CharacterModel*>(
                        d->modelsFacade.modelFor(movedItem->uuid()));
                    characters->moveCharacter(character->name(),
                                              _destination + (row - _sourceStart));
                }
            }
            //
            // Перемещаем локации
            //
            else if (sourceParentItem->type() == Domain::DocumentObjectType::Locations
                     && destinationItem->type() == Domain::DocumentObjectType::Locations) {
                auto locations = qobject_cast<BusinessLayer::LocationsModel*>(
                    d->modelsFacade.modelFor(sourceParentItem->type()));
                for (int row = _sourceStart; row <= _sourceEnd; ++row) {
                    const auto movedItemIndex
                        = d->projectStructureModel->index(row, 0, _sourceParent);
                    const auto movedItem = d->projectStructureModel->itemForIndex(movedItemIndex);
                    const auto location = qobject_cast<BusinessLayer::LocationModel*>(
                        d->modelsFacade.modelFor(movedItem->uuid()));
                    locations->moveLocation(location->name(), _destination + (row - _sourceStart));
                }
            }
        });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::versionAdded, this,
            [this] { d->view.active->setDraftsVisible(true); });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::versionRemoved, this,
            [this](const QUuid& _uuid) {
                const auto versionsCount
                    = d->projectStructureModel->itemForUuid(_uuid)->versions().count();
                d->view.active->setDraftsVisible(versionsCount > 0);
            });

    //
    // Соединения представления
    //
    for (auto view : { d->view.left, d->view.right }) {
        connect(view, &Ui::ProjectView::createNewItemPressed, this, [this] { d->addDocument(); });
        connect(view, &Ui::ProjectView::showDraftPressed, this, [this](int _versionIndex) {
            const auto currentItemIndex
                = d->projectStructureProxyModel->mapToSource(d->navigator->currentIndex());
            const auto currentItem = d->aliasedItemForIndex(currentItemIndex);

            //
            // Показать текущую версию
            //
            if (_versionIndex == 0) {
                showViewForVersion(currentItem);
                return;
            }

            //
            // Показать одну из установленных версий
            //
            showViewForVersion(currentItem->versions().at(_versionIndex - 1));
        });
        connect(view, &Ui::ProjectView::showDraftContextMenuPressed, this,
                [this](int _versionIndex) {
                    const auto currentItemIndex
                        = d->projectStructureProxyModel->mapToSource(d->navigator->currentIndex());
                    const auto item = d->projectStructureModel->itemForIndex(currentItemIndex);

                    const auto enabled = d->documentEditingMode(item) == DocumentEditingMode::Edit;

                    QVector<QAction*> menuActions;
                    auto createNewVersionAction = new QAction;
                    createNewVersionAction->setIconText(u8"\U000F00FB");
                    createNewVersionAction->setText(tr("Create new draft"));
                    createNewVersionAction->setEnabled(enabled);
                    connect(createNewVersionAction, &QAction::triggered, this,
                            [this, currentItemIndex] { d->createNewVersion(currentItemIndex); });
                    menuActions.append(createNewVersionAction);

                    //
                    // Для любой версии, кроме текущей показываем опции редактирования
                    //
                    if (_versionIndex > 0) {
                        --_versionIndex;
                        //
                        auto editVersionAction = new QAction;
                        editVersionAction->setIconText(u8"\U000F090C");
                        editVersionAction->setText(tr("Edit"));
                        editVersionAction->setEnabled(enabled);
                        connect(editVersionAction, &QAction::triggered, this,
                                [this, currentItemIndex, _versionIndex] {
                                    d->editVersion(currentItemIndex, _versionIndex);
                                });
                        menuActions.append(editVersionAction);
                        //
                        auto removeAction = new QAction;
                        removeAction->setIconText(u8"\U000F01B4");
                        removeAction->setText(tr("Remove"));
                        removeAction->setEnabled(enabled);
                        connect(removeAction, &QAction::triggered, this,
                                [this, currentItemIndex, _versionIndex] {
                                    d->removeVersion(currentItemIndex, _versionIndex);
                                });
                        menuActions.append(removeAction);
                        //
                        createNewVersionAction->setSeparator(true);
                        menuActions.move(0, menuActions.size() - 1);
                    }

                    auto menu = new ContextMenu(d->view.active);
                    menu->setActions(menuActions);
                    menu->setBackgroundColor(Ui::DesignSystem::color().background());
                    menu->setTextColor(Ui::DesignSystem::color().onBackground());
                    connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

                    menu->showContextMenu(QCursor::pos());
                });
    }

    //
    // Соединения со строителем моделей
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::modelNameChanged, this,
            [this](BusinessLayer::AbstractModel* _model, const QString& _name) {
                auto item = d->projectStructureModel->itemForUuid(_model->document()->uuid());
                d->projectStructureModel->setItemName(item, _name);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::modelColorChanged, this,
            [this](BusinessLayer::AbstractModel* _model, const QColor& _color) {
                auto item = d->projectStructureModel->itemForUuid(_model->document()->uuid());
                d->projectStructureModel->setItemColor(item, _color);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::modelContentChanged, this,
            &ProjectManager::handleModelChange);
    connect(&d->modelsFacade, &ProjectModelsFacade::modelUndoRequested, this,
            &ProjectManager::undoModelChange);
    connect(&d->modelsFacade, &ProjectModelsFacade::modelRemoveRequested, this,
            [this](BusinessLayer::AbstractModel* _model) {
                auto item = d->projectStructureModel->itemForUuid(_model->document()->uuid());
                auto index = d->projectStructureModel->indexForItem(item);
                d->removeDocuments({ index });
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::projectNameChanged, this,
            &ProjectManager::projectNameChanged);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectCollaboratorInviteRequested, this,
            &ProjectManager::projectCollaboratorInviteRequested);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectCollaboratorUpdateRequested, this,
            &ProjectManager::projectCollaboratorUpdateRequested);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectCollaboratorRemoveRequested, this,
            &ProjectManager::projectCollaboratorRemoveRequested);
    connect(&d->modelsFacade, &ProjectModelsFacade::createCharacterRequested, this,
            [this](const QString& _name, const QByteArray& _content) {
                d->addDocumentToContainer(Domain::DocumentObjectType::Characters,
                                          Domain::DocumentObjectType::Character, _name, _content);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::characterNameChanged, this,
            [this](BusinessLayer::AbstractModel* _character, const QString& _newName,
                   const QString& _oldName) {
                if (_character == nullptr || _oldName.isEmpty()) {
                    return;
                }

                //
                // Найти все модели где может встречаться персонаж и заменить в них его имя со
                // старого на новое
                //
                const auto screenplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
                for (auto model : screenplayModels) {
                    auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
                    screenplay->updateCharacterName(_oldName, _newName);
                }
                //
                const auto comicBookModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ComicBookText);
                for (auto model : comicBookModels) {
                    auto comicBook = qobject_cast<BusinessLayer::ComicBookTextModel*>(model);
                    comicBook->updateCharacterName(_oldName, _newName);
                }
                //
                const auto audioplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::AudioplayText);
                for (auto model : audioplayModels) {
                    auto audioplay = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
                    audioplay->updateCharacterName(_oldName, _newName);
                }
                //
                const auto stageplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::StageplayText);
                for (auto model : stageplayModels) {
                    auto stageplay = qobject_cast<BusinessLayer::StageplayTextModel*>(model);
                    stageplay->updateCharacterName(_oldName, _newName);
                }

                //
                // Если персонаж переименовывается в другого существующего, то удалим дубль
                //
                const auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                    d->modelsFacade.modelFor(Domain::DocumentObjectType::Characters));
                if (charactersModel->characters(_newName).size() > 1) {
                    auto document = _character->document();
                    if (document == nullptr) {
                        return;
                    }

                    //
                    // Сохраняем имя, т.к. ссылка перестанет работать, когда документ будет удалён
                    //
                    const auto characterToSelectName = _newName;
                    const auto item = d->projectStructureModel->itemForUuid(document->uuid());
                    charactersModel->removeCharacterModel(
                        qobject_cast<BusinessLayer::CharacterModel*>(_character));
                    d->modelsFacade.removeModelFor(document);
                    DataStorageLayer::StorageFacade::documentStorage()->removeDocument(document);
                    d->projectStructureModel->removeItem(item);

                    if (!d->navigator->currentIndex().isValid()) {
                        const auto item = d->projectStructureModel->itemForUuid(
                            charactersModel->character(characterToSelectName)->document()->uuid());
                        const auto itemIndex = d->projectStructureModel->indexForItem(item);
                        d->setCurrentIndex(itemIndex);
                    }
                }
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::characterDialoguesUpdateRequested, this,
            [this](BusinessLayer::AbstractModel* _character) {
                if (_character == nullptr) {
                    return;
                }
                auto character = qobject_cast<BusinessLayer::CharacterModel*>(_character);

                //
                // Найти все модели где может встречаться персонаж и извлечь его реплики
                //
                QVector<BusinessLayer::CharacterDialogues> documentDialogues;
                const auto screenplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
                for (auto model : screenplayModels) {
                    auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
                    const auto dialogues = screenplay->characterDialogues(character->name());
                    if (dialogues.isEmpty()) {
                        continue;
                    }

                    auto documentName = screenplay->informationModel()->name();
                    if (const auto screenplayItem
                        = d->projectStructureModel->itemForUuid(screenplay->document()->uuid());
                        screenplayItem->name() != tr("Screenplay")) {
                        documentName.append(
                            QString(" [%1 %2]").arg(tr("draft"), screenplayItem->name()));
                    }

                    documentDialogues.append(
                        { screenplay->document()->uuid(), documentName, dialogues });
                }
                //
                const auto comicBookModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ComicBookText);
                for (auto model : comicBookModels) {
                    auto comicBook = qobject_cast<BusinessLayer::ComicBookTextModel*>(model);
                    const auto dialogues = comicBook->characterDialogues(character->name());
                    if (dialogues.isEmpty()) {
                        continue;
                    }

                    QString documentName = comicBook->informationModel()->name();
                    if (const auto screenplayItem
                        = d->projectStructureModel->itemForUuid(comicBook->document()->uuid());
                        screenplayItem->name() != tr("Script")) {
                        documentName.append(
                            QString(" [%1 %2]").arg(tr("draft"), screenplayItem->name()));
                    }

                    documentDialogues.append(
                        { comicBook->document()->uuid(), documentName, dialogues });
                }
                //
                const auto audioplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::AudioplayText);
                for (auto model : audioplayModels) {
                    auto audioplay = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
                    const auto dialogues = audioplay->characterDialogues(character->name());
                    if (dialogues.isEmpty()) {
                        continue;
                    }

                    QString documentName = audioplay->informationModel()->name();
                    if (const auto screenplayItem
                        = d->projectStructureModel->itemForUuid(audioplay->document()->uuid());
                        screenplayItem->name() != tr("Script")) {
                        documentName.append(
                            QString(" [%1 %2]").arg(tr("draft"), screenplayItem->name()));
                    }

                    documentDialogues.append(
                        { audioplay->document()->uuid(), documentName, dialogues });
                }
                //
                const auto stageplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::StageplayText);
                for (auto model : stageplayModels) {
                    auto stageplay = qobject_cast<BusinessLayer::StageplayTextModel*>(model);
                    const auto dialogues = stageplay->characterDialogues(character->name());
                    if (dialogues.isEmpty()) {
                        continue;
                    }

                    QString documentName = stageplay->informationModel()->name();
                    if (const auto screenplayItem
                        = d->projectStructureModel->itemForUuid(stageplay->document()->uuid());
                        screenplayItem->name() != tr("Script")) {
                        documentName.append(
                            QString(" [%1 %2]").arg(tr("draft"), screenplayItem->name()));
                    }

                    documentDialogues.append(
                        { stageplay->document()->uuid(), documentName, dialogues });
                }

                character->setDialogues(documentDialogues);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::createLocationRequested, this,
            [this](const QString& _name, const QByteArray& _content) {
                d->addDocumentToContainer(Domain::DocumentObjectType::Locations,
                                          Domain::DocumentObjectType::Location, _name, _content);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::locationNameChanged, this,
            [this](BusinessLayer::AbstractModel* _location, const QString& _newName,
                   const QString& _oldName) {
                if (_location == nullptr || _oldName.isEmpty()) {
                    return;
                }

                //
                // Найти все модели где может встречаться локация и заменить в них её имя со
                // старого на новое
                //
                const auto models
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
                for (auto model : models) {
                    auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
                    screenplay->updateLocationName(_oldName, _newName);
                }

                //
                // Если локация переименовывается в другую существующую, то удалим дубль
                //
                const auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(
                    d->modelsFacade.modelFor(Domain::DocumentObjectType::Locations));
                if (locationsModel->locations(_newName).size() > 1) {
                    auto document = _location->document();
                    if (document == nullptr) {
                        return;
                    }

                    //
                    // Сохраняем имя, т.к. ссылка перестанет работать, когда документ будет удалён
                    //
                    const auto locationToSelectName = _newName;
                    const auto item = d->projectStructureModel->itemForUuid(document->uuid());
                    locationsModel->removeLocationModel(
                        qobject_cast<BusinessLayer::LocationModel*>(_location));
                    d->modelsFacade.removeModelFor(document);
                    DataStorageLayer::StorageFacade::documentStorage()->removeDocument(document);
                    d->projectStructureModel->removeItem(item);

                    if (!d->navigator->currentIndex().isValid()) {
                        const auto item = d->projectStructureModel->itemForUuid(
                            locationsModel->location(locationToSelectName)->document()->uuid());
                        const auto itemIndex = d->projectStructureModel->indexForItem(item);
                        d->setCurrentIndex(itemIndex);
                    }
                }

                //
                // Переименовываем вложенные локации
                //
                for (int index = 0; index < locationsModel->rowCount(); ++index) {
                    auto location = locationsModel->location(index);
                    if (location->name().startsWith(_newName)) {
                        continue;
                    }

                    auto oldNameDots = _oldName;
                    oldNameDots.replace(" - ", ". ");
                    if (location->name().startsWith(_oldName)) {
                        auto newLocationName = location->name();
                        newLocationName.replace(_oldName, _newName);
                        location->setName(newLocationName);
                    } else if (location->name().startsWith(oldNameDots)) {
                        auto newLocationName = location->name();
                        newLocationName.replace(oldNameDots, _newName);
                        location->setName(newLocationName);
                    }
                }
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::locationScenesUpdateRequested, this,
            [this](BusinessLayer::AbstractModel* _location) {
                if (_location == nullptr) {
                    return;
                }
                auto location = qobject_cast<BusinessLayer::LocationModel*>(_location);

                //
                // Найти все модели где может встречаться локация и извлечь её сцены
                //
                QVector<BusinessLayer::LocationScenes> documentScenes;
                const auto screenplayModels
                    = d->modelsFacade.modelsFor(Domain::DocumentObjectType::ScreenplayText);
                for (auto model : screenplayModels) {
                    auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
                    const auto scenes = screenplay->locationScenes(location->name());
                    if (scenes.isEmpty()) {
                        continue;
                    }

                    auto documentName = screenplay->informationModel()->name();
                    if (const auto screenplayItem
                        = d->projectStructureModel->itemForUuid(screenplay->document()->uuid());
                        screenplayItem->name() != tr("Screenplay")) {
                        documentName.append(
                            QString(" [%1 %2]").arg(tr("draft"), screenplayItem->name()));
                    }

                    documentScenes.append({ screenplay->document()->uuid(), documentName, scenes });
                }

                location->setScenes(documentScenes);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::createWorldRequested, this,
            [this](const QString& _name, const QByteArray& _content) {
                d->addDocumentToContainer(Domain::DocumentObjectType::Worlds,
                                          Domain::DocumentObjectType::World, _name, _content);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::titlePageCharactersUpdateRequested, this,
            [this](BusinessLayer::AbstractModel* _titlePage) {
                if (_titlePage == nullptr) {
                    return;
                }

                using namespace BusinessLayer;

                //
                // Найти сценарий, к которому относится эта титульная страница
                //
                const auto titlePageItem
                    = d->projectStructureModel->itemForUuid(_titlePage->document()->uuid());
                const auto scriptItem = titlePageItem->parent();
                ScriptTextModel* scriptTextModel = nullptr;
                for (int childIndex = 0; childIndex < scriptItem->childCount(); ++childIndex) {
                    const auto childItem = scriptItem->childAt(childIndex);
                    if (childItem->type() == Domain::DocumentObjectType::AudioplayText
                        || childItem->type() == Domain::DocumentObjectType::StageplayText) {
                        scriptTextModel = qobject_cast<ScriptTextModel*>(
                            d->modelsFacade.modelFor(childItem->uuid()));
                        break;
                    }
                }
                if (scriptTextModel == nullptr) {
                    return;
                }

                //
                // Определить список персонажей сценария
                //
                QVector<QPair<QString, QString>> characters;
                const auto charactersFromText = scriptTextModel->findCharactersFromText();
                for (const auto& characterName : charactersFromText) {
                    const auto character = scriptTextModel->character(characterName);
                    characters.append(
                        { characterName,
                          character != nullptr ? character->oneSentenceDescription() : QString() });
                };

                //
                // Задать список персонажей в титульную страницу
                //
                auto titlePage = qobject_cast<BusinessLayer::TitlePageModel*>(_titlePage);
                titlePage->setCharacters(characters);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayTitlePageVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayTitlePage,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplaySynopsisVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplaySynopsis,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayTreatmentVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayTreatment,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayTextVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplayStatisticsVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplayStatistics,
                                      _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplaySeriesTitlePageVisibilityChanged,
            this, [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplaySeriesTitlePage,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplaySeriesSynopsisVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplaySeriesSynopsis,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplaySeriesTreatmentVisibilityChanged,
            this, [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplaySeriesTreatment,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplaySeriesTextVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ScreenplaySeriesText,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::screenplaySeriesStatisticsVisibilityChanged,
            this, [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(
                    _model, Domain::DocumentObjectType::ScreenplaySeriesStatistics, _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookTitlePageVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookTitlePage,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookSynopsisVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookSynopsis,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookTextVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::comicBookStatisticsVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::ComicBookStatistics,
                                      _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplayTitlePageVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::AudioplayTitlePage,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplaySynopsisVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::AudioplaySynopsis,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplayTextVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::AudioplayText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::audioplayStatisticsVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::AudioplayStatistics,
                                      _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::stageplayTitlePageVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::StageplayTitlePage,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::stageplaySynopsisVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::StageplaySynopsis,
                                      _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::stageplayTextVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::StageplayText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::stageplayStatisticsVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::StageplayStatistics,
                                      _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::novelTitlePageVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::NovelTitlePage, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::novelSynopsisVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::NovelSynopsis, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::novelOutlineVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::NovelOutline, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::novelTextVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::NovelText, _visible);
            });
    connect(&d->modelsFacade, &ProjectModelsFacade::novelStatisticsVisibilityChanged, this,
            [this](BusinessLayer::AbstractModel* _model, bool _visible) {
                d->setDocumentVisible(_model, Domain::DocumentObjectType::NovelStatistics,
                                      _visible);
            });
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::emptyRecycleBinRequested, this,
            [this] { d->emptyRecycleBin(); });
    //
    // Соединения с хранилищем изображений
    //
    connect(&d->documentImageStorage, &DataStorageLayer::DocumentImageStorage::imageAdded, this,
            [this](const QUuid& _uuid) { emit uploadDocumentRequested(_uuid, true); });
    connect(&d->documentImageStorage, &DataStorageLayer::DocumentImageStorage::imageUpdated, this,
            [this](const QUuid& _uuid) { emit uploadDocumentRequested(_uuid, false); });
    connect(&d->documentImageStorage, &DataStorageLayer::DocumentImageStorage::imageRequested, this,
            [this](const QUuid& _uuid) { emit downloadDocumentRequested(_uuid); });
    //
    // Соединения со список соавторов
    //
    connect(d->collaboratorsToolBar, &Ui::CollaboratorsToolBar::collaboratorClicked, this,
            [this](const QString& _cursorId) {
                if (!d->collaboratorsCursors.contains(_cursorId)) {
                    return;
                }

                //
                // Покажем нужный документ
                //
                const auto collaboratorCursor = d->collaboratorsCursors.value(_cursorId);
                activateLink(QUuid(collaboratorCursor.documentUuid), {}, {});
                //
                // и сместим курсор к положению соавтора
                //
                if (d->view.active == d->view.left) {
                    d->pluginsBuilder.setViewCurrentCursor(collaboratorCursor,
                                                           d->view.activeViewMimeType);
                } else {
                    d->pluginsBuilder.setSecondaryViewCurrentCursor(collaboratorCursor,
                                                                    d->view.activeViewMimeType);
                }
            });
    connect(&d->collaboratorsUpdateDebouncer, &Debouncer::gotWork, this, [this] {
        setCursors({ d->collaboratorsCursors.begin(), d->collaboratorsCursors.end() });
    });
}

ProjectManager::~ProjectManager() = default;

QWidget* ProjectManager::toolBar() const
{
    return d->toolBar;
}

QWidget* ProjectManager::navigator() const
{
    return d->navigator;
}

QWidget* ProjectManager::view() const
{
    return d->view.container;
}

void ProjectManager::setHasUnreadNotifications(bool _hasUnreadNotifications)
{
    d->toolBar->setBadgeVisible(d->toolBar->actions().constFirst(), _hasUnreadNotifications);
}

void ProjectManager::toggleFullScreen(bool _isFullScreen)
{
    //
    // Отключаем возможность разделять и объединять панели в полноэкранном режиме
    //
    d->splitScreenAction->setEnabled(!_isFullScreen);

    //
    // При переходе в полноэкранный режим, если активировано разделение экрана, то скроем неактивный
    // редактор и запомним состояние разделения
    //
    if (_isFullScreen) {
        if (d->splitScreenAction->isChecked()) {
            d->view.stateBeforeFullscreen = d->view.container->saveState();
            d->view.inactive->hide();
        }
        d->view.active->setDraftsVisible(false);
    }

    //
    // Собственно активируем полноэкранный режим
    //
    if (d->view.active == d->view.left) {
        d->pluginsBuilder.toggleViewFullScreen(_isFullScreen, d->view.activeViewMimeType);
    } else {
        d->pluginsBuilder.toggleSecondaryViewFullScreen(_isFullScreen, d->view.activeViewMimeType);
    }

    //
    // При выходе из полноэкранного режима, если экран был разделён, то покажем неактивный редактор
    // и восстановим состояние разеделения
    //
    if (!_isFullScreen) {
        if (d->splitScreenAction->isChecked()) {
            d->view.inactive->show();
            d->view.container->restoreState(d->view.stateBeforeFullscreen);
        }

        const auto item = d->aliasedItemForIndex(d->view.activeIndex);
        d->view.active->setDraftsVisible(item->versions().count() > 0);
    }
}

void ProjectManager::reconfigureAll()
{
    d->pluginsBuilder.reconfigureAll();
}

void ProjectManager::reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureSimpleTextEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureSimpleTextNavigator()
{
    d->pluginsBuilder.reconfigureSimpleTextNavigator();
}

void ProjectManager::reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureScreenplayEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureScreenplayNavigator()
{
    d->pluginsBuilder.reconfigureScreenplayNavigator();
}

void ProjectManager::reconfigureScreenplayDuration()
{
    for (auto model : d->modelsFacade.loadedModels()) {
        auto screenplayModel = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
        if (screenplayModel == nullptr) {
            continue;
        }

        screenplayModel->recalculateCounters();
    }
}

void ProjectManager::reconfigureComicBookEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureComicBookEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureComicBookNavigator()
{
    d->pluginsBuilder.reconfigureComicBookNavigator();
}

void ProjectManager::reconfigureAudioplayEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureAudioplayEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureAudioplayNavigator()
{
    d->pluginsBuilder.reconfigureAudioplayNavigator();
}

void ProjectManager::reconfigureAudioplayDuration()
{
    for (auto model : d->modelsFacade.loadedModels()) {
        auto audioplayModel = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
        if (audioplayModel == nullptr) {
            continue;
        }

        audioplayModel->recalculateCounters();
    }
}

void ProjectManager::reconfigureStageplayEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureStageplayEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureStageplayNavigator()
{
    d->pluginsBuilder.reconfigureStageplayNavigator();
}

void ProjectManager::reconfigureNovelEditor(const QStringList& _changedSettingsKeys)
{
    d->pluginsBuilder.reconfigureNovelEditor(_changedSettingsKeys);
}

void ProjectManager::reconfigureNovelNavigator()
{
    d->pluginsBuilder.reconfigureNovelNavigator();
}

void ProjectManager::checkAvailabilityToEdit()
{
    d->pluginsBuilder.checkAvailabilityToEdit(d->isProjectInTeam);
}

void ProjectManager::loadCurrentProject(BusinessLayer::ProjectsModelProjectItem* _project)
{
    //
    // Загружаем структуру
    //
    d->projectStructureModel->setDocument(
        DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Structure));
    //
    // ... если структура только была создана, установим в документ болванку данных
    //
    if (d->projectStructureModel->document()->content().isEmpty()) {
        d->projectStructureModel->reassignContent();
    }
    emit structureModelChanged(d->projectStructureModel);

    //
    // Сохраняем параметры проекта для дальнейшего использования
    //
    updateCurrentProject(_project);

    //
    // Настраиваем доступность модулей для открываемого проекта
    //
    checkAvailabilityToEdit();

    //
    // Загружаем информацию о проекте
    //
    auto projectInformationModel = qobject_cast<BusinessLayer::ProjectInformationModel*>(
        d->modelsFacade.modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Project)));
    emit projectUuidChanged(projectInformationModel->document()->uuid());
    if (projectInformationModel->name().isEmpty()) {
        //
        // При создании нового проекта, применим его название и сразу сохраним изменения в документ
        // инфомации о проекте и структуре, чтобы эта информация сразу попала в облако, а не была
        // заменена на пустую
        //
        projectInformationModel->setName(_project->name());
        projectInformationModel->saveChanges();
        d->projectStructureModel->saveChanges();
    } else {
        emit projectNameChanged(projectInformationModel->name());
        emit projectLoglineChanged(projectInformationModel->logline());
        emit projectCoverChanged(projectInformationModel->cover());
    }
    projectInformationModel->setStructureModel(d->projectStructureModel);

    //
    // Восстанавливаем состояние проекта
    //
    restoreCurrentProjectState(_project->path());

    //
    // Обновляем режим редактирования для всех вьюх
    //
    d->updateViewsEditingMode();
}

void ProjectManager::updateCurrentProject(BusinessLayer::ProjectsModelProjectItem* _project)
{
    const auto projectTeam = _project->teamId() != Domain::kInvalidId
        ? static_cast<BusinessLayer::ProjectsModelTeamItem*>(_project->parent())
        : nullptr;

    d->projectPath = _project->path();
    d->isProjectRemote = _project->isRemote();
    d->isProjectOwner = _project->isOwner();
    d->isProjectInTeam = _project->teamId() != Domain::kInvalidId;
    d->allowGrantAccessToProject
        = d->isProjectOwner || (projectTeam != nullptr && projectTeam->allowGrantAccessToProject());
    d->editingMode = _project->editingMode();
    d->editingPermissions = _project->editingPermissions();
    d->pluginsBuilder.setEditingPermissions(d->editingPermissions);
    d->projectStructureProxyModel->setItemsFilter(d->editingPermissions.keys());
    d->navigator->setReadOnly(d->editingMode != DocumentEditingMode::Edit);

    d->projectStructureModel->setProjectName(_project->name());

    auto projectInformationModel = qobject_cast<BusinessLayer::ProjectInformationModel*>(
        d->modelsFacade.modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Project)));
    projectInformationModel->setCollaborators(_project->collaborators());
    if (projectTeam != nullptr) {
        projectInformationModel->setTeammates(projectTeam->members());
    }
}

void ProjectManager::restoreCurrentProjectState(const QString& _path)
{
    //
    // Загрузить состояние дерева
    //
    d->navigator->restoreState(d->projectStructureModel->isNewProject(),
                               settingsValue(DataStorageLayer::projectStructureKey(_path)));

    //
    // При необходимости открыть навигатор по документу
    //
    const auto isProjectStructureVisible
        = settingsValue(DataStorageLayer::projectStructureVisibleKey(_path));
    if (isProjectStructureVisible.isValid() && !isProjectStructureVisible.toBool()) {
        showNavigator(d->navigator->currentIndex());
    }
}

void ProjectManager::closeCurrentProject(const QString& _path)
{
    //
    // Сохранить состояние дерева
    //
    setSettingsValue(DataStorageLayer::projectStructureKey(_path), d->navigator->saveState());
    setSettingsValue(DataStorageLayer::projectStructureVisibleKey(_path),
                     d->navigator->isProjectNavigatorShown());

    //
    // Сохранить состояние отображения версий для текущего документа
    //
    if (d->view.activeModel != nullptr && d->view.activeModel->document() != nullptr) {
        const auto item = d->aliasedItemForIndex(d->view.activeIndex);
        setSettingsValue(documentSettingsKey(item->uuid(), kCurrentVersionKey),
                         d->view.active->currentDraft());
    }

    //
    // FIXME: Сохранять и восстанавливать состояние панелей
    //
    while (!d->view.windows.isEmpty()) {
        auto window = d->view.windows.takeFirst();
        window.view->asQWidget()->close();
        window.view->asQWidget()->deleteLater();
    }
    if (d->splitScreenAction->isChecked()) {
        d->splitScreenAction->toggle();
    }
    d->view.activeIndex = {};
    d->view.active->showDefaultPage();
    d->view.activeViewMimeType.clear();
    d->view.inactiveIndex = {};
    d->view.inactive->showDefaultPage();
    d->view.inactiveViewMimeType.clear();

    //
    // Очищаем все карты для синхронизации
    //
    d->documentToSyncTimer.clear();
    d->changesForSync.clear();

    //
    // Очищаем структуру
    //
    d->projectStructureModel->clear();

    //
    // Сбрасываем все плагины
    //
    d->pluginsBuilder.resetModels();

    //
    // Очищаем все загруженные модели документов
    //
    d->modelsFacade.clear();

    //
    // Сбрасываем помеченные на удаление и загруженные изображения
    //
    d->documentImageStorage.clear();

    //
    // Сбрасываем название последнего открытого проекта
    //
    d->projectStructureModel->setProjectName({});

    //
    // Сбрасываем список соавторов
    //
    d->collaboratorsCursors.clear();
    d->collaboratorsToolBar->setCollaborators({});
}

void ProjectManager::clearChangesHistory()
{
    DataStorageLayer::StorageFacade::documentChangeStorage()->removeAll();
}

void ProjectManager::saveChanges()
{
    //
    // Сохраняем структуру
    //
    const auto structure = d->projectStructureModel->document();
    DataStorageLayer::StorageFacade::documentStorage()->saveDocument(structure);

    //
    // Сохраняем остальные документы
    //
    const auto loadedDocuments = d->modelsFacade.loadedDocuments();
    for (auto document : loadedDocuments) {
        DataStorageLayer::StorageFacade::documentStorage()->saveDocument(document);
    }

    //
    // Сохраняем изображения
    //
    d->documentImageStorage.saveChanges();

    //
    // Сохраняем все изменения документов
    //
    DataStorageLayer::StorageFacade::documentChangeStorage()->store();
}

void ProjectManager::addCharacter(const QString& _name, const QString& _content)
{
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
        Domain::DocumentObjectType::Characters);
    auto model = d->modelsFacade.modelFor(document);
    auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(model);
    if (charactersModel == nullptr) {
        return;
    }

    charactersModel->createCharacter(_name, _content.toUtf8());
}

void ProjectManager::addLocation(const QString& _name, const QString& _content)
{
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(
        Domain::DocumentObjectType::Locations);
    auto model = d->modelsFacade.modelFor(document);
    auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(model);
    if (locationsModel == nullptr) {
        return;
    }

    locationsModel->createLocation(_name, _content.toUtf8());
}

void ProjectManager::addDocument(const BusinessLayer::AbstractImporter::Document& _document,
                                 BusinessLayer::StructureModelItem* _parentItem)
{
    //
    // ATTENTION: Копипаста из StructureModel::addDocument, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto visible = true;
        const auto readOnly = false;
        return new BusinessLayer::StructureModelItem(uuid, _type, _name, {}, visible, readOnly);
    };

    const auto parentItem
        = _parentItem == nullptr ? d->projectStructureModel->itemForIndex({}) : _parentItem;

    BusinessLayer::StructureModelItem* item = nullptr;
    switch (_document.type) {
    case DocumentObjectType::Folder:
    case DocumentObjectType::SimpleText: {
        item = createItem(_document.type, _document.name);
        break;
    }

    case DocumentObjectType::MindMap: {
        item = createItem(_document.type,
                          !_document.name.isEmpty() ? _document.name : tr("Mind map"));
        break;
    }

    case DocumentObjectType::ImagesGallery: {
        item = createItem(_document.type,
                          !_document.name.isEmpty() ? _document.name : tr("Images gallery"));
        break;
    }

    case DocumentObjectType::Presentation: {
        item = createItem(_document.type,
                          !_document.name.isEmpty() ? _document.name : tr("Presentation"));
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }

    //
    // Если удалось создать документ, добавляем его в проект
    //
    if (item != nullptr) {
        d->projectStructureModel->appendItem(item, parentItem, _document.content.toUtf8());
    }

    //
    // Добавляем вложенные документы, если таковые имеются
    //
    for (const auto& child : _document.children) {
        addDocument(child, item);
    }
}

void ProjectManager::addAudioplay(const QString& _name, const QString& _titlePage,
                                  const QString& _text)
{
    //
    // ATTENTION: Копипаста из StructureModel::addDocument, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto visible = true;
        const auto readOnly = false;
        return new BusinessLayer::StructureModelItem(uuid, _type, _name, {}, visible, readOnly);
    };

    auto rootItem = d->projectStructureModel->itemForIndex({});
    auto audioplayItem = createItem(DocumentObjectType::Audioplay, _name);
    d->projectStructureModel->appendItem(audioplayItem, rootItem);

    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::AudioplayTitlePage, tr("Title page")), audioplayItem,
        _titlePage.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::AudioplaySynopsis, tr("Synopsis")), audioplayItem, {});
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::AudioplayText, tr("Audioplay")), audioplayItem,
        _text.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::AudioplayStatistics, tr("Statistics")), audioplayItem, {});

    //
    // Фокусируем добавленный документ
    //
    d->setCurrentItem(audioplayItem);
}

void ProjectManager::addComicBook(const QString& _name, const QString& _titlePage,
                                  const QString& _text)
{
    //
    // ATTENTION: Копипаста из StructureModel::addDocument, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto visible = true;
        const auto readOnly = false;
        return new BusinessLayer::StructureModelItem(uuid, _type, _name, {}, visible, readOnly);
    };

    auto rootItem = d->projectStructureModel->itemForIndex({});
    auto comicbookItem = createItem(DocumentObjectType::ComicBook, _name);
    d->projectStructureModel->appendItem(comicbookItem, rootItem);

    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ComicBookTitlePage, tr("Title page")), comicbookItem,
        _titlePage.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ComicBookSynopsis, tr("Synopsis")), comicbookItem, {});
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ComicBookText, tr("Comic book")), comicbookItem,
        _text.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ComicBookStatistics, tr("Statistics")), comicbookItem, {});

    //
    // Фокусируем добавленный документ
    //
    d->setCurrentItem(comicbookItem);
}

void ProjectManager::addNovel(const QString& _name, const QString& _text)
{
    //
    // ATTENTION: Копипаста из StructureModel::addDocument, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto visible = true;
        const auto readOnly = false;
        return new BusinessLayer::StructureModelItem(uuid, _type, _name, {}, visible, readOnly);
    };

    auto rootItem = d->projectStructureModel->itemForIndex({});
    auto novelItem = createItem(DocumentObjectType::Novel, _name);
    d->projectStructureModel->appendItem(novelItem, rootItem);

    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::NovelTitlePage, tr("Title page")), novelItem, {});
    auto synopsisItem = createItem(DocumentObjectType::NovelSynopsis, tr("Synopsis"));
    d->projectStructureModel->appendItem(synopsisItem, novelItem, {});
    d->projectStructureModel->appendItem(createItem(DocumentObjectType::NovelText, tr("Novel")),
                                         novelItem, _text.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::NovelStatistics, tr("Statistics")), novelItem, {});
    //
    // Вставляем план после всех документов, т.к. он является алиасом к документу романа
    // и чтобы его сконструировать, нужны другие документы
    //
    d->projectStructureModel->insertItem(
        createItem(DocumentObjectType::NovelOutline, tr("Outline")), synopsisItem, {});

    //
    // Фокусируем добавленный документ
    //
    d->setCurrentItem(novelItem);
}

void ProjectManager::addScreenplay(const QString& _name, const QString& _titlePage,
                                   const QString& _synopsis, const QString& _treatment,
                                   const QString& _text)
{
    //
    // ATTENTION: Копипаста из StructureModel::addDocument, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto visible = true;
        const auto readOnly = false;
        return new BusinessLayer::StructureModelItem(uuid, _type, _name, {}, visible, readOnly);
    };

    auto rootItem = d->projectStructureModel->itemForIndex({});
    auto screenplayItem = createItem(DocumentObjectType::Screenplay, _name);
    d->projectStructureModel->appendItem(screenplayItem, rootItem);

    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ScreenplayTitlePage, tr("Title page")), screenplayItem,
        _titlePage.toUtf8());
    auto synopsisItem = createItem(DocumentObjectType::ScreenplaySynopsis, tr("Synopsis"));
    d->projectStructureModel->appendItem(synopsisItem, screenplayItem, _synopsis.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ScreenplayText, tr("Screenplay")), screenplayItem,
        _text.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::ScreenplayStatistics, tr("Statistics")), screenplayItem, {});
    //
    // Вставляем тритмент после всех документов, т.к. он является алиасом к документу сценария
    // и чтобы его сконструировать, нужны другие документы
    //
    d->projectStructureModel->insertItem(
        createItem(DocumentObjectType::ScreenplayTreatment, tr("Treatment")), synopsisItem,
        _treatment.toUtf8());

    //
    // Фокусируем добавленный документ
    //
    d->setCurrentItem(screenplayItem);
}

void ProjectManager::addStageplay(const QString& _name, const QString& _titlePage,
                                  const QString& _text)
{
    //
    // ATTENTION: Копипаста из StructureModel::addDocument, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto visible = true;
        const auto readOnly = false;
        return new BusinessLayer::StructureModelItem(uuid, _type, _name, {}, visible, readOnly);
    };

    auto rootItem = d->projectStructureModel->itemForIndex({});
    auto stageplayItem = createItem(DocumentObjectType::Stageplay, _name);
    d->projectStructureModel->appendItem(stageplayItem, rootItem);

    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::StageplayTitlePage, tr("Title page")), stageplayItem,
        _titlePage.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::StageplaySynopsis, tr("Synopsis")), stageplayItem, {});
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::StageplayText, tr("Stageplay")), stageplayItem,
        _text.toUtf8());
    d->projectStructureModel->appendItem(
        createItem(DocumentObjectType::StageplayStatistics, tr("Statistics")), stageplayItem, {});

    //
    // Фокусируем добавленный документ
    //
    d->setCurrentItem(stageplayItem);
}

void ProjectManager::importPresentation(const QString& _filePath, const QUuid& _documentUuid)
{
    const auto document
        = DataStorageLayer::StorageFacade::documentStorage()->document(_documentUuid);
    const auto model = d->modelsFacade.modelFor(document);
    const auto presentationModel = qobject_cast<BusinessLayer::PresentationModel*>(model);
    emit presentationModel->downloadPresentationRequested(_filePath);
}

QVector<QPair<QString, BusinessLayer::AbstractModel*>> ProjectManager::currentModelsForExport()
    const
{
    auto itemModels = [this](const BusinessLayer::StructureModelItem* _item)
        -> QVector<QPair<QString, BusinessLayer::AbstractModel*>> {
        if (_item == nullptr) {
            return {};
        }

        QVector<QPair<QString, BusinessLayer::AbstractModel*>> models;
        models.append({ tr("Current draft"), d->modelsFacade.modelFor(_item->uuid()) });
        for (const auto& version : _item->versions()) {
            models.append({ version->name(), d->modelsFacade.modelFor(version->uuid()) });
        }
        return models;
    };
    auto scriptTextModel = [itemModels](const BusinessLayer::StructureModelItem* _item)
        -> QVector<QPair<QString, BusinessLayer::AbstractModel*>> {
        if (_item == nullptr) {
            return {};
        }

        const QSet<Domain::DocumentObjectType> scriptTypes = {
            Domain::DocumentObjectType::AudioplayText,  Domain::DocumentObjectType::ComicBookText,
            Domain::DocumentObjectType::ScreenplayText, Domain::DocumentObjectType::StageplayText,
            Domain::DocumentObjectType::NovelText,
        };
        for (int childIndex = 2; childIndex < _item->childCount(); ++childIndex) {
            if (scriptTypes.contains(_item->childAt(childIndex)->type())) {
                return itemModels(_item->childAt(childIndex));
            }
        }
        return {};
    };

    const auto document = d->view.activeModel->document();
    const auto item = d->projectStructureModel->itemForUuid(document->uuid());
    switch (document->type()) {
    case Domain::DocumentObjectType::Audioplay:
    case Domain::DocumentObjectType::ComicBook:
    case Domain::DocumentObjectType::Screenplay:
    case Domain::DocumentObjectType::Stageplay:
    case Domain::DocumentObjectType::Novel: {
        return scriptTextModel(item);
    }
    case Domain::DocumentObjectType::AudioplayTitlePage:
    case Domain::DocumentObjectType::AudioplaySynopsis:
    case Domain::DocumentObjectType::AudioplayText:
    case Domain::DocumentObjectType::AudioplayStatistics:
    case Domain::DocumentObjectType::ComicBookTitlePage:
    case Domain::DocumentObjectType::ComicBookSynopsis:
    case Domain::DocumentObjectType::ComicBookText:
    case Domain::DocumentObjectType::ComicBookStatistics:
    case Domain::DocumentObjectType::ScreenplayTitlePage:
    case Domain::DocumentObjectType::ScreenplaySynopsis:
    case Domain::DocumentObjectType::ScreenplayTreatment:
    case Domain::DocumentObjectType::ScreenplayText:
    case Domain::DocumentObjectType::ScreenplayStatistics:
    case Domain::DocumentObjectType::StageplayTitlePage:
    case Domain::DocumentObjectType::StageplaySynopsis:
    case Domain::DocumentObjectType::StageplayText:
    case Domain::DocumentObjectType::StageplayStatistics:
    case Domain::DocumentObjectType::NovelTitlePage:
    case Domain::DocumentObjectType::NovelSynopsis:
    case Domain::DocumentObjectType::NovelOutline:
    case Domain::DocumentObjectType::NovelText:
    case Domain::DocumentObjectType::NovelStatistics: {
        if (item == nullptr) {
            break;
        }

        return scriptTextModel(item->parent());
    }

    default: {
        break;
    }
    }

    return itemModels(item);
}

BusinessLayer::AbstractModel* ProjectManager::firstScriptModel() const
{
    auto model = d->projectStructureModel;
    for (auto row = 0; row < model->rowCount(); ++row) {
        const auto index = model->index(row, 0);
        const auto item = d->projectStructureModel->itemForIndex(index);
        if (item != nullptr
            && (item->type() == Domain::DocumentObjectType::Audioplay
                || item->type() == Domain::DocumentObjectType::ComicBook
                || item->type() == Domain::DocumentObjectType::Screenplay
                || item->type() == Domain::DocumentObjectType::Stageplay
                || item->type() == Domain::DocumentObjectType::Novel)) {
            constexpr int scriptTextIndex = 2;
            return d->modelsFacade.modelFor(item->childAt(scriptTextIndex)->uuid());
        }
    }

    return nullptr;
}

Domain::DocumentObject* ProjectManager::currentDocument() const
{
    if (d->view.activeModel == nullptr) {
        return nullptr;
    }

    return d->view.activeModel->document();
}

void ProjectManager::setCurrentDocumentExportAvailable(bool _available)
{
    d->isCurrentDocumentExportAvailable = _available;
};

QVector<Domain::DocumentObject*> ProjectManager::unsyncedDocuments() const
{
    using namespace DataStorageLayer;

    QVector<Domain::DocumentObject*> documents;
    //
    // Если нет изменений, то это импортированный проект, нужно синхронизировать все документы
    //
    if (StorageFacade::documentChangeStorage()->isEmpty()) {
        documents = StorageFacade::documentStorage()->documents();
    }
    //
    // А если изменения есть, то синхронизируем только те документы, которые были изменены по факту
    //
    else {
        const auto unsyncedDocuments = StorageFacade::documentChangeStorage()->unsyncedDocuments();
        for (const auto& documentUuid : unsyncedDocuments) {
            auto document = StorageFacade::documentStorage()->document(documentUuid);
            if (document != nullptr) {
                documents.append(document);
            } else {
                const auto unsyncedDocumentChanges
                    = StorageFacade::documentChangeStorage()->unsyncedDocumentChanges(documentUuid);
                for (auto change : unsyncedDocumentChanges) {
                    StorageFacade::documentChangeStorage()->removeDocumentChange(change);
                }
            }
        }
    }
    return documents;
}

void ProjectManager::mergeDocumentInfo(const Domain::DocumentInfo& _documentInfo)
{
    Log::trace("Merge content for document %1", _documentInfo.uuid.toString());

    const auto syncDatetime = QDateTime::currentDateTimeUtc();

    Domain::DocumentObject* document = nullptr;
    const auto documentType = static_cast<Domain::DocumentObjectType>(_documentInfo.type);
    switch (documentType) {
    //
    // Изображения обрабатываем строго через отдельное хранилище
    //
    case Domain::DocumentObjectType::ImageData: {
        d->documentImageStorage.save(_documentInfo.uuid, _documentInfo.content);
        return;
    }

    //
    // Модели для документов, которые могут быть только в единственном экземпляре, достаём по
    // типу и применяем к ним помимо содержимого также и идентификатор с облака
    //
    case Domain::DocumentObjectType::Structure: {
        //
        // Сохраним состояние структуры на данный момент, чтобы не происходило перескока на другой
        // документ (который был открыт при старте сессии) при её синхронизации
        //
        setSettingsValue(DataStorageLayer::projectStructureKey(d->projectPath),
                         d->navigator->saveState());
        Q_FALLTHROUGH();
    }
    case Domain::DocumentObjectType::RecycleBin:
    case Domain::DocumentObjectType::Project:
    case Domain::DocumentObjectType::ScreenplayDictionaries:
    case Domain::DocumentObjectType::Characters:
    case Domain::DocumentObjectType::Locations: {
        document = DataStorageLayer::StorageFacade::documentStorage()->document(documentType);
        DataStorageLayer::StorageFacade::documentStorage()->updateDocumentUuid(document->uuid(),
                                                                               _documentInfo.uuid);
        break;
    }

    //
    // Остальные документы берём строго по UUID из базы, а в случае, если их нет, то создаём
    //
    default: {
        document = DataStorageLayer::StorageFacade::documentStorage()->document(_documentInfo.uuid);
        if (document == nullptr) {
            document = DataStorageLayer::StorageFacade::documentStorage()->createDocument(
                _documentInfo.uuid, documentType);
        }
        break;
    }
    }

    //
    // Загрузим модель для документа
    //
    auto documentModel = documentType == Domain::DocumentObjectType::Structure
        ? d->projectStructureModel
        : d->modelsFacade.modelFor(document);

    //
    // Если есть локальные несинхронизированные изменения данного документа, сохраним копию контента
    //
    QByteArray lastDocumentVersion;
    const auto unsyncedChanges
        = DataStorageLayer::StorageFacade::documentChangeStorage()->unsyncedDocumentChanges(
            document->uuid());
    if (!unsyncedChanges.isEmpty()) {
        lastDocumentVersion = document->content();
    }

    //
    // Загружаем в документ полученный контент и изменения
    //
    QVector<QByteArray> changes;
    for (const auto& change : _documentInfo.changes) {
        changes.append(change.redoPatch);
    }
    documentModel->mergeDocumentChanges(_documentInfo.content, changes);

    //
    // Пробуем накатить несинхронизированные изменения
    //
    if (!unsyncedChanges.isEmpty()) {
        changes.clear();
        for (const auto& change : unsyncedChanges) {
            changes.append(change->redoPatch());
        }
        const auto isChangesMerged = documentModel->mergeDocumentChanges({}, changes);
        //
        // ... если успех - пушим несинхронизированные изменения
        //
        if (isChangesMerged) {
            //
            // Симулируем изменение модели, чтобы приложение запушило несинхронизированные изменения
            //
            emit contentsChanged(documentModel);
        }
        //
        // ... в противном случае - создаём отдельную версию с документом содержащим конфликт и
        //     дропаем офлайновые правки из базы
        //
        else {
            //
            // Если было много изменений, то создаём версию документа с конфликтом
            //
            if (unsyncedChanges.size() > 4) {
                const auto item = d->projectStructureModel->itemForIndex(
                    d->projectStructureProxyModel->mapToSource(d->navigator->currentIndex()));
                Q_ASSERT(item);
                const auto versionName = QString("%1 [%2]").arg(
                    tr("Conflicted version"),
                    QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
                const auto color = Qt::red;
                const auto readOnly = false;
                d->projectStructureModel->addItemVersion(item, versionName, color, readOnly,
                                                         lastDocumentVersion);
                d->view.active->setDocumentDraft(item->versions());
            }

            //
            // Удаляем локальные изменения, которые привели к конфликту
            //
            for (auto change : unsyncedChanges) {
                DataStorageLayer::StorageFacade::documentChangeStorage()->removeDocumentChange(
                    change);
            }
        }
    }

    //
    // Дополнительные действия для полноты модели
    //
    switch (documentType) {
    case Domain::DocumentObjectType::Characters: {
        //
        // Берём элементы из модели структуры проекта, там они отсортированы в правильном порядке
        //
        const auto charactersItem
            = d->projectStructureModel->itemForType(Domain::DocumentObjectType::Characters);
        auto charactersModel = static_cast<BusinessLayer::CharactersModel*>(documentModel);
        for (int index = 0; index < charactersItem->childCount(); ++index) {
            auto characterModel = d->modelsFacade.modelFor(charactersItem->childAt(index)->uuid());
            charactersModel->addCharacterModel(
                qobject_cast<BusinessLayer::CharacterModel*>(characterModel));
        }
        break;
    }

    case Domain::DocumentObjectType::Character: {
        static_cast<BusinessLayer::CharactersModel*>(
            d->modelsFacade.modelFor(Domain::DocumentObjectType::Characters))
            ->addCharacterModel(static_cast<BusinessLayer::CharacterModel*>(documentModel));
        break;
    }

    case Domain::DocumentObjectType::Locations: {
        //
        // Берём элементы из модели структуры проекта, там они отсортированы в правильном порядке
        //
        const auto locationsItem
            = d->projectStructureModel->itemForType(Domain::DocumentObjectType::Locations);
        auto locationsModel = static_cast<BusinessLayer::LocationsModel*>(documentModel);
        for (int index = 0; index < locationsItem->childCount(); ++index) {
            auto characterModel = d->modelsFacade.modelFor(locationsItem->childAt(index)->uuid());
            locationsModel->addLocationModel(
                qobject_cast<BusinessLayer::LocationModel*>(characterModel));
        }
        break;
    }

    case Domain::DocumentObjectType::Location: {
        static_cast<BusinessLayer::LocationsModel*>(
            d->modelsFacade.modelFor(Domain::DocumentObjectType::Locations))
            ->addLocationModel(static_cast<BusinessLayer::LocationModel*>(documentModel));
        break;
    }

    case Domain::DocumentObjectType::Worlds: {
        //
        // Берём элементы из модели структуры проекта, там они отсортированы в правильном порядке
        //
        const auto worldsItem
            = d->projectStructureModel->itemForType(Domain::DocumentObjectType::Worlds);
        auto worldsModel = static_cast<BusinessLayer::WorldsModel*>(documentModel);
        for (int index = 0; index < worldsItem->childCount(); ++index) {
            auto characterModel = d->modelsFacade.modelFor(worldsItem->childAt(index)->uuid());
            worldsModel->addWorldModel(qobject_cast<BusinessLayer::WorldModel*>(characterModel));
        }
        break;
    }

    case Domain::DocumentObjectType::World: {
        static_cast<BusinessLayer::WorldsModel*>(
            d->modelsFacade.modelFor(Domain::DocumentObjectType::Worlds))
            ->addWorldModel(static_cast<BusinessLayer::WorldModel*>(documentModel));
        break;
    }

    default:
        break;
    }

    //
    // Помечаем документ синхронизированным и сохраняем
    //
    document->setSyncedAt(syncDatetime);
    DataStorageLayer::StorageFacade::documentStorage()->saveDocument(document);

    //
    // Если обновилась структура, восстановим последний выделенный элемент
    //
    if (document->type() == Domain::DocumentObjectType::Structure) {
        d->view.activeIndex = {};
        restoreCurrentProjectState(d->projectPath);
    }
    //
    // Если не структура, а какая-либо из моделей, то обновим представление для неё
    //
    else {
        const auto item = d->projectStructureModel->itemForIndex(
            d->projectStructureProxyModel->mapToSource(d->navigator->currentIndex()));
        if (item != nullptr) {
            if (d->view.active->currentDraft() == 0 && item->uuid() == _documentInfo.uuid) {
                showView(d->navigator->currentIndex(), d->view.activeViewMimeType);
            } else if (const auto versionItem
                       = item->versions().value(d->view.active->currentDraft() - 1);
                       versionItem != nullptr && versionItem->uuid() == _documentInfo.uuid) {
                showViewForVersion(versionItem);
            }
        }
    }

    //
    // TODO: Вынести в отдельный поток применение массовых изменений? либо как-то уведомлять
    //       пользователя о том, что сейчас идёт синхронизация данных проекта
    //
    if (d->mergedDocuments == 10) {
        d->mergedDocuments = 1;
        QApplication::processEvents();
    } else {
        ++d->mergedDocuments;
    }
}

void ProjectManager::applyDocumentChanges(const Domain::DocumentInfo& _documentInfo)
{
    //
    // Модели для документов, которые могут быть только в единственном экземпляре, достаём по типу и
    // применяем к ним помимо содержимого также и идентификатор с облака
    //
    auto document
        = DataStorageLayer::StorageFacade::documentStorage()->document(_documentInfo.uuid);
    if (document == nullptr) {
        return;
    }

    const auto documentType = static_cast<Domain::DocumentObjectType>(_documentInfo.type);

    //
    // Если прилетело обновление картинки, сохраним его
    //
    if (documentType == Domain::DocumentObjectType::ImageData) {
        d->documentImageStorage.save(_documentInfo.uuid, _documentInfo.content);
        return;
    }

    //
    // В остальных случаях загрузим модель для документа
    //
    auto documentModel = documentType == Domain::DocumentObjectType::Structure
        ? d->projectStructureModel
        : d->modelsFacade.modelFor(document);

    //
    // Применяем полученные изменения к модели
    //
    QVector<QByteArray> changes;
    for (const auto& change : _documentInfo.changes) {
        changes.append(change.redoPatch);
    }
    documentModel->applyDocumentChanges(changes);
}

QVector<Domain::DocumentChangeObject*> ProjectManager::unsyncedChanges(
    const QUuid& _documentUuid) const
{
    const auto unsyncedChanges
        = DataStorageLayer::StorageFacade::documentChangeStorage()->unsyncedDocumentChanges(
            _documentUuid);
    d->changesForSync[_documentUuid] = unsyncedChanges;
    return unsyncedChanges;
}

void ProjectManager::markChangesSynced(const QUuid& _documentUuid)
{
    const auto changes = d->changesForSync[_documentUuid];
    for (auto change : changes) {
        change->setSynced(true);
        DataStorageLayer::StorageFacade::documentChangeStorage()->updateDocumentChange(change);
    }
    d->changesForSync.remove(_documentUuid);
}

void ProjectManager::planDocumentSyncing(const QUuid& _documentUuid)
{
    const auto documentToSyncTimer = d->documentToSyncTimer.find(_documentUuid);
    if (documentToSyncTimer != d->documentToSyncTimer.end()) {
        return;
    }

    auto& timer = d->documentToSyncTimer[_documentUuid];
    timer.reset(new QTimer(this));
    timer->setSingleShot(true);
    timer->setInterval(std::chrono::minutes{ 1 });
    connect(timer.data(), &QTimer::timeout, this, [this, _documentUuid] {
        //
        // Убираем таймер
        //
        d->documentToSyncTimer.remove(_documentUuid);

        //
        // Если документ был удалён, пока мы ждали, не выполняем никаких запросов для него
        //
        if (DataStorageLayer::StorageFacade::documentStorage()->document(_documentUuid)
            == nullptr) {
            return;
        }

        //
        // Отправляем запрос на синхронизацию
        //
        emit uploadDocumentRequested(_documentUuid, false);
    });
    timer->start();
}

Domain::DocumentObject* ProjectManager::documentToSync(const QUuid& _documentUuid) const
{
    //
    // Возвращаем документ
    //
    return DataStorageLayer::StorageFacade::documentStorage()->document(_documentUuid);
}

QVector<QUuid> ProjectManager::documentBundle(const QUuid& _documentUuid) const
{
    const auto item = d->projectStructureModel->itemForUuid(_documentUuid);
    if (item == nullptr) {
        //
        // Если документ не найден в структуре, то это запрос generic-объекта (картинки),
        // поэтому возвращаем только его
        //
        return { _documentUuid };
    }

    auto topLevelParent = item->parent();
    while (topLevelParent->parent() != nullptr) {
        topLevelParent = topLevelParent->parent();
    }

    QVector<QUuid> documents;
    switch (item->type()) {
    case Domain::DocumentObjectType::Characters:
    case Domain::DocumentObjectType::Locations: {
        //
        // Сначала дети, потом сам документ
        //
        for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
            documents.append(item->childAt(childIndex)->uuid());
        }
        documents.append(_documentUuid);
        break;
    }

    case Domain::DocumentObjectType::ScreenplayTitlePage:
    case Domain::DocumentObjectType::ScreenplaySynopsis:
    case Domain::DocumentObjectType::ScreenplayTreatment:
    case Domain::DocumentObjectType::ScreenplayText:
    case Domain::DocumentObjectType::ScreenplayStatistics: {
        //
        // Локации
        //
        for (int index = 0; index < topLevelParent->childCount(); ++index) {
            auto childItem = topLevelParent->childAt(index);
            if (childItem->type() == Domain::DocumentObjectType::Locations) {
                //
                // ... каждая из локаций
                //
                for (int index = 0; index < childItem->childCount(); ++index) {
                    documents.append(childItem->childAt(index)->uuid());
                }
                //
                // ... сам группирующий документ
                //
                documents.append(childItem->uuid());
                break;
            }
        }

        Q_FALLTHROUGH();
    }

    case Domain::DocumentObjectType::AudioplayTitlePage:
    case Domain::DocumentObjectType::AudioplaySynopsis:
    case Domain::DocumentObjectType::AudioplayText:
    case Domain::DocumentObjectType::AudioplayStatistics:
    case Domain::DocumentObjectType::ComicBookSynopsis:
    case Domain::DocumentObjectType::ComicBookTitlePage:
    case Domain::DocumentObjectType::ComicBookText:
    case Domain::DocumentObjectType::ComicBookStatistics:
    case Domain::DocumentObjectType::StageplayTitlePage:
    case Domain::DocumentObjectType::StageplaySynopsis:
    case Domain::DocumentObjectType::StageplayText:
    case Domain::DocumentObjectType::StageplayStatistics:
    case Domain::DocumentObjectType::NovelTitlePage:
    case Domain::DocumentObjectType::NovelSynopsis:
    case Domain::DocumentObjectType::NovelOutline:
    case Domain::DocumentObjectType::NovelText:
    case Domain::DocumentObjectType::NovelStatistics: {
        //
        // Персонажи
        //
        for (int index = 0; index < topLevelParent->childCount(); ++index) {
            auto childItem = topLevelParent->childAt(index);
            if (childItem->type() == Domain::DocumentObjectType::Characters) {
                //
                // ... каждый из персонажей
                //
                for (int index = 0; index < childItem->childCount(); ++index) {
                    documents.append(childItem->childAt(index)->uuid());
                }
                //
                // ... сам группирующий документ
                //
                documents.append(childItem->uuid());
                break;
            }
        }

        //
        // Полный комплект
        //
        documents.append(item->parent()->uuid());
        for (int index = 0; index < item->parent()->childCount(); ++index) {
            auto childItem = item->parent()->childAt(index);
            documents.append(childItem->uuid());
        }
        //
        // Версии документов из комплекта добавляем вручную
        //
        if (!documents.contains(_documentUuid)) {
            documents.append(_documentUuid);
        }
        break;
    }

    default: {
        //
        // Для обычных документов загружаем лишь сам документ
        //
        documents.append(_documentUuid);
        break;
    }
    }

    return documents;
}

void ProjectManager::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    //
    // Отобразить список активных соавторов
    //
    // ... но сперва фильтранём всех
    //
    const auto showThreshold = QDateTime::currentDateTime().addSecs(-1 * 60);
    for (auto cursorIter = d->collaboratorsCursors.begin();
         cursorIter != d->collaboratorsCursors.end();) {
        if (cursorIter->updatedAt < showThreshold) {
            cursorIter = d->collaboratorsCursors.erase(cursorIter);
        } else {
            ++cursorIter;
        }
    }
    //
    // ... добавим новопришедших
    //
    for (const auto& cursor : _cursors) {
        //
        // ... при этом дополнительно проверяем, не сидит ли там кто-то, кто уже не актуален
        //
        if (cursor.updatedAt < showThreshold) {
            continue;
        }

        d->collaboratorsCursors[cursor.cursorId] = cursor;
    }
    //
    // ... и обновим
    //
    const QVector<Domain::CursorInfo> activeCursors(d->collaboratorsCursors.begin(),
                                                    d->collaboratorsCursors.end());
    d->collaboratorsToolBar->setCollaborators(activeCursors);

    //
    // Устновим курсоры в открытые редакторы
    //
    if (d->view.active == d->view.left) {
        d->pluginsBuilder.setViewCursors(activeCursors, d->view.activeViewMimeType);
        d->pluginsBuilder.setSecondaryViewCursors(activeCursors, d->view.inactiveViewMimeType);
    } else {
        d->pluginsBuilder.setSecondaryViewCursors(activeCursors, d->view.activeViewMimeType);
        d->pluginsBuilder.setViewCursors(activeCursors, d->view.inactiveViewMimeType);
    }

    //
    // Если есть соавторы, то запланируем принудительное обновление их курсоров
    //
    if (!d->collaboratorsCursors.isEmpty()) {
        d->collaboratorsUpdateDebouncer.orderWork();
    }
}

void ProjectManager::clearCursors()
{
    d->collaboratorsToolBar->setCollaborators({});
    d->collaboratorsCursors.clear();
}

void ProjectManager::setAvailableCredits(int _credits)
{
    d->pluginsBuilder.setAvailableCredits(_credits);
}

void ProjectManager::setRephrasedText(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setRephrasedText(_text);
    }
}

void ProjectManager::setExpandedText(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setExpandedText(_text);
    }
}

void ProjectManager::setShortenedText(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setShortenedText(_text);
    }
}

void ProjectManager::setInsertedText(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setInsertedText(_text);
    }
}

void ProjectManager::setSummarizeedText(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setSummarizedText(_text);
    }
}

void ProjectManager::setTranslatedText(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setTranslatedText(_text);
    }
}

void ProjectManager::setGeneratedSynopsis(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setGeneratedSynopsis(_text);
    }
}

void ProjectManager::setGeneratedText(const QString& _text)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setGeneratedText(_text);
    }
}

void ProjectManager::setGeneratedImage(const QPixmap& _image)
{
    auto view = d->activeDocumentView();
    if (view != nullptr) {
        view->setGeneratedImage(_image);
    }
}

QString ProjectManager::projectName() const
{
    const auto projectInformationModel = qobject_cast<BusinessLayer::ProjectInformationModel*>(
        d->modelsFacade.modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Project)));
    return projectInformationModel->name();
}

QString ProjectManager::projectLogline() const
{
    const auto projectInformationModel = qobject_cast<BusinessLayer::ProjectInformationModel*>(
        d->modelsFacade.modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Project)));
    return projectInformationModel->logline();
}

QPixmap ProjectManager::projectCover() const
{
    const auto projectInformationModel = qobject_cast<BusinessLayer::ProjectInformationModel*>(
        d->modelsFacade.modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(
            Domain::DocumentObjectType::Project)));
    return projectInformationModel->cover();
}

bool ProjectManager::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::IdleEvent): {
        //
        // Выполняем отложенную работу для всех моделей, где она имеет место быть
        //
        // ... для сценариев корректируем список подсказок имён персонажей
        //
        auto models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::ScreenplayText);
        for (auto model : std::as_const(models)) {
            auto screenplay = qobject_cast<BusinessLayer::ScreenplayTextModel*>(model);
            screenplay->updateRuntimeDictionariesIfNeeded();
        }
        //
        models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::ComicBookText);
        for (auto model : std::as_const(models)) {
            auto comicBook = qobject_cast<BusinessLayer::ComicBookTextModel*>(model);
            comicBook->updateRuntimeDictionariesIfNeeded();
        }
        //
        models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::AudioplayText);
        for (auto model : std::as_const(models)) {
            auto audioplay = qobject_cast<BusinessLayer::AudioplayTextModel*>(model);
            audioplay->updateRuntimeDictionariesIfNeeded();
        }
        //
        models = d->modelsFacade.loadedModelsFor(Domain::DocumentObjectType::StageplayText);
        for (auto model : std::as_const(models)) {
            auto stageplay = qobject_cast<BusinessLayer::StageplayTextModel*>(model);
            stageplay->updateRuntimeDictionariesIfNeeded();
        }
        break;
    }

    case static_cast<QEvent::Type>(EventType::FocusChangeEvent): {
        //
        // При смене сфокусированного виджета, проверяем не изменился ли активный из редакторов
        //

        if (!d->view.right->isVisible()) {
            break;
        }

        auto targetWidget = QApplication::focusWidget();
        if (targetWidget == nullptr) {
            break;
        }

        do {
            if (targetWidget == d->view.left) {
                if (d->view.active != d->view.left) {
                    d->switchViews();
                }
                break;
            } else if (targetWidget == d->view.right) {
                if (d->view.active != d->view.right) {
                    d->switchViews();
                }
                break;
            }

            targetWidget = targetWidget->parentWidget();
        } while (targetWidget != nullptr);
        break;
    }

    default:
        break;
    }

    return QObject::event(_event);
}

bool ProjectManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if (static_cast<EventType>(_event->type()) == EventType::DesignSystemChangeEvent
        && _watched == d->view.active) {
        for (const auto& window : std::as_const(d->view.windows)) {
            QCoreApplication::sendEvent(window.view->asQWidget(), _event);
        }
    } else if (_event->type() == QEvent::LanguageChange) {
        d->updateOptionsText();
    }

    return QObject::eventFilter(_watched, _event);
}

void ProjectManager::handleModelChange(BusinessLayer::AbstractModel* _model,
                                       const QByteArray& _undo, const QByteArray& _redo)
{
    using namespace DataStorageLayer;
    StorageFacade::documentChangeStorage()->appendDocumentChange(
        _model->document()->uuid(), QUuid::createUuid(), _undo, _redo,
        StorageFacade::settingsStorage()->accountName(),
        StorageFacade::settingsStorage()->accountEmail());

    emit contentsChanged(_model);
}

void ProjectManager::undoModelChange(BusinessLayer::AbstractModel* _model, int _undoStep)
{
    const auto change = DataStorageLayer::StorageFacade::documentChangeStorage()->documentChangeAt(
        _model->document()->uuid(), _undoStep);
    if (change == nullptr) {
        return;
    }

    _model->undoChange(change->undoPatch(), change->redoPatch());
}

void ProjectManager::showView(const QModelIndex& _index)
{
    d->toolBar->clearViews();

    if (!_index.isValid()) {
        d->view.active->showDefaultPage();
        return;
    }

    //
    // Определим выделенный элемент и скорректируем интерфейс
    //
    const auto mappedIndex = d->projectStructureProxyModel->mapToSource(_index);
    const auto item = d->projectStructureModel->itemForIndex(mappedIndex);
    const auto documentMimeType = Domain::mimeTypeFor(item->type());
    //
    // ... настроим иконки представлений
    //
    const auto views = d->pluginsBuilder.editorsInfoFor(documentMimeType, d->isProjectRemote);
    for (const auto& view : views) {
        const auto tooltip = d->pluginsBuilder.editorDescription(documentMimeType, view.mimeType);
        const bool isActive = view.mimeType == views.first().mimeType;
        d->toolBar->addView(view.mimeType, view.icon, tooltip, isActive);
    }

    //
    // Откроем документ на редактирование в первом из представлений
    //
    if (views.isEmpty()) {
        d->view.active->showDocumentLoadingPage();
        return;
    }
    showView(_index, {}, views.first().mimeType);
}

void ProjectManager::showView(const QModelIndex& _itemIndex, const QString& _viewMimeType,
                              const QString& _defaultMimeType)
{
    Log::info("Activate plugin \"%1\" (default \"%2\")", _viewMimeType, _defaultMimeType);

    const auto sourceItemIndex = d->projectStructureProxyModel->mapToSource(_itemIndex);

    //
    // Сохранить состояние представления текущего документа
    //
    if (!d->view.activeModel.isNull() && d->view.activeModel->document() != nullptr) {

        const auto previousActiveAliasedItem = d->aliasedItemForIndex(d->view.activeIndex);
        setSettingsValue(documentSettingsKey(previousActiveAliasedItem->uuid(), kCurrentVersionKey),
                         d->view.active->currentDraft());
    }

    if (!_itemIndex.isValid()) {
        updateCurrentDocument(nullptr, {});
        d->view.active->setDraftsVisible(false);
        d->view.active->showDefaultPage();
        return;
    }

    const auto sourceItem = d->projectStructureModel->itemForIndex(sourceItemIndex);
    auto aliasedItem = d->aliasedItemForIndex(sourceItemIndex);
    auto itemForShow = aliasedItem;
    if (const auto versionIndex
        = settingsValue(documentSettingsKey(aliasedItem->uuid(), kCurrentVersionKey), 0).toInt()
            - 1;
        versionIndex != -1 && !aliasedItem->versions().isEmpty()
        && versionIndex < aliasedItem->versions().size()) {
        itemForShow = aliasedItem->versions().at(versionIndex);
    }
    emit downloadDocumentRequested(itemForShow->uuid());

    //
    // Определим редактор для отображения
    //
    QString viewMimeType = _viewMimeType;
    if (viewMimeType.isEmpty()) {
        viewMimeType
            = settingsValue(documentSettingsKey(sourceItem->uuid(), kCurrentViewMimeTypeKey),
                            _defaultMimeType)
                  .toString();
    }
    d->toolBar->setCurrentViewMimeType(viewMimeType);

    //
    // ... сохраним последний используемый с данным документом редактор
    //
    setSettingsValue(documentSettingsKey(sourceItem->uuid(), kCurrentViewMimeTypeKey),
                     viewMimeType);

    //
    // Определим модель
    //
    updateCurrentDocument(d->modelsFacade.modelFor(itemForShow->uuid()), viewMimeType);
    if (d->view.activeModel == nullptr) {
        d->view.active->showDocumentLoadingPage();
        d->view.active->setDraftsVisible(false);
        return;
    }

    //
    // Определим представление и отобразим
    //
    Log::debug("Activate plugin view");
    Ui::IDocumentView* view = nullptr;
    const bool isPrimaryView = d->view.active == d->view.left;
    if (isPrimaryView) {
        view = d->pluginsBuilder.activateView(viewMimeType, d->view.activeModel);
    } else {
        view = d->pluginsBuilder.activateSecondView(viewMimeType, d->view.activeModel);
    }
    if (view == nullptr) {
        d->view.active->showNotImplementedPage();
        d->view.active->setDraftsVisible(false);
        return;
    }
    Log::trace("Set project info");
    view->setProjectInfo(d->isProjectRemote, d->isProjectOwner, d->allowGrantAccessToProject);
    Log::trace("Set editing mode");
    view->setEditingMode(d->documentEditingMode(itemForShow));
    Log::trace("Set view cursors");
    view->setCursors({ d->collaboratorsCursors.begin(), d->collaboratorsCursors.end() });
    Log::trace("Set document versions");
    d->view.active->setDocumentDraft(aliasedItem->versions());
    Log::trace("Show view");
    d->view.active->showEditor(view->asQWidget());
    d->view.activeIndex = sourceItemIndex;

    //
    // Связываем редакторы
    //
    d->pluginsBuilder.syncModelAndBindEditors(viewMimeType, d->view.activeModel, isPrimaryView);

    //
    // Устанавливаем опции редактора
    //
    Log::debug("Activate plugin view options");
    d->toolBar->setOptions(view->options(), AppBarOptionsLevel::View);

    //
    // Настроим уведомления редактора
    //
    if (auto viewWidget = view->asQWidget(); viewWidget != nullptr) {
        const auto invalidSignalIndex = -1;
        if (viewWidget->metaObject()->indexOfSignal("cursorChanged(QByteArray)")
            != invalidSignalIndex) {
            connect(viewWidget, SIGNAL(cursorChanged(QByteArray)), this,
                    SLOT(notifyCursorChanged(QByteArray)), Qt::UniqueConnection);
        }
    }

    //
    // Настроим возможность перехода в навигатор
    //
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(viewMimeType);
    d->projectStructureModel->setNavigatorAvailableFor(sourceItemIndex,
                                                       !navigatorMimeType.isEmpty());

    //
    // Если в данный момент отображён кастомный навигатор, откроем навигатор соответствующий
    // редактору
    //
    if (!d->navigator->isProjectNavigatorShown() && d->view.active == d->view.left) {
        showNavigator(_itemIndex, viewMimeType);
    }

    //
    // Установим видимость панели драфтов
    //
    d->view.active->setDraftsVisible(aliasedItem->versions().count() > 0);

    //
    // Настроим уведомления плагина
    //
    Log::debug("Activate plugin manager");
    if (auto documentManager = d->pluginsBuilder.plugin(viewMimeType)->asQObject();
        documentManager != nullptr) {
        const auto invalidSignalIndex = -1;
        if (documentManager->metaObject()->indexOfSignal("upgradeToProRequested()")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(upgradeToProRequested()), this,
                    SIGNAL(upgradeToProRequested()), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("upgradeToTeamRequested()")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(upgradeToTeamRequested()), this,
                    SIGNAL(upgradeToTeamRequested()), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("buyCreditsRequested()")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(buyCreditsRequested()), this,
                    SIGNAL(buyCreditsRequested()), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("linkActivated(QUuid,QModelIndex,QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(linkActivated(QUuid, QModelIndex, QString)), this,
                    SLOT(activateLink(QUuid, QModelIndex, QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("rephraseTextRequested(QString,QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(rephraseTextRequested(QString, QString)), this,
                    SIGNAL(rephraseTextRequested(QString, QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("expandTextRequested(QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(expandTextRequested(QString)), this,
                    SIGNAL(expandTextRequested(QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("shortenTextRequested(QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(shortenTextRequested(QString)), this,
                    SIGNAL(shortenTextRequested(QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("insertTextRequested(QString,QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(insertTextRequested(QString, QString)), this,
                    SIGNAL(insertTextRequested(QString, QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("summarizeTextRequested(QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(summarizeTextRequested(QString)), this,
                    SIGNAL(summarizeTextRequested(QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal("translateTextRequested(QString,QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(translateTextRequested(QString, QString)), this,
                    SIGNAL(translateTextRequested(QString, QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal(
                "generateSynopsisRequested(QVector<QString>,int,int)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(generateSynopsisRequested(QVector<QString>, int, int)),
                    this, SIGNAL(generateSynopsisRequested(QVector<QString>, int, int)),
                    Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal(
                "generateNovelRequested(QVector<QString>,int)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(generateNovelRequested(QVector<QString>, int)), this,
                    SIGNAL(generateNovelRequested(QVector<QString>, int)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal(
                "generateScriptRequested(QVector<QString>,int)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(generateScriptRequested(QVector<QString>, int)), this,
                    SIGNAL(generateScriptRequested(QVector<QString>, int)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal(
                "generateTextRequested(QString,QString,QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(generateTextRequested(QString, QString, QString)), this,
                    SIGNAL(generateTextRequested(QString, QString, QString)), Qt::UniqueConnection);
        }
        if (documentManager->metaObject()->indexOfSignal(
                "generateImageRequested(QString,QString,QString)")
            != invalidSignalIndex) {
            connect(documentManager, SIGNAL(generateImageRequested(QString, QString, QString)),
                    this, SIGNAL(generateImageRequested(QString, QString, QString)),
                    Qt::UniqueConnection);
        }
    }

    //
    // Выберем нужную версию документа
    //
    d->view.active->setCurrentDraft(
        settingsValue(documentSettingsKey(aliasedItem->uuid(), kCurrentVersionKey), 0).toInt());

    //
    // Фокусируем представление
    //
    QTimer::singleShot(d->view.active->animationDuration() * 1.3, this, [this] {
        if (auto editor = d->view.active->currentEditor(); editor != nullptr) {
            editor->setFocus();
        }
    });

    Log::info("Plugin activated");
}

void ProjectManager::showViewForVersion(BusinessLayer::StructureModelItem* _item)
{
    emit downloadDocumentRequested(_item->uuid());

    const auto viewMimeType = d->view.activeViewMimeType;

    //
    // Определим модель
    //
    updateCurrentDocument(d->modelsFacade.modelFor(_item->uuid()), viewMimeType);
    if (d->view.activeModel == nullptr) {
        d->view.active->showDocumentLoadingPage();
        return;
    }

    //
    // Определим представление и отобразим
    //
    Ui::IDocumentView* view = nullptr;
    if (d->view.active == d->view.left) {
        view = d->pluginsBuilder.activateView(viewMimeType, d->view.activeModel);
    } else {
        view = d->pluginsBuilder.activateSecondView(viewMimeType, d->view.activeModel);
    }
    if (view == nullptr) {
        d->view.active->showNotImplementedPage();
        return;
    }
    view->setEditingMode(d->documentEditingMode(_item));
    view->setCursors({ d->collaboratorsCursors.begin(), d->collaboratorsCursors.end() });
    d->view.active->showEditor(view->asQWidget());

    //
    // Если в данный момент отображён кастомный навигатов, откроем навигатор соответствующий
    // редактору
    //
    if (!d->navigator->isProjectNavigatorShown() && d->view.active == d->view.left) {
        showNavigatorForVersion(_item);
    }
}

void ProjectManager::activateView(const QModelIndex& _itemIndex, const QString& _viewMimeType)
{
    d->toolBar->clearViews();

    if (!_itemIndex.isValid()) {
        return;
    }

    //
    // Определим выделенный элемент и скорректируем интерфейс
    //
    const auto mappedIndex = d->projectStructureProxyModel->mapToSource(_itemIndex);
    const auto item = d->projectStructureModel->itemForIndex(mappedIndex);
    const auto documentMimeType = Domain::mimeTypeFor(item->type());
    //
    // ... настроим иконки представлений
    //
    const auto views = d->pluginsBuilder.editorsInfoFor(documentMimeType, d->isProjectRemote);
    for (const auto& view : views) {
        const auto tooltip = d->pluginsBuilder.editorDescription(documentMimeType, view.mimeType);
        const bool isActive = view.mimeType == views.first().mimeType;
        d->toolBar->addView(view.mimeType, view.icon, tooltip, isActive);
    }

    //
    // Определим редактор для отображения
    //
    d->toolBar->setCurrentViewMimeType(_viewMimeType);

    //
    // Определим представление и отобразим
    //
    Ui::IDocumentView* view = nullptr;
    if (d->view.active == d->view.left) {
        view = d->pluginsBuilder.activateView(_viewMimeType, d->view.activeModel);
    } else {
        view = d->pluginsBuilder.activateSecondView(_viewMimeType, d->view.activeModel);
    }
    if (view == nullptr) {
        return;
    }

    d->toolBar->setOptions(view->options(), AppBarOptionsLevel::View);
}

void ProjectManager::showNavigator(const QModelIndex& _itemIndex, const QString& _viewMimeType)
{
    const auto sourceItemIndex = d->projectStructureProxyModel->mapToSource(_itemIndex);
    if (!sourceItemIndex.isValid()) {
        d->navigator->showProjectNavigator();
        return;
    }

    auto item = d->aliasedItemForIndex(sourceItemIndex);
    if (int versionIndex = d->view.active->currentDraft() - 1;
        versionIndex != -1 && item->versions().size() > versionIndex) {
        item = item->versions().at(versionIndex);
    }

    //
    // Определим модель
    //
    auto model = d->modelsFacade.modelFor(item->uuid());
    if (model == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Определим представление и отобразим
    //
    const auto viewMimeType
        = !_viewMimeType.isEmpty() ? _viewMimeType : d->toolBar->currentViewMimeType();
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(viewMimeType);
    auto view = d->pluginsBuilder.activateView(navigatorMimeType, model);
    if (view == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Связываем редактор с навигатоом
    //
    d->pluginsBuilder.bind(viewMimeType, navigatorMimeType);

    //
    // Настроим опции редактора
    //
    d->toolBar->setOptions(view->options(), AppBarOptionsLevel::Navigation);

    //
    // Настраиваем возможность перехода к навигатору проекта
    //
    auto navigatorView = qobject_cast<Ui::AbstractNavigator*>(view->asQWidget());
    connect(navigatorView, &Ui::AbstractNavigator::backPressed, d->navigator,
            &Ui::ProjectNavigator::showProjectNavigator, Qt::UniqueConnection);
    connect(navigatorView, &Ui::AbstractNavigator::backPressed, d->toolBar,
            &Ui::ProjectToolBar::clearNavigatorOptions, Qt::UniqueConnection);
    d->navigator->setCurrentWidget(navigatorView);
}

void ProjectManager::showNavigatorForVersion(BusinessLayer::StructureModelItem* _item)
{
    //
    // Определим модель
    //
    auto model = d->modelsFacade.modelFor(_item->uuid());
    if (model == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Определим представление и отобразим
    //
    const auto viewMimeType = d->view.activeViewMimeType;
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(viewMimeType);
    auto view = d->pluginsBuilder.activateView(navigatorMimeType, model);
    if (view == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }
}

void ProjectManager::notifyCursorChanged(const QByteArray& _cursorData)
{
    if (d->view.activeModel == nullptr || d->view.activeModel->document() == nullptr) {
        return;
    }

    const auto widget = qobject_cast<QWidget*>(sender());
    Q_ASSERT(widget);
    if (!widget->isVisible()) {
        return;
    }

    emit cursorChanged(d->view.activeModel->document()->uuid(), _cursorData);
}

void ProjectManager::activateLink(const QUuid& _documentUuid, const QModelIndex& _index,
                                  const QString& _viewMimeType)
{
    const auto item = d->projectStructureModel->itemForUuid(_documentUuid);
    if (item == nullptr) {
        return;
    }

    auto sourceIndex = d->projectStructureModel->indexForItem(item);
    constexpr int invalidVersionIndex = -1;
    int versionIndex = invalidVersionIndex;
    if (!sourceIndex.isValid()) {
        const auto parent = item->parent();
        for (int childIndex = 0; childIndex < parent->childCount(); ++childIndex) {
            const auto childItem = parent->childAt(childIndex);
            if (childItem->versions().contains(item)) {
                sourceIndex = d->projectStructureModel->indexForItem(childItem);
                versionIndex = childItem->versions().indexOf(item) + 1;
                break;
            }
        }

        if (!sourceIndex.isValid() || versionIndex == invalidVersionIndex) {
            return;
        }
    }

    const auto itemIndex = d->projectStructureProxyModel->mapFromSource(sourceIndex);
    if (!itemIndex.isValid()) {
        return;
    }

    //
    // Откроем ссылку на элемент
    //
    const auto withActivation = false;
    //
    // ... если работаем в двухпанельном режиме, то открываем её во второй панели, при этом
    //     активируем вторую панель для установки в неё целевого документа без активации, чтобы это
    //     прошло незаметно для пользователя
    //
    if (d->view.right->isVisible()) {
        d->switchViews(withActivation);
    }
    //
    // ... выберем элемент в навигаторе
    //
    d->navigator->setCurrentIndex(itemIndex);
    //
    // ... определим майм-тип редактора, который нужно открыть
    //
    QString viewMimeType;
    const auto documentMimeType = Domain::mimeTypeFor(item->type());
    const auto views = d->pluginsBuilder.editorsInfoFor(documentMimeType, d->isProjectRemote);
    if (!_viewMimeType.isEmpty()) {
        for (const auto& view : views) {
            if (view.mimeType == _viewMimeType) {
                viewMimeType = view.mimeType;
                break;
            }
        }
    }
    if (viewMimeType.isEmpty()) {
        viewMimeType = views.constFirst().mimeType;
    }
    //
    // ... если в данный момент открыт другой редактор, загрузким запрашиваемый модуль
    //
    if (d->view.activeViewMimeType != viewMimeType) {
        showView(itemIndex, viewMimeType);
    }
    //
    // ... если нужно, активируем заданную версию
    //
    if (versionIndex != invalidVersionIndex) {
        d->view.active->setCurrentDraft(versionIndex);
    }
    //
    // ... если работаем с текущей версией, но редактор находится на другой, возвращаемся к текущей
    //
    else if (d->view.active->currentDraft() > 0) {
        d->view.active->setCurrentDraft(0);
    }
    //
    // ... фокусируем в представлении элемент с заданным индексом
    //
    if (d->view.active == d->view.left) {
        d->pluginsBuilder.setViewCurrentIndex(_index, viewMimeType);
    } else {
        d->pluginsBuilder.setSecondaryViewCurrentIndex(_index, viewMimeType);
    }
    //
    // ... возвращаем активность на исходную панель
    //
    if (d->view.right->isVisible()) {
        d->switchViews(withActivation);
    }
}

void ProjectManager::updateCurrentDocument(BusinessLayer::AbstractModel* _model,
                                           const QString& _viewMimeType)
{
    d->view.activeModel = _model;
    d->view.activeViewMimeType = _viewMimeType;

    emit currentModelChanged(d->view.activeModel);
}

} // namespace ManagementLayer
