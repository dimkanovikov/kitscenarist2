#pragma once

#include "abstract_comic_book_importer.h"


namespace BusinessLayer {

/**
 * @brief Импортер комикса из текстового файла
 */
class CORE_LIBRARY_EXPORT PlainTextImporter : public AbstractComicBookImporter
{
public:
    PlainTextImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Document importComicBook(const ComicBookImportOptions& _options) const override;
    Document importComicBook(const QString& _text) const;
};

} // namespace BusinessLayer
