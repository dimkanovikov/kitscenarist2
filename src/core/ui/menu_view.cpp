#include "menu_view.h"

#include "about_application_dialog.h"
#include "notifications/credits_view.h"
#include "notifications/release_view.h"
#include "notifications/subscription_view.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/drawer/drawer.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <utils/3rd_party/WAF/Animation/Animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>
#include <utils/logging.h>
#include <utils/shugar.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QMenuBar>
#include <QScrollArea>
#include <QTimer>

namespace Ui {

class MenuView::Implementation
{
public:
    explicit Implementation(MenuView* _parent);

    /**
     * @brief Создание MenuBar
     */
    void createMenuBar();

    MenuView* q = nullptr;

    QScrollArea* menuPage = nullptr;

    Drawer* drawer = nullptr;
    QAction* signIn = nullptr;
    QAction* projects = nullptr;
    QAction* createProject = nullptr;
    QAction* openProject = nullptr;
    QAction* project = nullptr;
    QAction* saveProject = nullptr;
    QAction* saveProjectAs = nullptr;
    QAction* exportCurrentDocument = nullptr;
    QAction* importProject = nullptr;
    QAction* fullScreen = nullptr;
    QAction* settings = nullptr;
    QAction* aboutApplicationAction = nullptr;

    QAction* writingStatistics = nullptr;
    QAction* writingSprint = nullptr;
    QAction* chat = nullptr;
    QAction* notifications = nullptr;

    Subtitle2LinkLabel* appName = nullptr;
    Body2LinkLabel* appVersion = nullptr;
    Body2Label* aboutAppSpacer = nullptr;
    Body2LinkLabel* aboutApp = nullptr;
    QGridLayout* appInfoLayout = nullptr;

    //

    Widget* notificationsPage = nullptr;

    IconButton* notificationsBackButton = nullptr;
    ButtonLabel* notificationsTitle = nullptr;
    IconButton* notificationsFilterButton = nullptr;

    QScrollArea* notificationsViewport = nullptr;
    QVBoxLayout* notificationsLayout = nullptr;

    QAction* showDevVersions = nullptr;
};

MenuView::Implementation::Implementation(MenuView* _parent)
    : q(_parent)
    , menuPage(UiHelper::createScrollArea(_parent))
    , drawer(new Drawer(_parent))
    , signIn(new QAction)
    , projects(new QAction)
    , createProject(new QAction)
    , openProject(new QAction)
    , project(new QAction)
    , saveProject(new QAction)
    , saveProjectAs(new QAction)
    , exportCurrentDocument(new QAction)
    , importProject(new QAction)
    , fullScreen(new QAction)
    , settings(new QAction)
    , aboutApplicationAction(new QAction)
    , writingStatistics(new QAction)
    , writingSprint(new QAction)
    , chat(new QAction)
    , notifications(new QAction)
    , appName(new Subtitle2LinkLabel(_parent))
    , appVersion(new Body2LinkLabel(_parent))
    , aboutAppSpacer(new Body2Label(_parent))
    , aboutApp(new Body2LinkLabel(_parent))
    , appInfoLayout(new QGridLayout)
    , notificationsPage(new Widget(_parent))
    , notificationsBackButton(new IconButton(_parent))
    , notificationsTitle(new ButtonLabel(_parent))
    , notificationsFilterButton(new IconButton(_parent))
    , notificationsViewport(UiHelper::createScrollArea(_parent))
    , notificationsLayout(new QVBoxLayout)
    , showDevVersions(new QAction(_parent))
{
    //
    // Настроим страницу меню
    //
    {
        drawer->addAction(signIn);
        drawer->addAction(projects);
        drawer->addAction(createProject);
        drawer->addAction(openProject);
        drawer->addAction(project);
        drawer->addAction(saveProject);
        drawer->addAction(saveProjectAs);
        drawer->addAction(importProject);
        drawer->addAction(exportCurrentDocument);
        drawer->addAction(fullScreen);
        drawer->addAction(settings);

        drawer->setAccountActions({
            writingStatistics,
            writingSprint,
            chat,
            notifications,
        });

        signIn->setIconText(u8"\U000F0004");
        signIn->setCheckable(false);
        signIn->setVisible(false);
        //
        projects->setIconText(u8"\U000f024b");
        projects->setCheckable(true);
        projects->setChecked(true);
        //
        createProject->setIconText(u8"\U000f0415");
        createProject->setCheckable(false);
        //
        openProject->setIconText(u8"\U000f0770");
        openProject->setCheckable(false);
        //
        project->setIconText(u8"\U000f00be");
        project->setCheckable(true);
        project->setVisible(false);
        project->setSeparator(true);
        //
        saveProject->setIconText(u8"\U000f0193");
        saveProject->setCheckable(false);
        saveProject->setEnabled(false);
        saveProject->setVisible(false);
        //
        saveProjectAs->setIconText(" ");
        saveProjectAs->setCheckable(false);
        saveProjectAs->setVisible(false);
        //
        exportCurrentDocument->setIconText(u8"\U000f0207");
        exportCurrentDocument->setCheckable(false);
        exportCurrentDocument->setEnabled(false);
        exportCurrentDocument->setVisible(false);
        //
        importProject->setIconText(u8"\U000f02fa");
        importProject->setCheckable(false);
        importProject->setVisible(false);
        //
        fullScreen->setIconText(u8"\U000F0293");
        fullScreen->setCheckable(false);
        fullScreen->setVisible(false);
        fullScreen->setSeparator(true);
        //
        settings->setIconText(u8"\U000f0493");
        settings->setCheckable(false);
        settings->setVisible(true);
        settings->setSeparator(true);

        QActionGroup* actions = new QActionGroup(_parent);
        actions->addAction(projects);
        actions->addAction(project);
        actions->addAction(settings);

        writingStatistics->setIconText(u8"\U000F085D");
        //
        writingSprint->setIconText(u8"\U000F13AB");
        //
        chat->setIconText(u8"\U000F0368");
        chat->setVisible(false);
        //
        notifications->setIconText(u8"\U000F009A");

        appName->setText("Story Architect");
        appName->setLink(QUrl("https://starc.app"));
        appVersion->setLink(QUrl("https://starc.app/blog/"));
        aboutAppSpacer->setText(" - ");

        appInfoLayout->setContentsMargins({});
        appInfoLayout->setSpacing(0);
        appInfoLayout->addWidget(appName, 0, 0, 1, 4, Qt::AlignLeft);
        appInfoLayout->addWidget(appVersion, 1, 0);
        appInfoLayout->addWidget(aboutAppSpacer, 1, 1);
        appInfoLayout->addWidget(aboutApp, 1, 2);
        appInfoLayout->setColumnStretch(3, 1);

        auto layout = static_cast<QVBoxLayout*>(menuPage->widget()->layout());
        layout->insertWidget(0, drawer);
        layout->addStretch();
        layout->addLayout(appInfoLayout);
    }

    //
    // Настроим страницу уведомлений
    //
    {
        notificationsBackButton->setIcon(u8"\U000F004D");
        notificationsFilterButton->setIcon(u8"\U000F1542");

        showDevVersions->setIconText(u8"\U000F1323");
        showDevVersions->setCheckable(true);

        new Shadow(Qt::TopEdge, notificationsViewport);

        auto topLayout = new QHBoxLayout;
        topLayout->setContentsMargins({});
        topLayout->setSpacing(0);
        topLayout->addWidget(notificationsBackButton);
        topLayout->addWidget(notificationsTitle, 1, Qt::AlignVCenter);
        topLayout->addWidget(notificationsFilterButton);

        notificationsLayout = static_cast<QVBoxLayout*>(notificationsViewport->widget()->layout());

        auto layout = new QVBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addLayout(topLayout);
        layout->addWidget(notificationsViewport);
        notificationsPage->setLayout(layout);
    }
}

// ****

MenuView::MenuView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
#ifdef CLOUD_SERVICE_MANAGER
    d->signIn->setVisible(true);
    d->projects->setSeparator(true);
#endif
    setAnimationType(StackWidget::AnimationType::Slide);
    setCurrentWidget(d->menuPage);
    addWidget(d->notificationsPage);

    connect(d->drawer, &Drawer::accountPressed, this, &MenuView::accountPressed);
    connect(d->signIn, &QAction::triggered, this, &MenuView::signInPressed);
    connect(d->projects, &QAction::triggered, this, &MenuView::projectsPressed);
    connect(d->createProject, &QAction::triggered, this, &MenuView::createProjectPressed);
    connect(d->openProject, &QAction::triggered, this, &MenuView::openProjectPressed);
    connect(d->project, &QAction::triggered, this, &MenuView::projectPressed);
    connect(d->saveProject, &QAction::triggered, this, &MenuView::saveProjectChangesPressed);
    connect(d->saveProjectAs, &QAction::triggered, this, &MenuView::saveProjectAsPressed);
    connect(d->importProject, &QAction::triggered, this, &MenuView::importPressed);
    connect(d->exportCurrentDocument, &QAction::triggered, this,
            &MenuView::exportCurrentDocumentPressed);
    connect(d->fullScreen, &QAction::triggered, this, &MenuView::fullscreenPressed);
    connect(d->settings, &QAction::triggered, this, &MenuView::settingsPressed);
    //
    connect(d->writingStatistics, &QAction::triggered, this, &MenuView::writingStatisticsPressed);
    connect(d->writingSprint, &QAction::triggered, this, &MenuView::writingSprintPressed);
    connect(d->notifications, &QAction::triggered, this, [this] {
        setCurrentWidget(d->notificationsPage);
        emit notificationsPressed();
    });
    //

    connect(d->aboutApplicationAction, &QAction::triggered, this, [this] {
        auto dialog = new AboutApplicationDialog(parentWidget());
        connect(dialog, &Ui::AboutApplicationDialog::disappeared, dialog,
                &Ui::AboutApplicationDialog::deleteLater);
        dialog->showDialog();
    });
    connect(d->aboutApp, &Body2LinkLabel::clicked, d->aboutApplicationAction, &QAction::trigger);

    connect(this, &MenuView::accountPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::projectsPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::createProjectPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::openProjectPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::projectPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::saveProjectAsPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::importPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::exportCurrentDocumentPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::fullscreenPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::settingsPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::helpPressed, this, &MenuView::closeMenu);
    //
    connect(this, &MenuView::writingStatisticsPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::writingSprintPressed, this, &MenuView::closeMenu);
    //
    connect(d->notificationsBackButton, &IconButton::clicked, this,
            [this] { setCurrentWidget(d->menuPage); });
    connect(d->notificationsFilterButton, &IconButton::clicked, this, [this] {
        //
        // Настроим меню опций
        //
        auto menu = new ContextMenu(this);
        menu->setBackgroundColor(Ui::DesignSystem::color().background());
        menu->setTextColor(Ui::DesignSystem::color().onBackground());
        menu->setActions({
            d->showDevVersions,
        });
        connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

        //
        // Покажем меню
        //
        menu->showContextMenu(QCursor::pos());
    });
    connect(d->showDevVersions, &QAction::toggled, this, [this](bool _checked) {
        d->showDevVersions->setText(_checked ? tr("Hide developers version")
                                             : tr("Show developers version"));
        emit showDevVersionsChanged(_checked);
    });

    setVisible(false);

    //
    // Добавляем MenuBar
    //
    d->createMenuBar();
}

MenuView::~MenuView() = default;

void MenuView::setAccountVisible(bool _visible)
{
    d->drawer->setAccountVisible(_visible);
}

void MenuView::setAvatar(const QPixmap& _avatar)
{
    d->drawer->setAvatar(_avatar);
}

void MenuView::setAccountName(const QString& _name)
{
    d->drawer->setAccountName(_name);
}

void MenuView::setAccountEmail(const QString& _email)
{
    d->drawer->setAccountEmail(_email);
}

void MenuView::setSignInVisible(bool _visible)
{
#ifdef CLOUD_SERVICE_MANAGER
    d->signIn->setVisible(_visible);
#else
    Q_UNUSED(_visible)
#endif
}

void MenuView::checkProjects()
{
    QSignalBlocker signalBlocker(this);
    d->projects->setChecked(true);
}

void MenuView::setProjectActionsVisible(bool _visible)
{
    d->project->setVisible(_visible);
    d->saveProject->setVisible(_visible);
    d->saveProjectAs->setVisible(_visible);
    d->exportCurrentDocument->setVisible(_visible);
    d->importProject->setVisible(_visible);
    d->fullScreen->setVisible(_visible);
    d->drawer->updateGeometry();
}

void MenuView::checkProject()
{
    QSignalBlocker signalBlocker(this);
    d->project->setChecked(true);
}

void MenuView::setProjectTitle(const QString& _title)
{
    d->project->setText(_title);
}

void MenuView::checkSettings()
{
    QSignalBlocker signalBlocker(this);
    d->importProject->setChecked(true);
}

void MenuView::markChangesSaved(bool _saved)
{
    d->saveProject->setEnabled(!_saved);
    d->saveProject->setText(_saved ? tr("All changes saved") : tr("Save changes"));
    update();
}

void MenuView::setImportShortcut(const QKeySequence& _key)
{
    d->importProject->setWhatsThis(_key.toString(QKeySequence::NativeText));
}

void MenuView::setImportAvailable(bool _available)
{
    d->importProject->setEnabled(_available);
}

void MenuView::setCurrentDocumentExportShortcut(const QKeySequence& _key)
{
    d->exportCurrentDocument->setWhatsThis(_key.toString(QKeySequence::NativeText));
}

void MenuView::setCurrentDocumentExportAvailable(bool _available)
{
    d->exportCurrentDocument->setEnabled(_available);
}

void MenuView::setHasUnreadNotifications(bool _hasUnreadNotifications)
{
    d->drawer->setAccountActionBadgeVisible(d->notifications, _hasUnreadNotifications);
}

void MenuView::setShowDevVersions(bool _show)
{
    d->showDevVersions->setChecked(_show);
}

void MenuView::setNotifications(const QVector<Domain::Notification>& _notifications)
{
    //
    // Удаляем старые уведомления
    //
    while (d->notificationsLayout->count() > 0) {
        auto item = d->notificationsLayout->takeAt(0);
        if (item->widget() == nullptr) {
            break;
        }

        item->widget()->deleteLater();
    }

    //
    // Т.к. уведомления приходят упорядоченными от новых к старым, разворачиваем список, чтобы
    // просто вставлять уведомления в начало списка
    //
    for (const auto& notification : reversed(_notifications)) {
        QWidget* notificationView = nullptr;
        switch (notification.type) {
        case Domain::NotificationType::UpdateDevLinux:
        case Domain::NotificationType::UpdateDevMac:
        case Domain::NotificationType::UpdateDevWindows32:
        case Domain::NotificationType::UpdateDevWindows64:
        case Domain::NotificationType::UpdateStableLinux:
        case Domain::NotificationType::UpdateStableMac:
        case Domain::NotificationType::UpdateStableWindows32:
        case Domain::NotificationType::UpdateStableWindows64: {
            notificationView = new ReleaseView(this, notification);
            break;
        }

        case Domain::NotificationType::ProSubscriptionEnds:
        case Domain::NotificationType::TeamSubscriptionEnds: {
            auto view = new SubscriptionView(this, notification);
            connect(view, &SubscriptionView::renewProPressed, this, &MenuView::renewProPressed);
            connect(view, &SubscriptionView::renewTeamPressed, this, &MenuView::renewTeamPressed);
            notificationView = view;
            break;
        }

        case Domain::NotificationType::CreditsAdded: {
            notificationView = new CreditsView(this, notification);
            break;
        }

        default: {
            break;
        }
        }
        if (notificationView != nullptr) {
            d->notificationsLayout->insertWidget(0, notificationView);
        }
    }
    d->notificationsLayout->addStretch();
}

void MenuView::openMenu()
{
    Log::info("Show menu");

    //
    // Скрываем, переключаем и показываем, чтобы виджет просто переключился, без анимации
    //
    hide();
    setCurrentWidget(d->menuPage);
    show();

    WAF::Animation::sideSlideIn(this);
}

void MenuView::closeMenu()
{
    Log::info("Hide menu");
    const auto duration = WAF::Animation::sideSlideOut(this);
    QTimer::singleShot(duration, this, &MenuView::hide);
}

QSize MenuView::sizeHint() const
{
    return d->drawer->sizeHint();
}

void MenuView::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Escape) {
        closeMenu();
    }

    StackWidget::keyPressEvent(_event);
}

void MenuView::updateTranslations()
{
    d->signIn->setText(tr("Sign in"));
    d->projects->setText(tr("Stories"));
    d->createProject->setText(tr("Create story"));
    d->openProject->setText(tr("Open story"));
    d->saveProject->setText(d->saveProject->isEnabled() ? tr("Save story changes")
                                                        : tr("All changes saved"));
    d->saveProject->setWhatsThis(
        QKeySequence(QKeySequence::Save).toString(QKeySequence::NativeText));
    d->saveProjectAs->setText(tr("Save story as..."));
    d->importProject->setText(tr("Import documents..."));
    d->exportCurrentDocument->setText(tr("Export current document..."));
    d->fullScreen->setText(tr("Full screen"));
    d->fullScreen->setWhatsThis(
        QKeySequence(QKeySequence::FullScreen).toString(QKeySequence::NativeText));
    d->settings->setText(tr("Application settings"));

    d->writingStatistics->setToolTip(tr("Show writing statistics"));
    d->writingSprint->setToolTip(tr("Show writing sprint timer"));
    d->notifications->setToolTip(tr("Show notifications"));

    d->appVersion->setText(
        QString("%1 %2").arg(tr("Version"), QCoreApplication::applicationVersion()));
    d->aboutApp->setText(tr("About"));

    d->notificationsBackButton->setToolTip(tr("Back to main menu"));
    d->notificationsTitle->setText(tr("Notifications"));
    d->notificationsFilterButton->setToolTip(tr("Notifications preferences"));

    d->showDevVersions->setText(d->showDevVersions->isChecked() ? tr("Hide developers version")
                                                                : tr("Show developers version"));

    d->chat->setText(tr("Account"));
    d->notifications->setText(tr("Notifications"));
    d->writingStatistics->setText(tr("Writing statistics"));
    d->writingSprint->setText(tr("Writing sprint"));
}

void MenuView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    for (auto label : std::vector<Widget*>{
             d->appName,
             d->appVersion,
             d->aboutAppSpacer,
             d->aboutApp,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().primary());
        label->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                     Ui::DesignSystem::disabledTextOpacity()));
    }
    d->appInfoLayout->setContentsMargins(
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16());
    d->appInfoLayout->setVerticalSpacing(Ui::DesignSystem::layout().px8());

    d->notificationsPage->setBackgroundColor(Ui::DesignSystem::color().primary());
    for (auto label : std::vector<Widget*>{
             d->notificationsBackButton,
             d->notificationsTitle,
             d->notificationsFilterButton,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().primary());
        label->setTextColor(Ui::DesignSystem::color().onPrimary());
    }
    d->notificationsBackButton->setContentsMargins(
        isLeftToRight() ? Ui::DesignSystem::layout().px4() : 0, Ui::DesignSystem::layout().px4(),
        isLeftToRight() ? 0 : Ui::DesignSystem::layout().px4(), 0);
    d->notificationsTitle->setContentsMargins(0, Ui::DesignSystem::layout().px4(), 0, 0);
    d->notificationsFilterButton->setContentsMargins(
        isLeftToRight() ? 0 : Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px4(),
        isLeftToRight() ? Ui::DesignSystem::layout().px4() : 0, 0);
}

void MenuView::Implementation::createMenuBar()
{
#ifdef Q_OS_MAC

    QMenuBar* menuBar = new QMenuBar;
    menuBar->setNativeMenuBar(true);

    //
    // Создаем пункты меню
    //

    // Основной пункт меню с нeзвазванием "Story Architect"
    QMenu* appMenu = menuBar->addMenu("Story Architect");
    aboutApplicationAction->setText(tr("About Story Architect"));
    aboutApplicationAction->setMenuRole(QAction::ApplicationSpecificRole);
    appMenu->addAction(aboutApplicationAction);
    appMenu->addSeparator();
    QAction* settingsAction = new QAction();
    settingsAction->setText(tr("Preferences"));
    connect(settingsAction, &QAction::triggered, q, &MenuView::settingsPressed);
    settingsAction->setMenuRole(QAction::ApplicationSpecificRole);
    appMenu->addAction(settingsAction);

#ifdef CLOUD_SERVICE_MANAGER
    signIn->setMenuRole(QAction::ApplicationSpecificRole);
    appMenu->addAction(signIn);

    chat->setMenuRole(QAction::ApplicationSpecificRole);
    appMenu->addAction(chat);

    notifications->setMenuRole(QAction::ApplicationSpecificRole);
    appMenu->addAction(notifications);

    appMenu->addSeparator();

    writingStatistics->setMenuRole(QAction::ApplicationSpecificRole);
    appMenu->addAction(writingStatistics);

    writingSprint->setMenuRole(QAction::ApplicationSpecificRole);
    appMenu->addAction(writingSprint);
#endif

    // Меню "File"
    QMenu* fileMenu = menuBar->addMenu(tr("File"));
    fileMenu->addAction(projects);
    fileMenu->addAction(createProject);
    fileMenu->addAction(openProject);
    saveProject->setShortcut(QKeySequence("⌘S"));
    fileMenu->addAction(saveProject);
    fileMenu->addAction(saveProjectAs);
    fileMenu->addSeparator();
    importProject->setShortcut(QKeySequence("⌥I"));
    fileMenu->addAction(importProject);
    exportCurrentDocument->setShortcut(QKeySequence("⌥E"));
    fileMenu->addAction(exportCurrentDocument);

    // Меню View
    QMenu* viewMenu = menuBar->addMenu(tr("View"));
    viewMenu->addAction(fullScreen);
#endif
}

} // namespace Ui
