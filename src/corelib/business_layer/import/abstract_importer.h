#pragma once

#include <domain/document_object.h>

#include <corelib_global.h>


namespace BusinessLayer {

struct ImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractImporter
{
public:
    virtual ~AbstractImporter() = default;

    /**
     * @brief Вспомогательные структуры для хранения данных импортируемых документов
     */
    struct Document {
        Domain::DocumentObjectType type = Domain::DocumentObjectType::Undefined;
        QString name;
        QString content;
        QVector<Document> children;

        int id = 0;
    };
    struct Documents {
        //
        // <Название, контент>
        //
        QVector<Document> characters;
        QVector<Document> locations;
        QVector<Document> research;
    };

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    virtual Documents importDocuments(const ImportOptions* _options) const = 0;
};

} // namespace BusinessLayer
