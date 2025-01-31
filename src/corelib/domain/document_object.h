#pragma once

#include "domain_object.h"

#include <QDateTime>
#include <QPixmap>
#include <QUuid>

namespace Domain {

/**
 * @brief Типы элементов
 */
enum class DocumentObjectType {
    Undefined = 0,

    //
    // Служебные для обслуживания работы с проектом
    //

    Structure = 1,

    //
    // Сырые данные
    //

    ImageData = 101,

    //
    // Собственно данные
    //

    Project = 10000,
    RecycleBin = 10001,
    //
    Screenplay = 10100,
    ScreenplayTitlePage = 10101,
    ScreenplaySynopsis = 10102,
    ScreenplayTreatment = 10103,
    ScreenplayText = 10104,
    ScreenplayDictionaries = 10105,
    ScreenplayStatistics = 10106,
    ScreenplaySeries = 10110,
    ScreenplaySeriesEpisodes = 10111,
    ScreenplaySeriesTitlePage = 10112,
    ScreenplaySeriesSynopsis = 10113,
    ScreenplaySeriesTreatment = 10114,
    ScreenplaySeriesText = 10115,
    ScreenplaySeriesStatistics = 10116,
    //
    ComicBook = 10200,
    ComicBookTitlePage = 10201,
    ComicBookSynopsis = 10202,
    ComicBookText = 10203,
    ComicBookDictionaries = 10204,
    ComicBookStatistics = 10205,
    //
    Audioplay = 10300,
    AudioplayTitlePage = 10301,
    AudioplaySynopsis = 10302,
    AudioplayText = 10303,
    AudioplayDictionaries = 10304,
    AudioplayStatistics = 10305,
    //
    Stageplay = 10400,
    StageplayTitlePage = 10401,
    StageplaySynopsis = 10402,
    StageplayText = 10403,
    StageplayDictionaries = 10404,
    StageplayStatistics = 10405,
    //
    Novel = 10500,
    NovelTitlePage = 10501,
    NovelSynopsis = 10502,
    NovelOutline = 10503,
    NovelText = 10504,
    NovelDictionaries = 10505,
    NovelStatistics = 10506,
    //
    GameScript = 10600,
    //
    Plots = 20000,
    Plot = 20001,
    //
    Characters = 30000,
    Character = 30001,
    //
    Locations = 40000,
    Location = 40001,
    //
    Worlds = 50000,
    World = 50001,
    //
    Folder = 100001,
    SimpleText = 100002,
    MindMap = 100003,
    Image = 100004,
    ImagesGallery = 100005,
    Presentation = 100006,
    Link = 100007,
};

/**
 * @brief Определим метод для возможности использовать типы объектов в виде ключей в словарях
 */
inline uint qHash(DocumentObjectType _type)
{
    return ::qHash(static_cast<int>(_type));
}

/**
 * @brief Получить майм-тип по типу объекта
 */
QByteArray CORE_LIBRARY_EXPORT mimeTypeFor(DocumentObjectType _type);

/**
 * @brief Получить тип объекта по майм-типу
 */
DocumentObjectType CORE_LIBRARY_EXPORT typeFor(const QByteArray& _mime);

/**
 * @brief Получить иконку по типу объекта
 */
QString CORE_LIBRARY_EXPORT iconForType(DocumentObjectType _type);

/**
 * @brief Изображение связанное с документом
 */
struct DocumentImage {
    QUuid uuid = {};
    QPixmap image = {};
};

/**
 * @brief Класс данных документа
 */
class CORE_LIBRARY_EXPORT DocumentObject : public DomainObject
{
public:
    /**
     * @brief Уникальный идентификатор
     */
    const QUuid& uuid() const;
    void setUuid(const QUuid& _uuid);

    /**
     * @brief Тип документа
     */
    DocumentObjectType type() const;
    void setType(DocumentObjectType _type);

    /**
     * @brief Содержимое документа
     */
    const QByteArray& content() const;
    void setContent(const QByteArray& _content);

    /**
     * @brief Дата и время последней синхронизации
     */
    const QDateTime& syncedAt() const;
    void setSyncedAt(const QDateTime& _syncedAt);

private:
    explicit DocumentObject(const Identifier& _id, const QUuid& _uuid, DocumentObjectType _type,
                            const QByteArray& _content, const QDateTime& _syncedAt);
    friend class ObjectsBuilder;

    /**
     * @brief Уникальный идентификатор
     */
    QUuid m_uuid;

    /**
     * @brief Тип
     */
    DocumentObjectType m_type = DocumentObjectType::Undefined;

    /**
     * @brief Содержимое объекта
     */
    QByteArray m_content;

    /**
     * @brief Дата время последней синхронизации содержимого документа
     */
    QDateTime m_syncedAt;
};

} // namespace Domain
