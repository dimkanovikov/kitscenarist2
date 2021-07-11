#include "document_change_storage.h"

#include <data_layer/database.h>
#include <data_layer/mapper/document_change_mapper.h>
#include <data_layer/mapper/mapper_facade.h>
#include <domain/document_change_object.h>
#include <domain/objects_builder.h>
#include <utils/shugar.h>


namespace DataStorageLayer {

class DocumentChangeStorage::Implementation
{
public:
    QVector<Domain::DocumentChangeObject*> newDocumentChanges;
};


// ****


DocumentChangeStorage::~DocumentChangeStorage() = default;

Domain::DocumentChangeObject* DocumentChangeStorage::appendDocumentChange(
    const QUuid& _documentUuid, const QUuid& _uuid, const QByteArray& _undoPatch,
    const QByteArray& _redoPatch, const QString& _userEmail, const QString& _userName)
{
    auto newDocumentChange = Domain::ObjectsBuilder::createDocumentChange(
        {}, _documentUuid, _uuid, _undoPatch, _redoPatch, QDateTime::currentDateTimeUtc(),
        _userEmail, _userName);
    d->newDocumentChanges.append(newDocumentChange);
    return newDocumentChange;
}

Domain::DocumentChangeObject* DocumentChangeStorage::documentChangeAt(const QUuid& _documentUuid,
                                                                      int _changeIndex)
{
    //
    // TODO: Изменения только от текущего пользователя
    //
    //    StorageFacade::settingsStorage()->userName(),
    //    StorageFacade::settingsStorage()->userEmail()

    auto correctedChangeIndex = _changeIndex;

    //
    // Изменения могут находиться и в списке недавних
    //
    for (const auto change : reversed(d->newDocumentChanges)) {
        if (change->documentUuid() != _documentUuid) {
            continue;
        }

        if (correctedChangeIndex == 0) {
            return change;
        } else {
            --correctedChangeIndex;
        }
    }

    return DataMappingLayer::MapperFacade::documentChangeMapper()->find(_documentUuid,
                                                                        correctedChangeIndex);
}

void DocumentChangeStorage::store()
{
    DatabaseLayer::Database::transaction();
    while (!d->newDocumentChanges.isEmpty()) {
        DataMappingLayer::MapperFacade::documentChangeMapper()->insert(
            d->newDocumentChanges.takeFirst());
    }
    DatabaseLayer::Database::commit();
}

void DocumentChangeStorage::clear()
{
    DataMappingLayer::MapperFacade::documentChangeMapper()->clear();
}

DocumentChangeStorage::DocumentChangeStorage()
    : d(new Implementation)
{
}

} // namespace DataStorageLayer
