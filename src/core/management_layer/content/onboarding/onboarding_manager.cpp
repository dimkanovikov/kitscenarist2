#include "onboarding_manager.h"

#include <ui/onboarding/onboarding_navigator.h>
#include <ui/onboarding/onboarding_tool_bar.h>
#include <ui/onboarding/onboarding_view.h>


namespace ManagementLayer {

namespace {
constexpr int kInvalidConfirmationCodeLength = -1;
}

class OnboardingManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Ui::OnboardingToolBar* toolBar = nullptr;
    Ui::OnboardingNavigator* navigator = nullptr;
    Ui::OnboardingView* view = nullptr;

    int confirmationCodeLength = kInvalidConfirmationCodeLength;
};

OnboardingManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new Ui::OnboardingToolBar(_parent))
    , navigator(new Ui::OnboardingNavigator(_parent))
    , view(new Ui::OnboardingView(_parent))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}


//****


OnboardingManager::OnboardingManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(_parentWidget))
{
    connect(d->navigator, &Ui::OnboardingNavigator::languageChanged, this,
            &OnboardingManager::languageChanged);
    connect(d->navigator, &Ui::OnboardingNavigator::themeChanged, this,
            &OnboardingManager::themeChanged);
    connect(d->navigator, &Ui::OnboardingNavigator::competitorColorSchemeSelected, this,
            &OnboardingManager::themeChangedCustom);
    connect(d->navigator, &Ui::OnboardingNavigator::scaleFactorChanged, this,
            &OnboardingManager::scaleFactorChanged);
    connect(d->navigator, &Ui::OnboardingNavigator::signInPressed, this,
            [this] { emit askConfirmationCodeRequested(d->navigator->email()); });
    connect(d->navigator, &Ui::OnboardingNavigator::confirmationCodeChanged, this,
            [this](const QString& _code) {
                if (d->confirmationCodeLength == kInvalidConfirmationCodeLength
                    || _code.length() != d->confirmationCodeLength) {
                    return;
                }

                emit checkConfirmationCodeRequested(_code);
            });
    connect(d->navigator, &Ui::OnboardingNavigator::accountInfoChanged, this,
            &OnboardingManager::updateAccountInfoRequested);
    connect(d->navigator, &Ui::OnboardingNavigator::finishOnboardingRequested, this,
            &OnboardingManager::finished);
}

OnboardingManager::~OnboardingManager() = default;

QWidget* OnboardingManager::toolBar() const
{
    return d->toolBar;
}

QWidget* OnboardingManager::navigator() const
{
    return d->navigator;
}

QWidget* OnboardingManager::view() const
{
    return d->view;
}

void OnboardingManager::showWelcomePage()
{
    d->navigator->showWelcomePage();
}

void OnboardingManager::setConfirmationCodeInfo(int _codeLength)
{
    d->confirmationCodeLength = _codeLength;
}

void OnboardingManager::completeSignIn()
{
    d->navigator->showAccountPage();
}

void OnboardingManager::setAccountInfo(const Domain::AccountInfo& _accountInfo)
{
    d->navigator->setAccountInfo(_accountInfo);
}

} // namespace ManagementLayer
