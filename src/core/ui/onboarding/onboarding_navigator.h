#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

#include <QLocale>

namespace Domain {
struct AccountInfo;
}


namespace Ui {
enum class ApplicationTheme;
enum class ApplicationToImportSettings;

/**
 * @brief Навигатор по посадочным страницам
 */
class OnboardingNavigator : public StackWidget
{
    Q_OBJECT

public:
    explicit OnboardingNavigator(QWidget* _parent = nullptr);
    ~OnboardingNavigator() override;

    /**
     * @brief Показать приветственную страницу
     */
    void showWelcomePage();

    /**
     * @brief Email, который ввёл пользователь
     */
    QString email() const;

    /**
     * @brief Код подтверждения авторизации
     */
    QString confirmationCode() const;

    /**
     * @brief Показать страницу с информацией об аккаунте
     */
    void showAccountPage();

    /**
     * @brief Установить параметры аккаунта
     */
    void setAccountInfo(const Domain::AccountInfo& _accountInfo);

    /**
     * @brief Тема выбранная пользователем на первом шаге
     * @return Светлая/тёмная/смешанная тема
     */
    Ui::ApplicationTheme getSelectedTheme() const;

signals:
    /**
     * @brief Пользователь выбрал язык приложения
     */
    void languageChanged(QLocale::Language _language);

    /**
     * @brief Пользователь выбрал тему приложения
     */
    void themeChanged(Ui::ApplicationTheme _theme);

    /**
     * @brief Пользователь выбрал приложение для импорта цветовой схемы
     */
    void competitorColorSchemeSelected(QString _color);

    /**
     * @brief Пользователь изменил масштаб интерфейса
     */
    void scaleFactorChanged(qreal _scaleFactor);

    /**
     * @brief Пользователь нажал кнопку входа в аккаунт
     */
    void signInPressed();

    /**
     * @brief Пользователь ввёл код подтверждения
     */
    void confirmationCodeChanged(const QString& _code);

    /**
     * @brief Пользователь обновил информацию об аккаунте
     */
    void accountInfoChanged(const QString& _email, const QString& _name,
                            const QString& _description, const QString& _subscriptionLanguage,
                            bool _subscribed, const QByteArray& _avatar);

    /**
     * @brief Пользователь хочет завершить настройку
     */
    void finishOnboardingRequested();

protected:
    /**
     * @brief Обновить перевод
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;

    Ui::ApplicationTheme selectedTheme;
};

} // namespace Ui
