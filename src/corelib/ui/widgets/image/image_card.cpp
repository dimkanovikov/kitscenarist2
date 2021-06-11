#include "image_card.h"

#include "image_cropping_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <QPainter>
#include <QResizeEvent>
#include <QVariantAnimation>

#include <NetworkRequestLoader.h>


class ImageCard::Implementation
{
public:
    explicit Implementation(ImageCard* _parent);

    /**
     * @brief Закешировать масштабированную версию изображения для отображения на виджете
     */
    void prepareImageForDisplaing(const QSize& _size);

    /**
     * @brief Выбрать изображение из файла на компьютере
     */
    void chooseImageFromFile();

    /**
     * @brief Обрезать выбранное изображение
     */
    void cropImage(const QPixmap& _image);

    /**
     * @brief Подготовить контекстное меню к отображению
     */
    void prepareContextMenu();

    //
    // Данные
    //

    ImageCard* q = nullptr;

    bool isDragActive = false;
    QString decorationIcon = u8"\U000F0513";
    QString decorationText;
    QString imageCroppingText;
    struct {
        QPixmap source;
        QPixmap display;
    } image;

    ContextMenu* contextMenu = nullptr;
    QAction* changeImageAction = nullptr;
    QAction* clearImageAction = nullptr;
    QAction* copyImageAction = nullptr;
    QAction* pasteImageAction = nullptr;

    QVariantAnimation dragIndicationOpacityAnimation;
    QVariantAnimation decorationColorAnimation;
};

ImageCard::Implementation::Implementation(ImageCard* _parent)
    : q(_parent)
    , contextMenu(new ContextMenu(_parent))
    , changeImageAction(new QAction(contextMenu))
    , clearImageAction(new QAction(contextMenu))
    , copyImageAction(new QAction(contextMenu))
    , pasteImageAction(new QAction(contextMenu))
{
    dragIndicationOpacityAnimation.setStartValue(0.0);
    dragIndicationOpacityAnimation.setEndValue(1.0);
    dragIndicationOpacityAnimation.setDuration(240);
    dragIndicationOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationColorAnimation.setDuration(240);
    decorationColorAnimation.setEasingCurve(QEasingCurve::OutQuad);

    contextMenu->setActions(
        { changeImageAction, clearImageAction, copyImageAction, pasteImageAction });
}

void ImageCard::Implementation::prepareImageForDisplaing(const QSize& _size)
{
    if (image.source.isNull()) {
        return;
    }

    //
    // Добавляем небольшую дельту, чтобы постер занимал всё доступное пространство
    //
    const auto sizeCorrected
        = QRect({}, _size)
              .marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins())
              .size();
    if (image.display.size() == sizeCorrected) {
        return;
    }

    image.display = image.source.scaled(sizeCorrected, Qt::KeepAspectRatioByExpanding,
                                        Qt::SmoothTransformation);
}

void ImageCard::Implementation::chooseImageFromFile()
{
    const QString imagePath = QFileDialog::getOpenFileName(
        q->window(), tr("Choose image"), {},
        QString("%1 (*.png *.jpeg *.jpg *.bmp *.tiff *.tif *.gif)").arg(tr("Images")));
    if (imagePath.isEmpty()) {
        return;
    }

    QPixmap image(imagePath);
    if (image.isNull()) {
        return;
    }

    cropImage(image);
}

void ImageCard::Implementation::cropImage(const QPixmap& _image)
{
    auto dlg = new ImageCroppingDialog(q->window());
    dlg->setImage(_image);
    dlg->setImageProportion(q->size());
    dlg->setImageProportionFixed(true);
    dlg->setImageCroppingText(imageCroppingText);
    connect(dlg, &ImageCroppingDialog::disappeared, dlg, &ImageCroppingDialog::deleteLater);
    connect(dlg, &ImageCroppingDialog::imageSelected, q, &ImageCard::setImage);

    dlg->showDialog();
}

void ImageCard::Implementation::prepareContextMenu()
{
    const auto hasImage = !image.source.isNull();
    clearImageAction->setVisible(hasImage);
    copyImageAction->setVisible(hasImage);
}


// ****


ImageCard::ImageCard(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    setAcceptDrops(true);
    setAttribute(Qt::WA_Hover);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &ImageCard::customContextMenuRequested, this, [this](const QPoint& _pos) {
        //
        // Настроим контекстное меню
        //
        d->prepareContextMenu();
        //
        // ... и отобразим
        //
        d->contextMenu->showContextMenu(mapToGlobal(_pos));

        //
        // При этом, запускаем анимацию ухода курсора из карточки изображения
        //
        d->decorationColorAnimation.setDirection(QVariantAnimation::Backward);
        d->decorationColorAnimation.start();
    });
    connect(d->changeImageAction, &QAction::triggered, this, [this] { d->chooseImageFromFile(); });
    connect(d->clearImageAction, &QAction::triggered, this, [this] { setImage({}); });
    connect(d->copyImageAction, &QAction::triggered, this,
            [this] { QApplication::clipboard()->setPixmap(d->image.source); });
    connect(d->pasteImageAction, &QAction::triggered, this, [this] {
        const auto image = QApplication::clipboard()->pixmap();
        if (!image.isNull()) {
            d->cropImage(image);
        }
    });
    connect(&d->dragIndicationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImageCard::update));
    connect(&d->decorationColorAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImageCard::update));

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ImageCard::~ImageCard() = default;

void ImageCard::setDecorationIcon(const QString& _icon)
{
    if (d->decorationIcon == _icon) {
        return;
    }

    d->decorationIcon = _icon;
    if (!d->image.source.isNull()) {
        return;
    }

    update();
}

void ImageCard::setDecorationText(const QString& _text)
{
    if (d->decorationText == _text) {
        return;
    }

    d->decorationText = _text;
    if (!d->image.source.isNull()) {
        return;
    }

    update();
}

void ImageCard::setImageCroppingText(const QString& _text)
{
    if (d->imageCroppingText == _text) {
        return;
    }

    d->imageCroppingText = _text;
}

void ImageCard::setImage(const QPixmap& _image)
{
    if ((d->image.source.isNull() && _image.isNull())
        || (!d->image.source.isNull() && !_image.isNull() && d->image.source == _image)) {
        return;
    }

    d->image.source = _image;
    d->image.display = {};
    d->prepareImageForDisplaing(size());
    update();

    emit imageChanged(d->image.source);
}

void ImageCard::paintEvent(QPaintEvent* _event)
{
    Card::paintEvent(_event);

    QPainter painter(this);

    //
    // Если не задано изображение
    //
    if (d->image.display.isNull()) {
        const auto decorationColor = d->decorationColorAnimation.currentValue().isValid()
            ? d->decorationColorAnimation.currentValue().value<QColor>()
            : d->decorationColorAnimation.startValue().value<QColor>();
        painter.setPen(decorationColor);
        painter.setBrush(Qt::NoBrush);
        auto iconFont = Ui::DesignSystem::font().iconsForEditors();
        iconFont.setPixelSize(Ui::DesignSystem::scaleFactor() * Ui::DesignSystem::layout().px48()
                              * 2);
        const auto iconHeight = TextHelper::fineLineSpacing(iconFont);
        const auto textHeight = TextHelper::fineLineSpacing(Ui::DesignSystem::font().button());
        const auto decorationHeight = iconHeight + textHeight;
        const auto iconRect = QRectF(0.0, (height() - decorationHeight) / 2.0, width(), iconHeight);
        painter.setFont(iconFont);
        painter.drawText(iconRect, Qt::AlignCenter, d->decorationIcon);
        //
        const auto textRect = QRectF(0.0, iconRect.bottom(), width(), textHeight);
        painter.setFont(Ui::DesignSystem::font().button());
        painter.drawText(textRect, Qt::AlignHCenter, d->decorationText);
    }
    //
    // Если задано изображение, то рисуем его
    //
    else {
        const auto imageRect
            = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins());
        const auto borderRadius = Ui::DesignSystem::card().borderRadius();
        ImageHelper::drawRoundedImage(painter, imageRect, d->image.display, borderRadius);
    }

    //
    // Если в режиме вставки из буфера
    //
    if (d->isDragActive
        || d->dragIndicationOpacityAnimation.state() == QVariantAnimation::Running) {
        painter.setOpacity(d->dragIndicationOpacityAnimation.currentValue().toReal());
        //
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().secondary());
        const auto cardRect
            = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins());
        const auto borderRadius = Ui::DesignSystem::card().borderRadius();
        painter.drawRoundedRect(cardRect, borderRadius, borderRadius);
        //
        painter.setPen(Ui::DesignSystem::color().onSecondary());
        painter.setBrush(Qt::NoBrush);
        auto iconFont = Ui::DesignSystem::font().iconsForEditors();
        iconFont.setPixelSize(Ui::DesignSystem::scaleFactor() * Ui::DesignSystem::layout().px48()
                              * 2);
        const auto icon = u8"\U000F01DA";
        const auto iconHeight = QFontMetricsF(iconFont).boundingRect(icon).height();
        const auto iconRect = QRectF(0.0, (height() - iconHeight) / 2.0, width(), iconHeight);
        painter.setFont(iconFont);
        painter.drawText(iconRect, Qt::AlignCenter, icon);
        //
        painter.setOpacity(1.0);
    }
}

void ImageCard::resizeEvent(QResizeEvent* _event)
{
    Card::resizeEvent(_event);

    d->prepareImageForDisplaing(_event->size());
}

void ImageCard::enterEvent(QEvent* _event)
{
    Card::enterEvent(_event);

    d->decorationColorAnimation.setDirection(QVariantAnimation::Forward);
    d->decorationColorAnimation.start();
}

void ImageCard::leaveEvent(QEvent* _event)
{
    Card::leaveEvent(_event);

    d->decorationColorAnimation.setDirection(QVariantAnimation::Backward);
    d->decorationColorAnimation.start();
}

void ImageCard::mousePressEvent(QMouseEvent* _event)
{
    Card::mousePressEvent(_event);

    if (_event->button() == Qt::LeftButton) {
        d->chooseImageFromFile();
    }
}

void ImageCard::dragEnterEvent(QDragEnterEvent* _event)
{
    _event->acceptProposedAction();

    d->isDragActive = true;
    d->dragIndicationOpacityAnimation.setDirection(QVariantAnimation::Forward);
    d->dragIndicationOpacityAnimation.start();
}

void ImageCard::dragMoveEvent(QDragMoveEvent* _event)
{
    _event->acceptProposedAction();
}

void ImageCard::dragLeaveEvent(QDragLeaveEvent* _event)
{
    _event->accept();
    d->isDragActive = false;
    d->dragIndicationOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->dragIndicationOpacityAnimation.start();
}

void ImageCard::dropEvent(QDropEvent* _event)
{
    QPixmap droppedImage;
    const QMimeData* mimeData = _event->mimeData();
    //
    // Первым делом проверяем список ссылок, возможно выбраны сразу несколько фотогафий
    //
    if (mimeData->hasUrls()) {
        for (const auto& url : mimeData->urls()) {
            //
            // Обрабатываем только изображения
            //
            const QString urlString = url.toString().toLower();
            if (urlString.contains(".png") || urlString.contains(".jpg")
                || urlString.contains(".jpeg") || urlString.contains(".gif")
                || urlString.contains(".tiff") || urlString.contains(".bmp")) {
                //
                // ... локальные считываем из файла
                //
                if (url.isLocalFile()) {
                    droppedImage = QPixmap(url.toLocalFile());
                }
                //
                // ... подгружаем картинки с инета
                //
                else {
                    const QByteArray pixmapData = NetworkRequestLoader::loadSync(url);
                    droppedImage.loadFromData(pixmapData);
                }
            }
        }
    } else if (mimeData->hasImage()) {
        droppedImage = qvariant_cast<QPixmap>(mimeData->imageData());
    }

    if (!droppedImage.isNull()) {
        d->cropImage(droppedImage);
    }

    _event->acceptProposedAction();

    d->isDragActive = false;
    d->dragIndicationOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->dragIndicationOpacityAnimation.start();
}

void ImageCard::updateTranslations()
{
    d->clearImageAction->setText(tr("Delete"));
    d->pasteImageAction->setText(tr("Paste"));
    d->copyImageAction->setText(tr("Copy"));
    d->changeImageAction->setText(tr("Change"));
}

void ImageCard::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    d->decorationColorAnimation.setStartValue(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::disabledTextOpacity()));
    d->decorationColorAnimation.setEndValue(Ui::DesignSystem::color().secondary());

    d->contextMenu->setBackgroundColor(Ui::DesignSystem::color().background());
    d->contextMenu->setTextColor(Ui::DesignSystem::color().onBackground());
}
