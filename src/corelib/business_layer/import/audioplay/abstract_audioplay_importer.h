#pragma once

#include <business_layer/import/abstract_importer.h>

#include <QVector>


namespace BusinessLayer {

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractAudioplayImporter : virtual public AbstractImporter
{
public:
    virtual ~AbstractAudioplayImporter() = default;

    /**
     * @brief Вспомогательная структура для хранения данных импортированной аудиопьесы
     */
    struct Audioplay {
        QString name;
        QString titlePage;
        QString synopsis;
        QString text;
    };

    /**
     * @brief Импорт аудиопьесы из заданного документа
     */
    virtual Audioplay importAudioplay(const ImportOptions* _options) const = 0;
};

} // namespace BusinessLayer
