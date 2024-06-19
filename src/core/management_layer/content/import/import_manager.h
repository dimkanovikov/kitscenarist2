#pragma once

#include <QObject>


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
     * @brief Документ загружен
     */
    void audioplayImported(const QString& _name, const QString& _titlePage, const QString& _text);
    void comicbookImported(const QString& _name, const QString& _titlePage, const QString& _text);
    void novelImported(const QString& _name, const QString& _screenplay);
    void screenplayImported(const QString& _name, const QString& _titlePage,
                            const QString& _synopsis, const QString& _treatment,
                            const QString& _screenplay);
    void stageplayImported(const QString& _name, const QString& _titlePage, const QString& _text);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
