#include "settings_view.h"

#include "widgets/theme_preview.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/slider/slider.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/toggle/toggle.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>
#include <ui/widgets/tree/tree_header_view.h>

#include <QAction>
#include <QFileDialog>
#include <QGridLayout>
#include <QLocale>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QVariantAnimation>
#include <QtMath>


namespace Ui {

namespace {

/**
 * @brief Сформиовать компоновщик для строки настроек
 */
QHBoxLayout* makeLayout()
{
    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    return layout;
};

/**
 * @brief Карта соответствия названий языков для проверки орфографии с их кодами
 * @link https://github.com/wooorm/dictionaries/
 * @link https://cgit.freedesktop.org/libreoffice/dictionaries/tree/
 * @link https://www.softmaker.com/en/dictionaries
 */
const QVector<QString> kSpellCheckerLanguagesNameToCode = {
    "af",    "an",         "ar",          "ast",   "az",       "be",    "bg",    "bn",    "bo",
    "br",    "bs",         "ca-valencia", "ca",    "cs",       "cy",    "da",    "de-AT", "de-CH",
    "de",    "el-polyton", "el",          "en-AU", "en-CA",    "en-GB", "en-NZ", "en-ZA", "en",
    "eo",    "es-AR",      "es-BO",       "es-CL", "es-CO",    "es-CR", "es-CU", "es-DO", "es-EC",
    "es-GT", "es-HN",      "es-MX",       "es-NI", "es-PA",    "es-PE", "es-PH", "es-PR", "es-PY",
    "es-SV", "es-US",      "es-UY",       "es-VE", "es",       "et",    "eu",    "fa",    "fo",
    "fr",    "fur",        "fy",          "ga",    "gd",       "gl",    "gu",    "gug",   "he",
    "hi",    "hr",         "hu",          "hy",    "hyw",      "ia",    "id",    "is",    "it",
    "ka",    "kk",         "kmr",         "ko",    "la",       "lb",    "lo",    "lt",    "ltg",
    "lv",    "mk",         "mn",          "mt",    "nb",       "nds",   "ne",    "nl",    "nn",
    "oc",    "pl",         "pt-BR",       "pt",    "qu",       "ro",    "ru-yo", "ru",    "rw",
    "si",    "sk",         "sl",          "sq",    "sr-Latn",  "sr",    "sv-FI", "sv",    "sw",
    "ta",    "te",         "th",          "tk",    "tlh-Latn", "tlh",   "tr",    "uk",    "vi"
};

/**
 * @brief Индекс для сохранения в модели информации о коде языка
 */
const int kSpellCheckerLanguageCodeRole = Qt::UserRole + 1;

/**
 * @brief Построить модель для всех доступных справочников проверки орфографии
 */
QStandardItemModel* buildSpellCheckerLanguagesModel(QObject* _parent)
{
    auto model = new QStandardItemModel(_parent);
    for (const auto& language : kSpellCheckerLanguagesNameToCode) {
        auto item = new QStandardItem(language);
        item->setData(language, kSpellCheckerLanguageCodeRole);
        model->appendRow(item);
    }
    return model;
}

/**
 * @brief Построить модель типов абзацев для модуля
 */
QStringListModel* buildParagraphTypesModel(QObject* _parent, QStringListModel* _model,
                                           const QVector<BusinessLayer::TextParagraphType>& _types)
{
    using namespace BusinessLayer;
    QStringList paragraphTypes;
    for (const auto& type : _types) {
        paragraphTypes.append(toDisplayString(type));
    }

    if (!_model) {
        _model = new QStringListModel(_parent);
    }
    _model->setStringList(paragraphTypes);

    return _model;
}
QStringListModel* buildSimpleTextParagraphTypesModel(QObject* _parent,
                                                     QStringListModel* _model = nullptr)
{
    using namespace BusinessLayer;
    return buildParagraphTypesModel(_parent, _model,
                                    {
                                        TextParagraphType::ChapterHeading1,
                                        TextParagraphType::ChapterHeading2,
                                        TextParagraphType::ChapterHeading3,
                                        TextParagraphType::ChapterHeading4,
                                        TextParagraphType::ChapterHeading5,
                                        TextParagraphType::ChapterHeading6,
                                        TextParagraphType::Text,
                                        TextParagraphType::InlineNote,
                                    });
}
QStringListModel* buildScreenplayParagraphTypesModel(QObject* _parent,
                                                     QStringListModel* _model = nullptr)
{
    using namespace BusinessLayer;
    return buildParagraphTypesModel(_parent, _model,
                                    {
                                        TextParagraphType::SceneHeading,
                                        TextParagraphType::SceneCharacters,
                                        TextParagraphType::BeatHeading,
                                        TextParagraphType::Action,
                                        TextParagraphType::Character,
                                        TextParagraphType::Parenthetical,
                                        TextParagraphType::Dialogue,
                                        TextParagraphType::Lyrics,
                                        TextParagraphType::Transition,
                                        TextParagraphType::Shot,
                                        TextParagraphType::InlineNote,
                                        TextParagraphType::SequenceHeading,
                                        TextParagraphType::ActHeading,
                                        TextParagraphType::UnformattedText,
                                    });
}
QStringListModel* buildComicBookParagraphTypesModel(QObject* _parent,
                                                    QStringListModel* _model = nullptr)
{
    using namespace BusinessLayer;
    return buildParagraphTypesModel(_parent, _model,
                                    {
                                        TextParagraphType::PageHeading,
                                        TextParagraphType::PanelHeading,
                                        TextParagraphType::Description,
                                        TextParagraphType::Character,
                                        TextParagraphType::Parenthetical,
                                        TextParagraphType::Dialogue,
                                        TextParagraphType::InlineNote,
                                        TextParagraphType::UnformattedText,
                                        TextParagraphType::SequenceHeading,
                                    });
}
QStringListModel* buildAudioplayParagraphTypesModel(QObject* _parent,
                                                    QStringListModel* _model = nullptr)
{
    using namespace BusinessLayer;
    return buildParagraphTypesModel(_parent, _model,
                                    {
                                        TextParagraphType::SceneHeading,
                                        TextParagraphType::Character,
                                        TextParagraphType::Dialogue,
                                        TextParagraphType::Sound,
                                        TextParagraphType::Music,
                                        TextParagraphType::Cue,
                                        TextParagraphType::InlineNote,
                                        TextParagraphType::UnformattedText,
                                    });
}
QStringListModel* buildStageplayParagraphTypesModel(QObject* _parent,
                                                    QStringListModel* _model = nullptr)
{
    using namespace BusinessLayer;
    return buildParagraphTypesModel(_parent, _model,
                                    {
                                        TextParagraphType::SceneHeading,
                                        TextParagraphType::Character,
                                        TextParagraphType::Parenthetical,
                                        TextParagraphType::Dialogue,
                                        TextParagraphType::Action,
                                        TextParagraphType::InlineNote,
                                        TextParagraphType::UnformattedText,
                                    });
}
QStringListModel* buildNovelParagraphTypesModel(QObject* _parent,
                                                QStringListModel* _model = nullptr)
{
    using namespace BusinessLayer;
    return buildParagraphTypesModel(_parent, _model,
                                    {
                                        TextParagraphType::SceneHeading,
                                        TextParagraphType::BeatHeading,
                                        TextParagraphType::Text,
                                        TextParagraphType::InlineNote,
                                        TextParagraphType::UnformattedText,
                                        TextParagraphType::ChapterHeading,
                                        TextParagraphType::PartHeading,
                                    });
}

/**
 * @brief Колонки таблицы горячих клавиш специализированного редактора текста
 */
enum TextEditorShortcuts {
    Type = 0,
    Shortcut,
    JumpByTab,
    JumpByEnter,
    ChangeByTab,
    ChangeByEnter,
};

} // namespace

class SettingsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Настроить карточку параметров приложения
     */
    void initApplicationCard();

    /**
     * @brief Настроить карточку параметров компонентов
     */
    void initSimpleTextCard();
    void initScreenplayCard();
    void initComicBookCard();
    void initAudioplayCard();
    void initStageplayCard();
    void initNovelCard();

    /**
     * @brief Настроить карточку горячих клавиш
     */
    void initShortcutsCard();

    /**
     * @brief Проскролить представление до заданного виджета
     */
    void scrollToTitle(AbstractLabel* title);

    /**
     * @brief Обновить размеры колонок в таблицах
     */
    void updateTablesGeometry();


    QScrollArea* content = nullptr;
    QVariantAnimation scrollAnimation;
    ContextMenu* contextMenu = nullptr;

    QVariantAnimation colorAnimation;
    AbstractLabel* colorableTitle = nullptr;

    //
    // Application
    //
    Card* applicationCard = nullptr;
    QGridLayout* applicationCardLayout = nullptr;
    H5Label* applicationTitle = nullptr;
    //
    // ... Common
    //
    Body1Label* language = nullptr;
    Button* changeLanuage = nullptr;
    //
    // ... User interface
    //
    H6Label* applicationUserInterfaceTitle = nullptr;
    QHBoxLayout* applicationThemesLayout = nullptr;
    ThemePreview* lightTheme = nullptr;
    ThemePreview* darkAndLightTheme = nullptr;
    ThemePreview* darkTheme = nullptr;
    ThemePreview* customTheme = nullptr;
    Body1Label* scaleFactorTitle = nullptr;
    Slider* scaleFactor = nullptr;
    Body2Label* scaleFactorSmallInfo = nullptr;
    Body2Label* scaleFactorBigInfo = nullptr;
    CheckBox* isCompact = nullptr;
    //
    // ... Save changes & backups
    //
    H6Label* applicationSaveAndBackupTitle = nullptr;
    CheckBox* autoSave = nullptr;
    CheckBox* saveBackups = nullptr;
    TextField* backupsFolderPath = nullptr;
    TextField* backupsQty = nullptr;
    //
    // ... Text editing
    //
    H6Label* applicationTextEditingTitle = nullptr;
    CheckBox* showDocumentsPages = nullptr;
    CheckBox* useTypewriterSound = nullptr;
    CheckBox* useSpellChecker = nullptr;
    ComboBox* spellCheckerLanguage = nullptr;
    QStandardItemModel* spellCheckerLanguagesModel = nullptr;
    IconButton* spellCheckerUserDictionary = nullptr;
    CheckBox* highlightCurrentLine = nullptr;
    CheckBox* focusCurrentParagraph = nullptr;
    CheckBox* useTypewriterScrolling = nullptr;
    Subtitle1Label* applicationTextEditingSubtitle = nullptr;
    CheckBox* correctDoubleCapitals = nullptr;
    CheckBox* capitalizeSingleILetter = nullptr;
    CheckBox* replaceThreeDotsWithEllipsis = nullptr;
    CheckBox* useSmartQuotes = nullptr;
    CheckBox* replaceTwoDashesWithEmDash = nullptr;
    CheckBox* avoidMultipleSpaces = nullptr;
    //
    int applicationCardBottomSpacerIndex = 0;

    H5Label* componentsTitle = nullptr;

    //
    // Comonents
    //
    // Simple text
    //
    Card* simpleTextCard = nullptr;
    QGridLayout* simpleTextCardLayout = nullptr;
    H5Label* simpleTextTitle = nullptr;
    Toggle* simpleTextAvailable = nullptr;
    //
    // ... Simple text editor
    //
    H6Label* simpleTextEditorTitle = nullptr;
    ComboBox* simpleTextEditorDefaultTemplate = nullptr;
    IconButton* simpleTextEditorDefaultTemplateOptions = nullptr;
    //
    // ... Simple text navigator
    //
    H6Label* simpleTextNavigatorTitle = nullptr;
    CheckBox* simpleTextNavigatorShowSceneText = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines5 = nullptr;
    //
    int simpleTextCardBottomSpacerIndex = 0;
    //
    // Screenplay
    //
    Card* screenplayCard = nullptr;
    QGridLayout* screenplayCardLayout = nullptr;
    H5Label* screenplayTitle = nullptr;
    Toggle* screenplayAvailable = nullptr;
    //
    // ... Screenplay editor
    //
    H6Label* screenplayEditorTitle = nullptr;
    ComboBox* screenplayEditorDefaultTemplate = nullptr;
    IconButton* screenplayEditorDefaultTemplateOptions = nullptr;
    CheckBox* screenplayEditorShowSceneNumber = nullptr;
    CheckBox* screenplayEditorShowSceneNumberOnLeft = nullptr;
    CheckBox* screenplayEditorShowSceneNumberOnRight = nullptr;
    CheckBox* screenplayEditorShowDialogueNumber = nullptr;
    CheckBox* screenplayEditorContinueDialogue = nullptr;
    CheckBox* screenplayEditorCorrectTextOnPageBreaks = nullptr;
    CheckBox* screenplayEditorSaveItemsFromText = nullptr;
    CheckBox* screenplayEditorShowHintsForAllItems = nullptr;
    CheckBox* screenplayEditorShowHintsForPrimaryItems = nullptr;
    CheckBox* screenplayEditorShowHintsForSecondaryItems = nullptr;
    CheckBox* screenplayEditorShowHintsForTertiaryItems = nullptr;
    CheckBox* screenplayEditorShowCharacterSuggestionsInEmptyBlock = nullptr;
    CheckBox* screenplayEditorUseOpenBracketInDialogueForParenthetical = nullptr;
    //
    // ... Screenplay navigator
    //
    H6Label* screenplayNavigatorTitle = nullptr;
    CheckBox* screenplayNavigatorShowBeats = nullptr;
    CheckBox* screenplayNavigatorShowBeatsInTreatment = nullptr;
    CheckBox* screenplayNavigatorShowBeatsInScreenplay = nullptr;
    CheckBox* screenplayNavigatorShowSceneNumber = nullptr;
    CheckBox* screenplayNavigatorShowSceneText = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines5 = nullptr;
    //
    // ... Screenplay duration
    //
    H6Label* screenplayDurationTitle = nullptr;
    RadioButton* screenplayDurationByPage = nullptr;
    TextField* screenplayDurationByPagePage = nullptr;
    TextField* screenplayDurationByPageDuration = nullptr;
    //
    RadioButton* screenplayDurationByCharacters = nullptr;
    TextField* screenplayDurationByCharactersCharacters = nullptr;
    CheckBox* screenplayDurationByCharactersIncludingSpaces = nullptr;
    TextField* screenplayDurationByCharactersDuration = nullptr;
    //
    RadioButton* screenplayDurationConfigurable = nullptr;
    Body1Label* screenplayDurationConfigurableForActionLabel = nullptr;
    TextField* screenplayDurationConfigurablePerParagraphForAction = nullptr;
    Body1Label* screenplayDurationConfigurableForActionPlus = nullptr;
    TextField* screenplayDurationConfigurablePerEvery50ForAction = nullptr;
    Body1Label* screenplayDurationConfigurableForDialogueLabel = nullptr;
    TextField* screenplayDurationConfigurablePerParagraphForDialogue = nullptr;
    Body1Label* screenplayDurationConfigurableForDialoguePlus = nullptr;
    TextField* screenplayDurationConfigurablePerEvery50ForDialogue = nullptr;
    Body1Label* screenplayDurationConfigurableForSceneHeadingLabel = nullptr;
    TextField* screenplayDurationConfigurablePerParagraphForSceneHeading = nullptr;
    Body1Label* screenplayDurationConfigurableForSceneHeadingPlus = nullptr;
    TextField* screenplayDurationConfigurablePerEvery50ForSceneHeading = nullptr;
    //
    int screenplayCardBottomSpacerIndex = 0;
    //
    // Comic book
    //
    Card* comicBookCard = nullptr;
    QGridLayout* comicBookCardLayout = nullptr;
    H5Label* comicBookTitle = nullptr;
    Toggle* comicBookAvailable = nullptr;
    //
    // ... Comic book editor
    //
    H6Label* comicBookEditorTitle = nullptr;
    ComboBox* comicBookEditorDefaultTemplate = nullptr;
    IconButton* comicBookEditorDefaultTemplateOptions = nullptr;
    CheckBox* comicBookEditorShowDialogueNumber = nullptr;
    CheckBox* comicBookEditorSaveItemsFromText = nullptr;
    CheckBox* comicBookEditorShowHintsForAllItems = nullptr;
    CheckBox* comicBookEditorShowHintsForPrimaryItems = nullptr;
    CheckBox* comicBookEditorShowHintsForSecondaryItems = nullptr;
    CheckBox* comicBookEditorShowHintsForTertiaryItems = nullptr;
    CheckBox* comicBookEditorShowCharacterSuggestionsInEmptyBlock = nullptr;
    //
    // ... Comic book navigator
    //
    H6Label* comicBookNavigatorTitle = nullptr;
    CheckBox* comicBookNavigatorShowSceneText = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines5 = nullptr;
    //
    int comicBookCardBottomSpacerIndex = 0;
    //
    // Audioplay
    //
    Card* audioplayCard = nullptr;
    QGridLayout* audioplayCardLayout = nullptr;
    H5Label* audioplayTitle = nullptr;
    Toggle* audioplayAvailable = nullptr;
    //
    // ... Audioplay editor
    //
    H6Label* audioplayEditorTitle = nullptr;
    ComboBox* audioplayEditorDefaultTemplate = nullptr;
    IconButton* audioplayEditorDefaultTemplateOptions = nullptr;
    CheckBox* audioplayEditorShowBlockNumber = nullptr;
    CheckBox* audioplayEditorContinueBlockNumbers = nullptr;
    CheckBox* audioplayEditorSaveItemsFromText = nullptr;
    CheckBox* audioplayEditorShowHintsForAllItems = nullptr;
    CheckBox* audioplayEditorShowHintsForPrimaryItems = nullptr;
    CheckBox* audioplayEditorShowHintsForSecondaryItems = nullptr;
    CheckBox* audioplayEditorShowHintsForTertiaryItems = nullptr;
    CheckBox* audioplayEditorShowCharacterSuggestionsInEmptyBlock = nullptr;
    //
    // ... Audioplay navigator
    //
    H6Label* audioplayNavigatorTitle = nullptr;
    CheckBox* audioplayNavigatorShowSceneNumber = nullptr;
    CheckBox* audioplayNavigatorShowSceneText = nullptr;
    RadioButton* audioplayNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* audioplayNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* audioplayNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* audioplayNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* audioplayNavigatorSceneDescriptionLines5 = nullptr;
    //
    // ... Audioplay duration
    //
    H6Label* audioplayDurationTitle = nullptr;
    Body1Label* audioplayDurationByWordsTitle = nullptr;
    TextField* audioplayDurationByWordsWords = nullptr;
    TextField* audioplayDurationByWordsDuration = nullptr;
    //
    int audioplayCardBottomSpacerIndex = 0;
    //
    // Stageplay
    //
    Card* stageplayCard = nullptr;
    QGridLayout* stageplayCardLayout = nullptr;
    H5Label* stageplayTitle = nullptr;
    Toggle* stageplayAvailable = nullptr;
    //
    // ... Stageplay editor
    //
    H6Label* stageplayEditorTitle = nullptr;
    ComboBox* stageplayEditorDefaultTemplate = nullptr;
    IconButton* stageplayEditorDefaultTemplateOptions = nullptr;
    CheckBox* stageplayEditorSaveItemsFromText = nullptr;
    CheckBox* stageplayEditorShowHintsForAllItems = nullptr;
    CheckBox* stageplayEditorShowHintsForPrimaryItems = nullptr;
    CheckBox* stageplayEditorShowHintsForSecondaryItems = nullptr;
    CheckBox* stageplayEditorShowHintsForTertiaryItems = nullptr;
    CheckBox* stageplayEditorShowCharacterSuggestionsInEmptyBlock = nullptr;
    //
    // ... Stageplay navigator
    //
    H6Label* stageplayNavigatorTitle = nullptr;
    CheckBox* stageplayNavigatorShowSceneNumber = nullptr;
    CheckBox* stageplayNavigatorShowSceneText = nullptr;
    RadioButton* stageplayNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* stageplayNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* stageplayNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* stageplayNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* stageplayNavigatorSceneDescriptionLines5 = nullptr;
    //
    int stageplayCardBottomSpacerIndex = 0;
    //
    // Simple text
    //
    Card* novelCard = nullptr;
    QGridLayout* novelCardLayout = nullptr;
    H5Label* novelTitle = nullptr;
    Toggle* novelAvailable = nullptr;
    //
    // ... Novel text editor
    //
    H6Label* novelEditorTitle = nullptr;
    ComboBox* novelEditorDefaultTemplate = nullptr;
    IconButton* novelEditorDefaultTemplateOptions = nullptr;
    //
    // ... Novel text navigator
    //
    H6Label* novelNavigatorTitle = nullptr;
    CheckBox* novelNavigatorShowSceneText = nullptr;
    RadioButton* novelNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* novelNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* novelNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* novelNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* novelNavigatorSceneDescriptionLines5 = nullptr;
    Subtitle1Label* novelNavigatorCounterType = nullptr;
    RadioButton* novelNavigatorCounterUseWords = nullptr;
    RadioButton* novelNavigatorCounterUseCharacters = nullptr;
    RadioButton* novelNavigatorCounterUseCharactersWithSpaces = nullptr;
    //
    int novelCardBottomSpacerIndex = 0;

    //
    // Shortcuts
    //
    Card* shortcutsCard = nullptr;
    QGridLayout* shortcutsCardLayout = nullptr;
    //
    H5Label* shortcutsTitle = nullptr;
    //
    // ... Appliction
    //
    H6Label* shortcutsForApplicationTitle = nullptr;
    Tree* shortcutsForApplication = nullptr;
    HierarchicalModel* shortcutsForApplicationModel = nullptr;
    //
    // ... Simple text
    //
    H6Label* shortcutsForSimpleTextTitle = nullptr;
    Tree* shortcutsForSimpleText = nullptr;
    HierarchicalModel* shortcutsForSimpleTextModel = nullptr;
    QStringListModel* simpleTextParagraphTypesModel = nullptr;
    ComboBoxItemDelegate* simpleTextParagraphAddTypeDelegate = nullptr;
    ComboBoxItemDelegate* simpleTextParagraphChangeTypeDelegate = nullptr;
    //
    // ... Screenplay
    //
    H6Label* shortcutsForScreenplayTitle = nullptr;
    Tree* shortcutsForScreenplay = nullptr;
    HierarchicalModel* shortcutsForScreenplayModel = nullptr;
    QStringListModel* screenplayParagraphTypesModel = nullptr;
    ComboBoxItemDelegate* screenplayParagraphAddTypeDelegate = nullptr;
    ComboBoxItemDelegate* screenplayParagraphChangeTypeDelegate = nullptr;
    //
    // ... ComicBook
    //
    H6Label* shortcutsForComicBookTitle = nullptr;
    Tree* shortcutsForComicBook = nullptr;
    HierarchicalModel* shortcutsForComicBookModel = nullptr;
    QStringListModel* comicBookParagraphTypesModel = nullptr;
    ComboBoxItemDelegate* comicBookParagraphAddTypeDelegate = nullptr;
    ComboBoxItemDelegate* comicBookParagraphChangeTypeDelegate = nullptr;
    //
    // ... Audioplay
    //
    H6Label* shortcutsForAudioplayTitle = nullptr;
    Tree* shortcutsForAudioplay = nullptr;
    HierarchicalModel* shortcutsForAudioplayModel = nullptr;
    QStringListModel* audioplayParagraphTypesModel = nullptr;
    ComboBoxItemDelegate* audioplayParagraphAddTypeDelegate = nullptr;
    ComboBoxItemDelegate* audioplayParagraphChangeTypeDelegate = nullptr;
    //
    // ... Stageplay
    //
    H6Label* shortcutsForStageplayTitle = nullptr;
    Tree* shortcutsForStageplay = nullptr;
    HierarchicalModel* shortcutsForStageplayModel = nullptr;
    QStringListModel* stageplayParagraphTypesModel = nullptr;
    ComboBoxItemDelegate* stageplayParagraphAddTypeDelegate = nullptr;
    ComboBoxItemDelegate* stageplayParagraphChangeTypeDelegate = nullptr;
    //
    // ... Novel
    //
    H6Label* shortcutsForNovelTitle = nullptr;
    Tree* shortcutsForNovel = nullptr;
    HierarchicalModel* shortcutsForNovelModel = nullptr;
    QStringListModel* novelParagraphTypesModel = nullptr;
    ComboBoxItemDelegate* novelParagraphAddTypeDelegate = nullptr;
    ComboBoxItemDelegate* novelParagraphChangeTypeDelegate = nullptr;
    //
    int shortcutsCardBottomSpacerIndex = 0;
};

SettingsView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , contextMenu(new ContextMenu(_parent))
    //
    , applicationCard(new Card(content))
    , applicationCardLayout(new QGridLayout)
    , applicationTitle(new H5Label(applicationCard))
    , language(new Body1Label(applicationCard))
    , changeLanuage(new Button(applicationCard))
    , applicationUserInterfaceTitle(new H6Label(applicationCard))
    , applicationThemesLayout(new QHBoxLayout)
    , lightTheme(new ThemePreview(applicationCard))
    , darkAndLightTheme(new ThemePreview(applicationCard))
    , darkTheme(new ThemePreview(applicationCard))
    , customTheme(new ThemePreview(applicationCard))
    , scaleFactorTitle(new Body1Label(applicationCard))
    , scaleFactor(new Slider(applicationCard))
    , scaleFactorSmallInfo(new Body2Label(applicationCard))
    , scaleFactorBigInfo(new Body2Label(applicationCard))
    , isCompact(new CheckBox(applicationCard))
    , applicationSaveAndBackupTitle(new H6Label(applicationCard))
    , autoSave(new CheckBox(applicationCard))
    , saveBackups(new CheckBox(applicationCard))
    , backupsFolderPath(new TextField(applicationCard))
    , backupsQty(new TextField(applicationCard))
    , applicationTextEditingTitle(new H6Label(applicationCard))
    , showDocumentsPages(new CheckBox(applicationCard))
    , useTypewriterSound(new CheckBox(applicationCard))
    , useSpellChecker(new CheckBox(applicationCard))
    , spellCheckerLanguage(new ComboBox(applicationCard))
    , spellCheckerLanguagesModel(buildSpellCheckerLanguagesModel(spellCheckerLanguage))
    , spellCheckerUserDictionary(new IconButton(applicationCard))
    , highlightCurrentLine(new CheckBox(applicationCard))
    , focusCurrentParagraph(new CheckBox(applicationCard))
    , useTypewriterScrolling(new CheckBox(applicationCard))
    , applicationTextEditingSubtitle(new Subtitle1Label(applicationCard))
    , correctDoubleCapitals(new CheckBox(applicationCard))
    , capitalizeSingleILetter(new CheckBox(applicationCard))
    , replaceThreeDotsWithEllipsis(new CheckBox(applicationCard))
    , useSmartQuotes(new CheckBox(applicationCard))
    , replaceTwoDashesWithEmDash(new CheckBox(applicationCard))
    , avoidMultipleSpaces(new CheckBox(applicationCard))
    //
    , componentsTitle(new H5Label(content))
    //
    , simpleTextCard(new Card(content))
    , simpleTextCardLayout(new QGridLayout)
    , simpleTextTitle(new H5Label(simpleTextCard))
    , simpleTextAvailable(new Toggle(simpleTextCard))
    , simpleTextEditorTitle(new H6Label(simpleTextCard))
    , simpleTextEditorDefaultTemplate(new ComboBox(simpleTextCard))
    , simpleTextEditorDefaultTemplateOptions(new IconButton(simpleTextCard))
    , simpleTextNavigatorTitle(new H6Label(simpleTextCard))
    , simpleTextNavigatorShowSceneText(new CheckBox(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines1(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines2(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines3(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines4(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines5(new RadioButton(simpleTextCard))
    //
    , screenplayCard(new Card(content))
    , screenplayCardLayout(new QGridLayout)
    , screenplayTitle(new H5Label(screenplayCard))
    , screenplayAvailable(new Toggle(screenplayCard))
    , screenplayEditorTitle(new H6Label(screenplayCard))
    , screenplayEditorDefaultTemplate(new ComboBox(screenplayCard))
    , screenplayEditorDefaultTemplateOptions(new IconButton(screenplayCard))
    , screenplayEditorShowSceneNumber(new CheckBox(screenplayCard))
    , screenplayEditorShowSceneNumberOnLeft(new CheckBox(screenplayCard))
    , screenplayEditorShowSceneNumberOnRight(new CheckBox(screenplayCard))
    , screenplayEditorShowDialogueNumber(new CheckBox(screenplayCard))
    , screenplayEditorContinueDialogue(new CheckBox(screenplayCard))
    , screenplayEditorCorrectTextOnPageBreaks(new CheckBox(screenplayCard))
    , screenplayEditorSaveItemsFromText(new CheckBox(screenplayCard))
    , screenplayEditorShowHintsForAllItems(new CheckBox(screenplayCard))
    , screenplayEditorShowHintsForPrimaryItems(new CheckBox(screenplayCard))
    , screenplayEditorShowHintsForSecondaryItems(new CheckBox(screenplayCard))
    , screenplayEditorShowHintsForTertiaryItems(new CheckBox(screenplayCard))
    , screenplayEditorShowCharacterSuggestionsInEmptyBlock(new CheckBox(screenplayCard))
    , screenplayEditorUseOpenBracketInDialogueForParenthetical(new CheckBox(screenplayCard))
    , screenplayNavigatorTitle(new H6Label(screenplayCard))
    , screenplayNavigatorShowBeats(new CheckBox(screenplayCard))
    , screenplayNavigatorShowBeatsInTreatment(new CheckBox(screenplayCard))
    , screenplayNavigatorShowBeatsInScreenplay(new CheckBox(screenplayCard))
    , screenplayNavigatorShowSceneNumber(new CheckBox(screenplayCard))
    , screenplayNavigatorShowSceneText(new CheckBox(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines1(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines2(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines3(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines4(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines5(new RadioButton(screenplayCard))
    , screenplayDurationTitle(new H6Label(screenplayCard))
    , screenplayDurationByPage(new RadioButton(screenplayCard))
    , screenplayDurationByPagePage(new TextField(screenplayCard))
    , screenplayDurationByPageDuration(new TextField(screenplayCard))
    , screenplayDurationByCharacters(new RadioButton(screenplayCard))
    , screenplayDurationByCharactersCharacters(new TextField(screenplayCard))
    , screenplayDurationByCharactersIncludingSpaces(new CheckBox(screenplayCard))
    , screenplayDurationByCharactersDuration(new TextField(screenplayCard))
    , screenplayDurationConfigurable(new RadioButton(screenplayCard))
    , screenplayDurationConfigurableForActionLabel(new Body1Label(screenplayCard))
    , screenplayDurationConfigurablePerParagraphForAction(new TextField(screenplayCard))
    , screenplayDurationConfigurableForActionPlus(new Body1Label(screenplayCard))
    , screenplayDurationConfigurablePerEvery50ForAction(new TextField(screenplayCard))
    , screenplayDurationConfigurableForDialogueLabel(new Body1Label(screenplayCard))
    , screenplayDurationConfigurablePerParagraphForDialogue(new TextField(screenplayCard))
    , screenplayDurationConfigurableForDialoguePlus(new Body1Label(screenplayCard))
    , screenplayDurationConfigurablePerEvery50ForDialogue(new TextField(screenplayCard))
    , screenplayDurationConfigurableForSceneHeadingLabel(new Body1Label(screenplayCard))
    , screenplayDurationConfigurablePerParagraphForSceneHeading(new TextField(screenplayCard))
    , screenplayDurationConfigurableForSceneHeadingPlus(new Body1Label(screenplayCard))
    , screenplayDurationConfigurablePerEvery50ForSceneHeading(new TextField(screenplayCard))
    //
    , comicBookCard(new Card(content))
    , comicBookCardLayout(new QGridLayout)
    , comicBookTitle(new H5Label(comicBookCard))
    , comicBookAvailable(new Toggle(comicBookCard))
    , comicBookEditorTitle(new H6Label(comicBookCard))
    , comicBookEditorDefaultTemplate(new ComboBox(comicBookCard))
    , comicBookEditorDefaultTemplateOptions(new IconButton(comicBookCard))
    , comicBookEditorShowDialogueNumber(new CheckBox(comicBookCard))
    , comicBookEditorSaveItemsFromText(new CheckBox(screenplayCard))
    , comicBookEditorShowHintsForAllItems(new CheckBox(screenplayCard))
    , comicBookEditorShowHintsForPrimaryItems(new CheckBox(screenplayCard))
    , comicBookEditorShowHintsForSecondaryItems(new CheckBox(screenplayCard))
    , comicBookEditorShowHintsForTertiaryItems(new CheckBox(screenplayCard))
    , comicBookEditorShowCharacterSuggestionsInEmptyBlock(new CheckBox(comicBookCard))
    , comicBookNavigatorTitle(new H6Label(comicBookCard))
    , comicBookNavigatorShowSceneText(new CheckBox(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines1(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines2(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines3(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines4(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines5(new RadioButton(comicBookCard))
    //
    , audioplayCard(new Card(content))
    , audioplayCardLayout(new QGridLayout)
    , audioplayTitle(new H5Label(audioplayCard))
    , audioplayAvailable(new Toggle(audioplayCard))
    , audioplayEditorTitle(new H6Label(audioplayCard))
    , audioplayEditorDefaultTemplate(new ComboBox(audioplayCard))
    , audioplayEditorDefaultTemplateOptions(new IconButton(audioplayCard))
    , audioplayEditorShowBlockNumber(new CheckBox(audioplayCard))
    , audioplayEditorContinueBlockNumbers(new CheckBox(audioplayCard))
    , audioplayEditorSaveItemsFromText(new CheckBox(screenplayCard))
    , audioplayEditorShowHintsForAllItems(new CheckBox(screenplayCard))
    , audioplayEditorShowHintsForPrimaryItems(new CheckBox(screenplayCard))
    , audioplayEditorShowHintsForSecondaryItems(new CheckBox(screenplayCard))
    , audioplayEditorShowHintsForTertiaryItems(new CheckBox(screenplayCard))
    , audioplayEditorShowCharacterSuggestionsInEmptyBlock(new CheckBox(audioplayCard))
    , audioplayNavigatorTitle(new H6Label(audioplayCard))
    , audioplayNavigatorShowSceneNumber(new CheckBox(audioplayCard))
    , audioplayNavigatorShowSceneText(new CheckBox(audioplayCard))
    , audioplayNavigatorSceneDescriptionLines1(new RadioButton(audioplayCard))
    , audioplayNavigatorSceneDescriptionLines2(new RadioButton(audioplayCard))
    , audioplayNavigatorSceneDescriptionLines3(new RadioButton(audioplayCard))
    , audioplayNavigatorSceneDescriptionLines4(new RadioButton(audioplayCard))
    , audioplayNavigatorSceneDescriptionLines5(new RadioButton(audioplayCard))
    , audioplayDurationTitle(new H6Label(audioplayCard))
    , audioplayDurationByWordsTitle(new Body1Label(audioplayCard))
    , audioplayDurationByWordsWords(new TextField(audioplayCard))
    , audioplayDurationByWordsDuration(new TextField(audioplayCard))
    //
    , stageplayCard(new Card(content))
    , stageplayCardLayout(new QGridLayout)
    , stageplayTitle(new H5Label(stageplayCard))
    , stageplayAvailable(new Toggle(stageplayCard))
    , stageplayEditorTitle(new H6Label(stageplayCard))
    , stageplayEditorDefaultTemplate(new ComboBox(stageplayCard))
    , stageplayEditorDefaultTemplateOptions(new IconButton(stageplayCard))
    , stageplayEditorSaveItemsFromText(new CheckBox(screenplayCard))
    , stageplayEditorShowHintsForAllItems(new CheckBox(screenplayCard))
    , stageplayEditorShowHintsForPrimaryItems(new CheckBox(screenplayCard))
    , stageplayEditorShowHintsForSecondaryItems(new CheckBox(screenplayCard))
    , stageplayEditorShowHintsForTertiaryItems(new CheckBox(screenplayCard))
    , stageplayEditorShowCharacterSuggestionsInEmptyBlock(new CheckBox(stageplayCard))
    , stageplayNavigatorTitle(new H6Label(stageplayCard))
    , stageplayNavigatorShowSceneNumber(new CheckBox(stageplayCard))
    , stageplayNavigatorShowSceneText(new CheckBox(stageplayCard))
    , stageplayNavigatorSceneDescriptionLines1(new RadioButton(stageplayCard))
    , stageplayNavigatorSceneDescriptionLines2(new RadioButton(stageplayCard))
    , stageplayNavigatorSceneDescriptionLines3(new RadioButton(stageplayCard))
    , stageplayNavigatorSceneDescriptionLines4(new RadioButton(stageplayCard))
    , stageplayNavigatorSceneDescriptionLines5(new RadioButton(stageplayCard))
    //
    , novelCard(new Card(content))
    , novelCardLayout(new QGridLayout)
    , novelTitle(new H5Label(novelCard))
    , novelAvailable(new Toggle(novelCard))
    , novelEditorTitle(new H6Label(novelCard))
    , novelEditorDefaultTemplate(new ComboBox(novelCard))
    , novelEditorDefaultTemplateOptions(new IconButton(novelCard))
    , novelNavigatorTitle(new H6Label(novelCard))
    , novelNavigatorShowSceneText(new CheckBox(novelCard))
    , novelNavigatorSceneDescriptionLines1(new RadioButton(novelCard))
    , novelNavigatorSceneDescriptionLines2(new RadioButton(novelCard))
    , novelNavigatorSceneDescriptionLines3(new RadioButton(novelCard))
    , novelNavigatorSceneDescriptionLines4(new RadioButton(novelCard))
    , novelNavigatorSceneDescriptionLines5(new RadioButton(novelCard))
    , novelNavigatorCounterType(new Subtitle1Label(novelCard))
    , novelNavigatorCounterUseWords(new RadioButton(novelCard))
    , novelNavigatorCounterUseCharacters(new RadioButton(novelCard))
    , novelNavigatorCounterUseCharactersWithSpaces(new RadioButton(novelCard))
    //
    , shortcutsCard(new Card(content))
    , shortcutsCardLayout(new QGridLayout)
    , shortcutsTitle(new H5Label(shortcutsCard))
    //
    , shortcutsForApplicationTitle(new H6Label(shortcutsCard))
    , shortcutsForApplication(new Tree(shortcutsCard))
    //
    , shortcutsForSimpleTextTitle(new H6Label(shortcutsCard))
    , shortcutsForSimpleText(new Tree(shortcutsCard))
    , simpleTextParagraphTypesModel(buildSimpleTextParagraphTypesModel(shortcutsForSimpleText))
    , simpleTextParagraphAddTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForSimpleText, simpleTextParagraphTypesModel))
    , simpleTextParagraphChangeTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForSimpleText, simpleTextParagraphTypesModel))
    //
    , shortcutsForScreenplayTitle(new H6Label(shortcutsCard))
    , shortcutsForScreenplay(new Tree(shortcutsCard))
    , screenplayParagraphTypesModel(buildScreenplayParagraphTypesModel(shortcutsForScreenplay))
    , screenplayParagraphAddTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForScreenplay, screenplayParagraphTypesModel))
    , screenplayParagraphChangeTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForScreenplay, screenplayParagraphTypesModel))
    //
    , shortcutsForComicBookTitle(new H6Label(shortcutsCard))
    , shortcutsForComicBook(new Tree(shortcutsCard))
    , comicBookParagraphTypesModel(buildComicBookParagraphTypesModel(shortcutsForComicBook))
    , comicBookParagraphAddTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForComicBook, comicBookParagraphTypesModel))
    , comicBookParagraphChangeTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForComicBook, comicBookParagraphTypesModel))
    //
    , shortcutsForAudioplayTitle(new H6Label(shortcutsCard))
    , shortcutsForAudioplay(new Tree(shortcutsCard))
    , audioplayParagraphTypesModel(buildAudioplayParagraphTypesModel(shortcutsForAudioplay))
    , audioplayParagraphAddTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForAudioplay, audioplayParagraphTypesModel))
    , audioplayParagraphChangeTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForAudioplay, audioplayParagraphTypesModel))
    //
    , shortcutsForStageplayTitle(new H6Label(shortcutsCard))
    , shortcutsForStageplay(new Tree(shortcutsCard))
    , stageplayParagraphTypesModel(buildStageplayParagraphTypesModel(shortcutsForStageplay))
    , stageplayParagraphAddTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForStageplay, stageplayParagraphTypesModel))
    , stageplayParagraphChangeTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForStageplay, stageplayParagraphTypesModel))
    //
    , shortcutsForNovelTitle(new H6Label(shortcutsCard))
    , shortcutsForNovel(new Tree(shortcutsCard))
    , novelParagraphTypesModel(buildNovelParagraphTypesModel(shortcutsForNovel))
    , novelParagraphAddTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForNovel, novelParagraphTypesModel))
    , novelParagraphChangeTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForNovel, novelParagraphTypesModel))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);
    scrollAnimation.setEasingCurve(QEasingCurve::OutQuad);
    scrollAnimation.setDuration(180);
    colorAnimation.setEasingCurve(QEasingCurve::InBack);
    colorAnimation.setDuration(1400);

    initApplicationCard();
    initSimpleTextCard();
    initScreenplayCard();
    initComicBookCard();
    initAudioplayCard();
    initStageplayCard();
    initNovelCard();
    initShortcutsCard();

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(applicationCard);
    layout->addWidget(componentsTitle);
    layout->addWidget(simpleTextCard);
    layout->addWidget(screenplayCard);
    layout->addWidget(comicBookCard);
    layout->addWidget(audioplayCard);
    layout->addWidget(stageplayCard);
    layout->addWidget(novelCard);
    layout->addWidget(shortcutsCard);
    layout->addStretch();
}

void SettingsView::Implementation::initApplicationCard()
{
    lightTheme->setTheme(ApplicationTheme::Light);
    darkAndLightTheme->setTheme(ApplicationTheme::DarkAndLight);
    darkTheme->setTheme(ApplicationTheme::Dark);
    customTheme->setTheme(ApplicationTheme::Custom);
    // 0 - 0.5, 500 - 1, 3500 - 4
    scaleFactor->setMaximumValue(3500);
    scaleFactor->setValue(500);
    scaleFactor->setDefaultValue(500);
    backupsFolderPath->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    backupsFolderPath->setEnabled(false);
    backupsFolderPath->setTrailingIcon(u8"\U000f0256");
    backupsQty->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    backupsQty->setEnabled(false);
    backupsQty->setSuffix("  ");
    spellCheckerLanguage->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    spellCheckerLanguage->setEnabled(false);
    spellCheckerLanguage->setModel(spellCheckerLanguagesModel);
    spellCheckerUserDictionary->setIcon(u8"\U000F0900");
    spellCheckerUserDictionary->hide();

    //
    // Компоновка
    //
    applicationCardLayout->setContentsMargins({});
    applicationCardLayout->setSpacing(0);
    int itemIndex = 0;
    applicationCardLayout->addWidget(applicationTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(language, 0, Qt::AlignCenter);
        layout->addWidget(changeLanuage);
        layout->addStretch();
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... интерфейс
    //
    applicationCardLayout->addWidget(applicationUserInterfaceTitle, itemIndex++, 0);
    {
        applicationThemesLayout->setContentsMargins({});
        applicationThemesLayout->setSpacing(0);
        applicationThemesLayout->addWidget(lightTheme);
        applicationThemesLayout->addWidget(darkAndLightTheme);
        applicationThemesLayout->addWidget(darkTheme);
        applicationThemesLayout->addWidget(customTheme);
        applicationThemesLayout->addStretch();
        applicationCardLayout->addLayout(applicationThemesLayout, itemIndex++, 0);
    }
    applicationCardLayout->addWidget(scaleFactorTitle, itemIndex++, 0);
    applicationCardLayout->addWidget(scaleFactor, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(scaleFactorSmallInfo);
        layout->addStretch();
        layout->addWidget(scaleFactorBigInfo);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    applicationCardLayout->addWidget(isCompact, itemIndex++, 0);
    //
    // ... сохранение и бэкапы
    //
    applicationCardLayout->addWidget(applicationSaveAndBackupTitle, itemIndex++, 0);
    applicationCardLayout->addWidget(autoSave, itemIndex++, 0);
    {
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        backupsFolderPath->setSizePolicy(policy);

        auto layout = makeLayout();
        layout->addWidget(saveBackups, 0, Qt::AlignCenter);
        layout->addWidget(backupsFolderPath);
        layout->addWidget(backupsQty);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... параметры редакторов текста
    //
    applicationCardLayout->addWidget(applicationTextEditingTitle, itemIndex++, 0);
    {
        spellCheckerLanguage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto layout = makeLayout();
        layout->addWidget(useSpellChecker, 0, Qt::AlignCenter);
        layout->addWidget(spellCheckerLanguage);
        layout->addWidget(spellCheckerUserDictionary);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    applicationCardLayout->addWidget(showDocumentsPages, itemIndex++, 0);
    applicationCardLayout->addWidget(useTypewriterSound, itemIndex++, 0);
    applicationCardLayout->addWidget(useTypewriterScrolling, itemIndex++, 0);
    applicationCardLayout->addWidget(focusCurrentParagraph, itemIndex++, 0);
    applicationCardLayout->addWidget(highlightCurrentLine, itemIndex++, 0);
    applicationCardLayout->addWidget(applicationTextEditingSubtitle, itemIndex++, 0);
    applicationCardLayout->addWidget(correctDoubleCapitals, itemIndex++, 0);
    applicationCardLayout->addWidget(capitalizeSingleILetter, itemIndex++, 0);
    applicationCardLayout->addWidget(replaceThreeDotsWithEllipsis, itemIndex++, 0);
    applicationCardLayout->addWidget(useSmartQuotes, itemIndex++, 0);
    applicationCardLayout->addWidget(replaceTwoDashesWithEmDash, itemIndex++, 0);
    applicationCardLayout->addWidget(avoidMultipleSpaces, itemIndex++, 0);

    applicationCardBottomSpacerIndex = itemIndex;
    applicationCard->setContentLayout(applicationCardLayout);
}

void SettingsView::Implementation::initSimpleTextCard()
{
    simpleTextEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    simpleTextEditorDefaultTemplate->setModel(
        BusinessLayer::TemplatesFacade::simpleTextTemplates());
    simpleTextEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    //
    auto linesGroup = new RadioButtonGroup(simpleTextCard);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines1);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines2);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines3);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines4);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines5);
    simpleTextNavigatorSceneDescriptionLines1->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines1->setChecked(true);
    simpleTextNavigatorSceneDescriptionLines2->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines3->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines4->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines5->setEnabled(false);


    //
    // Компоновка
    //
    simpleTextCardLayout->setContentsMargins({});
    simpleTextCardLayout->setSpacing(0);
    int itemIndex = 0;
    {
        auto layout = makeLayout();
        layout->addWidget(simpleTextTitle);
        layout->addStretch();
        layout->addWidget(simpleTextAvailable, 0, Qt::AlignVCenter);
        simpleTextCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... редактор текста
    //
    simpleTextCardLayout->addWidget(simpleTextEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(simpleTextEditorDefaultTemplate, 1);
        layout->addWidget(simpleTextEditorDefaultTemplateOptions);
        simpleTextCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... навигатор текста
    //
    simpleTextCardLayout->addWidget(simpleTextNavigatorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(simpleTextNavigatorShowSceneText);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines1);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines2);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines3);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines4);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines5);
        layout->addStretch();
        simpleTextCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    simpleTextCardBottomSpacerIndex = itemIndex;
    simpleTextCard->setContentLayout(simpleTextCardLayout);
}

void SettingsView::Implementation::initScreenplayCard()
{
    screenplayEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayEditorDefaultTemplate->setModel(
        BusinessLayer::TemplatesFacade::screenplayTemplates());
    screenplayEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    screenplayEditorShowSceneNumberOnLeft->setEnabled(false);
    screenplayEditorShowSceneNumberOnLeft->setChecked(true);
    screenplayEditorShowSceneNumberOnRight->setEnabled(false);
    //
    screenplayNavigatorShowBeatsInTreatment->setEnabled(false);
    screenplayNavigatorShowBeatsInTreatment->setChecked(true);
    screenplayNavigatorShowBeatsInScreenplay->setEnabled(false);
    screenplayNavigatorShowBeatsInScreenplay->setChecked(false);
    auto linesGroup = new RadioButtonGroup(screenplayCard);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines1);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines2);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines3);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines4);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines5);
    screenplayNavigatorSceneDescriptionLines1->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines1->setChecked(true);
    screenplayNavigatorSceneDescriptionLines2->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines3->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines4->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines5->setEnabled(false);
    //
    auto durationGroup = new RadioButtonGroup(screenplayCard);
    durationGroup->add(screenplayDurationByPage);
    durationGroup->add(screenplayDurationByCharacters);
    durationGroup->add(screenplayDurationConfigurable);
    screenplayDurationByPage->setChecked(true);
    screenplayDurationByPagePage->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByPagePage->setText("1");
    screenplayDurationByPagePage->setReadOnly(true);
    screenplayDurationByPageDuration->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersCharacters->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersCharacters->setVisible(false);
    screenplayDurationByCharactersIncludingSpaces->setVisible(false);
    screenplayDurationByCharactersDuration->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersDuration->setVisible(false);
    screenplayDurationConfigurableForActionLabel->setVisible(false);
    screenplayDurationConfigurablePerParagraphForAction->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerParagraphForAction->setVisible(false);
    screenplayDurationConfigurableForActionPlus->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForAction->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerEvery50ForAction->setVisible(false);
    screenplayDurationConfigurableForDialogueLabel->setVisible(false);
    screenplayDurationConfigurablePerParagraphForDialogue->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerParagraphForDialogue->setVisible(false);
    screenplayDurationConfigurablePerParagraphForDialogue->setLabelVisible(false);
    screenplayDurationConfigurableForDialoguePlus->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForDialogue->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerEvery50ForDialogue->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForDialogue->setLabelVisible(false);
    screenplayDurationConfigurableForSceneHeadingLabel->setVisible(false);
    screenplayDurationConfigurablePerParagraphForSceneHeading->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerParagraphForSceneHeading->setVisible(false);
    screenplayDurationConfigurablePerParagraphForSceneHeading->setLabelVisible(false);
    screenplayDurationConfigurableForSceneHeadingPlus->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForSceneHeading->setSpellCheckPolicy(
        SpellCheckPolicy::Manual);
    screenplayDurationConfigurablePerEvery50ForSceneHeading->setVisible(false);
    screenplayDurationConfigurablePerEvery50ForSceneHeading->setLabelVisible(false);


    //
    // Компоновка
    //
    screenplayCardLayout->setContentsMargins({});
    screenplayCardLayout->setSpacing(0);
    int itemIndex = 0;
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayTitle);
        layout->addStretch();
        layout->addWidget(screenplayAvailable, 0, Qt::AlignVCenter);
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... редактор сценария
    //
    screenplayCardLayout->addWidget(screenplayEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayEditorDefaultTemplate, 1);
        layout->addWidget(screenplayEditorDefaultTemplateOptions);
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayEditorShowSceneNumber);
        layout->addWidget(screenplayEditorShowSceneNumberOnLeft);
        layout->addWidget(screenplayEditorShowSceneNumberOnRight);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    screenplayCardLayout->addWidget(screenplayEditorShowDialogueNumber, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayEditorContinueDialogue, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayEditorCorrectTextOnPageBreaks, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayEditorSaveItemsFromText, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayEditorShowHintsForAllItems);
        layout->addWidget(screenplayEditorShowHintsForPrimaryItems);
        layout->addWidget(screenplayEditorShowHintsForSecondaryItems);
        layout->addWidget(screenplayEditorShowHintsForTertiaryItems);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    screenplayCardLayout->addWidget(screenplayEditorShowCharacterSuggestionsInEmptyBlock,
                                    itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayEditorUseOpenBracketInDialogueForParenthetical,
                                    itemIndex++, 0);
    //
    // ... навигатор сценария
    //
    screenplayCardLayout->addWidget(screenplayNavigatorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayNavigatorShowBeats);
        layout->addWidget(screenplayNavigatorShowBeatsInTreatment);
        layout->addWidget(screenplayNavigatorShowBeatsInScreenplay);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    screenplayCardLayout->addWidget(screenplayNavigatorShowSceneNumber, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayNavigatorShowSceneText);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines1);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines2);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines3);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines4);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines5);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... счётчики хронометража
    //
    screenplayCardLayout->addWidget(screenplayDurationTitle, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayDurationByPage, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayDurationByPagePage);
        layout->addWidget(screenplayDurationByPageDuration);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    screenplayCardLayout->addWidget(screenplayDurationByCharacters, itemIndex++, 0,
                                    Qt::AlignBottom);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayDurationByCharactersCharacters);
        layout->addWidget(screenplayDurationByCharactersIncludingSpaces, 0, Qt::AlignCenter);
        layout->addWidget(screenplayDurationByCharactersDuration);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    screenplayCardLayout->addWidget(screenplayDurationConfigurable, itemIndex++, 0,
                                    Qt::AlignBottom);
    {
        auto layout = new QGridLayout;
        int row = 0;
        int column = 0;
        layout->addWidget(screenplayDurationConfigurableForActionLabel, row, column++,
                          Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(screenplayDurationConfigurablePerParagraphForAction, row, column++);
        layout->addWidget(screenplayDurationConfigurableForActionPlus, row, column++,
                          Qt::AlignCenter);
        layout->addWidget(screenplayDurationConfigurablePerEvery50ForAction, row, column++);
        ++row;
        column = 0;
        layout->addWidget(screenplayDurationConfigurableForDialogueLabel, row, column++,
                          Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(screenplayDurationConfigurablePerParagraphForDialogue, row, column++);
        layout->addWidget(screenplayDurationConfigurableForDialoguePlus, row, column++,
                          Qt::AlignCenter);
        layout->addWidget(screenplayDurationConfigurablePerEvery50ForDialogue, row, column++);
        ++row;
        column = 0;
        layout->addWidget(screenplayDurationConfigurableForSceneHeadingLabel, row, column++,
                          Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(screenplayDurationConfigurablePerParagraphForSceneHeading, row, column++);
        layout->addWidget(screenplayDurationConfigurableForSceneHeadingPlus, row, column++,
                          Qt::AlignCenter);
        layout->addWidget(screenplayDurationConfigurablePerEvery50ForSceneHeading, row, column++);
        layout->setColumnStretch(column, 1);
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    screenplayCardBottomSpacerIndex = itemIndex;
    screenplayCard->setContentLayout(screenplayCardLayout);
}

void SettingsView::Implementation::initComicBookCard()
{
    comicBookEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    comicBookEditorDefaultTemplate->setModel(BusinessLayer::TemplatesFacade::comicBookTemplates());
    comicBookEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    //
    auto linesGroup = new RadioButtonGroup(comicBookCard);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines1);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines2);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines3);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines4);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines5);
    comicBookNavigatorSceneDescriptionLines1->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines1->setChecked(true);
    comicBookNavigatorSceneDescriptionLines2->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines3->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines4->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines5->setEnabled(false);


    //
    // Компоновка
    //
    comicBookCardLayout->setContentsMargins({});
    comicBookCardLayout->setSpacing(0);
    int itemIndex = 0;
    {
        auto layout = makeLayout();
        layout->addWidget(comicBookTitle);
        layout->addStretch();
        layout->addWidget(comicBookAvailable, 0, Qt::AlignVCenter);
        comicBookCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... редактор текста
    //
    comicBookCardLayout->addWidget(comicBookEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(comicBookEditorDefaultTemplate, 1);
        layout->addWidget(comicBookEditorDefaultTemplateOptions);
        comicBookCardLayout->addLayout(layout, itemIndex++, 0);
    }
    comicBookCardLayout->addWidget(comicBookEditorShowDialogueNumber, itemIndex++, 0);
    comicBookCardLayout->addWidget(comicBookEditorSaveItemsFromText, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(comicBookEditorShowHintsForAllItems);
        layout->addWidget(comicBookEditorShowHintsForPrimaryItems);
        layout->addWidget(comicBookEditorShowHintsForSecondaryItems);
        layout->addWidget(comicBookEditorShowHintsForTertiaryItems);
        layout->addStretch();
        comicBookCardLayout->addLayout(layout, itemIndex++, 0);
    }
    comicBookCardLayout->addWidget(comicBookEditorShowCharacterSuggestionsInEmptyBlock, itemIndex++,
                                   0);
    //
    // ... навигатор текста
    //
    comicBookCardLayout->addWidget(comicBookNavigatorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(comicBookNavigatorShowSceneText);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines1);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines2);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines3);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines4);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines5);
        layout->addStretch();
        comicBookCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    comicBookCardBottomSpacerIndex = itemIndex;
    comicBookCard->setContentLayout(comicBookCardLayout);
}

void SettingsView::Implementation::initAudioplayCard()
{
    audioplayEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    audioplayEditorDefaultTemplate->setModel(BusinessLayer::TemplatesFacade::audioplayTemplates());
    audioplayEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    audioplayEditorContinueBlockNumbers->hide();
    //
    auto linesGroup = new RadioButtonGroup(audioplayCard);
    linesGroup->add(audioplayNavigatorSceneDescriptionLines1);
    linesGroup->add(audioplayNavigatorSceneDescriptionLines2);
    linesGroup->add(audioplayNavigatorSceneDescriptionLines3);
    linesGroup->add(audioplayNavigatorSceneDescriptionLines4);
    linesGroup->add(audioplayNavigatorSceneDescriptionLines5);
    audioplayNavigatorSceneDescriptionLines1->setEnabled(false);
    audioplayNavigatorSceneDescriptionLines1->setChecked(true);
    audioplayNavigatorSceneDescriptionLines2->setEnabled(false);
    audioplayNavigatorSceneDescriptionLines3->setEnabled(false);
    audioplayNavigatorSceneDescriptionLines4->setEnabled(false);
    audioplayNavigatorSceneDescriptionLines5->setEnabled(false);
    //
    audioplayDurationByWordsWords->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    audioplayDurationByWordsDuration->setSpellCheckPolicy(SpellCheckPolicy::Manual);


    //
    // Компоновка
    //
    audioplayCardLayout->setContentsMargins({});
    audioplayCardLayout->setSpacing(0);
    int itemIndex = 0;
    {
        auto layout = makeLayout();
        layout->addWidget(audioplayTitle);
        layout->addStretch();
        layout->addWidget(audioplayAvailable, 0, Qt::AlignVCenter);
        audioplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... редактор сценария
    //
    audioplayCardLayout->addWidget(audioplayEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(audioplayEditorDefaultTemplate, 1);
        layout->addWidget(audioplayEditorDefaultTemplateOptions);
        audioplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    {
        auto layout = makeLayout();
        layout->addWidget(audioplayEditorShowBlockNumber);
        layout->addWidget(audioplayEditorContinueBlockNumbers);
        layout->addStretch();
        audioplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    audioplayCardLayout->addWidget(audioplayEditorSaveItemsFromText, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(audioplayEditorShowHintsForAllItems);
        layout->addWidget(audioplayEditorShowHintsForPrimaryItems);
        layout->addWidget(audioplayEditorShowHintsForSecondaryItems);
        layout->addWidget(audioplayEditorShowHintsForTertiaryItems);
        layout->addStretch();
        audioplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    audioplayCardLayout->addWidget(audioplayEditorShowCharacterSuggestionsInEmptyBlock, itemIndex++,
                                   0);
    //
    // ... навигатор сценария
    //
    audioplayCardLayout->addWidget(audioplayNavigatorTitle, itemIndex++, 0);
    audioplayCardLayout->addWidget(audioplayNavigatorShowSceneNumber, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(audioplayNavigatorShowSceneText);
        layout->addWidget(audioplayNavigatorSceneDescriptionLines1);
        layout->addWidget(audioplayNavigatorSceneDescriptionLines2);
        layout->addWidget(audioplayNavigatorSceneDescriptionLines3);
        layout->addWidget(audioplayNavigatorSceneDescriptionLines4);
        layout->addWidget(audioplayNavigatorSceneDescriptionLines5);
        layout->addStretch();
        audioplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... счётчики хронометража
    //
    audioplayCardLayout->addWidget(audioplayDurationTitle, itemIndex++, 0);
    audioplayCardLayout->addWidget(audioplayDurationByWordsTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(audioplayDurationByWordsWords);
        layout->addWidget(audioplayDurationByWordsDuration);
        layout->addStretch();
        audioplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    audioplayCardBottomSpacerIndex = itemIndex;
    audioplayCard->setContentLayout(audioplayCardLayout);
}

void SettingsView::Implementation::initStageplayCard()
{
    stageplayEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    stageplayEditorDefaultTemplate->setModel(BusinessLayer::TemplatesFacade::stageplayTemplates());
    stageplayEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    //
    auto linesGroup = new RadioButtonGroup(stageplayCard);
    linesGroup->add(stageplayNavigatorSceneDescriptionLines1);
    linesGroup->add(stageplayNavigatorSceneDescriptionLines2);
    linesGroup->add(stageplayNavigatorSceneDescriptionLines3);
    linesGroup->add(stageplayNavigatorSceneDescriptionLines4);
    linesGroup->add(stageplayNavigatorSceneDescriptionLines5);
    stageplayNavigatorSceneDescriptionLines1->setEnabled(false);
    stageplayNavigatorSceneDescriptionLines1->setChecked(true);
    stageplayNavigatorSceneDescriptionLines2->setEnabled(false);
    stageplayNavigatorSceneDescriptionLines3->setEnabled(false);
    stageplayNavigatorSceneDescriptionLines4->setEnabled(false);
    stageplayNavigatorSceneDescriptionLines5->setEnabled(false);


    //
    // Компоновка
    //
    stageplayCardLayout->setContentsMargins({});
    stageplayCardLayout->setSpacing(0);
    int itemIndex = 0;
    {
        auto layout = makeLayout();
        layout->addWidget(stageplayTitle);
        layout->addStretch();
        layout->addWidget(stageplayAvailable, 0, Qt::AlignVCenter);
        stageplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... редактор сценария
    //
    stageplayCardLayout->addWidget(stageplayEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(stageplayEditorDefaultTemplate, 1);
        layout->addWidget(stageplayEditorDefaultTemplateOptions);
        stageplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    stageplayCardLayout->addWidget(stageplayEditorSaveItemsFromText, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(stageplayEditorShowHintsForAllItems);
        layout->addWidget(stageplayEditorShowHintsForPrimaryItems);
        layout->addWidget(stageplayEditorShowHintsForSecondaryItems);
        layout->addWidget(stageplayEditorShowHintsForTertiaryItems);
        layout->addStretch();
        stageplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    stageplayCardLayout->addWidget(stageplayEditorShowCharacterSuggestionsInEmptyBlock, itemIndex++,
                                   0);
    //
    // ... навигатор сценария
    //
    stageplayCardLayout->addWidget(stageplayNavigatorTitle, itemIndex++, 0);
    stageplayCardLayout->addWidget(stageplayNavigatorShowSceneNumber, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(stageplayNavigatorShowSceneText);
        layout->addWidget(stageplayNavigatorSceneDescriptionLines1);
        layout->addWidget(stageplayNavigatorSceneDescriptionLines2);
        layout->addWidget(stageplayNavigatorSceneDescriptionLines3);
        layout->addWidget(stageplayNavigatorSceneDescriptionLines4);
        layout->addWidget(stageplayNavigatorSceneDescriptionLines5);
        layout->addStretch();
        stageplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    stageplayCardBottomSpacerIndex = itemIndex;
    stageplayCard->setContentLayout(stageplayCardLayout);
}

void SettingsView::Implementation::initNovelCard()
{
    novelEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    novelEditorDefaultTemplate->setModel(BusinessLayer::TemplatesFacade::novelTemplates());
    novelEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    //
    auto linesGroup = new RadioButtonGroup(novelCard);
    linesGroup->add(novelNavigatorSceneDescriptionLines1);
    linesGroup->add(novelNavigatorSceneDescriptionLines2);
    linesGroup->add(novelNavigatorSceneDescriptionLines3);
    linesGroup->add(novelNavigatorSceneDescriptionLines4);
    linesGroup->add(novelNavigatorSceneDescriptionLines5);
    novelNavigatorSceneDescriptionLines1->setEnabled(false);
    novelNavigatorSceneDescriptionLines1->setChecked(true);
    novelNavigatorSceneDescriptionLines2->setEnabled(false);
    novelNavigatorSceneDescriptionLines3->setEnabled(false);
    novelNavigatorSceneDescriptionLines4->setEnabled(false);
    novelNavigatorSceneDescriptionLines5->setEnabled(false);
    auto typesGroup = new RadioButtonGroup(novelCard);
    typesGroup->add(novelNavigatorCounterUseWords);
    typesGroup->add(novelNavigatorCounterUseCharacters);
    typesGroup->add(novelNavigatorCounterUseCharactersWithSpaces);
    novelNavigatorCounterUseWords->setChecked(true);


    //
    // Компоновка
    //
    novelCardLayout->setContentsMargins({});
    novelCardLayout->setSpacing(0);
    int itemIndex = 0;
    {
        auto layout = makeLayout();
        layout->addWidget(novelTitle);
        layout->addStretch();
        layout->addWidget(novelAvailable, 0, Qt::AlignVCenter);
        novelCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... редактор текста
    //
    novelCardLayout->addWidget(novelEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(novelEditorDefaultTemplate, 1);
        layout->addWidget(novelEditorDefaultTemplateOptions);
        novelCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... навигатор текста
    //
    novelCardLayout->addWidget(novelNavigatorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(novelNavigatorShowSceneText);
        layout->addWidget(novelNavigatorSceneDescriptionLines1);
        layout->addWidget(novelNavigatorSceneDescriptionLines2);
        layout->addWidget(novelNavigatorSceneDescriptionLines3);
        layout->addWidget(novelNavigatorSceneDescriptionLines4);
        layout->addWidget(novelNavigatorSceneDescriptionLines5);
        layout->addStretch();
        novelCardLayout->addLayout(layout, itemIndex++, 0);
    }
    {
        auto layout = makeLayout();
        layout->addWidget(novelNavigatorCounterType);
        layout->addWidget(novelNavigatorCounterUseWords);
        layout->addWidget(novelNavigatorCounterUseCharacters);
        layout->addWidget(novelNavigatorCounterUseCharactersWithSpaces);
        layout->addStretch();
        novelCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    novelCardBottomSpacerIndex = itemIndex;
    novelCard->setContentLayout(novelCardLayout);
}

void SettingsView::Implementation::initShortcutsCard()
{
    shortcutsForApplication->setHeader(new HierarchicalHeaderView(shortcutsForApplication));
    shortcutsForSimpleText->setHeader(new HierarchicalHeaderView(shortcutsForSimpleText));
    shortcutsForScreenplay->setHeader(new HierarchicalHeaderView(shortcutsForScreenplay));
    shortcutsForComicBook->setHeader(new HierarchicalHeaderView(shortcutsForComicBook));
    shortcutsForAudioplay->setHeader(new HierarchicalHeaderView(shortcutsForAudioplay));
    shortcutsForStageplay->setHeader(new HierarchicalHeaderView(shortcutsForStageplay));
    shortcutsForNovel->setHeader(new HierarchicalHeaderView(shortcutsForNovel));

    shortcutsCardLayout->setContentsMargins({});
    shortcutsCardLayout->setSpacing(0);
    int itemIndex = 0;
    shortcutsCardLayout->addWidget(shortcutsTitle, itemIndex++, 0);
    //
    shortcutsCardLayout->addWidget(shortcutsForApplicationTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForApplication, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForSimpleTextTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForSimpleText, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForScreenplayTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForScreenplay, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForComicBookTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForComicBook, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForAudioplayTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForAudioplay, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForStageplayTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForStageplay, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForNovelTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForNovel, itemIndex++, 0);
    //
    shortcutsCardBottomSpacerIndex = itemIndex;
    shortcutsCard->setContentLayout(shortcutsCardLayout);
}

void SettingsView::Implementation::scrollToTitle(AbstractLabel* title)
{
    const QRect microFocus = title->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    const QRect defaultMicroFocus
        = title->QWidget::inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QRect focusRect = (microFocus != defaultMicroFocus)
        ? QRect(title->mapTo(content->widget(), microFocus.topLeft()), microFocus.size())
        : QRect(title->mapTo(content->widget(), QPoint(0, 0)), title->size());

    focusRect.adjust(-50, -50, 50, 50);

    scrollAnimation.setStartValue(content->verticalScrollBar()->value());
    scrollAnimation.setEndValue(focusRect.top());
    scrollAnimation.start();

    colorAnimation.stop();
    if (colorableTitle != nullptr) {
        colorableTitle->setTextColor(colorAnimation.endValue().value<QColor>());
    }
    colorableTitle = title;
    colorAnimation.setEndValue(colorableTitle->textColor());
    colorableTitle->setTextColor(colorAnimation.startValue().value<QColor>());
    colorAnimation.start();
}

void SettingsView::Implementation::updateTablesGeometry()
{
    auto updateTableGeometry = [](Tree* _table, qreal _firstColumnWidthDelta) {
        if (_table->model() != nullptr) {
            _table->setColumnWidth(0, _table->width() * _firstColumnWidthDelta);
            _table->setFixedHeight((_table->model()->rowCount() + 2)
                                   * qCeil(Ui::DesignSystem::treeOneLineItem().height()));
        }
    };
    updateTableGeometry(shortcutsForApplication, 0.25);
    updateTableGeometry(shortcutsForSimpleText, 0.25);
    updateTableGeometry(shortcutsForScreenplay, 0.25);
    updateTableGeometry(shortcutsForComicBook, 0.25);
    updateTableGeometry(shortcutsForAudioplay, 0.25);
    updateTableGeometry(shortcutsForStageplay, 0.25);
    updateTableGeometry(shortcutsForNovel, 0.25);
}


// ****


SettingsView::SettingsView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::FadeThrough);
    showDefaultPage();

    connect(&d->scrollAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                d->content->verticalScrollBar()->setValue(_value.toInt());
            });
    connect(&d->colorAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                if (d->colorableTitle != nullptr) {
                    d->colorableTitle->setTextColor(_value.value<QColor>());
                }
            });
    //
    // Приложение
    //
    connect(d->useSpellChecker, &CheckBox::checkedChanged, d->spellCheckerLanguage,
            &ComboBox::setEnabled);
    connect(d->saveBackups, &CheckBox::checkedChanged, d->backupsFolderPath,
            &TextField::setEnabled);
    connect(d->saveBackups, &CheckBox::checkedChanged, d->backupsQty, &TextField::setEnabled);
    connect(d->backupsFolderPath, &TextField::trailingIconPressed, this, [this] {
        const auto path = QFileDialog::getExistingDirectory(
            this, tr("Choose the folder where backups will be saved"),
            d->backupsFolderPath->text());
        if (!path.isEmpty()) {
            d->backupsFolderPath->setText(path);
        }
    });
    //
    connect(d->changeLanuage, &Button::clicked, this, &SettingsView::applicationLanguagePressed);
    for (auto theme : {
             d->lightTheme,
             d->darkAndLightTheme,
             d->darkTheme,
             d->customTheme,
         }) {
        connect(theme, &ThemePreview::themePressed, this, &SettingsView::applicationThemePressed);
        connect(theme, &ThemePreview::customThemeHashPasted, this,
                &SettingsView::customThemeHashPasted);
    }
    connect(d->scaleFactor, &Slider::valueChanged, this, [this](int _value) {
        emit applicationScaleFactorChanged(0.5 + static_cast<qreal>(_value) / 1000.0);
    });
    connect(d->isCompact, &CheckBox::checkedChanged, this,
            [this](bool _checked) { emit applicationDensityChanged(_checked ? 2 : 0); });
    connect(d->autoSave, &CheckBox::checkedChanged, this,
            &SettingsView::applicationUseAutoSaveChanged);
    connect(d->saveBackups, &CheckBox::checkedChanged, this,
            &SettingsView::applicationSaveBackupsChanged);
    connect(d->backupsFolderPath, &TextField::textChanged, this,
            [this] { emit applicationBackupsFolderChanged(d->backupsFolderPath->text()); });
    connect(d->backupsQty, &TextField::textChanged, this, [this] {
        QSignalBlocker signalBlocker(d->backupsQty);

        bool isValidInt = false;
        auto backupsQty = d->backupsQty->text().toInt(&isValidInt);
        if (isValidInt) {
            if (backupsQty <= 2) {
                backupsQty = 2;
            } else if (backupsQty > 50) {
                backupsQty = 50;
            }
            setApplicationBackupsQty(backupsQty);
            emit applicationBackupsQtyChanged(backupsQty);
        } else {
            d->backupsQty->undo();
        }
    });
    connect(d->showDocumentsPages, &CheckBox::checkedChanged, this,
            &SettingsView::applicationShowDocumentsPagesChanged);
    connect(d->useTypewriterSound, &CheckBox::checkedChanged, this,
            &SettingsView::applicationUseTypewriterSoundChanged);
    connect(d->useSpellChecker, &CheckBox::checkedChanged, this, [this](bool _checked) {
        emit applicationUseSpellCheckerChanged(_checked);
        if (_checked) {
            emit applicationSpellCheckerLanguageChanged(d->spellCheckerLanguage->currentIndex()
                                                            .data(kSpellCheckerLanguageCodeRole)
                                                            .toString());
        }
    });
    connect(d->spellCheckerLanguage, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit applicationSpellCheckerLanguageChanged(
                    _index.data(kSpellCheckerLanguageCodeRole).toString());
            });
    connect(d->highlightCurrentLine, &CheckBox::checkedChanged, this,
            &SettingsView::applicationHighlightCurentLineChanged);
    connect(d->focusCurrentParagraph, &CheckBox::checkedChanged, this,
            &SettingsView::applicationFocusCurrentParagraphChanged);
    connect(d->useTypewriterScrolling, &CheckBox::checkedChanged, this,
            &SettingsView::applicationUseTypewriterScrollingChanged);
    connect(d->correctDoubleCapitals, &CheckBox::checkedChanged, this,
            &SettingsView::applicationCorrectDoubleCapitalsChanged);
    connect(d->capitalizeSingleILetter, &CheckBox::checkedChanged, this,
            &SettingsView::applicationCapitalizeSingleILetterChanged);
    connect(d->replaceThreeDotsWithEllipsis, &CheckBox::checkedChanged, this,
            &SettingsView::applicationReplaceThreeDotsWithEllipsisChanged);
    connect(d->useSmartQuotes, &CheckBox::checkedChanged, this,
            &SettingsView::applicationUseSmartQuotesChanged);
    connect(d->replaceTwoDashesWithEmDash, &CheckBox::checkedChanged, this,
            &SettingsView::applicationReplaceTwoDashedWithEmDashChanged);
    connect(d->avoidMultipleSpaces, &CheckBox::checkedChanged, this,
            &SettingsView::applicationAvoidMultipleSpacesChanged);

    //
    // Компоненты
    //
    // ... Редактор текста
    //
    connect(d->simpleTextAvailable, &Toggle::checkedChanged, this, [this](bool _available) {
        for (auto widget : std::vector<QWidget*>{
                 d->simpleTextEditorTitle,
                 d->simpleTextEditorDefaultTemplate,
                 d->simpleTextEditorDefaultTemplateOptions,
                 d->simpleTextNavigatorTitle,
                 d->simpleTextNavigatorShowSceneText,
                 d->simpleTextNavigatorSceneDescriptionLines1,
                 d->simpleTextNavigatorSceneDescriptionLines2,
                 d->simpleTextNavigatorSceneDescriptionLines3,
                 d->simpleTextNavigatorSceneDescriptionLines4,
                 d->simpleTextNavigatorSceneDescriptionLines5,
                 d->shortcutsForSimpleTextTitle,
                 d->shortcutsForSimpleText,
             }) {
            widget->setVisible(_available);
        }
        d->simpleTextCardLayout->setRowMinimumHeight(
            d->simpleTextCardBottomSpacerIndex,
            _available ? Ui::DesignSystem::layout().px24() : Ui::DesignSystem::layout().px12());
        d->updateTablesGeometry();

        emit simpleTextAvailableChanged(_available);
    });
    connect(d->simpleTextEditorDefaultTemplateOptions, &IconButton::clicked, this, [this] {
        QVector<QAction*> actions;
        const auto templateIndex = d->simpleTextEditorDefaultTemplate->currentIndex();
        const auto templateId
            = templateIndex.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
        const auto isDefaultTemplate
            = BusinessLayer::TemplatesFacade::simpleTextTemplate(templateId).isDefault();
        //
        if (!isDefaultTemplate) {
            auto editAction = new QAction(tr("Edit"), d->contextMenu);
            connect(editAction, &QAction::triggered, this, [this, templateId] {
                emit editCurrentSimpleTextEditorTemplateRequested(templateId);
            });
            actions.append(editAction);
        }
        //
        auto duplicateAction
            = new QAction(tr("Create new template based on current"), d->contextMenu);
        connect(duplicateAction, &QAction::triggered, this, [this, templateId] {
            emit duplicateCurrentSimpleTextEditorTemplateRequested(templateId);
        });
        actions.append(duplicateAction);
        //
        if (!isDefaultTemplate) {
            auto saveToFileAction = new QAction(tr("Save to file"), d->contextMenu);
            connect(saveToFileAction, &QAction::triggered, this, [this, templateId] {
                emit saveToFileCurrentSimpleTextEditorTemplateRequested(templateId);
            });
            actions.append(saveToFileAction);
            //
            auto removeAction = new QAction(tr("Remove"), d->contextMenu);
            connect(removeAction, &QAction::triggered, this, [this, templateId] {
                emit removeCurrentSimpleTextEditorTemplateRequested(templateId);
            });
            actions.append(removeAction);
        }
        //
        auto loadFromFileAction = new QAction(tr("Load template from file"), d->contextMenu);
        loadFromFileAction->setSeparator(true);
        connect(loadFromFileAction, &QAction::triggered, this,
                &SettingsView::loadFromFileSimpleTextEditorTemplateRequested);
        actions.append(loadFromFileAction);
        //
        d->contextMenu->setActions(actions);
        d->contextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->simpleTextEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit simpleTextEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    //
    // ... навигатор текста
    //
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    auto notifysimpleTextNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->simpleTextNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->simpleTextNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->simpleTextNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->simpleTextNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit simpleTextNavigatorShowSceneTextChanged(
            d->simpleTextNavigatorShowSceneText->isChecked(), sceneTextLines);
    };
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    //
    // ... Редактор сценария
    //
    connect(d->screenplayAvailable, &Toggle::checkedChanged, this, [this](bool _available) {
        for (auto widget : std::vector<QWidget*>{
                 d->screenplayEditorTitle,
                 d->screenplayEditorDefaultTemplate,
                 d->screenplayEditorDefaultTemplateOptions,
                 d->screenplayEditorShowSceneNumber,
                 d->screenplayEditorShowSceneNumberOnLeft,
                 d->screenplayEditorShowSceneNumberOnRight,
                 d->screenplayEditorShowDialogueNumber,
                 d->screenplayEditorContinueDialogue,
                 d->screenplayEditorCorrectTextOnPageBreaks,
                 d->screenplayEditorSaveItemsFromText,
                 d->screenplayEditorShowHintsForAllItems,
                 d->screenplayEditorShowHintsForPrimaryItems,
                 d->screenplayEditorShowHintsForSecondaryItems,
                 d->screenplayEditorShowHintsForTertiaryItems,
                 d->screenplayEditorShowCharacterSuggestionsInEmptyBlock,
                 d->screenplayEditorUseOpenBracketInDialogueForParenthetical,
                 d->screenplayNavigatorTitle,
                 d->screenplayNavigatorShowBeats,
                 d->screenplayNavigatorShowBeatsInTreatment,
                 d->screenplayNavigatorShowBeatsInScreenplay,
                 d->screenplayNavigatorShowSceneNumber,
                 d->screenplayNavigatorShowSceneText,
                 d->screenplayNavigatorSceneDescriptionLines1,
                 d->screenplayNavigatorSceneDescriptionLines2,
                 d->screenplayNavigatorSceneDescriptionLines3,
                 d->screenplayNavigatorSceneDescriptionLines4,
                 d->screenplayNavigatorSceneDescriptionLines5,
                 d->screenplayDurationTitle,
                 d->screenplayDurationByPage,
                 d->screenplayDurationByPagePage,
                 d->screenplayDurationByPageDuration,
                 d->screenplayDurationByCharacters,
                 d->screenplayDurationByCharactersCharacters,
                 d->screenplayDurationByCharactersIncludingSpaces,
                 d->screenplayDurationByCharactersDuration,
                 d->screenplayDurationConfigurable,
                 d->screenplayDurationConfigurableForActionLabel,
                 d->screenplayDurationConfigurablePerParagraphForAction,
                 d->screenplayDurationConfigurableForActionPlus,
                 d->screenplayDurationConfigurablePerEvery50ForAction,
                 d->screenplayDurationConfigurableForDialogueLabel,
                 d->screenplayDurationConfigurablePerParagraphForDialogue,
                 d->screenplayDurationConfigurableForDialoguePlus,
                 d->screenplayDurationConfigurablePerEvery50ForDialogue,
                 d->screenplayDurationConfigurableForSceneHeadingLabel,
                 d->screenplayDurationConfigurablePerParagraphForSceneHeading,
                 d->screenplayDurationConfigurableForSceneHeadingPlus,
                 d->screenplayDurationConfigurablePerEvery50ForSceneHeading,
                 d->shortcutsForScreenplayTitle,
                 d->shortcutsForScreenplay,
             }) {
            widget->setVisible(_available);
        }
        d->screenplayCardLayout->setRowMinimumHeight(
            d->screenplayCardBottomSpacerIndex,
            _available ? Ui::DesignSystem::layout().px24() : Ui::DesignSystem::layout().px12());
        const auto screenplayDurationByCharactersRow
            = d->screenplayCardLayout->indexOf(d->screenplayDurationByCharacters);
        d->screenplayCardLayout->setRowMinimumHeight(screenplayDurationByCharactersRow,
                                                     _available ? Ui::DesignSystem::layout().px62()
                                                                : 0.0);
        const auto screenplayDurationConfigurableRow
            = d->screenplayCardLayout->indexOf(d->screenplayDurationConfigurable);
        d->screenplayCardLayout->setRowMinimumHeight(screenplayDurationConfigurableRow,
                                                     _available ? Ui::DesignSystem::layout().px62()
                                                                : 0.0);
        d->updateTablesGeometry();

        emit screenplayAvailableChanged(_available);
    });
    connect(d->screenplayEditorDefaultTemplateOptions, &IconButton::clicked, this, [this] {
        QVector<QAction*> actions;
        const auto templateIndex = d->screenplayEditorDefaultTemplate->currentIndex();
        const auto templateId
            = templateIndex.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
        const auto isDefaultTemplate
            = BusinessLayer::TemplatesFacade::screenplayTemplate(templateId).isDefault();
        //
        if (!isDefaultTemplate) {
            auto editAction = new QAction(tr("Edit"), d->contextMenu);
            connect(editAction, &QAction::triggered, this, [this, templateId] {
                emit editCurrentScreenplayEditorTemplateRequested(templateId);
            });
            actions.append(editAction);
        }
        //
        auto duplicateAction
            = new QAction(tr("Create new template based on current"), d->contextMenu);
        connect(duplicateAction, &QAction::triggered, this, [this, templateId] {
            emit duplicateCurrentScreenplayEditorTemplateRequested(templateId);
        });
        actions.append(duplicateAction);
        //
        if (!isDefaultTemplate) {
            auto saveToFileAction = new QAction(tr("Save to file"), d->contextMenu);
            connect(saveToFileAction, &QAction::triggered, this, [this, templateId] {
                emit saveToFileCurrentScreenplayEditorTemplateRequested(templateId);
            });
            actions.append(saveToFileAction);
            //
            auto removeAction = new QAction(tr("Remove"), d->contextMenu);
            connect(removeAction, &QAction::triggered, this, [this, templateId] {
                emit removeCurrentScreenplayEditorTemplateRequested(templateId);
            });
            actions.append(removeAction);
        }
        //
        auto loadFromFileAction = new QAction(tr("Load template from file"), d->contextMenu);
        loadFromFileAction->setSeparator(true);
        connect(loadFromFileAction, &QAction::triggered, this,
                &SettingsView::loadFromFileScreenplayEditorTemplateRequested);
        actions.append(loadFromFileAction);
        //
        d->contextMenu->setActions(actions);
        d->contextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->screenplayEditorShowSceneNumber, &CheckBox::checkedChanged,
            d->screenplayEditorShowSceneNumberOnLeft, &CheckBox::setEnabled);
    connect(d->screenplayEditorShowSceneNumber, &CheckBox::checkedChanged,
            d->screenplayEditorShowSceneNumberOnRight, &CheckBox::setEnabled);
    auto screenplayEditorCorrectShownSceneNumber = [this] {
        if (!d->screenplayEditorShowSceneNumberOnLeft->isChecked()
            && !d->screenplayEditorShowSceneNumberOnRight->isChecked()) {
            d->screenplayEditorShowSceneNumberOnLeft->setChecked(true);
        }
    };
    connect(d->screenplayEditorShowSceneNumberOnLeft, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShownSceneNumber);
    connect(d->screenplayEditorShowSceneNumberOnRight, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShownSceneNumber);
    connect(d->screenplayEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            [this](bool _checked) {
                if (_checked) {
                    d->screenplayEditorShowHintsForPrimaryItems->setChecked(false);
                    d->screenplayEditorShowHintsForSecondaryItems->setChecked(false);
                    d->screenplayEditorShowHintsForTertiaryItems->setChecked(false);
                }
            });
    auto screenplayEditorCorrectShowHintsForAllItems = [this](bool _checked) {
        if (_checked) {
            d->screenplayEditorShowHintsForAllItems->setChecked(false);
        }
    };
    connect(d->screenplayEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShowHintsForAllItems);
    connect(d->screenplayEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShowHintsForAllItems);
    connect(d->screenplayEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShowHintsForAllItems);
    //
    connect(d->screenplayEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit screenplayEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    auto notifyScreenplayEditorShowSceneNumbersChanged = [this] {
        emit screenplayEditorShowSceneNumberChanged(
            d->screenplayEditorShowSceneNumber->isChecked(),
            d->screenplayEditorShowSceneNumberOnLeft->isChecked(),
            d->screenplayEditorShowSceneNumberOnRight->isChecked());
    };
    connect(d->screenplayEditorShowSceneNumber, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowSceneNumbersChanged);
    connect(d->screenplayEditorShowSceneNumberOnLeft, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowSceneNumbersChanged);
    connect(d->screenplayEditorShowSceneNumberOnRight, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowSceneNumbersChanged);
    connect(d->screenplayEditorShowDialogueNumber, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayEditorShowDialogueNumberChanged);
    connect(d->screenplayEditorContinueDialogue, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayEditorContinueDialogueChanged);
    connect(d->screenplayEditorCorrectTextOnPageBreaks, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayEditorCorrectTextOnPageBreaksChanged);
    connect(d->screenplayEditorSaveItemsFromText, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayEditorSaveItemsFromTextChanged);
    auto notifyScreenplayEditorShowHintsChanged = [this] {
        emit screenplayEditorShowHintsChanged(
            d->screenplayEditorShowHintsForAllItems->isChecked(),
            d->screenplayEditorShowHintsForPrimaryItems->isChecked(),
            d->screenplayEditorShowHintsForSecondaryItems->isChecked(),
            d->screenplayEditorShowHintsForTertiaryItems->isChecked());
    };
    connect(d->screenplayEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowHintsChanged);
    connect(d->screenplayEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowHintsChanged);
    connect(d->screenplayEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowHintsChanged);
    connect(d->screenplayEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowHintsChanged);
    connect(d->screenplayEditorShowCharacterSuggestionsInEmptyBlock, &CheckBox::checkedChanged,
            this, &SettingsView::screenplayEditorShowCharacterSuggestionsInEmptyBlockChanged);
    connect(d->screenplayEditorUseOpenBracketInDialogueForParenthetical, &CheckBox::checkedChanged,
            this, &SettingsView::screenplayEditorUseOpenBracketInDialogueForParentheticalChanged);
    //
    // ... навигатор сценария
    //
    connect(d->screenplayNavigatorShowBeats, &CheckBox::checkedChanged,
            d->screenplayNavigatorShowBeatsInTreatment, &CheckBox::setEnabled);
    connect(d->screenplayNavigatorShowBeats, &CheckBox::checkedChanged,
            d->screenplayNavigatorShowBeatsInScreenplay, &CheckBox::setEnabled);
    auto screenplayNavigatorCorrectShownBeats = [this] {
        if (!d->screenplayNavigatorShowBeatsInTreatment->isChecked()
            && !d->screenplayNavigatorShowBeatsInScreenplay->isChecked()) {
            d->screenplayNavigatorShowBeatsInTreatment->setChecked(true);
        }
    };
    connect(d->screenplayNavigatorShowBeatsInTreatment, &CheckBox::checkedChanged, this,
            screenplayNavigatorCorrectShownBeats);
    connect(d->screenplayNavigatorShowBeatsInScreenplay, &CheckBox::checkedChanged, this,
            screenplayNavigatorCorrectShownBeats);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    auto notifyScreenplayNavigatorShowBeatsChanged = [this] {
        emit screenplayNavigatorShowBeatsChanged(
            d->screenplayNavigatorShowBeats->isChecked(),
            d->screenplayNavigatorShowBeatsInTreatment->isChecked(),
            d->screenplayNavigatorShowBeatsInScreenplay->isChecked());
    };
    connect(d->screenplayNavigatorShowBeats, &CheckBox::checkedChanged, this,
            notifyScreenplayNavigatorShowBeatsChanged);
    connect(d->screenplayNavigatorShowBeatsInTreatment, &CheckBox::checkedChanged, this,
            notifyScreenplayNavigatorShowBeatsChanged);
    connect(d->screenplayNavigatorShowBeatsInScreenplay, &CheckBox::checkedChanged, this,
            notifyScreenplayNavigatorShowBeatsChanged);
    connect(d->screenplayNavigatorShowSceneNumber, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayNavigatorShowSceneNumberChanged);
    auto notifyScreenplayNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->screenplayNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->screenplayNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->screenplayNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->screenplayNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit screenplayNavigatorShowSceneTextChanged(
            d->screenplayNavigatorShowSceneText->isChecked(), sceneTextLines);
    };
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    //
    // ... хронометраж
    //
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged, this, [this](bool _checked) {
        d->screenplayDurationByPagePage->setVisible(d->screenplayAvailable->isChecked()
                                                    && _checked);
        d->screenplayDurationByPageDuration->setVisible(d->screenplayAvailable->isChecked()
                                                        && _checked);
    });
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged, this,
            [this](bool _checked) {
                d->screenplayDurationByCharactersCharacters->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationByCharactersIncludingSpaces->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationByCharactersDuration->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
            });
    connect(d->screenplayDurationConfigurable, &RadioButton::checkedChanged, this,
            [this](bool _checked) {
                d->screenplayDurationConfigurableForActionLabel->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForAction->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurableForActionPlus->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForAction->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurableForDialogueLabel->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForDialogue->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurableForDialoguePlus->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForDialogue->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurableForSceneHeadingLabel->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurablePerParagraphForSceneHeading->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurableForSceneHeadingPlus->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
                d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setVisible(
                    d->screenplayAvailable->isChecked() && _checked);
            });
    //
    auto notifyScreenplayDurationTypeChanged = [this] {
        using namespace BusinessLayer;
        if (d->screenplayDurationByPage->isChecked()) {
            emit screenplayDurationTypeChanged(static_cast<int>(ChronometerType::Page));
        } else if (d->screenplayDurationByCharacters->isChecked()) {
            emit screenplayDurationTypeChanged(static_cast<int>(ChronometerType::Characters));
        } else if (d->screenplayDurationConfigurable->isChecked()) {
            emit screenplayDurationTypeChanged(static_cast<int>(ChronometerType::Configurable));
        }
    };
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged, this,
            notifyScreenplayDurationTypeChanged);
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged, this,
            notifyScreenplayDurationTypeChanged);
    connect(d->screenplayDurationConfigurable, &RadioButton::checkedChanged, this,
            notifyScreenplayDurationTypeChanged);
    connect(d->screenplayDurationByPageDuration, &TextField::textChanged, this, [this] {
        emit screenplayDurationByPageDurationChanged(
            d->screenplayDurationByPageDuration->text().toInt());
    });
    connect(d->screenplayDurationByCharactersCharacters, &TextField::textChanged, this, [this] {
        emit screenplayDurationByCharactersCharactersChanged(
            d->screenplayDurationByCharactersCharacters->text().toInt());
    });
    connect(d->screenplayDurationByCharactersIncludingSpaces, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayDurationByCharactersIncludeSpacesChanged);
    connect(d->screenplayDurationByCharactersDuration, &TextField::textChanged, this, [this] {
        emit screenplayDurationByCharactersDurationChanged(
            d->screenplayDurationByCharactersDuration->text().toInt());
    });
    connect(d->screenplayDurationConfigurablePerParagraphForAction, &TextField::textChanged, this,
            [this] {
                emit screenplayDurationConfigurablePerParagraphForActionChanged(
                    d->screenplayDurationConfigurablePerParagraphForAction->text().toDouble());
            });
    connect(d->screenplayDurationConfigurablePerEvery50ForAction, &TextField::textChanged, this,
            [this] {
                emit screenplayDurationConfigurablePerEvery50ForActionChanged(
                    d->screenplayDurationConfigurablePerEvery50ForAction->text().toDouble());
            });
    connect(d->screenplayDurationConfigurablePerParagraphForDialogue, &TextField::textChanged, this,
            [this] {
                emit screenplayDurationConfigurablePerParagraphForDialogueChanged(
                    d->screenplayDurationConfigurablePerParagraphForDialogue->text().toDouble());
            });
    connect(d->screenplayDurationConfigurablePerEvery50ForDialogue, &TextField::textChanged, this,
            [this] {
                emit screenplayDurationConfigurablePerEvery50ForDialogueChanged(
                    d->screenplayDurationConfigurablePerEvery50ForDialogue->text().toDouble());
            });
    connect(
        d->screenplayDurationConfigurablePerParagraphForSceneHeading, &TextField::textChanged, this,
        [this] {
            emit screenplayDurationConfigurablePerParagraphForSceneHeadingChanged(
                d->screenplayDurationConfigurablePerParagraphForSceneHeading->text().toDouble());
        });
    connect(d->screenplayDurationConfigurablePerEvery50ForSceneHeading, &TextField::textChanged,
            this, [this] {
                emit screenplayDurationConfigurablePerEvery50ForSceneHeadingChanged(
                    d->screenplayDurationConfigurablePerEvery50ForSceneHeading->text().toDouble());
            });
    //
    // ... Редактор комикса
    //
    connect(d->comicBookAvailable, &Toggle::checkedChanged, this, [this](bool _available) {
        for (auto widget : std::vector<QWidget*>{
                 d->comicBookEditorTitle,
                 d->comicBookEditorDefaultTemplate,
                 d->comicBookEditorDefaultTemplateOptions,
                 d->comicBookEditorShowDialogueNumber,
                 d->comicBookEditorSaveItemsFromText,
                 d->comicBookEditorShowHintsForAllItems,
                 d->comicBookEditorShowHintsForPrimaryItems,
                 d->comicBookEditorShowHintsForSecondaryItems,
                 d->comicBookEditorShowHintsForTertiaryItems,
                 d->comicBookEditorShowCharacterSuggestionsInEmptyBlock,
                 d->comicBookNavigatorTitle,
                 d->comicBookNavigatorShowSceneText,
                 d->comicBookNavigatorSceneDescriptionLines1,
                 d->comicBookNavigatorSceneDescriptionLines2,
                 d->comicBookNavigatorSceneDescriptionLines3,
                 d->comicBookNavigatorSceneDescriptionLines4,
                 d->comicBookNavigatorSceneDescriptionLines5,
                 d->shortcutsForComicBookTitle,
                 d->shortcutsForComicBook,
             }) {
            widget->setVisible(_available);
        }
        d->comicBookCardLayout->setRowMinimumHeight(d->comicBookCardBottomSpacerIndex,
                                                    _available ? Ui::DesignSystem::layout().px24()
                                                               : Ui::DesignSystem::layout().px12());
        d->updateTablesGeometry();

        emit comicBookAvailableChanged(_available);
    });
    connect(d->comicBookEditorDefaultTemplateOptions, &IconButton::clicked, this, [this] {
        QVector<QAction*> actions;
        const auto templateIndex = d->comicBookEditorDefaultTemplate->currentIndex();
        const auto templateId
            = templateIndex.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
        const auto isDefaultTemplate
            = BusinessLayer::TemplatesFacade::comicBookTemplate(templateId).isDefault();
        //
        if (!isDefaultTemplate) {
            auto editAction = new QAction(tr("Edit"), d->contextMenu);
            connect(editAction, &QAction::triggered, this, [this, templateId] {
                emit editCurrentComicBookEditorTemplateRequested(templateId);
            });
            actions.append(editAction);
        }
        //
        auto duplicateAction
            = new QAction(tr("Create new template based on current"), d->contextMenu);
        connect(duplicateAction, &QAction::triggered, this, [this, templateId] {
            emit duplicateCurrentComicBookEditorTemplateRequested(templateId);
        });
        actions.append(duplicateAction);
        //
        if (!isDefaultTemplate) {
            auto saveToFileAction = new QAction(tr("Save to file"), d->contextMenu);
            connect(saveToFileAction, &QAction::triggered, this, [this, templateId] {
                emit saveToFileCurrentComicBookEditorTemplateRequested(templateId);
            });
            actions.append(saveToFileAction);
            //
            auto removeAction = new QAction(tr("Remove"), d->contextMenu);
            connect(removeAction, &QAction::triggered, this, [this, templateId] {
                emit removeCurrentComicBookEditorTemplateRequested(templateId);
            });
            actions.append(removeAction);
        }
        //
        auto loadFromFileAction = new QAction(tr("Load template from file"), d->contextMenu);
        loadFromFileAction->setSeparator(true);
        connect(loadFromFileAction, &QAction::triggered, this,
                &SettingsView::loadFromFileComicBookEditorTemplateRequested);
        actions.append(loadFromFileAction);
        //
        d->contextMenu->setActions(actions);
        d->contextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->comicBookEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            [this](bool _checked) {
                if (_checked) {
                    d->comicBookEditorShowHintsForPrimaryItems->setChecked(false);
                    d->comicBookEditorShowHintsForSecondaryItems->setChecked(false);
                    d->comicBookEditorShowHintsForTertiaryItems->setChecked(false);
                }
            });
    auto comicBookEditorCorrectShowHintsForAllItems = [this](bool _checked) {
        if (_checked) {
            d->comicBookEditorShowHintsForAllItems->setChecked(false);
        }
    };
    connect(d->comicBookEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            comicBookEditorCorrectShowHintsForAllItems);
    connect(d->comicBookEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            comicBookEditorCorrectShowHintsForAllItems);
    connect(d->comicBookEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            comicBookEditorCorrectShowHintsForAllItems);
    //
    connect(d->comicBookEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit comicBookEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    connect(d->comicBookEditorShowDialogueNumber, &CheckBox::checkedChanged, this,
            &SettingsView::comicBookEditorShowDialogueNumberChanged);
    connect(d->comicBookEditorSaveItemsFromText, &CheckBox::checkedChanged, this,
            &SettingsView::comicBookEditorSaveItemsFromTextChanged);
    auto notifyComicBookEditorShowHintsChanged = [this] {
        emit comicBookEditorShowHintsChanged(
            d->comicBookEditorShowHintsForAllItems->isChecked(),
            d->comicBookEditorShowHintsForPrimaryItems->isChecked(),
            d->comicBookEditorShowHintsForSecondaryItems->isChecked(),
            d->comicBookEditorShowHintsForTertiaryItems->isChecked());
    };
    connect(d->comicBookEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            notifyComicBookEditorShowHintsChanged);
    connect(d->comicBookEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            notifyComicBookEditorShowHintsChanged);
    connect(d->comicBookEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            notifyComicBookEditorShowHintsChanged);
    connect(d->comicBookEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            notifyComicBookEditorShowHintsChanged);
    connect(d->comicBookEditorShowCharacterSuggestionsInEmptyBlock, &CheckBox::checkedChanged, this,
            &SettingsView::comicBookEditorShowCharacterSuggestionsInEmptyBlockChanged);
    //
    // ... навигатор текста
    //
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    auto notifycomicBookNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->comicBookNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->comicBookNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->comicBookNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->comicBookNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit comicBookNavigatorShowSceneTextChanged(d->comicBookNavigatorShowSceneText->isChecked(),
                                                    sceneTextLines);
    };
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    //
    // ... Редактор аудиопостановки
    //
    connect(d->audioplayAvailable, &Toggle::checkedChanged, this, [this](bool _available) {
        for (auto widget : std::vector<QWidget*>{
                 d->audioplayEditorTitle,
                 d->audioplayEditorDefaultTemplate,
                 d->audioplayEditorDefaultTemplateOptions,
                 d->audioplayEditorShowBlockNumber,
                 d->audioplayEditorContinueBlockNumbers,
                 d->audioplayEditorSaveItemsFromText,
                 d->audioplayEditorShowHintsForAllItems,
                 d->audioplayEditorShowHintsForPrimaryItems,
                 d->audioplayEditorShowHintsForSecondaryItems,
                 d->audioplayEditorShowHintsForTertiaryItems,
                 d->audioplayEditorShowCharacterSuggestionsInEmptyBlock,
                 d->audioplayNavigatorTitle,
                 d->audioplayNavigatorShowSceneNumber,
                 d->audioplayNavigatorShowSceneText,
                 d->audioplayNavigatorSceneDescriptionLines1,
                 d->audioplayNavigatorSceneDescriptionLines2,
                 d->audioplayNavigatorSceneDescriptionLines3,
                 d->audioplayNavigatorSceneDescriptionLines4,
                 d->audioplayNavigatorSceneDescriptionLines5,
                 d->audioplayDurationTitle,
                 d->audioplayDurationByWordsTitle,
                 d->audioplayDurationByWordsWords,
                 d->audioplayDurationByWordsDuration,
                 d->shortcutsForAudioplayTitle,
                 d->shortcutsForAudioplay,
             }) {
            widget->setVisible(_available);
        }
        d->audioplayCardLayout->setRowMinimumHeight(d->audioplayCardBottomSpacerIndex,
                                                    _available ? Ui::DesignSystem::layout().px24()
                                                               : Ui::DesignSystem::layout().px12());
        d->updateTablesGeometry();

        emit audioplayAvailableChanged(_available);
    });
    connect(d->audioplayEditorDefaultTemplateOptions, &IconButton::clicked, this, [this] {
        QVector<QAction*> actions;
        const auto templateIndex = d->audioplayEditorDefaultTemplate->currentIndex();
        const auto templateId
            = templateIndex.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
        const auto isDefaultTemplate
            = BusinessLayer::TemplatesFacade::audioplayTemplate(templateId).isDefault();
        //
        if (!isDefaultTemplate) {
            auto editAction = new QAction(tr("Edit"), d->contextMenu);
            connect(editAction, &QAction::triggered, this, [this, templateId] {
                emit editCurrentAudioplayEditorTemplateRequested(templateId);
            });
            actions.append(editAction);
        }
        //
        auto duplicateAction
            = new QAction(tr("Create new template based on current"), d->contextMenu);
        connect(duplicateAction, &QAction::triggered, this, [this, templateId] {
            emit duplicateCurrentAudioplayEditorTemplateRequested(templateId);
        });
        actions.append(duplicateAction);
        //
        if (!isDefaultTemplate) {
            auto saveToFileAction = new QAction(tr("Save to file"), d->contextMenu);
            connect(saveToFileAction, &QAction::triggered, this, [this, templateId] {
                emit saveToFileCurrentAudioplayEditorTemplateRequested(templateId);
            });
            actions.append(saveToFileAction);
            //
            auto removeAction = new QAction(tr("Remove"), d->contextMenu);
            connect(removeAction, &QAction::triggered, this, [this, templateId] {
                emit removeCurrentAudioplayEditorTemplateRequested(templateId);
            });
            actions.append(removeAction);
        }
        //
        auto loadFromFileAction = new QAction(tr("Load template from file"), d->contextMenu);
        loadFromFileAction->setSeparator(true);
        connect(loadFromFileAction, &QAction::triggered, this,
                &SettingsView::loadFromFileAudioplayEditorTemplateRequested);
        actions.append(loadFromFileAction);
        //
        d->contextMenu->setActions(actions);
        d->contextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->audioplayEditorShowBlockNumber, &CheckBox::checkedChanged,
            d->audioplayEditorContinueBlockNumbers, &CheckBox::setEnabled);
    connect(d->audioplayEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            [this](bool _checked) {
                if (_checked) {
                    d->audioplayEditorShowHintsForPrimaryItems->setChecked(false);
                    d->audioplayEditorShowHintsForSecondaryItems->setChecked(false);
                    d->audioplayEditorShowHintsForTertiaryItems->setChecked(false);
                }
            });
    auto audioplayEditorCorrectShowHintsForAllItems = [this](bool _checked) {
        if (_checked) {
            d->audioplayEditorShowHintsForAllItems->setChecked(false);
        }
    };
    connect(d->audioplayEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            audioplayEditorCorrectShowHintsForAllItems);
    connect(d->audioplayEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            audioplayEditorCorrectShowHintsForAllItems);
    connect(d->audioplayEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            audioplayEditorCorrectShowHintsForAllItems);
    //
    connect(d->audioplayEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit audioplayEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    auto notifyAudioplayEditorShowSceneNumbersChanged = [this] {
        emit audioplayEditorShowBlockNumberChanged(
            d->audioplayEditorShowBlockNumber->isChecked(),
            d->audioplayEditorContinueBlockNumbers->isChecked());
    };
    connect(d->audioplayEditorShowBlockNumber, &CheckBox::checkedChanged, this,
            notifyAudioplayEditorShowSceneNumbersChanged);
    connect(d->audioplayEditorContinueBlockNumbers, &CheckBox::checkedChanged, this,
            notifyAudioplayEditorShowSceneNumbersChanged);
    connect(d->audioplayEditorSaveItemsFromText, &CheckBox::checkedChanged, this,
            &SettingsView::audioplayEditorSaveItemsFromTextChanged);
    auto notifyAudioplayEditorShowHintsChanged = [this] {
        emit audioplayEditorShowHintsChanged(
            d->audioplayEditorShowHintsForAllItems->isChecked(),
            d->audioplayEditorShowHintsForPrimaryItems->isChecked(),
            d->audioplayEditorShowHintsForSecondaryItems->isChecked(),
            d->audioplayEditorShowHintsForTertiaryItems->isChecked());
    };
    connect(d->audioplayEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            notifyAudioplayEditorShowHintsChanged);
    connect(d->audioplayEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            notifyAudioplayEditorShowHintsChanged);
    connect(d->audioplayEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            notifyAudioplayEditorShowHintsChanged);
    connect(d->audioplayEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            notifyAudioplayEditorShowHintsChanged);
    connect(d->audioplayEditorShowCharacterSuggestionsInEmptyBlock, &CheckBox::checkedChanged, this,
            &SettingsView::audioplayEditorShowCharacterSuggestionsInEmptyBlockChanged);
    //
    // ... навигатор сценария
    //
    connect(d->audioplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->audioplayNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->audioplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->audioplayNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->audioplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->audioplayNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->audioplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->audioplayNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->audioplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->audioplayNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    connect(d->audioplayNavigatorShowSceneNumber, &CheckBox::checkedChanged, this,
            &SettingsView::audioplayNavigatorShowSceneNumberChanged);
    auto notifyAudioplayNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->audioplayNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->audioplayNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->audioplayNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->audioplayNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit audioplayNavigatorShowSceneTextChanged(d->audioplayNavigatorShowSceneText->isChecked(),
                                                    sceneTextLines);
    };
    connect(d->audioplayNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifyAudioplayNavigatorShowSceneTextChanged);
    connect(d->audioplayNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifyAudioplayNavigatorShowSceneTextChanged);
    connect(d->audioplayNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifyAudioplayNavigatorShowSceneTextChanged);
    connect(d->audioplayNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifyAudioplayNavigatorShowSceneTextChanged);
    connect(d->audioplayNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifyAudioplayNavigatorShowSceneTextChanged);
    connect(d->audioplayNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifyAudioplayNavigatorShowSceneTextChanged);
    //
    // ... хронометраж
    //
    connect(d->audioplayDurationByWordsWords, &TextField::textChanged, this, [this] {
        emit audioplayDurationByWordsWordsChanged(d->audioplayDurationByWordsWords->text().toInt());
    });
    connect(d->audioplayDurationByWordsDuration, &TextField::textChanged, this, [this] {
        emit audioplayDurationByWordsDurationChanged(
            d->audioplayDurationByWordsDuration->text().toInt());
    });
    //
    // ... Редактор пьесы
    //
    connect(d->stageplayAvailable, &Toggle::checkedChanged, this, [this](bool _available) {
        for (auto widget : std::vector<QWidget*>{
                 d->stageplayEditorTitle,
                 d->stageplayEditorDefaultTemplate,
                 d->stageplayEditorDefaultTemplateOptions,
                 d->stageplayEditorSaveItemsFromText,
                 d->stageplayEditorShowHintsForAllItems,
                 d->stageplayEditorShowHintsForPrimaryItems,
                 d->stageplayEditorShowHintsForSecondaryItems,
                 d->stageplayEditorShowHintsForTertiaryItems,
                 d->stageplayEditorShowCharacterSuggestionsInEmptyBlock,
                 d->stageplayNavigatorTitle,
                 d->stageplayNavigatorShowSceneNumber,
                 d->stageplayNavigatorShowSceneText,
                 d->stageplayNavigatorSceneDescriptionLines1,
                 d->stageplayNavigatorSceneDescriptionLines2,
                 d->stageplayNavigatorSceneDescriptionLines3,
                 d->stageplayNavigatorSceneDescriptionLines4,
                 d->stageplayNavigatorSceneDescriptionLines5,
                 d->shortcutsForStageplayTitle,
                 d->shortcutsForStageplay,
             }) {
            widget->setVisible(_available);
        }
        d->stageplayCardLayout->setRowMinimumHeight(d->stageplayCardBottomSpacerIndex,
                                                    _available ? Ui::DesignSystem::layout().px24()
                                                               : Ui::DesignSystem::layout().px12());
        d->updateTablesGeometry();

        emit stageplayAvailableChanged(_available);
    });
    connect(d->stageplayEditorDefaultTemplateOptions, &IconButton::clicked, this, [this] {
        QVector<QAction*> actions;
        const auto templateIndex = d->stageplayEditorDefaultTemplate->currentIndex();
        const auto templateId
            = templateIndex.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
        const auto isDefaultTemplate
            = BusinessLayer::TemplatesFacade::stageplayTemplate(templateId).isDefault();
        //
        if (!isDefaultTemplate) {
            auto editAction = new QAction(tr("Edit"), d->contextMenu);
            connect(editAction, &QAction::triggered, this, [this, templateId] {
                emit editCurrentStageplayEditorTemplateRequested(templateId);
            });
            actions.append(editAction);
        }
        //
        auto duplicateAction
            = new QAction(tr("Create new template based on current"), d->contextMenu);
        connect(duplicateAction, &QAction::triggered, this, [this, templateId] {
            emit duplicateCurrentStageplayEditorTemplateRequested(templateId);
        });
        actions.append(duplicateAction);
        //
        if (!isDefaultTemplate) {
            auto saveToFileAction = new QAction(tr("Save to file"), d->contextMenu);
            connect(saveToFileAction, &QAction::triggered, this, [this, templateId] {
                emit saveToFileCurrentStageplayEditorTemplateRequested(templateId);
            });
            actions.append(saveToFileAction);
            //
            auto removeAction = new QAction(tr("Remove"), d->contextMenu);
            connect(removeAction, &QAction::triggered, this, [this, templateId] {
                emit removeCurrentStageplayEditorTemplateRequested(templateId);
            });
            actions.append(removeAction);
        }
        //
        auto loadFromFileAction = new QAction(tr("Load template from file"), d->contextMenu);
        loadFromFileAction->setSeparator(true);
        connect(loadFromFileAction, &QAction::triggered, this,
                &SettingsView::loadFromFileStageplayEditorTemplateRequested);
        actions.append(loadFromFileAction);
        //
        d->contextMenu->setActions(actions);
        d->contextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->stageplayEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            [this](bool _checked) {
                if (_checked) {
                    d->stageplayEditorShowHintsForPrimaryItems->setChecked(false);
                    d->stageplayEditorShowHintsForSecondaryItems->setChecked(false);
                    d->stageplayEditorShowHintsForTertiaryItems->setChecked(false);
                }
            });
    auto stageplayEditorCorrectShowHintsForAllItems = [this](bool _checked) {
        if (_checked) {
            d->stageplayEditorShowHintsForAllItems->setChecked(false);
        }
    };
    connect(d->stageplayEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            stageplayEditorCorrectShowHintsForAllItems);
    connect(d->stageplayEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            stageplayEditorCorrectShowHintsForAllItems);
    connect(d->stageplayEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            stageplayEditorCorrectShowHintsForAllItems);
    //
    connect(d->stageplayEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit stageplayEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    connect(d->stageplayEditorSaveItemsFromText, &CheckBox::checkedChanged, this,
            &SettingsView::stageplayEditorSaveItemsFromTextChanged);
    auto notifyStageplayEditorShowHintsChanged = [this] {
        emit stageplayEditorShowHintsChanged(
            d->stageplayEditorShowHintsForAllItems->isChecked(),
            d->stageplayEditorShowHintsForPrimaryItems->isChecked(),
            d->stageplayEditorShowHintsForSecondaryItems->isChecked(),
            d->stageplayEditorShowHintsForTertiaryItems->isChecked());
    };
    connect(d->stageplayEditorShowHintsForAllItems, &CheckBox::checkedChanged, this,
            notifyStageplayEditorShowHintsChanged);
    connect(d->stageplayEditorShowHintsForPrimaryItems, &CheckBox::checkedChanged, this,
            notifyStageplayEditorShowHintsChanged);
    connect(d->stageplayEditorShowHintsForSecondaryItems, &CheckBox::checkedChanged, this,
            notifyStageplayEditorShowHintsChanged);
    connect(d->stageplayEditorShowHintsForTertiaryItems, &CheckBox::checkedChanged, this,
            notifyStageplayEditorShowHintsChanged);
    connect(d->stageplayEditorShowCharacterSuggestionsInEmptyBlock, &CheckBox::checkedChanged, this,
            &SettingsView::stageplayEditorShowCharacterSuggestionsInEmptyBlockChanged);
    //
    // ... навигатор пьесы
    //
    connect(d->stageplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->stageplayNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->stageplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->stageplayNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->stageplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->stageplayNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->stageplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->stageplayNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->stageplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->stageplayNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    connect(d->stageplayNavigatorShowSceneNumber, &CheckBox::checkedChanged, this,
            &SettingsView::stageplayNavigatorShowSceneNumberChanged);
    auto notifyStageplayNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->stageplayNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->stageplayNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->stageplayNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->stageplayNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit stageplayNavigatorShowSceneTextChanged(d->stageplayNavigatorShowSceneText->isChecked(),
                                                    sceneTextLines);
    };
    connect(d->stageplayNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifyStageplayNavigatorShowSceneTextChanged);
    connect(d->stageplayNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifyStageplayNavigatorShowSceneTextChanged);
    connect(d->stageplayNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifyStageplayNavigatorShowSceneTextChanged);
    connect(d->stageplayNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifyStageplayNavigatorShowSceneTextChanged);
    connect(d->stageplayNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifyStageplayNavigatorShowSceneTextChanged);
    connect(d->stageplayNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifyStageplayNavigatorShowSceneTextChanged);
    //
    // ... Редактор романа
    //
    connect(d->novelAvailable, &Toggle::checkedChanged, this, [this](bool _available) {
        for (auto widget : std::vector<QWidget*>{
                 d->novelEditorTitle,
                 d->novelEditorDefaultTemplate,
                 d->novelEditorDefaultTemplateOptions,
                 d->novelNavigatorTitle,
                 d->novelNavigatorShowSceneText,
                 d->novelNavigatorSceneDescriptionLines1,
                 d->novelNavigatorSceneDescriptionLines2,
                 d->novelNavigatorSceneDescriptionLines3,
                 d->novelNavigatorSceneDescriptionLines4,
                 d->novelNavigatorSceneDescriptionLines5,
                 d->novelNavigatorCounterType,
                 d->novelNavigatorCounterUseWords,
                 d->novelNavigatorCounterUseCharacters,
                 d->novelNavigatorCounterUseCharactersWithSpaces,
                 d->shortcutsForNovelTitle,
                 d->shortcutsForNovel,
             }) {
            widget->setVisible(_available);
        }
        d->novelCardLayout->setRowMinimumHeight(d->novelCardBottomSpacerIndex,
                                                _available ? Ui::DesignSystem::layout().px24()
                                                           : Ui::DesignSystem::layout().px12());
        d->updateTablesGeometry();

        emit novelAvailableChanged(_available);
    });
    connect(d->novelEditorDefaultTemplateOptions, &IconButton::clicked, this, [this] {
        QVector<QAction*> actions;
        const auto templateIndex = d->novelEditorDefaultTemplate->currentIndex();
        const auto templateId
            = templateIndex.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
        const auto isDefaultTemplate
            = BusinessLayer::TemplatesFacade::novelTemplate(templateId).isDefault();
        //
        if (!isDefaultTemplate) {
            auto editAction = new QAction(tr("Edit"), d->contextMenu);
            connect(editAction, &QAction::triggered, this, [this, templateId] {
                emit editCurrentNovelEditorTemplateRequested(templateId);
            });
            actions.append(editAction);
        }
        //
        auto duplicateAction
            = new QAction(tr("Create new template based on current"), d->contextMenu);
        connect(duplicateAction, &QAction::triggered, this, [this, templateId] {
            emit duplicateCurrentNovelEditorTemplateRequested(templateId);
        });
        actions.append(duplicateAction);
        //
        if (!isDefaultTemplate) {
            auto saveToFileAction = new QAction(tr("Save to file"), d->contextMenu);
            connect(saveToFileAction, &QAction::triggered, this, [this, templateId] {
                emit saveToFileCurrentNovelEditorTemplateRequested(templateId);
            });
            actions.append(saveToFileAction);
            //
            auto removeAction = new QAction(tr("Remove"), d->contextMenu);
            connect(removeAction, &QAction::triggered, this, [this, templateId] {
                emit removeCurrentNovelEditorTemplateRequested(templateId);
            });
            actions.append(removeAction);
        }
        //
        auto loadFromFileAction = new QAction(tr("Load template from file"), d->contextMenu);
        loadFromFileAction->setSeparator(true);
        connect(loadFromFileAction, &QAction::triggered, this,
                &SettingsView::loadFromFileNovelEditorTemplateRequested);
        actions.append(loadFromFileAction);
        //
        d->contextMenu->setActions(actions);
        d->contextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->novelEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit novelEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    //
    // ... навигатор текста
    //
    connect(d->novelNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->novelNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->novelNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->novelNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->novelNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->novelNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->novelNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->novelNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->novelNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->novelNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    auto notifynovelNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->novelNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->novelNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->novelNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->novelNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit novelNavigatorShowSceneTextChanged(d->novelNavigatorShowSceneText->isChecked(),
                                                sceneTextLines);
    };
    connect(d->novelNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifynovelNavigatorShowSceneTextChanged);
    connect(d->novelNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifynovelNavigatorShowSceneTextChanged);
    connect(d->novelNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifynovelNavigatorShowSceneTextChanged);
    connect(d->novelNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifynovelNavigatorShowSceneTextChanged);
    connect(d->novelNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifynovelNavigatorShowSceneTextChanged);
    connect(d->novelNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifynovelNavigatorShowSceneTextChanged);
    //
    auto notifyNovelNavigatorCounterTypeChanged = [this] {
        int type = 0;
        if (d->novelNavigatorCounterUseCharacters->isChecked()) {
            type = 1;
        } else if (d->novelNavigatorCounterUseCharactersWithSpaces->isChecked()) {
            type = 2;
        }
        emit novelNavigatorCounterTypeChanged(type);
    };
    connect(d->novelNavigatorCounterUseWords, &RadioButton::checkedChanged, this,
            notifyNovelNavigatorCounterTypeChanged);
    connect(d->novelNavigatorCounterUseCharacters, &RadioButton::checkedChanged, this,
            notifyNovelNavigatorCounterTypeChanged);
    connect(d->novelNavigatorCounterUseCharactersWithSpaces, &RadioButton::checkedChanged, this,
            notifyNovelNavigatorCounterTypeChanged);

    //
    // Соединения шорткатов настраиваются в момент установки моделей с данными о них в представление
    //
}

void SettingsView::showDefaultPage()
{
    setCurrentWidget(d->content);
}

void SettingsView::showApplication()
{
    d->scrollToTitle(d->applicationTitle);
}

void SettingsView::showApplicationUserInterface()
{
    d->scrollToTitle(d->applicationUserInterfaceTitle);
}

void SettingsView::showApplicationSaveAndBackups()
{
    d->scrollToTitle(d->applicationSaveAndBackupTitle);
}

void SettingsView::showApplicationTextEditing()
{
    d->scrollToTitle(d->applicationTextEditingTitle);
}

void SettingsView::showComponents()
{
    d->scrollToTitle(d->componentsTitle);
}

void SettingsView::showComponentsSimpleText()
{
    d->scrollToTitle(d->simpleTextTitle);
}

void SettingsView::showComponentsScreenplay()
{
    d->scrollToTitle(d->screenplayTitle);
}

void SettingsView::showComponentsComicBook()
{
    d->scrollToTitle(d->comicBookTitle);
}

void SettingsView::showComponentsAudioplay()
{
    d->scrollToTitle(d->audioplayTitle);
}

void SettingsView::showComponentsStageplay()
{
    d->scrollToTitle(d->stageplayTitle);
}

void SettingsView::showComponentsNovel()
{
    d->scrollToTitle(d->novelTitle);
}

void SettingsView::showShortcuts()
{
    d->scrollToTitle(d->shortcutsTitle);
}

void SettingsView::setApplicationLanguage(int _language)
{
    auto languageString = [_language]() -> QString {
        switch (_language) {
        case QLocale::Arabic: {
            return "اَلْعَرَبِيَّةُ";
        }
        case QLocale::Armenian: {
            return "Հայերեն";
        }
        case QLocale::Azerbaijani: {
            return "Azərbaycan";
        }
        case QLocale::Belarusian: {
            return "Беларуский";
        }
        case QLocale::Catalan: {
            return "Català";
        }
        case QLocale::Chinese: {
            return "汉语";
        }
        case QLocale::Croatian: {
            return "Hrvatski";
        }
        case QLocale::Danish: {
            return "Dansk";
        }
        case QLocale::English: {
            return "English";
        }
        case QLocale::Esperanto: {
            return "Esperanto";
        }
        case QLocale::French: {
            return "Français";
        }
        case QLocale::Galician: {
            return "Galego";
        }
        case QLocale::German: {
            return "Deutsch";
        }
        case QLocale::Hebrew: {
            return "עִבְרִית";
        }
        case QLocale::Hindi: {
            return "हिन्दी";
        }
        case QLocale::Hungarian: {
            return "Magyar";
        }
        case QLocale::Indonesian: {
            return "Indonesian";
        }
        case QLocale::Italian: {
            return "Italiano";
        }
        case QLocale::Korean: {
            return "한국어";
        }
        case QLocale::Persian: {
            return "فارسی";
        }
        case QLocale::Polish: {
            return "Polski";
        }
        case QLocale::LastLanguage + 1: {
            return "Português";
        }
        case QLocale::Portuguese: {
            return "Português Brasileiro";
        }
        case QLocale::Romanian: {
            return "Română";
        }
        case QLocale::Russian: {
            return "Русский";
        }
        case QLocale::Slovenian: {
            return "Slovenski";
        }
        case QLocale::Spanish: {
            return "Español";
        }
        case QLocale::Swedish: {
            return "Svenska";
        }
        case QLocale::Filipino: {
            return "Tagalog";
        }
        case QLocale::Tamil: {
            return "தமிழ்";
        }
        case QLocale::Telugu: {
            return "తెలుగు";
        }
        case QLocale::Turkish: {
            return "Türkçe";
        }
        case QLocale::Ukrainian: {
            return "Українська";
        }
        default: {
            return QLocale::languageToString(static_cast<QLocale::Language>(_language));
        }
        }
    };
    d->changeLanuage->setText(languageString());
}

void SettingsView::setApplicationScaleFactor(qreal _scaleFactor)
{
    d->scaleFactor->setValue((_scaleFactor - 0.5) * 1000.0);
}

void SettingsView::setApplicationDensity(int _density)
{
    d->isCompact->setChecked(_density > 0);
}

void SettingsView::setApplicationUseAutoSave(bool _use)
{
    d->autoSave->setChecked(_use);
}

void SettingsView::setApplicationSaveBackups(bool _save)
{
    d->saveBackups->setChecked(_save);
}

void SettingsView::setApplicationBackupsFolder(const QString& _path)
{
    d->backupsFolderPath->setText(_path);
}

void SettingsView::setApplicationBackupsQty(int _qty)
{
    d->backupsQty->setText(QString::number(_qty));
}

void SettingsView::setApplicationShowDocumentsPages(bool _show)
{
    d->showDocumentsPages->setChecked(_show);
}

void SettingsView::setApplicationUseTypewriterSound(bool _use)
{
    d->useTypewriterSound->setChecked(_use);
}

void SettingsView::setApplicationUseSpellChecker(bool _use)
{
    d->useSpellChecker->setChecked(_use);
}

void SettingsView::setApplicationSpellCheckerLanguage(const QString& _languageCode)
{
    for (int row = 0; row < d->spellCheckerLanguagesModel->rowCount(); ++row) {
        auto item = d->spellCheckerLanguagesModel->item(row);
        if (item->data(kSpellCheckerLanguageCodeRole).toString() != _languageCode) {
            continue;
        }

        d->spellCheckerLanguage->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setApplicationHighlightCurrentLine(bool _highlight)
{
    d->highlightCurrentLine->setChecked(_highlight);
}

void SettingsView::setApplicationFocusCurrentParagraph(bool _focus)
{
    d->focusCurrentParagraph->setChecked(_focus);
}

void SettingsView::setApplicationUseTypewriterScrolling(bool _use)
{
    d->useTypewriterScrolling->setChecked(_use);
}

void SettingsView::setApplicationCorrectDoubleCapitals(bool _correct)
{
    d->correctDoubleCapitals->setChecked(_correct);
}

void SettingsView::setApplicationCapitalizeSinglaILetter(bool _capitalize)
{
    d->capitalizeSingleILetter->setChecked(_capitalize);
}

void SettingsView::setApplicationReplaceThreeDotsWithEllipsis(bool _replace)
{
    d->replaceThreeDotsWithEllipsis->setChecked(_replace);
}

void SettingsView::setApplicationUseSmartQuotes(bool _use)
{
    d->useSmartQuotes->setChecked(_use);
}

void SettingsView::setApplicationReplaceTwoDashesWithEmDash(bool _replace)
{
    d->replaceTwoDashesWithEmDash->setChecked(_replace);
}

void SettingsView::setApplicationAvoidMultipleSpaces(bool _avoid)
{
    d->avoidMultipleSpaces->setChecked(_avoid);
}

void SettingsView::setSimpleTextAvailable(bool _available)
{
    d->simpleTextAvailable->setChecked(_available);
}

void SettingsView::setSimpleTextEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::simpleTextTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::simpleTextTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->simpleTextEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setSimpleTextNavigatorShowSceneText(bool _show, int _lines)
{
    d->simpleTextNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->simpleTextNavigatorSceneDescriptionLines1 },
                { 2, d->simpleTextNavigatorSceneDescriptionLines2 },
                { 3, d->simpleTextNavigatorSceneDescriptionLines3 },
                { 4, d->simpleTextNavigatorSceneDescriptionLines4 },
                { 5, d->simpleTextNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setScreenplayAvailable(bool _available)
{
    d->screenplayAvailable->setChecked(_available);
}

void SettingsView::setScreenplayEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::screenplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::screenplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->screenplayEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight)
{
    QSignalBlocker blocker(d->screenplayEditorShowSceneNumberOnLeft);

    d->screenplayEditorShowSceneNumber->setChecked(_show);
    d->screenplayEditorShowSceneNumberOnLeft->setChecked(_atLeft);
    d->screenplayEditorShowSceneNumberOnRight->setChecked(_atRight);
}

void SettingsView::setScreenplayEditorShowDialogueNumber(bool _show)
{
    d->screenplayEditorShowDialogueNumber->setChecked(_show);
}

void SettingsView::setScreenplayEditorContinueDialogue(bool _continue)
{
    d->screenplayEditorContinueDialogue->setChecked(_continue);
}

void SettingsView::setScreenplayEditorCorrectTextOnPageBreaks(bool _correct)
{
    d->screenplayEditorCorrectTextOnPageBreaks->setChecked(_correct);
}

void SettingsView::setScreenplayEditorSaveItemsFromText(bool _save)
{
    d->screenplayEditorSaveItemsFromText->setChecked(_save);
}

void SettingsView::setScreenplayEditorShowHints(bool _all, bool _primary, bool _secondary,
                                                bool _tertiary)
{
    d->screenplayEditorShowHintsForAllItems->setChecked(_all);
    d->screenplayEditorShowHintsForPrimaryItems->setChecked(_primary);
    d->screenplayEditorShowHintsForSecondaryItems->setChecked(_secondary);
    d->screenplayEditorShowHintsForTertiaryItems->setChecked(_tertiary);
}

void SettingsView::setScreenplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    d->screenplayEditorShowCharacterSuggestionsInEmptyBlock->setChecked(_show);
}

void SettingsView::setScreenplayEditorUseOpenBracketInDialogueForParenthetical(bool _use)
{
    d->screenplayEditorUseOpenBracketInDialogueForParenthetical->setChecked(_use);
}

void SettingsView::setScreenplayNavigatorShowBeats(bool _show, bool _inTreatment,
                                                   bool _inScreenplay)
{
    QSignalBlocker blocker(d->screenplayNavigatorShowBeatsInTreatment);

    d->screenplayNavigatorShowBeats->setChecked(_show);
    d->screenplayNavigatorShowBeatsInTreatment->setChecked(_inTreatment);
    d->screenplayNavigatorShowBeatsInScreenplay->setChecked(_inScreenplay);
}

void SettingsView::setScreenplayNavigatorShowSceneNumber(bool _show)
{
    d->screenplayNavigatorShowSceneNumber->setChecked(_show);
}

void SettingsView::setScreenplayNavigatorShowSceneText(bool _show, int _lines)
{
    d->screenplayNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->screenplayNavigatorSceneDescriptionLines1 },
                { 2, d->screenplayNavigatorSceneDescriptionLines2 },
                { 3, d->screenplayNavigatorSceneDescriptionLines3 },
                { 4, d->screenplayNavigatorSceneDescriptionLines4 },
                { 5, d->screenplayNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setScreenplayDurationType(int _type)
{
    using namespace BusinessLayer;
    switch (static_cast<ChronometerType>(_type)) {
    case ChronometerType::Page: {
        d->screenplayDurationByPage->setChecked(true);
        break;
    }

    case ChronometerType::Characters: {
        d->screenplayDurationByCharacters->setChecked(true);
        break;
    }

    case ChronometerType::Configurable: {
        d->screenplayDurationConfigurable->setChecked(true);
        break;
    }
    }
}

void SettingsView::setScreenplayDurationByPageDuration(int _duration)
{
    d->screenplayDurationByPageDuration->setText(QString::number(_duration));
}

void SettingsView::setScreenplayDurationByCharactersCharacters(int _characters)
{
    d->screenplayDurationByCharactersCharacters->setText(QString::number(_characters));
}

void SettingsView::setScreenplayDurationByCharactersIncludeSpaces(bool _include)
{
    d->screenplayDurationByCharactersIncludingSpaces->setChecked(_include);
}

void SettingsView::setScreenplayDurationByCharactersDuration(int _duration)
{
    d->screenplayDurationByCharactersDuration->setText(QString::number(_duration));
}

void SettingsView::setScreenplayDurationConfigurablePerParagraphForAction(qreal _duration)
{
    d->screenplayDurationConfigurablePerParagraphForAction->setText(
        QString::number(_duration, 'f', 1));
}

void SettingsView::setScreenplayDurationConfigurablePerEvery50ForAction(qreal _duration)
{
    d->screenplayDurationConfigurablePerEvery50ForAction->setText(
        QString::number(_duration, 'f', 1));
}

void SettingsView::setScreenplayDurationConfigurablePerParagraphForDialogue(qreal _duration)
{
    d->screenplayDurationConfigurablePerParagraphForDialogue->setText(
        QString::number(_duration, 'f', 1));
}

void SettingsView::setScreenplayDurationConfigurablePerEvery50ForDialogue(qreal _duration)
{
    d->screenplayDurationConfigurablePerEvery50ForDialogue->setText(
        QString::number(_duration, 'f', 1));
}

void SettingsView::setScreenplayDurationConfigurablePerParagraphForSceneHeading(qreal _duration)
{
    d->screenplayDurationConfigurablePerParagraphForSceneHeading->setText(
        QString::number(_duration, 'f', 1));
}

void SettingsView::setScreenplayDurationConfigurablePerEvery50ForSceneHeading(qreal _duration)
{
    d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setText(
        QString::number(_duration, 'f', 1));
}

void SettingsView::setComicBookAvailable(bool _available)
{
    d->comicBookAvailable->setChecked(_available);
}

void SettingsView::setComicBookEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::comicBookTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::comicBookTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->comicBookEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setComicBookEditorShowDialogueNumber(bool _show)
{
    d->comicBookEditorShowDialogueNumber->setChecked(_show);
}

void SettingsView::setComicBookEditorSaveItemsFromText(bool _save)
{
    d->comicBookEditorSaveItemsFromText->setChecked(_save);
}

void SettingsView::setComicBookEditorShowHints(bool _all, bool _primary, bool _secondary,
                                               bool _tertiary)
{
    d->comicBookEditorShowHintsForAllItems->setChecked(_all);
    d->comicBookEditorShowHintsForPrimaryItems->setChecked(_primary);
    d->comicBookEditorShowHintsForSecondaryItems->setChecked(_secondary);
    d->comicBookEditorShowHintsForTertiaryItems->setChecked(_tertiary);
}

void SettingsView::setComicBookEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    d->comicBookEditorShowCharacterSuggestionsInEmptyBlock->setChecked(_show);
}

void SettingsView::setComicBookNavigatorShowSceneText(bool _show, int _lines)
{
    d->comicBookNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->comicBookNavigatorSceneDescriptionLines1 },
                { 2, d->comicBookNavigatorSceneDescriptionLines2 },
                { 3, d->comicBookNavigatorSceneDescriptionLines3 },
                { 4, d->comicBookNavigatorSceneDescriptionLines4 },
                { 5, d->comicBookNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setAudioplayAvailable(bool _available)
{
    d->audioplayAvailable->setChecked(_available);
}

void SettingsView::setAudioplayEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::audioplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::audioplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->audioplayEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setAudioplayEditorShowBlockNumber(bool _show, bool _continue)
{
    Q_UNUSED(_continue)
    d->audioplayEditorShowBlockNumber->setChecked(_show);
}

void SettingsView::setAudioplayEditorSaveItemsFromText(bool _save)
{
    d->audioplayEditorSaveItemsFromText->setChecked(_save);
}

void SettingsView::setAudioplayEditorShowHints(bool _all, bool _primary, bool _secondary,
                                               bool _tertiary)
{
    d->audioplayEditorShowHintsForAllItems->setChecked(_all);
    d->audioplayEditorShowHintsForPrimaryItems->setChecked(_primary);
    d->audioplayEditorShowHintsForSecondaryItems->setChecked(_secondary);
    d->audioplayEditorShowHintsForTertiaryItems->setChecked(_tertiary);
}

void SettingsView::setAudioplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    d->audioplayEditorShowCharacterSuggestionsInEmptyBlock->setChecked(_show);
}

void SettingsView::setAudioplayNavigatorShowSceneNumber(bool _show)
{
    d->audioplayNavigatorShowSceneNumber->setChecked(_show);
}

void SettingsView::setAudioplayNavigatorShowSceneText(bool _show, int _lines)
{
    d->audioplayNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->audioplayNavigatorSceneDescriptionLines1 },
                { 2, d->audioplayNavigatorSceneDescriptionLines2 },
                { 3, d->audioplayNavigatorSceneDescriptionLines3 },
                { 4, d->audioplayNavigatorSceneDescriptionLines4 },
                { 5, d->audioplayNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setAudioplayDurationByWordsWords(int _words)
{
    d->audioplayDurationByWordsWords->setText(QString::number(_words));
}

void SettingsView::setAudioplayDurationByWordsDuration(int _duration)
{
    d->audioplayDurationByWordsDuration->setText(QString::number(_duration));
}

void SettingsView::setStageplayAvailable(bool _available)
{
    d->stageplayAvailable->setChecked(_available);
}

void SettingsView::setStageplayEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::stageplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::stageplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->stageplayEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setStageplayEditorSaveItemsFromText(bool _save)
{
    d->stageplayEditorSaveItemsFromText->setChecked(_save);
}

void SettingsView::setStageplayEditorShowHints(bool _all, bool _primary, bool _secondary,
                                               bool _tertiary)
{
    d->stageplayEditorShowHintsForAllItems->setChecked(_all);
    d->stageplayEditorShowHintsForPrimaryItems->setChecked(_primary);
    d->stageplayEditorShowHintsForSecondaryItems->setChecked(_secondary);
    d->stageplayEditorShowHintsForTertiaryItems->setChecked(_tertiary);
}

void SettingsView::setStageplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    d->stageplayEditorShowCharacterSuggestionsInEmptyBlock->setChecked(_show);
}

void SettingsView::setStageplayNavigatorShowSceneNumber(bool _show)
{
    d->stageplayNavigatorShowSceneNumber->setChecked(_show);
}

void SettingsView::setStageplayNavigatorShowSceneText(bool _show, int _lines)
{
    d->stageplayNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->stageplayNavigatorSceneDescriptionLines1 },
                { 2, d->stageplayNavigatorSceneDescriptionLines2 },
                { 3, d->stageplayNavigatorSceneDescriptionLines3 },
                { 4, d->stageplayNavigatorSceneDescriptionLines4 },
                { 5, d->stageplayNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setNovelAvailable(bool _available)
{
    d->novelAvailable->setChecked(_available);
}

void SettingsView::setNovelEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::novelTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::novelTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->novelEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setNovelNavigatorShowSceneText(bool _show, int _lines)
{
    d->novelNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons = { { 1, d->novelNavigatorSceneDescriptionLines1 },
                                                   { 2, d->novelNavigatorSceneDescriptionLines2 },
                                                   { 3, d->novelNavigatorSceneDescriptionLines3 },
                                                   { 4, d->novelNavigatorSceneDescriptionLines4 },
                                                   { 5, d->novelNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setNovelNavigatorCounterType(int _type)
{
    switch (_type) {
    case 0: {
        d->novelNavigatorCounterUseWords->setChecked(true);
        break;
    }
    case 1: {
        d->novelNavigatorCounterUseCharacters->setChecked(true);
        break;
    }
    case 2: {
        d->novelNavigatorCounterUseCharactersWithSpaces->setChecked(true);
        break;
    }
    }
}

void SettingsView::setShortcutsForApplicationModel(HierarchicalModel* _model)
{
    if (d->shortcutsForApplicationModel) {
        d->shortcutsForApplicationModel->disconnect(this);
        d->shortcutsForApplicationModel->deleteLater();
    }

    d->shortcutsForApplicationModel = _model;

    d->shortcutsForApplication->setModel(d->shortcutsForApplicationModel);
    d->shortcutsForApplication->setItemDelegateForColumn(
        1, new KeySequenceDelegate(d->shortcutsForApplication));
    d->updateTablesGeometry();

    connect(d->shortcutsForApplicationModel, &HierarchicalModel::dataChanged, this,
            [this](const QModelIndex& _index) {
                if (!_index.isValid()) {
                    return;
                }

                const auto shortcutName
                    = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
                const auto shortcut
                    = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();

                emit shortcutForApplicationChanged(shortcutName, shortcut);
            });
}

void SettingsView::setShortcutsForSimpleTextModel(HierarchicalModel* _model)
{
    if (d->shortcutsForSimpleTextModel) {
        d->shortcutsForSimpleTextModel->disconnect(this);
        d->shortcutsForSimpleTextModel->deleteLater();
    }

    d->shortcutsForSimpleTextModel = _model;

    d->shortcutsForSimpleText->setModel(d->shortcutsForSimpleTextModel);
    d->shortcutsForSimpleText->setItemDelegateForColumn(
        1, new KeySequenceDelegate(d->shortcutsForSimpleText));
    d->shortcutsForSimpleText->setItemDelegateForColumn(2, d->simpleTextParagraphAddTypeDelegate);
    d->shortcutsForSimpleText->setItemDelegateForColumn(3, d->simpleTextParagraphAddTypeDelegate);
    d->shortcutsForSimpleText->setItemDelegateForColumn(4,
                                                        d->simpleTextParagraphChangeTypeDelegate);
    d->shortcutsForSimpleText->setItemDelegateForColumn(5,
                                                        d->simpleTextParagraphChangeTypeDelegate);
    d->updateTablesGeometry();

    connect(
        d->shortcutsForSimpleTextModel, &HierarchicalModel::dataChanged, this,
        [this](const QModelIndex& _index) {
            if (!_index.isValid()) {
                return;
            }

            const auto blockType
                = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
            const auto shortcut
                = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();
            const auto jumpByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByTab).data().toString();
            const auto jumpByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByEnter).data().toString();
            const auto changeByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByTab).data().toString();
            const auto changeByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByEnter)
                      .data()
                      .toString();

            emit shortcutsForSimpleTextEditorChanged(blockType, shortcut, jumpByTab, jumpByEnter,
                                                     changeByTab, changeByEnter);
        });
}

void SettingsView::setShortcutsForScreenplayModel(HierarchicalModel* _model)
{
    if (d->shortcutsForScreenplayModel) {
        d->shortcutsForScreenplayModel->disconnect(this);
        d->shortcutsForScreenplayModel->deleteLater();
    }

    d->shortcutsForScreenplayModel = _model;

    d->shortcutsForScreenplay->setModel(d->shortcutsForScreenplayModel);
    d->shortcutsForScreenplay->setItemDelegateForColumn(
        1, new KeySequenceDelegate(d->shortcutsForScreenplay));
    d->shortcutsForScreenplay->setItemDelegateForColumn(2, d->screenplayParagraphAddTypeDelegate);
    d->shortcutsForScreenplay->setItemDelegateForColumn(3, d->screenplayParagraphAddTypeDelegate);
    d->shortcutsForScreenplay->setItemDelegateForColumn(4,
                                                        d->screenplayParagraphChangeTypeDelegate);
    d->shortcutsForScreenplay->setItemDelegateForColumn(5,
                                                        d->screenplayParagraphChangeTypeDelegate);
    d->updateTablesGeometry();

    connect(
        d->shortcutsForScreenplayModel, &HierarchicalModel::dataChanged, this,
        [this](const QModelIndex& _index) {
            if (!_index.isValid()) {
                return;
            }

            const auto blockType
                = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
            const auto shortcut
                = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();
            const auto jumpByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByTab).data().toString();
            const auto jumpByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByEnter).data().toString();
            const auto changeByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByTab).data().toString();
            const auto changeByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByEnter)
                      .data()
                      .toString();

            emit shortcutsForScreenplayEditorChanged(blockType, shortcut, jumpByTab, jumpByEnter,
                                                     changeByTab, changeByEnter);
        });
}

void SettingsView::setShortcutsForComicBookModel(HierarchicalModel* _model)
{
    if (d->shortcutsForComicBookModel) {
        d->shortcutsForComicBookModel->disconnect(this);
        d->shortcutsForComicBookModel->deleteLater();
    }

    d->shortcutsForComicBookModel = _model;

    d->shortcutsForComicBook->setModel(d->shortcutsForComicBookModel);
    d->shortcutsForComicBook->setItemDelegateForColumn(
        1, new KeySequenceDelegate(d->shortcutsForComicBook));
    d->shortcutsForComicBook->setItemDelegateForColumn(2, d->comicBookParagraphAddTypeDelegate);
    d->shortcutsForComicBook->setItemDelegateForColumn(3, d->comicBookParagraphAddTypeDelegate);
    d->shortcutsForComicBook->setItemDelegateForColumn(4, d->comicBookParagraphChangeTypeDelegate);
    d->shortcutsForComicBook->setItemDelegateForColumn(5, d->comicBookParagraphChangeTypeDelegate);
    d->updateTablesGeometry();

    connect(
        d->shortcutsForComicBookModel, &HierarchicalModel::dataChanged, this,
        [this](const QModelIndex& _index) {
            if (!_index.isValid()) {
                return;
            }

            const auto blockType
                = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
            const auto shortcut
                = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();
            const auto jumpByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByTab).data().toString();
            const auto jumpByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByEnter).data().toString();
            const auto changeByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByTab).data().toString();
            const auto changeByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByEnter)
                      .data()
                      .toString();

            emit shortcutsForComicBookEditorChanged(blockType, shortcut, jumpByTab, jumpByEnter,
                                                    changeByTab, changeByEnter);
        });
}

void SettingsView::setShortcutsForAudioplayModel(HierarchicalModel* _model)
{
    if (d->shortcutsForAudioplayModel) {
        d->shortcutsForAudioplayModel->disconnect(this);
        d->shortcutsForAudioplayModel->deleteLater();
    }

    d->shortcutsForAudioplayModel = _model;

    d->shortcutsForAudioplay->setModel(d->shortcutsForAudioplayModel);
    d->shortcutsForAudioplay->setItemDelegateForColumn(
        1, new KeySequenceDelegate(d->shortcutsForAudioplay));
    d->shortcutsForAudioplay->setItemDelegateForColumn(2, d->audioplayParagraphAddTypeDelegate);
    d->shortcutsForAudioplay->setItemDelegateForColumn(3, d->audioplayParagraphAddTypeDelegate);
    d->shortcutsForAudioplay->setItemDelegateForColumn(4, d->audioplayParagraphChangeTypeDelegate);
    d->shortcutsForAudioplay->setItemDelegateForColumn(5, d->audioplayParagraphChangeTypeDelegate);
    d->updateTablesGeometry();

    connect(
        d->shortcutsForAudioplayModel, &HierarchicalModel::dataChanged, this,
        [this](const QModelIndex& _index) {
            if (!_index.isValid()) {
                return;
            }

            const auto blockType
                = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
            const auto shortcut
                = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();
            const auto jumpByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByTab).data().toString();
            const auto jumpByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByEnter).data().toString();
            const auto changeByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByTab).data().toString();
            const auto changeByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByEnter)
                      .data()
                      .toString();

            emit shortcutsForAudioplayEditorChanged(blockType, shortcut, jumpByTab, jumpByEnter,
                                                    changeByTab, changeByEnter);
        });
}

void SettingsView::setShortcutsForStageplayModel(HierarchicalModel* _model)
{
    if (d->shortcutsForStageplayModel) {
        d->shortcutsForStageplayModel->disconnect(this);
        d->shortcutsForStageplayModel->deleteLater();
    }

    d->shortcutsForStageplayModel = _model;

    d->shortcutsForStageplay->setModel(d->shortcutsForStageplayModel);
    d->shortcutsForStageplay->setItemDelegateForColumn(
        1, new KeySequenceDelegate(d->shortcutsForStageplay));
    d->shortcutsForStageplay->setItemDelegateForColumn(2, d->stageplayParagraphAddTypeDelegate);
    d->shortcutsForStageplay->setItemDelegateForColumn(3, d->stageplayParagraphAddTypeDelegate);
    d->shortcutsForStageplay->setItemDelegateForColumn(4, d->stageplayParagraphChangeTypeDelegate);
    d->shortcutsForStageplay->setItemDelegateForColumn(5, d->stageplayParagraphChangeTypeDelegate);
    d->updateTablesGeometry();

    connect(
        d->shortcutsForStageplayModel, &HierarchicalModel::dataChanged, this,
        [this](const QModelIndex& _index) {
            if (!_index.isValid()) {
                return;
            }

            const auto blockType
                = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
            const auto shortcut
                = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();
            const auto jumpByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByTab).data().toString();
            const auto jumpByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByEnter).data().toString();
            const auto changeByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByTab).data().toString();
            const auto changeByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByEnter)
                      .data()
                      .toString();

            emit shortcutsForStageplayEditorChanged(blockType, shortcut, jumpByTab, jumpByEnter,
                                                    changeByTab, changeByEnter);
        });
}

void SettingsView::setShortcutsForNovelModel(HierarchicalModel* _model)
{
    if (d->shortcutsForNovelModel) {
        d->shortcutsForNovelModel->disconnect(this);
        d->shortcutsForNovelModel->deleteLater();
    }

    d->shortcutsForNovelModel = _model;

    d->shortcutsForNovel->setModel(d->shortcutsForNovelModel);
    d->shortcutsForNovel->setItemDelegateForColumn(1,
                                                   new KeySequenceDelegate(d->shortcutsForNovel));
    d->shortcutsForNovel->setItemDelegateForColumn(2, d->novelParagraphAddTypeDelegate);
    d->shortcutsForNovel->setItemDelegateForColumn(3, d->novelParagraphAddTypeDelegate);
    d->shortcutsForNovel->setItemDelegateForColumn(4, d->novelParagraphChangeTypeDelegate);
    d->shortcutsForNovel->setItemDelegateForColumn(5, d->novelParagraphChangeTypeDelegate);
    d->updateTablesGeometry();

    connect(
        d->shortcutsForNovelModel, &HierarchicalModel::dataChanged, this,
        [this](const QModelIndex& _index) {
            if (!_index.isValid()) {
                return;
            }

            const auto blockType
                = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
            const auto shortcut
                = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();
            const auto jumpByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByTab).data().toString();
            const auto jumpByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByEnter).data().toString();
            const auto changeByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByTab).data().toString();
            const auto changeByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByEnter)
                      .data()
                      .toString();

            emit shortcutsForNovelEditorChanged(blockType, shortcut, jumpByTab, jumpByEnter,
                                                changeByTab, changeByEnter);
        });
}

void SettingsView::resizeEvent(QResizeEvent* _event)
{
    StackWidget::resizeEvent(_event);

    d->updateTablesGeometry();
}

void SettingsView::updateTranslations()
{
    d->applicationTitle->setText(tr("Application settings"));
    d->language->setText(tr("Language"));
    d->scaleFactorTitle->setText(tr("Size of the user interface elements:"));
    d->scaleFactorSmallInfo->setText(tr("small"));
    d->scaleFactorBigInfo->setText(tr("big"));
    d->isCompact->setText(tr("Make application interface compact"));
    d->applicationSaveAndBackupTitle->setText(tr("Save changes & backups"));
    d->autoSave->setText(tr("Automatically save changes as soon as possible"));
    d->autoSave->setToolTip(
        tr("Autosave works very accurately.\n"
           "It saves the project every 3 seconds if you do not use your mouse or keyboard.\n"
           "If you work with no interruptions it saves the project every 3 minutes."));
    d->saveBackups->setText(tr("Save backups"));
    d->backupsFolderPath->setLabel(tr("Backups folder path"));
    d->backupsQty->setLabel(tr("Qty"));
    d->applicationTextEditingTitle->setText(tr("Text editing"));
    d->showDocumentsPages->setText(tr("Show documents pages"));
    d->useTypewriterSound->setText(tr("Use typewriter sound for keys pressing"));
    d->useSpellChecker->setText(tr("Spell check"));
    d->spellCheckerLanguage->setLabel(tr("Spelling dictionary"));
    {
        const QVector<QString> languages = { tr("Afrikaans"),
                                             tr("Aragonese"),
                                             tr("Arabic"),
                                             tr("Asturian"),
                                             tr("Azerbaijani"),
                                             tr("Belarusian"),
                                             tr("Bulgarian"),
                                             tr("Bengali"),
                                             tr("Tibetan"),
                                             tr("Breton"),
                                             tr("Bosnian"),
                                             tr("Catalan (Valencian)"),
                                             tr("Catalan"),
                                             tr("Czech"),
                                             tr("Welsh"),
                                             tr("Danish"),
                                             tr("German (Austria)"),
                                             tr("German (Switzerland)"),
                                             tr("German"),
                                             tr("Greek (Polytonic)"),
                                             tr("Greek"),
                                             tr("English (Australia)"),
                                             tr("English (Canada)"),
                                             tr("English (United Kingdom)"),
                                             tr("English (New Zealand)"),
                                             tr("English (South Africa)"),
                                             tr("English (United States)"),
                                             tr("Esperanto"),
                                             tr("Spanish (Argentina)"),
                                             tr("Spanish (Bolivia)"),
                                             tr("Spanish (Chile)"),
                                             tr("Spanish (Colombia)"),
                                             tr("Spanish (Costa Rica)"),
                                             tr("Spanish (Cuba)"),
                                             tr("Spanish (Dominican Republic)"),
                                             tr("Spanish (Ecuador)"),
                                             tr("Spanish (Guatemala)"),
                                             tr("Spanish (Honduras)"),
                                             tr("Spanish (Mexico)"),
                                             tr("Spanish (Nicaragua)"),
                                             tr("Spanish (Panama)"),
                                             tr("Spanish (Peru)"),
                                             tr("Spanish (Philippines)"),
                                             tr("Spanish (Puerto Rico)"),
                                             tr("Spanish (Paraguay)"),
                                             tr("Spanish (El Salvador)"),
                                             tr("Spanish (United States)"),
                                             tr("Spanish (Uruguay)"),
                                             tr("Spanish (Venezuela)"),
                                             tr("Spanish"),
                                             tr("Estonian"),
                                             tr("Basque"),
                                             tr("Persian"),
                                             tr("Faroese"),
                                             tr("French"),
                                             tr("Friulian"),
                                             tr("Western Frisian"),
                                             tr("Irish"),
                                             tr("Gaelic"),
                                             tr("Galician"),
                                             tr("Gujarati"),
                                             tr("Guarani"),
                                             tr("Hebrew"),
                                             tr("Hindi"),
                                             tr("Croatian"),
                                             tr("Hungarian"),
                                             tr("Armenian"),
                                             tr("Armenian (Western)"),
                                             tr("Interlingua"),
                                             tr("Indonesian"),
                                             tr("Icelandic"),
                                             tr("Italian"),
                                             tr("Georgian"),
                                             tr("Kazakh"),
                                             tr("Kurdish"),
                                             tr("Korean"),
                                             tr("Latin"),
                                             tr("Luxembourgish"),
                                             tr("Lao"),
                                             tr("Lithuanian"),
                                             tr("Latgalian"),
                                             tr("Latvian"),
                                             tr("Macedonian"),
                                             tr("Mongolian"),
                                             tr("Maltese"),
                                             tr("Norwegian"),
                                             tr("Low German"),
                                             tr("Nepali"),
                                             tr("Dutch"),
                                             tr("Norwegian"),
                                             tr("Occitan"),
                                             tr("Polish"),
                                             tr("Portuguese (Brazilian)"),
                                             tr("Portuguese"),
                                             tr("Quechua"),
                                             tr("Romanian"),
                                             tr("Russian (with Yo)"),
                                             tr("Russian"),
                                             tr("Kinyarwanda"),
                                             tr("Sinhala"),
                                             tr("Slovak"),
                                             tr("Slovenian"),
                                             tr("Albanian"),
                                             tr("Serbian (Latin)"),
                                             tr("Serbian"),
                                             tr("Swedish (Finland)"),
                                             tr("Swedish"),
                                             tr("Swahili"),
                                             tr("Tamil"),
                                             tr("Telugu"),
                                             tr("Thai"),
                                             tr("Turkmen"),
                                             tr("Klingon (Latin)"),
                                             tr("Klingon"),
                                             tr("Turkish"),
                                             tr("Ukrainian"),
                                             tr("Vietnamese") };
        for (int index = 0; index < d->spellCheckerLanguagesModel->rowCount(); ++index) {
            const auto languageCode = d->spellCheckerLanguagesModel->item(index)
                                          ->data(kSpellCheckerLanguageCodeRole)
                                          .toString();
            const auto languageIndex = kSpellCheckerLanguagesNameToCode.indexOf(languageCode);
            d->spellCheckerLanguagesModel->item(index)->setText(languages.at(languageIndex));
        }
        d->spellCheckerLanguagesModel->sort(0);
    }
    d->applicationUserInterfaceTitle->setText(tr("User interface"));
    d->spellCheckerUserDictionary->setToolTip(tr("Manage user dictionary"));
    d->highlightCurrentLine->setText(tr("Highlight current line"));
    d->focusCurrentParagraph->setText(tr("Focus current paragraph"));
    d->useTypewriterScrolling->setText(
        tr("Use typewriter scrolling (keeps line with the cursor on the screen center)"));
    d->applicationTextEditingSubtitle->setText(tr("Automatic text corrections"));
    d->correctDoubleCapitals->setText(tr("Correct DOuble CApitals"));
    d->capitalizeSingleILetter->setText(tr("Capitalize single \"i\" letter"));
    d->replaceThreeDotsWithEllipsis->setText(tr("Replace three single dots with ellipsis"));
    d->useSmartQuotes->setText(tr("Replace vertical quotation marks to curly ones"));
    d->replaceTwoDashesWithEmDash->setText(tr("Replace two dashes with em dash"));
    d->avoidMultipleSpaces->setText(tr("Avoid multiple spaces"));

    d->componentsTitle->setText(tr("Components"));
    //
    BusinessLayer::TemplatesFacade::updateTranslations();
    //
    d->simpleTextTitle->setText(tr("Simple text module"));
    d->simpleTextAvailable->setToolTip(tr("Turn on/off text and folders module"));
    d->simpleTextEditorTitle->setText(tr("Text editor"));
    d->simpleTextEditorDefaultTemplate->setLabel(tr("Default template"));
    d->simpleTextEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->simpleTextNavigatorTitle->setText(tr("Navigator"));
    d->simpleTextNavigatorShowSceneText->setText(tr("Show chapter text, lines"));
    d->simpleTextNavigatorSceneDescriptionLines1->setText("1");
    d->simpleTextNavigatorSceneDescriptionLines2->setText("2");
    d->simpleTextNavigatorSceneDescriptionLines3->setText("3");
    d->simpleTextNavigatorSceneDescriptionLines4->setText("4");
    d->simpleTextNavigatorSceneDescriptionLines5->setText("5");
    //
    d->screenplayTitle->setText(tr("Screenplay module"));
    d->screenplayAvailable->setToolTip(tr("Turn on/off screenplay module"));
    d->screenplayEditorTitle->setText(tr("Text editor"));
    d->screenplayEditorDefaultTemplate->setLabel(tr("Default template"));
    d->screenplayEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->screenplayEditorShowSceneNumber->setText(tr("Show scene number"));
    d->screenplayEditorShowSceneNumberOnLeft->setText(tr("on the left"));
    d->screenplayEditorShowSceneNumberOnRight->setText(tr("on the right"));
    d->screenplayEditorShowDialogueNumber->setText(tr("Show dialogue number"));
    d->screenplayEditorContinueDialogue->setText(
        tr("Add a continuation (CONT'D) to dialogues separated by an action"));
    d->screenplayEditorCorrectTextOnPageBreaks->setText(
        tr("Automatically correct screenplay text on page breaks"));
    d->screenplayEditorSaveItemsFromText->setText(tr("Save characters & locations on typing"));
    d->screenplayEditorShowHintsForAllItems->setText(
        tr("Show hints for all characters & locations, or only for"));
    d->screenplayEditorShowHintsForPrimaryItems->setText(tr("primary"));
    d->screenplayEditorShowHintsForSecondaryItems->setText(tr("secondary"));
    d->screenplayEditorShowHintsForTertiaryItems->setText(tr("tertiary"));
    d->screenplayEditorShowCharacterSuggestionsInEmptyBlock->setText(
        tr("Show characters suggestions in empty paragraph"));
    d->screenplayEditorUseOpenBracketInDialogueForParenthetical->setText(
        tr("Use opening bracket in dialogue to switch to parenthetical"));
    d->screenplayNavigatorTitle->setText(tr("Screenplay navigator"));
    d->screenplayNavigatorShowBeats->setText(tr("Show beats"));
    d->screenplayNavigatorShowBeatsInTreatment->setText(tr("in treatment"));
    d->screenplayNavigatorShowBeatsInScreenplay->setText(tr("in screenplay"));
    d->screenplayNavigatorShowSceneNumber->setText(tr("Show scene number"));
    d->screenplayNavigatorShowSceneText->setText(tr("Show scene text, lines"));
    d->screenplayNavigatorSceneDescriptionLines1->setText("1");
    d->screenplayNavigatorSceneDescriptionLines2->setText("2");
    d->screenplayNavigatorSceneDescriptionLines3->setText("3");
    d->screenplayNavigatorSceneDescriptionLines4->setText("4");
    d->screenplayNavigatorSceneDescriptionLines5->setText("5");
    d->screenplayDurationTitle->setText(tr("Duration"));
    d->screenplayDurationByPage->setText(tr("Calculate duration based on the count of pages"));
    d->screenplayDurationByPagePage->setLabel(tr("at the rate of"));
    d->screenplayDurationByPagePage->setSuffix(tr("pages"));
    d->screenplayDurationByPageDuration->setLabel(tr("has duration"));
    d->screenplayDurationByPageDuration->setSuffix(tr("seconds"));
    d->screenplayDurationByCharacters->setText(
        tr("Calculate duration based on the count of letters"));
    d->screenplayDurationByCharactersCharacters->setLabel(tr("at the rate of"));
    d->screenplayDurationByCharactersCharacters->setSuffix(tr("letters"));
    d->screenplayDurationByCharactersIncludingSpaces->setText(tr("including spaces"));
    d->screenplayDurationByCharactersDuration->setLabel(tr("has duration"));
    d->screenplayDurationByCharactersDuration->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurable->setText(tr("Calculate duration based on the custom rules"));
    d->screenplayDurationConfigurableForActionLabel->setText(tr("For action"));
    d->screenplayDurationConfigurablePerParagraphForAction->setLabel(tr("Per entry"));
    d->screenplayDurationConfigurablePerParagraphForAction->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForActionPlus->setText(tr("+"));
    d->screenplayDurationConfigurablePerEvery50ForAction->setLabel(tr("Each 50 characters"));
    d->screenplayDurationConfigurablePerEvery50ForAction->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForDialogueLabel->setText(tr("For dialogue"));
    d->screenplayDurationConfigurablePerParagraphForDialogue->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForDialoguePlus->setText(tr("+"));
    d->screenplayDurationConfigurablePerEvery50ForDialogue->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForSceneHeadingLabel->setText(tr("For scene heading"));
    d->screenplayDurationConfigurablePerParagraphForSceneHeading->setSuffix(tr("seconds"));
    d->screenplayDurationConfigurableForSceneHeadingPlus->setText(tr("+"));
    d->screenplayDurationConfigurablePerEvery50ForSceneHeading->setSuffix(tr("seconds"));
    //
    d->comicBookTitle->setText(tr("Comic book module"));
    d->comicBookAvailable->setToolTip(tr("Turn on/off comic book module"));
    d->comicBookEditorTitle->setText(tr("Text editor"));
    d->comicBookEditorDefaultTemplate->setLabel(tr("Default template"));
    d->comicBookEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->comicBookEditorShowDialogueNumber->setText(tr("Show dialogue number"));
    d->comicBookEditorSaveItemsFromText->setText(tr("Save characters on typing"));
    d->comicBookEditorShowHintsForAllItems->setText(
        tr("Show hints for all characters, or only for"));
    d->comicBookEditorShowHintsForPrimaryItems->setText(tr("primary"));
    d->comicBookEditorShowHintsForSecondaryItems->setText(tr("secondary"));
    d->comicBookEditorShowHintsForTertiaryItems->setText(tr("tertiary"));
    d->comicBookEditorShowCharacterSuggestionsInEmptyBlock->setText(
        tr("Show characters suggestions in empty paragraph"));
    d->comicBookNavigatorTitle->setText(tr("Navigator"));
    d->comicBookNavigatorShowSceneText->setText(tr("Show panel text, lines"));
    d->comicBookNavigatorSceneDescriptionLines1->setText("1");
    d->comicBookNavigatorSceneDescriptionLines2->setText("2");
    d->comicBookNavigatorSceneDescriptionLines3->setText("3");
    d->comicBookNavigatorSceneDescriptionLines4->setText("4");
    d->comicBookNavigatorSceneDescriptionLines5->setText("5");
    //
    d->audioplayTitle->setText(tr("Audioplay module"));
    d->audioplayAvailable->setToolTip(tr("Turn on/off audioplay module"));
    d->audioplayEditorTitle->setText(tr("Text editor"));
    d->audioplayEditorDefaultTemplate->setLabel(tr("Default template"));
    d->audioplayEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->audioplayEditorShowBlockNumber->setText(tr("Show block numbers"));
    d->audioplayEditorContinueBlockNumbers->setText(tr("Continue block numbers through document"));
    d->audioplayEditorSaveItemsFromText->setText(tr("Save characters on typing"));
    d->audioplayEditorShowHintsForAllItems->setText(
        tr("Show hints for all characters, or only for"));
    d->audioplayEditorShowHintsForPrimaryItems->setText(tr("primary"));
    d->audioplayEditorShowHintsForSecondaryItems->setText(tr("secondary"));
    d->audioplayEditorShowHintsForTertiaryItems->setText(tr("tertiary"));
    d->audioplayEditorShowCharacterSuggestionsInEmptyBlock->setText(
        tr("Show characters suggestions in empty paragraph"));
    d->audioplayNavigatorTitle->setText(tr("Audioplay navigator"));
    d->audioplayNavigatorShowSceneNumber->setText(tr("Show scene number"));
    d->audioplayNavigatorShowSceneText->setText(tr("Show scene text, lines"));
    d->audioplayNavigatorSceneDescriptionLines1->setText("1");
    d->audioplayNavigatorSceneDescriptionLines2->setText("2");
    d->audioplayNavigatorSceneDescriptionLines3->setText("3");
    d->audioplayNavigatorSceneDescriptionLines4->setText("4");
    d->audioplayNavigatorSceneDescriptionLines5->setText("5");
    d->audioplayDurationTitle->setText(tr("Duration"));
    d->audioplayDurationByWordsTitle->setText(tr("Calculate duration based on the count of words"));
    d->audioplayDurationByWordsWords->setLabel(tr("at the rate of"));
    d->audioplayDurationByWordsWords->setSuffix(tr("words"));
    d->audioplayDurationByWordsDuration->setLabel(tr("has duration"));
    d->audioplayDurationByWordsDuration->setSuffix(tr("seconds"));
    //
    d->stageplayTitle->setText(tr("Stageplay module"));
    d->stageplayAvailable->setToolTip(tr("Turn on/off stageplay module"));
    d->stageplayEditorTitle->setText(tr("Text editor"));
    d->stageplayEditorDefaultTemplate->setLabel(tr("Default template"));
    d->stageplayEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->stageplayEditorSaveItemsFromText->setText(tr("Save characters on typing"));
    d->stageplayEditorShowHintsForAllItems->setText(
        tr("Show hints for all characters, or only for"));
    d->stageplayEditorShowHintsForPrimaryItems->setText(tr("primary"));
    d->stageplayEditorShowHintsForSecondaryItems->setText(tr("secondary"));
    d->stageplayEditorShowHintsForTertiaryItems->setText(tr("tertiary"));
    d->stageplayEditorShowCharacterSuggestionsInEmptyBlock->setText(
        tr("Show characters suggestions in empty paragraph"));
    d->stageplayNavigatorTitle->setText(tr("Stageplay navigator"));
    d->stageplayNavigatorShowSceneNumber->setText(tr("Show scene number"));
    d->stageplayNavigatorShowSceneText->setText(tr("Show scene text, lines"));
    d->stageplayNavigatorSceneDescriptionLines1->setText("1");
    d->stageplayNavigatorSceneDescriptionLines2->setText("2");
    d->stageplayNavigatorSceneDescriptionLines3->setText("3");
    d->stageplayNavigatorSceneDescriptionLines4->setText("4");
    d->stageplayNavigatorSceneDescriptionLines5->setText("5");
    //
    d->novelTitle->setText(tr("Novel module"));
    d->novelAvailable->setToolTip(tr("Turn on/off novel module"));
    d->novelEditorTitle->setText(tr("Text editor"));
    d->novelEditorDefaultTemplate->setLabel(tr("Default template"));
    d->novelEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->novelNavigatorTitle->setText(tr("Navigator"));
    d->novelNavigatorShowSceneText->setText(tr("Show scene text, lines"));
    d->novelNavigatorSceneDescriptionLines1->setText("1");
    d->novelNavigatorSceneDescriptionLines2->setText("2");
    d->novelNavigatorSceneDescriptionLines3->setText("3");
    d->novelNavigatorSceneDescriptionLines4->setText("4");
    d->novelNavigatorSceneDescriptionLines5->setText("5");
    d->novelNavigatorCounterType->setText(tr("Scene content counter type"));
    d->novelNavigatorCounterUseWords->setText(tr("words"));
    d->novelNavigatorCounterUseCharacters->setText(tr("characters"));
    d->novelNavigatorCounterUseCharactersWithSpaces->setText(tr("characters with spaces"));

    d->shortcutsTitle->setText(tr("Shortcuts"));
    //
    d->shortcutsForApplicationTitle->setText(tr("Application"));
    {
        auto model = d->shortcutsForApplicationModel->headerModel();
        model->setData(model->index(0, 0), tr("Action"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
    }
    //
    d->shortcutsForSimpleTextTitle->setText(tr("Simple text editor"));
    {
        auto model = d->shortcutsForSimpleTextModel->headerModel();
        model->setData(model->index(0, 0), tr("Block name"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
        model->setData(model->index(0, 2), tr("If you press in paragraphs end"), Qt::DisplayRole);
        model->setData(model->index(0, 3), tr("If you press in empty paragraph"), Qt::DisplayRole);
    }
    buildSimpleTextParagraphTypesModel(d->shortcutsForSimpleText, d->simpleTextParagraphTypesModel);
    d->simpleTextParagraphAddTypeDelegate->setLabel(tr("Add paragraph"));
    d->simpleTextParagraphChangeTypeDelegate->setLabel(tr("Change to"));
    //
    d->shortcutsForScreenplayTitle->setText(tr("Screenplay editor"));
    {
        auto model = d->shortcutsForScreenplayModel->headerModel();
        model->setData(model->index(0, 0), tr("Block name"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
        model->setData(model->index(0, 2), tr("If you press in paragraphs end"), Qt::DisplayRole);
        model->setData(model->index(0, 3), tr("If you press in empty paragraph"), Qt::DisplayRole);
    }
    buildScreenplayParagraphTypesModel(d->shortcutsForScreenplay, d->screenplayParagraphTypesModel);
    d->screenplayParagraphAddTypeDelegate->setLabel(tr("Add paragraph"));
    d->screenplayParagraphChangeTypeDelegate->setLabel(tr("Change to"));
    //
    d->shortcutsForComicBookTitle->setText(tr("Comic book editor"));
    {
        auto model = d->shortcutsForComicBookModel->headerModel();
        model->setData(model->index(0, 0), tr("Block name"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
        model->setData(model->index(0, 2), tr("If you press in paragraphs end"), Qt::DisplayRole);
        model->setData(model->index(0, 3), tr("If you press in empty paragraph"), Qt::DisplayRole);
    }
    buildComicBookParagraphTypesModel(d->shortcutsForComicBook, d->comicBookParagraphTypesModel);
    d->comicBookParagraphAddTypeDelegate->setLabel(tr("Add paragraph"));
    d->comicBookParagraphChangeTypeDelegate->setLabel(tr("Change to"));
    //
    d->shortcutsForAudioplayTitle->setText(tr("Audioplay editor"));
    {
        auto model = d->shortcutsForAudioplayModel->headerModel();
        model->setData(model->index(0, 0), tr("Block name"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
        model->setData(model->index(0, 2), tr("If you press in paragraphs end"), Qt::DisplayRole);
        model->setData(model->index(0, 3), tr("If you press in empty paragraph"), Qt::DisplayRole);
    }
    buildAudioplayParagraphTypesModel(d->shortcutsForAudioplay, d->audioplayParagraphTypesModel);
    d->audioplayParagraphAddTypeDelegate->setLabel(tr("Add paragraph"));
    d->audioplayParagraphChangeTypeDelegate->setLabel(tr("Change to"));
    //
    d->shortcutsForStageplayTitle->setText(tr("Stageplay editor"));
    {
        auto model = d->shortcutsForStageplayModel->headerModel();
        model->setData(model->index(0, 0), tr("Block name"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
        model->setData(model->index(0, 2), tr("If you press in paragraphs end"), Qt::DisplayRole);
        model->setData(model->index(0, 3), tr("If you press in empty paragraph"), Qt::DisplayRole);
    }
    buildStageplayParagraphTypesModel(d->shortcutsForStageplay, d->stageplayParagraphTypesModel);
    d->stageplayParagraphAddTypeDelegate->setLabel(tr("Add paragraph"));
    d->stageplayParagraphChangeTypeDelegate->setLabel(tr("Change to"));
    //
    d->shortcutsForNovelTitle->setText(tr("Novel editor"));
    {
        auto model = d->shortcutsForNovelModel->headerModel();
        model->setData(model->index(0, 0), tr("Block name"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
        model->setData(model->index(0, 2), tr("If you press in paragraphs end"), Qt::DisplayRole);
        model->setData(model->index(0, 3), tr("If you press in empty paragraph"), Qt::DisplayRole);
    }
    buildNovelParagraphTypesModel(d->shortcutsForNovel, d->novelParagraphTypesModel);
    d->novelParagraphAddTypeDelegate->setLabel(tr("Add paragraph"));
    d->novelParagraphChangeTypeDelegate->setLabel(tr("Change to"));
}

SettingsView::~SettingsView() = default;

void SettingsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());
    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());
    d->contextMenu->setBackgroundColor(Ui::DesignSystem::color().background());
    d->contextMenu->setTextColor(Ui::DesignSystem::color().onBackground());

    d->colorAnimation.setStartValue(Ui::DesignSystem::color().accent());

    for (auto card : {
             d->applicationCard,
             d->simpleTextCard,
             d->screenplayCard,
             d->comicBookCard,
             d->audioplayCard,
             d->stageplayCard,
             d->novelCard,
             d->shortcutsCard,
         }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    auto titleColor = DesignSystem::color().onBackground();
    titleColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setBottom(Ui::DesignSystem::compactLayout().px12());
    for (auto cardTitle : std::vector<Widget*>{
             d->applicationTitle,
             d->applicationUserInterfaceTitle,
             d->applicationSaveAndBackupTitle,
             d->applicationTextEditingTitle,
             d->applicationTextEditingSubtitle,
             d->simpleTextTitle,
             d->simpleTextEditorTitle,
             d->simpleTextNavigatorTitle,
             d->screenplayTitle,
             d->screenplayEditorTitle,
             d->screenplayNavigatorTitle,
             d->screenplayDurationTitle,
             d->comicBookTitle,
             d->comicBookEditorTitle,
             d->comicBookNavigatorTitle,
             d->audioplayTitle,
             d->audioplayEditorTitle,
             d->audioplayNavigatorTitle,
             d->audioplayDurationTitle,
             d->stageplayTitle,
             d->stageplayEditorTitle,
             d->stageplayNavigatorTitle,
             d->novelTitle,
             d->novelEditorTitle,
             d->novelNavigatorTitle,
             d->shortcutsTitle,
             d->shortcutsForApplicationTitle,
             d->shortcutsForSimpleTextTitle,
             d->shortcutsForScreenplayTitle,
             d->shortcutsForComicBookTitle,
             d->shortcutsForAudioplayTitle,
             d->shortcutsForStageplayTitle,
             d->shortcutsForNovelTitle,
         }) {
        cardTitle->setBackgroundColor(DesignSystem::color().background());
        cardTitle->setTextColor(titleColor);
        cardTitle->setContentsMargins(titleMargins);
    }
    titleMargins.setBottom(0);
    for (auto title : std::vector<Widget*>{ d->componentsTitle }) {
        title->setBackgroundColor(DesignSystem::color().surface());
        title->setTextColor(titleColor);
        title->setContentsMargins(titleMargins);
    }
    for (auto toggle : std::vector<Widget*>{
             d->simpleTextAvailable,
             d->screenplayAvailable,
             d->comicBookAvailable,
             d->audioplayAvailable,
             d->stageplayAvailable,
             d->novelAvailable,
         }) {
        toggle->setBackgroundColor(DesignSystem::color().background());
        toggle->setTextColor(DesignSystem::color().onBackground());
        toggle->setContentsMargins(Ui::DesignSystem::layout().px16(),
                                   Ui::DesignSystem::layout().px16(),
                                   Ui::DesignSystem::layout().px16(), 0);
    }

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setTop(static_cast<int>(Ui::DesignSystem::button().shadowMargins().top()));
    labelMargins.setBottom(static_cast<int>(Ui::DesignSystem::button().shadowMargins().bottom()));
    for (auto label : std::vector<Widget*>{
             d->language,
             d->scaleFactorTitle,
             d->scaleFactorSmallInfo,
             d->scaleFactorBigInfo,
             d->screenplayDurationConfigurableForActionLabel,
             d->screenplayDurationConfigurableForActionPlus,
             d->screenplayDurationConfigurableForDialogueLabel,
             d->screenplayDurationConfigurableForDialoguePlus,
             d->screenplayDurationConfigurableForSceneHeadingLabel,
             d->screenplayDurationConfigurableForSceneHeadingPlus,
             d->audioplayDurationByWordsTitle,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
        label->setContentsMargins(labelMargins);
    }

    auto iconLabelMargins = labelMargins;
    iconLabelMargins.setLeft(0);
    for (auto iconLabel : std::vector<Widget*>{
             d->spellCheckerUserDictionary,
             d->simpleTextEditorDefaultTemplateOptions,
             d->screenplayEditorDefaultTemplateOptions,
             d->comicBookEditorDefaultTemplateOptions,
             d->audioplayEditorDefaultTemplateOptions,
             d->stageplayEditorDefaultTemplateOptions,
             d->novelEditorDefaultTemplateOptions,
         }) {
        iconLabel->setBackgroundColor(DesignSystem::color().background());
        iconLabel->setTextColor(DesignSystem::color().onBackground());
        iconLabel->setContentsMargins(iconLabelMargins);
    }

    d->novelNavigatorCounterType->setContentsMargins(
        DesignSystem::layout().px24(), DesignSystem::layout().px12(), DesignSystem::layout().px24(),
        DesignSystem::layout().px(36));
    for (auto checkBox : std::vector<Widget*>{
             d->isCompact,
             d->autoSave,
             d->saveBackups,
             //
             d->showDocumentsPages,
             d->useTypewriterSound,
             d->useSpellChecker,
             d->highlightCurrentLine,
             d->focusCurrentParagraph,
             d->useTypewriterScrolling,
             d->correctDoubleCapitals,
             d->capitalizeSingleILetter,
             d->replaceThreeDotsWithEllipsis,
             d->useSmartQuotes,
             d->replaceTwoDashesWithEmDash,
             d->avoidMultipleSpaces,
             //
             d->simpleTextNavigatorShowSceneText,
             //
             d->screenplayEditorShowSceneNumber,
             d->screenplayEditorShowSceneNumberOnLeft,
             d->screenplayEditorShowSceneNumberOnRight,
             d->screenplayEditorShowDialogueNumber,
             d->screenplayEditorContinueDialogue,
             d->screenplayEditorCorrectTextOnPageBreaks,
             d->screenplayEditorSaveItemsFromText,
             d->screenplayEditorShowHintsForAllItems,
             d->screenplayEditorShowHintsForPrimaryItems,
             d->screenplayEditorShowHintsForSecondaryItems,
             d->screenplayEditorShowHintsForTertiaryItems,
             d->screenplayEditorShowCharacterSuggestionsInEmptyBlock,
             d->screenplayEditorUseOpenBracketInDialogueForParenthetical,
             d->screenplayNavigatorShowBeats,
             d->screenplayNavigatorShowBeatsInTreatment,
             d->screenplayNavigatorShowBeatsInScreenplay,
             d->screenplayNavigatorShowSceneNumber,
             d->screenplayNavigatorShowSceneText,
             d->screenplayDurationByCharactersIncludingSpaces,
             //
             d->comicBookEditorShowDialogueNumber,
             d->comicBookEditorSaveItemsFromText,
             d->comicBookEditorShowHintsForAllItems,
             d->comicBookEditorShowHintsForPrimaryItems,
             d->comicBookEditorShowHintsForSecondaryItems,
             d->comicBookEditorShowHintsForTertiaryItems,
             d->comicBookEditorShowCharacterSuggestionsInEmptyBlock,
             d->comicBookNavigatorShowSceneText,
             //
             d->audioplayEditorShowBlockNumber,
             d->audioplayEditorContinueBlockNumbers,
             d->audioplayEditorSaveItemsFromText,
             d->audioplayEditorShowHintsForAllItems,
             d->audioplayEditorShowHintsForPrimaryItems,
             d->audioplayEditorShowHintsForSecondaryItems,
             d->audioplayEditorShowHintsForTertiaryItems,
             d->audioplayEditorShowCharacterSuggestionsInEmptyBlock,
             d->audioplayNavigatorShowSceneNumber,
             d->audioplayNavigatorShowSceneText,
             //
             d->stageplayEditorSaveItemsFromText,
             d->stageplayEditorShowHintsForAllItems,
             d->stageplayEditorShowHintsForPrimaryItems,
             d->stageplayEditorShowHintsForSecondaryItems,
             d->stageplayEditorShowHintsForTertiaryItems,
             d->stageplayEditorShowCharacterSuggestionsInEmptyBlock,
             d->stageplayNavigatorShowSceneNumber,
             d->stageplayNavigatorShowSceneText,
             //
             d->novelNavigatorShowSceneText,
             d->novelNavigatorCounterType,
         }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto radioButton : {
             d->simpleTextNavigatorSceneDescriptionLines1,
             d->simpleTextNavigatorSceneDescriptionLines2,
             d->simpleTextNavigatorSceneDescriptionLines3,
             d->simpleTextNavigatorSceneDescriptionLines4,
             d->simpleTextNavigatorSceneDescriptionLines5,
             //
             d->screenplayNavigatorSceneDescriptionLines1,
             d->screenplayNavigatorSceneDescriptionLines2,
             d->screenplayNavigatorSceneDescriptionLines3,
             d->screenplayNavigatorSceneDescriptionLines4,
             d->screenplayNavigatorSceneDescriptionLines5,
             d->screenplayDurationByPage,
             d->screenplayDurationByCharacters,
             d->screenplayDurationConfigurable,
             //
             d->comicBookNavigatorSceneDescriptionLines1,
             d->comicBookNavigatorSceneDescriptionLines2,
             d->comicBookNavigatorSceneDescriptionLines3,
             d->comicBookNavigatorSceneDescriptionLines4,
             d->comicBookNavigatorSceneDescriptionLines5,
             //
             d->audioplayNavigatorSceneDescriptionLines1,
             d->audioplayNavigatorSceneDescriptionLines2,
             d->audioplayNavigatorSceneDescriptionLines3,
             d->audioplayNavigatorSceneDescriptionLines4,
             d->audioplayNavigatorSceneDescriptionLines5,
             //
             d->stageplayNavigatorSceneDescriptionLines1,
             d->stageplayNavigatorSceneDescriptionLines2,
             d->stageplayNavigatorSceneDescriptionLines3,
             d->stageplayNavigatorSceneDescriptionLines4,
             d->stageplayNavigatorSceneDescriptionLines5,
             //
             d->novelNavigatorSceneDescriptionLines1,
             d->novelNavigatorSceneDescriptionLines2,
             d->novelNavigatorSceneDescriptionLines3,
             d->novelNavigatorSceneDescriptionLines4,
             d->novelNavigatorSceneDescriptionLines5,
             d->novelNavigatorCounterUseWords,
             d->novelNavigatorCounterUseCharacters,
             d->novelNavigatorCounterUseCharactersWithSpaces,
         }) {
        radioButton->setBackgroundColor(DesignSystem::color().background());
        radioButton->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto textField : std::vector<TextField*>{
             d->backupsFolderPath,
             d->backupsQty,
             d->spellCheckerLanguage,
             //
             d->simpleTextEditorDefaultTemplate,
             //
             d->screenplayEditorDefaultTemplate,
             d->screenplayDurationByPagePage,
             d->screenplayDurationByPageDuration,
             d->screenplayDurationByCharactersCharacters,
             d->screenplayDurationByCharactersDuration,
             d->screenplayDurationConfigurablePerParagraphForAction,
             d->screenplayDurationConfigurablePerEvery50ForAction,
             d->screenplayDurationConfigurablePerParagraphForDialogue,
             d->screenplayDurationConfigurablePerEvery50ForDialogue,
             d->screenplayDurationConfigurablePerParagraphForSceneHeading,
             d->screenplayDurationConfigurablePerEvery50ForSceneHeading,
             //
             d->comicBookEditorDefaultTemplate,
             //
             d->audioplayEditorDefaultTemplate,
             d->audioplayDurationByWordsWords,
             d->audioplayDurationByWordsDuration,
             //
             d->stageplayEditorDefaultTemplate,
             //
             d->novelEditorDefaultTemplate,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setTextColor(DesignSystem::color().onBackground());
    }
    d->backupsQty->setCustomMargins({ isLeftToRight() ? 0.0 : DesignSystem::layout().px24(), 0.0,
                                      isLeftToRight() ? DesignSystem::layout().px24() : 0.0, 0.0 });
    for (auto textField : {
             d->simpleTextEditorDefaultTemplate,
             d->screenplayEditorDefaultTemplate,
             d->comicBookEditorDefaultTemplate,
             d->audioplayEditorDefaultTemplate,
             d->stageplayEditorDefaultTemplate,
             d->novelEditorDefaultTemplate,
         }) {
        textField->setCustomMargins({ isLeftToRight() ? Ui::DesignSystem::layout().px24() : 0, 0,
                                      isLeftToRight() ? 0 : Ui::DesignSystem::layout().px24(), 0 });
    }
    for (auto combobox : {
             d->spellCheckerLanguage,
             d->simpleTextEditorDefaultTemplate,
             d->screenplayEditorDefaultTemplate,
             d->comicBookEditorDefaultTemplate,
             d->audioplayEditorDefaultTemplate,
             d->stageplayEditorDefaultTemplate,
             d->novelEditorDefaultTemplate,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }
    for (auto icon : {
             d->simpleTextEditorDefaultTemplateOptions,
             d->screenplayEditorDefaultTemplateOptions,
             d->comicBookEditorDefaultTemplateOptions,
             d->audioplayEditorDefaultTemplateOptions,
             d->stageplayEditorDefaultTemplateOptions,
             d->novelEditorDefaultTemplateOptions,
         }) {
        icon->setContentsMargins(isLeftToRight() ? 0 : Ui::DesignSystem::layout().px16(), 0,
                                 isLeftToRight() ? Ui::DesignSystem::layout().px16() : 0, 0);
    }

    for (auto button : { d->changeLanuage }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    for (auto theme : {
             d->lightTheme,
             d->darkAndLightTheme,
             d->darkTheme,
             d->customTheme,
         }) {
        theme->setBackgroundColor(Ui::DesignSystem::color().background());
        theme->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->scaleFactor->setBackgroundColor(DesignSystem::color().background());
    d->scaleFactor->setContentsMargins({ static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                         static_cast<int>(Ui::DesignSystem::layout().px24()), 0 });

    d->applicationCardLayout->setRowMinimumHeight(
        d->applicationCardBottomSpacerIndex,
        static_cast<int>(Ui::DesignSystem::compactLayout().px24()));
    d->applicationThemesLayout->setSpacing(Ui::DesignSystem::compactLayout().px24());
    d->applicationThemesLayout->setContentsMargins(
        QMargins(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px12(),
                 Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px16()));
    //
    d->simpleTextCardLayout->setRowMinimumHeight(d->simpleTextCardBottomSpacerIndex,
                                                 d->simpleTextAvailable->isChecked()
                                                     ? Ui::DesignSystem::compactLayout().px24()
                                                     : Ui::DesignSystem::compactLayout().px12());
    //
    d->screenplayCardLayout->setRowMinimumHeight(d->screenplayCardBottomSpacerIndex,
                                                 d->screenplayAvailable->isChecked()
                                                     ? Ui::DesignSystem::compactLayout().px24()
                                                     : Ui::DesignSystem::compactLayout().px12());
    const auto screenplayDurationByCharactersRow
        = d->screenplayCardLayout->indexOf(d->screenplayDurationByCharacters);
    d->screenplayCardLayout->setRowMinimumHeight(
        screenplayDurationByCharactersRow,
        d->screenplayAvailable->isChecked() ? Ui::DesignSystem::compactLayout().px62() : 0.0);
    const auto screenplayDurationConfigurableRow
        = d->screenplayCardLayout->indexOf(d->screenplayDurationConfigurable);
    d->screenplayCardLayout->setRowMinimumHeight(
        screenplayDurationConfigurableRow,
        d->screenplayAvailable->isChecked() ? Ui::DesignSystem::compactLayout().px62() : 0.0);
    //
    d->comicBookCardLayout->setRowMinimumHeight(d->comicBookCardBottomSpacerIndex,
                                                d->comicBookAvailable->isChecked()
                                                    ? Ui::DesignSystem::compactLayout().px24()
                                                    : Ui::DesignSystem::compactLayout().px12());
    //
    d->audioplayCardLayout->setRowMinimumHeight(d->audioplayCardBottomSpacerIndex,
                                                d->audioplayAvailable->isChecked()
                                                    ? Ui::DesignSystem::compactLayout().px24()
                                                    : Ui::DesignSystem::compactLayout().px12());
    //
    d->stageplayCardLayout->setRowMinimumHeight(d->stageplayCardBottomSpacerIndex,
                                                d->stageplayAvailable->isChecked()
                                                    ? Ui::DesignSystem::compactLayout().px24()
                                                    : Ui::DesignSystem::compactLayout().px12());
    //
    d->novelCardLayout->setRowMinimumHeight(d->simpleTextCardBottomSpacerIndex,
                                            d->novelAvailable->isChecked()
                                                ? Ui::DesignSystem::compactLayout().px24()
                                                : Ui::DesignSystem::compactLayout().px12());
    //
    d->shortcutsCardLayout->setRowMinimumHeight(
        d->shortcutsCardBottomSpacerIndex,
        static_cast<int>(Ui::DesignSystem::compactLayout().px24()));

    for (auto table : {
             d->shortcutsForApplication,
             d->shortcutsForSimpleText,
             d->shortcutsForScreenplay,
             d->shortcutsForComicBook,
             d->shortcutsForAudioplay,
             d->shortcutsForStageplay,
             d->shortcutsForNovel,
         }) {
        table->setBackgroundColor(Ui::DesignSystem::color().background());
        table->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->updateTablesGeometry();
}

} // namespace Ui
