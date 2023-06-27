#include "project.h"

#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>

#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>


namespace BusinessLayer {

class Project::Implementation
{
public:
    ProjectType type = ProjectType::Invalid;
    QString path;
    QString realPath;

    mutable QPixmap poster;
    QString posterPath;
    QUuid uuid;
    QString name;
    QString logline;
    QDateTime lastEditTime;
    bool canAskAboutSwitch = true;
    bool canBeSynced = true;
    int id = -1;
    bool isOwner = true;
    ManagementLayer::DocumentEditingMode editingMode = ManagementLayer::DocumentEditingMode::Edit;
    QHash<QUuid, ManagementLayer::DocumentEditingMode> editingPermissions;
    QVector<Domain::ProjectCollaboratorInfo> collaborators;
};


// **

QString Project::extension()
{
    return ".starc";
}


Project::Project()
    : d(new Implementation)
{
}

Project::Project(const Project& _other)
    : d(new Implementation(*_other.d))
{
}

Project::~Project() = default;

const Project& Project::operator=(const Project& _other)
{
    d.reset(new Implementation(*_other.d));
    return *this;
}

bool Project::isValid() const
{
    return d->type != ProjectType::Invalid;
}

bool Project::isLocal() const
{
    return d->type == ProjectType::Local || d->type == ProjectType::LocalShadow;
}

bool Project::isRemote() const
{
    return d->type == ProjectType::Cloud;
}

ProjectType Project::type() const
{
    return d->type;
}

void Project::setType(ProjectType _type)
{
    d->type = _type;
}

QString Project::path() const
{
    return d->path;
}

void Project::setPath(const QString& _path)
{
    d->path = _path;

    if (d->type == ProjectType::Local || d->type == ProjectType::LocalShadow) {
        d->editingMode = QFileInfo(d->path).isWritable()
            ? ManagementLayer::DocumentEditingMode::Edit
            : ManagementLayer::DocumentEditingMode::Read;
    }
}

QString Project::realPath() const
{
    return d->realPath;
}

void Project::setRealPath(const QString& _path)
{
    d->realPath = _path;
}

const QPixmap& Project::poster() const
{
    if (d->poster.isNull()) {
        d->poster.load(d->posterPath);
        if (d->poster.isNull()) {
            static const QPixmap kDefaultPoster(":/images/movie-poster");
            d->poster = kDefaultPoster;
        }
    }

    return d->poster;
}

QString Project::posterPath() const
{
    return d->posterPath;
}

void Project::setPosterPath(const QString& _path)
{
    if (d->posterPath == _path) {
        return;
    }

    d->posterPath = _path;

    //
    // Обнуляем постер, чтобы он потом извлёкся по заданному пути
    //
    d->poster = {};
}

QUuid Project::uuid() const
{
    return d->uuid;
}

void Project::setUuid(const QUuid& _uuid)
{
    d->uuid = _uuid;
}

QString Project::name() const
{
    return d->name;
}

void Project::setName(const QString& _name)
{
    d->name = _name;
}

QString Project::logline() const
{
    return d->logline;
}

void Project::setLogline(const QString& _logline)
{
    d->logline = _logline;
}

QString Project::displayLastEditTime() const
{
    switch (d->lastEditTime.daysTo(QDateTime::currentDateTime())) {
    case 0: {
        return QApplication::translate("Domain::Project", "today at")
            + d->lastEditTime.toString(" hh:mm");
    }

    case 1: {
        return QApplication::translate("Domain::Project", "yesterday at")
            + d->lastEditTime.toString(" hh:mm");
    }

    default: {
        return d->lastEditTime.toString("dd.MM.yyyy hh:mm");
    }
    }
}

QDateTime Project::lastEditTime() const
{
    return d->lastEditTime;
}

void Project::setLastEditTime(const QDateTime& _time)
{
    d->lastEditTime = _time;
}

bool Project::canAskAboutSwitch() const
{
    return d->canAskAboutSwitch;
}

void Project::setCanAskAboutSwitch(bool _can)
{
    d->canAskAboutSwitch = _can;
}

bool Project::canBeSynced() const
{
    return d->canBeSynced;
}

void Project::setCanBeSynced(bool _can)
{
    d->canBeSynced = _can;
}

int Project::id() const
{
    return d->id;
}

void Project::setId(int _id)
{
    d->id = _id;
}

bool Project::isOwner() const
{
    return d->isOwner;
}

void Project::setOwner(bool _isOwner)
{
    d->isOwner = _isOwner;
}

ManagementLayer::DocumentEditingMode Project::editingMode() const
{
    return d->editingMode;
}

void Project::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    d->editingMode = _mode;
}

bool Project::isReadOnly() const
{
    return d->editingMode == ManagementLayer::DocumentEditingMode::Read;
}

QHash<QUuid, ManagementLayer::DocumentEditingMode> Project::editingPermissions() const
{
    return d->editingPermissions;
}

void Project::setEditingPermissions(
    const QHash<QUuid, ManagementLayer::DocumentEditingMode>& _permissions)
{
    d->editingPermissions = _permissions;
}

void Project::clearEditingPermissions()
{
    d->editingPermissions.clear();
}

QVector<Domain::ProjectCollaboratorInfo> Project::collaborators() const
{
    return d->collaborators;
}

void Project::setCollaborators(const QVector<Domain::ProjectCollaboratorInfo>& _collaborators)
{
    d->collaborators = _collaborators;
}

QVariant Project::data(int _role) const
{
    switch (_role) {
    case ProjectDataRole::Type: {
        return static_cast<int>(type());
    }

    case ProjectDataRole::Path: {
        return path();
    }

    case ProjectDataRole::PosterPath: {
        return posterPath();
    }

    case ProjectDataRole::Name: {
        return name();
    }

    case ProjectDataRole::Logline: {
        return logline();
    }

    case ProjectDataRole::LastEditTime: {
        return lastEditTime();
    }

    default: {
        return {};
    }
    }
}

bool operator==(const Project& _lhs, const Project& _rhs)
{
    if (!_lhs.uuid().isNull() && !_rhs.uuid().isNull()) {
        return _lhs.uuid() == _rhs.uuid();
    }

    return _lhs.type() == _rhs.type() && _lhs.path() == _rhs.path() && _lhs.name() == _rhs.name()
        && _lhs.logline() == _rhs.logline() && _lhs.lastEditTime() == _rhs.lastEditTime();
}

} // namespace BusinessLayer
