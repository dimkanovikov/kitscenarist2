#pragma once

#include <QLocale>
#include <QObject>

namespace Ui {
enum class ApplicationTheme;
}

namespace Domain {
struct AccountInfo;
}


namespace ManagementLayer {

/**
 * @brief Менеджер посадочного экрана
 */
class OnboardingManager : public QObject
{
    Q_OBJECT

public:
    OnboardingManager(QObject* _parent, QWidget* _parentWidget);
    ~OnboardingManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Показать начальную страницу онбординга
     */
    void showWelcomePage();

    /**
     * @brief Задать параметры кода активации
     */
    void setConfirmationCodeInfo(int _codeLength);

    /**
     * @brief Завершить авторизацию (если это новый пользователь, то необходимо перейти в кабинет)
     */
    void completeSignIn();

    /**
     * @brief Установить параметры аккаунта
     */
    void setAccountInfo(const Domain::AccountInfo& _accountInfo);

signals:
    //
    // Страница параметров внешнего вида
    //

    /**
     * @brief Пользователь выбрал язык приложения
     */
    void languageChanged(QLocale::Language _language);

    /**
     * @brief Пользователь выбрал тему приложения
     */
    void themeChanged(Ui::ApplicationTheme _theme);

    /**
     * @brief Пользователь выбрал тему приложения как у конкурента
     */
    void useCustomThemeRequested(const QString& _hash);

    /**
     * @brief Пользователь изменил масштаб интерфейса
     */
    void scaleFactorChanged(qreal _scaleFactor);

    //
    // Страница авторизации
    //

    /**
     * @brief Email для авторизации был введён пользователем
     */
    void askConfirmationCodeRequested(const QString& _email);

    /**
     * @brief Код проверки авторизации был введён пользователем
     */
    void checkConfirmationCodeRequested(const QString& _code);

    //
    // Страница с информацией об аккаунте
    //

    /**
     * @brief Пользователь хочет обновить информацию об аккаунте
     */
    void updateAccountInfoRequested(const QString& _email, const QString& _name,
                                    const QString& _description,
                                    const QString& _subscriptionLanguage, bool _subscribed,
                                    const QByteArray& _avatar);

    //
    // Последняя страница
    //

    /**
     * @brief Посадка окончена
     */
    void finished();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
