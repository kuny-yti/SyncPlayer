#include "audio_output.h"
#include <QVector>
#include <QDebug>

const size_t audio_output::_num_buffers = 3;
const size_t audio_output::_buffer_size = 20160 * 2;

audio_output::audio_output():
    _initialized(false)
{
    const char *p = NULL;
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
    {
        p = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    }
    else if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
    {
        p = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }
    while (p && *p)
    {
        _devices << (QString(p));
        p += _devices.back().length() + 1;
    }
    DEBUG(QString("%1 OpenAL devices available:").arg(devices()));
    for (size_t i = 0; i < (size_t)_devices.size(); i++)
    {
        DEBUG(_devices[i]);
    }
}
audio_output::~audio_output()
{
    deinit();
}
int audio_output::devices() const
{
    return _devices.size();
}
const QString &audio_output::device_name(int i)const
{
    Q_ASSERT(i >= 0 && i < devices());
    return _devices[i];
}
void audio_output::init(int i)
{
    if (_initialized)
        deinit();
    if (!_initialized)
    {
        if (i < 0)
        {
            if (!(_device = alcOpenDevice(NULL)))
            {
                qDebug() << QString("No OpenAL device available.");
            }
        }
        else if (i >= static_cast<int>(_devices.size()))
        {
            qDebug() << QString("OpenAL device 'unknown' is not available.");
        }
        else
        {
            if (!(_device = alcOpenDevice(_devices[i].toLatin1().constData())))
            {
                qDebug() << QString("OpenAL device '%1' is not available.").arg(_devices[i].toLatin1().constData());
            }
        }
        if (!(_context = alcCreateContext(_device, NULL)))
        {
            alcCloseDevice(_device);
            qDebug() << QString("No OpenAL context available.");
        }
        alcMakeContextCurrent(_context);

        _buffers.resize(_num_buffers);
        alGenBuffers(_num_buffers, &(_buffers[0]));
        if (alGetError() != AL_NO_ERROR)
        {
            alcMakeContextCurrent(NULL);
            alcDestroyContext(_context);
            alcCloseDevice(_device);
            qDebug() << QString("Cannot create OpenAL buffers.");
        }
        alGenSources(1, &_source);
        if (alGetError() != AL_NO_ERROR)
        {
            alDeleteBuffers(_num_buffers, &(_buffers[0]));
            alcMakeContextCurrent(NULL);
            alcDestroyContext(_context);
            alcCloseDevice(_device);
            qDebug() << QString("Cannot create OpenAL source.");
        }

        alSourcei(_source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(_source, AL_ROLLOFF_FACTOR, 0);
        if (alGetError() != AL_NO_ERROR)
        {
            alDeleteSources(1, &_source);
            alDeleteBuffers(_num_buffers, &(_buffers[0]));
            alcMakeContextCurrent(NULL);
            alcDestroyContext(_context);
            alcCloseDevice(_device);
            qDebug() << QString("Cannot set OpenAL source parameters.");
        }
        _state = 0;
        _initialized = true;
    }
}
void audio_output::deinit()
{
    if (_initialized)
    {
        do
        {
            alGetSourcei(_source, AL_SOURCE_STATE, &_state);
        }
        while (alGetError() == AL_NO_ERROR && _state == AL_PLAYING);
        alDeleteSources(1, &_source);
        alDeleteBuffers(_num_buffers, &(_buffers[0]));
        alcMakeContextCurrent(NULL);
        alcDestroyContext(_context);
        alcCloseDevice(_device);
        _initialized = false;
    }
}
size_t audio_output::initial_size() const
{
    return _num_buffers * _buffer_size;
}
size_t audio_output::update_size() const
{
    return _buffer_size;
}
qint64 audio_output::status(bool *need_data)
{
    if (_state == 0)
    {
        if (need_data)
        {
            *need_data = true;
        }
        return 0UL;
    }
    else
    {
        ALint processed = 0;
        alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed);
        if (processed == 0)
        {
            alGetSourcei(_source, AL_SOURCE_STATE, &_state);
            if (alGetError() != AL_NO_ERROR)
            {
                qDebug() << QString("Cannot check OpenAL source state.");
            }
            if (_state != AL_PLAYING)
            {
                alSourcePlay(_source);
                if (alGetError() != AL_NO_ERROR)
                {
                    qDebug() << QString("Cannot restart OpenAL source playback.(state:%1)").arg(_state, 0, 16);
                }
            }
            if (need_data)
            {
                *need_data = false;
            }
        }
        else
        {
            if (need_data)
            {
                *need_data = true;
            }
        }
        ALint offset;
        alGetSourcei(_source, AL_SAMPLE_OFFSET, &offset);
        /* The time inside the current buffer */
        qint64 timestamp = static_cast<qint64>(offset) * 1000000 / _buffer_rates[0];
        /* Add the time for all past buffers */
        timestamp += _past_time;
        /* This timestamp unfortunately only grows in relatively large steps. This is
         * too imprecise for syncing a video stream with. Therefore, we use an external
         * time source between two timestamp steps. In case this external time runs
         * faster than the audio time, we also need to make sure that we do not report
         * timestamps that run backwards. */
        if (timestamp != _last_time)
        {
            _last_time = timestamp;
            _ext_last_time = get_microseconds(TIMER_MONOTONIC);
            _last_reported = max(_last_reported, timestamp);
            return _last_reported;
        }
        else
        {
            _last_reported = _last_time + (get_microseconds(TIMER_MONOTONIC) - _ext_last_time);
            return _last_reported;
        }
    }
}
ALenum audio_output::al_format(const audio_blob &blob)
{
    ALenum format = 0;
    if (blob.sample_format == FORMAT_U8)
    {
        if (blob.channels == 1)
        {
            format = AL_FORMAT_MONO8;
        }
        else if (blob.channels == 2)
        {
            format = AL_FORMAT_STEREO8;
        }
        else if (alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
            if (blob.channels == 4)
            {
                format = alGetEnumValue("AL_FORMAT_QUAD8");
            }
            else if (blob.channels == 6)
            {
                format = alGetEnumValue("AL_FORMAT_51CHN8");
            }
            else if (blob.channels == 7)
            {
                format = alGetEnumValue("AL_FORMAT_71CHN8");
            }
            else if (blob.channels == 8)
            {
                format = alGetEnumValue("AL_FORMAT_81CHN8");
            }
        }
    }
    else if (blob.sample_format == FORMAT_S16)
    {
        if (blob.channels == 1)
        {
            format = AL_FORMAT_MONO16;
        }
        else if (blob.channels == 2)
        {
            format = AL_FORMAT_STEREO16;
        }
        else if (alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
            if (blob.channels == 4)
            {
                format = alGetEnumValue("AL_FORMAT_QUAD16");
            }
            else if (blob.channels == 6)
            {
                format = alGetEnumValue("AL_FORMAT_51CHN16");
            }
            else if (blob.channels == 7)
            {
                format = alGetEnumValue("AL_FORMAT_61CHN16");
            }
            else if (blob.channels == 8)
            {
                format = alGetEnumValue("AL_FORMAT_71CHN16");
            }
        }
    }
    else if (blob.sample_format == FORMAT_F32)
    {
        if (alIsExtensionPresent("AL_EXT_float32"))
        {
            if (blob.channels == 1)
            {
                format = alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
            }
            else if (blob.channels == 2)
            {
                format = alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
            }
            else if (alIsExtensionPresent("AL_EXT_MCFORMATS"))
            {
                if (blob.channels == 4)
                {
                    format = alGetEnumValue("AL_FORMAT_QUAD32");
                }
                else if (blob.channels == 6)
                {
                    format = alGetEnumValue("AL_FORMAT_51CHN32");
                }
                else if (blob.channels == 7)
                {
                    format = alGetEnumValue("AL_FORMAT_61CHN32");
                }
                else if (blob.channels == 8)
                {
                    format = alGetEnumValue("AL_FORMAT_71CHN32");
                }
            }
        }
    }
    else if (blob.sample_format == FORMAT_D64)
    {
        if (alIsExtensionPresent("AL_EXT_double"))
        {
            if (blob.channels == 1)
            {
                format = alGetEnumValue("AL_FORMAT_MONO_DOUBLE_EXT");
            }
            else if (blob.channels == 2)
            {
                format = alGetEnumValue("AL_FORMAT_STEREO_DOUBLE_EXT");
            }
        }
    }
    if (format == 0)
    {
        qDebug() << QString("No OpenAL format available for "
                        "audio data format ")+blob.format_name();
    }
    return format;
}
void audio_output::set_volume(float val)
{
    alSourcef(_source, AL_GAIN, val);
    if (alGetError() != AL_NO_ERROR)
    {
        qDebug() << QString("Cannot set OpenAL audio volume.");
    }
}
void audio_output::data(const audio_blob &blob)
{
    if (!_initialized)
        return;
    Q_ASSERT(blob.data);
    ALenum format = al_format(blob);
    DEBUG(QString("Buffering ") + QString::number(blob.size) + " bytes of audio data.");
    if (_state == 0)
    {
        set_volume(1.0);
        // Initial buffering
        Q_ASSERT(blob.size == _num_buffers * _buffer_size);
        char *data = static_cast<char *>(blob.data);
        for (size_t j = 0; j < _num_buffers; j++)
        {
            _buffer_channels << (blob.channels);
            _buffer_sample_bits << (blob.sample_bits());
            _buffer_rates << (blob.rate);
            alBufferData(_buffers[j], format, data, _buffer_size, blob.rate);
            alSourceQueueBuffers(_source, 1, &(_buffers[j]));
            data += _buffer_size;
        }
        if (alGetError() != AL_NO_ERROR)
        {
            qDebug() << QString("Cannot buffer initial OpenAL data.");
        }
    }
    else if (blob.size > 0)
    {
        // Replace one buffer
        Q_ASSERT(blob.size == _buffer_size);
        return ;
        ALuint buf = 0;
        alSourceUnqueueBuffers(_source, 1, &buf);
        Q_ASSERT(buf != 0);
        alBufferData(buf, format, blob.data, _buffer_size, blob.rate);
        alSourceQueueBuffers(_source, 1, &buf);
        if (alGetError() != AL_NO_ERROR)
        {
            qDebug() << QString("Cannot buffer OpenAL data. %1, %2, %3").arg(alGetError()).arg(_source).arg(buf);
        }
        // 一秒的数据计算方法为 采样率＊采样位数＊通道数／8
        // Update the time spent on all past buffers
        qint64 current_buffer_samples = _buffer_size / _buffer_channels[0] * 8 / _buffer_sample_bits[0];
        qint64 current_buffer_time = current_buffer_samples * 1000000 / _buffer_rates[0];
        _past_time += current_buffer_time;
        // Remove current buffer entries
        _buffer_channels.erase(_buffer_channels.begin());
        _buffer_sample_bits.erase(_buffer_sample_bits.begin());
        _buffer_rates.erase(_buffer_rates.begin());
        // Add entries for the new buffer
        _buffer_channels << (blob.channels);
        _buffer_sample_bits << (blob.sample_bits());
        _buffer_rates << (blob.rate);
    }
}
qint64 audio_output::start()
{
    DEBUG("Starting audio output.");
    Q_ASSERT(_state == 0);
    alSourcePlay(_source);
    alGetSourcei(_source, AL_SOURCE_STATE, &_state);
    if (alGetError() != AL_NO_ERROR)
    {
        qDebug() << QString("Cannot start OpenAL source playback.");
    }
    _past_time = 0;
    _last_time = 0;
    _ext_last_time = get_microseconds(TIMER_MONOTONIC);
    _last_reported = _last_time;
    return _last_time;
}
void audio_output::pause()
{
    DEBUG("pause audio output.");
    if (!_initialized) return;
    alSourcePause(_source);
    if (alGetError() != AL_NO_ERROR)
    {
        qDebug() << QString("Cannot pause OpenAL source playback.");
    }
}
void audio_output::unpause()
{
    DEBUG("unpause audio output.");
    if (!_initialized) return;
    alSourcePlay(_source);
    if (alGetError() != AL_NO_ERROR)
    {
        qDebug() << QString("Cannot unpause OpenAL source playback.");
    }
}
void audio_output::stop()
{
    DEBUG("stop audio output.");
    if (!_initialized) return;
    alSourceStop(_source);
    if (alGetError() != AL_NO_ERROR)
    {
        qDebug() << QString("Cannot stop OpenAL source playback.");
    }
    // flush all buffers and reset the state
    ALint processed_buffers;
    alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed_buffers);
    while (processed_buffers > 0)
    {
        ALuint buf = 0;
        alSourceUnqueueBuffers(_source, 1, &buf);
        if (alGetError() != AL_NO_ERROR)
        {
            qDebug() << QString("Cannot unqueue OpenAL source buffers.");
        }
        alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed_buffers);
    }
    _state = 0;
}

