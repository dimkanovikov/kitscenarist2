#pragma once

#include "abstract_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT PdfExporter : public AbstractExporter
{
public:
    PdfExporter();

    /**
     * @brief Экспортировать сценарий в PDF
     */
    void exportTo(ScreenplayTextModel* _model, const ExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer
