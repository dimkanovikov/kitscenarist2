#pragma once

#include "abstract_screenplay_importer.h"

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов КИТа
 */
class CORE_LIBRARY_EXPORT ScreenplayKitScenaristImporter : public AbstractScreenplayImporter
{
public:
    ScreenplayKitScenaristImporter();
    ~ScreenplayKitScenaristImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions* _options) const override;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions* _options) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
