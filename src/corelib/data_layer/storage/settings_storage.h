#pragma once

#include <QVariant>
#include <QVariantMap>

#include <corelib_global.h>


/**
 * Правила именования настроек
 *
 * Приложение
 * application/key
 *
 * Проект
 * project/key - общее для всех
 * project/concrete/path/ - для конкретного проекта
 *
 * Система
 * system/key
 *
 * Виджеты
 * widgets/widget-name/key
 */

namespace DataStorageLayer {

namespace {
//
// Устройтво
//
const QString kDeviceGroupKey = "device";
//
// уникальный индентификатор устройства
const QString kDeviceUuidKey = kDeviceGroupKey + "/uuid";
//
// Приложение
//
const QString kApplicationGroupKey = "application";
//
// первый запуск приложения (false) или оно уже сконфигурировано (true)
const QString kApplicationConfiguredKey = kApplicationGroupKey + "/configured";
// язык приложения
const QString kApplicationLanguagedKey = kApplicationGroupKey + "/language";
const QString kApplicationLanguagedFileKey = kApplicationGroupKey + "/language-file";
// тема приложения
const QString kApplicationThemeKey = kApplicationGroupKey + "/theme";
// параметры кастомной тема приложения
const QString kApplicationCustomThemeColorsKey = kApplicationGroupKey + "/custom-theme";
// масштаб приложения
const QString kApplicationScaleFactorKey = kApplicationGroupKey + "/scale-factor";
// использование компактного режима
const QString kApplicationDensityKey = kApplicationGroupKey + "/density";
// состояние и геометрия основного окна приложения
const QString kApplicationViewStateKey = kApplicationGroupKey + "-view/";
// включено ли автосохранение
const QString kApplicationUseAutoSaveKey = kApplicationGroupKey + "/autosave";
// включено ли сохранение резервных копий
const QString kApplicationSaveBackupsKey = kApplicationGroupKey + "/save-backups";
// папка в которую будут сохраняться резервные копии
const QString kApplicationBackupsFolderKey = kApplicationGroupKey + "/backups-folder";
// максимальное кол-во бекапов для сохранения
const QString kApplicationBackupsQtyKey = kApplicationGroupKey + "/backups-qty";
// показывать ли страницы текстовых документов
const QString kApplicationShowDocumentsPagesKey = kApplicationGroupKey + "/show-documents-pages";
// включены ли звуки печатной машинки при наборе текста
const QString kApplicationUseTypewriterSoundKey = kApplicationGroupKey + "/typewriter-sound";
// включена ли проверка орфографии
const QString kApplicationUseSpellCheckerKey = kApplicationGroupKey + "/use-spell-checker";
// словарь для проверки орфографии
const QString kApplicationSpellCheckerLanguageKey
    = kApplicationGroupKey + "/spell-checker-language";
// включена ли подсветка текущей строки для текстовых редакторов
const QString kApplicationHighlightCurrentLineKey
    = kApplicationGroupKey + "/highlight-current-line";
// включён ли режим фокусировки на текущем параграфе для текстовых редакторов
const QString kApplicationFocusCurrentParagraphKey
    = kApplicationGroupKey + "/focus-current-paragraph";
// включён ли режим печатной машинки для текстовых редакторов
const QString kApplicationUseTypewriterScrollingKey
    = kApplicationGroupKey + "/use-typewriter-scrolling";
// активна ли автозамена трёх точек на символ многоточия
const QString kApplicationReplaceThreeDotsWithEllipsisKey
    = kApplicationGroupKey + "/replace-three-dots-with-ellipsis";
// активны ли замены обычных кавычек на умные для текущего языка
const QString kApplicationSmartQuotesKey = kApplicationGroupKey + "/smart-quotes";
// заменять два тире на длинное тире
const QString kApplicationReplaceTwoDashesWithEmDashKey
    = kApplicationGroupKey + "/replace-two-dashes-with-em-dash";
// запретить вводить несколько пробелов подряд
const QString kApplicationAvoidMultipleSpacesKey = kApplicationGroupKey + "/avoid-multiple-spaces";
// список недавних проектов
const QString kApplicationProjectsKey = kApplicationGroupKey + "/projects";
//
// Аккаунт
//
const QString kAccountGroupKey = "account";
//
// имейл
const QString kAccountEmailKey = kAccountGroupKey + "/email";
// имя пользователя
const QString kAccountUserNameKey = kAccountGroupKey + "/user-name";
// описание пользователя
const QString kAccountDescriptionKey = kAccountGroupKey + "/description";
// подписка пользователя на новости
const QString kAccountNewsletterLanguageKey = kAccountGroupKey + "/newsletter-language";
const QString kAccountNewsletterSubscribedKey = kAccountGroupKey + "/newsletter-subscribed";
// аватарка
const QString kAccountAvatarKey = kAccountGroupKey + "/avatar";

//
// Проект
//
const QString kProjectGroupKey = "project";
//
// тип нового проекта
const QString kProjectTypeKey = kProjectGroupKey + "/type";
// папка сохранения проектов
const QString kProjectSaveFolderKey = kProjectGroupKey + "/save-folder";
// папка открытия проектов
const QString kProjectOpenFolderKey = kProjectGroupKey + "/open-folder";
// папка импорта проектов
const QString kProjectImportFolderKey = kProjectGroupKey + "/import-folder";
// папка экспорта проектов
const QString kProjectExportFolderKey = kProjectGroupKey + "/export-folder";
// путь до свойств конкретного проекта
QString projectKey(const QString& _path)
{
    return kProjectGroupKey + "/concrete/" + _path;
}
// путь до структуры проекта
QString projectStructureKey(const QString& _path)
{
    return projectKey(_path) + "/structure";
}
// был ли октрыт ли навигатор по проекту
QString projectStructureVisibleKey(const QString& _path)
{
    return projectKey(_path) + "/structure-visible";
}

//
// Система
//
const QString kSystemGroupKey = "system";
//
// системное имя пользователя
const QString kSystemUsernameKey = kSystemGroupKey + "/username";

//
// Компоненты
//
const QString kComponentsGroupKey = QStringLiteral("components");
//
// текст
const QString kComponentsSimpleTextKey = kComponentsGroupKey + QStringLiteral("/simple-text");
const QString kComponentsSimpleTextAvailableKey
    = kComponentsSimpleTextKey + QStringLiteral("/available");
// ... редактор
const QString kComponentsSimpleTextEditorKey = kComponentsSimpleTextKey + QStringLiteral("/editor");
const QString kComponentsSimpleTextEditorDefaultTemplateKey
    = kComponentsSimpleTextEditorKey + QStringLiteral("/default-template");
const QString kComponentsSimpleTextEditorCorrectTextOnPageBreaksKey
    = kComponentsSimpleTextEditorKey + QStringLiteral("/correct-text-on-page-breaks");
const QString kComponentsSimpleTextEditorShortcutsKey
    = kComponentsSimpleTextEditorKey + QStringLiteral("/shortcuts");
// ... навигатор
const QString kComponentsSimpleTextNavigatorKey
    = kComponentsSimpleTextKey + QStringLiteral("/navigator");
const QString kComponentsSimpleTextNavigatorShowSceneTextKey
    = kComponentsSimpleTextNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsSimpleTextNavigatorSceneTextLinesKey
    = kComponentsSimpleTextNavigatorKey + QStringLiteral("/scene-text-lines");
//
// сценарий
const QString kComponentsScreenplayKey = kComponentsGroupKey + QStringLiteral("/screenplay");
const QString kComponentsScreenplayAvailableKey
    = kComponentsScreenplayKey + QStringLiteral("/available");
// ... редактор тритмента
const QString kComponentsScreenplayTreatmentEditorKey
    = kComponentsScreenplayKey + QStringLiteral("/treatment");
const QString kComponentsScreenplayTreatmentEditorShortcutsKey
    = kComponentsScreenplayTreatmentEditorKey + QStringLiteral("/shortcuts");
// ... редактор текста
const QString kComponentsScreenplayEditorKey = kComponentsScreenplayKey + QStringLiteral("/editor");
const QString kComponentsScreenplayEditorDefaultTemplateKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/default-template");
const QString kComponentsScreenplayEditorShowSceneNumbersKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-scene-numbers");
const QString kComponentsScreenplayEditorShowSceneNumbersOnRightKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-scene-numbers-on-left");
const QString kComponentsScreenplayEditorShowSceneNumbersOnLeftKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-scene-number-on-right");
const QString kComponentsScreenplayEditorShowDialogueNumbersKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-dialogue-number");
const QString kComponentsScreenplayEditorContinueDialogueKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/continue-dialogue");
const QString kComponentsScreenplayEditorCorrectTextOnPageBreaksKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/correct-text-on-page-breaks");
const QString kComponentsScreenplayEditorSaveItemsFromTextKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/save-items-from-text");
const QString kComponentsScreenplayEditorShowHintsForAllItemsKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-hints-for-all-items");
const QString kComponentsScreenplayEditorShowHintsForPrimaryItemsKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-hints-for-primary-items");
const QString kComponentsScreenplayEditorShowHintsForSecondaryItemsKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-hints-for-secondary-items");
const QString kComponentsScreenplayEditorShowHintsForTertiaryItemsKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-hints-for-tertiary-items");
const QString kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-characters-hints-in-empty-blocks");
const QString kComponentsScreenplayEditorUseOpenBracketInDialogueForParentheticalKey
    = kComponentsScreenplayEditorKey
    + QStringLiteral("/use-open-bracket-in-dialogue-for-parenthetical");
const QString kComponentsScreenplayEditorShortcutsKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/shortcuts");
// ... навигатор
const QString kComponentsScreenplayNavigatorKey
    = kComponentsScreenplayKey + QStringLiteral("/navigator");
const QString kComponentsScreenplayNavigatorShowBeatsKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/show-beats");
const QString kComponentsScreenplayNavigatorShowBeatsInTreatmentKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/show-beats-in-treatment");
const QString kComponentsScreenplayNavigatorShowBeatsInScreenplayKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/show-beats-in-screenplay");
const QString kComponentsScreenplayNavigatorShowSceneNumberKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/show-scene-number");
const QString kComponentsScreenplayNavigatorShowSceneTextKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsScreenplayNavigatorSceneTextLinesKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/scene-text-lines");
// ... хронометраж
const QString kComponentsScreenplayDurationKey
    = kComponentsScreenplayKey + QStringLiteral("/duration");
const QString kComponentsScreenplayDurationTypeKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/type");
const QString kComponentsScreenplayDurationByPageDurationKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-page-duration");
const QString kComponentsScreenplayDurationByCharactersCharactersKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-characters-characters");
const QString kComponentsScreenplayDurationByCharactersIncludeSpacesKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-characters-include-spaces");
const QString kComponentsScreenplayDurationByCharactersDurationKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-characters-duration");
const QString kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey
    = kComponentsScreenplayDurationKey
    + QStringLiteral("/configurable-secsonds-per-paragraph-for-action");
const QString kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey
    = kComponentsScreenplayDurationKey
    + QStringLiteral("/configurable-secsonds-per-every50-for-action");
const QString kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey
    = kComponentsScreenplayDurationKey
    + QStringLiteral("/configurable-secsonds-per-paragraph-for-dialogue");
const QString kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey
    = kComponentsScreenplayDurationKey
    + QStringLiteral("/configurable-secsonds-per-every50-for-dialogue");
const QString kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey
    = kComponentsScreenplayDurationKey
    + QStringLiteral("/configurable-secsonds-per-paragraph-for-scene-heading");
const QString kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey
    = kComponentsScreenplayDurationKey
    + QStringLiteral("/configurable-secsonds-per-every50-for-scene-heading");
// ... редактор карточек
const QString kComponentsScreenplayCardsKey = kComponentsScreenplayKey + QStringLiteral("/cards");
const QString kComponentsScreenplayCardsSplitterStateKey
    = kComponentsScreenplayCardsKey + QStringLiteral("/splitter-state");
const QString kComponentsScreenplayCardsLockCardsKey
    = kComponentsScreenplayCardsKey + QStringLiteral("/lock-cards");
const QString kComponentsScreenplayCardsArrangeByRowsKey
    = kComponentsScreenplayCardsKey + QStringLiteral("/arrange-by-rows");
const QString kComponentsScreenplayCardsCardsSizeKey
    = kComponentsScreenplayCardsKey + QStringLiteral("/cards-size");
const QString kComponentsScreenplayCardsCardsRatioKey
    = kComponentsScreenplayCardsKey + QStringLiteral("/cards-ratio");
const QString kComponentsScreenplayCardsCardsSpacingKey
    = kComponentsScreenplayCardsKey + QStringLiteral("/cards-spacing");
const QString kComponentsScreenplayCardsCardsInRowKey
    = kComponentsScreenplayCardsKey + QStringLiteral("/cards-in-row");
// ... статистика
const QString kComponentsScreenplayStatisticsKey
    = kComponentsScreenplayKey + QStringLiteral("/statistics");
const QString kComponentsScreenplayStatisticsSplitterStateKey
    = kComponentsScreenplayStatisticsKey + QStringLiteral("/splitter-state");
//
// комикс
const QString kComponentsComicBookKey = kComponentsGroupKey + QStringLiteral("/comicbook");
const QString kComponentsComicBookAvailableKey
    = kComponentsComicBookKey + QStringLiteral("/available");
// ... редактор
const QString kComponentsComicBookEditorKey = kComponentsComicBookKey + QStringLiteral("/editor");
const QString kComponentsComicBookEditorDefaultTemplateKey
    = kComponentsComicBookEditorKey + QStringLiteral("/default-template");
const QString kComponentsComicBookEditorShowDialogueNumberKey
    = kComponentsComicBookEditorKey + QStringLiteral("/show-dialogue-number");
const QString kComponentsComicBookEditorSaveItemsFromTextKey
    = kComponentsComicBookEditorKey + QStringLiteral("/save-items-from-text");
const QString kComponentsComicBookEditorShowHintsForAllItemsKey
    = kComponentsComicBookEditorKey + QStringLiteral("/show-hints-for-all-items");
const QString kComponentsComicBookEditorShowHintsForPrimaryItemsKey
    = kComponentsComicBookEditorKey + QStringLiteral("/show-hints-for-primary-items");
const QString kComponentsComicBookEditorShowHintsForSecondaryItemsKey
    = kComponentsComicBookEditorKey + QStringLiteral("/show-hints-for-secondary-items");
const QString kComponentsComicBookEditorShowHintsForTertiaryItemsKey
    = kComponentsComicBookEditorKey + QStringLiteral("/show-hints-for-tertiary-items");
const QString kComponentsComicBookEditorShowCharacterSuggestionsInEmptyBlockKey
    = kComponentsComicBookEditorKey + QStringLiteral("/show-characters-hints-in-empty-blocks");
const QString kComponentsComicBookEditorShortcutsKey
    = kComponentsComicBookEditorKey + QStringLiteral("/shortcuts");
// ... навигатор
const QString kComponentsComicBookNavigatorKey
    = kComponentsComicBookKey + QStringLiteral("/navigator");
const QString kComponentsComicBookNavigatorShowSceneTextKey
    = kComponentsComicBookNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsComicBookNavigatorSceneTextLinesKey
    = kComponentsComicBookNavigatorKey + QStringLiteral("/scene-text-lines");
//
// аудиопостановка
const QString kComponentsAudioplayKey = kComponentsGroupKey + QStringLiteral("/audioplay");
const QString kComponentsAudioplayAvailableKey
    = kComponentsAudioplayKey + QStringLiteral("/available");
// ... редактор
const QString kComponentsAudioplayEditorKey = kComponentsAudioplayKey + QStringLiteral("/editor");
const QString kComponentsAudioplayEditorDefaultTemplateKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/default-template");
const QString kComponentsAudioplayEditorShowBlockNumbersKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/show-block-numbers");
const QString kComponentsAudioplayEditorContinueBlockNumbersKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/continue-block-numbers");
const QString kComponentsAudioplayEditorSaveItemsFromTextKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/save-items-from-text");
const QString kComponentsAudioplayEditorShowHintsForAllItemsKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/show-hints-for-all-items");
const QString kComponentsAudioplayEditorShowHintsForPrimaryItemsKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/show-hints-for-primary-items");
const QString kComponentsAudioplayEditorShowHintsForSecondaryItemsKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/show-hints-for-secondary-items");
const QString kComponentsAudioplayEditorShowHintsForTertiaryItemsKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/show-hints-for-tertiary-items");
const QString kComponentsAudioplayEditorShowCharacterSuggestionsInEmptyBlockKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/show-characters-hints-in-empty-blocks");
const QString kComponentsAudioplayEditorShortcutsKey
    = kComponentsAudioplayEditorKey + QStringLiteral("/shortcuts");
// ... навигатор
const QString kComponentsAudioplayNavigatorKey
    = kComponentsAudioplayKey + QStringLiteral("/navigator");
const QString kComponentsAudioplayNavigatorShowSceneNumberKey
    = kComponentsAudioplayNavigatorKey + QStringLiteral("/show-scene-number");
const QString kComponentsAudioplayNavigatorShowSceneTextKey
    = kComponentsAudioplayNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsAudioplayNavigatorSceneTextLinesKey
    = kComponentsAudioplayNavigatorKey + QStringLiteral("/scene-text-lines");
// ... хронометраж
const QString kComponentsAudioplayDurationKey
    = kComponentsAudioplayKey + QStringLiteral("/duration");
const QString kComponentsAudioplayDurationByWordsWordsKey
    = kComponentsAudioplayDurationKey + QStringLiteral("/by-words-characters");
const QString kComponentsAudioplayDurationByWordsDurationKey
    = kComponentsAudioplayDurationKey + QStringLiteral("/by-words-duration");
// ... статистика
const QString kComponentsAudioplayStatisticsKey
    = kComponentsAudioplayKey + QStringLiteral("/statistics");
const QString kComponentsAudioplayStatisticsSplitterStateKey
    = kComponentsAudioplayStatisticsKey + QStringLiteral("/splitter-state");
//
// пьеса
const QString kComponentsStageplayKey = kComponentsGroupKey + QStringLiteral("/stageplay");
const QString kComponentsStageplayAvailableKey
    = kComponentsStageplayKey + QStringLiteral("/available");
// ... редактор
const QString kComponentsStageplayEditorKey = kComponentsStageplayKey + QStringLiteral("/editor");
const QString kComponentsStageplayEditorDefaultTemplateKey
    = kComponentsStageplayEditorKey + QStringLiteral("/default-template");
const QString kComponentsStageplayEditorSaveItemsFromTextKey
    = kComponentsStageplayEditorKey + QStringLiteral("/save-items-from-text");
const QString kComponentsStageplayEditorShowHintsForAllItemsKey
    = kComponentsStageplayEditorKey + QStringLiteral("/show-hints-for-all-items");
const QString kComponentsStageplayEditorShowHintsForPrimaryItemsKey
    = kComponentsStageplayEditorKey + QStringLiteral("/show-hints-for-primary-items");
const QString kComponentsStageplayEditorShowHintsForSecondaryItemsKey
    = kComponentsStageplayEditorKey + QStringLiteral("/show-hints-for-secondary-items");
const QString kComponentsStageplayEditorShowHintsForTertiaryItemsKey
    = kComponentsStageplayEditorKey + QStringLiteral("/show-hints-for-tertiary-items");
const QString kComponentsStageplayEditorShowCharacterSuggestionsInEmptyBlockKey
    = kComponentsStageplayEditorKey + QStringLiteral("/show-characters-hints-in-empty-blocks");
const QString kComponentsStageplayEditorShortcutsKey
    = kComponentsStageplayEditorKey + QStringLiteral("/shortcuts");
// ... навигатор
const QString kComponentsStageplayNavigatorKey
    = kComponentsStageplayKey + QStringLiteral("/navigator");
const QString kComponentsStageplayNavigatorShowSceneNumberKey
    = kComponentsStageplayNavigatorKey + QStringLiteral("/show-scene-number");
const QString kComponentsStageplayNavigatorShowSceneTextKey
    = kComponentsStageplayNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsStageplayNavigatorSceneTextLinesKey
    = kComponentsStageplayNavigatorKey + QStringLiteral("/scene-text-lines");
//
// роман
const QString kComponentsNovelKey = kComponentsGroupKey + QStringLiteral("/novel");
const QString kComponentsNovelAvailableKey = kComponentsNovelKey + QStringLiteral("/available");
// ... редактор плана
const QString kComponentsNovelOutlineEditorKey = kComponentsNovelKey + QStringLiteral("/outline");
const QString kComponentsNovelOutlineEditorShortcutsKey
    = kComponentsNovelOutlineEditorKey + QStringLiteral("/shortcuts");
// ... редактор
const QString kComponentsNovelEditorKey = kComponentsNovelKey + QStringLiteral("/editor");
const QString kComponentsNovelEditorDefaultTemplateKey
    = kComponentsNovelEditorKey + QStringLiteral("/default-template");
const QString kComponentsNovelEditorCorrectTextOnPageBreaksKey
    = kComponentsNovelEditorKey + QStringLiteral("/correct-text-on-page-breaks");
const QString kComponentsNovelEditorShortcutsKey
    = kComponentsNovelEditorKey + QStringLiteral("/shortcuts");
// ... навигатор
const QString kComponentsNovelNavigatorKey = kComponentsNovelKey + QStringLiteral("/navigator");
const QString kComponentsNovelNavigatorShowBeatsKey
    = kComponentsNovelNavigatorKey + QStringLiteral("/show-beats");
const QString kComponentsNovelNavigatorShowBeatsInOutlineKey
    = kComponentsNovelNavigatorKey + QStringLiteral("/show-beats-in-outline");
const QString kComponentsNovelNavigatorShowBeatsInTextKey
    = kComponentsNovelNavigatorKey + QStringLiteral("/show-beats-in-text");
const QString kComponentsNovelNavigatorShowSceneTextKey
    = kComponentsNovelNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsNovelNavigatorSceneTextLinesKey
    = kComponentsNovelNavigatorKey + QStringLiteral("/scene-text-lines");
// ... редактор карточек
const QString kComponentsNovelCardsKey = kComponentsNovelKey + QStringLiteral("/cards");
const QString kComponentsNovelCardsSplitterStateKey
    = kComponentsNovelCardsKey + QStringLiteral("/splitter-state");
const QString kComponentsNovelCardsLockCardsKey
    = kComponentsNovelCardsKey + QStringLiteral("/lock-cards");
const QString kComponentsNovelCardsArrangeByRowsKey
    = kComponentsNovelCardsKey + QStringLiteral("/arrange-by-rows");
const QString kComponentsNovelCardsCardsSizeKey
    = kComponentsNovelCardsKey + QStringLiteral("/cards-size");
const QString kComponentsNovelCardsCardsRatioKey
    = kComponentsNovelCardsKey + QStringLiteral("/cards-ratio");
const QString kComponentsNovelCardsCardsSpacingKey
    = kComponentsNovelCardsKey + QStringLiteral("/cards-spacing");
const QString kComponentsNovelCardsCardsInRowKey
    = kComponentsNovelCardsKey + QStringLiteral("/cards-in-row");
// ... статистика
const QString kComponentsNovelStatisticsKey = kComponentsNovelKey + QStringLiteral("/statistics");
const QString kComponentsNovelStatisticsSplitterStateKey
    = kComponentsNovelStatisticsKey + QStringLiteral("/splitter-state");
//
// редактор отношений персонажей
const QString kComponentsCharactersRelationsKey
    = kComponentsGroupKey + QStringLiteral("/characters-relations");
const QString kComponentsCharactersRelationsFocusCurrentCharacterKey
    = kComponentsCharactersRelationsKey + QStringLiteral("/focus-current-character");
const QString kComponentsCharactersRelationsSplitterStateKey
    = kComponentsCharactersRelationsKey + QStringLiteral("/splitter-state");
//
// редактор карты локаций
const QString kComponentsLocationsMapKey = kComponentsGroupKey + QStringLiteral("/locations-map");
const QString kComponentsLocationsMapFocusCurrentLocationKey
    = kComponentsLocationsMapKey + QStringLiteral("/focus-current-location");
const QString kComponentsLocationsMapSplitterStateKey
    = kComponentsLocationsMapKey + QStringLiteral("/splitter-state");
//
// редактор карты миров
const QString kComponentsWorldsMapKey = kComponentsGroupKey + QStringLiteral("/worlds-map");
const QString kComponentsWorldsMapFocusCurrentWorldKey
    = kComponentsWorldsMapKey + QStringLiteral("/focus-current-world");
const QString kComponentsWorldsMapSplitterStateKey
    = kComponentsWorldsMapKey + QStringLiteral("/splitter-state");

} // namespace

/**
 * @brief Хранилище настроек
 */
class CORE_LIBRARY_EXPORT SettingsStorage
{
public:
    enum class SettingsPlace { Application, Project };

public:
    ~SettingsStorage();

    /**
     * @brief Сохранить значение с заданным ключём
     */
    void setValue(const QString& _key, const QVariant& _value, SettingsPlace _settingsPlace);

    /**
     * @brief Сохранить карту параметров
     */
    void setValues(const QString& _valuesGroup, const QVariantMap& _values,
                   SettingsPlace _settingsPlace);

    /**
     * @brief Получить значение по ключу
     */
    QVariant value(const QString& _key, SettingsPlace _settingsPlace,
                   const QVariant& _defaultValue = {}) const;

    /**
     * @brief Получить группу значений
     */
    QVariantMap values(const QString& _valuesGroup, SettingsPlace _settingsPlace);

    //
    // Вспомогательные функции для работы с данными о пользователе
    //

    /**
     * @brief Получить имя пользователя (если авторизован - имя пользователя в облаке,
     *        если не авторизован - имя пользователя в системе)
     */
    QString accountName() const;

    /**
     * @brief Получить имейл пользователя (если не авторизован возвращает пустое значение)
     */
    QString accountEmail() const;


    //
    // Вспомогательные функции для работы с путями к специальным папкам и файлам в них
    //

    /**
     * @brief Получить путь к папке с документами для сохранения по заданному ключу
     */
    QString documentFolderPath(const QString& _key) const;

    /**
     * @brief Получить путь к файлу в папке с документами для заданного ключа и имени файла
     */
    QString documentFilePath(const QString& _key, const QString& _fileName) const;

    /**
     * @brief Сохранить путь к папке с документами по заданному ключу и пути файла из этой папки
     */
    void setDocumentFolderPath(const QString& _key, const QString& _filePath);


    /**
     * @brief Сбросить все заданные пользователем настройки (по-сути удаляем файл с настройками)
     */
    void resetToDefaults();

private:
    SettingsStorage();
    friend class StorageFacade;
    //
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer
