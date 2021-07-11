#pragma once

#include <QVector>

#include <corelib_global.h>


namespace BusinessLayer {

struct TextImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractTextImporter
{
public:
    virtual ~AbstractTextImporter() = default;

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
    virtual Document importDocument(const TextImportOptions& _options) const = 0;
};

} // namespace BusinessLayer
