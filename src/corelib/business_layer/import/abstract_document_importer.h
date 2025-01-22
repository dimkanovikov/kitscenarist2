#pragma once

#include "abstract_importer.h"

#include <corelib_global.h>

class QTextDocument;
class QTextCursor;
class QString;
class QXmlStreamWriter;

namespace BusinessLayer {

struct ImportOptions;
enum class TextParagraphType;

/**
 * @brief Класс для импорта из QTextDocument
 */
class CORE_LIBRARY_EXPORT AbstractDocumentImporter : virtual public AbstractImporter
{
public:
    AbstractDocumentImporter();
    ~AbstractDocumentImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions* _options) const override;

    /**
     * @brief Получить из QTextDocument xml-строку
     */
    QString parseDocument(const ImportOptions* _options, QTextDocument& _document) const;

protected:
    /**
     * @brief Получить документ для импорта
     * @return true, если получилось открыть заданный файл
     */
    virtual bool documentForImport(const QString& _filePath, QTextDocument& _document) const = 0;

    /**
     * @brief Определить тип блока в текущей позиции курсора
     *		  с указанием предыдущего типа и количества предшествующих пустых строк
     */
    TextParagraphType typeForTextCursor(const QTextCursor& _cursor,
                                        TextParagraphType _lastBlockType, int _prevEmptyLines,
                                        int _minLeftMargin) const;

    /**
     * @brief Очистка блоков от мусора и их корректировки
     */
    QString clearBlockText(TextParagraphType _blockType, const QString& _blockText) const;

    /**
     * @brief Получить регулярное выражение для определения строки, начинающейся с номера
     */
    QRegularExpression startFromNumberChecker() const;

    /**
     * @brief Следует ли сохранять номера сцен
     */
    virtual bool shouldKeepSceneNumbers(const ImportOptions* _options) const;

    /**
     * @brief Записать редакторские заметки
     */
    virtual void writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const;

    /**
     * @brief Получить имя персонажа
     */
    virtual QString characterName(const QString& _text) const = 0;

    /**
     * @brief Получить название локации
     */
    virtual QString locationName(const QString& _text) const = 0;
};


} // namespace BusinessLayer
