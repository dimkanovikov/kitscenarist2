#include "fdx_importer.h"

#include "screenlay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamWriter>

#include <set>


namespace BusinessLayer {

AbstractScreenplayImporter::Documents FdxImporter::importDocuments(
    const ScreenplayImportOptions& _options) const
{
    //
    // Открываем файл
    //
    QFile fdxFile(_options.filePath);
    if (!fdxFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Читаем XML
    //
    QDomDocument fdxDocument;
    fdxDocument.setContent(&fdxFile);

    //
    // Content - текст сценария
    //
    QDomElement rootElement = fdxDocument.documentElement();
    QDomElement content = rootElement.firstChildElement("Content");
    QDomNode paragraph = content.firstChild();
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    while (!paragraph.isNull()) {
        //
        // Определим тип блока
        //
        const QString paragraphType = paragraph.attributes().namedItem("Type").nodeValue();
        auto blockType = ScreenplayParagraphType::Undefined;
        if (paragraphType == "Scene Heading") {
            blockType = ScreenplayParagraphType::SceneHeading;
        } else if (paragraphType == "Character") {
            blockType = ScreenplayParagraphType::Character;
        }

        //
        // Получим текст блока
        //
        QString paragraphText;
        {
            QDomElement textNode = paragraph.firstChildElement("Text");
            while (!textNode.isNull()) {
                //
                // ... читаем текст
                //
                if (!textNode.text().isEmpty()) {
                    paragraphText.append(textNode.text());
                } else {
                    //
                    // NOTE: Qt пропускает узлы содержащие только пробельные символы,
                    //       поэтому прибегнем к небольшому воркэраунду
                    //
                    paragraphText.append(" ");
                }

                textNode = textNode.nextSiblingElement("Text");
            }
        }

        switch (blockType) {
        case ScreenplayParagraphType::SceneHeading: {
            if (!_options.importLocations) {
                break;
            }

            const auto locationName = SceneHeadingParser::location(paragraphText);
            if (locationName.isEmpty()) {
                break;
            }

            locationNames.emplace(locationName);
            break;
        }

        case ScreenplayParagraphType::Character: {
            if (!_options.importCharacters) {
                break;
            }

            const auto characterName = CharacterParser::name(paragraphText);
            if (characterName.isEmpty()) {
                break;
            }

            characterNames.emplace(characterName);
            break;
        }

        default:
            break;
        }

        //
        // Переходим к следующему
        //
        paragraph = paragraph.nextSibling();
    }

    Documents documents;
    for (const auto& characterName : characterNames) {
        documents.characters.append({ characterName, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ locationName, {} });
    }
    return documents;
}

QVector<AbstractScreenplayImporter::Screenplay> FdxImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    Screenplay result;
    result.name = QFileInfo(_options.filePath).completeBaseName();

    //
    // Открываем файл
    //
    QFile fdxFile(_options.filePath);
    if (!fdxFile.open(QIODevice::ReadOnly)) {
        return { result };
    }

    //
    // Читаем XML
    //
    QDomDocument fdxDocument;
    fdxDocument.setContent(&fdxFile);
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&result.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Content - текст сценария
    //
    QDomElement rootElement = fdxDocument.documentElement();
    QDomElement content = rootElement.firstChildElement("Content");
    QDomNode paragraph = content.firstChild();
    bool alreadyInScene = false;
    while (!paragraph.isNull()) {
        //
        // Определим тип блока
        //
        const QString paragraphType = paragraph.attributes().namedItem("Type").nodeValue();
        auto blockType = ScreenplayParagraphType::Action;
        if (paragraphType == "Scene Heading") {
            blockType = ScreenplayParagraphType::SceneHeading;
        } else if (paragraphType == "Action") {
            blockType = ScreenplayParagraphType::Action;
        } else if (paragraphType == "Character") {
            blockType = ScreenplayParagraphType::Character;
        } else if (paragraphType == "Parenthetical") {
            blockType = ScreenplayParagraphType::Parenthetical;
        } else if (paragraphType == "Dialogue") {
            blockType = ScreenplayParagraphType::Dialogue;
        } else if (paragraphType == "Transition") {
            blockType = ScreenplayParagraphType::Transition;
        } else if (paragraphType == "Shot") {
            blockType = ScreenplayParagraphType::Shot;
        } else if (paragraphType == "Cast List") {
            blockType = ScreenplayParagraphType::SceneCharacters;
        } else if (paragraphType == "Lyrics") {
            blockType = ScreenplayParagraphType::Lyrics;
        }

        //
        // Получим текст блока
        //
        QString paragraphText;
        QVector<ScreenplayTextModelTextItem::TextFormat> formats;
        {
            QDomElement textNode = paragraph.firstChildElement("Text");
            while (!textNode.isNull()) {
                //
                // ... читаем форматирование
                //
                if (textNode.hasAttribute("Style")) {
                    const QString style = textNode.attribute("Style");
                    ScreenplayTextModelTextItem::TextFormat format;
                    format.from = paragraphText.length();
                    format.length = textNode.text().length();
                    format.isBold = style.contains("Bold");
                    format.isItalic = style.contains("Italic");
                    format.isUnderline = style.contains("Underline");
                    if (format.isValid()) {
                        formats.append(format);
                    }
                }
                //
                // ... читаем текст
                //
                if (!textNode.text().isEmpty()) {
                    paragraphText.append(textNode.text());
                } else {
                    //
                    // NOTE: Qt пропускает узлы содержащие только пробельные символы,
                    //       поэтому прибегнем к небольшому воркэраунду
                    //
                    paragraphText.append(" ");
                }

                textNode = textNode.nextSiblingElement("Text");
            }

            //
            // Корректируем при необходимости
            //
            if (blockType == ScreenplayParagraphType::Parenthetical) {
                if (!paragraphText.isEmpty() && paragraphText.front() == "(") {
                    paragraphText.remove(0, 1);
                }
                if (!paragraphText.isEmpty() && paragraphText.back() == ")") {
                    paragraphText.chop(1);
                }
            }
        }

        //
        // По возможности получим цвет, заголовок и описание сцены
        //
        QString sceneColor;
        QString sceneTitle;
        QString sceneDescription;
        QDomElement sceneProperties = paragraph.firstChildElement("SceneProperties");
        if (!sceneProperties.isNull()) {
            if (sceneProperties.hasAttribute("Color")) {
                sceneColor = QColor(sceneProperties.attribute("Color")).name();
            }

            if (sceneProperties.hasAttribute("Title")) {
                sceneTitle = sceneProperties.attribute("Title");
            }

            QDomElement summary = sceneProperties.firstChildElement("Summary");
            if (!summary.isNull()) {
                QDomElement summaryParagraph = summary.firstChildElement("Paragraph");
                while (!summaryParagraph.isNull()) {
                    QDomElement textNode = summaryParagraph.firstChildElement("Text");
                    while (!textNode.isNull()) {
                        sceneDescription.append(textNode.text());
                        textNode = textNode.nextSiblingElement("Text");
                    }
                    summaryParagraph = summaryParagraph.nextSiblingElement("Paragraph");
                }
            }
        }

        //
        // Формируем блок сценария
        //
        if (blockType == ScreenplayParagraphType::SceneHeading) {
            if (alreadyInScene) {
                writer.writeEndElement(); // контент предыдущей сцены
                writer.writeEndElement(); // предыдущая сцена
            }
            alreadyInScene = true;

            writer.writeStartElement(xml::kSceneTag);
            writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
            writer.writeStartElement(xml::kContentTag);
            if (!sceneColor.isEmpty()) {
                //
                // TODO:
                //
            }
            if (!sceneTitle.isEmpty()) {
                //
                // TODO:
                //
            }
            if (!sceneDescription.isEmpty()) {
                //
                // TODO:
                //
            }
        }
        writer.writeStartElement(toString(blockType));
        writer.writeStartElement(xml::kValueTag);
        writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
        writer.writeEndElement(); // value
        if (!formats.isEmpty()) {
            writer.writeStartElement(xml::kFormatsTag);
            for (const auto& format : std::as_const(formats)) {
                writer.writeStartElement(xml::kFormatTag);
                //
                // Данные пользовательского форматирования
                //
                writer.writeAttribute(xml::kFromAttribute, QString::number(format.from));
                writer.writeAttribute(xml::kLengthAttribute, QString::number(format.length));
                writer.writeAttribute(xml::kBoldAttribute, format.isBold ? "true" : "false");
                writer.writeAttribute(xml::kItalicAttribute, format.isItalic ? "true" : "false");
                writer.writeAttribute(xml::kUnderlineAttribute,
                                      format.isUnderline ? "true" : "false");
                //
                writer.writeEndElement(); // format
            }
            writer.writeEndElement(); // formats
        }
        writer.writeEndElement(); // block type

        //
        // Переходим к следующему
        //
        paragraph = paragraph.nextSibling();
    }
    writer.writeEndDocument();

    return { result };
}

} // namespace BusinessLayer
