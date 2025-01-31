#include "structure_model.h"

#include "structure_model_item.h"

#include <business_layer/model/abstract_model_xml.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>
#include <utils/tools/edit_distance.h>

#include <QColor>
#include <QDataStream>
#include <QDomDocument>
#include <QIODevice>
#include <QMimeData>
#include <QSet>

#ifdef QT_DEBUG
#define XML_CHECKS
#endif


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/document";
const QLatin1String kDocumentKey("document");
const QLatin1String kItemKey("item");
const QLatin1String kVersionKey("version");
const QLatin1String kUuidAttribute("uuid");
const QLatin1String kTypeAttribute("type");
const QLatin1String kNameAttribute("name");
const QLatin1String kColorAttribute("color");
const QLatin1String kVisibleAttribute("visible");
const QLatin1String kReadOnlyAttribute("readonly");
} // namespace

class StructureModel::Implementation
{
public:
    Implementation();

    /**
     * @brief Построить модель структуры из xml хранящегося в документе
     */
    void buildModel(Domain::DocumentObject* _structure);

    /**
     * @brief Построить элемент из заданной ноды xml документа
     */
    StructureModelItem* buildItem(const QDomElement& _node, StructureModelItem* _parent = nullptr);

    /**
     * @brief Сформировать xml из данных модели
     */
    QByteArray toXml(Domain::DocumentObject* _structure) const;


    /**
     * @brief Является ли проект вновь созданным
     */
    bool isNewProject = false;

    /**
     * @brief Название текущего проекта
     */
    QString projectName;

    /**
     * @brief Корневой элемент дерева
     */
    StructureModelItem* rootItem = nullptr;

    /**
     * @brief Последние положенные в майм элементы
     */
    mutable QVector<StructureModelItem*> lastMimeItems;

    /**
     * @brief Список индексов для которых доступен навигатор
     */
    QSet<QModelIndex> navigatorAvailableIndexes;
};

StructureModel::Implementation::Implementation()
    : rootItem(
        new StructureModelItem({}, Domain::DocumentObjectType::Undefined, {}, {}, true, false))
{
}

void StructureModel::Implementation::buildModel(Domain::DocumentObject* _structure)
{
    if (_structure == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(_structure->content());
    auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto itemNode = documentNode.firstChildElement();
    while (!itemNode.isNull()) {
        buildItem(itemNode, rootItem);
        itemNode = itemNode.nextSiblingElement();
    }
}

StructureModelItem* StructureModel::Implementation::buildItem(const QDomElement& _node,
                                                              StructureModelItem* _parent)
{
    //
    // Формируем элемент структуры
    //
    const auto readOnly = false;
    auto item = new StructureModelItem(QUuid::fromString(_node.attribute(kUuidAttribute)),
                                       Domain::typeFor(_node.attribute(kTypeAttribute).toUtf8()),
                                       TextHelper::fromHtmlEscaped(_node.attribute(kNameAttribute)),
                                       ColorHelper::fromString(_node.attribute(kColorAttribute)),
                                       _node.attribute(kVisibleAttribute) == "true", readOnly);
    //
    // ... вкладываем в родителя
    //
    if (_parent != nullptr) {
        _parent->appendItem(item);
    }

    //
    // ... определяем его
    //
    auto child = _node.firstChildElement();
    while (!child.isNull()) {
        //
        // ... детей
        //
        if (child.tagName() == kItemKey) {
            buildItem(child, item);
        }
        //
        // ... и версии
        //
        else if (child.tagName() == kVersionKey) {
            const auto versionNode = child.toElement();
            const auto visible = true;
            const auto readOnly = versionNode.hasAttribute(kReadOnlyAttribute)
                && versionNode.attribute(kReadOnlyAttribute) == "true";
            auto version = new StructureModelItem(
                QUuid::fromString(versionNode.attribute(kUuidAttribute)), item->type(),
                TextHelper::fromHtmlEscaped(versionNode.attribute(kNameAttribute)),
                ColorHelper::fromString(versionNode.attribute(kColorAttribute)), visible, readOnly);
            item->addVersion(version);
        }
        child = child.nextSiblingElement();
    }

    return item;
}

QByteArray StructureModel::Implementation::toXml(Domain::DocumentObject* _structure) const
{
    if (_structure == nullptr) {
        return {};
    }

    //
    // TODO: Продумать, как оптимизировать формирование xml
    //

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(_structure->type()))
               .toUtf8();
    auto writeVersionXml = [&xml](StructureModelItem* _item) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\" %8=\"%9\"/>\n")
                   .arg(kVersionKey, kUuidAttribute, _item->uuid().toString(), kNameAttribute,
                        TextHelper::toHtmlEscaped(_item->name()), kColorAttribute,
                        ColorHelper::toString(_item->color()), kReadOnlyAttribute,
                        (_item->isReadOnly() ? "true" : "false"))
                   .toUtf8();
    };
    std::function<void(StructureModelItem*)> writeItemXml;
    writeItemXml = [&xml, &writeItemXml, writeVersionXml](StructureModelItem* _item) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\" %8=\"%9\" %10=\"%11\"")
                   .arg(kItemKey, kUuidAttribute, _item->uuid().toString(), kTypeAttribute,
                        Domain::mimeTypeFor(_item->type()), kNameAttribute,
                        TextHelper::toHtmlEscaped(_item->name()), kColorAttribute,
                        ColorHelper::toString(_item->color()), kVisibleAttribute,
                        (_item->isVisible() ? "true" : "false"))
                   .toUtf8();
        if (_item->versions().isEmpty() && !_item->hasChildren()) {
            xml += "/>\n";
            return;
        }

        xml += ">\n";
        for (const auto version : _item->versions()) {
            writeVersionXml(version);
        }
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            writeItemXml(_item->childAt(childIndex));
        }
        xml += QString("</%1>\n").arg(kItemKey).toUtf8();
    };
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        writeItemXml(rootItem->childAt(childIndex));
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}


// ****


StructureModel::StructureModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

StructureModel::~StructureModel() = default;

bool StructureModel::isNewProject() const
{
    return d->isNewProject;
}

void StructureModel::setProjectName(const QString& _name)
{
    d->projectName = _name;
}

QModelIndex StructureModel::addDocument(Domain::DocumentObjectType _type, const QString& _name,
                                        const QModelIndex& _parent, const QByteArray& _content,
                                        bool _visible, int _episodesAmount)
{
    //
    // ATTENTION: В ProjectManager::addDocument и ProjectManager::addScreenplay есть копипаста
    // отсюда, быть внимательным при обновлении
    //

    using namespace Domain;

    auto createItem = [_visible](DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        const auto readOnly = false;
        return new StructureModelItem(uuid, _type, _name, {}, _visible, readOnly);
    };

    auto parentItem = itemForIndex(_parent);

    switch (_type) {
    case DocumentObjectType::Project: {
        appendItem(createItem(_type, d->projectName), parentItem, _content);
        break;
    }

    case DocumentObjectType::RecycleBin: {
        appendItem(createItem(_type, tr("Recycle bin")), parentItem, _content);
        break;
    }

    case DocumentObjectType::Screenplay: {
        auto screenplayItem = createItem(DocumentObjectType::Screenplay,
                                         !_name.isEmpty() ? _name : tr("Screenplay"));
        appendItem(screenplayItem, parentItem);
        appendItem(createItem(DocumentObjectType::ScreenplayTitlePage, tr("Title page")),
                   screenplayItem);
        auto synopsisItem = createItem(DocumentObjectType::ScreenplaySynopsis, tr("Synopsis"));
        appendItem(synopsisItem, screenplayItem);
        appendItem(createItem(DocumentObjectType::ScreenplayText, tr("Screenplay")),
                   screenplayItem);
        appendItem(createItem(DocumentObjectType::ScreenplayStatistics, tr("Statistics")),
                   screenplayItem);
        //
        // Вставляем тритмент после всех документов, т.к. он является алиасом к документу сценария
        // и чтобы его сконструировать, нужны другие документы
        //
        insertItem(createItem(DocumentObjectType::ScreenplayTreatment, tr("Treatment")),
                   synopsisItem);
        break;
    }

    case DocumentObjectType::ScreenplaySeries: {
        auto seriesItem = createItem(DocumentObjectType::ScreenplaySeries,
                                     !_name.isEmpty() ? _name : tr("Series"));
        appendItem(seriesItem, parentItem);
        auto episodesItem
            = createItem(DocumentObjectType::ScreenplaySeriesEpisodes, tr("Episodes"));
        appendItem(episodesItem, seriesItem);
        appendItem(createItem(DocumentObjectType::ScreenplaySeriesStatistics, tr("Statistics")),
                   seriesItem);

        for (int episode = 0; episode < _episodesAmount; ++episode) {
            auto episodeItem = createItem(DocumentObjectType::Screenplay,
                                          tr("Episode") + QString(" %1").arg(episode + 1));
            appendItem(episodeItem, episodesItem);
            appendItem(createItem(DocumentObjectType::ScreenplayTitlePage, tr("Title page")),
                       episodeItem);
            auto synopsisItem = createItem(DocumentObjectType::ScreenplaySynopsis, tr("Synopsis"));
            appendItem(synopsisItem, episodeItem);
            appendItem(createItem(DocumentObjectType::ScreenplayText, tr("Screenplay")),
                       episodeItem);
            appendItem(createItem(DocumentObjectType::ScreenplayStatistics, tr("Statistics")),
                       episodeItem);
            //
            // Вставляем тритмент после всех документов, т.к. он является алиасом к документу
            // сценария и чтобы его сконструировать, нужны другие документы
            //
            insertItem(createItem(DocumentObjectType::ScreenplayTreatment, tr("Treatment")),
                       synopsisItem);
        }

        break;
    }

    case DocumentObjectType::ComicBook: {
        auto comicBookItem = createItem(DocumentObjectType::ComicBook,
                                        !_name.isEmpty() ? _name : tr("Comic book"));
        appendItem(comicBookItem, parentItem);
        appendItem(createItem(DocumentObjectType::ComicBookTitlePage, tr("Title page")),
                   comicBookItem);
        appendItem(createItem(DocumentObjectType::ComicBookSynopsis, tr("Synopsis")),
                   comicBookItem);
        appendItem(createItem(DocumentObjectType::ComicBookText, tr("Script")), comicBookItem);
        appendItem(createItem(DocumentObjectType::ComicBookStatistics, tr("Statistics")),
                   comicBookItem);
        break;
    }

    case DocumentObjectType::Audioplay: {
        auto audioplayItem
            = createItem(DocumentObjectType::Audioplay, !_name.isEmpty() ? _name : tr("Audioplay"));
        appendItem(audioplayItem, parentItem);
        appendItem(createItem(DocumentObjectType::AudioplayTitlePage, tr("Title page")),
                   audioplayItem);
        appendItem(createItem(DocumentObjectType::AudioplaySynopsis, tr("Synopsis")),
                   audioplayItem);
        appendItem(createItem(DocumentObjectType::AudioplayText, tr("Script")), audioplayItem);
        appendItem(createItem(DocumentObjectType::AudioplayStatistics, tr("Statistics")),
                   audioplayItem);
        break;
    }

    case DocumentObjectType::Stageplay: {
        auto audioplayItem
            = createItem(DocumentObjectType::Stageplay, !_name.isEmpty() ? _name : tr("Stageplay"));
        appendItem(audioplayItem, parentItem);
        appendItem(createItem(DocumentObjectType::StageplayTitlePage, tr("Title page")),
                   audioplayItem);
        appendItem(createItem(DocumentObjectType::StageplaySynopsis, tr("Synopsis")),
                   audioplayItem);
        appendItem(createItem(DocumentObjectType::StageplayText, tr("Script")), audioplayItem);
        appendItem(createItem(DocumentObjectType::StageplayStatistics, tr("Statistics")),
                   audioplayItem);
        break;
    }

    case DocumentObjectType::Novel: {
        auto novelItem
            = createItem(DocumentObjectType::Novel, !_name.isEmpty() ? _name : tr("Novel"));
        appendItem(novelItem, parentItem);
        appendItem(createItem(DocumentObjectType::NovelTitlePage, tr("Title page")), novelItem);
        auto synopsisItem = createItem(DocumentObjectType::NovelSynopsis, tr("Synopsis"));
        appendItem(synopsisItem, novelItem);
        appendItem(createItem(DocumentObjectType::NovelText, tr("Text")), novelItem);
        appendItem(createItem(DocumentObjectType::NovelStatistics, tr("Statistics")), novelItem);
        //
        // Вставляем аутлайн после всех документов, т.к. он является алиасом к документу текста
        // и чтобы его сконструировать, нужны другие документы
        //
        insertItem(createItem(DocumentObjectType::NovelOutline, tr("Outline")), synopsisItem);
        break;
    }

    case DocumentObjectType::Characters: {
        appendItem(createItem(_type, tr("Characters")), parentItem, _content);
        break;
    }
    case DocumentObjectType::Character: {
        parentItem = itemForType(Domain::DocumentObjectType::Characters);
        Q_ASSERT(parentItem);
        appendItem(createItem(_type, _name.toUpper()), parentItem, _content);

        //
        // Обновляем родителя, т.к. у него видмость зависит от наличия детей
        //
        updateItem(parentItem);

        break;
    }

    case DocumentObjectType::Locations: {
        appendItem(createItem(_type, tr("Locations")), parentItem, _content);
        break;
    }
    case DocumentObjectType::Location: {
        parentItem = itemForType(Domain::DocumentObjectType::Locations);
        Q_ASSERT(parentItem);
        appendItem(createItem(_type, _name.toUpper()), parentItem, _content);

        //
        // Обновляем родителя, т.к. у него видмость зависит от наличия детей
        //
        updateItem(parentItem);

        break;
    }

    case DocumentObjectType::Worlds: {
        appendItem(createItem(_type, tr("Worlds")), parentItem, _content);
        break;
    }
    case DocumentObjectType::World: {
        //
        // FIXME: SAD-811 выпилить в версии 0.6.0
        //
        auto worldsItem = itemForType(Domain::DocumentObjectType::Worlds);
        if (worldsItem == nullptr) {
            appendItem(createItem(Domain::DocumentObjectType::Worlds, tr("Worlds")), parentItem);
        }

        parentItem = itemForType(Domain::DocumentObjectType::Worlds);
        Q_ASSERT(parentItem);
        appendItem(createItem(_type, _name.toUpper()), parentItem, _content);

        //
        // Обновляем родителя, т.к. у него видмость зависит от наличия детей
        //
        updateItem(parentItem);

        break;
    }

    case DocumentObjectType::Folder:
    case DocumentObjectType::SimpleText: {
        appendItem(createItem(_type, _name), parentItem, _content);
        break;
    }

    case DocumentObjectType::MindMap: {
        appendItem(createItem(_type, !_name.isEmpty() ? _name : tr("Mind map")), parentItem,
                   _content);
        break;
    }

    case DocumentObjectType::ImagesGallery: {
        appendItem(createItem(_type, !_name.isEmpty() ? _name : tr("Images gallery")), parentItem,
                   _content);
        break;
    }

    case DocumentObjectType::Presentation: {
        appendItem(createItem(_type, !_name.isEmpty() ? _name : tr("Presentation")), parentItem,
                   _content);
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }

    //
    // Вставка идёт в конец, но в корневом элементе учитываем корзину, поэтому смещаем на 2
    //
    const int indexDelta = parentItem == d->rootItem ? 2 : 1;
    return index(parentItem->childCount() - indexDelta, 0, indexForItem(parentItem));
}

void StructureModel::prependItem(StructureModelItem* _item, StructureModelItem* _parentItem,
                                 const QByteArray& _content)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (_parentItem->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int itemRowIndex = 0; // т.к. в самое начало
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    _parentItem->prependItem(_item);
    endInsertRows();

    emit documentAdded(_item->uuid(), _parentItem->uuid(), _item->type(), _item->name(), _content);
}

void StructureModel::appendItem(StructureModelItem* _item, StructureModelItem* _parentItem,
                                const QByteArray& _content)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (_parentItem->hasChild(_item)) {
        return;
    }

    //
    // Если уже создана корзина, то добавляем все новые элементы перед ней,
    // для этого определим смещение по индексу для добавляемого элемента
    //
    int recycleBinDelta = 0;
    if (_parentItem == d->rootItem && _parentItem->hasChildren()
        && _parentItem->childAt(_parentItem->childCount() - 1)->type()
            == Domain::DocumentObjectType::RecycleBin) {
        recycleBinDelta = -1;
    }

    //
    // Собственно добавляем новый элемент
    //
    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int itemRowIndex = _parentItem->childCount() + recycleBinDelta;
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    _parentItem->insertItem(itemRowIndex, _item);
    endInsertRows();

    emit documentAdded(_item->uuid(), _parentItem->uuid(), _item->type(), _item->name(), _content);
}

void StructureModel::insertItem(StructureModelItem* _item, StructureModelItem* _afterSiblingItem,
                                const QByteArray& _content)
{
    if (_item == nullptr || _afterSiblingItem == nullptr
        || _afterSiblingItem->parent() == nullptr) {
        return;
    }

    auto parent = _afterSiblingItem->parent();

    if (parent->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(parent);
    const int itemRowIndex = parent->rowOfChild(_afterSiblingItem) + 1;
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    parent->insertItem(itemRowIndex, _item);
    endInsertRows();

    emit documentAdded(_item->uuid(), parent->uuid(), _item->type(), _item->name(), _content);
}

void StructureModel::moveItem(StructureModelItem* _item, StructureModelItem* _parentItem)
{
    moveItem(_item, _parentItem, _parentItem->childCount());
}

void StructureModel::moveItem(StructureModelItem* _item, StructureModelItem* _parentItem,
                              int _targetIndex)
{
    if (_item == nullptr || _parentItem == nullptr) {
        return;
    }

    auto sourceParent = _item->parent();
    if (sourceParent == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    if (sourceParent == _parentItem && itemIndex.row() == _targetIndex) {
        return;
    }

    const auto sourceParentIndex = indexForItem(sourceParent);
    const auto destinationIndex = indexForItem(_parentItem);
    beginMoveRows(sourceParentIndex, itemIndex.row(), itemIndex.row(), destinationIndex,
                  _targetIndex);
    sourceParent->takeItem(_item);
    _parentItem->insertItem(_targetIndex, _item);
    endMoveRows();
}

void StructureModel::moveItem(const QUuid& _uuid, int _to)
{
    const auto item = itemForUuid(_uuid);
    moveItem(item, item->parent(), _to);
}

void StructureModel::takeItem(StructureModelItem* _item)
{
    if (_item == nullptr || _item->parent() == nullptr) {
        return;
    }

    auto itemParent = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = itemParent->rowOfChild(_item);
    beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);
    itemParent->takeItem(_item);
    endRemoveRows();
}

void StructureModel::removeItem(StructureModelItem* _item)
{
    if (_item == nullptr || _item->parent() == nullptr) {
        return;
    }

    auto itemParent = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = itemParent->rowOfChild(_item);
    beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);

    emit documentAboutToBeRemoved(_item->uuid());

    itemParent->removeItem(_item);
    endRemoveRows();
}

void StructureModel::updateItem(StructureModelItem* _item)
{
    if (_item == nullptr || _item->parent() == nullptr) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
}

QModelIndex StructureModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    auto parentItem = itemForIndex(_parent);
    Q_ASSERT(parentItem);

    auto indexItem = parentItem->childAt(_row);
    if (indexItem == nullptr) {
        return {};
    }

    return createIndex(_row, _column, indexItem);
}

QModelIndex StructureModel::parent(const QModelIndex& _child) const
{
    if (!_child.isValid()) {
        return {};
    }

    auto childItem = itemForIndex(_child);
    auto parentItem = childItem->parent();
    if (parentItem == nullptr || parentItem == d->rootItem) {
        return {};
    }

    auto grandParentItem = parentItem->parent();
    if (grandParentItem == nullptr) {
        return {};
    }

    const auto row = grandParentItem->rowOfChild(parentItem);
    return createIndex(row, 0, parentItem);
}

int StructureModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int StructureModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid() && _parent.column() != 0) {
        return 0;
    }

    auto item = itemForIndex(_parent);
    if (item == nullptr) {
        return 0;
    }

    return item->childCount();
}

Qt::ItemFlags StructureModel::flags(const QModelIndex& _index) const
{
    const auto item = itemForIndex(_index);
    const auto defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (item->type()) {
    //
    // Элемент можно перемещать и вставлять внутрь другие
    //
    case Domain::DocumentObjectType::Characters:
    case Domain::DocumentObjectType::Character:
    case Domain::DocumentObjectType::Locations:
    case Domain::DocumentObjectType::Location:
    case Domain::DocumentObjectType::Worlds:
    case Domain::DocumentObjectType::World:
    case Domain::DocumentObjectType::Folder: {
        return defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    //
    // В элемент можно только вставлять другие
    //
    case Domain::DocumentObjectType::Project:
    case Domain::DocumentObjectType::ScreenplaySeriesEpisodes:
    case Domain::DocumentObjectType::RecycleBin: {
        return defaultFlags | Qt::ItemIsDropEnabled;
    }

    //
    // Элемент можно только перемещать
    //
    case Domain::DocumentObjectType::Screenplay:
    case Domain::DocumentObjectType::ScreenplaySeries:
    case Domain::DocumentObjectType::ComicBook:
    case Domain::DocumentObjectType::Audioplay:
    case Domain::DocumentObjectType::Stageplay:
    case Domain::DocumentObjectType::Novel:
    case Domain::DocumentObjectType::SimpleText:
    case Domain::DocumentObjectType::MindMap:
    case Domain::DocumentObjectType::ImagesGallery:
    case Domain::DocumentObjectType::Presentation: {
        return defaultFlags | Qt::ItemIsDragEnabled;
    }

    //
    // Элемент нельзя ни перемещать ни вставлять внутрь в другие
    //
    case Domain::DocumentObjectType::ScreenplayTitlePage:
    case Domain::DocumentObjectType::ScreenplaySynopsis:
    case Domain::DocumentObjectType::ScreenplayTreatment:
    case Domain::DocumentObjectType::ScreenplayText:
    case Domain::DocumentObjectType::ScreenplayStatistics:
    case Domain::DocumentObjectType::ScreenplaySeriesTitlePage:
    case Domain::DocumentObjectType::ScreenplaySeriesSynopsis:
    case Domain::DocumentObjectType::ScreenplaySeriesTreatment:
    case Domain::DocumentObjectType::ScreenplaySeriesText:
    case Domain::DocumentObjectType::ScreenplaySeriesStatistics:
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
    case Domain::DocumentObjectType::NovelStatistics: {
        return defaultFlags;
    }

    default: {
        return Qt::ItemIsDropEnabled;
    }
    }
}

QVariant StructureModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    //
    // Кастомные данные модели
    //
    switch (_role) {
    case StructureModelDataRole::DocumentUuidRole: {
        return itemForIndex(_index)->uuid();
    }
    case StructureModelDataRole::IsNavigatorAvailable: {
        return d->navigatorAvailableIndexes.contains(_index);
    }
    default: {
        break;
    }
    }

    //
    // Данные непосредственно элемента модели
    //

    auto item = itemForIndex(_index);
    if (item == nullptr) {
        return {};
    }

    return item->data(_role);
}

bool StructureModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
                                     int _column, const QModelIndex& _parent) const
{
    Q_UNUSED(_action)
    Q_UNUSED(_row)

    if (!_data->hasFormat(kMimeType)) {
        return false;
    }

    if (_column > 0) {
        return false;
    }

    if (d->lastMimeItems.isEmpty()) {
        return false;
    }

    //
    // Проверяем, что перемещаются данные из модели
    //
    QByteArray encodedData = _data->data(kMimeType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int row = 0;
    while (!stream.atEnd()) {
        QUuid itemUuid;
        stream >> itemUuid;
        if (itemUuid != d->lastMimeItems[row]->uuid()) {
            //
            // ... если это какие-то внешние данные, то ничего не делаем
            //
            return false;
        }

        ++row;
    }

    //
    // Смотрим, что за данные перемещаются
    //
    bool hasCharacters = false;
    bool hasCharacter = false;
    bool hasLocations = false;
    bool hasLocation = false;
    bool hasWorlds = false;
    bool hasWorld = false;
    bool onlyScreenplays = true;
    for (const auto item : std::as_const(d->lastMimeItems)) {
        if (onlyScreenplays && item->type() != Domain::DocumentObjectType::Screenplay) {
            onlyScreenplays = false;
        }

        switch (item->type()) {
        case Domain::DocumentObjectType::Characters: {
            hasCharacters = true;
            break;
        }
        case Domain::DocumentObjectType::Character: {
            hasCharacter = true;
            break;
        }

        case Domain::DocumentObjectType::Locations: {
            hasLocations = true;
            break;
        }
        case Domain::DocumentObjectType::Location: {
            hasLocation = true;
            break;
        }

        case Domain::DocumentObjectType::Worlds: {
            hasWorlds = true;
            break;
        }
        case Domain::DocumentObjectType::World: {
            hasWorld = true;
            break;
        }

        default: {
            break;
        }
        }
    }

    //
    // Обработка конкретных случаев что куда можно бросать
    //
    const auto dropTarget = itemForIndex(_parent);
    //
    // ... eсли среди перемещаемых есть хотя бы два из трёх особенных, то запрещаем перемещение
    //
    if (hasCharacters + hasCharacter + hasLocations + hasLocation + hasWorlds + hasWorld > 1) {
        return false;
    }
    //
    // ... карты персонажей, локаций и миров можно таскать только по верхнему уровню, кроме корзины
    //
    else if (hasCharacters || hasLocations || hasWorlds) {
        return dropTarget->type() == Domain::DocumentObjectType::Undefined;
    }
    //
    // ... персонажей и локации можно перетаскивать только внутри родительского элемента
    //
    else if (hasCharacter) {
        return dropTarget->type() == Domain::DocumentObjectType::Characters
            || dropTarget->type() == Domain::DocumentObjectType::RecycleBin;
    } else if (hasLocation) {
        return dropTarget->type() == Domain::DocumentObjectType::Locations
            || dropTarget->type() == Domain::DocumentObjectType::RecycleBin;
    } else if (hasWorld) {
        return dropTarget->type() == Domain::DocumentObjectType::Worlds
            || dropTarget->type() == Domain::DocumentObjectType::RecycleBin;
    }
    //
    // ... остальные случаи
    //
    switch (dropTarget->type()) {
    //
    // ... внутрь персонажей, локаций и миров можно вставлять только соответствующие документы,
    //     это проверяется выше, любые другие документы нельзя сюда вставлять
    //
    case Domain::DocumentObjectType::Characters:
    case Domain::DocumentObjectType::Locations:
    case Domain::DocumentObjectType::Worlds: {
        return false;
    }

    //
    // ... внутрь сериала можно вставить только сценарии
    //
    case Domain::DocumentObjectType::ScreenplaySeriesEpisodes: {
        return onlyScreenplays;
    }

    //
    // Во всех остальных случаях можно
    //
    default: {
        return true;
    }
    }
}

bool StructureModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
                                  int _column, const QModelIndex& _parent)
{
    if (!canDropMimeData(_data, _action, _row, _column, _parent)) {
        return false;
    }

    if (_action == Qt::IgnoreAction) {
        return true;
    }

    //
    // Перемещаем все элементы по очереди
    //

    auto parentItem = itemForIndex(_parent);

    //
    // Добавляем после всех элементов выбранного
    //
    if (_row == -1 || _row == parentItem->childCount()) {
        //
        // Если вставляем перед корзиной, то добавляем дельту
        //
        const int recycleBinIndexDelta = parentItem == d->rootItem ? -1 : 0;

        while (!d->lastMimeItems.isEmpty()) {
            auto item = d->lastMimeItems.takeFirst();
            const auto itemIndex = indexForItem(item);

            if (item->parent() == parentItem
                && itemIndex.row() == (parentItem->childCount() - 1 + recycleBinIndexDelta)) {
                continue;
            }

            beginMoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row(), _parent,
                          parentItem->childCount() + recycleBinIndexDelta);
            item->parent()->takeItem(item);
            parentItem->insertItem(parentItem->childCount() + recycleBinIndexDelta, item);
            endMoveRows();
        }
    }
    //
    // Вставляем между элементами
    //
    else {
        const QModelIndex insertBeforeItemIndex = index(_row, _column, _parent);
        auto insertBeforeItem = itemForIndex(insertBeforeItemIndex);

        if (d->lastMimeItems.contains(insertBeforeItem)) {
            return false;
        }

        while (!d->lastMimeItems.isEmpty()) {
            auto item = d->lastMimeItems.takeFirst();
            auto itemIndex = indexForItem(item);

            //
            // Нет смысла перемещать элемент на то же самое место
            //
            if (itemIndex.parent() == insertBeforeItemIndex.parent()
                && (itemIndex.row() == insertBeforeItemIndex.row()
                    || itemIndex.row() == insertBeforeItemIndex.row() - 1)) {
                continue;
            }

            beginMoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row(),
                          insertBeforeItemIndex.parent(), _row);
            item->parent()->takeItem(item);
            parentItem->insertItem(parentItem->rowOfChild(insertBeforeItem), item);
            endMoveRows();
        }
    }

    return true;
}

QMimeData* StructureModel::mimeData(const QModelIndexList& _indexes) const
{
    d->lastMimeItems.clear();

    if (_indexes.isEmpty()) {
        return nullptr;
    }

    //
    // Формируем список элементов для перемещения
    //
    for (const QModelIndex& index : _indexes) {
        if (index.isValid()) {
            d->lastMimeItems << itemForIndex(index);
        }
    }
    //
    // ... и упорядочиваем его
    //
    std::sort(d->lastMimeItems.begin(), d->lastMimeItems.end(),
              [](StructureModelItem* _lhs, StructureModelItem* _rhs) {
                  //
                  // Для элементов находящихся на одном уровне сравниваем их позиции
                  //
                  if (_lhs->parent() == _rhs->parent()) {
                      return _lhs->parent()->rowOfChild(_lhs) < _rhs->parent()->rowOfChild(_rhs);
                  }

                  //
                  // Для разноуровневых элементов определяем путь до верха и сравниваем пути
                  //
                  auto buildPath = [](StructureModelItem* _item) {
                      QString path;
                      auto child = _item;
                      auto parent = child->parent();
                      while (parent != nullptr) {
                          path.prepend(QString("0000%1").arg(parent->rowOfChild(child)).right(5));
                          child = parent;
                          parent = child->parent();
                      }
                      return path;
                  };
                  return buildPath(_lhs) < buildPath(_rhs);
              });

    //
    // Помещаем индексы перемещаемых элементов в майм
    //
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for (const auto& item : std::as_const(d->lastMimeItems)) {
        stream << item->uuid();
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData(kMimeType, encodedData);
    return mimeData;
}

QStringList StructureModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions StructureModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions StructureModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QModelIndex StructureModel::indexForItem(StructureModelItem* _item) const
{
    if (_item == nullptr) {
        return {};
    }

    int row = 0;
    QModelIndex parent;
    if (_item->hasParent() && _item->parent()->hasParent()) {
        row = _item->parent()->rowOfChild(_item);
        parent = indexForItem(_item->parent());
    } else {
        row = d->rootItem->rowOfChild(_item);
    }

    return index(row, 0, parent);
}

StructureModelItem* StructureModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<StructureModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

StructureModelItem* StructureModel::itemForUuid(const QUuid& _uuid) const
{
    std::function<StructureModelItem*(StructureModelItem*)> search;
    search = [&search, _uuid](StructureModelItem* _parent) -> StructureModelItem* {
        for (int itemRow = 0; itemRow < _parent->childCount(); ++itemRow) {
            auto item = _parent->childAt(itemRow);
            if (item->uuid() == _uuid) {
                return item;
            }

            const auto versions = item->versions();
            for (auto version : versions) {
                if (version->uuid() == _uuid) {
                    return version;
                }
            }

            auto childItem = search(item);
            if (childItem != nullptr) {
                return childItem;
            }
        }
        return nullptr;
    };

    return search(d->rootItem);
}

StructureModelItem* StructureModel::itemForType(Domain::DocumentObjectType _type) const
{
    //
    // Ищем только по верхнеуровневым элементам
    //
    for (int itemIndex = 0; itemIndex < d->rootItem->childCount(); ++itemIndex) {
        auto item = d->rootItem->childAt(itemIndex);
        if (item->type() == _type) {
            return item;
        }
    }

    return nullptr;
}

void StructureModel::moveItemToRecycleBin(StructureModelItem* _item)
{
    if (_item == nullptr) {
        return;
    }

    //
    // Идём снизу, т.к. обычно корзина находится внизу
    //
    StructureModelItem* recycleBin = itemForType(Domain::DocumentObjectType::RecycleBin);
    Q_ASSERT(recycleBin);

    //
    // Собственно перемещаем элемент в корзину
    //
    moveItem(_item, recycleBin);
}

void StructureModel::setItemName(const QModelIndex& _index, const QString& _name)
{
    auto item = itemForIndex(_index);
    if (item == d->rootItem) {
        return;
    }

    item->setName(_name);
    emit dataChanged(_index, _index);
}

void StructureModel::setItemName(StructureModelItem* _item, const QString& _name)
{
    if (_item == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    _item->setName(_name);
    emit dataChanged(itemIndex, itemIndex);
}

void StructureModel::setItemColor(StructureModelItem* _item, const QColor& _color)
{
    if (_item == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    _item->setColor(_color);
    emit dataChanged(itemIndex, itemIndex);
}

void StructureModel::setItemVisible(StructureModelItem* _item, bool _visible)
{
    if (_item == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    _item->setVisible(_visible);
    emit dataChanged(itemIndex, itemIndex);
}

void StructureModel::addItemVersion(StructureModelItem* _item, const QString& _name,
                                    const QColor& _color, bool _readOnly,
                                    const QByteArray& _content)
{
    if (_item == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    auto newVersion = _item->addVersion(_name, _color, _readOnly);
    emit dataChanged(itemIndex, itemIndex);

    emit documentAdded(newVersion->uuid(), _item->parent()->uuid(), newVersion->type(),
                       newVersion->name(), _content);

    emit versionAdded(_item->uuid());
}

void StructureModel::updateItemVersion(StructureModelItem* _item, int _versionIndex,
                                       const QString& _name, const QColor& _color, bool _readOnly)
{
    if (_item == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    auto version = _item->versions().at(_versionIndex);
    version->setName(_name);
    version->setColor(_color);
    version->setReadOnly(_readOnly);
    emit dataChanged(itemIndex, itemIndex);
}

void StructureModel::removeItemVersion(StructureModelItem* _item, int _versionIndex)
{
    if (_item == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    _item->removeVersion(_versionIndex);
    emit dataChanged(itemIndex, itemIndex);
    emit versionRemoved(_item->uuid());
}

void StructureModel::setNavigatorAvailableFor(const QModelIndex& _index, bool isAvailable)
{
    if (isAvailable) {
        d->navigatorAvailableIndexes.insert(_index);
    } else {
        d->navigatorAvailableIndexes.remove(_index);
    }
}

void StructureModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        d->isNewProject = true;

        addDocument(Domain::DocumentObjectType::Project);
        addDocument(Domain::DocumentObjectType::Characters);
        addDocument(Domain::DocumentObjectType::Locations);
        addDocument(Domain::DocumentObjectType::Worlds);

        //
        // При необходимости добавим дополнительные элементы в первоначальную структуру
        //
        const auto projectType = static_cast<Domain::DocumentObjectType>(
            settingsValue(DataStorageLayer::kProjectTypeKey).toInt());
        if (projectType != Domain::DocumentObjectType::Undefined) {
            addDocument(projectType);
        }

        addDocument(Domain::DocumentObjectType::RecycleBin);
    }
    //
    // А если данные есть, то загрузим их из документа
    //
    else {
        beginResetModelTransaction();
        d->buildModel(document());
        endResetModelTransaction();
    }
}

void StructureModel::clearDocument()
{
    if (!d->rootItem->hasChildren()) {
        return;
    }

    beginRemoveRows({}, 0, d->rootItem->childCount() - 1);
    while (d->rootItem->childCount() > 0) {
        d->rootItem->removeItem(d->rootItem->childAt(0));
    }
    endRemoveRows();
}

QByteArray StructureModel::toXml() const
{
    return d->toXml(document());
}

ChangeCursor StructureModel::applyPatch(const QByteArray& _patch)
{
    Q_ASSERT(document());

#ifdef XML_CHECKS
    const auto newContent = dmpController().applyPatch(toXml(), _patch);
    qDebug(QString("Before applying patch xml is\n\n%1\n\n").arg(toXml().constData()).toUtf8());
    qDebug(QString("Patch is\n\n%1\n\n")
               .arg(QByteArray::fromPercentEncoding(_patch).constData())
               .toUtf8());
#endif

    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Patch don't lead to any changes");
        return {};
    }

    changes.first.xml = xml::prepareXml(changes.first.xml);
    changes.second.xml = xml::prepareXml(changes.second.xml);

#ifdef XML_CHECKS
    qDebug(QString("Xml data changes first item\n\n%1\n\n")
               .arg(changes.first.xml.constData())
               .toUtf8());
    qDebug(QString("Xml data changes second item\n\n%1\n\n")
               .arg(changes.second.xml.constData())
               .toUtf8());
#endif

    //
    // Считываем элементы из обоих изменений для дальнейшего определения необходимых изменений
    //
    auto readItems = [this](const QString& _xml) -> QVector<StructureModelItem*> {
        QVector<StructureModelItem*> items;
        QDomDocument domDocument;
        domDocument.setContent(_xml);
        auto documentNode = domDocument.firstChildElement(kDocumentKey);
        auto itemNode = documentNode.firstChildElement();
        while (!itemNode.isNull()) {
            items.append(d->buildItem(itemNode));
            itemNode = itemNode.nextSiblingElement();
        }
        return items;
    };
    const auto oldItems = readItems(changes.first.xml);
    const auto newItems = readItems(changes.second.xml);

    //
    // Раскладываем элементы в плоские списки для сравнения
    //
    std::function<QVector<StructureModelItem*>(const QVector<StructureModelItem*>&)> makeItemsPlain;
    makeItemsPlain = [&makeItemsPlain](const QVector<StructureModelItem*>& _items) {
        QVector<StructureModelItem*> itemsPlain;
        for (auto item : _items) {
            itemsPlain.append(item);
            for (int row = 0; row < item->childCount(); ++row) {
                itemsPlain.append(makeItemsPlain({ item->childAt(row) }));
            }
        }
        return itemsPlain;
    };
    auto oldItemsPlain = makeItemsPlain(oldItems);
    Q_ASSERT(!oldItemsPlain.isEmpty());
    auto newItemsPlain = makeItemsPlain(newItems);
    Q_ASSERT(!newItemsPlain.isEmpty());

    //
    // Определим необходимые операции для применения изменения
    //
    const auto operations = edit_distance::editDistance(oldItemsPlain, newItemsPlain);
    //
    auto modelItem = itemForUuid(oldItemsPlain.constFirst()->uuid());
    StructureModelItem* previousModelItem = nullptr;
    //
    std::function<StructureModelItem*(StructureModelItem*, bool)> findNextItemWithChildren;
    findNextItemWithChildren
        = [&findNextItemWithChildren](StructureModelItem* _item,
                                      bool _searchInChildren) -> StructureModelItem* {
        if (_item == nullptr) {
            return nullptr;
        }

        if (_searchInChildren) {
            //
            // Если есть дети, идём в дочерний элемент
            //
            if (_item->hasChildren()) {
                return _item->childAt(0);
            }
        }

        //
        // Если детей нет, идём в следующий
        //

        if (!_item->hasParent()) {
            return nullptr;
        }
        auto parent = _item->parent();

        auto itemIndex = parent->rowOfChild(_item);
        if (itemIndex < 0 || itemIndex >= parent->childCount()) {
            return nullptr;
        }

        //
        // Не последний в родителе, берём следующий с этого же уровня
        //
        if (itemIndex < parent->childCount() - 1) {
            return parent->childAt(itemIndex + 1);
        }
        //
        // Последний в родителе, берём следующий с предыдущего уровня
        //
        else {
            return findNextItemWithChildren(parent, false);
        }
    };
    auto findNextItem = [&findNextItemWithChildren](StructureModelItem* _item) {
        return findNextItemWithChildren(_item, true);
    };
    //
    // И применяем их
    //
    for (const auto& operation : operations) {
        //
        // Собственно применяем операции
        //
        auto newItem = operation.value;
        switch (operation.type) {
        case edit_distance::OperationType::Skip: {
            //
            // При необходимости, корректируем положение элемента
            //
            if (newItem->parent() == nullptr) {
                if (modelItem->parent() != d->rootItem) {
                    while (previousModelItem->parent() != d->rootItem) {
                        previousModelItem = previousModelItem->parent();
                    }
                    takeItem(modelItem);
                    insertItem(modelItem, previousModelItem);
                }
            } else if (newItem->parent()->uuid() != modelItem->parent()->uuid()) {
                if (newItem->parent()->uuid() == previousModelItem->uuid()) {
                    moveItem(modelItem, previousModelItem);
                } else {
                    while (previousModelItem->parent() != d->rootItem
                           && newItem->parent()->uuid() != previousModelItem->parent()->uuid()) {
                        previousModelItem = previousModelItem->parent();
                    }
                    takeItem(modelItem);
                    insertItem(modelItem, previousModelItem);
                }
            }

            previousModelItem = modelItem;
            modelItem = findNextItem(modelItem);
            break;
        }

        case edit_distance::OperationType::Remove: {
            //
            // Выносим детей на предыдущий уровень
            //
            while (modelItem->hasChildren()) {
                auto childItem = modelItem->childAt(modelItem->childCount() - 1);
                takeItem(childItem);
                insertItem(childItem, modelItem);
            }
            //
            // ... и удаляем сам элемент
            //
            auto nextItem = findNextItem(modelItem);
            removeItem(modelItem);

            //
            // Обновляем родителя, т.к. у него видмость зависит от наличия детей
            //
            if (previousModelItem->type() == Domain::DocumentObjectType::Characters
                || previousModelItem->type() == Domain::DocumentObjectType::Locations
                || previousModelItem->type() == Domain::DocumentObjectType::Worlds) {
                updateItem(previousModelItem);
            }

            modelItem = nextItem;
            break;
        }

        case edit_distance::OperationType::Insert: {
            //
            // Создаём новый элемент
            //
            auto itemToInsert = new StructureModelItem(*newItem);
            //
            // ... и вставляем в нужного родителя
            //
            if (newItem->parent() == nullptr) {
                while (previousModelItem->parent() != d->rootItem) {
                    previousModelItem = previousModelItem->parent();
                }
                insertItem(itemToInsert, previousModelItem);
            } else {
                if (newItem->parent()->uuid() == previousModelItem->uuid()) {
                    prependItem(itemToInsert, previousModelItem);
                } else {
                    while (previousModelItem->parent() != d->rootItem
                           && newItem->parent()->uuid() != previousModelItem->parent()->uuid()) {
                        previousModelItem = previousModelItem->parent();
                    }
                    insertItem(itemToInsert, previousModelItem);
                }
            }

            //
            // Обновляем родителя, т.к. у него видмость зависит от наличия детей
            //
            if (previousModelItem->type() == Domain::DocumentObjectType::Characters
                || previousModelItem->type() == Domain::DocumentObjectType::Locations
                || previousModelItem->type() == Domain::DocumentObjectType::Worlds) {
                updateItem(previousModelItem);
            }

            previousModelItem = itemToInsert;
            break;
        }

        case edit_distance::OperationType::Replace: {
            //
            // Обновляем элемент
            //
            if (!modelItem->isEqual(newItem)) {
                modelItem->copyFrom(newItem);
                updateItem(modelItem);
                //
                // Выносим детей на предыдущий уровень, т.к. мог измениться их родитель
                //
                while (modelItem->hasChildren()) {
                    auto childItem = modelItem->childAt(modelItem->childCount() - 1);
                    takeItem(childItem);
                    insertItem(childItem, modelItem);
                }
            }
            //
            // Корректируем положение элемента
            //
            if (newItem->parent() == nullptr) {
                if (modelItem->parent() != d->rootItem) {
                    while (previousModelItem->parent() != d->rootItem) {
                        previousModelItem = previousModelItem->parent();
                    }
                    takeItem(modelItem);
                    insertItem(modelItem, previousModelItem);
                }
            } else if (newItem->parent()->uuid() != modelItem->parent()->uuid()) {
                if (newItem->parent()->uuid() == previousModelItem->uuid()) {
                    moveItem(modelItem, previousModelItem);
                } else {
                    while (previousModelItem->parent() != d->rootItem
                           && newItem->parent()->uuid() != previousModelItem->parent()->uuid()) {
                        previousModelItem = previousModelItem->parent();
                    }
                    takeItem(modelItem);
                    insertItem(modelItem, previousModelItem);
                }
            }

            previousModelItem = modelItem;
            modelItem = findNextItem(modelItem);
            break;
        }
        }
    }

    qDeleteAll(oldItems);
    qDeleteAll(newItems);

#ifdef XML_CHECKS
    //
    // Делаем проверку на соответствие обновлённой модели прямому наложению патча
    //
    if (newContent != toXml()) {
        qDebug(QString("New content should be\n\n%1\n\n").arg(newContent.constData()).toUtf8());
        qDebug(QString("New content is\n\n%1\n\n").arg(toXml().constData()).toUtf8());
    }
    Q_ASSERT(newContent == toXml());
#endif

    return {};
}

} // namespace BusinessLayer
