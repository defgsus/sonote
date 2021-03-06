/***************************************************************************

Copyright (C) 2016  stefan.berke @ modular-audio-graphics.com

This source is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

****************************************************************************/

#ifndef SONOTSRC_SAMPLEPLAYER_H
#define SONOTSRC_SAMPLEPLAYER_H

#include <QObject>

class QIODevice;

namespace Sonot {


/** Basic interface to Qt's QAudioOutput.
    Currently expects float* data */
class SamplePlayer : public QObject
{
    Q_OBJECT
public:
    explicit SamplePlayer(QObject *parent = 0);
    ~SamplePlayer();

signals:

public slots:

    void play(const float* samples, size_t numSamples,
              size_t numChannels, size_t sampleRate);

    void play(QIODevice* data, size_t numChannels, size_t sampleRate);

    /** Stops all samples */
    void stop();

private:
    struct Private;
    Private* p_;
};

} // namespace Sonot

#endif // SONOTSRC_SAMPLEPLAYER_H
