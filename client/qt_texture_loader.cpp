#include "qt_texture_loader.h"
#include <QFileInfo>
#include <QDir>

QPixmap LoadPixmapFromFile(const QString& filename) {
    QPixmap pixmap;
    if (QFileInfo::exists(filename)) {
        pixmap.load(filename);
    }
    return pixmap;
}

QPixmap LoadPixmapFromAny(const std::vector<QString>& candidates) {
    for (const QString& path : candidates) {
        QPixmap pixmap = LoadPixmapFromFile(path);
        if (!pixmap.isNull()) {
            return pixmap;
        }
    }
    return QPixmap();
}

bool QtFileExists(const QString& filename) {
    return QFileInfo::exists(filename);
}
