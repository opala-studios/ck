#include "ck/audio/wavwriter.h"
#include "ck/audio/fourcharcode.h"

#include <vector>
namespace Cki
{


WavWriter::WavWriter(const char* path, int channels, int sampleRate, bool fixedPoint) :
    AudioWriter(fixedPoint),
    m_file(path, FileStream::k_writeTruncate)
{
    if (m_file.isValid())
    {
        // byte 0
        m_file << Cki::FourCharCode('R', 'I', 'F', 'F');
        m_file << (uint32) 0; // will be updated with size of WAVE chunk
        m_file << Cki::FourCharCode('W', 'A', 'V', 'E');

        // byte 12
        // format chunk
        m_file << Cki::FourCharCode('f', 'm', 't', ' ');
        m_file << (uint32) 16; // chunk size
        
#if CK_PLATFORM_ANDROID
        m_file << (uint16) 1; // format (pcm)
#else
        m_file << (uint16) 3; // format (float)
#endif
        
        m_file << (uint16) channels;
        m_file << (uint32) sampleRate;
#if CK_PLATFORM_ANDROID
        m_file << (uint32) (sampleRate * channels * sizeof(int16_t)); // avgBytesPerSec
        m_file << (uint16) (channels * sizeof(int16_t)); // block align
        m_file << (uint16) (8 * sizeof(int16_t)); // bits per sample
#else
        m_file << (uint32) (sampleRate * channels * sizeof(float)); // avgBytesPerSec
        m_file << (uint16) (channels * sizeof(float)); // block align
        m_file << (uint16) (8 * sizeof(float)); // bits per sample
#endif
       
        // byte 36
        // data chunk
        m_file << Cki::FourCharCode('d', 'a', 't', 'a');
        m_file << (uint32) (0); // will be updated with size of data chunk
    }
}

WavWriter::~WavWriter()
{
    close();
}

bool WavWriter::isValid() const
{
    return m_file.isValid();
}

int WavWriter::write(const float* buf, int samples)
{
    
#if CK_PLATFORM_ANDROID
        int16_t pcm16Buffer[samples];
        for (int i = 0; i < samples; i++){
            float value = buf[i];
            // Offset before casting so that we can avoid using floor().
            // Also round by adding 0.5 so that very small signals go to zero.
            float temp = (INT16_MAX * value) + 0.5 - INT16_MIN;
            int32_t sample = ((int) temp) + INT16_MIN;
            if (sample > INT16_MAX) {
                sample = INT16_MAX;
            } else if (sample < INT16_MIN) {
                sample = INT16_MIN;
            }
            pcm16Buffer[i] = sample;
        }
        return m_file.write(pcm16Buffer, samples*sizeof(int16_t)) / sizeof(int16_t);
#else
        return m_file.write(buf, samples*sizeof(float)) / sizeof(float);
#endif

}

void WavWriter::close()
{
    if (m_file.isValid())
    {
        // update sizes in header
        int pos = m_file.getPos();
        int dataBytes = pos - 44; // 44 bytes for header
        m_file.setPos(4);
        m_file << (uint32) (dataBytes + 36);
        m_file.setPos(40);
        m_file << (uint32) dataBytes;
        m_file.close();
    }
}


}
