#pragma once

#include "abstract_comic_book_importer.h"

#include <business_layer/import/abstract_fountain_importer.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер комиксов из файлов fountain
 */
class CORE_LIBRARY_EXPORT ComicBookFountainImporter : public AbstractComicBookImporter,
                                                      public AbstractFountainImporter
{
public:
    ComicBookFountainImporter();
    ~ComicBookFountainImporter() override;

    /**
     * @brief Импортировать комикс
     */
    ComicBook importComicBook(const ImportOptions* _options) const override;

    /**
     * @brief Получить основной текст комикса в формате xml из заданного текста
     */
    ComicBook comicbookText(const QString& _comicbookText) const;

protected:
    /**
     * @brief Получить имя персонажа
     */
    QString characterName(const QString& _text) const override;

    /**
     * @brief Получить название локации
     */
    QString locationName(const QString& _text) const override;

    /**
     * @brief Определить тип блока
     */
    TextParagraphType blockType(QString& _paragraphText) const override;

    /**
     * @brief Записать данные блока
     */
    void writeBlock(const QString& _paragraphText, TextParagraphType _type,
                    QXmlStreamWriter& _writer) const override;

    /**
     * @brief Помещать диалоги в таблицу
     */
    bool placeDialoguesInTable() const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
