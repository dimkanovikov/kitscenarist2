#include "novel_markdown_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/text_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QTextCursor>
#include <QXmlStreamWriter>

namespace BusinessLayer {

namespace {

/**
 * @brief Регулярное выражение для поиска любого типа выделения текста
 */
static const QRegularExpression kSelectionTypeChecker(
    "(^|[^\\\\])(?<format>(\\*\\*)|(__)|(\\*)|(_)|(~~))");

/**
 * @brief Имя группы захвата форматных символов
 */
static const QString kCapturedGroup("format");

/**
 * @brief Возможные типы выделения текста в markdown
 */
static const QMap<QString, QLatin1String> kMarkdownSelectionTypes({
    { "**", xml::kBoldAttribute },
    { "__", xml::kBoldAttribute },
    { "*", xml::kItalicAttribute },
    { "_", xml::kItalicAttribute },
    { "~~", xml::kStrikethroughAttribute },
});

/**
 * @brief Регулярное выражение для поиска символа экранирования
 */
static const QRegularExpression kEscapeingSymbolChecker("(^|[^\\\\])(\\\\)([^\\\\])");


/**
 * @brief Очистить параграф от символов экранирования
 */
static void removeEscapeingSymbol(QString& _paragraphText)
{
    QRegularExpressionMatch match = kEscapeingSymbolChecker.match(_paragraphText);
    while (match.hasMatch()) {
        _paragraphText.remove(match.capturedStart(2), 1);
        match = kEscapeingSymbolChecker.match(_paragraphText);
    }
}

} // namespace

NovelMarkdownImporter::NovelMarkdownImporter()
    : AbstractNovelImporter()
    , AbstractMarkdownImporter(kMarkdownSelectionTypes, kSelectionTypeChecker, kCapturedGroup)
{
}

NovelMarkdownImporter::~NovelMarkdownImporter() = default;

AbstractNovelImporter::Document NovelMarkdownImporter::importNovel(
    const ImportOptions* _options) const
{
    //
    // Открываем файл
    //
    QFile textFile(_options->filePath);
    if (!textFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Импортируем
    //
    Document textDocument = novelText(textFile.readAll());
    if (textDocument.name.isEmpty()) {
        textDocument.name = QFileInfo(_options->filePath).completeBaseName();
    }

    return textDocument;
}

AbstractNovelImporter::Document NovelMarkdownImporter::novelText(const QString& _text) const
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
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Novel));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    const QStringList paragraphs = QString(_text).remove('\r').split('\n');
    for (const auto& paragraph : paragraphs) {
        if (paragraph.simplified().isEmpty()) {
            continue;
        }

        auto paragraphText = paragraph;

        //
        // Собираем типы выделения текста
        //
        collectSelectionTypes(paragraphText);


        //
        // Обрабатываем блоки
        //
        if (paragraph.startsWith("#")) {
            const QString stringBegin = paragraph.split(' ')[0];
            //
            // Если начало строки состоит только из '#'
            //
            if (!stringBegin.contains(QRegularExpression("[^#]"))) {
                //
                // ... то определяем уровень заголовка
                //
                const int headingLevel = stringBegin.count('#');
                switch (headingLevel) {
                case 1: {
                    paragraphText.remove(0, headingLevel + 1);
                    movePreviousTypes(0, headingLevel + 1);
                    writer.writeStartElement(toString(TextParagraphType::PartHeading));
                    break;
                }
                case 2:
                case 3:
                case 4:
                case 5: {
                    paragraphText.remove(0, headingLevel + 1);
                    movePreviousTypes(0, headingLevel + 1);
                    writer.writeStartElement(toString(TextParagraphType::ChapterHeading));
                    break;
                }
                case 6: {
                    paragraphText.remove(0, headingLevel + 1);
                    movePreviousTypes(0, headingLevel + 1);
                    writer.writeStartElement(toString(TextParagraphType::SceneHeading));
                    break;
                }
                default: {
                    writer.writeStartElement(toString(TextParagraphType::Text));
                    break;
                }
                }
            } else {
                writer.writeStartElement(toString(TextParagraphType::Text));
            }

            writer.writeStartElement(xml::kValueTag);
            removeEscapeingSymbol(paragraphText);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
            writer.writeEndElement(); // value
        } else {
            writer.writeStartElement(toString(TextParagraphType::Text));
            writer.writeStartElement(xml::kValueTag);
            removeEscapeingSymbol(paragraphText);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
            writer.writeEndElement(); // value
        }

        //
        // Пишем данные о типах выделения текста
        //
        writeSelectionTypes(writer);

        writer.writeEndElement(); // block type
    }
    writer.writeEndDocument();

    return textDocument;
}

} // namespace BusinessLayer
