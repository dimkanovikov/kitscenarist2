#include "project_models_facade.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/audioplay_statistics_model.h>
#include <business_layer/model/audioplay/audioplay_synopsis_model.h>
#include <business_layer/model/audioplay/audioplay_title_page_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/comic_book_statistics_model.h>
#include <business_layer/model/comic_book/comic_book_synopsis_model.h>
#include <business_layer/model/comic_book/comic_book_title_page_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/images/images_gallery_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/mind_map/mind_map_model.h>
#include <business_layer/model/novel/novel_dictionaries_model.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/model/novel/novel_statistics_model.h>
#include <business_layer/model/novel/novel_synopsis_model.h>
#include <business_layer/model/novel/novel_title_page_model.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/presentation/presentation_model.h>
#include <business_layer/model/project/project_information_model.h>
#include <business_layer/model/recycle_bin/recycle_bin_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_statistics_model.h>
#include <business_layer/model/screenplay/screenplay_synopsis_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/screenplay/series/screenplay_series_episodes_model.h>
#include <business_layer/model/screenplay/series/screenplay_series_information_model.h>
#include <business_layer/model/screenplay/series/screenplay_series_statistics_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/stageplay_statistics_model.h>
#include <business_layer/model/stageplay/stageplay_synopsis_model.h>
#include <business_layer/model/stageplay/stageplay_title_page_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>
#include <business_layer/model/worlds/world_model.h>
#include <business_layer/model/worlds/worlds_model.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QSet>


namespace ManagementLayer {

class ProjectModelsFacade::Implementation
{
public:
    explicit Implementation(BusinessLayer::StructureModel* _projectStructureModel,
                            BusinessLayer::AbstractImageWrapper* _imageWrapper);


    BusinessLayer::StructureModel* projectStructureModel = nullptr;
    BusinessLayer::AbstractImageWrapper* imageWrapper = nullptr;
    QHash<Domain::DocumentObject*, BusinessLayer::AbstractModel*> documentsToModels;
};

ProjectModelsFacade::Implementation::Implementation(
    BusinessLayer::StructureModel* _projectStructureModel,
    BusinessLayer::AbstractImageWrapper* _imageWrapper)
    : projectStructureModel(_projectStructureModel)
    , imageWrapper(_imageWrapper)
{
}


// ****


ProjectModelsFacade::ProjectModelsFacade(BusinessLayer::StructureModel* _projectStructureModel,
                                         BusinessLayer::AbstractImageWrapper* _imageWrapper,
                                         QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(_projectStructureModel, _imageWrapper))
{
}

ProjectModelsFacade::~ProjectModelsFacade()
{
    clear();
}

void ProjectModelsFacade::clear()
{
    //
    // Формируем список моделей для удаления, т.к. некоторые модели являются лишь ссылками на другие
    // модели, например модель тритмента - это ссылка на модель текста сценария
    //
    QSet<BusinessLayer::AbstractModel*> modelsToDelete;
    for (auto model : std::as_const(d->documentsToModels)) {
        modelsToDelete.insert(model);

        //
        // ... а заодно отсоединяем модели от клиентов и очищаем их
        //
        model->disconnect();
        model->clear();
    }

    //
    // Собственно удаляем модели
    //
    qDeleteAll(modelsToDelete);

    //
    // И очищаем список загруженных моделей
    //
    d->documentsToModels.clear();
}

BusinessLayer::AbstractModel* ProjectModelsFacade::modelFor(const QUuid& _uuid)
{
    return modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(_uuid));
}

BusinessLayer::AbstractModel* ProjectModelsFacade::modelFor(Domain::DocumentObjectType _type)
{
    return modelFor(DataStorageLayer::StorageFacade::documentStorage()->document(_type));
}

BusinessLayer::AbstractModel* ProjectModelsFacade::modelFor(Domain::DocumentObject* _document)
{
    if (_document == nullptr) {
        return nullptr;
    }

    QVector<Domain::DocumentObject*> documentsToLoad = { _document };

    while (!documentsToLoad.isEmpty()) {
        auto documentToLoad = documentsToLoad.takeFirst();
        //
        // Является ли документы алиасом к другому, если да, то не исопльзуем его контент
        //
        bool isDocumentAlias = false;

        if (!d->documentsToModels.contains(documentToLoad)) {
            BusinessLayer::AbstractModel* model = nullptr;
            switch (documentToLoad->type()) {
            case Domain::DocumentObjectType::Project: {
                auto projectInformationModel = new BusinessLayer::ProjectInformationModel;
                connect(projectInformationModel,
                        &BusinessLayer::ProjectInformationModel::nameChanged, this,
                        &ProjectModelsFacade::projectNameChanged, Qt::UniqueConnection);
                connect(projectInformationModel,
                        &BusinessLayer::ProjectInformationModel::collaboratorInviteRequested, this,
                        &ProjectModelsFacade::projectCollaboratorInviteRequested,
                        Qt::UniqueConnection);
                connect(projectInformationModel,
                        &BusinessLayer::ProjectInformationModel::collaboratorUpdateRequested, this,
                        &ProjectModelsFacade::projectCollaboratorUpdateRequested,
                        Qt::UniqueConnection);
                connect(projectInformationModel,
                        &BusinessLayer::ProjectInformationModel::collaboratorRemoveRequested, this,
                        &ProjectModelsFacade::projectCollaboratorRemoveRequested,
                        Qt::UniqueConnection);
                model = projectInformationModel;
                break;
            }

            case Domain::DocumentObjectType::RecycleBin: {
                auto recycleBinModel = new BusinessLayer::RecycleBinModel;

                auto updateChildCount = [this, recycleBinModel,
                                         documentUuid = documentToLoad->uuid()] {
                    auto recycleBinItem = d->projectStructureModel->itemForUuid(documentUuid);
                    if (recycleBinItem == nullptr) {
                        return;
                    }

                    std::function<int(BusinessLayer::StructureModelItem*)> countChildren;
                    countChildren = [&countChildren](BusinessLayer::StructureModelItem* _item) {
                        int childrenSize = 0;
                        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                            childrenSize += 1 + countChildren(_item->childAt(childIndex));
                        }
                        return childrenSize;
                    };
                    recycleBinModel->setDocumentsToRemoveSize(countChildren(recycleBinItem));
                };
                updateChildCount();
                connect(d->projectStructureModel, &BusinessLayer::StructureModel::rowsInserted,
                        recycleBinModel, updateChildCount);
                connect(d->projectStructureModel, &BusinessLayer::StructureModel::rowsMoved,
                        recycleBinModel, updateChildCount);
                connect(d->projectStructureModel, &BusinessLayer::StructureModel::rowsRemoved,
                        recycleBinModel, updateChildCount);
                connect(recycleBinModel, &BusinessLayer::RecycleBinModel::emptyRecycleBinRequested,
                        this, &ProjectModelsFacade::emptyRecycleBinRequested);

                model = recycleBinModel;
                break;
            }

            case Domain::DocumentObjectType::Screenplay: {
                const auto screenplayItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (screenplayItem == nullptr) {
                    return nullptr;
                }

                //
                // Если сценарий является эпизодом сериала, нужно подгрузить информацию о сериале
                //
                if (screenplayItem->parent() != nullptr
                    && screenplayItem->parent()->type()
                        == Domain::DocumentObjectType::ScreenplaySeriesEpisodes) {
                    documentsToLoad.append(
                        DataStorageLayer::StorageFacade::documentStorage()->document(
                            screenplayItem->parent()->uuid()));
                }

                auto screenplayModel = new BusinessLayer::ScreenplayInformationModel;
                connect(screenplayModel,
                        &BusinessLayer::ScreenplayInformationModel::titlePageVisibleChanged, this,
                        [this, screenplayModel](bool _visible) {
                            emit screenplayTitlePageVisibilityChanged(screenplayModel, _visible);
                        });
                connect(screenplayModel,
                        &BusinessLayer::ScreenplayInformationModel::synopsisVisibleChanged, this,
                        [this, screenplayModel](bool _visible) {
                            emit screenplaySynopsisVisibilityChanged(screenplayModel, _visible);
                        });
                connect(screenplayModel,
                        &BusinessLayer::ScreenplayInformationModel::treatmentVisibleChanged, this,
                        [this, screenplayModel](bool _visible) {
                            emit screenplayTreatmentVisibilityChanged(screenplayModel, _visible);
                        });
                connect(screenplayModel,
                        &BusinessLayer::ScreenplayInformationModel::screenplayTextVisibleChanged,
                        this, [this, screenplayModel](bool _visible) {
                            emit screenplayTextVisibilityChanged(screenplayModel, _visible);
                        });
                connect(
                    screenplayModel,
                    &BusinessLayer::ScreenplayInformationModel::screenplayStatisticsVisibleChanged,
                    this, [this, screenplayModel](bool _visible) {
                        emit screenplayStatisticsVisibilityChanged(screenplayModel, _visible);
                    });

                model = screenplayModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayTitlePage: {
                const auto titlePageItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (titlePageItem == nullptr) {
                    return nullptr;
                }

                Q_ASSERT(titlePageItem->parent());

                auto titlePageModel = new BusinessLayer::ScreenplayTitlePageModel;
                const auto parentUuid = titlePageItem->parent()->uuid();

                //
                // Добавляем в модель титульной страницы, модель информации о сценарие
                //
                auto informationModel = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(
                    modelFor(parentUuid));
                titlePageModel->setInformationModel(informationModel);

                model = titlePageModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplaySynopsis: {
                const auto synopsisItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (synopsisItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(synopsisItem->parent());

                auto synopsisModel = new BusinessLayer::ScreenplaySynopsisModel;
                const auto parentUuid = synopsisItem->parent()->uuid();

                //
                // Добавляем в модель синопсиса, модель информации о сценарие
                //
                auto informationModel = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(
                    modelFor(parentUuid));
                synopsisModel->setInformationModel(informationModel);

                model = synopsisModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayTreatment: {
                //
                // Модель по сути является алиасом к тексту сценария, поэтому используем модель
                // сценария
                //
                isDocumentAlias = true;

                const auto treatmentItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (treatmentItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(treatmentItem->parent());
                //
                // ... модель сценария
                //
                BusinessLayer::StructureModelItem* screenplayItem = nullptr;
                for (int index = 0; index < treatmentItem->parent()->childCount(); ++index) {
                    auto childItem = treatmentItem->parent()->childAt(index);
                    if (childItem->type() == Domain::DocumentObjectType::ScreenplayText) {
                        screenplayItem = childItem;
                        break;
                    }
                }
                Q_ASSERT(screenplayItem);
                model = modelFor(screenplayItem->uuid());
                //
                // ... если не удалось получить модель сценария, то значит она ещё не была загружена
                // из
                //     облака, возвращаем тут пустую модель, т.к. это нормальная штатная ситуация
                //
                if (model == nullptr) {
                    return nullptr;
                }

                break;
            }

            case Domain::DocumentObjectType::ScreenplayText: {
                const auto screenplayItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (screenplayItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(screenplayItem->parent());

                auto screenplayModel = new BusinessLayer::ScreenplayTextModel;
                const auto parentUuid = screenplayItem->parent()->uuid();

                //
                // Добавляем в модель сценария, модель информации о сценарие
                //
                auto informationModel = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(
                    modelFor(parentUuid));
                Q_ASSERT(informationModel);
                screenplayModel->setInformationModel(informationModel);
                //
                // ... модель титульной страницы
                //
                const auto titlePageIndex = 0;
                auto titlePageItem = screenplayItem->parent()->childAt(titlePageIndex);
                Q_ASSERT(titlePageItem);
                Q_ASSERT(titlePageItem->type() == Domain::DocumentObjectType::ScreenplayTitlePage);
                auto titlePageModel = qobject_cast<BusinessLayer::SimpleTextModel*>(
                    modelFor(titlePageItem->uuid()));
                Q_ASSERT(titlePageModel);
                screenplayModel->setTitlePageModel(titlePageModel);
                //
                // ... модель синопсиса
                //
                const auto synopsisIndex = 1;
                auto synopsisItem = screenplayItem->parent()->childAt(synopsisIndex);
                Q_ASSERT(synopsisItem);
                Q_ASSERT(synopsisItem->type() == Domain::DocumentObjectType::ScreenplaySynopsis);
                auto synopsisModel
                    = qobject_cast<BusinessLayer::SimpleTextModel*>(modelFor(synopsisItem->uuid()));
                Q_ASSERT(synopsisModel);
                screenplayModel->setSynopsisModel(synopsisModel);
                //
                // ... модель справочников сценариев
                //
                auto dictionariesModel = qobject_cast<BusinessLayer::ScreenplayDictionariesModel*>(
                    modelFor(Domain::DocumentObjectType::ScreenplayDictionaries));
                Q_ASSERT(dictionariesModel);
                screenplayModel->setDictionariesModel(dictionariesModel);
                //
                // ... модель персонажей
                //
                auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                    modelFor(Domain::DocumentObjectType::Characters));
                Q_ASSERT(charactersModel);
                screenplayModel->setCharactersModel(charactersModel);
                //
                // ... и модель локаций
                //
                auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(
                    modelFor(Domain::DocumentObjectType::Locations));
                Q_ASSERT(locationsModel);
                screenplayModel->setLocationsModel(locationsModel);
                //
                // ... обновим словари рантайма
                //
                screenplayModel->updateRuntimeDictionariesIfNeeded();

                model = screenplayModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayDictionaries: {
                model = new BusinessLayer::ScreenplayDictionariesModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplayStatistics: {
                const auto statisticsItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (statisticsItem == nullptr) {
                    return nullptr;
                }

                auto statisticsModel = new BusinessLayer::ScreenplayStatisticsModel;
                const auto screenplayItem = statisticsItem->parent();
                Q_ASSERT(screenplayItem);
                QUuid screenplayTextItemUuid;
                for (int childIndex = 0; childIndex < screenplayItem->childCount(); ++childIndex) {
                    const auto childItem = screenplayItem->childAt(childIndex);
                    if (childItem->type() == Domain::DocumentObjectType::ScreenplayText) {
                        screenplayTextItemUuid = childItem->uuid();
                        break;
                    }
                }
                Q_ASSERT(!screenplayTextItemUuid.isNull());
                auto screenplayModel = qobject_cast<BusinessLayer::ScreenplayTextModel*>(
                    modelFor(screenplayTextItemUuid));
                statisticsModel->setScreenplayTextModel(screenplayModel);

                model = statisticsModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplaySeries: {
                auto seriesModel = new BusinessLayer::ScreenplaySeriesInformationModel;
                connect(seriesModel,
                        &BusinessLayer::ScreenplaySeriesInformationModel::titlePageVisibleChanged,
                        this, [this, seriesModel](bool _visible) {
                            emit screenplaySeriesTitlePageVisibilityChanged(seriesModel, _visible);
                        });
                connect(seriesModel,
                        &BusinessLayer::ScreenplaySeriesInformationModel::synopsisVisibleChanged,
                        this, [this, seriesModel](bool _visible) {
                            emit screenplaySeriesSynopsisVisibilityChanged(seriesModel, _visible);
                        });
                connect(seriesModel,
                        &BusinessLayer::ScreenplaySeriesInformationModel::treatmentVisibleChanged,
                        this, [this, seriesModel](bool _visible) {
                            emit screenplaySeriesTreatmentVisibilityChanged(seriesModel, _visible);
                        });
                connect(
                    seriesModel,
                    &BusinessLayer::ScreenplaySeriesInformationModel::screenplayTextVisibleChanged,
                    this, [this, seriesModel](bool _visible) {
                        emit screenplaySeriesTextVisibilityChanged(seriesModel, _visible);
                    });
                connect(seriesModel,
                        &BusinessLayer::ScreenplaySeriesInformationModel::
                            screenplayStatisticsVisibleChanged,
                        this, [this, seriesModel](bool _visible) {
                            emit screenplaySeriesStatisticsVisibilityChanged(seriesModel, _visible);
                        });

                model = seriesModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplaySeriesEpisodes: {
                const auto episodesItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (episodesItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(episodesItem->parent());

                auto episodesModel = new BusinessLayer::ScreenplaySeriesEpisodesModel;
                const auto parentUuid = episodesItem->parent()->uuid();

                //
                // Добавляем в модель эпизодов, модель информации о сериале
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::ScreenplaySeriesInformationModel*>(
                        modelFor(parentUuid));
                Q_ASSERT(informationModel);
                episodesModel->setInformationModel(informationModel);

                //
                // Мониторим список вложенных сценариев
                //
                auto updateEpisodes = [this, episodesModel, documentUuid = documentToLoad->uuid()] {
                    auto episodesItem = d->projectStructureModel->itemForUuid(documentUuid);
                    if (episodesItem == nullptr) {
                        return;
                    }

                    QVector<BusinessLayer::ScreenplayTextModel*> episodes;
                    for (int childIndex = 0; childIndex < episodesItem->childCount();
                         ++childIndex) {
                        const auto screenplayItem = episodesItem->childAt(childIndex);
                        for (int childIndex = 0; childIndex < screenplayItem->childCount();
                             ++childIndex) {
                            const auto childItem = screenplayItem->childAt(childIndex);
                            if (childItem->type() == Domain::DocumentObjectType::ScreenplayText) {
                                const auto screenplayTextItemUuid = childItem->uuid();
                                Q_ASSERT(!screenplayTextItemUuid.isNull());
                                episodes.append(qobject_cast<BusinessLayer::ScreenplayTextModel*>(
                                    modelFor(screenplayTextItemUuid)));
                                break;
                            }
                        }
                    }

                    episodesModel->setEpisodes(episodes);
                };
                updateEpisodes();
                connect(d->projectStructureModel, &BusinessLayer::StructureModel::rowsInserted,
                        episodesModel, updateEpisodes);
                connect(d->projectStructureModel, &BusinessLayer::StructureModel::rowsMoved,
                        episodesModel, updateEpisodes);
                connect(d->projectStructureModel, &BusinessLayer::StructureModel::rowsRemoved,
                        episodesModel, updateEpisodes);

                model = episodesModel;
                break;
            }

            case Domain::DocumentObjectType::ScreenplaySeriesStatistics: {
                const auto statisticsItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (statisticsItem == nullptr) {
                    return nullptr;
                }

                auto statisticsModel = new BusinessLayer::ScreenplaySeriesStatisticsModel;
                const auto seriesItem = statisticsItem->parent();
                Q_ASSERT(seriesItem);
                QUuid episodesItemUuid;
                for (int childIndex = 0; childIndex < seriesItem->childCount(); ++childIndex) {
                    const auto childItem = seriesItem->childAt(childIndex);
                    if (childItem->type() == Domain::DocumentObjectType::ScreenplaySeriesEpisodes) {
                        episodesItemUuid = childItem->uuid();
                        break;
                    }
                }
                Q_ASSERT(!episodesItemUuid.isNull());
                auto episodesModel = qobject_cast<BusinessLayer::ScreenplaySeriesEpisodesModel*>(
                    modelFor(episodesItemUuid));
                statisticsModel->setEpisodesModel(episodesModel);

                model = statisticsModel;
                break;
            }

            case Domain::DocumentObjectType::ComicBook: {
                auto comicBookModel = new BusinessLayer::ComicBookInformationModel;
                connect(comicBookModel,
                        &BusinessLayer::ComicBookInformationModel::titlePageVisibleChanged, this,
                        [this, comicBookModel](bool _visible) {
                            emit comicBookTitlePageVisibilityChanged(comicBookModel, _visible);
                        });
                connect(comicBookModel,
                        &BusinessLayer::ComicBookInformationModel::synopsisVisibleChanged, this,
                        [this, comicBookModel](bool _visible) {
                            emit comicBookSynopsisVisibilityChanged(comicBookModel, _visible);
                        });
                connect(comicBookModel,
                        &BusinessLayer::ComicBookInformationModel::comicBookTextVisibleChanged,
                        this, [this, comicBookModel](bool _visible) {
                            emit comicBookTextVisibilityChanged(comicBookModel, _visible);
                        });
                connect(
                    comicBookModel,
                    &BusinessLayer::ComicBookInformationModel::comicBookStatisticsVisibleChanged,
                    this, [this, comicBookModel](bool _visible) {
                        emit comicBookStatisticsVisibilityChanged(comicBookModel, _visible);
                    });

                model = comicBookModel;
                break;
            }

            case Domain::DocumentObjectType::ComicBookTitlePage: {
                const auto titlePageItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (titlePageItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(titlePageItem->parent());

                auto titlePageModel = new BusinessLayer::ComicBookTitlePageModel;
                const auto parentUuid = titlePageItem->parent()->uuid();

                //
                // Добавляем в модель титульной страницы, модель информации о комиксе
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::ComicBookInformationModel*>(modelFor(parentUuid));
                titlePageModel->setInformationModel(informationModel);

                model = titlePageModel;
                break;
            }

            case Domain::DocumentObjectType::ComicBookSynopsis: {
                const auto sinopsisItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (sinopsisItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(sinopsisItem->parent());

                auto synopsisModel = new BusinessLayer::ComicBookSynopsisModel;
                const auto parentUuid = sinopsisItem->parent()->uuid();

                //
                // Добавляем в модель титульной страницы, модель информации о комиксе
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::ComicBookInformationModel*>(modelFor(parentUuid));
                synopsisModel->setInformationModel(informationModel);

                model = synopsisModel;
                break;
            }

            case Domain::DocumentObjectType::ComicBookText: {
                const auto comicBookItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (comicBookItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(comicBookItem->parent());

                auto comicBookModel = new BusinessLayer::ComicBookTextModel;
                const auto parentUuid = comicBookItem->parent()->uuid();

                //
                // Добавляем в модель комикса, модель информации о комиксе
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::ComicBookInformationModel*>(modelFor(parentUuid));
                comicBookModel->setInformationModel(informationModel);
                //
                // ... модель титульной страницы
                //
                const auto titlePageIndex = 0;
                auto titlePageItem = comicBookItem->parent()->childAt(titlePageIndex);
                Q_ASSERT(titlePageItem);
                Q_ASSERT(titlePageItem->type() == Domain::DocumentObjectType::ComicBookTitlePage);
                auto titlePageModel = qobject_cast<BusinessLayer::SimpleTextModel*>(
                    modelFor(titlePageItem->uuid()));
                comicBookModel->setTitlePageModel(titlePageModel);
                //
                // ... модель синопсиса
                //
                const auto synopsisIndex = 1;
                auto synopsisItem = comicBookItem->parent()->childAt(synopsisIndex);
                Q_ASSERT(synopsisItem);
                Q_ASSERT(synopsisItem->type() == Domain::DocumentObjectType::ComicBookSynopsis);
                auto synopsisModel
                    = qobject_cast<BusinessLayer::SimpleTextModel*>(modelFor(synopsisItem->uuid()));
                comicBookModel->setSynopsisModel(synopsisModel);
                //
                // ... модель справочников
                //
                auto dictionariesModel = qobject_cast<BusinessLayer::ComicBookDictionariesModel*>(
                    modelFor(Domain::DocumentObjectType::ComicBookDictionaries));
                comicBookModel->setDictionariesModel(dictionariesModel);
                //
                // ... модель персонажей
                //
                auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                    modelFor(Domain::DocumentObjectType::Characters));
                comicBookModel->setCharactersModel(charactersModel);
                //
                // ... обновим словари рантайма
                //
                comicBookModel->updateRuntimeDictionariesIfNeeded();

                model = comicBookModel;
                break;
            }

            case Domain::DocumentObjectType::ComicBookDictionaries: {
                model = new BusinessLayer::ComicBookDictionariesModel;
                break;
            }

            case Domain::DocumentObjectType::ComicBookStatistics: {
                const auto statisticsItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (statisticsItem == nullptr) {
                    return nullptr;
                }

                auto statisticsModel = new BusinessLayer::ComicBookStatisticsModel;
                const auto comicBookItem = statisticsItem->parent();
                Q_ASSERT(comicBookItem);
                QUuid comicBookTextItemUuid;
                for (int childIndex = 0; childIndex < comicBookItem->childCount(); ++childIndex) {
                    const auto childItem = comicBookItem->childAt(childIndex);
                    if (childItem->type() == Domain::DocumentObjectType::ComicBookText) {
                        comicBookTextItemUuid = childItem->uuid();
                        break;
                    }
                }
                Q_ASSERT(!comicBookTextItemUuid.isNull());
                auto comicBookModel = qobject_cast<BusinessLayer::ComicBookTextModel*>(
                    modelFor(comicBookTextItemUuid));
                statisticsModel->setComicBookTextModel(comicBookModel);

                model = statisticsModel;
                break;
            }

            case Domain::DocumentObjectType::Audioplay: {
                auto audioplayModel = new BusinessLayer::AudioplayInformationModel;
                connect(audioplayModel,
                        &BusinessLayer::AudioplayInformationModel::titlePageVisibleChanged, this,
                        [this, audioplayModel](bool _visible) {
                            emit audioplayTitlePageVisibilityChanged(audioplayModel, _visible);
                        });
                connect(audioplayModel,
                        &BusinessLayer::AudioplayInformationModel::synopsisVisibleChanged, this,
                        [this, audioplayModel](bool _visible) {
                            emit audioplaySynopsisVisibilityChanged(audioplayModel, _visible);
                        });
                connect(audioplayModel,
                        &BusinessLayer::AudioplayInformationModel::audioplayTextVisibleChanged,
                        this, [this, audioplayModel](bool _visible) {
                            emit audioplayTextVisibilityChanged(audioplayModel, _visible);
                        });
                connect(
                    audioplayModel,
                    &BusinessLayer::AudioplayInformationModel::audioplayStatisticsVisibleChanged,
                    this, [this, audioplayModel](bool _visible) {
                        emit audioplayStatisticsVisibilityChanged(audioplayModel, _visible);
                    });

                model = audioplayModel;
                break;
            }

            case Domain::DocumentObjectType::AudioplayTitlePage: {
                const auto titlePageItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (titlePageItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(titlePageItem->parent());

                auto titlePageModel = new BusinessLayer::AudioplayTitlePageModel;
                const auto parentUuid = titlePageItem->parent()->uuid();

                //
                // Добавляем в модель титульной страницы, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::AudioplayInformationModel*>(modelFor(parentUuid));
                titlePageModel->setInformationModel(informationModel);

                connect(titlePageModel,
                        &BusinessLayer::AudioplayTitlePageModel::charactersUpdateRequested, this,
                        [this, titlePageModel] {
                            emit titlePageCharactersUpdateRequested(titlePageModel);
                        });

                model = titlePageModel;
                break;
            }

            case Domain::DocumentObjectType::AudioplaySynopsis: {
                const auto synopsisItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (synopsisItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(synopsisItem->parent());

                auto synopsisModel = new BusinessLayer::AudioplaySynopsisModel;
                const auto parentUuid = synopsisItem->parent()->uuid();

                //
                // Добавляем в модель синопсиса, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::AudioplayInformationModel*>(modelFor(parentUuid));
                synopsisModel->setInformationModel(informationModel);

                model = synopsisModel;
                break;
            }

            case Domain::DocumentObjectType::AudioplayText: {
                const auto audioplayItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (audioplayItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(audioplayItem->parent());

                auto audioplayModel = new BusinessLayer::AudioplayTextModel;
                const auto parentUuid = audioplayItem->parent()->uuid();

                //
                // Добавляем в модель сценария, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::AudioplayInformationModel*>(modelFor(parentUuid));
                audioplayModel->setInformationModel(informationModel);
                //
                // ... модель титульной страницы
                //
                const auto titlePageIndex = 0;
                auto titlePageItem = audioplayItem->parent()->childAt(titlePageIndex);
                Q_ASSERT(titlePageItem);
                Q_ASSERT(titlePageItem->type() == Domain::DocumentObjectType::AudioplayTitlePage);
                auto titlePageModel = qobject_cast<BusinessLayer::SimpleTextModel*>(
                    modelFor(titlePageItem->uuid()));
                audioplayModel->setTitlePageModel(titlePageModel);
                //
                // ... модель синопсиса
                //
                const auto synopsisIndex = 1;
                auto synopsisItem = audioplayItem->parent()->childAt(synopsisIndex);
                Q_ASSERT(synopsisItem);
                Q_ASSERT(synopsisItem->type() == Domain::DocumentObjectType::AudioplaySynopsis);
                auto synopsisModel
                    = qobject_cast<BusinessLayer::SimpleTextModel*>(modelFor(synopsisItem->uuid()));
                audioplayModel->setSynopsisModel(synopsisModel);
                //
                // ... модель персонажей
                //
                auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                    modelFor(Domain::DocumentObjectType::Characters));
                audioplayModel->setCharactersModel(charactersModel);
                //
                // ... обновим словари рантайма
                //
                audioplayModel->updateRuntimeDictionariesIfNeeded();

                model = audioplayModel;
                break;
            }

            case Domain::DocumentObjectType::AudioplayStatistics: {
                const auto statisticsItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (statisticsItem == nullptr) {
                    return nullptr;
                }

                auto statisticsModel = new BusinessLayer::AudioplayStatisticsModel;
                const auto audioplayItem = statisticsItem->parent();
                Q_ASSERT(audioplayItem);
                QUuid audioplayTextItemUuid;
                for (int childIndex = 0; childIndex < audioplayItem->childCount(); ++childIndex) {
                    const auto childItem = audioplayItem->childAt(childIndex);
                    if (childItem->type() == Domain::DocumentObjectType::AudioplayText) {
                        audioplayTextItemUuid = childItem->uuid();
                        break;
                    }
                }
                Q_ASSERT(!audioplayTextItemUuid.isNull());
                auto audioplayModel = qobject_cast<BusinessLayer::AudioplayTextModel*>(
                    modelFor(audioplayTextItemUuid));
                statisticsModel->setAudioplayTextModel(audioplayModel);

                model = statisticsModel;
                break;
            }

            case Domain::DocumentObjectType::Stageplay: {
                auto stageplayModel = new BusinessLayer::StageplayInformationModel;
                connect(stageplayModel,
                        &BusinessLayer::StageplayInformationModel::titlePageVisibleChanged, this,
                        [this, stageplayModel](bool _visible) {
                            emit stageplayTitlePageVisibilityChanged(stageplayModel, _visible);
                        });
                connect(stageplayModel,
                        &BusinessLayer::StageplayInformationModel::synopsisVisibleChanged, this,
                        [this, stageplayModel](bool _visible) {
                            emit stageplaySynopsisVisibilityChanged(stageplayModel, _visible);
                        });
                connect(stageplayModel,
                        &BusinessLayer::StageplayInformationModel::stageplayTextVisibleChanged,
                        this, [this, stageplayModel](bool _visible) {
                            emit stageplayTextVisibilityChanged(stageplayModel, _visible);
                        });
                connect(
                    stageplayModel,
                    &BusinessLayer::StageplayInformationModel::stageplayStatisticsVisibleChanged,
                    this, [this, stageplayModel](bool _visible) {
                        emit stageplayStatisticsVisibilityChanged(stageplayModel, _visible);
                    });

                model = stageplayModel;
                break;
            }

            case Domain::DocumentObjectType::StageplayTitlePage: {
                const auto titlePageItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (titlePageItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(titlePageItem->parent());

                auto titlePageModel = new BusinessLayer::StageplayTitlePageModel;
                const auto parentUuid = titlePageItem->parent()->uuid();

                //
                // Добавляем в модель титульной страницы, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::StageplayInformationModel*>(modelFor(parentUuid));
                titlePageModel->setInformationModel(informationModel);

                connect(titlePageModel,
                        &BusinessLayer::StageplayTitlePageModel::charactersUpdateRequested, this,
                        [this, titlePageModel] {
                            emit titlePageCharactersUpdateRequested(titlePageModel);
                        });

                model = titlePageModel;
                break;
            }

            case Domain::DocumentObjectType::StageplaySynopsis: {
                const auto synopsisItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (synopsisItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(synopsisItem->parent());

                auto synopsisModel = new BusinessLayer::StageplaySynopsisModel;
                const auto parentUuid = synopsisItem->parent()->uuid();

                //
                // Добавляем в модель синопсиса, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::StageplayInformationModel*>(modelFor(parentUuid));
                synopsisModel->setInformationModel(informationModel);

                model = synopsisModel;
                break;
            }

            case Domain::DocumentObjectType::StageplayText: {
                const auto stageplayItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (stageplayItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(stageplayItem->parent());

                auto stageplayModel = new BusinessLayer::StageplayTextModel;
                const auto parentUuid = stageplayItem->parent()->uuid();

                //
                // Добавляем в модель сценария, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::StageplayInformationModel*>(modelFor(parentUuid));
                stageplayModel->setInformationModel(informationModel);
                //
                // ... модель титульной страницы
                //
                const auto titlePageIndex = 0;
                auto titlePageItem = stageplayItem->parent()->childAt(titlePageIndex);
                Q_ASSERT(titlePageItem);
                Q_ASSERT(titlePageItem->type() == Domain::DocumentObjectType::StageplayTitlePage);
                auto titlePageModel = qobject_cast<BusinessLayer::SimpleTextModel*>(
                    modelFor(titlePageItem->uuid()));
                stageplayModel->setTitlePageModel(titlePageModel);
                //
                // ... модель синопсиса
                //
                const auto synopsisIndex = 1;
                auto synopsisItem = stageplayItem->parent()->childAt(synopsisIndex);
                Q_ASSERT(synopsisItem);
                Q_ASSERT(synopsisItem->type() == Domain::DocumentObjectType::StageplaySynopsis);
                auto synopsisModel
                    = qobject_cast<BusinessLayer::SimpleTextModel*>(modelFor(synopsisItem->uuid()));
                stageplayModel->setSynopsisModel(synopsisModel);
                //
                // ... модель персонажей
                //
                auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                    modelFor(Domain::DocumentObjectType::Characters));
                stageplayModel->setCharactersModel(charactersModel);
                //
                // ... обновим словари рантайма
                //
                stageplayModel->updateRuntimeDictionariesIfNeeded();

                model = stageplayModel;
                break;
            }

            case Domain::DocumentObjectType::StageplayStatistics: {
                const auto statisticsItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (statisticsItem == nullptr) {
                    return nullptr;
                }

                auto statisticsModel = new BusinessLayer::StageplayStatisticsModel;
                const auto stageplayItem = statisticsItem->parent();
                Q_ASSERT(stageplayItem);
                QUuid stageplayTextItemUuid;
                for (int childIndex = 0; childIndex < stageplayItem->childCount(); ++childIndex) {
                    const auto childItem = stageplayItem->childAt(childIndex);
                    if (childItem->type() == Domain::DocumentObjectType::StageplayText) {
                        stageplayTextItemUuid = childItem->uuid();
                        break;
                    }
                }
                Q_ASSERT(!stageplayTextItemUuid.isNull());
                auto stageplayModel = qobject_cast<BusinessLayer::StageplayTextModel*>(
                    modelFor(stageplayTextItemUuid));
                statisticsModel->setStageplayTextModel(stageplayModel);

                model = statisticsModel;
                break;
            }

            case Domain::DocumentObjectType::Novel: {
                auto novelModel = new BusinessLayer::NovelInformationModel;
                connect(novelModel, &BusinessLayer::NovelInformationModel::titlePageVisibleChanged,
                        this, [this, novelModel](bool _visible) {
                            emit novelTitlePageVisibilityChanged(novelModel, _visible);
                        });
                connect(novelModel, &BusinessLayer::NovelInformationModel::synopsisVisibleChanged,
                        this, [this, novelModel](bool _visible) {
                            emit novelSynopsisVisibilityChanged(novelModel, _visible);
                        });
                connect(novelModel, &BusinessLayer::NovelInformationModel::outlineVisibleChanged,
                        this, [this, novelModel](bool _visible) {
                            emit novelOutlineVisibilityChanged(novelModel, _visible);
                        });
                connect(novelModel, &BusinessLayer::NovelInformationModel::novelTextVisibleChanged,
                        this, [this, novelModel](bool _visible) {
                            emit novelTextVisibilityChanged(novelModel, _visible);
                        });
                connect(novelModel,
                        &BusinessLayer::NovelInformationModel::novelStatisticsVisibleChanged, this,
                        [this, novelModel](bool _visible) {
                            emit novelStatisticsVisibilityChanged(novelModel, _visible);
                        });

                model = novelModel;
                break;
            }

            case Domain::DocumentObjectType::NovelTitlePage: {
                const auto titlePageItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (titlePageItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(titlePageItem->parent());

                auto titlePageModel = new BusinessLayer::NovelTitlePageModel;
                const auto parentUuid = titlePageItem->parent()->uuid();

                //
                // Добавляем в модель титульной страницы, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::NovelInformationModel*>(modelFor(parentUuid));
                titlePageModel->setInformationModel(informationModel);

                model = titlePageModel;
                break;
            }

            case Domain::DocumentObjectType::NovelSynopsis: {
                const auto synopsisItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (synopsisItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(synopsisItem->parent());

                auto synopsisModel = new BusinessLayer::NovelSynopsisModel;
                const auto parentUuid = synopsisItem->parent()->uuid();

                //
                // Добавляем в модель синопсиса, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::NovelInformationModel*>(modelFor(parentUuid));
                synopsisModel->setInformationModel(informationModel);

                model = synopsisModel;
                break;
            }

            case Domain::DocumentObjectType::NovelOutline: {
                //
                // Модель по сути является алиасом к тексту сценария, поэтому используем модель
                // сценария
                //
                isDocumentAlias = true;

                const auto treatmentItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (treatmentItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(treatmentItem->parent());

                //
                // ... модель сценария
                //
                BusinessLayer::StructureModelItem* novelItem = nullptr;
                for (int index = 0; index < treatmentItem->parent()->childCount(); ++index) {
                    auto childItem = treatmentItem->parent()->childAt(index);
                    if (childItem->type() == Domain::DocumentObjectType::NovelText) {
                        novelItem = childItem;
                        break;
                    }
                }
                Q_ASSERT(novelItem);
                model = modelFor(novelItem->uuid());
                //
                // ... если не удалось получить модель романа, то значит она ещё не была загружена
                // из
                //     облака, возвращаем тут пустую модель, т.к. это нормальная штатная ситуация
                //
                if (model == nullptr) {
                    return nullptr;
                }

                break;
            }

            case Domain::DocumentObjectType::NovelText: {
                const auto novelItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (novelItem == nullptr) {
                    return nullptr;
                }
                Q_ASSERT(novelItem->parent());

                auto novelModel = new BusinessLayer::NovelTextModel;
                const auto parentUuid = novelItem->parent()->uuid();

                //
                // Добавляем в модель сценария, модель информации о сценарие
                //
                auto informationModel
                    = qobject_cast<BusinessLayer::NovelInformationModel*>(modelFor(parentUuid));
                Q_ASSERT(informationModel);
                novelModel->setInformationModel(informationModel);
                //
                // ... модель титульной страницы
                //
                const auto titlePageIndex = 0;
                auto titlePageItem = novelItem->parent()->childAt(titlePageIndex);
                Q_ASSERT(titlePageItem);
                Q_ASSERT(titlePageItem->type() == Domain::DocumentObjectType::NovelTitlePage);
                auto titlePageModel = qobject_cast<BusinessLayer::SimpleTextModel*>(
                    modelFor(titlePageItem->uuid()));
                Q_ASSERT(titlePageModel);
                novelModel->setTitlePageModel(titlePageModel);
                //
                // ... модель синопсиса
                //
                const auto synopsisIndex = 1;
                auto synopsisItem = novelItem->parent()->childAt(synopsisIndex);
                Q_ASSERT(synopsisItem);
                Q_ASSERT(synopsisItem->type() == Domain::DocumentObjectType::NovelSynopsis);
                auto synopsisModel
                    = qobject_cast<BusinessLayer::SimpleTextModel*>(modelFor(synopsisItem->uuid()));
                Q_ASSERT(synopsisModel);
                novelModel->setSynopsisModel(synopsisModel);
                //
                // ... модель справочников сценариев
                //
                auto dictionariesModel = qobject_cast<BusinessLayer::NovelDictionariesModel*>(
                    modelFor(Domain::DocumentObjectType::NovelDictionaries));
                Q_ASSERT(dictionariesModel);
                novelModel->setDictionariesModel(dictionariesModel);
                //
                // ... модель персонажей
                //
                auto charactersModel = qobject_cast<BusinessLayer::CharactersModel*>(
                    modelFor(Domain::DocumentObjectType::Characters));
                Q_ASSERT(charactersModel);
                novelModel->setCharactersModel(charactersModel);
                //
                // ... и модель локаций
                //
                auto locationsModel = qobject_cast<BusinessLayer::LocationsModel*>(
                    modelFor(Domain::DocumentObjectType::Locations));
                Q_ASSERT(locationsModel);
                novelModel->setLocationsModel(locationsModel);

                model = novelModel;
                break;
            }

            case Domain::DocumentObjectType::NovelDictionaries: {
                model = new BusinessLayer::NovelDictionariesModel;
                break;
            }

            case Domain::DocumentObjectType::NovelStatistics: {
                const auto statisticsItem
                    = d->projectStructureModel->itemForUuid(documentToLoad->uuid());
                if (statisticsItem == nullptr) {
                    return nullptr;
                }

                auto statisticsModel = new BusinessLayer::NovelStatisticsModel;
                const auto novelItem = statisticsItem->parent();
                Q_ASSERT(novelItem);
                QUuid novelTextItemUuid;
                for (int childIndex = 0; childIndex < novelItem->childCount(); ++childIndex) {
                    const auto childItem = novelItem->childAt(childIndex);
                    if (childItem->type() == Domain::DocumentObjectType::NovelText) {
                        novelTextItemUuid = childItem->uuid();
                        break;
                    }
                }
                Q_ASSERT(!novelTextItemUuid.isNull());
                auto novelModel
                    = qobject_cast<BusinessLayer::NovelTextModel*>(modelFor(novelTextItemUuid));
                statisticsModel->setNovelTextModel(novelModel);

                model = statisticsModel;
                break;
            }

            case Domain::DocumentObjectType::Characters: {
                auto charactersModel = new BusinessLayer::CharactersModel;

                const auto characterDocuments
                    = DataStorageLayer::StorageFacade::documentStorage()->documents(
                        Domain::DocumentObjectType::Character);
                for (const auto characterDocument : characterDocuments) {
                    const auto characterItem
                        = d->projectStructureModel->itemForUuid(characterDocument->uuid());
                    if (characterItem == nullptr
                        || characterItem->parent()->uuid() != documentToLoad->uuid()) {
                        continue;
                    }

                    auto characterModel = modelFor(characterDocument);
                    charactersModel->addCharacterModel(
                        qobject_cast<BusinessLayer::CharacterModel*>(characterModel));
                }

                connect(charactersModel, &BusinessLayer::CharactersModel::createCharacterRequested,
                        this, &ProjectModelsFacade::createCharacterRequested);

                model = charactersModel;
                break;
            }

            case Domain::DocumentObjectType::Character: {
                auto characterModel = new BusinessLayer::CharacterModel;

                connect(characterModel, &BusinessLayer::CharacterModel::nameChanged, this,
                        [this, characterModel](const QString& _oldName, const QString& _newName) {
                            emit characterNameChanged(characterModel, _oldName, _newName);
                        });
                connect(characterModel, &BusinessLayer::CharacterModel::dialoguesUpdateRequested,
                        this, [this, characterModel] {
                            emit characterDialoguesUpdateRequested(characterModel);
                        });

                model = characterModel;
                break;
            }

            case Domain::DocumentObjectType::Locations: {
                auto locationsModel = new BusinessLayer::LocationsModel;

                const auto locationDocuments
                    = DataStorageLayer::StorageFacade::documentStorage()->documents(
                        Domain::DocumentObjectType::Location);
                for (const auto locationDocument : locationDocuments) {
                    const auto locationItem
                        = d->projectStructureModel->itemForUuid(locationDocument->uuid());
                    if (locationItem == nullptr
                        || locationItem->parent()->uuid() != documentToLoad->uuid()) {
                        continue;
                    }

                    auto locationModel = modelFor(locationDocument);
                    locationsModel->addLocationModel(
                        qobject_cast<BusinessLayer::LocationModel*>(locationModel));
                }

                connect(locationsModel, &BusinessLayer::LocationsModel::createLocationRequested,
                        this, &ProjectModelsFacade::createLocationRequested);

                model = locationsModel;
                break;
            }

            case Domain::DocumentObjectType::Location: {
                auto locationModel = new BusinessLayer::LocationModel;

                connect(locationModel, &BusinessLayer::LocationModel::nameChanged, this,
                        [this, locationModel](const QString& _oldName, const QString& _newName) {
                            emit locationNameChanged(locationModel, _oldName, _newName);
                        });
                connect(
                    locationModel, &BusinessLayer::LocationModel::scenesUpdateRequested, this,
                    [this, locationModel] { emit locationScenesUpdateRequested(locationModel); });

                model = locationModel;
                break;
            }

            case Domain::DocumentObjectType::Worlds: {
                auto worldsModel = new BusinessLayer::WorldsModel;

                const auto worldDocuments
                    = DataStorageLayer::StorageFacade::documentStorage()->documents(
                        Domain::DocumentObjectType::World);
                for (const auto worldDocument : worldDocuments) {
                    const auto worldItem
                        = d->projectStructureModel->itemForUuid(worldDocument->uuid());
                    if (worldItem == nullptr
                        || worldItem->parent()->uuid() != documentToLoad->uuid()) {
                        continue;
                    }

                    auto worldModel = modelFor(worldDocument);
                    worldsModel->addWorldModel(
                        qobject_cast<BusinessLayer::WorldModel*>(worldModel));
                }

                connect(worldsModel, &BusinessLayer::WorldsModel::createWorldRequested, this,
                        &ProjectModelsFacade::createWorldRequested);

                model = worldsModel;
                break;
            }

            case Domain::DocumentObjectType::World: {
                model = new BusinessLayer::WorldModel;
                break;
            }

            case Domain::DocumentObjectType::Folder:
            case Domain::DocumentObjectType::SimpleText: {
                model = new BusinessLayer::SimpleTextModel;
                break;
            }

            case Domain::DocumentObjectType::MindMap: {
                model = new BusinessLayer::MindMapModel;
                break;
            }

            case Domain::DocumentObjectType::ImagesGallery: {
                model = new BusinessLayer::ImagesGalleryModel;
                break;
            }

            case Domain::DocumentObjectType::Presentation: {
                model = new BusinessLayer::PresentationModel;
                break;
            }

            default: {
                Q_ASSERT(false);
                return nullptr;
            }
            }
            Q_ASSERT(model);

            model->setImageWrapper(d->imageWrapper);

            //
            // Если документ не является алиасом, то загрузим из него модели и соединим модель со
            // всеми необходимыми обработчиками событий модели
            //
            if (!isDocumentAlias) {
                model->setDocument(documentToLoad);

                connect(
                    model, &BusinessLayer::AbstractModel::documentNameChanged, this,
                    [this, model](const QString& _name) { emit modelNameChanged(model, _name); });
                connect(
                    model, &BusinessLayer::AbstractModel::documentColorChanged, this,
                    [this, model](const QColor& _color) { emit modelColorChanged(model, _color); });
                connect(model, &BusinessLayer::AbstractModel::contentsChanged, this,
                        [this, model](const QByteArray& _undo, const QByteArray& _redo) {
                            emit modelContentChanged(model, _undo, _redo);
                        });
                connect(
                    model, &BusinessLayer::AbstractModel::undoRequested, this,
                    [this, model](int _undoStep) { emit modelUndoRequested(model, _undoStep); });
                connect(model, &BusinessLayer::AbstractModel::removeRequested, this,
                        [this, model] { emit modelRemoveRequested(model); });
            }

            d->documentsToModels.insert(documentToLoad, model);
        }
    }

    return d->documentsToModels.value(_document);
}

QVector<BusinessLayer::AbstractModel*> ProjectModelsFacade::modelsFor(
    Domain::DocumentObjectType _type)
{
    QVector<BusinessLayer::AbstractModel*> models;
    const auto documents = DataStorageLayer::StorageFacade::documentStorage()->documents(_type);
    for (const auto document : documents) {
        auto model = modelFor(document);
        if (model == nullptr) {
            continue;
        }

        models.append(model);
    }
    return models;
}

void ProjectModelsFacade::removeModelFor(Domain::DocumentObject* _document)
{
    if (!d->documentsToModels.contains(_document)) {
        return;
    }

    auto model = d->documentsToModels.take(_document);
    model->disconnect();
    model->clear();
    model->deleteLater();
}

QVector<BusinessLayer::AbstractModel*> ProjectModelsFacade::loadedModels() const
{
    QVector<BusinessLayer::AbstractModel*> models;
    for (auto model : std::as_const(d->documentsToModels)) {
        models.append(model);
    }
    return models;
}

QVector<BusinessLayer::AbstractModel*> ProjectModelsFacade::loadedModelsFor(
    Domain::DocumentObjectType _type) const
{
    QVector<BusinessLayer::AbstractModel*> models;
    for (auto iter = d->documentsToModels.begin(); iter != d->documentsToModels.end(); ++iter) {
        if (iter.key()->type() == _type) {
            models.append(iter.value());
        }
    }
    return models;
}

QVector<Domain::DocumentObject*> ProjectModelsFacade::loadedDocuments() const
{
    QVector<Domain::DocumentObject*> documents;
    for (auto iter = d->documentsToModels.cbegin(); iter != d->documentsToModels.cend(); ++iter) {
        documents.append(iter.key());
    }
    return documents;
}

} // namespace ManagementLayer
