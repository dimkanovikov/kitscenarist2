#include "abstract_fountain_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStack>

#include <set>

namespace BusinessLayer {

namespace {

/**
 * @brief Регулярное выражение для поиска любого типа выделения текста
 */
static const QRegularExpression kSelectionTypeChecker(
    "(^|[^\\\\])(?<format>((?<!\\/)\\*\\*)|((?<!\\/)\\*)|(_))");

/**
 * @brief Имя группы захвата форматных символов
 */
static const QString kCapturedGroup("format");

/**
 * @brief Возможные типы выделения текста в fountain
 */
static const QMap<QString, QLatin1String> kFountainSelectionTypes({
    { "**", xml::kBoldAttribute },
    { "*", xml::kItalicAttribute },
    { "_", xml::kUnderlineAttribute },
});

/**
 * @brief С чего может начинаться название сцены
 */
QStringList sceneHeadingsDictionary()
{
    return { QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "INT"),
             QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "EXT"),
             QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "EST"),
             QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "INT./EXT"),
             QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "INT/EXT"),
             QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "EXT./INT"),
             QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "EXT/INT"),
             QCoreApplication::translate("BusinessLayer::AbstractFountainImporter", "I/E"),

             //
             // Тот же список, но без переводов
             //
             "INT", "EXT", "EST", "INT./EXT", "INT/EXT", "EXT./INT", "EXT/INT", "I/E" };
}

/**
 * @brief Ключи титульной страницы
 */
QHash<QString, QString> titleKeysDictionary()
{
    return { std::make_pair(QString("Title"), QString("name")),
             std::make_pair(QString("Author"), QString("author")),
             std::make_pair(QString("Authors"), QString("author")),
             std::make_pair(QString("Draft date"), QString("year")),
             std::make_pair(QString("Copyright"), QString("year")),
             std::make_pair(QString("Contact"), QString("contacts")),
             std::make_pair(QString("Credit"), QString("genre")),
             std::make_pair(QString("Source"), QString("additional_info")) };
}

const QString kDoubleWhitespace = QLatin1String("  ");

} // namespace

class AbstractFountainImporter::Implementation
{
public:
    explicit Implementation(AbstractFountainImporter* _q,
                            const QSet<TextParagraphType>& _possibleBlockTypes,
                            const TextParagraphType& _defaultBlockType);

    /**
     * @brief Обработка конкретного блока перед его добавлением
     */
    void processBlock(const QString& _paragraphText, TextParagraphType _type,
                      QXmlStreamWriter& _writer);

    /**
     * @brief Добавление блока
     */
    void appendBlock(const QString& _paragraphText, TextParagraphType _type,
                     QXmlStreamWriter& _writer);

    /**
     * @brief Добавление комментариев к блоку
     */
    void appendComments(QXmlStreamWriter& _writer);

    /**
     * @brief Постобработка блока после его закрытия
     */
    void postProcessPreviousBlock(TextParagraphType _currentBlockType, QXmlStreamWriter& _writer);


    AbstractFountainImporter* q = nullptr;

    /**
     * @brief Начало позиции в блоке для потенциальной будущей редакторской заметки
     */
    unsigned noteStartPos = 0;

    /**
     * @brief Длина потенциальной будущей редакторской заметки
     */
    unsigned noteLen = 0;

    /**
     * @brief Идет ли сейчас редакторская заметка
     */
    bool isNotation = false;

    /**
     * @brief Идет ли сейчас комментарий
     */
    bool isCommenting = false;

    /**
     * @brief Является ли текущий блок первым
     */
    bool isFirstBlock = true;

    /**
     * @brief Зашли ли мы уже в сцену
     */
    bool alreadyInScene = false;

    /**
     * @brief Зашли ли мы уже в бит
     */
    bool alreadyInBeat = false;

    /**
     * @brief Идет ли сейчас двойной диалог
     */
    bool isDoubleDialogue = false;

    /**
     * @brief Идет ли сейчас первый персонаж в двойном диалоге
     */
    bool isFirstCharacter = false;

    /**
     * @brief Открыт ли разделитель параграфа
     */
    bool splitterIsOpen = false;

    /**
     * @brief Текст блока
     */
    QString blockText;

    /**
     * @brief Текст последнего сохранённого блока
     */
    QString lastBlockText;

    /**
     * @brief Текст редакторской заметки
     */
    QString note;

    /**
     * @brief Кастомный номер сцены
     */
    QString customSceneNumber;

    /**
     * @brief Редакторская заметка к текущему блоку
     * 		  tuple содержит комментарий, позиция и длина области редакторской заметки
     */
    QVector<std::tuple<QString, unsigned, unsigned>> notes;

    /**
     * @brief Типы блоков, которые могут быть в документе
     */
    const QSet<TextParagraphType> possibleBlockTypes;

    /**
     * @brief Тип блока по умолчанию
     */
    const TextParagraphType defaultBlockType = TextParagraphType::Undefined;
};

AbstractFountainImporter::Implementation::Implementation(
    AbstractFountainImporter* _q, const QSet<TextParagraphType>& _possibleBlockTypes,
    const TextParagraphType& _defaultBlockType)
    : q(_q)
    , possibleBlockTypes(_possibleBlockTypes)
    , defaultBlockType(_defaultBlockType)
{
}

void AbstractFountainImporter::Implementation::processBlock(const QString& _paragraphText,
                                                            TextParagraphType _type,
                                                            QXmlStreamWriter& _writer)
{
    //
    // Начинается новая сущность
    //
    if (!isNotation && !isCommenting) {
        blockText.reserve(_paragraphText.size());

        //
        // Добавим комментарии к предыдущему блоку
        //
        appendComments(_writer);

        noteLen = 0;
        noteStartPos = 0;
    }
    //
    // Продолжается комментарий или заметка
    //
    else {
        if (isCommenting) {
            blockText.append(QChar::LineSeparator);
        } else {
            note.append(QChar::LineFeed);
        }
    }

    //
    // Комментарии и заметки
    //
    char prevSymbol = '\0';
    for (int i = 0; i != _paragraphText.size(); ++i) {
        //
        // Если предыдущий символ - \, то просто добавим текущий
        //
        if (prevSymbol == '\\') {
            if (isNotation) {
                note.append(_paragraphText[i]);
            } else {
                blockText.append(_paragraphText[i]);
            }
            //
            // Раз мы добавляем символ через '\',
            // то он не должен участвовать в какой-либо обработке
            //
            prevSymbol = '\0';
            continue;
        }

        char curSymbol = _paragraphText[i].toLatin1();
        switch (curSymbol) {
        case '\\': {
            q->movePreviousTypes(i, 1);
            break;
        }

        case '/': {
            if (prevSymbol == '*' && isCommenting) {
                //
                // Заканчивается комментирование
                //
                isCommenting = false;
                noteStartPos += noteLen;
                noteLen = blockText.size();

                //
                // Закроем предыдущий блок и сделаем его постобработку
                //
                if (!lastBlockText.isEmpty()) {
                    _writer.writeEndElement();
                    postProcessPreviousBlock(_type, _writer);
                }

                //
                // Добавим текущий
                //
                appendBlock(blockText.left(blockText.size()), TextParagraphType::InlineNote,
                            _writer);
                blockText.clear();
            } else {
                if (isNotation) {
                    note.append('/');
                } else {
                    blockText.append('/');
                }
            }
            break;
        }

        case '*': {
            if (prevSymbol == '/' && !isCommenting && !isNotation) {
                //
                // Начинается комментирование
                //
                isCommenting = true;
                noteStartPos += noteLen;
                noteLen = blockText.size() - 1;

                //
                // Закроем предыдущий блок, сделаем его постобработку и, если комментирование
                // начинается в середние текущего блока то добавим этот текущий блок
                //
                if (blockText.size() != 1) {
                    _writer.writeEndElement();
                    postProcessPreviousBlock(_type, _writer);
                    appendBlock(blockText.left(blockText.size() - 1), _type, _writer);
                    appendComments(_writer);
                    notes.clear();
                }
                blockText.clear();
            } else {
                if (isNotation) {
                    note.append('*');
                }
            }
            break;
        }

        case '[': {
            if (prevSymbol == '[' && !isCommenting && !isNotation) {
                //
                // Начинается редакторская заметка
                //
                isNotation = true;
                noteLen = blockText.size() - 1 - noteStartPos;
                blockText = blockText.left(blockText.size() - 1);
            } else {
                if (isNotation) {
                    note.append('[');
                } else {
                    blockText.append('[');
                }
            }
            break;
        }

        case ']': {
            if (prevSymbol == ']' && isNotation) {
                //
                // Закончилась редакторская заметка. Добавим ее в список редакторских заметок к
                // текущему блоку
                //
                isNotation = false;
                notes.append(std::make_tuple(note.left(note.size() - 1), noteStartPos, noteLen));
                noteStartPos += noteLen;
                noteLen = 0;
                note.clear();
            } else {
                if (isNotation) {
                    note.append(']');
                } else {
                    blockText.append(']');
                }
            }
            break;
        }

        default: {
            //
            // Самый обычный символ
            //
            if (isNotation) {
                note.append(_paragraphText[i]);
            } else {
                blockText.append(_paragraphText[i]);
            }
            break;
        }
        }

        prevSymbol = curSymbol;
    }

    if (!isNotation && !isCommenting) {
        //
        // Если блок действительно закончился
        //
        noteLen += blockText.size() - noteStartPos;

        //
        // Добавим текущий блок
        //
        if (!blockText.isEmpty() || _type == TextParagraphType::SequenceFooter) {
            //
            // ... но перед добавлением закроем предыдущий блок и сделаем его постобработку
            //
            if (!isFirstBlock) {
                _writer.writeEndElement();
                postProcessPreviousBlock(_type, _writer);
            }

            appendBlock(blockText, _type, _writer);
        }
        blockText.clear();
    }
}

void AbstractFountainImporter::Implementation::appendBlock(const QString& _paragraphText,
                                                           TextParagraphType _type,
                                                           QXmlStreamWriter& _writer)
{
    QString paragraphText = _paragraphText;
    while (!paragraphText.isEmpty() && paragraphText.startsWith(" ")) {
        paragraphText = paragraphText.mid(1);
    }

    //
    // Пишем текст блока
    //
    q->writeBlock(paragraphText, _type, _writer);

    //
    // Пишем форматирование, если оно есть
    //
    q->writeSelectionTypes(_writer);

    lastBlockText = blockText;

    //
    // Первый блок в тексте может встретиться лишь однажды
    //
    if (isFirstBlock) {
        isFirstBlock = false;
    }

    //
    // Не закрываем блок, чтобы можно было добавить редакторских заметок
    //
}

void AbstractFountainImporter::Implementation::appendComments(QXmlStreamWriter& _writer)
{
    if (notes.isEmpty()) {
        return;
    }

    _writer.writeStartElement(xml::kReviewMarksTag);

    for (int i = 0; i != notes.size(); ++i) {
        int endPos = std::get<2>(notes[i]);
        if (endPos == 0) {
            endPos = lastBlockText.length();
        }

        if (i != 0) {
            _writer.writeEndElement(); // review mark
        }
        _writer.writeStartElement(xml::kReviewMarkTag);
        _writer.writeAttribute(xml::kFromAttribute, QString::number(std::get<1>(notes[i])));
        _writer.writeAttribute(xml::kLengthAttribute, QString::number(endPos));
        _writer.writeAttribute(xml::kBackgroundColorAttribute, "#FFD302");

        _writer.writeStartElement(xml::kCommentTag);
        _writer.writeAttribute(xml::kAuthorAttribute, "fountain author");
        _writer.writeAttribute(xml::kDateAttribute,
                               QDateTime::currentDateTime().toString(Qt::ISODate));
        _writer.writeCDATA(TextHelper::toHtmlEscaped(std::get<0>(notes[i])));
        _writer.writeEndElement(); // comment
    }

    _writer.writeEndElement(); // review mark
    _writer.writeEndElement(); // review marks

    notes.clear();
}

void AbstractFountainImporter::Implementation::postProcessPreviousBlock(
    TextParagraphType _currentBlockType, QXmlStreamWriter& _writer)
{
    if (splitterIsOpen) {
        if (isDoubleDialogue) {
            if (_currentBlockType != TextParagraphType::Dialogue
                && _currentBlockType != TextParagraphType::Parenthetical
                && isFirstCharacter == false) {
                _writer.writeEmptyElement(xml::kSplitterTag);
                _writer.writeAttribute(xml::kTypeAttribute, "end");
                splitterIsOpen = false;
            }
        } else if (q->placeDialoguesInTable()) {
            if (_currentBlockType != TextParagraphType::Dialogue
                && _currentBlockType != TextParagraphType::Parenthetical) {
                _writer.writeEmptyElement(xml::kSplitterTag);
                _writer.writeAttribute(xml::kTypeAttribute, "end");
                splitterIsOpen = false;
            }
        }
    }
}


// ****


AbstractFountainImporter::AbstractFountainImporter(
    const QSet<TextParagraphType>& _possibleBlockTypes, const TextParagraphType& _defaultBlockType)
    : AbstractImporter()
    , AbstractMarkdownImporter(kFountainSelectionTypes, kSelectionTypeChecker, kCapturedGroup)
    , d(new Implementation(this, _possibleBlockTypes, _defaultBlockType))
{
}

AbstractFountainImporter::~AbstractFountainImporter() = default;

AbstractScreenplayImporter::Documents AbstractFountainImporter::importDocuments(
    const ImportOptions* _options) const
{
    //
    // Открываем файл
    //
    QFile fountainFile(_options->filePath);
    if (!fountainFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Читаем plain text
    //
    QString scriptText = fountainFile.readAll();

    //
    // Сформируем список строк, содержащий текст сценария
    //
    QVector<QString> paragraphs;
    bool isTitle = false;
    bool isFirstLine = true;
    for (const auto& str : scriptText.remove('\r').split("\n")) {
        //
        // Если первая строка содержит один из ключей титульной страницы, то в начале идет титульная
        // страница, которую мы обрабатываем не здесь
        //
        if (isFirstLine) {
            isFirstLine = false;
            for (const auto& titleKey : titleKeysDictionary().keys()) {
                if (str.startsWith(titleKey + ":")) {
                    isTitle = true;
                    break;
                }
            }
        }

        if (isTitle) {
            //
            // Титульная страница заканчивается пустой строкой
            //
            if (str.simplified().isEmpty()) {
                isTitle = false;
            }
        } else {
            if (str == kDoubleWhitespace) {
                //
                // Если строка состоит из 2 пробелов, то это нужно сохранить
                // Используется для многострочных диалогов с пустыми строками
                //
                paragraphs.push_back(kDoubleWhitespace);
            } else {
                paragraphs.push_back(str.simplified());
            }
        }
    }

    const int paragraphsCount = paragraphs.size();
    auto prevBlockType = TextParagraphType::Undefined;
    TextParagraphType blockType;
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    for (int i = 0; i != paragraphsCount; ++i) {
        if (paragraphs[i].isEmpty()) {
            continue;
        }

        blockType = TextParagraphType::Undefined;
        QString paragraphText;

        switch (paragraphs[i][0].toLatin1()) {
        case '.': {
            blockType = TextParagraphType::SceneHeading;
            //
            // Номера сцен игнорируем
            //
            int sharpPos = paragraphs[i].size();
            if (paragraphs[i].endsWith("#")) {
                sharpPos = paragraphs[i].lastIndexOf('#', paragraphs[i].size() - 2);
            }
            if (sharpPos == -1) {
                sharpPos = paragraphs[i].size();
            }
            paragraphText = paragraphs[i].mid(1, sharpPos - 1);
            break;
        }

        case '@': {
            blockType = TextParagraphType::Character;
            paragraphText = paragraphs[i].mid(1);
            break;
        }

        case '!':
        case '>':
        case '=':
        case '~':
        case '#': {
            continue;
        }

        default: {
            bool startsWithHeading = false;
            for (const QString& sceneHeading : sceneHeadingsDictionary()) {
                if (paragraphs[i].startsWith(sceneHeading)) {
                    startsWithHeading = true;
                    break;
                }
            }

            if (startsWithHeading && i + 1 < paragraphsCount && paragraphs[i + 1].isEmpty()) {
                //
                // Если начинается с одного из времен действия, а после обязательно пустая строка
                // Значит это заголовок сцены
                //
                blockType = TextParagraphType::SceneHeading;

                //
                // Номера сцен игнорируем
                //
                int sharpPos = paragraphs[i].size();
                if (paragraphs[i].endsWith("#")) {
                    sharpPos = paragraphs[i].lastIndexOf('#', paragraphs[i].size() - 2);
                }
                if (sharpPos == -1) {
                    sharpPos = paragraphs[i].size();
                }
                paragraphText = paragraphs[i].left(sharpPos);
            } else if (paragraphs[i].startsWith("[[") && paragraphs[i].endsWith("]]")) {
                continue;
            } else if (paragraphs[i].startsWith("/*")) {
                continue;
            } else if (paragraphs[i] == TextHelper::smartToUpper(paragraphs[i]) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && paragraphs[i + 1].isEmpty() && paragraphs[i].endsWith("TO:")) {
                continue;
            } else if (paragraphs[i].startsWith("(") && paragraphs[i].endsWith(")")
                       && (prevBlockType == TextParagraphType::Character
                           || prevBlockType == TextParagraphType::Dialogue)) {
                continue;
            } else if (paragraphs[i] == TextHelper::smartToUpper(paragraphs[i]) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && !paragraphs[i + 1].isEmpty()) {
                //
                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
                // пустая Значит это имя персонажа (для реплики)
                //
                blockType = TextParagraphType::Character;
                if (paragraphs[i].endsWith("^")) {
                    //
                    // Двойной диалог, который мы пока что не умеем обрабатывать
                    //
                    paragraphText = paragraphs[i].left(paragraphs[i].size() - 1);
                } else {
                    paragraphText = paragraphs[i];
                }
            } else {
                continue;
            }
        }
        }

        switch (blockType) {
        case TextParagraphType::SceneHeading: {
            if (!_options->importLocations) {
                break;
            }

            const auto currentLocationName = locationName(paragraphText);
            if (!currentLocationName.isEmpty()) {
                locationNames.emplace(currentLocationName);
            }
            break;
        }

        case TextParagraphType::Character: {
            if (!_options->importCharacters) {
                break;
            }

            const auto currentCharacterName = characterName(paragraphText);
            if (!currentCharacterName.isEmpty()) {
                characterNames.emplace(currentCharacterName);
            }
            break;
        }

        default:
            break;
        }

        //
        // И очищаем текст
        //
        paragraphText.clear();
    }

    Documents documents;
    for (const auto& characterName : characterNames) {
        documents.characters.append(
            { Domain::DocumentObjectType::Character, characterName, {}, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ Domain::DocumentObjectType::Location, locationName, {}, {} });
    }
    return documents;
}

QString AbstractFountainImporter::documentText(const QString& _text, bool _keepSceneNumbers) const
{
    if (_text.simplified().isEmpty()) {
        return {};
    }

    QString result;

    //
    // Читаем plain text
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&result);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Сформируем список строк, содержащий текст сценария
    //
    QVector<QString> paragraphs;
    bool isTitle = false;
    bool isFirstLine = true;
    for (const auto& str : QString(_text).remove('\r').split("\n")) {
        //
        // Если первая строка содержит один из ключей титульной страницы, то в начале идет титульная
        // страница, которую мы обрабатываем не здесь
        //
        if (isFirstLine) {
            isFirstLine = false;
            for (const auto& titleKey : titleKeysDictionary().keys()) {
                if (str.startsWith(titleKey + ":")) {
                    isTitle = true;
                    break;
                }
            }
        }

        if (isTitle) {
            //
            // Титульная страница заканчивается пустой строкой
            //
            if (str.simplified().isEmpty()) {
                isTitle = false;
            }
        } else {
            if (str == kDoubleWhitespace) {
                //
                // Если строка состоит из 2 пробелов, то это нужно сохранить
                // Используется для многострочных диалогов с пустыми строками
                //
                paragraphs.push_back(kDoubleWhitespace);
            } else {
                paragraphs.push_back(str.simplified());
            }
        }
    }

    const int paragraphsCount = paragraphs.size();
    QStack<QString> dirs;
    auto prevBlockType = TextParagraphType::Undefined;
    TextParagraphType currentBlockType = TextParagraphType::Undefined;

    for (int i = 0; i != paragraphsCount; ++i) {
        QString paragraphText = paragraphs[i];

        if (d->isNotation || d->isCommenting) {
            //
            // Если мы комментируем или делаем заметку, то продолжим это
            //
            d->processBlock(paragraphText, prevBlockType, writer);
            continue;
        }

        if (paragraphText.isEmpty()) {
            continue;
        }

        //
        // Собираем типы выделения текста и очищаем текст от форматных символов
        //
        collectSelectionTypes(paragraphText);

        //
        // Определяем тип блока
        //
        currentBlockType = blockType(paragraphText);

        //
        // Если не получилось определить тип блока, сделаем дополнительную проверку
        //
        if (currentBlockType == TextParagraphType::Undefined) {
            bool startsWithHeading = false;
            for (const QString& sceneHeading : sceneHeadingsDictionary()) {
                if (paragraphText.startsWith(sceneHeading)) {
                    startsWithHeading = true;
                    break;
                }
            }

            if (startsWithHeading && i + 1 < paragraphsCount && paragraphs[i + 1].isEmpty()) {
                //
                // Если начинается с одного из времен действия, а после обязательно пустая строка
                // Значит это заголовок сцены
                //
                currentBlockType = TextParagraphType::SceneHeading;
            } else if (paragraphText.startsWith("[[") && paragraphText.endsWith("]]")) {
                //
                // Редакторская заметка
                //
                paragraphText.chop(2);
                paragraphText.remove(0, 2);
                movePreviousTypes(0, 2);
                d->notes.append(std::make_tuple(paragraphText, d->noteStartPos, d->noteLen));
                d->noteStartPos += d->noteLen;
                d->noteLen = 0;
                continue;
            } else if (paragraphText.startsWith("/*")) {
                //
                // Начинается комментарий
                //
            } else if (paragraphText == TextHelper::smartToUpper(paragraphText) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && paragraphs[i + 1].isEmpty() && paragraphText.endsWith("TO:")) {
                //
                // Если состоит только из заглавных букв, предыдущая и следующая строки пустые
                // и заканчивается "TO:", то это переход
                //
                currentBlockType = TextParagraphType::Transition;
            } else if (paragraphText.startsWith("(") && paragraphText.endsWith(")")
                       && (prevBlockType == TextParagraphType::Character
                           || prevBlockType == TextParagraphType::Dialogue)) {
                //
                // Если текущий блок обернут в (), то это ремарка
                //
                currentBlockType = TextParagraphType::Parenthetical;
            } else if (paragraphText == TextHelper::smartToUpper(paragraphText) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && !paragraphs[i + 1].isEmpty()) {
                //
                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
                // пустая Значит это имя персонажа (для реплики)
                //
                currentBlockType = TextParagraphType::Character;
            } else if ((prevBlockType == TextParagraphType::Character
                        || prevBlockType == TextParagraphType::Parenthetical
                        || prevBlockType == TextParagraphType::Dialogue)
                       && i > 0 && !paragraphs[i - 1].isEmpty()) {
                //
                // Если предыдущий блок - имя персонажа, ремарка или диалог
                // и предыдущая строка не пустая, то сейчас диалог
                //
                currentBlockType = TextParagraphType::Dialogue;
            } else if (paragraphText.startsWith("=")) {
                bool isPageBreak = false;
                if (paragraphText.startsWith("===")) {
                    isPageBreak = true;
                    for (int j = 3; j != paragraphText.size(); ++j) {
                        if (paragraphText[j] != '=') {
                            isPageBreak = false;
                            break;
                        }
                    }
                }
                if (isPageBreak) {
                    //
                    // Если состоит из трех или более '=', то это PageBreak
                    // TODO: У нас такого сейчас нет
                    //
                    continue;
                }
            } else {
                //
                // Во всех остальных случаях устанавливаем тип по умолчанию
                //
                currentBlockType = d->defaultBlockType;
            }
        }

        //
        // Если в документе не может быть такого типа блока, то установим его как дефолтный
        //
        if (!d->possibleBlockTypes.contains(currentBlockType)) {
            //
            // Но ремарки будем рассматривать как реплики, т.к. иначе они могут сбить форматирование
            //
            if (currentBlockType == TextParagraphType::Parenthetical) {
                currentBlockType = TextParagraphType::Dialogue;
            } else {
                currentBlockType = d->defaultBlockType;
            }
        }

        //
        // Если тип текущего блока - "Персонаж", проверим будет ли двойной диалог
        //
        if (currentBlockType == TextParagraphType::Character) {
            for (int j = i + 1; j + 1 < paragraphsCount; ++j) {
                if (paragraphs[j].isEmpty()) {
                    if (paragraphs[j + 1].endsWith("^")) {
                        d->isDoubleDialogue = true;
                    }
                    break;
                }
            }
        }

        //
        // Перед отправкой блока на обработку сделаем предобработку определенных блоков
        //
        if (currentBlockType == TextParagraphType::SequenceHeading) {
            //
            // ... если директория - закроем нужное число раз уже открытые
            //
            int sharpCount = 0;
            while (paragraphText[sharpCount] == '#') {
                ++sharpCount;
            }

            if (sharpCount <= dirs.size()) {
                unsigned toClose = dirs.size() - sharpCount + 1;
                for (unsigned i = 0; i != toClose; ++i) {
                    d->processBlock({}, TextParagraphType::SequenceFooter, writer);
                    dirs.pop();
                }
            }

            //
            // ... и откроем новую
            //
            QString text = paragraphText.mid(sharpCount);
            d->processBlock(text, TextParagraphType::SequenceHeading, writer);
            dirs.push(text);
        } else if (currentBlockType == TextParagraphType::SceneHeading) {
            //
            // ... если сцена - обработаем её номер
            //
            int sharpPos = paragraphText.size();
            if (paragraphText.endsWith("#")) {
                sharpPos = paragraphText.lastIndexOf('#', paragraphText.size() - 2);
            }
            d->customSceneNumber = "";
            if (sharpPos == -1) {
                sharpPos = paragraphText.size();
            } else if (_keepSceneNumbers) {
                d->customSceneNumber
                    = paragraphText.mid(sharpPos + 1, paragraphText.size() - 2 - sharpPos);
                //
                // Удалим точку в конце, если она есть, т.к. она будет добавлена потом
                //
                if (d->customSceneNumber.endsWith(".")) {
                    d->customSceneNumber.chop(1);
                }
            }
            paragraphText = paragraphText.left(sharpPos);
            d->processBlock(paragraphText, currentBlockType, writer);
        } else if (currentBlockType == TextParagraphType::Character) {
            //
            // ... если персонаж, удалим обозначение двойного диалога, если оно есть
            //
            if (paragraphText.endsWith("^")) {
                paragraphText.chop(1);
                while (!paragraphText.isEmpty() && paragraphText.back().isSpace()) {
                    paragraphText.chop(1);
                }
            }
            //
            // ... и добавим двоеточие, если диалоги располагаются в таблице
            //
            if (placeDialoguesInTable()) {
                paragraphText.append(":");
            }
            d->processBlock(paragraphText, currentBlockType, writer);
        } else if (currentBlockType == TextParagraphType::Parenthetical) {
            //
            // ... если ремарка обернута в скобки, то удалим их
            //
            if (paragraphText.startsWith("(") && paragraphText.endsWith(")")) {
                paragraphText.chop(1);
                paragraphText.remove(0, 1);
                movePreviousTypes(0, 1);
            }
            d->processBlock(paragraphText, currentBlockType, writer);
        } else {
            //
            // ... иначе сразу отправим на обработку
            //
            d->processBlock(paragraphText, currentBlockType, writer);
        }
        prevBlockType = currentBlockType;
    }

    //
    // Добавим комментарии к последнему блоку
    //
    d->appendComments(writer);

    //
    // Закроем последний блок
    //
    writer.writeEndElement();
    d->postProcessPreviousBlock(currentBlockType, writer);

    //
    // Закроем директории нужное число раз
    //
    while (!dirs.empty()) {
        d->processBlock({}, TextParagraphType::SequenceFooter, writer);
        dirs.pop();
    }

    //
    // Закроем документ
    //
    writer.writeEndElement();
    writer.writeEndDocument();

    return result;
}

TextParagraphType AbstractFountainImporter::blockType(QString& _paragraphText) const
{
    TextParagraphType blockType = d->defaultBlockType;
    switch (_paragraphText[0].toLatin1()) {
    case '.': {
        blockType = TextParagraphType::SceneHeading;
        _paragraphText.remove(0, 1);
        movePreviousTypes(0, 1);
        break;
    }

    case '!': {
        blockType = TextParagraphType::Action;
        _paragraphText.remove(0, 1);
        movePreviousTypes(0, 1);
        break;
    }

    case '@': {
        blockType = TextParagraphType::Character;
        _paragraphText.remove(0, 1);
        movePreviousTypes(0, 1);
        break;
    }

    case '>': {
        if (_paragraphText.endsWith("<")) {
            blockType = TextParagraphType::Action;
            _paragraphText.chop(1);
            _paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
        } else {
            blockType = TextParagraphType::Transition;
            _paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
        }
        break;
    }

    case '=': {
        if (_paragraphText.size() > 1 && _paragraphText[1] != "=") {
            blockType = TextParagraphType::BeatHeading;
            _paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
        } else {
            blockType = TextParagraphType::Undefined;
        }
        break;
    }

    case '~': {
        //
        // Лирика
        //
        blockType = TextParagraphType::Lyrics;
        _paragraphText.remove(0, 1);
        movePreviousTypes(0, 1);
        break;
    }

    case '#': {
        blockType = TextParagraphType::SequenceHeading;
        break;
    }

    default: {
        blockType = TextParagraphType::Undefined;
        break;
    }
    }
    return blockType;
}

void AbstractFountainImporter::writeBlock(const QString& _paragraphText, TextParagraphType _type,
                                          QXmlStreamWriter& _writer) const
{
    auto writeValue = [&_writer](const QString& _text) {
        _writer.writeStartElement(xml::kValueTag);
        _writer.writeCDATA(TextHelper::toHtmlEscaped(_text));
        _writer.writeEndElement(); // value
    };

    //
    // Формируем блок сценария
    //
    switch (_type) {
    case TextParagraphType::SequenceHeading: {
        if (d->alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
            d->alreadyInBeat = false; // вышли из бита
        }

        if (d->alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
            d->alreadyInScene = false; // вышли из сцены
        }

        _writer.writeStartElement(toString(TextFolderType::Sequence));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        _writer.writeStartElement(toString(_type));
        writeValue(_paragraphText);
        break;
    }

    case TextParagraphType::SequenceFooter: {
        if (d->alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
            d->alreadyInBeat = false; // вышли из бита
        }

        if (d->alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
            d->alreadyInScene = false; // вышли из сцены
        }

        _writer.writeEndElement(); // контент текущей папки
        _writer.writeEndElement(); // текущая папка
        _writer.writeStartElement(toString(_type));
        writeValue(_paragraphText);
        break;
    }

    case TextParagraphType::SceneHeading: {
        if (d->alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
            d->alreadyInBeat = false; // вышли из бита
        }

        if (d->alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }

        d->alreadyInScene = true; // вошли в новую сцену

        _writer.writeStartElement(toString(TextGroupType::Scene));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());

        if (!d->customSceneNumber.isEmpty()) {
            _writer.writeStartElement(xml::kNumberTag);
            _writer.writeAttribute(xml::kNumberValueAttribute, d->customSceneNumber);
            _writer.writeAttribute(xml::kNumberIsCustomAttribute, "true");
            _writer.writeEndElement();
            d->customSceneNumber = "";
        }

        _writer.writeStartElement(xml::kContentTag);
        _writer.writeStartElement(toString(_type));
        writeValue(_paragraphText);
        break;
    }

    case TextParagraphType::BeatHeading: {
        if (d->alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
        }

        d->alreadyInBeat = true; // вошли в новый бит

        _writer.writeStartElement(toString(TextGroupType::Beat));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        _writer.writeStartElement(toString(_type));
        writeValue(_paragraphText);
        break;
    }

    case TextParagraphType::Character: {
        //
        // Если диалоги располагаются в таблице
        //
        if (placeDialoguesInTable()) {
            //
            // ... и разделитель ещё не открыт, то откроем
            //
            if (!d->splitterIsOpen) {
                _writer.writeEmptyElement(xml::kSplitterTag);
                _writer.writeAttribute(xml::kTypeAttribute, "start");
                d->splitterIsOpen = true;
            }
            _writer.writeStartElement(toString(_type));
            _writer.writeEmptyElement(xml::kParametersTag);
            _writer.writeAttribute(xml::kInFirstColumnAttribute, "true");
            writeValue(_paragraphText);
        }
        //
        // Если диалоги не располгаются в таблице, но пишем двойной диалог
        //
        else if (d->isDoubleDialogue) {
            //
            // Если разделитель не открыт, то это первый персонаж
            //
            if (!d->splitterIsOpen) {
                _writer.writeEmptyElement(xml::kSplitterTag);
                _writer.writeAttribute(xml::kTypeAttribute, "start");
                d->splitterIsOpen = true;
                _writer.writeStartElement(toString(_type));
                _writer.writeEmptyElement(xml::kParametersTag);
                _writer.writeAttribute(xml::kInFirstColumnAttribute, "true");
                d->isFirstCharacter = true;
            } else {
                _writer.writeStartElement(toString(_type));
                _writer.writeEmptyElement(xml::kParametersTag);
                _writer.writeAttribute(xml::kInFirstColumnAttribute, "false");
                d->isFirstCharacter = false;
            }
            writeValue(_paragraphText);
        }
        //
        // Иначе пишем без разделителя
        //
        else {
            _writer.writeStartElement(toString(_type));
            writeValue(_paragraphText);
        }
        break;
    }

    case TextParagraphType::Dialogue: {
        //
        // Если диалоги располагаются в таблице
        //
        if (placeDialoguesInTable()) {
            //
            // ... и разделитель ещё не открыт, то откроем и запишем пустое имя персонажа в первую
            // колонку
            //
            if (!d->splitterIsOpen) {
                _writer.writeEmptyElement(xml::kSplitterTag);
                _writer.writeAttribute(xml::kTypeAttribute, "start");
                d->splitterIsOpen = true;
                _writer.writeStartElement(toString(TextParagraphType::Character));
                _writer.writeEmptyElement(xml::kParametersTag);
                _writer.writeAttribute(xml::kInFirstColumnAttribute, "true");
                writeValue("");
                _writer.writeEndElement(); // сharacter
            }
            _writer.writeStartElement(toString(_type));
            _writer.writeEmptyElement(xml::kParametersTag);
            _writer.writeAttribute(xml::kInFirstColumnAttribute, "false");
            writeValue(_paragraphText);
        }
        //
        // Если диалоги не располгаются в таблице, но пишем двойной диалог
        //
        else if (d->isDoubleDialogue) {
            _writer.writeStartElement(toString(_type));
            _writer.writeEmptyElement(xml::kParametersTag);
            if (d->isFirstCharacter) {
                _writer.writeAttribute(xml::kInFirstColumnAttribute, "true");
            } else {
                _writer.writeAttribute(xml::kInFirstColumnAttribute, "false");
            }
            writeValue(_paragraphText);
        }
        //
        // Иначе пишем без разделителя
        //
        else {
            _writer.writeStartElement(toString(_type));
            writeValue(_paragraphText);
        }
        break;
    }

    default: {
        _writer.writeStartElement(toString(_type));
        writeValue(_paragraphText);
        break;
    }
    }

    if (d->isDoubleDialogue && !d->splitterIsOpen) {
        d->isDoubleDialogue = false;
    }
}

} // namespace BusinessLayer
