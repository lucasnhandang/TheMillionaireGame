// Qt-based texture/image loader to replace OpenGL texture loader
#pragma once

#include <QPixmap>
#include <QString>
#include <string>
#include <vector>

// Load image from file using Qt
// Returns QPixmap (empty if failed)
QPixmap LoadPixmapFromFile(const QString& filename);

// Helper that tries multiple candidate paths (first that exists wins)
QPixmap LoadPixmapFromAny(const std::vector<QString>& candidates);

// Check if file exists
bool QtFileExists(const QString& filename);
