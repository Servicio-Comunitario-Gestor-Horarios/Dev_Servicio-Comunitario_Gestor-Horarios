#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>

namespace Middleware {

inline constexpr char FRAME_DELIMITER = '\n';
inline constexpr int  MAX_FRAME_BYTES = 4 * 1024 * 1024;

inline QByteArray encodeFrame(const QJsonObject& mensaje)
{
    QByteArray frame = QJsonDocument(mensaje).toJson(QJsonDocument::Compact);
    frame.append(FRAME_DELIMITER);
    return frame;
}

inline QVector<QByteArray> takeCompleteFrames(QByteArray& buffer, bool* overflow = nullptr)
{
    if (overflow) *overflow = false;
    QVector<QByteArray> frames;
    int inicio = 0;
    while (true) {
        const int idx = buffer.indexOf(FRAME_DELIMITER, inicio);
        if (idx < 0) break;
        QByteArray frame = buffer.mid(inicio, idx - inicio);
        inicio = idx + 1;
        if (frame.endsWith('\r')) frame.chop(1);
        if (!frame.trimmed().isEmpty()) frames.append(frame);
    }
    buffer.remove(0, inicio);
    if (overflow && buffer.size() > MAX_FRAME_BYTES) { *overflow = true; buffer.clear(); }
    return frames;
}

} // namespace Middleware
```