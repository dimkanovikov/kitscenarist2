#pragma once

#include <interfaces/management_layer/i_application_manager.h>

#include <QObject>

#include <core_global.h>


namespace ManagementLayer {

/**
 * @brief Менеджер приложения
 */
class CORE_PLUGIN_EXPORT ApplicationManager : public QObject, IApplicationManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IApplicationManager" FILE
                          "applicationmanager.json")
    Q_INTERFACES(ManagementLayer::IApplicationManager)

public:
    explicit ApplicationManager(QObject* _parent = nullptr);
    ~ApplicationManager() override;

    /**
     * @brief Получить путь к файлу с логами
     */
    QString logFilePath() const override;

    /**
     * @brief Запуск приложения
     */
    void exec(const QString& _fileToOpenPath) override final;

    /**
     * @brief Открытие файла
     */
    bool openProject(const QString& _path) override final;

protected:
    /**
     * @brief Переопределяем для обработки кастомных событий
     */
    bool event(QEvent* _event) override;

private:
    /**
     * @brief Настроить соединнеия
     */
    void initConnections();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
