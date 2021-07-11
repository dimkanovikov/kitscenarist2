#include "image_cropping_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/image_cropper/image_cropper.h>
#include <ui/widgets/label/label.h>

#include <QGridLayout>


class ImageCroppingDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    ImageCropper* imageCropper = nullptr;
    Body1Label* imageCroppingLabel = nullptr;
    // Body1Label* imageCroppingNote = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* selectButton = nullptr;
};

ImageCroppingDialog::Implementation::Implementation(QWidget* _parent)
    : imageCropper(new ImageCropper(_parent))
    , imageCroppingLabel(new Body1Label(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , selectButton(new Button(_parent))
{
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(selectButton);
}


// ****


ImageCroppingDialog::ImageCroppingDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    contentsLayout()->addWidget(d->imageCropper, 0, 0);
    // contentsLayout()->addWidget(d->imageCroppingNote, 1, 0);
    contentsLayout()->addWidget(d->imageCroppingLabel, 2, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 3, 0);

    connect(d->cancelButton, &Button::clicked, this, &ImageCroppingDialog::hideDialog);
    connect(d->selectButton, &Button::clicked, this, [this] {
        emit imageSelected(d->imageCropper->croppedImage());
        hideDialog();
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ImageCroppingDialog::~ImageCroppingDialog() = default;

void ImageCroppingDialog::setImage(const QPixmap& _image)
{
    d->imageCropper->setImage(_image);
}

void ImageCroppingDialog::setImageProportion(const QSizeF& _proportion)
{
    d->imageCropper->setProportion(_proportion);
}

void ImageCroppingDialog::setImageProportionFixed(bool _fixed)
{
    d->imageCropper->setProportionFixed(_fixed);
}

void ImageCroppingDialog::setImageCroppingText(const QString& _text)
{
    d->imageCroppingLabel->setText(_text);
}

QWidget* ImageCroppingDialog::focusedWidgetAfterShow() const
{
    return d->imageCropper;
}

QWidget* ImageCroppingDialog::lastFocusableWidget() const
{
    return d->selectButton;
}

void ImageCroppingDialog::updateTranslations()
{
    d->cancelButton->setText(tr("Cancel"));
    d->selectButton->setText(tr("Select"));
    d->imageCroppingLabel->setText(tr("Select an area for project cover"));
}

void ImageCroppingDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->imageCropper->setMaximumSize(500, 500);

    d->imageCropper->setBackgroundColor(Ui::DesignSystem::color().shadow());
    d->imageCropper->setTextColor(Ui::DesignSystem::color().onShadow());

    d->imageCroppingLabel->setTextColor(Ui::DesignSystem::color().onBackground());
    d->imageCroppingLabel->setBackgroundColor(Ui::DesignSystem::color().background());
    d->imageCroppingLabel->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(),
                                                        Ui::DesignSystem::layout().px12(),
                                                        Ui::DesignSystem::layout().px24(), 0.0)
                                                  .toMargins());

    for (auto button : { d->cancelButton, d->selectButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px8())
            .toMargins());
}
