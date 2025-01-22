#include "screenplay_docx_importer.h"

#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include <format_helpers.h>
#include <format_manager.h>
#include <format_reader.h>


namespace BusinessLayer {

ScreenplayDocxImporter::ScreenplayDocxImporter()
    : AbstractScreenplayImporter()
    , AbstractDocumentImporter()
{
}

ScreenplayDocxImporter::~ScreenplayDocxImporter() = default;


QVector<AbstractScreenplayImporter::Screenplay> ScreenplayDocxImporter::importScreenplays(
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

bool ScreenplayDocxImporter::documentForImport(const QString& _filePath,
                                               QTextDocument& _document) const
{
    QFile documentFile(_filePath);
    if (documentFile.open(QIODevice::ReadOnly)) {
        QScopedPointer<FormatReader> reader(FormatManager::createReader(&documentFile));
        reader->read(&documentFile, &_document);
        return true;
    }
    return false;
}

void ScreenplayDocxImporter::writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const
{
    const QTextBlock currentBlock = _cursor.block();
    if (!currentBlock.textFormats().isEmpty()) {
        _writer.writeStartElement(xml::kReviewMarksTag);
        for (const auto& range : currentBlock.textFormats()) {
            if (range.format.boolProperty(Docx::IsForeground)
                || range.format.boolProperty(Docx::IsBackground)
                || range.format.boolProperty(Docx::IsHighlight)
                || range.format.boolProperty(Docx::IsComment)) {
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
                // ... комментарии
                //
                QStringList authors = range.format.property(Docx::CommentsAuthors).toStringList();
                if (authors.isEmpty()) {
                    authors.append(
                        DataStorageLayer::StorageFacade::settingsStorage()->accountName());
                }
                QStringList dates = range.format.property(Docx::CommentsDates).toStringList();
                if (dates.isEmpty()) {
                    dates.append(QDateTime::currentDateTime().toString(Qt::ISODate));
                }
                QStringList comments = range.format.property(Docx::Comments).toStringList();
                if (comments.isEmpty()) {
                    comments.append(QString());
                }
                for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
                    _writer.writeStartElement(xml::kCommentTag);
                    _writer.writeAttribute(xml::kAuthorAttribute, authors.at(commentIndex));
                    _writer.writeAttribute(xml::kDateAttribute, dates.at(commentIndex));
                    _writer.writeCDATA(TextHelper::toHtmlEscaped(comments.at(commentIndex)));
                    _writer.writeEndElement(); // comment
                }
                //
                _writer.writeEndElement(); // review mark
            }
        }
        _writer.writeEndElement(); // review marks
    }
}

bool ScreenplayDocxImporter::shouldKeepSceneNumbers(const ImportOptions* _options) const
{
    const auto options = static_cast<const ScreenplayImportOptions*>(_options);
    return options->keepSceneNumbers;
}

QString ScreenplayDocxImporter::characterName(const QString& _text) const
{
    return ScreenplayCharacterParser::name(_text);
}

QString ScreenplayDocxImporter::locationName(const QString& _text) const
{
    return ScreenplaySceneHeadingParser::location(_text);
}

} // namespace BusinessLayer
