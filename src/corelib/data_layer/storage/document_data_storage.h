#pragma once

#include <business_layer/model/abstract_image_wrapper.h>

#include <QScopedPointer>

#include <corelib_global.h>


namespace DataStorageLayer {

class CORE_LIBRARY_EXPORT DocumentDataStorage : public BusinessLayer::AbstractImageWrapper
{
public:
    DocumentDataStorage();
    ~DocumentDataStorage() override;

    QPixmap load(const QUuid& _uuid) const override;
    QUuid save(const QPixmap& _image) override;

    void clear();
    void saveChanges();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer
