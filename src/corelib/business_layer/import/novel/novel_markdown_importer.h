#pragma once

#include "abstract_novel_importer.h"

#include <corelib/business_layer/import/abstract_markdown_importer.h>


namespace BusinessLayer {

/**
 * @brief Импортер текста из markdown файла
 */
class CORE_LIBRARY_EXPORT NovelMarkdownImporter : public AbstractNovelImporter,
                                                  public AbstractMarkdownImporter
{
public:
    NovelMarkdownImporter();
    ~NovelMarkdownImporter() override;

    /**
     * @brief Импортировать роман
     */
    Document importNovel(const ImportOptions* _options) const override;

    /**
     * @brief Получить основной текст романа в формате xml из заданного текста
     */
    Document novelText(const QString& _text) const;
};

} // namespace BusinessLayer
