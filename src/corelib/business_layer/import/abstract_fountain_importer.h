#pragma once

#include "abstract_markdown_importer.h"

#include <business_layer/import/screenplay/abstract_screenplay_importer.h>

namespace BusinessLayer {

enum class TextParagraphType;


/**
 * @brief Абстрактный импортер из формата fountain
 */
class CORE_LIBRARY_EXPORT AbstractFountainImporter : virtual public AbstractImporter,
                                                     public AbstractMarkdownImporter
{
public:
    AbstractFountainImporter(const QSet<TextParagraphType>& _possibleBlockTypes,
                             const TextParagraphType& _defaultBlockType);
    ~AbstractFountainImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions* _options) const override;

protected:
    /**
     * @brief Получить основной текст документа в формате xml из заданного текста
     */
    QString documentText(const QString& _text, bool _keepSceneNumbers = false) const;

    /**
     * @brief Определить тип блока
     */
    virtual TextParagraphType blockType(QString& _paragraphText) const;

    /**
     * @brief Записать данные блока
     */
    virtual void writeBlock(const QString& _paragraphText, TextParagraphType _type,
                            QXmlStreamWriter& _writer) const;

    /**
     * @brief Получить имя персонажа
     */
    virtual QString characterName(const QString& _text) const = 0;

    /**
     * @brief Получить название локации
     */
    virtual QString locationName(const QString& _text) const = 0;

    /**
     * @brief Помещать диалоги в таблицу
     */
    virtual bool placeDialoguesInTable() const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
