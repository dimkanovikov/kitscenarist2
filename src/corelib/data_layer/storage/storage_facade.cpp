#include "storage_facade.h"

#include "document_change_storage.h"
#include "document_storage.h"
#include "settings_storage.h"


namespace DataStorageLayer {

void StorageFacade::clearStorages()
{
    documentChangeStorage()->clear();
    documentStorage()->clear();
}

DocumentChangeStorage* StorageFacade::documentChangeStorage()
{
    if (s_documentChangeStorage == nullptr) {
        s_documentChangeStorage = new DocumentChangeStorage;
    }

    return s_documentChangeStorage;
}

DocumentStorage* StorageFacade::documentStorage()
{
    if (s_documentStorage == nullptr) {
        s_documentStorage = new DocumentStorage;
    }

    return s_documentStorage;
}

SettingsStorage* StorageFacade::settingsStorage()
{
    if (s_settingsStorage == nullptr) {
        s_settingsStorage = new SettingsStorage;
    }
    return s_settingsStorage;
}

DocumentChangeStorage* StorageFacade::s_documentChangeStorage = nullptr;
DocumentStorage* StorageFacade::s_documentStorage = nullptr;
SettingsStorage* StorageFacade::s_settingsStorage = nullptr;

} // namespace DataStorageLayer
