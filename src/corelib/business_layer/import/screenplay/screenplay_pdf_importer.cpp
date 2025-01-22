#include "screenplay_pdf_importer.h"

#include "TableExtraction.h"
#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QFileInfo>
#include <QTextBlock>
#include <QTextDocument>
#include <QXmlStreamWriter>


namespace BusinessLayer {

ScreenplayPdfImporter::ScreenplayPdfImporter()
    : AbstractScreenplayImporter()
    , AbstractDocumentImporter()
{
}

ScreenplayPdfImporter::~ScreenplayPdfImporter() = default;


QVector<AbstractScreenplayImporter::Screenplay> ScreenplayPdfImporter::importScreenplays(
    const ScreenplayImportOptions* _options) const
{
    Screenplay screenplay;
    screenplay.name = QFileInfo(_options->filePath).completeBaseName();

    if (_options->importText == false) {
        return { screenplay };
    }

    //
    // Преобразуем заданный документ в QTextDocument и парсим его
    //
    if (QTextDocument document; documentForImport(_options->filePath, document)) {
        screenplay.text = parseDocument(_options, document);
    }

    return { screenplay };
}

bool ScreenplayPdfImporter::documentForImport(const QString& _filePath,
                                              QTextDocument& _document) const
{
    QFile documentFile(_filePath);
    if (documentFile.open(QIODevice::ReadOnly)) {
        //
        // Используем TableExtraction, чтобы извлечь не только текст, но и линии
        //
        TableExtraction tableExtractor;
        tableExtractor.ExtractTables(_filePath.toStdString(), 0, -1, false);
        tableExtractor.GetResultsAsDocument(_document);
        return true;
    }
    return false;
}

void ScreenplayPdfImporter::writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const
{
    const QTextBlock currentBlock = _cursor.block();
    if (!currentBlock.textFormats().isEmpty()) {
        _writer.writeStartElement(xml::kReviewMarksTag);
        for (const auto& range : currentBlock.textFormats()) {
            if (range.format.hasProperty(QTextFormat::BackgroundBrush)
                || range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                _writer.writeStartElement(xml::kReviewMarkTag);
                _writer.writeAttribute(xml::kFromAttribute, QString::number(range.start));
                _writer.writeAttribute(xml::kLengthAttribute, QString::number(range.length));
                if (range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                    _writer.writeAttribute(xml::kColorAttribute,
                                           range.format.foreground().color().name());
                }
                if (range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                    _writer.writeAttribute(xml::kBackgroundColorAttribute,
                                           range.format.background().color().name());
                }
                //
                // Пишем пустой комментарий
                //
                _writer.writeStartElement(xml::kCommentTag);
                _writer.writeAttribute(
                    xml::kAuthorAttribute,
                    DataStorageLayer::StorageFacade::settingsStorage()->accountName());
                _writer.writeAttribute(xml::kDateAttribute,
                                       QDateTime::currentDateTime().toString(Qt::ISODate));
                _writer.writeCDATA(QString());
                _writer.writeEndElement(); // comment
                //
                _writer.writeEndElement(); // review mark
            }
        }
        _writer.writeEndElement(); // review marks
    }
}

bool ScreenplayPdfImporter::shouldKeepSceneNumbers(const ImportOptions* _options) const
{
    const auto options = static_cast<const ScreenplayImportOptions*>(_options);
    return options->keepSceneNumbers;
}

QString ScreenplayPdfImporter::characterName(const QString& _text) const
{
    return ScreenplayCharacterParser::name(_text);
}

QString ScreenplayPdfImporter::locationName(const QString& _text) const
{
    return ScreenplaySceneHeadingParser::location(_text);
}

} // namespace BusinessLayer
