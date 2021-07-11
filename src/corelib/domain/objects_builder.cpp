#include "objects_builder.h"

#include "document_change_object.h"
#include "document_object.h"


namespace Domain {

DocumentObject* ObjectsBuilder::createDocument(const Identifier& _id, const QUuid& _uuid,
                                               DocumentObjectType _type, const QByteArray& _content)
{
    return new DocumentObject(_id, _uuid, _type, _content);
}

DocumentChangeObject* ObjectsBuilder::createDocumentChange(
    const Identifier& _id, const QUuid& _documentUuid, const QUuid& _uuid,
    const QByteArray& _undoPatch, const QByteArray& _redoPatch, const QDateTime& _dateTime,
    const QString& _userName, const QString& _userEmail)
{
    return new DocumentChangeObject(_id, _documentUuid, _uuid, _undoPatch, _redoPatch, _dateTime,
                                    _userName, _userEmail);
}

} // namespace Domain
