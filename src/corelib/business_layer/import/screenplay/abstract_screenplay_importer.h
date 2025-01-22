#pragma once

#include <business_layer/import/abstract_importer.h>

#include <QVector>


namespace BusinessLayer {

struct ScreenplayImportOptions;

/**
 * @brief Абстрактный класс для реализации импортера сценариев
 */
class CORE_LIBRARY_EXPORT AbstractScreenplayImporter : virtual public AbstractImporter
{
public:
    virtual ~AbstractScreenplayImporter() = default;

    /**
     * @brief Вспомогательная структура для хранения данных импортированного сценария
     */
    struct Screenplay {
        QString name;
        QString header;
        QString footer;
        QString logline;
        QString titlePage;
        QString synopsis;
        QString treatment;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual QVector<Screenplay> importScreenplays(
        const ScreenplayImportOptions* _options) const = 0;
};

} // namespace BusinessLayer
