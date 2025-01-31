#pragma once

#include "../abstract_model.h"

namespace Domain {
struct DocumentImage;
}

namespace BusinessLayer {

/**
 * @brief Модель презентации
 */
class CORE_LIBRARY_EXPORT PresentationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit PresentationModel(QObject* _parent = nullptr);
    ~PresentationModel() override;

    QString documentName() const override;
    void setDocumentName(const QString& _name) override;
    Q_SIGNAL void nameChanged(const QString& _name);

    QString description() const;
    void setDescription(const QString& _description);
    Q_SIGNAL void descriptionChanged(const QString& _description);

    QVector<Domain::DocumentImage> images() const;
    void setImages(const QVector<QPixmap>& _images);
    Q_SIGNAL void imagesIsSet(const QVector<Domain::DocumentImage>& _images);

    Q_SIGNAL void downloadPresentationRequested(const QString& _filePath);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
