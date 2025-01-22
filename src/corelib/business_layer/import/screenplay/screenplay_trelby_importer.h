#pragma once

#include "abstract_screenplay_importer.h"


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Trelby
 */
class CORE_LIBRARY_EXPORT ScreenplayTrelbyImporter : public AbstractScreenplayImporter
{
public:
    ScreenplayTrelbyImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions* _options) const override;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions* _options) const override;
};

} // namespace BusinessLayer
