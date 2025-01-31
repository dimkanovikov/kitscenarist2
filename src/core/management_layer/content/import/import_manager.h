#pragma once

#include <business_layer/import/abstract_importer.h>

#include <QObject>

namespace Domain {
enum class DocumentObjectType;
}


namespace ManagementLayer {

/**
 * @brief Управляющий импортом документов
 */
class ImportManager : public QObject
{
    Q_OBJECT

public:
    ImportManager(QObject* _parent, QWidget* _parentWidget);
    ~ImportManager() override;

    /**
     * @brief Запустить процесс импорта данных
     */
    void import();

    /**
     * @brief Импортировать все возможные данные из указанного файла
     */
    void importScreenplay(const QString& _filePath, bool _importDocuments = true);
    void importNovel(const QString& _filePath);

    /**
     * @brief Импортировать данные файла в заданный документ
     */
    void importFileDataToDocument(const QString& _filePath, const QUuid& _documentUuid,
                                  Domain::DocumentObjectType _type);

signals:
    /**
     * @brief Персонаж загружен
     */
    void characterImported(const QString& _name, const QString& _content);

    /**
     * @brief Локация загружена
     */
    void locationImported(const QString& _name, const QString& _content);

    /**
     * @brief Импортирован документ разработки
     */
    void documentImported(const BusinessLayer::AbstractImporter::Document& _document);

    /**
     * @brief Документ загружен
     */
    void audioplayImported(const QString& _name, const QString& _titlePage, const QString& _text);
    void comicbookImported(const QString& _name, const QString& _titlePage, const QString& _text);
    void novelImported(const QString& _name, const QString& _screenplay);
    void screenplayImported(const QString& _name, const QString& _titlePage,
                            const QString& _synopsis, const QString& _treatment,
                            const QString& _screenplay);
    void stageplayImported(const QString& _name, const QString& _titlePage, const QString& _text);

    /**
     * @brief Запрос на загрузку презентации
     */
    void importPresentationRequested(const QString& _filePath, const QUuid& _documentUuid);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
