#include "presentation_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>
#include <QPixmap>

namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kDescriptionKey("description");
const QLatin1String kImageKey("image");
const QLatin1String kImagesKey("images");
} // namespace


class PresentationModel::Implementation
{
public:
    QString name;
    QString description;
    QVector<Domain::DocumentImage> images;
};

PresentationModel::PresentationModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
    connect(this, &PresentationModel::imagesIsSet, this, &PresentationModel::updateDocumentContent);
    connect(this, &PresentationModel::nameChanged, this, &PresentationModel::updateDocumentContent);
    connect(this, &PresentationModel::descriptionChanged, this,
            &PresentationModel::updateDocumentContent);
}

PresentationModel::~PresentationModel() = default;

QString PresentationModel::documentName() const
{
    return d->name;
}

void PresentationModel::setDocumentName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString PresentationModel::description() const
{
    return d->description;
}

void PresentationModel::setDescription(const QString& _description)
{
    if (d->description == _description) {
        return;
    }

    d->description = _description;
    emit descriptionChanged(d->description);
}

QVector<Domain::DocumentImage> PresentationModel::images() const
{
    return d->images;
}

void PresentationModel::setImages(const QVector<QPixmap>& _images)
{
    if (_images.isEmpty()) {
        return;
    }

    for (const auto& image : std::as_const(d->images)) {
        imageWrapper()->remove(image.uuid);
    }
    d->images.clear();

    for (const auto& image : _images) {
        d->images.append({ imageWrapper()->save(image), image });
    }

    emit imagesIsSet(d->images);
}

void PresentationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto load = [&documentNode](const QString& _key) {
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
    };
    d->name = load(kNameKey);
    d->description = load(kDescriptionKey);
    const auto imagesNode = documentNode.firstChildElement(kImagesKey);
    if (!imagesNode.isNull()) {
        auto imageNode = imagesNode.firstChildElement(kImageKey);
        while (!imageNode.isNull()) {
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(imageNode.text()));
            if (!uuid.isNull()) {
                d->images.append({ uuid, imageWrapper()->load(uuid) });
            }

            imageNode = imageNode.nextSiblingElement();
        }
    }
}

void PresentationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray PresentationModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    auto save = [&xml](const QString& _key, const QString& _value) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(_key, TextHelper::toHtmlEscaped(_value))
                   .toUtf8();
    };
    save(kNameKey, d->name);
    save(kDescriptionKey, d->description);
    if (!d->images.isEmpty()) {
        xml += QString("<%1>\n").arg(kImagesKey).toUtf8();
        for (const auto& image : std::as_const(d->images)) {
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kImageKey, TextHelper::toHtmlEscaped(image.uuid.toString()))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(kImagesKey).toUtf8();
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
