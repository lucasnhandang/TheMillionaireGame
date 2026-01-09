#include "qt_texture_loader.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QApplication>
#include <iostream>

// Helper to get executable directory
QString getExecutableDir() {
    QString appPath = QCoreApplication::applicationDirPath();
    return appPath;
}

// Helper to get assets directory (relative to executable or source)
QString getAssetsDir() {
    QString exeDir = getExecutableDir();
    
    // Try multiple possible locations
    QStringList candidates = {
        exeDir + "/assets",                    // Same dir as executable
        exeDir + "/../assets",                 // Parent dir
        exeDir + "/../client/assets",          // Client subdir
        exeDir + "/../../client/assets",       // Two levels up
        "assets",                              // Current working dir
        "client/assets",                       // Client subdir from working dir
        "../client/assets",                    // Parent/client/assets
        "../../client/assets"                  // Two levels up
    };
    
    for (const QString& candidate : candidates) {
        QDir dir(candidate);
        if (dir.exists() && QFileInfo(candidate + "/millionaire_logo.png").exists()) {
            return candidate;
        }
    }
    
    return "";  // Not found
}

QPixmap LoadPixmapFromFile(const QString& filename) {
    QPixmap pixmap;
    
    // First try Qt resource system (if using .qrc)
    if (filename.startsWith(":/")) {
        pixmap.load(filename);
        if (!pixmap.isNull()) {
            return pixmap;
        }
    }
    
    // Try direct path
    if (QFileInfo::exists(filename)) {
        pixmap.load(filename);
        if (!pixmap.isNull()) {
            return pixmap;
        }
    }
    
    // Try with assets directory
    QString assetsDir = getAssetsDir();
    if (!assetsDir.isEmpty()) {
        QString fullPath = assetsDir + "/" + filename;
        if (QFileInfo::exists(fullPath)) {
            pixmap.load(fullPath);
            if (!pixmap.isNull()) {
                return pixmap;
            }
        }
        
        // Also try just filename if it's already a path
        QFileInfo fileInfo(filename);
        QString justFilename = fileInfo.fileName();
        fullPath = assetsDir + "/" + justFilename;
        if (QFileInfo::exists(fullPath)) {
            pixmap.load(fullPath);
            if (!pixmap.isNull()) {
                return pixmap;
            }
        }
    }
    
    // Try relative to executable
    QString exeDir = getExecutableDir();
    QStringList exeCandidates = {
        exeDir + "/" + filename,
        exeDir + "/assets/" + QFileInfo(filename).fileName(),
        exeDir + "/../assets/" + QFileInfo(filename).fileName(),
        exeDir + "/../client/assets/" + QFileInfo(filename).fileName()
    };
    
    for (const QString& path : exeCandidates) {
        if (QFileInfo::exists(path)) {
            pixmap.load(path);
            if (!pixmap.isNull()) {
                std::cerr << "[DEBUG] Loaded image from: " << path.toStdString() << std::endl;
                return pixmap;
            }
        }
    }
    
    std::cerr << "[WARNING] Failed to load image: " << filename.toStdString() << std::endl;
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
