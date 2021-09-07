#include "plain_text_importer.h"

#include "comic_book_import_options.h"

#include <business_layer/model/comic_book/text/comic_book_text_model_xml.h>
#include <business_layer/templates/comic_book_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QXmlStreamWriter>

namespace BusinessLayer {

AbstractComicBookImporter::Document PlainTextImporter::importComicBook(
    const ComicBookImportOptions& _options) const
{
    //
    // Открываем файл
    //
    QFile textFile(_options.filePath);
    if (!textFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Импортируем
    //
    Document textDocument = importComicBook(textFile.readAll());
    if (textDocument.name.isEmpty()) {
        textDocument.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return textDocument;
}

AbstractComicBookImporter::Document PlainTextImporter::importComicBook(const QString& _text) const
{
    if (_text.simplified().isEmpty()) {
        return {};
    }

    Document textDocument;

    //
    // Читаем plain text
    //
    // ... и пишем в документ
    //
    QXmlStreamWriter writer(&textDocument.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ComicBookText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    const QStringList paragraphs = QString(_text).split("\n");
    for (const auto& paragraph : paragraphs) {
        writer.writeStartElement(toString(ComicBookParagraphType::Description));
        writer.writeStartElement(xml::kValueTag);
        writer.writeCDATA(TextHelper::toHtmlEscaped(paragraph));
        writer.writeEndElement();
        writer.writeEndElement();
    }
    writer.writeEndDocument();

    return textDocument;
}

} // namespace BusinessLayer
