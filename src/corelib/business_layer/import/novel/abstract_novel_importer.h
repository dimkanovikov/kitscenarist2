#pragma once

#include <QVector>

#include <corelib_global.h>


namespace BusinessLayer {

struct ImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractNovelImporter
{
public:
    virtual ~AbstractNovelImporter() = default;

    /**
     * @brief Вспомогательные структуры для хранения данных импортируемых документов
     */
    struct Document {
        QString name;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual Document importNovel(const ImportOptions* _options) const = 0;
};

} // namespace BusinessLayer
