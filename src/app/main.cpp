#include "application.h"

#include <interfaces/management_layer/i_application_manager.h>

#include <QBreakpadHandler.h>
#include <QDebug>
#include <QDir>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QtPlugin>

using ManagementLayer::IApplicationManager;

/**
 * @brief Загрузить менеджер приложения
 */
QObject* loadApplicationManager()
{
    const auto staticInstances = QPluginLoader::staticInstances();
    for (QObject* plugin : staticInstances) {
        if (qobject_cast<IApplicationManager*>(plugin)) {
            return plugin;
        }
    }


    //
    // Смотрим папку с данными приложения на компе
    // NOTE: В Debug-режим работает с папкой сборки приложения
    //
    // TODO: Когда дорастём включить этот функционал, плюс продумать, как быть в режиме разработки
    //
    const QString pluginsDirName = "plugins";
    QDir pluginsDir(
        //#ifndef QT_NO_DEBUG
        QApplication::applicationDirPath()
        //#else
        //                QStandardPaths::writableLocation(QStandardPaths::DataLocation)
        //#endif
    );

#if defined(Q_OS_MAC)
    pluginsDir.cdUp();
#endif

    //
    // Если там нет плагинов приложения
    //
    if (!pluginsDir.cd(pluginsDirName)) {
        //
        // ... создаём локальную папку с плагинами
        //
        pluginsDir.mkpath(pluginsDir.absoluteFilePath(pluginsDirName));
        pluginsDir.cd(pluginsDirName);
        //
        // ... и копируем туда все плагины из папки, в которую установлено приложение
        //
        QDir installedPluginsDir(QApplication::applicationDirPath());
        installedPluginsDir.cd(pluginsDirName);
        for (const auto& file : installedPluginsDir.entryList(QDir::Files)) {
            QFile::copy(installedPluginsDir.absoluteFilePath(file),
                        pluginsDir.absoluteFilePath(file));
        }
    }
    //
    // Если плагины есть и если есть обновления
    //
    else {
        //
        // ... корректируем названия файлов для использования обновлённых версий
        //
        for (const QFileInfo& fileName : pluginsDir.entryInfoList({ "*.update" }, QDir::Files)) {
            QFile::rename(fileName.absoluteFilePath(),
                          fileName.absoluteFilePath().remove(".update"));
        }
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
    const QStringList libCorePluginEntries
        = pluginsDir.entryList({ "*coreplugin*" + extensionFilter }, QDir::Files);
    if (libCorePluginEntries.isEmpty()) {
        qCritical() << "Core plugin isn't found";
        return nullptr;
    }
    if (libCorePluginEntries.size() > 1) {
        qCritical() << "Found more than 1 core plugins";
        return nullptr;
    }

    const auto pluginPath = libCorePluginEntries.first();
    QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(pluginPath));
    QObject* plugin = pluginLoader.instance();
    if (plugin == nullptr) {
        qDebug() << pluginLoader.errorString();
    }

    return plugin;
}

Q_IMPORT_PLUGIN(ApplicationManager)
Q_IMPORT_PLUGIN(ScreenplayTextManager)

/**
 * @brief Погнали!
 */
int main(int argc, char* argv[])
{
    //
    // Инициилизируем приложение
    //
    Application application(argc, argv);

    //
    // Загружаем менеджера приложения
    //
    auto applicationManager = loadApplicationManager();
    if (applicationManager == nullptr) {
        return 1;
    }

    //
    // Настраиваем сборщик крашдампов
    //
    const auto crashReportsFolderPath
        = QString("%1/crashreports")
              .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QBreakpadInstance.init(
        crashReportsFolderPath,
        qobject_cast<ManagementLayer::IApplicationManager*>(applicationManager)->logFilePath());

    //
    // Устанавливаем менеджера в приложение и запускаемся
    //
    application.setApplicationManager(applicationManager);
    application.startUp();
    return application.exec();
}
