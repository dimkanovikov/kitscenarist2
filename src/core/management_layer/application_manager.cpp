#include "application_manager.h"

#include "content/account/account_manager.h"
#include "content/export/export_manager.h"
#include "content/import/import_manager.h"
#include "content/notifications/notifications_manager.h"
#include "content/onboarding/onboarding_manager.h"
#include "content/project/project_manager.h"
#include "content/projects/projects_manager.h"
#include "content/projects/projects_model_project_item.h"
#include "content/settings/settings_manager.h"
#include "content/writing_session/writing_session_manager.h"
#include "plugins_builder.h"

#ifdef CLOUD_SERVICE_MANAGER
#include <cloud/cloud_service_manager.h>
#endif

#include <business_layer/model/abstract_model.h>
#include <data_layer/database.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_change_object.h>
#include <domain/document_object.h>
#include <domain/starcloud_api.h>
#include <include/custom_events.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/account/connection_status_tool_bar.h>
#include <ui/application_style.h>
#include <ui/application_view.h>
#include <ui/crash_report_dialog.h>
#include <ui/design_system/design_system.h>
#include <ui/menu_view.h>
#include <ui/modules/avatar_generator/avatar_generator.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>
#include <ui/widgets/text_edit/spell_check/spell_check_text_edit.h>
#include <utils/3rd_party/WAF/Animation/Animation.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/platform_helper.h>
#include <utils/logging.h>
#include <utils/tools/backup_builder.h>
#include <utils/tools/once.h>
#include <utils/tools/run_once.h>
#include <utils/validators/email_validator.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLocale>
#include <QLockFile>
#include <QMenu>
#include <QProcess>
#include <QScopedPointer>
#include <QShortcut>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTemporaryFile>
#include <QTimer>
#include <QTranslator>
#include <QUuid>
#include <QVariant>
#include <QWidget>
#include <QtConcurrentRun>

#include <NetworkRequestLoader.h>

namespace ManagementLayer {

namespace {
/**
 * @brief Состояние приложения
 */
enum class ApplicationState { Initializing, ProjectLoading, ProjectClosing, Working, Importing };
} // namespace

class ApplicationManager::Implementation
{
public:
    explicit Implementation(ApplicationManager* _q);
    ~Implementation();

    /**
     * @brief Настроить док-меню
     */
    void initDockMenu();

    /**
     * @brief Отправить инфу о запуске приложения в статистику
     */
    void sendStartupStatistics();

    /**
     * @brief Отправить краш-репорт
     */
    void sendCrashInfo();

    /**
     * @brief Загрузить недостающие шрифты
     */
    void loadMissedFonts();

    /**
     * @brief Предложить обновиться до последней версии
     */
    void askUpdateToLatestVersion();

    /**
     * @brief Настроить параметры автосохранения
     */
    void configureAutoSave();

    /**
     * @brief Показать контент приложения
     */
    void showContent();

    /**
     * @brief Показать меню приложения
     */
    void showMenu();

    /**
     * @brief Показать личный кабинет
     */
    void showAccount();

    /**
     * @brief Показать онбординг
     */
    void showOnboarding();

    /**
     * @brief Показать страницу проектов
     */
    void showProjects();

    /**
     * @brief Показать страницу проекта
     */
    void showProject();

    /**
     * @brief Показать страницу настроек
     */
    void showSettings();

    /**
     * @brief Показать страницу статистика работы с программой
     */
    void showSessionStatistics();

    /**
     * @brief Показать предыдущий контент
     */
    void showLastContent();

    /**
     * @brief Установить перевод
     */
    void setTranslation(QLocale::Language _language);

    /**
     * @brief Установить тему
     */
    void setDesignSystemTheme(Ui::ApplicationTheme _theme);

    /**
     * @brief Установить цвета кастомной темы
     */
    void setDesignSystemCustomThemeColors(const Ui::DesignSystem::Color& _color);

    /**
     * @brief Установить коэффициент масштабирования
     */
    void setDesignSystemScaleFactor(qreal _scaleFactor);

    /**
     * @brief Задать использование компакстного режима
     */
    void setDesignSystemDensity(int _density);

    //
    // Работа с проектом
    //

    /**
     * @brief Обновить заголовок приложения
     */
    void updateWindowTitle();

    /**
     * @brief Настроить состояние приложения сохранены ли все изменения или нет
     */
    void markChangesSaved(bool _saved);

    /**
     * @brief Сохранить изменения проекта
     */
    void saveChanges();

    /**
     * @brief Если проект был изменён, но не сохранён предложить пользователю сохранить его
     * @param _callback - метод, который будет вызван, если пользователь хочет (Да),
     *        или не хочет (Нет) сохранять и не будет вызван, если пользователь передумал (Отмена)
     */
    void saveIfNeeded(std::function<void()> _callback);

    /**
     * @brief Сохранить проект как другой файл
     */
    void saveAs();

    /**
     * @brief Создать проект
     */
    void createProject();
    void createLocalProject(const QString& _projectName, const QString& _projectPath,
                            const QString& _importFilePath);
    void createRemoteProject(const QString& _projectName, const QString& _importFilePath,
                             int _teamId);

    /**
     * @brief Открыть проект по заданному пути
     */
    void openProject();
    bool openProject(const QString& _path);

    /**
     * @brief Попробовать захватить владение файлом, заблокировав его изменение другими копиями
     *        приложения
     */
    bool tryLockProject(const QString& _path);

    /**
     * @brief Попробовать захватить владение файлом, заблокировав его изменение другими копиями
     *        приложения при открытии проекта
     * @note В данном методе есть возможность зафорсить открытие проекта
     */
    bool tryLockProjectOnOpen(const QString& _path);

    /**
     * @brief Перейти к редактированию текущего проекта
     */
    void goToEditCurrentProject(bool _afterProjectCreation, const QString& _importFilePath);

    /**
     * @brief Закрыть текущий проект
     */
    void closeCurrentProject();

    /**
     * @brief Импортировать проект
     */
    void importProject();

    /**
     * @brief Экспортировать текущий документ
     */
    void exportCurrentDocument();

    /**
     * @brief Активировать полноэкранный режим
     */
    void toggleFullScreen();

    //

    /**
     * @brief Если необходимо имитировать звуки печатной машинки в зависимости от нажатой клавиши
     */
    void imitateTypewriterSound(QKeyEvent* _event) const;

    /**
     * @brief Выйти из приложения
     */
    void exit();

    //
    // Данные
    //

    ApplicationManager* q = nullptr;

    /**
     * @brief Используем для блокировки файла во время работы
     */
    QScopedPointer<QLockFile> lockFile;

    /**
     * @brief Док-меню
     */
    QMenu* dockMenu = nullptr;

    /**
     * @brief Интерфейс приложения
     */
    Ui::ApplicationView* applicationView = nullptr;
    Ui::MenuView* menuView = nullptr;
    struct LastContent {
        QWidget* toolBar = nullptr;
        QWidget* navigator = nullptr;
        QWidget* view = nullptr;
        bool isHideNavigatorButtonAvailable = false;
    } lastContent;
    Ui::ConnectionStatusToolBar* connectionStatus = nullptr;
    QPointer<Dialog> saveChangesDialog;

    /**
     * @brief Построитель плагинов редакторов
     */
    PluginsBuilder pluginsBuilder;

    /**
     * @brief Менеджеры управляющие конкретными частями приложения
     */
    QScopedPointer<AccountManager> accountManager;
    QScopedPointer<OnboardingManager> onboardingManager;
    QScopedPointer<ProjectsManager> projectsManager;
    QScopedPointer<ProjectManager> projectManager;
    QScopedPointer<ImportManager> importManager;
    QScopedPointer<ExportManager> exportManager;
    QScopedPointer<SettingsManager> settingsManager;
    QScopedPointer<WritingSessionManager> writingSessionManager;
    QScopedPointer<NotificationsManager> notificationsManager;
#ifdef CLOUD_SERVICE_MANAGER
    QScopedPointer<CloudServiceManager> cloudServiceManager;
#endif

    /**
     * @brief Таймер автосохранения проекта
     */
    QTimer autosaveTimer;

    /**
     * @brief Дата и время последней активности пользователя
     */
    QDateTime lastActivityDateTime;

    /**
     * @brief Состояние приложения в данный момент
     */
    ApplicationState state = ApplicationState::Initializing;

private:
    template<typename Manager>
    void showContent(Manager* _manager);

    template<typename Manager>
    void saveLastContent(Manager* _manager);
};

ApplicationManager::Implementation::Implementation(ApplicationManager* _q)
    : q(_q)
    , applicationView(new Ui::ApplicationView)
    , menuView(new Ui::MenuView(applicationView))
    , connectionStatus(new Ui::ConnectionStatusToolBar(applicationView))
    , accountManager(new AccountManager(nullptr, applicationView))
    , onboardingManager(new OnboardingManager(nullptr, applicationView))
    , projectsManager(new ProjectsManager(nullptr, applicationView))
    , projectManager(new ProjectManager(nullptr, applicationView, pluginsBuilder))
    , importManager(new ImportManager(nullptr, applicationView))
    , exportManager(new ExportManager(nullptr, applicationView))
    , settingsManager(new SettingsManager(nullptr, applicationView, pluginsBuilder))
    , writingSessionManager(new WritingSessionManager(nullptr, applicationView->view()))
    , notificationsManager(new NotificationsManager)
#ifdef CLOUD_SERVICE_MANAGER
    , cloudServiceManager(new CloudServiceManager)
#endif
{
    initDockMenu();

    menuView->setShowDevVersions(notificationsManager->showDevVersions());

    settingsManager->setThemeSetupView(applicationView->themeSetupView());

#ifdef Q_OS_MACOS
    new QShortcut(
        QKeySequence("Ctrl+M"), applicationView, [this] { applicationView->showMinimized(); },
        Qt::ApplicationShortcut);
#endif
}

ApplicationManager::Implementation::~Implementation()
{
    for (auto object : std::vector<QObject*>{
             applicationView,
             menuView,
             connectionStatus,
             accountManager.data(),
             onboardingManager.data(),
             projectsManager.data(),
             projectManager.data(),
             importManager.data(),
             exportManager.data(),
             writingSessionManager.data(),
#ifdef CLOUD_SERVICE_MANAGER
             cloudServiceManager.data(),
#endif
         }) {
        object->disconnect();
    }
}

void ApplicationManager::Implementation::initDockMenu()
{
#ifdef Q_OS_MAC
    //
    // Добавляем в маке возможность открытия ещё одного окна приложения
    //

    //
    // Если меню, ещё не было создано, то конфигурируем его
    //
    if (dockMenu == nullptr) {
        dockMenu = new QMenu(applicationView);
        auto openNewWindow = dockMenu->addAction(tr("Open new window"));
        connect(openNewWindow, &QAction::triggered, [=] {
            QString appPath = QApplication::applicationFilePath();
            appPath = appPath.split(".app").first();
            appPath += ".app";
            QProcess::startDetached("open", { "-na", appPath });
        });
        dockMenu->setAsDockMenu();
    }
    //
    // А если оно уже создано, то обновим переводы
    //
    else {
        constexpr int openNewWindowIndex = 0;
        dockMenu->actions().at(openNewWindowIndex)->setText(tr("Open new window"));
    }

#endif
}

void ApplicationManager::Implementation::sendStartupStatistics()
{
    //
    // Сформируем uuid для приложения, по которому будем идентифицировать данного пользователя
    //
    auto deviceUuidValue = settingsValue(DataStorageLayer::kDeviceUuidKey).toUuid();
    if (deviceUuidValue.isNull()) {
        deviceUuidValue = QUuid::createUuid();
        setSettingsValue(DataStorageLayer::kDeviceUuidKey, deviceUuidValue);
    }

    //
    // Построим ссылку, чтобы учитывать запрос на проверку обновлений
    //
    auto loader = new NetworkRequest;
    QObject::connect(loader, &NetworkRequest::finished, loader, &NetworkRequest::deleteLater);
    //
    loader->setRequestMethod(NetworkRequestMethod::Post);
    QJsonObject data;
    data["device_uuid"] = deviceUuidValue.toString();
    data["application_name"] = QApplication::applicationName();
    data["application_version"] = QApplication::applicationVersion();
    data["application_language"] = QLocale::languageToString(QLocale().language());
    data["system_type"] =
#ifdef Q_OS_WIN
        "windows"
#elif defined Q_OS_LINUX
        "linux"
#elif defined Q_OS_MAC
        "mac"
#else
        QSysInfo::kernelType()
#endif
        ;
    data["system_name"] = QSysInfo::prettyProductName();
    data["system_language"] = QLocale::languageToString(QLocale::system().language());
    data["action_name"] = "startup";
    data["action_content"] = QString();
    loader->setRawRequestData(QJsonDocument(data).toJson(), "application/json");
    loader->loadAsync("https://demo.storyapps.dev/telemetry/");
}

void ApplicationManager::Implementation::sendCrashInfo()
{
    const auto crashReportsFolderPath
        = QString("%1/crashreports")
              .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const auto crashDumps = QDir(crashReportsFolderPath).entryInfoList(QDir::Files);
    if (crashDumps.isEmpty()) {
        return;
    }

    //
    // Если есть дампы для отправки, то предложим пользователю отправить отчёт об ошибке
    //

    auto dialog = new Ui::CrashReportDialog(applicationView);
    dialog->setContactEmail(settingsValue(DataStorageLayer::kAccountEmailKey).toString());

    //
    // Настраиваем соединения диалога
    //
    connect(dialog, &Ui::CrashReportDialog::sendReportPressed, q, [this, crashDumps, dialog] {
        //
        // Сформируем пары <дамп, лог>
        //
        const auto logs = QFileInfo(Log::logFilePath()).dir().entryInfoList(QDir::Files);
        QVector<QPair<QString, QString>> dumpsAndLogs;
        for (const auto& dump : crashDumps) {
            for (const auto& log : logs) {
                if (!log.completeBaseName().startsWith(dump.completeBaseName())) {
                    continue;
                }

                dumpsAndLogs.append({ dump.absoluteFilePath(), log.absoluteFilePath() });
                break;
            }
        }

        //
        // Отправляем дампы с логами
        //
        auto appInfo = [](const QString& logPath) {
            QFile log(logPath);
            if (log.open(QIODevice::ReadOnly)) {
                QString info = log.readLine();
                info = info.remove("[I] ");
                return info;
            }
            return QString();
        };
        for (const auto& dumpAndLog : dumpsAndLogs) {
            auto loader = new NetworkRequest;
            QObject::connect(loader, &NetworkRequest::finished, loader, [loader, dumpAndLog] {
                loader->deleteLater();
                QFile::remove(dumpAndLog.first);
                QFile::remove(dumpAndLog.second);
            });
            loader->setRequestMethod(NetworkRequestMethod::Post);
            loader->addRequestAttribute("app_info", appInfo(dumpAndLog.second));
            loader->addRequestAttribute("email", dialog->contactEmail());
            loader->addRequestAttribute("username", accountManager->accountInfo().name);
            loader->addRequestAttribute("frequency", dialog->frequency());
            loader->addRequestAttribute("crashSource", dialog->crashSource());
            loader->addRequestAttribute("message", dialog->crashDetails());
            loader->addRequestAttributeFile("dump", dumpAndLog.first);
            loader->addRequestAttributeFile("log", dumpAndLog.second);
            loader->loadAsync("https://starc.app/api/app/feedback/");
        }

        //
        // Сохраняем email, если он был введён
        //
        if (!dialog->contactEmail().isEmpty() && EmailValidator::isValid(dialog->contactEmail())) {
            setSettingsValue(DataStorageLayer::kAccountEmailKey, dialog->contactEmail());
        }

        //
        // Закрываем диалог
        //
        dialog->hideDialog();
    });
    connect(dialog, &Ui::CrashReportDialog::disappeared, dialog,
            &Ui::CrashReportDialog::deleteLater);

    //
    // Отображаем диалог
    //
    dialog->showDialog();
}

void ApplicationManager::Implementation::loadMissedFonts()
{
    //
    // Сформировать список ссылок для загрузки
    //
    const auto missedFonts = Ui::DesignSystem::font().missedFonts();
    if (missedFonts.isEmpty()) {
        return;
    }

    //
    // Загрузить
    //
    const QHash<QString, QVector<QString>> fontsToUlsMap = {
        { QLatin1String("Noto Sans Arabic"),
          {
              QLatin1String("noto-sans-arabic-light.ttf"),
              QLatin1String("noto-sans-arabic-regular.ttf"),
              QLatin1String("noto-sans-arabic-medium.ttf"),
          } },
        { QLatin1String("Noto Sans SC"),
          {
              QLatin1String("noto-sans-sc-light.otf"),
              QLatin1String("noto-sans-sc-regular.otf"),
              QLatin1String("noto-sans-sc-medium.otf"),
          } },
        { QLatin1String("Noto Sans Hebrew"),
          {
              QLatin1String("noto-sans-hebrew-light.ttf"),
              QLatin1String("noto-sans-hebrew-regular.ttf"),
              QLatin1String("noto-sans-hebrew-medium.ttf"),
          } },
        { QLatin1String("Noto Sans Tamil"),
          {
              QLatin1String("noto-sans-tamil-light.ttf"),
              QLatin1String("noto-sans-tamil-regular.ttf"),
              QLatin1String("noto-sans-tamil-medium.ttf"),
          } },
        { QLatin1String("Noto Sans Telugu"),
          {
              QLatin1String("noto-sans-telugu-light.ttf"),
              QLatin1String("noto-sans-telugu-regular.ttf"),
              QLatin1String("noto-sans-telugu-medium.ttf"),
          } },
        { QLatin1String("Noto Sans KR"),
          {
              QLatin1String("noto-sans-kr-light.otf"),
              QLatin1String("noto-sans-kr-regular.otf"),
              QLatin1String("noto-sans-kr-medium.otf"),
          } },
        { QLatin1String("Noto Color Emoji"),
          {
              QLatin1String("noto-color-emoji-regular.ttf"),
          } },
    };
    QVector<QString> fontsUrls;
    for (const auto& fontFamily : missedFonts) {
        fontsUrls.append(fontsToUlsMap.value(fontFamily));
    }
    for (auto& url : fontsUrls) {
        url.prepend("https://starc.app/downloads/fonts/");
    }
    const auto taskId = "fonts-loading";
    TaskBar::addTask(taskId);
    TaskBar::setTaskTitle(taskId, tr("Loading missed fonts"));
    const auto taskStep = 100.0 / missedFonts.size();
    NetworkRequestLoader::loadAsync(
        fontsUrls, [this, taskId, taskStep](const QByteArray& _data, const QUrl& _url) {
            auto taskProgress = TaskBar::taskProgress(taskId);
            taskProgress += taskStep;
            TaskBar::setTaskProgress(taskId, taskProgress);

            //
            // Сохраняем шрифт в файл
            //
            const auto fontsFolderPath
                = QString("%1/fonts")
                      .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
            QDir::root().mkpath(fontsFolderPath);
            const auto fontFilePath = QString("%1/%2").arg(fontsFolderPath, _url.fileName());
            QFile fontFile(fontFilePath);
            if (fontFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                fontFile.write(_data);
                fontFile.close();

                //
                // ... и добавляем шрифт в библиотеку
                //
                QFontDatabase().addApplicationFont(fontFilePath);
            }

            //
            // Когда все шрифты были загружены
            //
            if (qFuzzyCompare(taskProgress, 100.0)) {
                TaskBar::finishTask(taskId);
                //
                // ... обновить дизайн систему
                //
                Ui::DesignSystem::updateLanguage();
                //
                // ... и основное окно приложения
                //
                applicationView->update();
            }
        });
}

void ApplicationManager::Implementation::askUpdateToLatestVersion()
{
    auto dialog = new Dialog(applicationView);
    const int kCancelButtonId = 0;
    const int kYesButtonId = 1;
    dialog->showDialog(
        {}, tr("Please update to the latest version if you want to use the Cloud."),
        { { kCancelButtonId, tr("Nope, I’m fine without Cloud"), Dialog::RejectButton },
          { kYesButtonId, tr("Update"), Dialog::AcceptButton } });
    QObject::connect(
        dialog, &Dialog::finished, dialog,
        [this, dialog, kCancelButtonId](const Dialog::ButtonInfo& _buttonInfo) {
            dialog->hideDialog();

            //
            // Пользователь не хочет обновляться
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Загружаем апдейт и после загрузки атоматически стартуем его
            //
            const auto taskId = Q_FUNC_INFO;
            TaskBar::addTask(taskId);
            TaskBar::setTaskTitle(taskId, tr("The last version is downloading"));
            //
            auto downloader = new NetworkRequest;
            connect(downloader, &NetworkRequest::downloadProgress, q,
                    [taskId](int _progress) { TaskBar::setTaskProgress(taskId, _progress); });
            connect(downloader, &NetworkRequest::downloadComplete, q,
                    [taskId](const QByteArray& _data, const QUrl _url) {
                        TaskBar::finishTask(taskId);

                        const QString tempDirPath
                            = QDir::toNativeSeparators(QStandardPaths::writableLocation(
#ifdef Q_OS_LINUX
                                QStandardPaths::DownloadLocation
#else
                                QStandardPaths::TempLocation
#endif
                                ));
                        auto downloadedFilePath = tempDirPath + QDir::separator() + _url.fileName();
                        QFileInfo tempFileInfo(downloadedFilePath);
                        int copyIndex = 1;
                        while (tempFileInfo.exists()) {
                            auto fileName = _url.fileName();
                            fileName.replace(".", QString(".%1.").arg(copyIndex++));
                            downloadedFilePath = tempDirPath + QDir::separator() + fileName;
                            tempFileInfo.setFile(downloadedFilePath);
                        }
                        QFile tempFile(downloadedFilePath);
                        if (tempFile.open(QIODevice::WriteOnly)) {
                            tempFile.write(_data);
                            tempFile.close();
                        }

                        const bool updateStarted =
#ifdef Q_OS_LINUX
                            //
                            // Т.к. не все линуксы умеют устанавливать AppImage, то просто открываем
                            // папку с файлом
                            //
                            PlatformHelper::showInGraphicalShell(downloadedFilePath);
#else
                            //
                            // Для остальных операционок запускаем процесс установки обновления
                            //
                            QDesktopServices::openUrl(QUrl::fromLocalFile(downloadedFilePath));
#endif
                        if (updateStarted) {
                            QCoreApplication::quit();
                        }
                    });

            downloader->loadAsync(notificationsManager->lastVersionDownloadLink());
        });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ApplicationManager::Implementation::configureAutoSave()
{
    Log::info("Configure autosaving");
    autosaveTimer.stop();
    autosaveTimer.disconnect();

    if (settingsValue(DataStorageLayer::kApplicationUseAutoSaveKey).toBool()) {
        connect(&autosaveTimer, &QTimer::timeout, q, [this] { saveChanges(); });
        autosaveTimer.start(std::chrono::minutes{ 3 });
    }
}

void ApplicationManager::Implementation::showContent()
{
    //
    // Если это первый запуск приложения, то покажем онбординг
    //
    if (settingsValue(DataStorageLayer::kApplicationConfiguredKey).toBool() == false) {
        showOnboarding();
    }
    //
    // В противном случае показываем недавние проекты
    //
    else {
        //
        // Сперва проекты нужно загрузить
        //
        // ... локальные
        //
        projectsManager->loadProjects();
        //
        // ... облачные автоматом загружаются в момент подключения к серверу
        //
        //
        // ... а затем уже отобразить
        //
        showProjects();
    }
}

void ApplicationManager::Implementation::showMenu()
{
    menuView->setFocus();
    menuView->setFixedWidth(std::max(menuView->sizeHint().width(),
                                     static_cast<int>(Ui::DesignSystem::drawer().width())));
    menuView->openMenu();
}

void ApplicationManager::Implementation::showAccount()
{
    Log::info("Show account screen");
    showContent(accountManager.data());
}

void ApplicationManager::Implementation::showOnboarding()
{
    Log::info("Show onboarding screen");
    onboardingManager->showWelcomePage();
    showContent(onboardingManager.data());
}

void ApplicationManager::Implementation::showProjects()
{
    Log::info("Show projects screen");
    menuView->checkProjects();
    showContent(projectsManager.data());
    saveLastContent(projectsManager.data());

    projectsManager->view()->setFocus();
}

void ApplicationManager::Implementation::showProject()
{
    Log::info("Show project screen");
    menuView->checkProject();
    showContent(projectManager.data());
    saveLastContent(projectManager.data());
}

void ApplicationManager::Implementation::showSettings()
{
    Log::info("Show settings screen");
    showContent(settingsManager.data());
}

void ApplicationManager::Implementation::showSessionStatistics()
{
    Log::info("Show session statistics screen");
    showContent(writingSessionManager.data());
}

void ApplicationManager::Implementation::showLastContent()
{
    if (lastContent.toolBar == nullptr || lastContent.navigator == nullptr
        || lastContent.view == nullptr) {
        return;
    }

    Log::info("Show last content");
    applicationView->showContent(lastContent.toolBar, lastContent.navigator, lastContent.view);
    applicationView->setHideNavigationButtonAvailable(lastContent.isHideNavigatorButtonAvailable);
}

void ApplicationManager::Implementation::setTranslation(QLocale::Language _language)
{
    //
    // Определим файл перевода
    //
    const QLocale::Language currentLanguage
        = _language != QLocale::AnyLanguage ? _language : QLocale::system().language();
    QString translation;
    switch (currentLanguage) {
    default:
    case QLocale::English: {
        translation = "en";
        break;
    }

    case QLocale::Arabic: {
        translation = "ar";
        break;
    }

    case QLocale::Azerbaijani: {
        translation = "az";
        break;
    }

    case QLocale::Belarusian: {
        translation = "be";
        break;
    }

    case QLocale::Catalan: {
        translation = "ca";
        break;
    }

    case QLocale::Chinese: {
        translation = "zh_CN";
        break;
    }

    case QLocale::Croatian: {
        translation = "hr";
        break;
    }

    case QLocale::Danish: {
        translation = "da_DK";
        break;
    }

    case QLocale::Dutch: {
        translation = "nl";
        break;
    }

    case QLocale::Esperanto: {
        translation = "eo";
        break;
    }

    case QLocale::French: {
        translation = "fr";
        break;
    }

    case QLocale::Galician: {
        translation = "gl_ES";
        break;
    }

    case QLocale::German: {
        translation = "de";
        break;
    }

    case QLocale::Hebrew: {
        translation = "he";
        break;
    }

    case QLocale::Hindi: {
        translation = "hi";
        break;
    }

    case QLocale::Hungarian: {
        translation = "hu";
        break;
    }

    case QLocale::Indonesian: {
        translation = "id";
        break;
    }

    case QLocale::Italian: {
        translation = "it";
        break;
    }

    case QLocale::Korean: {
        translation = "ko";
        break;
    }

    case QLocale::Persian: {
        translation = "fa";
        break;
    }

    case QLocale::Polish: {
        translation = "pl";
        break;
    }

    case QLocale::LastLanguage + 1: {
        translation = "pt_PT";
        break;
    }

    case QLocale::Portuguese: {
        translation = "pt_BR";
        break;
    }

    case QLocale::Romanian: {
        translation = "ro_RO";
        break;
    }

    case QLocale::Russian: {
        translation = "ru";
        break;
    }

    case QLocale::Slovenian: {
        translation = "sl";
        break;
    }

    case QLocale::Spanish: {
        translation = "es";
        break;
    }

    case QLocale::Filipino: {
        translation = "tl_PH";
        break;
    }

    case QLocale::Tamil: {
        translation = "ta";
        break;
    }

    case QLocale::Telugu: {
        translation = "te";
        break;
    }

    case QLocale::Turkish: {
        translation = "tr";
        break;
    }

    case QLocale::Ukrainian: {
        translation = "uk";
        break;
    }
    }

    QLocale::setDefault(QLocale(currentLanguage));

    //
    // Подключим файл переводов программы
    //
    Log::info("Setup translation for %1", translation);
    static QTranslator* appTranslator = [] {
        //
        // ... небольшой workaround для того, чтобы при запуске приложения кинуть событие о смене
        // языка
        //
        QTranslator* translator = new QTranslator;
        QApplication::installTranslator(translator);
        return translator;
    }();
    //
    // ... для DEV-версии проверим, не задан ли кастомный файл с переводом
    //
    QApplication::removeTranslator(appTranslator);
    if (QApplication::applicationVersion().contains("dev")) {
        const auto translationFilePath
            = settingsValue(DataStorageLayer::kApplicationLanguagedFileKey).toString();
        //
        // ... если файл задан, то установим его в качестве источника перевода
        //
        if (!translationFilePath.isEmpty()) {
            appTranslator->load(translationFilePath);
            QApplication::installTranslator(appTranslator);
            //
            // ... подписываемся на изменения файла, чтобы автоматически перезагрузить его
            //
            auto watcher = new QFileSystemWatcher(q);
            watcher->addPath(translationFilePath);
            connect(watcher, &QFileSystemWatcher::fileChanged, q, [this, watcher] {
                setTranslation(QLocale().language());
                watcher->deleteLater();
            });
            //
            // ... а эту переменную очищаем, чтобы далее не применялся стандартный перевод вшитый в
            //     программу
            //
            translation.clear();
        }
    }
    //
    // ... применяем файл с переводом из ресурсов приложения, если задан язык
    //
    if (!translation.isEmpty()) {
        appTranslator->load(":/translations/translation_" + translation + ".qm");
        QApplication::installTranslator(appTranslator);
    }

    //
    // Для языков, которые пишутся справа-налево настроим соответствующее выравнивание интерфейса
    //
    if (currentLanguage == QLocale::Arabic || currentLanguage == QLocale::Hebrew
        || currentLanguage == QLocale::Persian) {
        QApplication::setLayoutDirection(Qt::RightToLeft);
    } else {
        QApplication::setLayoutDirection(Qt::LeftToRight);
    }

    //
    // Настроим дизайн систему так, чтобы она использовала шрифт подходящий для используемого языка
    //
    Ui::DesignSystem::updateLanguage();
    QApplication::postEvent(q, new DesignSystemChangeEvent);

    //
    // При необходимости загрузим недостающие шрифты
    //
    loadMissedFonts();

    //
    // Настроим/обновим переводы для док-меню
    //
    initDockMenu();
}

void ApplicationManager::Implementation::setDesignSystemTheme(Ui::ApplicationTheme _theme)
{
    Log::info("Setup design system theme");
    if (state == ApplicationState::Working) {
        WAF::Animation::circleTransparentIn(applicationView, QCursor::pos(),
                                            applicationView->grab());
    }
    Ui::DesignSystem::setTheme(_theme);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::setDesignSystemCustomThemeColors(
    const Ui::DesignSystem::Color& _color)
{
    if (Ui::DesignSystem::theme() != Ui::ApplicationTheme::Custom) {
        return;
    }

    Log::info("Setup design system theme custom colors");
    if (state == ApplicationState::Working) {
        WAF::Animation::circleTransparentIn(applicationView, QCursor::pos(),
                                            applicationView->grab());
    }
    Ui::DesignSystem::setColor(_color);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::setDesignSystemScaleFactor(qreal _scaleFactor)
{
    Log::info("Setup design system scale factor");
    Ui::DesignSystem::setScaleFactor(_scaleFactor);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::setDesignSystemDensity(int _density)
{
    Log::info("Setup design system density");
    Ui::DesignSystem::setDensity(_density);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::updateWindowTitle()
{
    if (projectsManager->currentProject() == nullptr) {
        applicationView->setWindowTitle("Story Architect");
        return;
    }

    applicationView->setWindowTitle(QString("%1%2 - Story Architect %3")
                                        .arg(
#ifndef Q_OS_MAC
                                            "[*]"
#else
                                            ""
#endif
                                            )
                                        .arg(projectsManager->currentProject()->name(),
                                             (projectsManager->currentProject()->isReadOnly()
                                                  ? QString("- %1").arg(tr("Read only"))
                                                  : "")));

    if (applicationView->isWindowModified()) {
        markChangesSaved(false);
    }
}

void ApplicationManager::Implementation::markChangesSaved(bool _saved)
{
    const QString suffix
        = QApplication::translate("ManagementLayer::ApplicationManager", " - changed");
    if (_saved && applicationView->windowTitle().endsWith(suffix)) {
        applicationView->setWindowTitle(applicationView->windowTitle().remove(suffix));
    } else if (!_saved && !applicationView->windowTitle().endsWith(suffix)) {
        applicationView->setWindowTitle(applicationView->windowTitle() + suffix);
    }

    applicationView->setWindowModified(!_saved);
    menuView->markChangesSaved(_saved);
}

void ApplicationManager::Implementation::saveChanges()
{
    //
    // Избегаем рекурсии
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Сохраняем только, если приложение находится в рабочем состоянии
    //
    if (state != ApplicationState::Working) {
        return;
    }

    //
    // Сохраняем только, если есть какие-либо изменения
    //
    if (!applicationView->isWindowModified()) {
        return;
    }

    //
    // Управляющие должны сохранить все изменения
    //
    DatabaseLayer::Database::transaction();
    projectsManager->saveChanges();
    projectManager->saveChanges();
    DatabaseLayer::Database::commit();

    //
    // Если произошла ошибка сохранения, то делаем дополнительные проверки и работаем с
    // пользователем
    //
    if (DatabaseLayer::Database::hasError()) {
        //
        // Если файл, в который мы пробуем сохранять изменения существует
        //
        if (QFile::exists(DatabaseLayer::Database::currentFile())) {
            //
            // ... то у нас случилась какая-то внутренняя ошибка базы данных
            //
            StandardDialog::information(
                applicationView, tr("Saving error"),
                tr("Changes can't be written. There is an internal database error: \"%1\" "
                   "Please check, if your file exists and if you have permission to write.")
                    .arg(DatabaseLayer::Database::lastError()));

            //
            // TODO: пока хер знает, как реагировать на данную проблему...
            //       нужны реальные кейсы и пробовать что-то предпринимать
            //
        }
        //
        // Файла с базой данных не найдено
        //
        else {
            //
            // ... возможно файл был на флешке, а она отошла, или файл был переименован во время
            // работы программы
            //
            StandardDialog::information(
                applicationView, tr("Saving error"),
                tr("Changes can't be written because the story located at \"%1\" doesn't exist. "
                   "Please move the file back and retry saving.")
                    .arg(DatabaseLayer::Database::currentFile()));
        }
        return;
    }

    //
    // Если работает с теневым проектом, то экспортируем его при сохранении
    //
    if (projectsManager->currentProject()->projectType()
        == BusinessLayer::ProjectType::LocalShadow) {
        exportManager->exportDocument(projectManager->firstScriptModel(),
                                      projectsManager->currentProject()->path());
    }

    //
    // Если изменения сохранились без ошибок, то изменим статус окна на сохранение изменений
    //
    markChangesSaved(true);

    //
    // И, если необходимо создадим резервную копию закрываемого файла
    //
    if (settingsValue(DataStorageLayer::kApplicationSaveBackupsKey).toBool()) {
        QString baseBackupName;
        const auto currentProject = projectsManager->currentProject();
        if (currentProject->isRemote()) {
            //
            // Для удаленных проектов имя бекапа - имя проекта + id проекта
            // В случае, если имя удаленного проекта изменилось, то бэкапы со старым именем
            // останутся навсегда
            //
            baseBackupName
                = QString("%1 [%2]").arg(currentProject->name()).arg(currentProject->id());
        }
        QFuture<void> future = QtConcurrent::run(
            BackupBuilder::save, projectsManager->currentProject()->path(),
            settingsValue(DataStorageLayer::kApplicationBackupsFolderKey).toString(),
            baseBackupName, settingsValue(DataStorageLayer::kApplicationBackupsQtyKey).toInt());
    }
}

void ApplicationManager::Implementation::saveIfNeeded(std::function<void()> _callback)
{
    //
    // Избегаем зацикливания, проверяя, что диалог уже показан
    //
    if (!saveChangesDialog.isNull()) {
        return;
    }

    //
    // Если нет изменений, то просто переходим к следующему действию
    //
    if (!applicationView->isWindowModified()) {
        _callback();
        return;
    }

    //
    // Если проект облачный, то сохраняем без вопросов
    //
    if (projectsManager->currentProject()->isRemote()) {
        saveChanges();
        _callback();
        return;
    }

    const int kCancelButtonId = 0;
    const int kNoButtonId = 1;
    const int kYesButtonId = 2;
    saveChangesDialog = new Dialog(applicationView);
    saveChangesDialog->showDialog({}, tr("Project was modified. Save changes?"),
                                  { { kCancelButtonId, tr("Cancel"), Dialog::RejectButton },
                                    { kNoButtonId, tr("Don't save"), Dialog::NormalButton },
                                    { kYesButtonId, tr("Save"), Dialog::AcceptButton } });
    QObject::connect(
        saveChangesDialog, &Dialog::finished, saveChangesDialog,
        [this, _callback, kCancelButtonId, kNoButtonId](const Dialog::ButtonInfo& _buttonInfo) {
            saveChangesDialog->hideDialog();

            //
            // Пользователь передумал сохранять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Пользователь не хочет сохранять изменения
            //
            if (_buttonInfo.id == kNoButtonId) {
                markChangesSaved(true);
            }
            //
            // ... пользователь хочет сохранить изменения перед следующим действием
            //
            else {
                saveChanges();
            }

            _callback();
        });
    QObject::connect(saveChangesDialog, &Dialog::disappeared, saveChangesDialog,
                     &Dialog::deleteLater);

    QApplication::alert(applicationView);
}

void ApplicationManager::Implementation::saveAs()
{
    //
    // Изначально высвечивается текущее имя проекта
    //
    const auto currentProject = projectsManager->currentProject();
    QString projectPath = currentProject->path();
    switch (currentProject->projectType()) {
    //
    // Для теневых проектов добавляем расширение старка, чтобы пользователя не пугал вопрос о
    // перезаписи основного файла
    //
    case BusinessLayer::ProjectType::LocalShadow: {
        projectPath += "." + ExtensionHelper::starc();
        break;
    }

    //
    // Для удаленных проектов используем имя проекта + id проекта
    // и сохраняем в папку вновь создаваемых проектов
    //
    case BusinessLayer::ProjectType::Cloud: {
        const auto projectsFolderPath
            = settingsValue(DataStorageLayer::kProjectSaveFolderKey).toString();
        projectPath = projectsFolderPath + QDir::separator()
            + QString("%1 [%2]%3")
                  .arg(currentProject->name())
                  .arg(currentProject->id())
                  .arg(BusinessLayer::ProjectsModelProjectItem::extension());
        break;
    }

    default: {
        break;
    }
    }

    //
    // Получим имя файла для сохранения
    //
    QString saveAsProjectFilePath
        = QFileDialog::getSaveFileName(applicationView, tr("Choose file to save story"),
                                       projectPath, DialogHelper::starcProjectFilter());
    if (saveAsProjectFilePath.isEmpty()) {
        return;
    }

    //
    // Если файл выбран
    //

    //
    // Установим расширение, если не задано
    //
    if (!saveAsProjectFilePath.endsWith(BusinessLayer::ProjectsModelProjectItem::extension())) {
        saveAsProjectFilePath.append(BusinessLayer::ProjectsModelProjectItem::extension());
    }

    //
    // Если пользователь указал тот же путь, ничего не делаем
    //
    if (saveAsProjectFilePath == currentProject->path()) {
        return;
    }

    //
    // Cохраняем в новый файл
    //
    // ... если файл существовал, удалим его для удаления данных в нём
    //
    if (QFile::exists(saveAsProjectFilePath)) {
        QFile::remove(saveAsProjectFilePath);
    }
    //
    // ... скопируем текущую базу в указанный файл
    //
    const auto isCopied = QFile::copy(currentProject->realPath(), saveAsProjectFilePath);
    if (!isCopied) {
        StandardDialog::information(
            applicationView, tr("Saving error"),
            tr("Can't save the story to the file %1. Please check permissions and retry.")
                .arg(saveAsProjectFilePath));
        return;
    }

    //
    // Откроем копию текущего проекта
    //
    openProject(saveAsProjectFilePath);
}

void ApplicationManager::Implementation::createProject()
{
    auto callback = [this] { projectsManager->createProject(); };
    saveIfNeeded(callback);
}

void ApplicationManager::Implementation::createLocalProject(const QString& _projectName,
                                                            const QString& _projectPath,
                                                            const QString& _importFilePath)
{
    if (_projectPath.isEmpty()) {
        return;
    }

    //
    // Закроем текущий проект
    //
    closeCurrentProject();

    //
    // Папки, в которую пользователь хочет писать может и не быть,
    // создадим на всякий случай, чтобы его не мучать
    //
    QDir::root().mkpath(QFileInfo(_projectPath).absolutePath());

    //
    // Проверяем, можем ли мы писать в выбранный файл
    //
    QFile file(_projectPath);
    const bool canWrite = file.open(QIODevice::WriteOnly);
    file.close();

    //
    // Если невозможно записать в файл, предупреждаем пользователя и отваливаемся
    //
    if (!canWrite) {
        const QFileInfo fileInfo(_projectPath);
        QString errorMessage;
        if (!fileInfo.dir().exists()) {
            errorMessage = tr("You tried to create a project in nonexistent folder %1. "
                              "Please, choose another location for the new project.")
                               .arg(fileInfo.dir().absolutePath());
        } else if (fileInfo.exists()) {
            errorMessage
                = tr("The file can't be written. Looks like it is opened by another application. "
                     "Please close it and retry to create a new project.");
        } else {
            errorMessage
                = tr("The file can't be written. Please, check and give permissions to the app "
                     "to write into the selected folder, or choose another folder for saving a new "
                     "project.");
        }
        StandardDialog::information(applicationView, tr("Create project error"), errorMessage);
        return;
    }

    //
    // ... проверяем не открыт ли файл в другом приложении
    //
    if (!tryLockProject(_projectPath)) {
        return;
    }

    //
    // Если возможна запись в файл
    //
    // ... если в новый проект должны быть импортированы данные другого проекта,
    //     то просто создаём новый, как копию импортируемого файла
    //
    if (_importFilePath.endsWith(ExtensionHelper::starc())) {
        //
        // ... удаляем файл, который был создан при проверки возможности записи в файл
        //
        QFile::remove(QDir::toNativeSeparators(_projectPath));
        //
        // ... а затем копируем исходный файл
        //
        QFile::copy(QDir::toNativeSeparators(_importFilePath),
                    QDir::toNativeSeparators(_projectPath));
    }
    //
    // ... создаём новую базу данных в файле и делаем её текущим проектом
    //
    projectsManager->setCurrentProject(_projectPath);
    projectsManager->setCurrentProjectName(_projectName);
    //
    // ... сохраняем новый проект в списке недавних
    //
    projectsManager->saveProjects();
    //
    // ... перейдём к редактированию
    //
    constexpr auto afterProjectCreation = true;
    goToEditCurrentProject(afterProjectCreation, _importFilePath);
}

#ifdef CLOUD_SERVICE_MANAGER
void ApplicationManager::Implementation::createRemoteProject(const QString& _projectName,
                                                             const QString& _importFilePath,
                                                             int _teamId)
{
    //
    // Закроем текущий проект
    //
    closeCurrentProject();

    //
    // Т.к. взаимодействие с сервисом асинхронное, ожидаем ответа с информацией о созданном проекте
    // и переключаемся на работу с ним уже только тогда
    //
    Once::connect(cloudServiceManager.data(), &CloudServiceManager::projectCreated, q,
                  [this, _projectName, _importFilePath](const Domain::ProjectInfo& _projectInfo) {
                      if (_projectInfo.name != _projectName) {
                          return;
                      }

                      //
                      // Добавляем проект в список недавних
                      //
                      projectsManager->addOrUpdateCloudProject(_projectInfo);

                      //
                      // Если в новый проект должны быть импортированы данные другого проекта,
                      // то просто создаём новый, как копию импортируемого файла
                      //
                      if (_importFilePath.endsWith(ExtensionHelper::starc())) {
                          const auto project = projectsManager->project(_projectInfo.id);
                          QFile::copy(QDir::toNativeSeparators(_importFilePath),
                                      QDir::toNativeSeparators(project->realPath()));
                      }

                      //
                      // Переключаемся на работу с новым проектом
                      //
                      projectsManager->setCurrentProject(_projectInfo.id);
                      projectsManager->setCurrentProjectName(_projectInfo.name);
                      //
                      // ... заблокируем открытие файла в другом приложении
                      //
                      if (!tryLockProject(projectsManager->currentProject()->path())) {
                          return;
                      }
                      //
                      // ... сохраняем новый проект в списке недавних
                      //
                      projectsManager->saveProjects();
                      //
                      // ... перейдём к редактированию
                      //
                      constexpr auto afterProjectCreation = true;
                      goToEditCurrentProject(afterProjectCreation, _importFilePath);
                      //
                      // ... подписываться на документы структуры и параметров проекта нет
                      //     необходимости, т.к. это будет осуществлено на этапе их создания
                      //
                  });

    //
    // Создаём новый проект в облаке
    //
    cloudServiceManager->createProject(_projectName, _teamId);
}
#endif

void ApplicationManager::Implementation::openProject()
{
    auto callback = [this] { projectsManager->openProject(); };
    saveIfNeeded(callback);
}

bool ApplicationManager::Implementation::openProject(const QString& _path)
{
    if (_path.isEmpty()) {
        return false;
    }

    if (projectsManager->project(_path) != nullptr && projectsManager->project(_path)->isLocal()
        && !QFileInfo::exists(_path)) {
        projectsManager->hideProject(_path);
        return false;
    }

    if (projectsManager->currentProject() != nullptr
        && projectsManager->currentProject()->path() == _path) {
        showProject();
        return false;
    }

    //
    // ... закроем текущий проект
    //
    closeCurrentProject();

    //
    // ... проверяем открыт ли файл в другом приложении
    //
    if (!tryLockProjectOnOpen(_path)) {
        return false;
    }

    //
    // Если это не проект старка, то создаём временный файл проекта и в последующем импортируем
    // данные в него
    //
    QString projectFilePath = _path;
    QString importFilePath;
    if (!projectFilePath.endsWith(ExtensionHelper::starc(), Qt::CaseInsensitive)) {
        QTemporaryFile tempProject;
        tempProject.setAutoRemove(false);
        if (!tempProject.open()) {
            return false;
        }

        projectFilePath = tempProject.fileName();
        importFilePath = _path;
    }

    //
    // ... переключаемся на работу с выбранным файлом
    //
    projectsManager->setCurrentProject(_path, projectFilePath);

    //
    // ... перейдём к редактированию
    //
    constexpr auto afterProjectCreation = false;
    goToEditCurrentProject(afterProjectCreation, importFilePath);

    return true;
}

bool ApplicationManager::Implementation::tryLockProject(const QString& _path)
{
    const QFileInfo projectFileInfo(_path);
    lockFile.reset(new QLockFile(
        QString("%1/.~lock.%2").arg(projectFileInfo.absolutePath(), projectFileInfo.fileName())));
    if (!lockFile->tryLock()) {
        StandardDialog::information(applicationView, {},
                                    tr("This file can't be open at this moment, because it is "
                                       "already open in another copy of the application."));
        return false;
    }

    lockFile->setStaleLockTime(0);
    return true;
}

bool ApplicationManager::Implementation::tryLockProjectOnOpen(const QString& _path)
{
    const QFileInfo projectFileInfo(_path);
    lockFile.reset(new QLockFile(
        QString("%1/.~lock.%2").arg(projectFileInfo.absolutePath(), projectFileInfo.fileName())));
    if (!lockFile->tryLock()) {
        //
        // В некоторых случаях, после падения приложения (особенно в маке), файл блокировки не
        // освобождается, т.к. названия процессов совпадают, поэтому для кейса с открытием документа
        // даём возможность форсировать открытие путём удаления протухшего файла
        //
        auto dialog = new Dialog(applicationView->topLevelWidget());
        dialog->setContentMaximumWidth(Ui::DesignSystem::dialog().maximumWidth());
        dialog->showDialog({},
                           tr("This file can't be open at this moment, because it is already open "
                              "in another copy of the application."),
                           { { 0, StandardDialog::generateOkTerm(), Dialog::RejectButton },
                             { 1, tr("Ignore and open"), Dialog::AcceptButton } });
        connect(dialog, &Dialog::finished, q,
                [this, dialog, _path](const Dialog::ButtonInfo& _presedButton) {
                    dialog->hideDialog();
                    if (_presedButton.type == Dialog::AcceptButton) {
                        lockFile->removeStaleLockFile();
                        lockFile.reset();
                        openProject(_path);
                    }
                });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
        return false;
    }

    lockFile->setStaleLockTime(0);
    return true;
}

void ApplicationManager::Implementation::goToEditCurrentProject(bool _afterProjectCreation,
                                                                const QString& _importFilePath)
{
    state = ApplicationState::ProjectLoading;

    //
    // Установим заголовок
    //
    updateWindowTitle();

    //
    // Настроим меню
    //
    const auto currentProject = projectsManager->currentProject();
    menuView->setProjectTitle(currentProject->name());
    menuView->setProjectActionsVisible(true);

    //
    // Сохраняем тип проекта по-умолчанию
    //
    const auto projectType = static_cast<Domain::DocumentObjectType>(
        settingsValue(DataStorageLayer::kProjectTypeKey).toInt());

    //
    // Импортировать будем всё, кроме старковских файлов, т.к. данные из них копируются на
    // предыдущем шаге
    //
    const auto isImportedFromStarc = _importFilePath.endsWith(ExtensionHelper::starc());
    const auto shouldPerformImport = !_importFilePath.isEmpty() && !isImportedFromStarc;

    //
    // Если будет импорт, то сбросим умолчальный тип проекта, чтобы не создавать лишних документов
    //
    if (shouldPerformImport || !_afterProjectCreation) {
        setSettingsValue(DataStorageLayer::kProjectTypeKey,
                         static_cast<int>(Domain::DocumentObjectType::Undefined));
    }

    //
    // Загрузим данные текущего проекта
    //
    projectManager->loadCurrentProject(currentProject);
    menuView->setImportAvailable(currentProject->editingMode() == DocumentEditingMode::Edit);

    //
    // Восстанавливаем тип проекта по-умолчанию для будущих свершений
    //
    if (shouldPerformImport || !_afterProjectCreation) {
        setSettingsValue(DataStorageLayer::kProjectTypeKey, static_cast<int>(projectType));
    }

    //
    // При необходимости импортируем данные из заданного файла
    //
    if (shouldPerformImport) {
        importManager->import(_importFilePath);

        //
        // ... а после импорта пробуем повторно восстановить состояние,
        //     особенно актуально для кейса с теневым проектом
        //
        projectManager->restoreCurrentProjectState(currentProject->path());
    }

    //
    // Отобразить страницу самого проекта
    //
    showProject();

    state = ApplicationState::Working;

    //
    // Для локальных проектов доступных только для чтения, покажем соответствующее уведомление
    //
    if (currentProject->projectType() != BusinessLayer::ProjectType::Cloud
        && currentProject->isReadOnly()) {
        StandardDialog::information(
            applicationView, {},
            tr("A file you are trying to open does not have write permissions. Check out file "
               "properties and allow it to be edited. Since it isn't editable, it will be opened "
               "in a read-only mode."));
    }

    if (currentProject->projectType() == BusinessLayer::ProjectType::LocalShadow) {
        //
        // Покажем диалог с предупреждением о том, что не все функции могут работать и предложим
        // сохранить в формате старка
        //
        if (currentProject->canAskAboutSwitch()) {
            const int kNeverAskAgainButtonId = 0;
            const int kKeepButtonId = 1;
            const int kYesButtonId = 2;
            Dialog* informationDialog = new Dialog(applicationView);
            const auto projectFileSuffix = QFileInfo(currentProject->path()).suffix().toUpper();
            informationDialog->showDialog(
                tr("Do you want continue to use .%1 file format?").arg(projectFileSuffix),
                tr("Some project data cannot be saved in .%1 format. We recommend you to use Story "
                   "Architect .%2 format so all the project data will be saved properly.")
                    .arg(projectFileSuffix.toUpper(), ExtensionHelper::starc().toUpper()),
                { { kNeverAskAgainButtonId, tr("Never ask again"), Dialog::NormalButton },
                  { kKeepButtonId, tr("Keep .%1").arg(projectFileSuffix), Dialog::RejectButton },
                  { kYesButtonId, tr("Switch to .STARC"), Dialog::AcceptButton } });
            QObject::connect(informationDialog, &Dialog::finished, informationDialog,
                             [this, informationDialog, kNeverAskAgainButtonId,
                              kKeepButtonId](const Dialog::ButtonInfo& _buttonInfo) {
                                 informationDialog->hideDialog();

                                 //
                                 // Пользователь хочет использовать оригинальный формат и просит
                                 // больше его не беспокоить
                                 //
                                 if (_buttonInfo.id == kNeverAskAgainButtonId) {
                                     projectsManager->setCurrentProjectNeverAskAboutSwitch();
                                     return;
                                 }

                                 //
                                 // Пользователь хочет использовать оригинальный формат в текущей
                                 // сессии
                                 //
                                 if (_buttonInfo.id == kKeepButtonId) {
                                     return;
                                 }

                                 //
                                 // Пользователь хочет перейти на формат старка
                                 //
                                 // ... создаём новый документ и туда импортируем данные текущего
                                 //
                                 const auto project = projectsManager->currentProject();
                                 const QString projectPath
                                     = project->path() + "." + ExtensionHelper::starc();
                                 createLocalProject(project->name(), projectPath, project->path());
                                 //
                                 // ... скрываем текущий из списка недавних
                                 //
                                 projectsManager->hideProject(project->path());
                             });
            QObject::connect(informationDialog, &Dialog::disappeared, informationDialog,
                             &Dialog::deleteLater);

            QApplication::alert(applicationView);
        }
    }

    //
    // При импорте из проекта старка, очищаем всю историю изменений, т.к. она может быть неполной,
    // например если это проект из облака
    //
    if (isImportedFromStarc) {
        //
        // TODO: менять юид документа проекта, чтобы при копировании проектов не было одинаковых
        //       юидов у разных проектов
        //
        projectManager->clearChangesHistory();
    }

#ifdef CLOUD_SERVICE_MANAGER
    //
    // Для облачных проектов делаем синхронизацию офлайн изменений
    //
    if (currentProject->isRemote()) {
        projectsManager->setCurrentProjectCanBeSynced(true);
        const auto unsyncedDocuments = projectManager->unsyncedDocuments();
        for (const auto document : unsyncedDocuments) {
            cloudServiceManager->openDocument(projectsManager->currentProject()->id(), document);
        }
    }
#endif

    //
    // Запускаем писательскую сессию
    //
    writingSessionManager->startSession(currentProject->uuid(), currentProject->name());
}

void ApplicationManager::Implementation::closeCurrentProject()
{
    Log::info("Closing current project");

    if (projectsManager->currentProject() == nullptr) {
        Log::warning("Current project is not valid. Skip closing.");
        return;
    }

    Q_ASSERT(!lockFile.isNull());
    Q_ASSERT(lockFile->isLocked());

    lockFile->unlock();
    lockFile.reset();

    state = ApplicationState::ProjectClosing;

    //
    // Порядок важен
    //
#ifdef CLOUD_SERVICE_MANAGER
    if (projectsManager->currentProject()->isRemote()) {
        cloudServiceManager->closeProject(projectsManager->currentProject()->id());
    }
#endif
    projectManager->closeCurrentProject(projectsManager->currentProject()->path());
    projectsManager->closeCurrentProject();

    menuView->setProjectActionsVisible(false);

    writingSessionManager->finishSession();

    state = ApplicationState::Working;
}

void ApplicationManager::Implementation::importProject()
{
    importManager->import();
}

void ApplicationManager::Implementation::exportCurrentDocument()
{
    exportManager->exportDocument(projectManager->currentModelForExport());
}

void ApplicationManager::Implementation::toggleFullScreen()
{
    const bool isFullScreen = !applicationView->isFullScreen();

    //
    // Настраиваем видимость панели навигации
    //
    applicationView->toggleFullScreen(!isFullScreen);

    //
    // Конфигурируем редактор для отображения в полноэкранном режиме
    //
    projectManager->toggleFullScreen(isFullScreen);

    //
    // Собственно настраиваем состояние окна
    //
    const char* isMaximizedKey = "is-window-maximized";
    if (applicationView->isFullScreen()) {
        if (applicationView->property(isMaximizedKey).toBool()) {
            applicationView->showMaximized();
        } else {
            applicationView->showNormal();
        }
    } else {
        //
        // Сохраним состояние окна перед переходом в полноэкранный режим
        //
        applicationView->setProperty(isMaximizedKey,
                                     applicationView->windowState().testFlag(Qt::WindowMaximized));
        applicationView->showFullScreen();
    }
}

void ApplicationManager::Implementation::imitateTypewriterSound(QKeyEvent* _event) const
{
    //
    // Обрабатываем событие только в случае, если в виджет в фокусе можно вводить текст
    //
    if (QApplication::focusWidget()
        && !QApplication::focusWidget()->testAttribute(Qt::WA_InputMethodEnabled)) {
        return;
    }
    //
    // ... и если опция озвучивания печати включена
    //
    const auto keyboardSoundEnabled
        = settingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey).toBool();
    if (!keyboardSoundEnabled) {
        return;
    }

    auto makeSound = [this](const QString& path) {
        QSoundEffect* sound = new QSoundEffect(applicationView);
        sound->setSource(QUrl::fromLocalFile(path));
        return sound;
    };
    static auto s_returnSound = makeSound(":/audio/return");
    static auto s_spaceSound = makeSound(":/audio/space");
    static auto s_deleteSound = makeSound(":/audio/backspace");
    static QVector<QSoundEffect*> s_keySounds
        = { makeSound(":/audio/key-01"), makeSound(":/audio/key-02"), makeSound(":/audio/key-03"),
            makeSound(":/audio/key-04") };
    switch (_event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        s_returnSound->play();
        break;
    }

    case Qt::Key_Space: {
        s_spaceSound->play();
        break;
    }

    case Qt::Key_Backspace:
    case Qt::Key_Delete: {
        s_deleteSound->play();
        break;
    }

    default: {
        if (_event->text().isEmpty()) {
            break;
        }

        const int firstSoundId = 0;
        const int maxSoundId = 3;
        static int lastSoundId = firstSoundId;
        if (lastSoundId > maxSoundId) {
            lastSoundId = firstSoundId;
        }
        s_keySounds[lastSoundId++]->play();
        break;
    }
    }
}

void ApplicationManager::Implementation::exit()
{
    Log::info("Closing application");

    //
    // Скрываем интерфейс, чтобы для пользователя всё было максимально быстро
    //
    applicationView->hide();

    //
    // Закрываем текущий открытый проект
    //
    closeCurrentProject();

    //
    // Сохраняем состояние приложения
    //
    setSettingsValues(DataStorageLayer::kApplicationViewStateKey, applicationView->saveState());

    //
    // Сохраним расположение проектов
    //
    projectsManager->saveProjects();

    //
    // Выходим
    //
    QApplication::processEvents();
    QApplication::quit();
}

template<typename Manager>
void ApplicationManager::Implementation::showContent(Manager* _manager)
{
    applicationView->showContent(_manager->toolBar(), _manager->navigator(), _manager->view());
    applicationView->setHideNavigationButtonAvailable((void*)_manager
                                                      == (void*)projectManager.data());
}

template<typename Manager>
void ApplicationManager::Implementation::saveLastContent(Manager* _manager)
{
    lastContent.toolBar = _manager->toolBar();
    lastContent.navigator = _manager->navigator();
    lastContent.view = _manager->view();
    lastContent.isHideNavigatorButtonAvailable = (void*)_manager == (void*)projectManager.data();
}

// ****

ApplicationManager::ApplicationManager(QObject* _parent)
    : QObject(_parent)
    , IApplicationManager()
{
    //
    // Настроим вывод лога в консоль на всех платформах
    //
    PlatformHelper::initConsoleOutput();

    //
    // Первым делом настраиваем сбор логов
    //
    const auto logFilePath
        = QString("%1/logs/%2.log")
              .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                   PlatformHelper::systemSavebleFileName(
                       QDateTime::currentDateTime().toString(Qt::ISODateWithMs)));
    const auto loggingLevel =
#ifdef QT_DEBUG
        Log::Level::Trace;
#else
        Log::Level::Debug;
#endif
    Log::init(loggingLevel, logFilePath);


    QString applicationVersion = "0.5.5";
#if defined(DEV_BUILD) && DEV_BUILD > 0
    applicationVersion += QString(" dev %1").arg(DEV_BUILD);
#endif
    QApplication::setApplicationVersion(applicationVersion);

    Log::info("%1 version %2, %3, %4", QApplication::applicationName(),
              QApplication::applicationVersion(), QSysInfo().prettyProductName(),
              QSysInfo().currentCpuArchitecture());

    QApplication::setStyle(new ApplicationStyle(QStyleFactory::create("Fusion")));


    //
    // Загрузим шрифты в базу шрифтов программы, если их там ещё нет
    //
    Log::info("Loading fonts");
    //
    // ... встроенные в бинарник
    //
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/fonts/materialdesignicons");
    fontDatabase.addApplicationFont(":/fonts/font-awesome-brands");
    fontDatabase.addApplicationFont(":/fonts/roboto-bold");
    fontDatabase.addApplicationFont(":/fonts/roboto-light");
    fontDatabase.addApplicationFont(":/fonts/roboto-medium");
    fontDatabase.addApplicationFont(":/fonts/roboto-regular");
    fontDatabase.addApplicationFont(":/fonts/noto-sans");
    fontDatabase.addApplicationFont(":/fonts/noto-sans-light");
    fontDatabase.addApplicationFont(":/fonts/noto-sans-medium");
    //
    fontDatabase.addApplicationFont(":/fonts/arial");
    fontDatabase.addApplicationFont(":/fonts/arial-bold");
    fontDatabase.addApplicationFont(":/fonts/arial-italic");
    fontDatabase.addApplicationFont(":/fonts/arial-bold-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-new");
    fontDatabase.addApplicationFont(":/fonts/courier-new-bold");
    fontDatabase.addApplicationFont(":/fonts/courier-new-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-new-bold-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-prime");
    fontDatabase.addApplicationFont(":/fonts/courier-prime-bold");
    fontDatabase.addApplicationFont(":/fonts/courier-prime-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-prime-bold-italic");
    fontDatabase.addApplicationFont(":/fonts/mallanna-regular");
    fontDatabase.addApplicationFont(":/fonts/muktamalar-bold");
    fontDatabase.addApplicationFont(":/fonts/muktamalar-regular");
    fontDatabase.addApplicationFont(":/fonts/times-new-roman");
    fontDatabase.addApplicationFont(":/fonts/times-new-roman-bold");
    fontDatabase.addApplicationFont(":/fonts/times-new-roman-italic");
    fontDatabase.addApplicationFont(":/fonts/times-new-roman-bold-italic");
    //
    fontDatabase.addApplicationFont(":/fonts/montserrat-regular");
    fontDatabase.addApplicationFont(":/fonts/montserrat-bold");
    fontDatabase.addApplicationFont(":/fonts/montserrat-italic");
    fontDatabase.addApplicationFont(":/fonts/montserrat-bold-italic");
    fontDatabase.addApplicationFont(":/fonts/sf-movie-poster");
    //
    // ... скаченные
    //
    const auto fontsFolderPath
        = QString("%1/fonts")
              .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const auto fonts = QDir(fontsFolderPath).entryInfoList(QDir::Files);
    for (const auto& font : fonts) {
        fontDatabase.addApplicationFont(font.absoluteFilePath());
    }

    //
    // Инициилизируем данные после подгрузки шрифтов, чтобы они сразу подхватились системой
    //
    d.reset(new Implementation(this));

    initConnections();
}

ApplicationManager::~ApplicationManager()
{
    //
    // Удаляем старые логи
    //
    const auto logsDirPath
        = QString("%1/logs").arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const auto logsFiles = QDir(logsDirPath).entryInfoList();
    //
    // ... храним логи недельной давности
    //
    const auto dateBorder = QDateTime::currentDateTime().addDays(-7);
    for (const auto& logFile : logsFiles) {
        if (logFile.lastModified() >= dateBorder) {
            continue;
        }

        QFile::remove(logFile.absoluteFilePath());
    }

    //
    // Удаляем крашдампы, которые по какой-либо причине не были удалены ранее
    //
    const auto crashReportsFolderPath
        = QString("%1/crashreports")
              .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const auto crashDumps = QDir(crashReportsFolderPath).entryInfoList(QDir::Files);
    for (const auto& crashDump : crashDumps) {
        QFile::remove(crashDump.absoluteFilePath());
    }
}

QString ApplicationManager::logFilePath() const
{
    return Log::logFilePath();
}

void ApplicationManager::exec(const QString& _fileToOpenPath)
{
    Log::info("Starting the application");

    //
    // Самое главное - настроить заголовок!
    //
    d->updateWindowTitle();

    //
    // Сразу регистрируем панель уведомлений
    //
    TaskBar::registerTaskBar(d->applicationView, {}, {}, {});

    //
    // Пробуем загрузить геометрию и состояние приложения
    //
    d->setTranslation(
        settingsValue(DataStorageLayer::kApplicationLanguagedKey).value<QLocale::Language>());
    d->setDesignSystemTheme(static_cast<Ui::ApplicationTheme>(
        settingsValue(DataStorageLayer::kApplicationThemeKey).toInt()));
    d->setDesignSystemCustomThemeColors(Ui::DesignSystem::Color(
        settingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey).toString()));
    d->setDesignSystemScaleFactor(
        settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
    d->setDesignSystemDensity(settingsValue(DataStorageLayer::kApplicationDensityKey).toInt());
    d->applicationView->restoreState(
        settingsValue(DataStorageLayer::kApplicationConfiguredKey).toBool(),
        settingsValues(DataStorageLayer::kApplicationViewStateKey));

    //
    // Покажем интерфейс
    //
    Log::info("Show application window");
    d->applicationView->show();

    //
    // Осуществляем остальную настройку и показываем содержимое, после того, как на экране
    // отобразится приложение, чтобы у пользователя возник эффект моментального запуска
    //
    QMetaObject::invokeMethod(
        this,
        [this, _fileToOpenPath] {
            Log::info("Make startup checks");

#ifdef CLOUD_SERVICE_MANAGER
            //
            // Запуск облачного сервиса
            //
            d->cloudServiceManager->start();
#endif

            //
            // Настройка
            //
            d->configureAutoSave();

            //
            // Отображение
            //
            d->showContent();

            //
            // Открыть заданный проект
            //
            openProject(_fileToOpenPath);

            //
            // Отправим запрос в статистику
            //
            d->sendStartupStatistics();

            //
            // Покажем диалог отправки сообщения об ошибке
            //
            d->sendCrashInfo();

            //
            // Загружаем недостающие шрифты
            //
            d->loadMissedFonts();

            //
            // Переводим состояние приложение в рабочий режим
            //
            d->state = ApplicationState::Working;
        },
        Qt::QueuedConnection);
}

bool ApplicationManager::openProject(const QString& _path)
{
    return d->openProject(_path);
}

bool ApplicationManager::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::IdleEvent): {
        //
        // Сохраняем только если пользователь желает делать это автоматически
        //
        if (d->autosaveTimer.isActive()) {
            d->saveChanges();
        }
        //
        // Уведомляем заинтересованные менеджеры о том, что перешли в состояние простоя
        //
        for (auto manager : { d->projectManager.data() }) {
            QApplication::sendEvent(manager, _event);
        }
        //
        // Если был долгий простой, то завершаем текущую сессию, пользователь ушёл
        //
        if (auto event = static_cast<IdleEvent*>(_event); event->isLongIdle) {
            //
            // ... текущая сессия завершилась во время последней активности в приложении
            //
            d->writingSessionManager->splitSession(d->lastActivityDateTime);
        }
        //
        // А если был маленький простой после активности персонажа, запоминаем этот момент,
        // как дату и время последней активности пользователя
        //
        else {
            d->lastActivityDateTime = QDateTime::currentDateTimeUtc();
        }

        _event->accept();
        return true;
    }

    case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
        //
        // Уведомляем все виджеты о том, что сменилась дизайн система
        //
        const auto widgets = d->applicationView->findChildren<QWidget*>();
        for (auto widget : widgets) {
            QApplication::sendEvent(widget, _event);
        }
        QApplication::sendEvent(d->applicationView, _event);

        _event->accept();
        return true;
    }

    case static_cast<QEvent::Type>(EventType::SpellingChangeEvent): {
        //
        // Уведомляем все редакторы текста о том, что сменились опции проверки орфографии
        //
        const auto textEdits = d->applicationView->findChildren<SpellCheckTextEdit*>();
        for (auto textEdit : textEdits) {
            QApplication::sendEvent(textEdit, _event);
        }
        const auto scalableWrappers = d->applicationView->findChildren<ScalableWrapper*>();
        for (auto scalableWrapper : scalableWrappers) {
            QApplication::sendEvent(scalableWrapper, _event);
        }
        QApplication::sendEvent(d->applicationView, _event);

        _event->accept();
        return true;
    }

    case QEvent::KeyPress: {
        const auto keyEvent = static_cast<QKeyEvent*>(_event);

        //
        // Музицируем
        //
        d->imitateTypewriterSound(keyEvent);

        //
        // Сохраняем стату
        //
        d->writingSessionManager->addKeyPressEvent(keyEvent);

        return false;
    }

    case static_cast<QEvent::Type>(EventType::FocusChangeEvent): {
        //
        // Посылаем событие о смене фокуса в менеджер проекта, чтобы определить текущее активное
        // представление
        //
        QApplication::sendEvent(d->projectManager.data(), _event);

        return false;
    }

    default: {
        return QObject::event(_event);
    }
    }
}

void ApplicationManager::initConnections()
{
    //
    // Горячие клавиши
    //
    QShortcut* saveShortcut = new QShortcut(QKeySequence::Save, d->applicationView);
    saveShortcut->setContext(Qt::ApplicationShortcut);
    connect(saveShortcut, &QShortcut::activated, this, [this] { d->saveChanges(); });
    //
    QShortcut* importShortcut = new QShortcut(QKeySequence("Alt+I"), d->applicationView);
    importShortcut->setContext(Qt::ApplicationShortcut);
    connect(importShortcut, &QShortcut::activated, this, [this] { d->importProject(); });
    //
    QShortcut* exportShortcut = new QShortcut(QKeySequence("Alt+E"), d->applicationView);
    exportShortcut->setContext(Qt::ApplicationShortcut);
    connect(exportShortcut, &QShortcut::activated, this, [this] { d->exportCurrentDocument(); });
    //
    QShortcut* fullScreenShortcut = new QShortcut(QKeySequence::FullScreen, d->applicationView);
    fullScreenShortcut->setContext(Qt::ApplicationShortcut);
    connect(fullScreenShortcut, &QShortcut::activated, this, [this] { d->toggleFullScreen(); });

    //
    // Представление приложения
    //
    connect(d->applicationView, &Ui::ApplicationView::turnOffFullScreenRequested, this,
            [this] { d->toggleFullScreen(); });
    connect(d->applicationView, &Ui::ApplicationView::closeRequested, this, [this] {
        auto callback = [this] { d->exit(); };
        d->saveIfNeeded(callback);
    });

    //
    // Представление меню
    //
    connect(d->menuView, &Ui::MenuView::signInPressed, d->accountManager.data(),
            &AccountManager::signIn);
    connect(d->menuView, &Ui::MenuView::accountPressed, this, [this] { d->showAccount(); });
    connect(d->menuView, &Ui::MenuView::projectsPressed, this, [this] { d->showProjects(); });
    connect(d->menuView, &Ui::MenuView::createProjectPressed, this, [this] { d->createProject(); });
    connect(d->menuView, &Ui::MenuView::openProjectPressed, this, [this] { d->openProject(); });
    connect(d->menuView, &Ui::MenuView::projectPressed, this, [this] { d->showProject(); });
    connect(d->menuView, &Ui::MenuView::saveProjectChangesPressed, this,
            [this] { d->saveChanges(); });
    connect(d->menuView, &Ui::MenuView::saveProjectAsPressed, this, [this] {
        auto callback = [this] { d->saveAs(); };
        d->saveIfNeeded(callback);
    });
    connect(d->menuView, &Ui::MenuView::importPressed, this, [this] { d->importProject(); });
    connect(d->menuView, &Ui::MenuView::exportCurrentDocumentPressed, this,
            [this] { d->exportCurrentDocument(); });
    connect(d->menuView, &Ui::MenuView::fullscreenPressed, this, [this] { d->toggleFullScreen(); });
    connect(d->menuView, &Ui::MenuView::settingsPressed, this, [this] { d->showSettings(); });
    //
    connect(d->menuView, &Ui::MenuView::writingStatisticsPressed, this, [this] {
#ifdef CLOUD_SERVICE_MANAGER
        d->cloudServiceManager->askSessionStatistics(
            d->writingSessionManager->sessionStatisticsLastSyncDateTime());
#endif
        d->showSessionStatistics();
    });
    connect(d->menuView, &Ui::MenuView::writingSprintPressed, this,
            [this] { d->writingSessionManager->showSprintPanel(); });
    //
    connect(d->menuView, &Ui::MenuView::renewProPressed, d->accountManager.data(),
            &AccountManager::renewPro);
    connect(d->menuView, &Ui::MenuView::renewTeamPressed, d->accountManager.data(),
            &AccountManager::renewTeam);

    //
    // Менеджер посадки
    //
    connect(d->onboardingManager.data(), &OnboardingManager::languageChanged, this,
            [this](QLocale::Language _language) { d->setTranslation(_language); });
    connect(d->onboardingManager.data(), &OnboardingManager::themeChanged, this,
            [this](Ui::ApplicationTheme _theme) { d->setDesignSystemTheme(_theme); });
    connect(d->onboardingManager.data(), &OnboardingManager::scaleFactorChanged, this,
            [this](qreal _scaleFactor) {
                d->setDesignSystemScaleFactor(_scaleFactor);
                d->settingsManager->updateScaleFactor();
            });
    connect(d->onboardingManager.data(), &OnboardingManager::finished, this, [this] {
        setSettingsValue(DataStorageLayer::kApplicationConfiguredKey, true);
        setSettingsValue(DataStorageLayer::kApplicationLanguagedKey, QLocale::system().language());
        setSettingsValue(DataStorageLayer::kApplicationThemeKey,
                         static_cast<int>(Ui::DesignSystem::theme()));
        setSettingsValue(DataStorageLayer::kApplicationScaleFactorKey,
                         Ui::DesignSystem::scaleFactor());

        d->showContent();
        d->applicationView->slideViewOut();
    });

    //
    // Менеджер аккаунта
    //
    connect(d->accountManager.data(), &AccountManager::showAccountRequested, this,
            [this] { d->showAccount(); });
    connect(d->accountManager.data(), &AccountManager::closeAccountRequested, this,
            [this] { d->showLastContent(); });

    //
    // Менеджер проектов
    //
    connect(d->projectsManager.data(), &ProjectsManager::menuRequested, this,
            [this] { d->showMenu(); });
    connect(d->projectsManager.data(), &ProjectsManager::signInRequested, d->accountManager.data(),
            &AccountManager::signIn);
    connect(d->projectsManager.data(), &ProjectsManager::renewTeamSubscriptionRequested,
            d->accountManager.data(), &AccountManager::renewTeam);
    connect(d->projectsManager.data(), &ProjectsManager::createProjectRequested, this,
            [this] { d->createProject(); });
    connect(d->projectsManager.data(), &ProjectsManager::createLocalProjectRequested, this,
            [this](const QString& _projectName, const QString& _projectPath,
                   const QString& _importFilePath) {
                d->createLocalProject(_projectName, _projectPath, _importFilePath);
            });
    connect(d->projectsManager.data(), &ProjectsManager::openProjectRequested, this,
            [this] { d->openProject(); });
    connect(d->projectsManager.data(), &ProjectsManager::openLocalProjectRequested, this,
            [this](const QString& _path) {
                if (d->projectsManager->currentProject() != nullptr
                    && d->projectsManager->currentProject()->path() == _path) {
                    d->showProject();
                    return;
                }

                auto callback = [this, _path] { openProject(_path); };
                d->saveIfNeeded(callback);
            });
    connect(d->projectsManager.data(), &ProjectsManager::closeCurrentProjectRequested, this,
            [this] {
                d->closeCurrentProject();
                d->markChangesSaved(true);
                d->updateWindowTitle();
            });

    //
    // Менеджер проекта
    //
    connect(d->projectManager.data(), &ProjectManager::menuRequested, this,
            [this] { d->showMenu(); });
    connect(d->projectManager.data(), &ProjectManager::upgradeToProRequested,
            d->accountManager.data(), &AccountManager::upgradeAccountToPro);
    connect(d->projectManager.data(), &ProjectManager::upgradeToTeamRequested,
            d->accountManager.data(), &AccountManager::upgradeAccountToTeam);
    connect(d->projectManager.data(), &ProjectManager::buyCreditsRequested,
            d->accountManager.data(), &AccountManager::buyCredits);
    connect(d->projectManager.data(), &ProjectManager::contentsChanged, this,
            [this] { d->markChangesSaved(false); });
    connect(d->projectManager.data(), &ProjectManager::projectUuidChanged,
            d->projectsManager.data(), &ProjectsManager::setCurrentProjectUuid);
    connect(d->projectManager.data(), &ProjectManager::projectNameChanged, this,
            [this](const QString& _name) {
                d->projectsManager->setCurrentProjectName(_name);
                d->menuView->setProjectTitle(_name);
                d->updateWindowTitle();
            });
    connect(d->projectManager.data(), &ProjectManager::projectLoglineChanged,
            d->projectsManager.data(), &ProjectsManager::setCurrentProjectLogline);
    connect(d->projectManager.data(), &ProjectManager::projectCoverChanged,
            d->projectsManager.data(), &ProjectsManager::setCurrentProjectCover);
    connect(d->projectManager.data(), &ProjectManager::currentModelChanged, this,
            [this](BusinessLayer::AbstractModel* _model) {
                d->menuView->setCurrentDocumentExportAvailable(
                    d->exportManager->canExportDocument(_model));
            });

    //
    // Менеджер импорта
    //
    connect(d->importManager.data(), &ImportManager::characterImported, d->projectManager.data(),
            &ProjectManager::addCharacter);
    connect(d->importManager.data(), &ImportManager::locationImported, d->projectManager.data(),
            &ProjectManager::addLocation);
    connect(d->importManager.data(), &ImportManager::screenplayImported, d->projectManager.data(),
            &ProjectManager::addScreenplay);

    //
    // Менеджер настроек
    //
    connect(d->settingsManager.data(), &SettingsManager::closeSettingsRequested, this,
            [this] { d->showLastContent(); });
    connect(d->settingsManager.data(), &SettingsManager::applicationLanguageChanged, this,
            [this](QLocale::Language _language) { d->setTranslation(_language); });
    connect(d->settingsManager.data(), &SettingsManager::applicationLanguageFileChanged, this,
            [this] { d->setTranslation(QLocale().language()); });
    //
    auto postSpellingChangeEvent = [this] {
        const auto useSpellChecker
            = settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool();
        const auto spellingLanguage
            = settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString();
        QApplication::postEvent(this, new SpellingChangeEvent(useSpellChecker, spellingLanguage));
    };
    connect(d->settingsManager.data(), &SettingsManager::applicationUseSpellCheckerChanged, this,
            postSpellingChangeEvent);
    connect(d->settingsManager.data(), &SettingsManager::applicationSpellCheckerLanguageChanged,
            this, postSpellingChangeEvent);
    //
    connect(
        d->settingsManager.data(), &SettingsManager::applicationThemeChanged, this,
        [this](Ui::ApplicationTheme _theme) {
            d->setDesignSystemTheme(_theme);
            //
            // ... если применяется кастомная тема, то нужно загрузить её цвета
            //
            if (_theme == Ui::ApplicationTheme::Custom) {
                d->setDesignSystemCustomThemeColors(Ui::DesignSystem::Color(
                    settingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey).toString()));
            }
        });
    connect(d->settingsManager.data(), &SettingsManager::applicationCustomThemeColorsChanged, this,
            [this](const Ui::DesignSystem::Color& _color) {
                d->setDesignSystemCustomThemeColors(_color);
            });
    connect(d->settingsManager.data(), &SettingsManager::applicationScaleFactorChanged, this,
            [this](qreal _scaleFactor) { d->setDesignSystemScaleFactor(_scaleFactor); });
    connect(d->settingsManager.data(), &SettingsManager::applicationDensityChanged, this,
            [this](int _density) { d->setDesignSystemDensity(_density); });
    connect(d->settingsManager.data(), &SettingsManager::applicationUseAutoSaveChanged, this,
            [this] { d->configureAutoSave(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::simpleTextEditorChanged, this,
            [this](const QStringList& _changedSettingsKeys) {
                d->projectManager->reconfigureSimpleTextEditor(_changedSettingsKeys);
            });
    connect(d->settingsManager.data(), &SettingsManager::simpleTextNavigatorChanged, this,
            [this] { d->projectManager->reconfigureSimpleTextNavigator(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::screenplayEditorChanged, this,
            [this](const QStringList& _changedSettingsKeys) {
                d->projectManager->reconfigureScreenplayEditor(_changedSettingsKeys);
            });
    connect(d->settingsManager.data(), &SettingsManager::screenplayNavigatorChanged, this,
            [this] { d->projectManager->reconfigureScreenplayNavigator(); });
    connect(d->settingsManager.data(), &SettingsManager::screenplayDurationChanged, this,
            [this] { d->projectManager->reconfigureScreenplayDuration(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::comicBookEditorChanged, this,
            [this](const QStringList& _changedSettingsKeys) {
                d->projectManager->reconfigureComicBookEditor(_changedSettingsKeys);
            });
    connect(d->settingsManager.data(), &SettingsManager::comicBookNavigatorChanged, this,
            [this] { d->projectManager->reconfigureComicBookNavigator(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::audioplayEditorChanged, this,
            [this](const QStringList& _changedSettingsKeys) {
                d->projectManager->reconfigureAudioplayEditor(_changedSettingsKeys);
            });
    connect(d->settingsManager.data(), &SettingsManager::audioplayNavigatorChanged, this,
            [this] { d->projectManager->reconfigureAudioplayNavigator(); });
    connect(d->settingsManager.data(), &SettingsManager::audioplayDurationChanged, this,
            [this] { d->projectManager->reconfigureAudioplayDuration(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::stageplayEditorChanged, this,
            [this](const QStringList& _changedSettingsKeys) {
                d->projectManager->reconfigureStageplayEditor(_changedSettingsKeys);
            });
    connect(d->settingsManager.data(), &SettingsManager::stageplayNavigatorChanged, this,
            [this] { d->projectManager->reconfigureStageplayNavigator(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::novelEditorChanged, this,
            [this](const QStringList& _changedSettingsKeys) {
                d->projectManager->reconfigureNovelEditor(_changedSettingsKeys);
            });
    connect(d->settingsManager.data(), &SettingsManager::novelNavigatorChanged, this,
            [this] { d->projectManager->reconfigureNovelNavigator(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::resetToDefaultsRequested, this, [this] {
        //
        // Если пользователь хочет сбросить все настройки, закроем текущий проект
        //
        d->closeCurrentProject();
        //
        // ... сбросим все настройки
        //
        DataStorageLayer::StorageFacade::settingsStorage()->resetToDefaults();
        //
        // Перезапустим приложение
        //
        QApplication::quit();
        if (QApplication::arguments().size() > 0) {
            QProcess::startDetached(QApplication::arguments().constFirst(), {});
        }
    });

    //
    // Менеджер статистики по сессиям работы с программой
    //
    connect(d->writingSessionManager.data(),
            &WritingSessionManager::closeSessionStatisticsRequested, this,
            [this] { d->showLastContent(); });

#ifdef CLOUD_SERVICE_MANAGER
    //
    // Менеджер облака
    //

    auto configureConnectionStatus = [this](bool _connected) {
        Log::trace("Connection status changed. %1.", _connected ? "Conected" : "Disconnected");
        d->accountManager->setConnected(_connected);
        d->connectionStatus->setConnectionAvailable(_connected);
        d->projectsManager->setConnected(_connected);

        //
        // Синхронизация офлайн правок будет сделана, после проверки ключа сессии на сервере
        //
    };
    connect(d->cloudServiceManager.data(), &CloudServiceManager::connected, d->connectionStatus,
            [configureConnectionStatus] { configureConnectionStatus(true); });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::disconnected, d->connectionStatus,
            [configureConnectionStatus] { configureConnectionStatus(false); });
    connect(d->connectionStatus, &Ui::ConnectionStatusToolBar::checkConnectionPressed,
            d->cloudServiceManager.data(), &CloudServiceManager::start);
    //
    // Уведомления
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::notificationsReceived,
            d->notificationsManager.data(), &NotificationsManager::processNotifications);
    connect(d->notificationsManager.data(), &NotificationsManager::showNotificationsRequested,
            d->menuView, &Ui::MenuView::setNotifications);
    connect(d->notificationsManager.data(), &NotificationsManager::hasUnreadNotificationsChanged,
            this, [this](bool _hasUnreadNotifications) {
                d->projectsManager->setHasUnreadNotifications(_hasUnreadNotifications);
                d->projectManager->setHasUnreadNotifications(_hasUnreadNotifications);
                d->menuView->setHasUnreadNotifications(_hasUnreadNotifications);
            });
    connect(d->menuView, &Ui::MenuView::showDevVersionsChanged, d->notificationsManager.data(),
            &NotificationsManager::setShowDevVersions);
    connect(d->menuView, &Ui::MenuView::notificationsPressed, d->notificationsManager.data(),
            &NotificationsManager::markAllRead);
    //
    // Нужно обновить версию приложения для работы с облаком
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::appVersionUpgradeRequired, this,
            [this] { d->askUpdateToLatestVersion(); });
    //
    // Проверка регистрация или вход
    //
    connect(d->onboardingManager.data(), &OnboardingManager::askConfirmationCodeRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::askConfirmationCode);
    connect(d->accountManager.data(), &AccountManager::askConfirmationCodeRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::askConfirmationCode);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::confirmationCodeInfoRecieved,
            d->accountManager.data(), [this](int _codeLength) {
                d->onboardingManager->setConfirmationCodeInfo(_codeLength);
                d->accountManager->setConfirmationCodeInfo(_codeLength);
            });
    connect(d->onboardingManager.data(), &OnboardingManager::checkConfirmationCodeRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::checkConfirmationCode);
    connect(d->accountManager.data(), &AccountManager::checkConfirmationCodeRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::checkConfirmationCode);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::loginCompleted,
            d->accountManager.data(), [this](bool _isNewAccount) {
                d->cloudServiceManager->askAccountInfo();
                d->onboardingManager->completeSignIn();
                d->accountManager->completeSignIn(_isNewAccount);
                if (_isNewAccount) {
                    d->menuView->closeMenu();
                }
                d->cloudServiceManager->askNotifications();
                d->cloudServiceManager->askTeams();
                d->cloudServiceManager->askProjects();
                d->cloudServiceManager->askSessionStatistics(
                    d->writingSessionManager->sessionStatisticsLastSyncDateTime());

                //
                // Если поймали подключение и сейчас работаем с облачным проектом
                //
                if (d->projectsManager->currentProject() != nullptr
                    && d->projectsManager->currentProject()->isRemote()) {
                    const auto currentProjectId = d->projectsManager->currentProject()->id();
                    //
                    // ... то синхронизируем все документы, у которых есть офлайн правки
                    //
                    const auto unsyncedDocuments = d->projectManager->unsyncedDocuments();
                    for (const auto document : unsyncedDocuments) {
                        d->cloudServiceManager->openDocument(currentProjectId, document);
                    }
                    //
                    // ... текущий открытый документ
                    //
                    d->cloudServiceManager->openDocument(currentProjectId,
                                                         d->projectManager->currentDocument());
                    //
                    // ... а также структуру, данные проекта и словари
                    //
                    d->cloudServiceManager->openStructure(currentProjectId);
                    d->cloudServiceManager->openProjectInfo(currentProjectId);
                    d->cloudServiceManager->openScreenplayDictionaries(currentProjectId);
                }
            });

    //
    // Параметры аккаунта
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::accountInfoReceived, this,
            [this](const Domain::AccountInfo& _accountInfo) {
                d->onboardingManager->setAccountInfo(_accountInfo);
                d->accountManager->setAccountInfo(_accountInfo);

                d->menuView->setSignInVisible(false);
                d->menuView->setAccountVisible(true);
                d->menuView->setAvatar(ImageHelper::imageFromBytes(_accountInfo.avatar));
                d->menuView->setAccountName(_accountInfo.name);
                d->menuView->setAccountEmail(_accountInfo.email);
                d->projectsManager->setProjectsInCloudCanBeCreated(
                    true,
                    (_accountInfo.subscriptions.isEmpty()
                         ? Domain::SubscriptionType::Undefined
                         : _accountInfo.subscriptions.constLast().type));
                d->projectManager->checkAvailabilityToEdit();
                d->projectManager->setAvailableCredits(_accountInfo.credits);
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::promocodeActivated,
            d->accountManager.data(), &AccountManager::showPromocodeActivationMessage);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::promocodeErrorRecieved,
            d->accountManager.data(), &AccountManager::setPromocodeError);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::logoutRequired,
            d->accountManager.data(), &AccountManager::logoutRequested);
    connect(d->accountManager.data(), &AccountManager::askAccountInfoRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::askAccountInfo);
    connect(d->onboardingManager.data(), &OnboardingManager::updateAccountInfoRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setAccountInfo);
    connect(d->accountManager.data(), &AccountManager::updateAccountInfoRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setAccountInfo);
    connect(d->accountManager.data(), &AccountManager::activatePaymentOptionRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::activatePaymentOption);
    connect(d->accountManager.data(), &AccountManager::activatePromocodeRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::activatePromocode);

    connect(d->accountManager.data(), &AccountManager::terminateSessionRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::terminateSession);
    connect(d->accountManager.data(), &AccountManager::logoutRequested, this, [this] {
        d->showLastContent();
        d->cloudServiceManager->logout();
        d->accountManager->clearAccountInfo();
        d->menuView->setSignInVisible(true);
        d->menuView->setAccountVisible(false);
        d->projectManager->checkAvailabilityToEdit();
        if (d->projectsManager->currentProject() != nullptr
            && d->projectsManager->currentProject()->isRemote()) {
            d->closeCurrentProject();
            d->showProjects();
        }
        d->projectsManager->setTeams({});
        d->projectsManager->setCloudProjects({});
        d->projectsManager->setProjectsInCloudCanBeCreated(false, Domain::SubscriptionType::Free);
    });
    //
    connect(Ui::AvatarGenerator::instance(), &Ui::AvatarGenerator::avatarRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::askAvatar);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::avatarRecieved,
            Ui::AvatarGenerator::instance(), &Ui::AvatarGenerator::setAvatar);

    //
    // Команды
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::teamsReceived, this,
            [this](const QVector<Domain::TeamInfo>& _teams) {
                d->accountManager->setAccountTeams(_teams);
                d->projectsManager->setTeams(_teams);
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::teamCreated, this,
            [this](const Domain::TeamInfo& _team) {
                d->accountManager->addAccountTeam(_team);
                d->projectsManager->addOrUpdateTeam(_team);
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::teamUpdated, this,
            [this](const Domain::TeamInfo& _team) {
                d->accountManager->updateAccountTeam(_team);
                d->projectsManager->addOrUpdateTeam(_team);
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::teamRemoved, this,
            [this](int _id) {
                d->accountManager->removeAccountTeam(_id);
                d->projectsManager->hideTeam(_id);
            });
    connect(d->accountManager.data(), &AccountManager::createTeamRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::createTeam);
    connect(d->accountManager.data(), &AccountManager::updateTeamRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::updateTeam);
    connect(d->accountManager.data(), &AccountManager::removeTeamRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::removeTeam);
    connect(d->accountManager.data(), &AccountManager::exitFromTeamRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::exitFormTeam);
    connect(d->accountManager.data(), &AccountManager::addMemberRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::addTeamMember);
    connect(d->accountManager.data(), &AccountManager::changeMemberRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::updateTeamMember);
    connect(d->accountManager.data(), &AccountManager::removeMemberRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::removeTeamMember);

    //
    // Проекты
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::projectsReceived,
            d->projectsManager.data(), [this](const QVector<Domain::ProjectInfo>& _projects) {
                d->projectsManager->setCloudProjects(_projects);
                if (d->projectsManager->currentProject() != nullptr
                    && d->projectsManager->currentProject()->isRemote()) {
                    d->projectManager->updateCurrentProject(d->projectsManager->currentProject());
                }
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::projectUpdated, this,
            [this](const Domain::ProjectInfo& _projectInfo) {
                d->projectsManager->addOrUpdateCloudProject(_projectInfo);
                if (d->projectsManager->currentProject() != nullptr
                    && _projectInfo.id == d->projectsManager->currentProject()->id()) {
                    d->projectManager->updateCurrentProject(d->projectsManager->currentProject());
                }
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::projectRemoved,
            d->projectsManager.data(), [this](int _projectId) {
                if (d->projectsManager->currentProject() != nullptr
                    && d->projectsManager->currentProject()->id() == _projectId) {
                    d->closeCurrentProject();
                    d->markChangesSaved(true);
                    d->updateWindowTitle();
                    d->showProjects();
                }
                d->projectsManager->removeProject(_projectId);
            });
    connect(d->projectsManager.data(), &ProjectsManager::createCloudProjectRequested, this,
            [this](const QString& _projectName, const QString& _importFilePath, int _teamId) {
                d->createRemoteProject(_projectName, _importFilePath, _teamId);
            });
    connect(d->projectsManager.data(), &ProjectsManager::openCloudProjectRequested, this,
            [this](int _id, const QString& _path) {
                if (d->projectsManager->currentProject() != nullptr
                    && d->projectsManager->currentProject()->id() == _id) {
                    d->showProject();
                    return;
                }

                auto callback = [this, _id, _path] {
                    const auto isProjectOpened = openProject(_path);
                    if (!isProjectOpened) {
                        return;
                    }

                    d->cloudServiceManager->openStructure(_id);
                    d->cloudServiceManager->openProjectInfo(_id);
                    d->cloudServiceManager->openScreenplayDictionaries(_id);
                };
                d->saveIfNeeded(callback);
            });
    connect(d->projectsManager.data(), &ProjectsManager::updateCloudProjectRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::updateProject);
    connect(d->projectManager.data(), &ProjectManager::projectCollaboratorInviteRequested,
            d->projectsManager.data(),
            [this](const QString& _email, const QColor& _color, int _role,
                   const QHash<QUuid, int>& _permissions) {
                Q_UNUSED(_color)
                const auto accountRole = _role;
                d->cloudServiceManager->addProjectCollaborator(
                    d->projectsManager->currentProject()->id(), _email, accountRole, _permissions);
            });
    connect(d->projectManager.data(), &ProjectManager::projectCollaboratorUpdateRequested,
            d->projectsManager.data(),
            [this](const QString& _email, const QColor& _color, int _role,
                   const QHash<QUuid, int>& _permissions) {
                Q_UNUSED(_color)
                const auto accountRole = _role;
                d->cloudServiceManager->updateProjectCollaborator(
                    d->projectsManager->currentProject()->id(), _email, accountRole, _permissions);
            });
    connect(d->projectManager.data(), &ProjectManager::projectCollaboratorRemoveRequested,
            d->projectsManager.data(), [this](const QString& _email) {
                d->cloudServiceManager->removeProjectCollaborator(
                    d->projectsManager->currentProject()->id(), _email);
            });
    connect(d->projectsManager.data(), &ProjectsManager::removeCloudProjectRequested, this,
            [this](int _id) { d->cloudServiceManager->removeProject(_id); });
    connect(d->projectsManager.data(), &ProjectsManager::unsubscribeFromCloudProjectRequested, this,
            [this](int _id) { d->cloudServiceManager->unsubscribeFromProject(_id); });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::projectCantBeSynced, this, [this] {
        const auto& currentProject = d->projectsManager->currentProject();

        //
        // Если проект уже в состоянии проблем синхронизации, то не показываем диалог вновь,
        // чтобы не поймать эффект множественного наложения диалогов
        //
        if (!currentProject->canBeSynced()) {
            return;
        }

        d->projectsManager->setCurrentProjectCanBeSynced(false);

        if (currentProject->isOwner()) {
            auto dialog = new Dialog(d->applicationView);
            const int kCancelButtonId = 0;
            const int kAcceptButtonId = 1;
            dialog->showDialog(
                {},
                tr("Your cloud service subscription is expired. Activate subscription to continue "
                   "working with the project."),
                { { kCancelButtonId, tr("Continue offline"), Dialog::RejectButton },
                  { kAcceptButtonId, tr("Renew subscription"), Dialog::AcceptButton } });
            QObject::connect(
                dialog, &Dialog::finished, dialog,
                [this, dialog, kCancelButtonId](const Dialog::ButtonInfo& _pressedButton) {
                    dialog->hideDialog();

                    if (_pressedButton.id == kCancelButtonId) {
                        return;
                    }

                    d->accountManager->renewTeam();
                });
            QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
        } else {
            auto dialog = new Dialog(d->applicationView);
            const int kCancelButtonId = 0;
            dialog->showDialog(
                {},
                tr("Cloud service subscription of the project owners is expired. You can continue "
                   "working with the project as soon as they renew the subscription."),
                { { kCancelButtonId, tr("Continue offline"), Dialog::RejectButton } });
            QObject::connect(dialog, &Dialog::finished, dialog, &Dialog::hideDialog);
            QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
        }
    });

    //
    // Документы
    //
    connect(d->projectManager.data(), &ProjectManager::structureModelChanged, this,
            [this](BusinessLayer::AbstractModel* _model) {
                const auto& currentProject = d->projectsManager->currentProject();
                if (!currentProject->isRemote() || !currentProject->canBeSynced()) {
                    return;
                }

                d->cloudServiceManager->openDocument(currentProject->id(), _model->document());
            });
    connect(d->projectManager.data(), &ProjectManager::downloadDocumentRequested, this,
            [this](const QUuid& _documentUuid) {
                const auto& currentProject = d->projectsManager->currentProject();
                if (!currentProject->isRemote() || !currentProject->canBeSynced()
                    || _documentUuid.isNull()) {
                    return;
                }

                const auto document = d->projectManager->documentToSync(_documentUuid);

                //
                // Для сложных типов документов, нужна подгрузка документов, которые с ним связаны
                //
                if (const auto connectedDocuments
                    = d->projectManager->connectedDocuments(_documentUuid);
                    !connectedDocuments.isEmpty()) {
                    for (const auto& connectedDocumentUuid : connectedDocuments) {
                        d->cloudServiceManager->openDocument(currentProject->id(),
                                                             connectedDocumentUuid);
                    }
                }

                //
                // Если документ не удалось вытащить из базы, значит он ещё не был синхронизирован
                //
                if (document == nullptr) {
                    d->cloudServiceManager->openDocument(currentProject->id(), _documentUuid);
                }
                //
                // А если есть, то значит его инстанс уже есть в базе и его надо обновить
                //
                else {
                    d->cloudServiceManager->openDocument(currentProject->id(), document);
                }
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::documentReceived,
            d->projectManager.data(), &ProjectManager::mergeDocumentInfo);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::documentChanged,
            d->projectManager.data(), &ProjectManager::applyDocumentChanges);
    connect(d->projectManager.data(), &ProjectManager::contentsChanged, this,
            [this](BusinessLayer::AbstractModel* _model) {
                const auto& currentProject = d->projectsManager->currentProject();
                if (!currentProject->isRemote() || !currentProject->canBeSynced()) {
                    return;
                }

                //
                // Запросить отправку изменений
                //
                d->cloudServiceManager->startDocumentChange(currentProject->id(),
                                                            _model->document()->uuid());

                //
                // Запустить отсчёт до следующей отправки документа целиком
                //
                d->projectManager->planDocumentSyncing(_model->document()->uuid());
            });
    connect(d->projectManager.data(), &ProjectManager::uploadDocumentRequested, this,
            [this](const QUuid& _documentUuid, bool _isNewDocument) {
                const auto& currentProject = d->projectsManager->currentProject();
                if (!currentProject->isRemote() || !currentProject->canBeSynced()) {
                    return;
                }

                //
                // Если был создан новый документ, то кидаем его в облако и подписываемся
                //
                if (_isNewDocument) {
                    d->cloudServiceManager->openDocument(
                        currentProject->id(), d->projectManager->documentToSync(_documentUuid));
                }
                //
                // В противном случае, запросим отправку изменений
                //
                else {
                    d->cloudServiceManager->startDocumentChange(currentProject->id(),
                                                                _documentUuid);
                }
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::documentChangeAllowed, this,
            [this](const QUuid& _documentUuid) {
                const auto& currentProject = d->projectsManager->currentProject();
                if (currentProject != nullptr
                    && (!currentProject->isRemote() || !currentProject->canBeSynced())) {
                    return;
                }

                //
                // Если есть изменения, то отправляем их
                //
                const auto changes = d->projectManager->unsyncedChanges(_documentUuid);
                if (!changes.isEmpty()) {
                    d->cloudServiceManager->pushDocumentChange(currentProject->id(), _documentUuid,
                                                               changes);
                }
                //
                // В противном случае, отправляем документ целиком
                //
                else if (auto document = d->projectManager->documentToSync(_documentUuid);
                         document != nullptr) {
                    d->cloudServiceManager->pushDocumentChange(currentProject->id(), _documentUuid,
                                                               document->content());
                }
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::documentChangesPushed,
            d->projectManager.data(), &ProjectManager::markChangesSynced);
    connect(d->projectManager.data(), &ProjectManager::documentRemoved, this,
            [this](const QUuid& _documentUuid) {
                const auto& currentProject = d->projectsManager->currentProject();
                if (!currentProject->isRemote() || !currentProject->canBeSynced()) {
                    return;
                }

                d->cloudServiceManager->removeDocument(currentProject->id(), _documentUuid);
            });

    //
    // Курсоры
    //
    connect(d->projectManager.data(), &ProjectManager::cursorChanged, this,
            [this](const QUuid& _documentUuid, const QByteArray& _cursorData) {
                const auto currentProject = d->projectsManager->currentProject();
                if (!currentProject->isRemote() || !currentProject->canBeSynced()) {
                    return;
                }

                d->cloudServiceManager->updateCursor(currentProject->id(), _documentUuid,
                                                     _cursorData);
            });
    connect(d->cloudServiceManager.data(), &CloudServiceManager::cursorsChanged, this,
            [this](int _projectId, const QUuid& _documentUuid,
                   const QVector<Domain::CursorInfo>& _cursors) {
                if (const auto currentProject = d->projectsManager->currentProject();
                    currentProject->isRemote() && currentProject->id() == _projectId) {
                    d->projectManager->setCursors(_documentUuid, _cursors);
                }
            });

    //
    // Статистика по сессии
    //
    connect(d->writingSessionManager.data(),
            &WritingSessionManager::sessionStatisticsPublishRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::pushSessionStatistics);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::sessionStatisticsReceived,
            d->writingSessionManager.data(), &WritingSessionManager::setSessionStatistics);

    //
    // Генерация текста
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::buyCreditsRequested,
            d->accountManager.data(), &AccountManager::buyCredits);
    connect(d->projectManager.data(), &ProjectManager::rephraseTextRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiRephraseText);
    connect(d->projectManager.data(), &ProjectManager::expandTextRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiExpandText);
    connect(d->projectManager.data(), &ProjectManager::shortenTextRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiShortenText);
    connect(d->projectManager.data(), &ProjectManager::insertTextRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiInsertText);
    connect(d->projectManager.data(), &ProjectManager::summarizeTextRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiSummarizeText);
    connect(d->projectManager.data(), &ProjectManager::translateTextRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiTranslateText);
    connect(d->projectManager.data(), &ProjectManager::generateTextRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiGenerateText);
    connect(d->projectManager.data(), &ProjectManager::generateImageRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::aiGenerateImage);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::textRephrased,
            d->projectManager.data(), &ProjectManager::setRephrasedText);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::textExpanded,
            d->projectManager.data(), &ProjectManager::setExpandedText);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::textShortened,
            d->projectManager.data(), &ProjectManager::setShortenedText);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::textInserted,
            d->projectManager.data(), &ProjectManager::setInsertedText);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::textSummarizeed,
            d->projectManager.data(), &ProjectManager::setSummarizeedText);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::textTranslated,
            d->projectManager.data(), &ProjectManager::setTranslatedText);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::textGenerated,
            d->projectManager.data(), &ProjectManager::setGeneratedText);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::imageGenerated,
            d->projectManager.data(), &ProjectManager::setGeneratedImage);
#endif
}

} // namespace ManagementLayer
