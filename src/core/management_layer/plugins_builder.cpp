#include "plugins_builder.h"

#include <interfaces/management_layer/i_document_manager.h>
#include <interfaces/ui/i_document_view.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QHash>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QWidget>


namespace ManagementLayer {

namespace {

/**
 * @brief Тип представления для активации из плагина
 */
enum ViewType {
    Primary,
    Secondary,
    Window,
};

// clang-format off
/**
 * @brief Майм-типы плагинов
 */

const QString kProjectCollaboratorsMime = QStringLiteral("application/x-starc/editor/project/collaborators");

const QString kCharactersRelationsMime = QStringLiteral("application/x-starc/editor/characters/relations");
const QString kCharacterEditorMime = QStringLiteral("application/x-starc/editor/character/information");
const QString kCharacterNavigatorMime = QStringLiteral("application/x-starc/navigator/character/information");
const QString kCharacterDialoguesMime = QStringLiteral("application/x-starc/editor/character/dialogues");
//
const QString kLocationsMapMime = QStringLiteral("application/x-starc/editor/locations/map");
const QString kLocationEditorMime = QStringLiteral("application/x-starc/editor/location/information");
const QString kLocationNavigatorMime = QStringLiteral("application/x-starc/navigator/location/information");
const QString kLocationScenesMime = QStringLiteral("application/x-starc/editor/location/scenes");
//
const QString kWorldsMapMime = QStringLiteral("application/x-starc/editor/worlds/map");
const QString kWorldEditorMime = QStringLiteral("application/x-starc/editor/world/information");
const QString kWorldNavigatorMime = QStringLiteral("application/x-starc/navigator/world/information");
//
const QString kSimpleTextFolderEditorMime = QStringLiteral("application/x-starc/editor/folder/text");
const QString kSimpleTextEditorMime = QStringLiteral("application/x-starc/editor/text/text");
const QString kSimpleTextNavigatorMime = QStringLiteral("application/x-starc/navigator/text/text");
//
const QString kScreenplayTitlePageEditorMime = QStringLiteral("application/x-starc/editor/screenplay/title-page");
const QString kScreenplayTreatmentEditorMime = QStringLiteral("application/x-starc/editor/screenplay/treatment/text");
const QString kScreenplayTreatmentNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay/treatment");
const QString kScreenplayTreatmentCardsMime = QStringLiteral("application/x-starc/editor/screenplay/treatment/cards");
const QString kScreenplayTextEditorMime = QStringLiteral("application/x-starc/editor/screenplay/text/text");
const QString kScreenplayTextNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay/text");
const QString kScreenplayTextCardsMime = QStringLiteral("application/x-starc/editor/screenplay/text/cards");
const QString kScreenplayTextTimelineMime = QStringLiteral("application/x-starc/editor/screenplay/text/timeline");
const QString kScreenplayTextBreakdownMime = QStringLiteral("application/x-starc/editor/screenplay/text/breakdown");
const QString kScreenplayBreakdownNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay/breakdown");
const QString kScreenplayStatisticsViewMime = QStringLiteral("application/x-starc/view/screenplay/statistics");
const QString kScreenplayStatisticsNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay/statistics");
//
const QString kScreenplaySeriesTitlePageEditorMime = QStringLiteral("application/x-starc/editor/screenplay-series/title-page");
const QString kScreenplaySeriesTreatmentEditorMime = QStringLiteral("application/x-starc/editor/screenplay-series/treatment/text");
const QString kScreenplaySeriesTreatmentNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay-series/treatment");
const QString kScreenplaySeriesTreatmentCardsMime = QStringLiteral("application/x-starc/editor/screenplay-series/treatment/cards");
const QString kScreenplaySeriesTextEditorMime = QStringLiteral("application/x-starc/editor/screenplay-series/text/text");
const QString kScreenplaySeriesTextNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay-series/text");
const QString kScreenplaySeriesTextCardsMime = QStringLiteral("application/x-starc/editor/screenplay-series/text/cards");
const QString kScreenplaySeriesTextTimelineMime = QStringLiteral("application/x-starc/editor/screenplay-series/text/timeline");
const QString kScreenplaySeriesTextBreakdownMime = QStringLiteral("application/x-starc/editor/screenplay-series/text/breakdown");
const QString kScreenplaySeriesBreakdownNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay-series/breakdown");
const QString kScreenplaySeriesStatisticsViewMime = QStringLiteral("application/x-starc/view/screenplay-series/statistics");
const QString kScreenplaySeriesStatisticsNavigatorMime = QStringLiteral("application/x-starc/navigator/screenplay-series/statistics");
//
const QString kComicBookTitlePageEditorMime = QStringLiteral("application/x-starc/editor/comicbook/title-page");
const QString kComicBookTextEditorMime = QStringLiteral("application/x-starc/editor/comicbook/text");
const QString kComicBookTextNavigatorMime = QStringLiteral("application/x-starc/navigator/comicbook/text");
const QString kComicBookStatisticsViewMime = QStringLiteral("application/x-starc/view/comicbook/statistics");
//
const QString kAudioplayInformationViewMime = QStringLiteral("application/x-starc/view/audioplay/information");
const QString kAudioplayParametersViewMime = QStringLiteral("application/x-starc/view/audioplay/parameters");
const QString kAudioplayTitlePageEditorMime = QStringLiteral("application/x-starc/editor/audioplay/title-page");
const QString kAudioplayTextEditorMime = QStringLiteral("application/x-starc/editor/audioplay/text");
const QString kAudioplayTextNavigatorMime = QStringLiteral("application/x-starc/navigator/audioplay/text");
const QString kAudioplayStatisticsViewMime = QStringLiteral("application/x-starc/view/audioplay/statistics");
const QString kAudioplayStatisticsNavigatorMime = QStringLiteral("application/x-starc/navigator/audioplay/statistics");
//
const QString kStageplayInformationViewMime = QStringLiteral("application/x-starc/view/stageplay/information");
const QString kStageplayParametersViewMime = QStringLiteral("application/x-starc/view/stageplay/parameters");
const QString kStageplayTitlePageEditorMime = QStringLiteral("application/x-starc/editor/stageplay/title-page");
const QString kStageplayTextEditorMime = QStringLiteral("application/x-starc/editor/stageplay/text");
const QString kStageplayTextNavigatorMime = QStringLiteral("application/x-starc/navigator/stageplay/text");
const QString kStageplayStatisticsViewMime = QStringLiteral("application/x-starc/view/stageplay/statistics");
//
const QString kNovelTitlePageEditorMime = QStringLiteral("application/x-starc/editor/novel/title-page");
const QString kNovelOutlineEditorMime = QStringLiteral("application/x-starc/editor/novel/outline/text");
const QString kNovelOutlineNavigatorMime = QStringLiteral("application/x-starc/navigator/novel/outline");
const QString kNovelOutlineCardsMime = QStringLiteral("application/x-starc/editor/novel/outline/cards");
const QString kNovelTextEditorMime = QStringLiteral("application/x-starc/editor/novel/text/text");
const QString kNovelTextNavigatorMime = QStringLiteral("application/x-starc/navigator/novel/text");
const QString kNovelTextCardsMime = QStringLiteral("application/x-starc/editor/novel/text/cards");
const QString kNovelTextTimelineMime = QStringLiteral("application/x-starc/editor/novel/text/timeline");
const QString kNovelStatisticsViewMime = QStringLiteral("application/x-starc/view/novel/statistics");
const QString kNovelStatisticsNavigatorMime = QStringLiteral("application/x-starc/navigator/novel/statistics");
//
const QString kMindMapMime = QStringLiteral("application/x-starc/editor/mind_map/mind_map");
const QString kImagesGalleryMime = QStringLiteral("application/x-starc/editor/images/images-gallery");
const QString kPresentationMime = QStringLiteral("application/x-starc/view/presentation");

/**
 * @brief Карта соотвествия майм-типов редактора к навигатору
 */
const QHash<QString, QString> kEditorToNavigator
    = { { kCharacterEditorMime, kCharacterNavigatorMime },
        { kLocationEditorMime, kLocationNavigatorMime },
        { kWorldEditorMime, kWorldNavigatorMime },
        { kSimpleTextEditorMime, kSimpleTextNavigatorMime },
        { kScreenplayTreatmentEditorMime, kScreenplayTreatmentNavigatorMime },
        { kScreenplayTreatmentCardsMime, kScreenplayTreatmentNavigatorMime },
        { kScreenplayTextEditorMime, kScreenplayTextNavigatorMime },
        { kScreenplayTextCardsMime, kScreenplayTextNavigatorMime },
        { kScreenplayTextTimelineMime, kScreenplayTextNavigatorMime },
        { kScreenplayTextBreakdownMime, kScreenplayBreakdownNavigatorMime },
        { kScreenplayStatisticsViewMime, kScreenplayStatisticsNavigatorMime },
        { kScreenplaySeriesTreatmentEditorMime, kScreenplaySeriesTreatmentNavigatorMime },
        { kScreenplaySeriesTreatmentCardsMime, kScreenplaySeriesTreatmentNavigatorMime },
        { kScreenplaySeriesTextEditorMime, kScreenplaySeriesTextNavigatorMime },
        { kScreenplaySeriesTextCardsMime, kScreenplaySeriesTextNavigatorMime },
        { kScreenplaySeriesTextTimelineMime, kScreenplaySeriesTextNavigatorMime },
        { kScreenplaySeriesTextBreakdownMime, kScreenplaySeriesBreakdownNavigatorMime },
        { kScreenplaySeriesStatisticsViewMime, kScreenplaySeriesStatisticsNavigatorMime },
        { kComicBookTextEditorMime, kComicBookTextNavigatorMime },
        { kAudioplayTextEditorMime, kAudioplayTextNavigatorMime },
        { kAudioplayStatisticsViewMime, kAudioplayStatisticsNavigatorMime },
        { kStageplayTextEditorMime, kStageplayTextNavigatorMime },
        { kNovelOutlineEditorMime, kNovelOutlineNavigatorMime },
        { kNovelOutlineCardsMime, kNovelTextNavigatorMime },
        { kNovelTextEditorMime, kNovelTextNavigatorMime },
        { kNovelTextCardsMime, kNovelTextNavigatorMime },
        { kNovelTextTimelineMime, kNovelTextNavigatorMime },
//        { kNovelStatisticsViewMime, kNovelStatisticsNavigatorMime },
      };

/**
 * @brief Карта соответствия майм-типов документа к редакторам
 */
const QHash<QString, QVector<PluginsBuilder::EditorInfo>> kDocumentToEditors
    = { { "application/x-starc/document/project",    { { "application/x-starc/editor/project/information", u8"\U000f02fd" },
                                                       { kProjectCollaboratorsMime, u8"\U000f0b58" } } },
        //
        { "application/x-starc/document/screenplay", { { "application/x-starc/editor/screenplay/information", u8"\U000f02fd" },
                                                       { "application/x-starc/editor/screenplay/parameters", u8"\U000f0493" } } },
        { "application/x-starc/document/screenplay/title-page", { { kScreenplayTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay/synopsis",   { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay/treatment",  { { kScreenplayTreatmentEditorMime, u8"\U000f09ed" },
                                                                  { kScreenplayTreatmentCardsMime, u8"\U000f0554" },
                                                                  /*{ "application/x-starc/editor/screenplay/beatboard", u8"\U000F13D2" }*/ } },
        { "application/x-starc/document/screenplay/text",       { { kScreenplayTextEditorMime, u8"\U000f09ed" },
                                                                  { kScreenplayTextCardsMime, u8"\U000f0554" },
                                                                  { kScreenplayTextTimelineMime, u8"\U000F066C" },
                                                                  { kScreenplayTextBreakdownMime, u8"\U000F14DD" } } },
        { "application/x-starc/document/screenplay/statistics", { { kScreenplayStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/screenplay-series", { { "application/x-starc/editor/screenplay-series/information", u8"\U000f02fd" },
                                                              { "application/x-starc/editor/screenplay-series/parameters", u8"\U000f0493" } } },
        { "application/x-starc/document/screenplay-series/episodes",   { { "application/x-starc/editor/screenplay-series/episodes", u8"\U000f02fd" } } },
        { "application/x-starc/document/screenplay-series/title-page", { { kScreenplaySeriesTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay-series/synopsis",   { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/screenplay-series/treatment",  { { kScreenplaySeriesTreatmentEditorMime, u8"\U000f09ed" },
                                                                         { kScreenplaySeriesTreatmentCardsMime, u8"\U000f0554" },
                                                                       /*{ "application/x-starc/editor/screenplay-series/beatboard", u8"\U000F13D2" }*/ } },
        { "application/x-starc/document/screenplay-series/text",       { { kScreenplaySeriesTextEditorMime, u8"\U000f09ed" },
                                                                         { kScreenplaySeriesTextCardsMime, u8"\U000f0554" },
                                                                         { kScreenplaySeriesTextTimelineMime, u8"\U000F066C" },
                                                                         { kScreenplaySeriesTextBreakdownMime, u8"\U000F14DD" } } },
        { "application/x-starc/document/screenplay-series/statistics", { { kScreenplaySeriesStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/comicbook",  { { "application/x-starc/editor/comicbook/information", u8"\U000f02fd" },
                                                       { "application/x-starc/editor/comicbook/parameters", u8"\U000f0493" } } },
        { "application/x-starc/document/comicbook/title-page",  { { kComicBookTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/comicbook/synopsis",    { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/comicbook/text",        { { kComicBookTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/comicbook/statistics",  { { kComicBookStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/audioplay", { { kAudioplayInformationViewMime, u8"\U000f02fd" },
                                                       { kAudioplayParametersViewMime, u8"\U000f0493" } } },
        { "application/x-starc/document/audioplay/title-page",  { { kAudioplayTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/audioplay/synopsis",    { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/audioplay/text",        { { kAudioplayTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/audioplay/statistics",  { { kAudioplayStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/stageplay", { { kStageplayInformationViewMime, u8"\U000f02fd" },
                                                       { kStageplayParametersViewMime, u8"\U000f0493" } } },
        { "application/x-starc/document/stageplay/title-page",  { { kStageplayTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/stageplay/synopsis",    { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/stageplay/text",        { { kStageplayTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/stageplay/statistics",  { { kStageplayStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/novel", { { "application/x-starc/editor/novel/information", u8"\U000f02fd" },
                                                       { "application/x-starc/editor/novel/parameters", u8"\U000f0493" } } },
        { "application/x-starc/document/novel/title-page", { { kNovelTitlePageEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/novel/synopsis",   { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/novel/outline",  { { kNovelOutlineEditorMime, u8"\U000f09ed" },
                                                           { kNovelOutlineCardsMime, u8"\U000f0554" },
                                                                  /*{ "application/x-starc/editor/novel/beatboard", u8"\U000F13D2" }*/ } },
        { "application/x-starc/document/novel/text",       { { kNovelTextEditorMime, u8"\U000f09ed" },
                                                             { kNovelTextCardsMime, u8"\U000f0554" },
                                                             { kNovelTextTimelineMime, u8"\U000F066C" } } },
        { "application/x-starc/document/novel/statistics", { { kNovelStatisticsViewMime, u8"\U000f0127" } } },
        //
        { "application/x-starc/document/characters",  { { kCharactersRelationsMime, u8"\U000F0D3D" } } },
        { "application/x-starc/document/character",  { { kCharacterEditorMime, u8"\U000f02fd" },
                                                       { kCharacterDialoguesMime, u8"\U000F017C" } } },
        //
        { "application/x-starc/document/locations",  { { kLocationsMapMime, u8"\U000F0982" } } },
        { "application/x-starc/document/location",   { { kLocationEditorMime, u8"\U000f02fd" },
                                                       { kLocationScenesMime, u8"\U000F0571" } } },
        //
        { "application/x-starc/document/worlds",  { { kWorldsMapMime, u8"\U000F01E7" } } },
        { "application/x-starc/document/world",   { { kWorldEditorMime, u8"\U000F01E7" } } },
        //
        { "application/x-starc/document/folder",     { { kSimpleTextFolderEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/text",       { { kSimpleTextEditorMime, u8"\U000f09ed" } } },
        { "application/x-starc/document/mind-map",  { { kMindMapMime, u8"\U000F04AA" } } },
        { "application/x-starc/document/images-gallery",  { { kImagesGalleryMime, u8"\U000F02F9" } } },
        { "application/x-starc/document/presentation",  { { kPresentationMime, u8"\U000F0428" } } },
        //,
        { "application/x-starc/document/recycle-bin",       { { "application/x-starc/editor/recycle-bin", u8"\U000f01b4" } } }
      };

/**
 * @brief Карта соответсвий майм-типов навигаторов/редакторов к названиям библиотек с плагинами
 * @note Нужно это для того, чтобы не прогружать каждый раз плагин со всеми зависимостями лишь для
 *       извлечения майм-типа навигатора/редактора, а подгружать точечно только то, что нужно
 */
const QHash<QString, QString> kMimeToPlugin
    = { { "application/x-starc/editor/project/information", "*projectinformationplugin*" },
        { kProjectCollaboratorsMime, "*projectcollaboratorsplugin*" },
        //
        { kSimpleTextFolderEditorMime, "*simpletextplugin*" },
        { kSimpleTextEditorMime, "*simpletextplugin*" },
        { kSimpleTextNavigatorMime, "*simpletextstructureplugin*" },
        //
        { "application/x-starc/editor/screenplay/information", "*screenplayinformationplugin*" },
        { "application/x-starc/editor/screenplay/parameters", "*screenplayparametersplugin*" },
        { kScreenplayTitlePageEditorMime, "*titlepageplugin*" },
        { kScreenplayTreatmentEditorMime, "*screenplaytreatmentplugin*" },
        { kScreenplayTreatmentNavigatorMime, "*screenplaytreatmentstructureplugin*" },
        { kScreenplayTreatmentCardsMime, "*screenplaycardsplugin*" },
        { kScreenplayTextEditorMime, "*screenplaytextplugin*" },
        { kScreenplayTextNavigatorMime, "*screenplaytextstructureplugin*" },
        { kScreenplayTextCardsMime, "*screenplaycardsplugin*" },
        { kScreenplayTextTimelineMime, "*screenplaytimelineplugin*" },
        { kScreenplayTextBreakdownMime, "*screenplaybreakdownplugin*" },
        { kScreenplayBreakdownNavigatorMime, "*screenplaybreakdownstructureplugin*" },
        { kScreenplayStatisticsViewMime, "*screenplaystatisticsplugin*" },
        { kScreenplayStatisticsNavigatorMime, "*screenplaystatisticsstructureplugin*" },
        //
        { "application/x-starc/editor/screenplay-series/information", "*screenplayseriesinformationplugin*" },
        { "application/x-starc/editor/screenplay-series/parameters", "*screenplayseriesparametersplugin*" },
        { "application/x-starc/editor/screenplay-series/episodes", "*screenplayseriesepisodesplugin*" },
        { kScreenplaySeriesTitlePageEditorMime, "*titlepageplugin*" },
        { kScreenplaySeriesTreatmentEditorMime, "*screenplayseriestreatmentplugin*" },
        { kScreenplaySeriesTreatmentNavigatorMime, "*screenplayseriestreatmentstructureplugin*" },
        { kScreenplaySeriesTreatmentCardsMime, "*screenplayseriescardsplugin*" },
        { kScreenplaySeriesTextEditorMime, "*screenplayseriestextplugin*" },
        { kScreenplaySeriesTextNavigatorMime, "*screenplayseriestextstructureplugin*" },
        { kScreenplaySeriesTextCardsMime, "*screenplayseriescardsplugin*" },
        { kScreenplaySeriesTextTimelineMime, "*screenplayseriestimelineplugin*" },
        { kScreenplaySeriesTextBreakdownMime, "*screenplayseriesbreakdownplugin*" },
        { kScreenplaySeriesBreakdownNavigatorMime, "*screenplayseriesbreakdownstructureplugin*" },
        { kScreenplaySeriesStatisticsViewMime, "*screenplayseriesstatisticsplugin*" },
        { kScreenplaySeriesStatisticsNavigatorMime, "*screenplayseriesstatisticsstructureplugin*" },
        //
        { "application/x-starc/editor/comicbook/information", "*comicbookinformationplugin*" },
        { "application/x-starc/editor/comicbook/parameters", "*comicbookparametersplugin*" },
        { kComicBookTitlePageEditorMime, "*titlepageplugin*" },
        { kComicBookTextEditorMime, "*comicbooktextplugin*" },
        { kComicBookTextNavigatorMime, "*comicbooktextstructureplugin*" },
        { kComicBookStatisticsViewMime, "*comicbookstatisticsplugin*" },
        //
        { kAudioplayInformationViewMime, "*audioplayinformationplugin*" },
        { kAudioplayParametersViewMime, "*audioplayparametersplugin*" },
        { kAudioplayTitlePageEditorMime, "*titlepageplugin*" },
        { kAudioplayTextEditorMime, "*audioplaytextplugin*" },
        { kAudioplayTextNavigatorMime, "*audioplaytextstructureplugin*" },
        { kAudioplayStatisticsViewMime, "*audioplaystatisticsplugin*" },
        { kAudioplayStatisticsNavigatorMime, "*audioplaystatisticsstructureplugin*" },
        //
        { kStageplayInformationViewMime, "*stageplayinformationplugin*" },
        { kStageplayParametersViewMime, "*stageplayparametersplugin*" },
        { kStageplayTitlePageEditorMime, "*titlepageplugin*" },
        { kStageplayTextEditorMime, "*stageplaytextplugin*" },
        { kStageplayTextNavigatorMime, "*stageplaytextstructureplugin*" },
        { kStageplayStatisticsViewMime, "*stageplaystatisticsplugin*" },
        //
        { "application/x-starc/editor/novel/information", "*novelinformationplugin*" },
        { "application/x-starc/editor/novel/parameters", "*novelparametersplugin*" },
        { kNovelTitlePageEditorMime, "*titlepageplugin*" },
        { kNovelOutlineEditorMime, "*noveloutlineplugin*" },
        { kNovelOutlineNavigatorMime, "*noveloutlinestructureplugin*" },
        { kNovelOutlineCardsMime, "*novelcardsplugin*" },
        { kNovelTextEditorMime, "*noveltextplugin*" },
        { kNovelTextNavigatorMime, "*noveltextstructureplugin*" },
        { kNovelTextCardsMime, "*novelcardsplugin*" },
        { kNovelTextTimelineMime, "*noveltimelineplugin*" },
        { kNovelStatisticsViewMime, "*novelstatisticsplugin*" },
        { kNovelStatisticsNavigatorMime, "*novelstatisticsstructureplugin*" },
        //
        { kCharactersRelationsMime, "*charactersrelationsplugin*" },
        { kCharacterEditorMime, "*characterinformationplugin*" },
        { kCharacterNavigatorMime, "*characterstructureplugin*" },
        { kCharacterDialoguesMime, "*characterdialoguesplugin*" },
        //
        { kLocationsMapMime, "*locationsmapplugin*" },
        { kLocationEditorMime, "*locationinformationplugin*" },
        { kLocationNavigatorMime, "*locationstructureplugin*" },
        { kLocationScenesMime, "*locationscenesplugin*" },
        //
        { kWorldsMapMime, "*worldsmapplugin*" },
        { kWorldEditorMime, "*worldinformationplugin*" },
        { kWorldNavigatorMime, "*worldstructureplugin*" },
        //
        { kMindMapMime, "*mindmapplugin*" },
        { kImagesGalleryMime, "*imagesgalleryplugin*" },
        { kPresentationMime, "*presentationplugin*" },
        //
        { "application/x-starc/editor/recycle-bin", "*recyclebinplugin*" },
      };
// clang-format on

} // namespace

class PluginsBuilder::Implementation
{
public:
    /**
     * @brief Активировать плагин заданного типа указанной моделью
     */
    Ui::IDocumentView* activatePlugin(const QString& _mimeType,
                                      BusinessLayer::AbstractModel* _model, ViewType _type);

    /**
     * @brief Загруженные плагины <mime, plugin>
     */
    QHash<QString, ManagementLayer::IDocumentManager*> plugins;

    /**
     * @brief Находится ли текущий проект в команде
     */
    bool isProjectInTeam = false;

    /**
     * @brief Права доступа к конкретным документам
     */
    QHash<QUuid, DocumentEditingMode> editingPermissions;

    /**
     * @brief Количество кредитов доступных для использования с ИИ инструментами
     */
    int availableCredits = 0;
};

Ui::IDocumentView* PluginsBuilder::Implementation::activatePlugin(
    const QString& _mimeType, BusinessLayer::AbstractModel* _model, ViewType _type)
{
    //
    // Если плагин ещё не был загружен, загружаем его
    //
    if (!plugins.contains(_mimeType)) {
        //
        // Смотрим папку с данными приложения на компе
        // NOTE: В Debug-режим работает с папкой сборки приложения
        //
        // TODO: Когда дорастём включить этот функционал, плюс продумать, как быть в режиме
        // разработки
        //
        const QString pluginsDirName = "plugins";
        QDir pluginsDir(
            //#ifndef QT_NO_DEBUG
            QApplication::applicationDirPath()
            //#else
            //                    QStandardPaths::writableLocation(QStandardPaths::DataLocation)
            //#endif
        );

#if defined(Q_OS_MAC)
        pluginsDir.cdUp();
#endif

        //
        // Если папки с плагинами нет, идём лесом
        //
        if (!pluginsDir.cd(pluginsDirName)) {
            return nullptr;
        }

        //
        // Подгружаем плагин
        //
        const QString extensionFilter =
#ifdef Q_OS_WIN
            ".dll";
#elif defined(Q_OS_LINUX)
            ".so";
#elif defined(Q_OS_MAC)
            ".dylib";
#else
            "";
#endif
        const QStringList libCorePluginEntries = pluginsDir.entryList(
            { kMimeToPlugin.value(_mimeType) + extensionFilter }, QDir::Files);
        if (libCorePluginEntries.isEmpty()) {
            qCritical() << "Plugin isn't found for mime-type:" << _mimeType;
            return nullptr;
        }
        if (libCorePluginEntries.size() > 1) {
            qCritical() << "Found more than 1 plugins for mime-type:" << _mimeType;
            return nullptr;
        }

        const auto pluginPath = libCorePluginEntries.first();
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(pluginPath));
        QObject* pluginObject = pluginLoader.instance();
        if (pluginObject == nullptr) {
            qDebug() << pluginLoader.errorString();
        }

        auto plugin = qobject_cast<ManagementLayer::IDocumentManager*>(pluginObject);
        plugin->setAvailableCredits(availableCredits);
        plugins.insert(_mimeType, plugin);
    }

    //
    // Получим плагин для нужного типа редактора
    //
    auto plugin = plugins.value(_mimeType);
    if (plugin == nullptr) {
        return nullptr;
    }

    //
    // Получим представление (при необходимости новое будет создано)
    //
    Ui::IDocumentView* view = nullptr;
    switch (_type) {
    case Primary: {
        view = plugin->view(_model);
        break;
    }

    case Secondary: {
        view = plugin->secondaryView(_model);
        break;
    }

    default: {
        view = plugin->createView(_model);
        break;
    }
    }

    //
    // Настроим доступность для редактирования в плагине
    //
    plugin->checkAvailabilityToEdit(isProjectInTeam);
    plugin->setEditingPermissions(editingPermissions);
    //
    // ... а также доступные кредиты для работы с ИИ
    //
    view->setAvailableCredits(availableCredits);

    return view;
}


// ****


PluginsBuilder::PluginsBuilder()
    : d(new Implementation)
{
}

PluginsBuilder::~PluginsBuilder() = default;

IDocumentManager* PluginsBuilder::plugin(const QString& _mimeType) const
{
    auto plugin = d->plugins.find(_mimeType);
    if (plugin == d->plugins.end()) {
        return nullptr;
    }

    return plugin.value();
}

QVector<PluginsBuilder::EditorInfo> PluginsBuilder::editorsInfoFor(const QString& _documentMimeType,
                                                                   bool _isProjectRemote) const
{
    auto editors = kDocumentToEditors.value(_documentMimeType);
    if (!_isProjectRemote) {
        editors.erase(std::remove_if(editors.begin(), editors.end(),
                                     [](const auto& _editorInfo) {
                                         return _editorInfo.mimeType == kProjectCollaboratorsMime;
                                     }),
                      editors.end());
    }
    return editors;
}

QString PluginsBuilder::editorDescription(const QString& _documentMimeType,
                                          const QString& _editorMimeType) const
{
    // clang-format off
    const QHash<QString, QHash<QString, QString>> descriptions
        = { { "application/x-starc/document/project",
              { { "application/x-starc/editor/project/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about project") },
                { kProjectCollaboratorsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Project collaborators") } } },
            //
            { "application/x-starc/document/screenplay",
              { { "application/x-starc/editor/screenplay/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about screenplay") },
                { "application/x-starc/editor/screenplay/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Screenplay parameters") } } },
            { "application/x-starc/document/screenplay/title-page",
              { { kScreenplayTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/screenplay/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/screenplay/treatment",
              { { kScreenplayTreatmentEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Treatment text") },
                { "application/x-starc/editor/screenplay/treatment/beatboard",
                  QApplication::translate("ProjectPluginsBuilder", "Beats") },
                { kScreenplayTreatmentCardsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/screenplay/text",
              { { kScreenplayTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Screenplay text") },
                { kScreenplayTextCardsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Cards") },
                { kScreenplayTextTimelineMime,
                  QApplication::translate("ProjectPluginsBuilder", "Timeline") },
                { kScreenplayTextBreakdownMime,
                  QApplication::translate("ProjectPluginsBuilder", "Breakdown") } } },
            { "application/x-starc/document/screenplay/statistics",
              { { kScreenplayStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/screenplay-series",
              { { "application/x-starc/editor/screenplay-series/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about series") },
                { "application/x-starc/editor/screenplay-series/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Series parameters") } } },
            { "application/x-starc/document/screenplay-series/plan",
              { { "application/x-starc/editor/screenplay-series/plan",
                  QApplication::translate("ProjectPluginsBuilder", "Series plan") } } },
            { "application/x-starc/document/screenplay-series/title-page",
              { { kScreenplaySeriesTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/screenplay-series/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/screenplay-series/treatment",
              { { kScreenplaySeriesTreatmentEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Treatment text") },
                { "application/x-starc/editor/screenplay-series/treatment/beatboard",
                  QApplication::translate("ProjectPluginsBuilder", "Beats") },
                { kScreenplaySeriesTreatmentCardsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/screenplay-series/text",
              { { kScreenplaySeriesTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Screenplay text") },
                { kScreenplaySeriesTextCardsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Cards") },
                { kScreenplaySeriesTextTimelineMime,
                  QApplication::translate("ProjectPluginsBuilder", "Timeline") },
                { kScreenplaySeriesTextBreakdownMime,
                  QApplication::translate("ProjectPluginsBuilder", "Breakdown") } } },
            { "application/x-starc/document/screenplay-series/statistics",
              { { kScreenplaySeriesStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/comicbook",
              { { "application/x-starc/editor/comicbook/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about comic book") },
                { "application/x-starc/editor/comicbook/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Comic book parameters") } } },
            { "application/x-starc/document/comicbook/title-page",
              { { kComicBookTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/comicbook/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/comicbook/text",
              { { kComicBookTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Comic book text") },
                { "application/x-starc/editor/comicbook/cards",
                    QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/comicbook/statistics",
              { { kComicBookStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/audioplay",
              { { kAudioplayInformationViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Information about audioplay") },
                { "application/x-starc/editor/audioplay/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Audioplay parameters") } } },
            { "application/x-starc/document/audioplay/title-page",
              { { kAudioplayTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/audioplay/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/audioplay/text",
              { { kAudioplayTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Audioplay text") },
                { "application/x-starc/editor/audioplay/cards",
                    QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/audioplay/statistics",
              { { kAudioplayStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/stageplay",
              { { kStageplayInformationViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Information about stageplay") },
                { "application/x-starc/editor/stageplay/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Stageplay parameters") } } },
            { "application/x-starc/document/stageplay/title-page",
              { { kStageplayTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/stageplay/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/stageplay/text",
              { { kStageplayTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Stageplay text") },
                { "application/x-starc/editor/stageplay/cards",
                    QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/stageplay/statistics",
              { { kStageplayStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/novel",
              { { "application/x-starc/editor/novel/information",
                  QApplication::translate("ProjectPluginsBuilder", "Information about novel") },
                { "application/x-starc/editor/novel/parameters",
                  QApplication::translate("ProjectPluginsBuilder", "Novel parameters") } } },
            { "application/x-starc/document/novel/title-page",
              { { kNovelTitlePageEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Title page text") } } },
            { "application/x-starc/document/novel/synopsis",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Synopsis text") } } },
            { "application/x-starc/document/novel/outline",
              { { kNovelOutlineEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Outline text") },
                { "application/x-starc/editor/novel/outline/beatboard",
                  QApplication::translate("ProjectPluginsBuilder", "Beats") },
                { kNovelOutlineCardsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Cards") } } },
            { "application/x-starc/document/novel/text",
              { { kNovelTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Novel text") },
                { kNovelTextCardsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Cards") },
                { kNovelTextTimelineMime,
                  QApplication::translate("ProjectPluginsBuilder", "Timeline") } } },
            { "application/x-starc/document/novel/statistics",
              { { kNovelStatisticsViewMime,
                  QApplication::translate("ProjectPluginsBuilder", "Statistics") } } },
            //
            { "application/x-starc/document/characters",
              { { kCharactersRelationsMime,
                  QApplication::translate("ProjectPluginsBuilder", "Characters relations") } } },
            { "application/x-starc/document/character",
              { { kCharacterEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Character information") },
                { kCharacterDialoguesMime,
                  QApplication::translate("ProjectPluginsBuilder", "Character dialogues") } } },
            //
            { "application/x-starc/document/locations",
              { { kLocationsMapMime,
                  QApplication::translate("ProjectPluginsBuilder", "Locations map") } } },
            { "application/x-starc/document/location",
              { { kLocationEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Location information") },
                { kLocationScenesMime,
                  QApplication::translate("ProjectPluginsBuilder", "Location scenes") } } },
            //
            { "application/x-starc/document/worlds",
              { { kWorldsMapMime,
                  QApplication::translate("ProjectPluginsBuilder", "Worlds map") } } },
            { "application/x-starc/document/world",
              { { kWorldEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "World information") } } },
            //
            { "application/x-starc/document/folder",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Folder text") } } },
            { "application/x-starc/document/text",
              { { kSimpleTextEditorMime,
                  QApplication::translate("ProjectPluginsBuilder", "Document text") } } },
            //,
            { "application/x-starc/document/recycle-bin",
              { { "application/x-starc/editor/recycle-bin",
                  QApplication::translate("ProjectPluginsBuilder", "Recycle bin summary info") } } },
            { "application/x-starc/document/mind-map",
              { { kMindMapMime,
                  QApplication::translate("ProjectPluginsBuilder", "Mind map") } } },
            { "application/x-starc/document/images-gallery",
              { { kImagesGalleryMime,
                  QApplication::translate("ProjectPluginsBuilder", "Images gallery") } } },
            { "application/x-starc/document/presentation",
              { { kPresentationMime,
                  QApplication::translate("ProjectPluginsBuilder", "Presentation") } } },
             };
    // clang-format on
    return descriptions.value(_documentMimeType).value(_editorMimeType);
}

QString PluginsBuilder::navigatorMimeTypeFor(const QString& _editorMimeType) const
{
    return kEditorToNavigator.value(_editorMimeType);
}

Ui::IDocumentView* PluginsBuilder::activateView(const QString& _viewMimeType,
                                                BusinessLayer::AbstractModel* _model) const
{
    return d->activatePlugin(_viewMimeType, _model, ViewType::Primary);
}

Ui::IDocumentView* PluginsBuilder::activateSecondView(const QString& _viewMimeType,
                                                      BusinessLayer::AbstractModel* _model) const
{
    return d->activatePlugin(_viewMimeType, _model, ViewType::Secondary);
}

Ui::IDocumentView* PluginsBuilder::activateWindowView(const QString& _viewMimeType,
                                                      BusinessLayer::AbstractModel* _model) const
{
    return d->activatePlugin(_viewMimeType, _model, ViewType::Window);
}

void PluginsBuilder::bind(const QString& _viewMimeType, const QString& _navigatorMimeType) const
{
    auto viewPlugin = d->plugins.value(_viewMimeType);
    Q_ASSERT(viewPlugin);

    auto navigatorPlugin = d->plugins.value(_navigatorMimeType);
    Q_ASSERT(navigatorPlugin);

    viewPlugin->bind(navigatorPlugin);
    navigatorPlugin->bind(viewPlugin);
}

void PluginsBuilder::syncModelAndBindEditors(const QString& _viewMimeType,
                                             BusinessLayer::AbstractModel* _model,
                                             bool _isPrimaryView) const
{
    if (_viewMimeType.isEmpty()) {
        return;
    }

    //
    // Определеим группу редакторо, чтобы их связать
    //
    auto viewGroupMimeType = _viewMimeType;
    const auto groupsCount = viewGroupMimeType.count('/') + 1;
    //
    // ... если групп меньше пяти, то не связываем, чтобы не привязать несовместимые плагины
    //
    if (groupsCount < 5) {
        return;
    }
    //
    // ... если групп всего пять, то для связи используем именно этот майм тип
    //
    else if (groupsCount == 5) {
    }
    //
    // ... если групп шесть, то связываем на предыдущем уровне майма
    //
    else if (groupsCount == 6) {
        viewGroupMimeType = viewGroupMimeType.left(_viewMimeType.lastIndexOf("/"));
    }
    //
    // ... такого, чтобы групп было больше шести пока нет
    //
    else {
        Q_ASSERT(false);
    }
    //
    // Соберём все редакторы заданной группы
    //
    QHash<QString, IDocumentManager*> pluginsFiltered;
    for (auto iter = d->plugins.begin(); iter != d->plugins.end(); ++iter) {
        if (iter.key().startsWith(viewGroupMimeType)) {
            pluginsFiltered.insert(iter.key(), iter.value());
        }
    }
    //
    // Установим им единую модель
    //
    for (auto i = pluginsFiltered.begin(); i != pluginsFiltered.end(); ++i) {
        if (_isPrimaryView) {
            if (i.value()->view() != nullptr) {
                i.value()->view(_model);
            }
        } else {
            if (i.value()->secondaryView() != nullptr) {
                i.value()->secondaryView(_model);
            }
        }
    }
    //
    // И свяжем их
    //
    for (auto i = pluginsFiltered.begin(); i != pluginsFiltered.end(); ++i) {
        for (auto j = pluginsFiltered.begin(); j != pluginsFiltered.end(); ++j) {
            if (i.key() == j.key()) {
                continue;
            }

            i.value()->bind(j.value());
        }
    }
}

void PluginsBuilder::toggleViewFullScreen(bool _isFullScreen, const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->view();
    if (view != nullptr) {
        view->toggleFullScreen(_isFullScreen);
    }
}

void PluginsBuilder::toggleSecondaryViewFullScreen(bool _isFullScreen,
                                                   const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->secondaryView();
    if (view != nullptr) {
        view->toggleFullScreen(_isFullScreen);
    }
}

void PluginsBuilder::setViewCursors(const QVector<Domain::CursorInfo>& _cursors,
                                    const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->view();
    if (view != nullptr) {
        view->setCursors(_cursors);
    }
}

void PluginsBuilder::setSecondaryViewCursors(const QVector<Domain::CursorInfo>& _cursors,
                                             const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->secondaryView();
    if (view != nullptr) {
        view->setCursors(_cursors);
    }
}

void PluginsBuilder::setViewCurrentCursor(const Domain::CursorInfo& _cursor,
                                          const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->view();
    if (view != nullptr) {
        view->setCurrentCursor(_cursor);
    }
}

void PluginsBuilder::setSecondaryViewCurrentCursor(const Domain::CursorInfo& _cursor,
                                                   const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->secondaryView();
    if (view != nullptr) {
        view->setCurrentCursor(_cursor);
    }
}

void PluginsBuilder::setViewCurrentIndex(const QModelIndex& _index,
                                         const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->view();
    if (view != nullptr) {
        view->setCurrentModelIndex(_index);
    }
}

void PluginsBuilder::setSecondaryViewCurrentIndex(const QModelIndex& _index,
                                                  const QString& _viewMimeType) const
{
    if (!d->plugins.contains(_viewMimeType)) {
        return;
    }

    auto view = d->plugins.value(_viewMimeType)->secondaryView();
    if (view != nullptr) {
        view->setCurrentModelIndex(_index);
    }
}

void PluginsBuilder::reconfigureAll() const
{
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->reconfigure({});
    }
}

void PluginsBuilder::reconfigurePlugin(const QString& _mimeType,
                                       const QStringList& _changedSettingsKeys) const
{
    auto plugin = this->plugin(_mimeType);
    if (!plugin) {
        return;
    }

    plugin->reconfigure(_changedSettingsKeys);
}

void PluginsBuilder::reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kSimpleTextEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplaySeriesTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kComicBookTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kAudioplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kStageplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kNovelTitlePageEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureSimpleTextNavigator() const
{
    reconfigurePlugin(kSimpleTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kScreenplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTreatmentEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTreatmentCardsMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTextEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTextCardsMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTextTimelineMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplayTextBreakdownMime, _changedSettingsKeys);
    //
    reconfigurePlugin(kScreenplaySeriesTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplaySeriesTreatmentEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplaySeriesTreatmentCardsMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplaySeriesTextEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplaySeriesTextCardsMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplaySeriesTextTimelineMime, _changedSettingsKeys);
    reconfigurePlugin(kScreenplaySeriesTextBreakdownMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureScreenplayNavigator() const
{
    reconfigurePlugin(kScreenplayTreatmentNavigatorMime, {});
    reconfigurePlugin(kScreenplayTextNavigatorMime, {});
    reconfigurePlugin(kScreenplayBreakdownNavigatorMime, {});
    //
    reconfigurePlugin(kScreenplaySeriesTreatmentNavigatorMime, {});
    reconfigurePlugin(kScreenplaySeriesTextNavigatorMime, {});
    reconfigurePlugin(kScreenplaySeriesBreakdownNavigatorMime, {});
}

void PluginsBuilder::reconfigureComicBookEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kComicBookTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kComicBookTextEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureComicBookNavigator() const
{
    reconfigurePlugin(kComicBookTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureAudioplayEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kAudioplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kAudioplayTextEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureAudioplayNavigator() const
{
    reconfigurePlugin(kAudioplayTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureStageplayEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kStageplayTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kStageplayTextEditorMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureStageplayNavigator() const
{
    reconfigurePlugin(kStageplayTextNavigatorMime, {});
}

void PluginsBuilder::reconfigureNovelEditor(const QStringList& _changedSettingsKeys) const
{
    reconfigurePlugin(kNovelTitlePageEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kNovelOutlineEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kNovelTextEditorMime, _changedSettingsKeys);
    reconfigurePlugin(kNovelTextCardsMime, _changedSettingsKeys);
    reconfigurePlugin(kNovelTextTimelineMime, _changedSettingsKeys);
}

void PluginsBuilder::reconfigureNovelNavigator() const
{
    reconfigurePlugin(kNovelTextNavigatorMime, {});
}

void PluginsBuilder::checkAvailabilityToEdit(bool _projectInTeam) const
{
    d->isProjectInTeam = _projectInTeam;
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->checkAvailabilityToEdit(d->isProjectInTeam);
    }
}

void PluginsBuilder::setEditingPermissions(
    const QHash<QUuid, DocumentEditingMode>& _permissions) const
{
    if (d->editingPermissions == _permissions) {
        return;
    }

    d->editingPermissions = _permissions;
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->setEditingPermissions(d->editingPermissions);
    }
}

void PluginsBuilder::setAvailableCredits(int _credits) const
{
    if (d->availableCredits == _credits) {
        return;
    }

    d->availableCredits = _credits;
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->setAvailableCredits(_credits);
    }
}

void PluginsBuilder::resetModels() const
{
    for (auto plugin : std::as_const(d->plugins)) {
        plugin->saveSettings();
        plugin->resetModels();
    }
}

Ui::IDocumentView* PluginsBuilder::projectCollaboratorsView(
    BusinessLayer::AbstractModel* _model) const
{
    return d->activatePlugin(kProjectCollaboratorsMime, _model, Primary);
}

} // namespace ManagementLayer
