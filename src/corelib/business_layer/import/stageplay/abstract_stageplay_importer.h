#pragma once

#include <business_layer/import/abstract_importer.h>

#include <QVector>


namespace BusinessLayer {

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractStageplayImporter : virtual public AbstractImporter
{
public:
    virtual ~AbstractStageplayImporter() = default;

    /**
     * @brief Вспомогательная структура для хранения данных импортированной пьесы
     */
    struct Stageplay {
        QString name;
        QString titlePage;
        QString synopsis;
        QString text;
    };

    /**
     * @brief Импорт пьесы из заданного документа
     */
    virtual Stageplay importStageplay(const ImportOptions* _options) const = 0;
};

} // namespace BusinessLayer
