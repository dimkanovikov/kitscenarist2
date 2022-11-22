#include "settings_manager.h"

#include "template_options_manager.h"

#include <3rd_party/webloader/src/NetworkRequest.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/settings/language_dialog.h>
#include <ui/settings/settings_navigator.h>
#include <ui/settings/settings_tool_bar.h>
#include <ui/settings/settings_view.h>
#include <ui/settings/theme_setup_view.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <ui/widgets/tree/tree_header_view.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>
#include <utils/helpers/shortcuts_helper.h>

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTimer>


namespace ManagementLayer {

namespace {

/**
 * @brief Сформировать идентификатор задачи для загрузки языка проверки орфографии
 */
QString spellCheckerLoadingTaskId(const QString& _languageId)
{
    return QLatin1String("spell_checker_loading_task_id_") + _languageId;
}

} // namespace

class SettingsManager::Implementation
{
public:
    explicit Implementation(QObject* _parent, QWidget* _parentWidget,
                            const PluginsBuilder& _pluginsBuilder);

    /**
     * @brief Загрузить параметры приложения
     */
    void loadApplicationSettings();

    /**
     * @brief Загрузить настройки компонентов
     */
    void loadComponentsSettings();
    void loadSimpleTextSettings();
    void loadScreenplaySettings();
    void loadComicBookSettings();
    void loadAudioplaySettings();
    void loadStageplaySettings();
    void loadShortcutsSettings();


    Ui::SettingsToolBar* toolBar = nullptr;
    Ui::SettingsNavigator* navigator = nullptr;
    Ui::SettingsView* view = nullptr;
    Ui::ThemeSetupView* themeSetupView = nullptr;

    TemplateOptionsManager* templateOptionsManager = nullptr;
};

SettingsManager::Implementation::Implementation(QObject* _parent, QWidget* _parentWidget,
                                                const PluginsBuilder& _pluginsBuilder)
    : toolBar(new Ui::SettingsToolBar(_parentWidget))
    , navigator(new Ui::SettingsNavigator(_parentWidget))
    , view(new Ui::SettingsView(_parentWidget))
    , templateOptionsManager(new TemplateOptionsManager(_parent, _parentWidget, _pluginsBuilder))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}

void SettingsManager::Implementation::loadApplicationSettings()
{
    view->setApplicationLanguage(settingsValue(DataStorageLayer::kApplicationLanguagedKey).toInt());
    view->setApplicationScaleFactor(
        settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
    view->setApplicationUseAutoSave(
        settingsValue(DataStorageLayer::kApplicationUseAutoSaveKey).toBool());
    view->setApplicationSaveBackups(
        settingsValue(DataStorageLayer::kApplicationSaveBackupsKey).toBool());
    view->setApplicationBackupsFolder(
        settingsValue(DataStorageLayer::kApplicationBackupsFolderKey).toString());
    view->setApplicationShowDocumentsPages(
        settingsValue(DataStorageLayer::kApplicationShowDocumentsPagesKey).toBool());
    view->setApplicationUseTypewriterSound(
        settingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey).toBool());
    view->setApplicationUseSpellChecker(
        settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool());
    view->setApplicationSpellCheckerLanguage(
        settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString());
    view->setApplicationHighlightCurrentLine(
        settingsValue(DataStorageLayer::kApplicationHighlightCurrentLineKey).toBool());
    view->setApplicationFocusCurrentParagraph(
        settingsValue(DataStorageLayer::kApplicationFocusCurrentParagraphKey).toBool());
    view->setApplicationUseTypewriterScrolling(
        settingsValue(DataStorageLayer::kApplicationUseTypewriterScrollingKey).toBool());
    view->setApplicationReplaceThreeDotsWithEllipsis(
        settingsValue(DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey).toBool());
    view->setApplicationUseSmartQuotes(
        settingsValue(DataStorageLayer::kApplicationSmartQuotesKey).toBool());
}

void SettingsManager::Implementation::loadComponentsSettings()
{
    loadSimpleTextSettings();
    loadScreenplaySettings();
    loadComicBookSettings();
    loadAudioplaySettings();
    loadStageplaySettings();
}

void SettingsManager::Implementation::loadSimpleTextSettings()
{
    view->setSimpleTextAvailable(
        settingsValue(DataStorageLayer::kComponentsSimpleTextAvailableKey).toBool());
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey).toString();
    view->setSimpleTextEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultSimpleTextTemplate(defaultTemplate);
    //
    view->setSimpleTextNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorSceneTextLinesKey).toInt());
}

void SettingsManager::Implementation::loadScreenplaySettings()
{
    view->setScreenplayAvailable(
        settingsValue(DataStorageLayer::kComponentsScreenplayAvailableKey).toBool());
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey).toString();
    view->setScreenplayEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultScreenplayTemplate(defaultTemplate);
    view->setScreenplayEditorShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
            .toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
            .toBool());
    view->setScreenplayEditorShowDialogueNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
            .toBool());
    view->setScreenplayEditorContinueDialogue(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey).toBool());
    view->setScreenplayEditorCorrectTextOnPageBreaks(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorCorrectTextOnPageBreaksKey)
            .toBool());
    view->setScreenplayEditorUseCharactersFromText(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorUseCharactersFromTextKey)
            .toBool());
    view->setScreenplayEditorShowCharacterSuggestionsInEmptyBlock(
        settingsValue(
            DataStorageLayer::kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey)
            .toBool());
    //
    view->setScreenplayNavigatorShowBeats(
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowBeatsKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowFullBeatTextKey)
            .toBool());
    view->setScreenplayNavigatorShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey).toBool());
    view->setScreenplayNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey).toInt());
    //
    view->setScreenplayDurationType(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationTypeKey).toInt());
    view->setScreenplayDurationByPageDuration(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByPageDurationKey).toInt());
    view->setScreenplayDurationByCharactersCharacters(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersCharactersKey)
            .toInt());
    view->setScreenplayDurationByCharactersIncludeSpaces(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersIncludeSpacesKey)
            .toInt());
    view->setScreenplayDurationByCharactersDuration(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersDurationKey)
            .toInt());
    view->setScreenplayDurationConfigurablePerParagraphForAction(
        settingsValue(DataStorageLayer::
                          kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey)
            .toDouble());
    view->setScreenplayDurationConfigurablePerEvery50ForAction(
        settingsValue(DataStorageLayer::
                          kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey)
            .toDouble());
    view->setScreenplayDurationConfigurablePerParagraphForDialogue(
        settingsValue(
            DataStorageLayer::
                kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey)
            .toDouble());
    view->setScreenplayDurationConfigurablePerEvery50ForDialogue(
        settingsValue(DataStorageLayer::
                          kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey)
            .toDouble());
    view->setScreenplayDurationConfigurablePerParagraphForSceneHeading(
        settingsValue(
            DataStorageLayer::
                kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey)
            .toDouble());
    view->setScreenplayDurationConfigurablePerEvery50ForSceneHeading(
        settingsValue(
            DataStorageLayer::
                kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey)
            .toDouble());
}

void SettingsManager::Implementation::loadComicBookSettings()
{
    view->setComicBookAvailable(
        settingsValue(DataStorageLayer::kComponentsComicBookAvailableKey).toBool());
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey).toString();
    view->setComicBookEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultComicBookTemplate(defaultTemplate);
    view->setComicBookEditorUseCharactersFromText(
        settingsValue(DataStorageLayer::kComponentsComicBookEditorUseCharactersFromTextKey)
            .toBool());
    view->setComicBookEditorShowCharacterSuggestionsInEmptyBlock(
        settingsValue(
            DataStorageLayer::kComponentsComicBookEditorShowCharacterSuggestionsInEmptyBlockKey)
            .toBool());
    //
    view->setComicBookNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsComicBookNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsComicBookNavigatorSceneTextLinesKey).toInt());
}

void SettingsManager::Implementation::loadAudioplaySettings()
{
    view->setAudioplayAvailable(
        settingsValue(DataStorageLayer::kComponentsAudioplayAvailableKey).toBool());
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsAudioplayEditorDefaultTemplateKey).toString();
    view->setAudioplayEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultAudioplayTemplate(defaultTemplate);
    view->setAudioplayEditorShowBlockNumber(
        settingsValue(DataStorageLayer::kComponentsAudioplayEditorShowBlockNumbersKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsAudioplayEditorContinueBlockNumbersKey)
            .toBool());
    view->setAudioplayEditorUseCharactersFromText(
        settingsValue(DataStorageLayer::kComponentsAudioplayEditorUseCharactersFromTextKey)
            .toBool());
    view->setAudioplayEditorShowCharacterSuggestionsInEmptyBlock(
        settingsValue(
            DataStorageLayer::kComponentsAudioplayEditorShowCharacterSuggestionsInEmptyBlockKey)
            .toBool());
    //
    view->setAudioplayNavigatorShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsAudioplayNavigatorShowSceneNumberKey).toBool());
    view->setAudioplayNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsAudioplayNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsAudioplayNavigatorSceneTextLinesKey).toInt());
    //
    view->setAudioplayDurationByWordsWords(
        settingsValue(DataStorageLayer::kComponentsAudioplayDurationByWordsWordsKey).toInt());
    view->setAudioplayDurationByWordsDuration(
        settingsValue(DataStorageLayer::kComponentsAudioplayDurationByWordsDurationKey).toInt());
}

void SettingsManager::Implementation::loadStageplaySettings()
{
    view->setAudioplayAvailable(
        settingsValue(DataStorageLayer::kComponentsStageplayAvailableKey).toBool());
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsStageplayEditorDefaultTemplateKey).toString();
    view->setStageplayEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultStageplayTemplate(defaultTemplate);
    view->setStageplayEditorUseCharactersFromText(
        settingsValue(DataStorageLayer::kComponentsStageplayEditorUseCharactersFromTextKey)
            .toBool());
    view->setStageplayEditorShowCharacterSuggestionsInEmptyBlock(
        settingsValue(
            DataStorageLayer::kComponentsStageplayEditorShowCharacterSuggestionsInEmptyBlockKey)
            .toBool());
    view->setStageplayEditorShowCharacterSuggestionsInEmptyBlock(
        settingsValue(DataStorageLayer::kComponentsStageplayEditorShortcutsKey).toBool());
    //
    view->setStageplayNavigatorShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsStageplayNavigatorShowSceneNumberKey).toBool());
    view->setStageplayNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsStageplayNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsStageplayNavigatorSceneTextLinesKey).toInt());
}

void SettingsManager::Implementation::loadShortcutsSettings()
{
    using namespace BusinessLayer;

    //
    // Сформировать строку таблицы переходов между блоками для заданного типа
    //
    auto blocksJumpsModelRow = [](TextParagraphType _type) {
        const auto shortcut = ShortcutsHelper::screenplayShortcut(_type);
        const auto jumpForTab = ShortcutsHelper::screenplayJumpByTab(_type);
        const auto jumpForEnter = ShortcutsHelper::screenplayJumpByEnter(_type);
        const auto changeForTab = ShortcutsHelper::screenplayChangeByTab(_type);
        const auto changeForEnter = ShortcutsHelper::screenplayChangeByEnter(_type);

        QList<QStandardItem*> result;
        result << new QStandardItem(toDisplayString(_type)) << new QStandardItem(shortcut)
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(jumpForTab)))
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(jumpForEnter)))
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(changeForTab)))
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(changeForEnter)));
        result.first()->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return result;
    };
    //
    // ... таблика переходов между разными блоками
    //
    const int rows = 0;
    const int columns = 6;
    QStandardItemModel* blocksJumpsModel = new QStandardItemModel(rows, columns, view);
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::SceneHeading));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::SceneCharacters));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Action));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Character));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Parenthetical));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Dialogue));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Lyrics));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Transition));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Shot));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::InlineNote));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::SequenceHeading));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::ActHeading));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::UnformattedText));
    //
    // ... модель переходов между блоками, её заголовок и делегат
    //
    QStandardItemModel* blockJumpsHeaderModel = new QStandardItemModel(view);
    {
        auto columnAfterText = new QStandardItem();
        {
            auto tab = new QStandardItem("Tab");
            tab->setData(u8"\U000F0312", Qt::DecorationRole);
            columnAfterText->appendColumn({ tab });

            auto enter = new QStandardItem("Enter");
            enter->setData(u8"\U000F0311", Qt::DecorationRole);
            columnAfterText->appendColumn({ enter });
        }
        auto columnEmptyText = new QStandardItem();
        {
            auto tab = new QStandardItem("Tab");
            tab->setData(u8"\U000F0312", Qt::DecorationRole);
            columnEmptyText->appendColumn({ tab });

            auto enter = new QStandardItem("Enter");
            enter->setData(u8"\U000F0311", Qt::DecorationRole);
            columnEmptyText->appendColumn({ enter });
        }

        blockJumpsHeaderModel->setItem(0, 0, new QStandardItem());
        blockJumpsHeaderModel->setItem(0, 1, new QStandardItem());
        blockJumpsHeaderModel->setItem(0, 2, columnAfterText);
        blockJumpsHeaderModel->setItem(0, 3, columnEmptyText);
    }

    auto model = new HierarchicalModel(view);
    model->setSourceModel(blocksJumpsModel);
    model->setHeaderModel(blockJumpsHeaderModel);
    view->setShortcutsForScreenplayModel(model);
}


// ****


SettingsManager::SettingsManager(QObject* _parent, QWidget* _parentWidget,
                                 const PluginsBuilder& _pluginsBuilder)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget, _pluginsBuilder))
{
    d->loadApplicationSettings();
    d->loadComponentsSettings();
    d->loadShortcutsSettings();
    d->view->installEventFilter(this);

    connect(d->toolBar, &Ui::SettingsToolBar::backPressed, this,
            &SettingsManager::closeSettingsRequested);

    connect(d->navigator, &Ui::SettingsNavigator::applicationPressed, d->view,
            &Ui::SettingsView::showApplication);
    connect(d->navigator, &Ui::SettingsNavigator::applicationUserInterfacePressed, d->view,
            &Ui::SettingsView::showApplicationUserInterface);
    connect(d->navigator, &Ui::SettingsNavigator::applicationSaveAndBackupsPressed, d->view,
            &Ui::SettingsView::showApplicationSaveAndBackups);
    connect(d->navigator, &Ui::SettingsNavigator::applicationTextEditingPressed, d->view,
            &Ui::SettingsView::showApplicationTextEditing);
    connect(d->navigator, &Ui::SettingsNavigator::componentsPressed, d->view,
            &Ui::SettingsView::showComponents);
    connect(d->navigator, &Ui::SettingsNavigator::componentsSimpleTextPressed, d->view,
            &Ui::SettingsView::showComponentsSimpleText);
    connect(d->navigator, &Ui::SettingsNavigator::componentsScreenplayPressed, d->view,
            &Ui::SettingsView::showComponentsScreenplay);
    connect(d->navigator, &Ui::SettingsNavigator::componentsComicBookPressed, d->view,
            &Ui::SettingsView::showComponentsComicBook);
    connect(d->navigator, &Ui::SettingsNavigator::componentsAudioplayPressed, d->view,
            &Ui::SettingsView::showComponentsAudioplay);
    connect(d->navigator, &Ui::SettingsNavigator::componentsStageplayPressed, d->view,
            &Ui::SettingsView::showComponentsStageplay);
    connect(d->navigator, &Ui::SettingsNavigator::shortcutsPressed, d->view,
            &Ui::SettingsView::showShortcuts);
    connect(d->navigator, &Ui::SettingsNavigator::resetToDefaultsPressed, this, [this] {
        auto dialog = new Dialog(d->view->topLevelWidget());
        const int kCancelButtonId = 0;
        const int kYesButtonId = 1;
        dialog->showDialog({},
                           tr("Do you want to revert all changes in settings to the default state? "
                              "This action can't be undone."),
                           { { kCancelButtonId, tr("Cancel"), Dialog::RejectButton },
                             { kYesButtonId, tr("Reset"), Dialog::AcceptButton } });
        QObject::connect(dialog, &Dialog::finished, dialog,
                         [this, dialog, kCancelButtonId](const Dialog::ButtonInfo& _buttonInfo) {
                             dialog->hideDialog();

                             if (_buttonInfo.id == kCancelButtonId) {
                                 return;
                             }

                             //
                             // Вызываем сигнал о желании пользователя сбросить настройки после
                             // того, как диалог завершится, чтобы в него не прилетели события о
                             // смене дизайн системы
                             //
                             connect(dialog, &Dialog::disappeared, this,
                                     &SettingsManager::resetToDefaultsRequested,
                                     Qt::QueuedConnection);
                         });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
    });

    connect(d->view, &Ui::SettingsView::applicationLanguagePressed, this, [this, _parentWidget] {
        auto dialog = new Ui::LanguageDialog(_parentWidget);
        dialog->setCurrentLanguage(QLocale().language());
        dialog->showDialog();
        connect(dialog, &Ui::LanguageDialog::languageChanged, this,
                &SettingsManager::applicationLanguageChanged);
        connect(dialog, &Ui::LanguageDialog::languageChanged, this,
                &SettingsManager::setApplicationLanguage);
        connect(dialog, &Ui::LanguageDialog::disappeared, dialog, &Ui::LanguageDialog::deleteLater);
    });
    connect(
        d->view, &Ui::SettingsView::applicationThemePressed, this,
        [this](Ui::ApplicationTheme _theme) {
            //
            // Настроим видимость панели настройки темы приложения
            //
            if (_theme == Ui::ApplicationTheme::Custom) {
                //
                // Для кастомной темы сохраним текущее состояние, чтобы пользователь
                // мог отменить свои изменения в ней
                //
                d->themeSetupView->setSourceThemeHash(
                    settingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey).toString());
                //
                // Показываем панель параметров темы отложенно, чтобы гармонично сочеталось с
                // анимацией смены темы, в случае если она будет активирована
                //
                QTimer::singleShot(200, this, [this] { d->themeSetupView->showView(); });
            } else {
                d->themeSetupView->hideView();
            }

            if (_theme == Ui::DesignSystem::theme()) {
                return;
            }

            //
            // Уведомляем об изменении темы
            //
            setApplicationTheme(_theme);
            emit applicationThemeChanged(_theme);
        });
    connect(d->view, &Ui::SettingsView::applicationScaleFactorChanged, this,
            &SettingsManager::setApplicationScaleFactor);
    connect(d->view, &Ui::SettingsView::applicationUseAutoSaveChanged, this,
            &SettingsManager::setApplicationUseAutoSave);
    connect(d->view, &Ui::SettingsView::applicationSaveBackupsChanged, this,
            &SettingsManager::setApplicationSaveBackups);
    connect(d->view, &Ui::SettingsView::applicationBackupsFolderChanged, this,
            &SettingsManager::setApplicationBackupsFolder);
    connect(d->view, &Ui::SettingsView::applicationShowDocumentsPagesChanged, this,
            &SettingsManager::setApplicationShowDocumentsPages);
    connect(d->view, &Ui::SettingsView::applicationUseTypewriterSoundChanged, this,
            &SettingsManager::setApplicationUseTypeWriterSound);
    connect(d->view, &Ui::SettingsView::applicationUseSpellCheckerChanged, this,
            &SettingsManager::setApplicationUseSpellChecker);
    connect(d->view, &Ui::SettingsView::applicationSpellCheckerLanguageChanged, this,
            &SettingsManager::setApplicationSpellCheckerLanguage);
    connect(d->view, &Ui::SettingsView::applicationHighlightCurentLineChanged, this,
            &SettingsManager::setApplicationHighlightCurrentLine);
    connect(d->view, &Ui::SettingsView::applicationFocusCurrentParagraphChanged, this,
            &SettingsManager::setApplicationFocusCurrentParagraph);
    connect(d->view, &Ui::SettingsView::applicationUseTypewriterScrollingChanged, this,
            &SettingsManager::setApplicationUseTypewriterScrolling);
    connect(d->view, &Ui::SettingsView::applicationReplaceThreeDotsWithEllipsisChanged, this,
            &SettingsManager::setApplicationReplaceThreeDotsWithEllipsis);
    connect(d->view, &Ui::SettingsView::applicationUseSmartQuotesChanged, this,
            &SettingsManager::setApplicationUseSmartQuotes);
    //
    // ... простой редактор текста
    //
    connect(d->view, &Ui::SettingsView::simpleTextAvailableChanged, this,
            &SettingsManager::setSimpleTextAvailable);
    //
    connect(d->view, &Ui::SettingsView::simpleTextEditorDefaultTemplateChanged, this,
            &SettingsManager::setSimpleTextEditorDefaultTemplate);
    //
    connect(d->view, &Ui::SettingsView::simpleTextNavigatorShowSceneTextChanged, this,
            &SettingsManager::setSimpleTextNavigatorShowSceneText);
    //
    // ... сценарий
    //
    connect(d->view, &Ui::SettingsView::screenplayAvailableChanged, this,
            &SettingsManager::setScreenplayAvailable);
    //
    connect(d->view, &Ui::SettingsView::screenplayEditorDefaultTemplateChanged, this,
            &SettingsManager::setScreenplayEditorDefaultTemplate);
    connect(d->view, &Ui::SettingsView::screenplayEditorShowSceneNumberChanged, this,
            &SettingsManager::setScreenplayEditorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::screenplayEditorShowDialogueNumberChanged, this,
            &SettingsManager::setScreenplayEditorShowDialogueNumber);
    connect(d->view, &Ui::SettingsView::screenplayEditorContinueDialogueChanged, this,
            &SettingsManager::setScreenplayEditorContinueDialogue);
    connect(d->view, &Ui::SettingsView::screenplayEditorCorrectTextOnPageBreaksChanged, this,
            &SettingsManager::setScreenplayEditorCorrectTextOnPageBreaks);
    connect(d->view, &Ui::SettingsView::screenplayEditorUseCharactersFromTextChanged, this,
            &SettingsManager::setScreenplayEditorUseCharactersFromText);
    connect(d->view, &Ui::SettingsView::screenplayEditorShowCharacterSuggestionsInEmptyBlockChanged,
            this, &SettingsManager::setScreenplayEditorShowCharacterSuggestionsInEmptyBlock);
    //
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowBeatsChanged, this,
            &SettingsManager::setScreenplayNavigatorShowBeats);
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneNumberChanged, this,
            &SettingsManager::setScreenplayNavigatorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneTextChanged, this,
            &SettingsManager::setScreenplayNavigatorShowSceneText);
    //
    connect(d->view, &Ui::SettingsView::screenplayDurationTypeChanged, this,
            &SettingsManager::setScreenplayDurationType);
    connect(d->view, &Ui::SettingsView::screenplayDurationByPageDurationChanged, this,
            &SettingsManager::setScreenplayDurationByPageDuration);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersCharactersChanged, this,
            &SettingsManager::setScreenplayDurationByCharactersCharacters);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersIncludeSpacesChanged, this,
            &SettingsManager::setScreenplayDurationByCharactersIncludeSpaces);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersDurationChanged, this,
            &SettingsManager::setScreenplayDurationByCharactersDuration);
    connect(d->view, &Ui::SettingsView::screenplayDurationConfigurablePerParagraphForActionChanged,
            this, &SettingsManager::setScreenplayDurationConfigurablePerParagraphForAction);
    connect(d->view, &Ui::SettingsView::screenplayDurationConfigurablePerEvery50ForActionChanged,
            this, &SettingsManager::setScreenplayDurationConfigurablePerEvery50ForAction);
    connect(d->view,
            &Ui::SettingsView::screenplayDurationConfigurablePerParagraphForDialogueChanged, this,
            &SettingsManager::setScreenplayDurationConfigurablePerParagraphForDialogue);
    connect(d->view, &Ui::SettingsView::screenplayDurationConfigurablePerEvery50ForDialogueChanged,
            this, &SettingsManager::setScreenplayDurationConfigurablePerEvery50ForDialogue);
    connect(d->view,
            &Ui::SettingsView::screenplayDurationConfigurablePerParagraphForSceneHeadingChanged,
            this, &SettingsManager::setScreenplayDurationConfigurablePerParagraphForSceneHeading);
    connect(d->view,
            &Ui::SettingsView::screenplayDurationConfigurablePerEvery50ForSceneHeadingChanged, this,
            &SettingsManager::setScreenplayDurationConfigurablePerEvery50ForSceneHeading);
    //
    // ... комиксы
    //
    connect(d->view, &Ui::SettingsView::comicBookAvailableChanged, this,
            &SettingsManager::setComicBookAvailable);
    //
    connect(d->view, &Ui::SettingsView::comicBookEditorDefaultTemplateChanged, this,
            &SettingsManager::setComicBookEditorDefaultTemplate);
    connect(d->view, &Ui::SettingsView::comicBookEditorUseCharactersFromTextChanged, this,
            &SettingsManager::setComicBookEditorUseCharactersFromText);
    connect(d->view, &Ui::SettingsView::comicBookEditorShowCharacterSuggestionsInEmptyBlockChanged,
            this, &SettingsManager::setComicBookEditorShowCharacterSuggestionsInEmptyBlock);
    //
    connect(d->view, &Ui::SettingsView::comicBookNavigatorShowSceneTextChanged, this,
            &SettingsManager::setComicBookNavigatorShowSceneText);
    //
    // ... аудиопостановка
    //
    connect(d->view, &Ui::SettingsView::audioplayAvailableChanged, this,
            &SettingsManager::setAudioplayAvailable);
    //
    connect(d->view, &Ui::SettingsView::audioplayEditorDefaultTemplateChanged, this,
            &SettingsManager::setAudioplayEditorDefaultTemplate);
    connect(d->view, &Ui::SettingsView::audioplayEditorShowBlockNumberChanged, this,
            &SettingsManager::setAudioplayEditorShowBlockNumber);
    connect(d->view, &Ui::SettingsView::audioplayEditorUseCharactersFromTextChanged, this,
            &SettingsManager::setAudioplayEditorUseCharactersFromText);
    connect(d->view, &Ui::SettingsView::audioplayEditorShowCharacterSuggestionsInEmptyBlockChanged,
            this, &SettingsManager::setAudioplayEditorShowCharacterSuggestionsInEmptyBlock);
    //
    connect(d->view, &Ui::SettingsView::audioplayNavigatorShowSceneNumberChanged, this,
            &SettingsManager::setAudioplayNavigatorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::audioplayNavigatorShowSceneTextChanged, this,
            &SettingsManager::setAudioplayNavigatorShowSceneText);
    //
    connect(d->view, &Ui::SettingsView::audioplayDurationByWordsWordsChanged, this,
            &SettingsManager::setAudioplayDurationByWordsWords);
    connect(d->view, &Ui::SettingsView::audioplayDurationByWordsDurationChanged, this,
            &SettingsManager::setAudioplayDurationByWordsDuration);
    //
    // ... пьеса
    //
    connect(d->view, &Ui::SettingsView::stageplayAvailableChanged, this,
            &SettingsManager::setStageplayAvailable);
    //
    connect(d->view, &Ui::SettingsView::stageplayEditorDefaultTemplateChanged, this,
            &SettingsManager::setStageplayEditorDefaultTemplate);
    connect(d->view, &Ui::SettingsView::stageplayEditorUseCharactersFromTextChanged, this,
            &SettingsManager::setStageplayEditorUseCharactersFromText);
    connect(d->view, &Ui::SettingsView::stageplayEditorShowCharacterSuggestionsInEmptyBlockChanged,
            this, &SettingsManager::setStageplayEditorShowCharacterSuggestionsInEmptyBlock);
    //
    connect(d->view, &Ui::SettingsView::stageplayNavigatorShowSceneNumberChanged, this,
            &SettingsManager::setStageplayNavigatorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::stageplayNavigatorShowSceneTextChanged, this,
            &SettingsManager::setStageplayNavigatorShowSceneText);

    //
    // Работа с библиотекой шаблонов сценария
    //
    auto showTemplateOptionsEditor = [this] {
        d->toolBar->setCurrentWidget(d->templateOptionsManager->toolBar());
        d->navigator->setCurrentWidget(d->templateOptionsManager->navigator());
        d->view->setCurrentWidget(d->templateOptionsManager->view());
        d->templateOptionsManager->viewToolBar()->setParent(d->view);
        d->templateOptionsManager->viewToolBar()->show();
    };
    //
    // ... простой текстовый документ
    //
    connect(d->view, &Ui::SettingsView::editCurrentSimpleTextEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::SimpleText);
                d->templateOptionsManager->editTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::duplicateCurrentSimpleTextEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::SimpleText);
                d->templateOptionsManager->duplicateTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::saveToFileCurrentSimpleTextEditorTemplateRequested, this,
            [this](const QString& _templateId) {
                auto saveToFilePath = QFileDialog::getSaveFileName(
                    d->view->topLevelWidget(), tr("Choose the file to save template"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (saveToFilePath.isEmpty()) {
                    return;
                }

                if (!saveToFilePath.endsWith(ExtensionHelper::starct())) {
                    saveToFilePath.append(QString(".%1").arg(ExtensionHelper::starct()));
                }
                const auto simpleTextTemplate
                    = BusinessLayer::TemplatesFacade::simpleTextTemplate(_templateId);
                simpleTextTemplate.saveToFile(saveToFilePath);
            });
    connect(d->view, &Ui::SettingsView::removeCurrentSimpleTextEditorTemplateRequested, this,
            [](const QString& _templateId) {
                BusinessLayer::TemplatesFacade::removeSimpleTextTemplate(_templateId);
            });
    connect(d->view, &Ui::SettingsView::loadFromFileSimpleTextEditorTemplateRequested, this,
            [this] {
                const auto templateFilePath = QFileDialog::getOpenFileName(
                    d->view->topLevelWidget(), tr("Choose the file with template to load"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (templateFilePath.isEmpty()) {
                    return;
                }

                const BusinessLayer::SimpleTextTemplate simpleTextTemplate(templateFilePath);
                BusinessLayer::TemplatesFacade::saveSimpleTextTemplate(simpleTextTemplate);
            });
    //
    // ... сценарий
    //
    connect(d->view, &Ui::SettingsView::editCurrentScreenplayEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::Screenplay);
                d->templateOptionsManager->editTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::duplicateCurrentScreenplayEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::Screenplay);
                d->templateOptionsManager->duplicateTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::saveToFileCurrentScreenplayEditorTemplateRequested, this,
            [this](const QString& _templateId) {
                auto saveToFilePath = QFileDialog::getSaveFileName(
                    d->view->topLevelWidget(), tr("Choose the file to save template"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (saveToFilePath.isEmpty()) {
                    return;
                }

                if (!saveToFilePath.endsWith(ExtensionHelper::starct())) {
                    saveToFilePath.append(QString(".%1").arg(ExtensionHelper::starct()));
                }
                const auto screenplayTemplate
                    = BusinessLayer::TemplatesFacade::screenplayTemplate(_templateId);
                screenplayTemplate.saveToFile(saveToFilePath);
            });
    connect(d->view, &Ui::SettingsView::removeCurrentScreenplayEditorTemplateRequested, this,
            [](const QString& _templateId) {
                BusinessLayer::TemplatesFacade::removeScreenplayTemplate(_templateId);
            });
    connect(d->view, &Ui::SettingsView::loadFromFileScreenplayEditorTemplateRequested, this,
            [this] {
                const auto templateFilePath = QFileDialog::getOpenFileName(
                    d->view->topLevelWidget(), tr("Choose the file with template to load"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (templateFilePath.isEmpty()) {
                    return;
                }

                const BusinessLayer::ScreenplayTemplate screenplayTemplate(templateFilePath);
                BusinessLayer::TemplatesFacade::saveScreenplayTemplate(screenplayTemplate);
            });
    //
    // ... комикс
    //
    connect(d->view, &Ui::SettingsView::editCurrentComicBookEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::ComicBook);
                d->templateOptionsManager->editTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::duplicateCurrentComicBookEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::ComicBook);
                d->templateOptionsManager->duplicateTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::saveToFileCurrentComicBookEditorTemplateRequested, this,
            [this](const QString& _templateId) {
                auto saveToFilePath = QFileDialog::getSaveFileName(
                    d->view->topLevelWidget(), tr("Choose the file to save template"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (saveToFilePath.isEmpty()) {
                    return;
                }

                if (!saveToFilePath.endsWith(ExtensionHelper::starct())) {
                    saveToFilePath.append(QString(".%1").arg(ExtensionHelper::starct()));
                }
                const auto comicBookTemplate
                    = BusinessLayer::TemplatesFacade::comicBookTemplate(_templateId);
                comicBookTemplate.saveToFile(saveToFilePath);
            });
    connect(d->view, &Ui::SettingsView::removeCurrentComicBookEditorTemplateRequested, this,
            [](const QString& _templateId) {
                BusinessLayer::TemplatesFacade::removeComicBookTemplate(_templateId);
            });
    connect(d->view, &Ui::SettingsView::loadFromFileComicBookEditorTemplateRequested, this, [this] {
        const auto templateFilePath = QFileDialog::getOpenFileName(
            d->view->topLevelWidget(), tr("Choose the file with template to load"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
            DialogHelper::starcTemplateFilter());
        if (templateFilePath.isEmpty()) {
            return;
        }

        const BusinessLayer::ComicBookTemplate comicBookTemplate(templateFilePath);
        BusinessLayer::TemplatesFacade::saveComicBookTemplate(comicBookTemplate);
    });
    //
    // ... аудиопостановка
    //
    connect(d->view, &Ui::SettingsView::editCurrentAudioplayEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::Audioplay);
                d->templateOptionsManager->editTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::duplicateCurrentAudioplayEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::Audioplay);
                d->templateOptionsManager->duplicateTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::saveToFileCurrentAudioplayEditorTemplateRequested, this,
            [this](const QString& _templateId) {
                auto saveToFilePath = QFileDialog::getSaveFileName(
                    d->view->topLevelWidget(), tr("Choose the file to save template"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (saveToFilePath.isEmpty()) {
                    return;
                }

                if (!saveToFilePath.endsWith(ExtensionHelper::starct())) {
                    saveToFilePath.append(QString(".%1").arg(ExtensionHelper::starct()));
                }
                const auto audioplayTemplate
                    = BusinessLayer::TemplatesFacade::audioplayTemplate(_templateId);
                audioplayTemplate.saveToFile(saveToFilePath);
            });
    connect(d->view, &Ui::SettingsView::removeCurrentAudioplayEditorTemplateRequested, this,
            [](const QString& _templateId) {
                BusinessLayer::TemplatesFacade::removeAudioplayTemplate(_templateId);
            });
    connect(d->view, &Ui::SettingsView::loadFromFileAudioplayEditorTemplateRequested, this, [this] {
        const auto templateFilePath = QFileDialog::getOpenFileName(
            d->view->topLevelWidget(), tr("Choose the file with template to load"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
            DialogHelper::starcTemplateFilter());
        if (templateFilePath.isEmpty()) {
            return;
        }

        const BusinessLayer::AudioplayTemplate audioplayTemplate(templateFilePath);
        BusinessLayer::TemplatesFacade::saveAudioplayTemplate(audioplayTemplate);
    });
    //
    // ... пьеса
    //
    connect(d->view, &Ui::SettingsView::editCurrentStageplayEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::Stageplay);
                d->templateOptionsManager->editTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::duplicateCurrentStageplayEditorTemplateRequested, this,
            [this, showTemplateOptionsEditor](const QString& _templateId) {
                d->templateOptionsManager->setCurrentDocumentType(
                    Domain::DocumentObjectType::Stageplay);
                d->templateOptionsManager->duplicateTemplate(_templateId);
                showTemplateOptionsEditor();
            });
    connect(d->view, &Ui::SettingsView::saveToFileCurrentStageplayEditorTemplateRequested, this,
            [this](const QString& _templateId) {
                auto saveToFilePath = QFileDialog::getSaveFileName(
                    d->view->topLevelWidget(), tr("Choose the file to save template"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (saveToFilePath.isEmpty()) {
                    return;
                }

                if (!saveToFilePath.endsWith(ExtensionHelper::starct())) {
                    saveToFilePath.append(QString(".%1").arg(ExtensionHelper::starct()));
                }
                const auto stageplayTemplate
                    = BusinessLayer::TemplatesFacade::stageplayTemplate(_templateId);
                stageplayTemplate.saveToFile(saveToFilePath);
            });
    connect(d->view, &Ui::SettingsView::removeCurrentStageplayEditorTemplateRequested, this,
            [](const QString& _templateId) {
                BusinessLayer::TemplatesFacade::removeStageplayTemplate(_templateId);
            });
    connect(d->view, &Ui::SettingsView::loadFromFileStageplayEditorTemplateRequested, this, [this] {
        const auto templateFilePath = QFileDialog::getOpenFileName(
            d->view->topLevelWidget(), tr("Choose the file with template to load"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
            DialogHelper::starcTemplateFilter());
        if (templateFilePath.isEmpty()) {
            return;
        }

        const BusinessLayer::StageplayTemplate stageplayTemplate(templateFilePath);
        BusinessLayer::TemplatesFacade::saveStageplayTemplate(stageplayTemplate);
    });
    //
    // ... сам менеджер шаблона
    //
    connect(d->templateOptionsManager, &TemplateOptionsManager::closeRequested, this, [this] {
        d->toolBar->showDefaultPage();
        d->navigator->showDefaultPage();
        d->view->showDefaultPage();
        d->templateOptionsManager->viewToolBar()->hide();

        //
        // После закрытия уведомляем клиентов о том, что текущий шаблон обновился
        //
        switch (d->templateOptionsManager->currentDocumentType()) {
        default:
        case Domain::DocumentObjectType::SimpleText: {
            emit simpleTextEditorChanged(
                { DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey });
            break;
        }

        case Domain::DocumentObjectType::Screenplay: {
            emit screenplayEditorChanged(
                { DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey });
            break;
        }

        case Domain::DocumentObjectType::ComicBook: {
            emit comicBookEditorChanged(
                { DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey });
            break;
        }

        case Domain::DocumentObjectType::Audioplay: {
            emit audioplayEditorChanged(
                { DataStorageLayer::kComponentsAudioplayEditorDefaultTemplateKey });
            break;
        }

        case Domain::DocumentObjectType::Stageplay: {
            emit stageplayEditorChanged(
                { DataStorageLayer::kComponentsStageplayEditorDefaultTemplateKey });
            break;
        }
        }
    });
    connect(d->templateOptionsManager, &TemplateOptionsManager::showViewRequested, this,
            [this](QWidget* _view) {
                d->view->setCurrentWidget(_view);
                d->templateOptionsManager->viewToolBar()->raise();
            });

    //
    // Горячие клавиши
    //
    connect(d->view, &Ui::SettingsView::shortcutsForScreenplayEditorChanged, this,
            &SettingsManager::setShortcutsForScreenplayEdit);
}

SettingsManager::~SettingsManager() = default;

QWidget* SettingsManager::toolBar() const
{
    return d->toolBar;
}

QWidget* SettingsManager::navigator() const
{
    return d->navigator;
}

QWidget* SettingsManager::view() const
{
    return d->view;
}

void SettingsManager::setThemeSetupView(Ui::ThemeSetupView* _view)
{
    d->themeSetupView = _view;
    connect(d->themeSetupView, &Ui::ThemeSetupView::customThemeColorsChanged, this,
            &SettingsManager::applicationCustomThemeColorsChanged);
    connect(d->themeSetupView, &Ui::ThemeSetupView::customThemeColorsChanged, this,
            &SettingsManager::setApplicationCustomThemeColors);
}

void SettingsManager::updateScaleFactor()
{
    d->view->setApplicationScaleFactor(
        settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
}

bool SettingsManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::LanguageChange && _watched == d->view) {
        d->view->setApplicationLanguage(
            settingsValue(DataStorageLayer::kApplicationLanguagedKey).toInt());
        d->loadShortcutsSettings();
    }

    return QObject::eventFilter(_watched, _event);
}

void SettingsManager::setApplicationLanguage(int _language)
{
    setSettingsValue(DataStorageLayer::kApplicationLanguagedKey, _language);
}

void SettingsManager::setApplicationTheme(Ui::ApplicationTheme _theme)
{
    setSettingsValue(DataStorageLayer::kApplicationThemeKey, static_cast<int>(_theme));
}

void SettingsManager::setApplicationCustomThemeColors(const Ui::DesignSystem::Color& _color)
{
    setSettingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey, _color.toString());
}

void SettingsManager::setApplicationScaleFactor(qreal _scaleFactor)
{
    setSettingsValue(DataStorageLayer::kApplicationScaleFactorKey, _scaleFactor);
    emit applicationScaleFactorChanged(_scaleFactor);
}

void SettingsManager::setApplicationUseAutoSave(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseAutoSaveKey, _use);
    emit applicationUseAutoSaveChanged(_use);
}

void SettingsManager::setApplicationSaveBackups(bool _save)
{
    setSettingsValue(DataStorageLayer::kApplicationSaveBackupsKey, _save);
    emit applicationSaveBackupsChanged(_save);
}

void SettingsManager::setApplicationBackupsFolder(const QString& _path)
{
    setSettingsValue(DataStorageLayer::kApplicationBackupsFolderKey, _path);
    emit applicationBackupsFolderChanged(_path);
}

void SettingsManager::setApplicationShowDocumentsPages(bool _show)
{
    setSettingsValue(DataStorageLayer::kApplicationShowDocumentsPagesKey, _show);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
    emit audioplayEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
    emit stageplayEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
}

void SettingsManager::setApplicationUseTypeWriterSound(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey, _use);
}

void SettingsManager::setApplicationUseSpellChecker(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey, _use);
    emit applicationUseSpellCheckerChanged(_use);
}

void SettingsManager::setApplicationSpellCheckerLanguage(const QString& _languageCode)
{
    //
    // Сохраняем значение выбранного языка
    //
    setSettingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey, _languageCode);

    //
    // Проверяем установлен ли выбранный словарь
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString hunspellDictionariesFolderPath = appDataFolderPath + "/hunspell/";
    const QString affFileName = _languageCode + ".aff";
    const QString dicFileName = _languageCode + ".dic";
    //
    // Получим информацию о файлах словаря
    //
    const QFileInfo affFileInfo(hunspellDictionariesFolderPath + affFileName);
    const QFileInfo dicFileInfo(hunspellDictionariesFolderPath + dicFileName);

    //
    // Если словарь установлен, просто будем использовать его
    //
    if (affFileInfo.exists() && dicFileInfo.exists()) {
        emit applicationSpellCheckerLanguageChanged(_languageCode);
        return;
    }

    //
    // Ежели словарь не установлен, будем качать
    //
    loadSpellingDictionary(_languageCode);
}

void SettingsManager::loadSpellingDictionary(const QString& _languageCode)
{
    //
    // Добавляем идентификацию процесса загрузки словарей
    //
    const auto taskId = spellCheckerLoadingTaskId(_languageCode);
    TaskBar::addTask(taskId);
    TaskBar::setTaskTitle(taskId,
                          tr("Spelling dictionary loading") + QString(" (%1)").arg(_languageCode));
    TaskBar::setTaskProgress(taskId, 0.0);

    //
    // Создаём папку для пользовательских файлов
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString hunspellDictionariesFolderPath = appDataFolderPath + "/hunspell/";
    QDir::root().mkpath(hunspellDictionariesFolderPath);

    //
    // Начинаем загрузку файлов словаря
    //
    loadSpellingDictionaryAffFile(_languageCode);
}

void SettingsManager::loadSpellingDictionaryAffFile(const QString& _languageCode)
{
    const auto hunspellDictionariesFolderUrl
        = QString("https://starc.app/downloads/hunspell/%1/").arg(_languageCode);
    const QString affFileName = _languageCode + ".aff";

    //
    // Настраиваем загрузчик
    //
    auto dictionaryLoader = new NetworkRequest;
    connect(dictionaryLoader, &NetworkRequest::downloadProgress, this, [_languageCode](int _value) {
        //
        // Aff-файлы считаем за 10 процентов всего словаря
        //
        const qreal progress = _value * 0.1;
        TaskBar::setTaskProgress(spellCheckerLoadingTaskId(_languageCode), progress);
    });
    connect(dictionaryLoader, &NetworkRequest::downloadComplete, this,
            [this, _languageCode, affFileName](const QByteArray& _data) {
                if (_data.isEmpty()) {
                    //
                    // TODO: сообщить об ошибке
                    //
                    return;
                }

                //
                // Сохраняем файл
                //
                QFile affFile(
                    QString("%1/hunspell/%2")
                        .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                             affFileName));
                affFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
                affFile.write(_data);
                affFile.close();

                //
                // Загрузим следующий файл
                //
                loadSpellingDictionaryDicFile(_languageCode);
            });
    connect(dictionaryLoader, &NetworkRequest::error, this, [] {
        //
        // TODO: сообщить об ошибке
        //
    });
    connect(dictionaryLoader, &NetworkRequest::finished, dictionaryLoader,
            &NetworkRequest::deleteLater);

    //
    // Запускаем загрузку
    //
    dictionaryLoader->loadAsync(hunspellDictionariesFolderUrl + affFileName);
}

void SettingsManager::loadSpellingDictionaryDicFile(const QString& _languageCode)
{
    const auto hunspellDictionariesFolderUrl
        = QString("https://starc.app/downloads/hunspell/%1/").arg(_languageCode);
    const QString dicFileName = _languageCode + ".dic";

    //
    // Настраиваем загрузчик
    //
    auto dictionaryLoader = new NetworkRequest;
    connect(dictionaryLoader, &NetworkRequest::downloadProgress, this, [_languageCode](int _value) {
        //
        // Dic-файлы считаем за 90 процентов всего словаря
        //
        const qreal progress = 10 + _value * 0.9;
        TaskBar::setTaskProgress(spellCheckerLoadingTaskId(_languageCode), progress);
    });
    connect(dictionaryLoader, &NetworkRequest::downloadComplete, this,
            [this, _languageCode, dicFileName](const QByteArray& _data) {
                if (_data.isEmpty()) {
                    //
                    // TODO: сообщить об ошибке
                    //
                    return;
                }

                //
                // Сохраняем файл
                //
                QFile affFile(
                    QString("%1/hunspell/%2")
                        .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                             dicFileName));
                affFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
                affFile.write(_data);
                affFile.close();

                //
                // Cкрываем прогресс
                //
                TaskBar::finishTask(spellCheckerLoadingTaskId(_languageCode));

                //
                // Уведимим клиентов, что теперь можно использовать данный словарь
                //
                emit applicationSpellCheckerLanguageChanged(_languageCode);
            });
    connect(dictionaryLoader, &NetworkRequest::error, this, [] {
        //
        // TODO: сообщить об ошибке
        //
    });
    connect(dictionaryLoader, &NetworkRequest::finished, dictionaryLoader,
            &NetworkRequest::deleteLater);

    //
    // Запускаем загрузку
    //
    dictionaryLoader->loadAsync(hunspellDictionariesFolderUrl + dicFileName);
}

void SettingsManager::setApplicationHighlightCurrentLine(bool _highlight)
{
    setSettingsValue(DataStorageLayer::kApplicationHighlightCurrentLineKey, _highlight);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
    emit audioplayEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
    emit stageplayEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
}

void SettingsManager::setApplicationFocusCurrentParagraph(bool _focus)
{
    setSettingsValue(DataStorageLayer::kApplicationFocusCurrentParagraphKey, _focus);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
    emit audioplayEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
    emit stageplayEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
}

void SettingsManager::setApplicationUseTypewriterScrolling(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseTypewriterScrollingKey, _use);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
    emit audioplayEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
    emit stageplayEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
}

void SettingsManager::setApplicationReplaceThreeDotsWithEllipsis(bool _replace)
{
    setSettingsValue(DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey, _replace);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey });
    emit audioplayEditorChanged({ DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey });
    emit stageplayEditorChanged({ DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey });
}

void SettingsManager::setApplicationUseSmartQuotes(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationSmartQuotesKey, _use);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationSmartQuotesKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationSmartQuotesKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationSmartQuotesKey });
    emit audioplayEditorChanged({ DataStorageLayer::kApplicationSmartQuotesKey });
    emit stageplayEditorChanged({ DataStorageLayer::kApplicationSmartQuotesKey });
}

void SettingsManager::setSimpleTextAvailable(bool _available)
{
    setSettingsValue(DataStorageLayer::kComponentsSimpleTextAvailableKey, _available);
}

void SettingsManager::setSimpleTextEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultSimpleTextTemplate(_templateId);
    emit simpleTextEditorChanged(
        { DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey });
}

void SettingsManager::setSimpleTextNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorSceneTextLinesKey, _lines);
    emit simpleTextNavigatorChanged();
}

void SettingsManager::setScreenplayAvailable(bool _available)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayAvailableKey, _available);
}

void SettingsManager::setScreenplayEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultScreenplayTemplate(_templateId);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey });
}

void SettingsManager::setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey,
                     _atLeft);
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey,
                     _atRight);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey,
          DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey,
          DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey });
}

void SettingsManager::setScreenplayEditorShowDialogueNumber(bool _show)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey, _show);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey });
}

void SettingsManager::setScreenplayEditorContinueDialogue(bool _continue)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey, _continue);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey });
}

void SettingsManager::setScreenplayEditorCorrectTextOnPageBreaks(bool _correct)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorCorrectTextOnPageBreaksKey,
                     _correct);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorCorrectTextOnPageBreaksKey });
}

void SettingsManager::setScreenplayEditorUseCharactersFromText(bool _use)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorUseCharactersFromTextKey, _use);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorUseCharactersFromTextKey });
}

void SettingsManager::setScreenplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    setSettingsValue(
        DataStorageLayer::kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey,
        _show);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey });
}

void SettingsManager::setScreenplayNavigatorShowBeats(bool _show, bool _withFullText)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowBeatsKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowFullBeatTextKey,
                     _withFullText);
    emit screenplayNavigatorChanged();
}

void SettingsManager::setScreenplayNavigatorShowSceneNumber(bool _show)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey, _show);
    emit screenplayNavigatorChanged();
}

void SettingsManager::setScreenplayNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey, _lines);
    emit screenplayNavigatorChanged();
}

void SettingsManager::setScreenplayDurationType(int _type)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationTypeKey, _type);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByPageDuration(int _duration)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByPageDurationKey, _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByCharactersCharacters(int _characters)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersCharactersKey,
                     _characters);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByCharactersIncludeSpaces(bool _include)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersIncludeSpacesKey,
                     _include);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByCharactersDuration(int _duration)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersDurationKey,
                     _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationConfigurablePerParagraphForAction(qreal _duration)
{
    setSettingsValue(
        DataStorageLayer::kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey,
        _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationConfigurablePerEvery50ForAction(qreal _duration)
{
    setSettingsValue(
        DataStorageLayer::kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey,
        _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationConfigurablePerParagraphForDialogue(qreal _duration)
{
    setSettingsValue(DataStorageLayer::
                         kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey,
                     _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationConfigurablePerEvery50ForDialogue(qreal _duration)
{
    setSettingsValue(
        DataStorageLayer::kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey,
        _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationConfigurablePerParagraphForSceneHeading(qreal _duration)
{
    setSettingsValue(
        DataStorageLayer::
            kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey,
        _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationConfigurablePerEvery50ForSceneHeading(qreal _duration)
{
    setSettingsValue(
        DataStorageLayer::
            kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey,
        _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setComicBookAvailable(bool _available)
{
    setSettingsValue(DataStorageLayer::kComponentsComicBookAvailableKey, _available);
}

void SettingsManager::setComicBookEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultComicBookTemplate(_templateId);
    emit comicBookEditorChanged({ DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey });
}

void SettingsManager::setComicBookEditorUseCharactersFromText(bool _use)
{
    setSettingsValue(DataStorageLayer::kComponentsComicBookEditorUseCharactersFromTextKey, _use);
    emit comicBookEditorChanged(
        { DataStorageLayer::kComponentsComicBookEditorUseCharactersFromTextKey });
}

void SettingsManager::setComicBookEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    setSettingsValue(
        DataStorageLayer::kComponentsComicBookEditorShowCharacterSuggestionsInEmptyBlockKey, _show);
    emit comicBookEditorChanged(
        { DataStorageLayer::kComponentsComicBookEditorShowCharacterSuggestionsInEmptyBlockKey });
}

void SettingsManager::setComicBookNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsComicBookNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsComicBookNavigatorSceneTextLinesKey, _lines);
    emit comicBookNavigatorChanged();
}

void SettingsManager::setAudioplayAvailable(bool _available)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayAvailableKey, _available);
}

void SettingsManager::setAudioplayEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultAudioplayTemplate(_templateId);
    emit audioplayEditorChanged({ DataStorageLayer::kComponentsAudioplayEditorDefaultTemplateKey });
}

void SettingsManager::setAudioplayEditorShowBlockNumber(bool _show, bool _continued)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayEditorShowBlockNumbersKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsAudioplayEditorContinueBlockNumbersKey,
                     _continued);
    emit audioplayEditorChanged(
        { DataStorageLayer::kComponentsAudioplayEditorShowBlockNumbersKey,
          DataStorageLayer::kComponentsAudioplayEditorContinueBlockNumbersKey });
}

void SettingsManager::setAudioplayEditorUseCharactersFromText(bool _use)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayEditorUseCharactersFromTextKey, _use);
    emit audioplayEditorChanged(
        { DataStorageLayer::kComponentsAudioplayEditorUseCharactersFromTextKey });
}

void SettingsManager::setAudioplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    setSettingsValue(
        DataStorageLayer::kComponentsAudioplayEditorShowCharacterSuggestionsInEmptyBlockKey, _show);
    emit audioplayEditorChanged(
        { DataStorageLayer::kComponentsAudioplayEditorShowCharacterSuggestionsInEmptyBlockKey });
}

void SettingsManager::setAudioplayNavigatorShowSceneNumber(bool _show)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayNavigatorShowSceneNumberKey, _show);
    emit audioplayNavigatorChanged();
}

void SettingsManager::setAudioplayNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsAudioplayNavigatorSceneTextLinesKey, _lines);
    emit audioplayNavigatorChanged();
}

void SettingsManager::setAudioplayDurationByWordsWords(int _words)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayDurationByWordsWordsKey, _words);
    emit audioplayDurationChanged();
}

void SettingsManager::setAudioplayDurationByWordsDuration(int _duration)
{
    setSettingsValue(DataStorageLayer::kComponentsAudioplayDurationByWordsDurationKey, _duration);
    emit audioplayDurationChanged();
}

void SettingsManager::setStageplayAvailable(bool _available)
{
    setSettingsValue(DataStorageLayer::kComponentsStageplayAvailableKey, _available);
}

void SettingsManager::setStageplayEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsStageplayEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultStageplayTemplate(_templateId);
    emit stageplayEditorChanged({ DataStorageLayer::kComponentsStageplayEditorDefaultTemplateKey });
}

void SettingsManager::setStageplayEditorUseCharactersFromText(bool _use)
{
    setSettingsValue(DataStorageLayer::kComponentsStageplayEditorUseCharactersFromTextKey, _use);
    emit stageplayEditorChanged(
        { DataStorageLayer::kComponentsStageplayEditorUseCharactersFromTextKey });
}

void SettingsManager::setStageplayEditorShowCharacterSuggestionsInEmptyBlock(bool _show)
{
    setSettingsValue(
        DataStorageLayer::kComponentsStageplayEditorShowCharacterSuggestionsInEmptyBlockKey, _show);
    emit stageplayEditorChanged(
        { DataStorageLayer::kComponentsStageplayEditorShowCharacterSuggestionsInEmptyBlockKey });
}

void SettingsManager::setStageplayNavigatorShowSceneNumber(bool _show)
{
    setSettingsValue(DataStorageLayer::kComponentsStageplayNavigatorShowSceneNumberKey, _show);
    emit stageplayNavigatorChanged();
}

void SettingsManager::setStageplayNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsStageplayNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsStageplayNavigatorSceneTextLinesKey, _lines);
    emit stageplayNavigatorChanged();
}

void SettingsManager::setShortcutsForScreenplayEdit(
    const QString& _blockType, const QString& _shortcut, const QString& _jumpByTab,
    const QString& _jumpByEnter, const QString& _changeByTab, const QString& _changeByEnter)
{
    const auto blockType = BusinessLayer::textParagraphTypeFromDisplayString(_blockType);
    ShortcutsHelper::setScreenplayShortcut(blockType, _shortcut);
    ShortcutsHelper::setScreenplayJumpByTab(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_jumpByTab));
    ShortcutsHelper::setScreenplayJumpByEnter(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_jumpByEnter));
    ShortcutsHelper::setScreenplayChangeByTab(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_changeByTab));
    ShortcutsHelper::setScreenplayChangeByEnter(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_changeByEnter));
    emit screenplayEditorChanged({ DataStorageLayer::kComponentsScreenplayEditorShortcutsKey });
}

} // namespace ManagementLayer
