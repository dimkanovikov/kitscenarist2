#pragma once

#include "abstract_screenplay_importer.h"
#include "business_layer/import/abstract_document_importer.h"

class QTextCursor;

namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Docx
 */
class CORE_LIBRARY_EXPORT ScreenplayDocxImporter : public AbstractScreenplayImporter,
                                                   public AbstractDocumentImporter
{
public:
    ScreenplayDocxImporter();
    ~ScreenplayDocxImporter() override;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions* _options) const override;

protected:
    /**
     * @brief Получить документ для импорта
     * @return true, если получилось открыть заданный файл
     */
    bool documentForImport(const QString& _filePath, QTextDocument& _document) const override;

    /**
     * @brief Записать редакторские заметки
     */
    void writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const override;

    /**
     * @brief Следует ли сохранять номера сцен
     */
    bool shouldKeepSceneNumbers(const ImportOptions* _options) const override;

    /**
     * @brief Получить имя персонажа
     */
    QString characterName(const QString& _text) const override;

    /**
     * @brief Получить название локации
     */
    QString locationName(const QString& _text) const override;
};

} // namespace BusinessLayer
