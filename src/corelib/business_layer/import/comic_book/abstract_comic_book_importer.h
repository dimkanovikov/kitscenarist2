#pragma once

#include <business_layer/import/abstract_importer.h>

#include <QVector>


namespace BusinessLayer {

struct ImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractComicBookImporter : virtual public AbstractImporter
{
public:
    virtual ~AbstractComicBookImporter() = default;

    /**
     * @brief Вспомогательная структура для хранения данных импортированной аудиопьесы
     */
    struct ComicBook {
        QString name;
        QString titlePage;
        QString synopsis;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual ComicBook importComicBook(const ImportOptions* _options) const = 0;
};

} // namespace BusinessLayer
