#include "base64.h"
#include <cassert>

static void base64_encode_tail_triplet(
        const char* enc_tab,
        const unsigned char*& data_p,
        size_t tail_bytes,
        char*& encoded_p)
{
    assert(tail_bytes < 3);
    switch (tail_bytes) {
        case 1:
            {
                const unsigned u = (data_p[0] << 8*2);
                encoded_p[0] = enc_tab[(u >> 6*3) & 0x3f];
                encoded_p[1] = enc_tab[(u >> 6*2) & 0x3f];
                encoded_p[2] = '=';
                encoded_p[3] = '=';
                data_p += 1;
            }
            encoded_p += 4;
            break;

        case 2:
            {
                const unsigned u = (data_p[0] << 8*2) | (data_p[1] << 8*1);
                encoded_p[0] = enc_tab[(u >> 6*3) & 0x3f];
                encoded_p[1] = enc_tab[(u >> 6*2) & 0x3f];
                encoded_p[2] = enc_tab[(u >> 6*1) & 0x3f];
                encoded_p[3] = '=';
                data_p += 2;
            }
            encoded_p += 4;
            break;
    }
}

size_t base64_get_encoded_size(size_t data_size)
{
    return ((data_size + 2) / 3) * 4;
}

void base64_encode(const unsigned char* data, size_t data_size, std::string& encoded)
{
    encoded.resize(base64_get_encoded_size(data_size));
    if(encoded.empty())
        return;
    char* encoded_p = &encoded[0];
    base64_encode(data, data_size, encoded_p, encoded.size());
}

void base64_encode(const unsigned char* data, size_t data_size, char* encoded_p, size_t encoded_z)
{
    const char enc_tab[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    const unsigned char* data_p = data;
    const unsigned char* data_end = data + data_size;

    auto start_pos = encoded_p;
    while (data_p + 2 < data_end) {
        const unsigned u = (data_p[0] << 8*2) | (data_p[1] << 8*1) | (data_p[2] << 8*0);
        encoded_p[0] = enc_tab[(u >> 6*3) & 0x3f];
        encoded_p[1] = enc_tab[(u >> 6*2) & 0x3f];
        encoded_p[2] = enc_tab[(u >> 6*1) & 0x3f];
        encoded_p[3] = enc_tab[(u >> 6*0) & 0x3f];
        encoded_p += 4;
        data_p += 3;
    }

    base64_encode_tail_triplet(enc_tab, data_p, data_size % 3, encoded_p);

    assert(data_p == data_end);
    assert(encoded_p == start_pos + encoded_z);
}

static size_t base64_decode_data_size(const char* encoded, size_t encoded_size)
{
    const size_t max_bytes = (encoded_size / 4) * 3;

    if (encoded_size < 4)
        return 0;
    else if (encoded[encoded_size - 2] == '=')
        return max_bytes - 2;
    else if (encoded[encoded_size - 1] == '=')
        return max_bytes - 1;
    else
        return max_bytes;
}

static void base64_decode_tail_quartet(
        const unsigned char* dec_tab,
        const char*& encoded_p,
        unsigned char*& data_p)
{
    const unsigned u
        = (dec_tab[(unsigned)encoded_p[0]] << 6*3)
        | (dec_tab[(unsigned)encoded_p[1]] << 6*2)
        | (dec_tab[(unsigned)encoded_p[2]] << 6*1)
        | (dec_tab[(unsigned)encoded_p[3]] << 6*0);
    if (encoded_p[2] == '=') {
        data_p[0] = (unsigned char)(u >> 8*2);
        data_p += 1;
    } else if (encoded_p[3] == '=') {
        data_p[0] = (unsigned char)(u >> 8*2);
        data_p[1] = (unsigned char)(u >> 8*1);
        data_p += 2;
    } else {
        data_p[0] = (unsigned char)(u >> 8*2);
        data_p[1] = (unsigned char)(u >> 8*1);
        data_p[2] = (unsigned char)(u >> 8*0);
        data_p += 3;
    }
    encoded_p += 4;
}

void base64_decode(const char* encoded, size_t encoded_size, std::vector<unsigned char>& data)
{
    const unsigned char dec_tab[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x3f,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
        0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    encoded_size &= ~3; /* encoded_size must be multiple of 4 */
    data.resize(base64_decode_data_size(encoded, encoded_size));

    const char* encoded_p = encoded;
    const char* encoded_end = encoded + encoded_size;
    unsigned char* data_p = data.data();
    
    /* decode all but tail quartet */
    while ((encoded_p + 4) + 3 < encoded_end) {
        const unsigned u
            = (dec_tab[(unsigned)encoded_p[0]] << 6*3)
            | (dec_tab[(unsigned)encoded_p[1]] << 6*2)
            | (dec_tab[(unsigned)encoded_p[2]] << 6*1)
            | (dec_tab[(unsigned)encoded_p[3]] << 6*0);
        data_p[0] = (unsigned char)(u >> 8*2);
        data_p[1] = (unsigned char)(u >> 8*1);
        data_p[2] = (unsigned char)(u >> 8*0);
        data_p += 3;
        encoded_p += 4;
    }

    /* decode tail quartet */
    if (encoded_p + 3 < encoded_end)
        base64_decode_tail_quartet(dec_tab, encoded_p, data_p);

    assert(encoded_p == encoded_end);
    assert(data_p == data.data() + data.size());
}

bool base64_decode_formatted(const char* encoded, size_t encoded_size, std::vector<unsigned char>& data)
{
    constexpr unsigned b64_eoln  = 0xf0;
    constexpr unsigned b64_cr    = 0xf1;
    constexpr unsigned b64_eof   = 0xf2;
    constexpr unsigned b64_ws    = 0xe0;
    constexpr unsigned b64_error = 0xff;

    constexpr unsigned not_b64_mask = 0x13;
    constexpr unsigned not_b64_val = b64_eoln | b64_cr | b64_eof | b64_ws;
    static const unsigned invalid_char_before_padding_mask[] = { 0, 0, 0x0f, 0x3 };

    static const unsigned dec_tab[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xe0, 0xf0, 0xff, 0xff, 0xf1, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0x3e, 0xff, 0xf2, 0xff, 0x3f,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff,
        0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
        0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    data.resize((encoded_size / 4) * 3); //maximal length case
    const unsigned char* encoded_p = reinterpret_cast<const unsigned char*>(encoded);
    const unsigned char* encoded_end = encoded_p + encoded_size;
    unsigned char* data_p = data.data();
    size_t decoded_size = 0;

    unsigned decode_buf[4] = {0};
    unsigned cur_val = 0;
    size_t buf_pos = 0;
    size_t padding = 0;

    for (; encoded_p < encoded_end; ++encoded_p)
    {
        cur_val = dec_tab[*encoded_p];

        if ( b64_error == cur_val)
        {
            break;
        }

        if (( cur_val | not_b64_mask) != not_b64_val )
        {
            if (*encoded_p == '=')
            {
                if ((buf_pos > 1) && !(decode_buf[buf_pos - 1] & invalid_char_before_padding_mask[buf_pos] ))
                {
                    ++padding;
                }
                else
                {
                    break;
                }
            }
            else if (padding)
            {
                break;
            }
            decode_buf[buf_pos++] = cur_val;
        }

        // ignoring missing padding
        if ((buf_pos > 1) && (encoded_p + 1 == encoded_end) && (buf_pos < 4)
                && !(decode_buf[buf_pos - 1] & invalid_char_before_padding_mask[buf_pos] ))
        {
            for(;buf_pos < 4; ++buf_pos)
            {
                decode_buf[buf_pos] = 0;
                ++padding;
            }; // fill by zero
        }

        if (buf_pos == 4)
        {
            const unsigned u =
                  (decode_buf[0] << 6*3)
                | (decode_buf[1] << 6*2)
                | (decode_buf[2] << 6*1)
                | (decode_buf[3] << 6*0);
            data_p[0] = static_cast<unsigned char>(u >> 8*2);
            data_p[1] = static_cast<unsigned char>(u >> 8*1);
            data_p[2] = static_cast<unsigned char>(u >> 8*0);
            data_p += 3;
            buf_pos = 0;
            decoded_size += 3;
            if (padding)
            {
                decoded_size -= padding;
            }
        }
    }
    data.resize(decoded_size);
    return !buf_pos && (encoded_p == encoded_end);
}

#ifdef XP_TESTING
#include "test.h"
#include "xp_test_utils.h"
#include "checkunit.h"
namespace
{
std::string test_str_1_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "L/8=    \n  ";

std::string test_str_1_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        "BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG"
        "L/8=";

std::string test_str_2_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "Lw==";

std::string test_str_2_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        "BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG"
        "Lw==";

std::string test_str_3_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "L===";

std::string test_str_3_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        "BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG"
        ;

std::string test_str_4_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            Bg=NVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "L/==";

std::string test_str_4_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        ;

std::string test_str_5_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgN=VHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "L/==";

std::string test_str_5_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        ;

std::string test_str_6_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "L/8";

std::string test_str_6_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        "BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG"
        "L/8=";


std::string test_str_7_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "L/8      \n  ";

std::string test_str_7_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        "BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG"
        "L/8=";

std::string test_str_8_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV\n"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD\n"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN\n"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            ”BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "L/8      \n  ";

std::string test_str_8_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        ;

std::string test_str_9_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "Lw=      \n  ";

std::string test_str_9_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        "BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG"
        "Lw==";

std::string test_str_10_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwM\nT\r\nQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN\n"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF\n"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY\n"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr\n"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf\n"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ\n"
        "            BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0\n"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG\r\n"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By   \r \n"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy\n"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0\n"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn\n"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG\n"
        "Lw=";

std::string test_str_10_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTYxMjIyMDcwMTQ5WjA2MRMwEQYDVQQDDApUZXN0IFVzZXI1MR8wHQYJKoZIhvcN"
        "AQkBFhBjYXNAYWx0bGludXgub3JnMGMwHAYGKoUDAgITMBIGByqFAwICJAAGByqF"
        "AwICHgEDQwAEQL+bqte2+gxoEzKxLir8uvsCx+DQDZfOTTVwENLIAfQVbEzfPfkY"
        "9mrhEJmUOUI5Tr61VMSUzGsIEeJBez6kWwCjggF3MIIBczAdBgNVHSUEFjAUBggr"
        "BgEFBQcDBAYIKwYBBQUHAwIwCwYDVR0PBAQDAgTwMB0GA1UdDgQWBBQkUy9JoVVf"
        "AnrM0HR7y9munR7VljAfBgNVHSMEGDAWgBQVMXywjRreZtcVnElSlxckuQF6gzBZ"
        "BgNVHR8EUjBQME6gTKBKhkhodHRwOi8vdGVzdGNhLmNyeXB0b3Byby5ydS9DZXJ0"
        "RW5yb2xsL0NSWVBUTy1QUk8lMjBUZXN0JTIwQ2VudGVyJTIwMi5jcmwwgakGCCsG"
        "AQUFBwEBBIGcMIGZMGEGCCsGAQUFBzAChlVodHRwOi8vdGVzdGNhLmNyeXB0b3By"
        "by5ydS9DZXJ0RW5yb2xsL3Rlc3QtY2EtMjAxNF9DUllQVE8tUFJPJTIwVGVzdCUy"
        "MENlbnRlciUyMDIuY3J0MDQGCCsGAQUFBzABhihodHRwOi8vdGVzdGNhLmNyeXB0"
        "b3Byby5ydS9vY3NwL29jc3Auc3JmMAgGBiqFAwICAwNBAGx0SdcnHKC/w9g15IZn"
        "9/06yKLzfJ5znTXQLWjDE67MHrFssz/75idikNA9hEspnSuy0Me0XkehQocBCqVG"
        "Lw==";

std::string test_str_11_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MT"
        ;

std::string test_str_11_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        ;

std::string test_str_12_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "M"
        ;

std::string test_str_12_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        ;

std::string test_str_13_dec =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgN\nVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        "MTb"
        ;

std::string test_str_13_enc =
        "MIIDLjCCAt2gAwIBAgITEgAVXwXU0YXOa5+DxgAAABVfBTAIBgYqhQMCAgMwfzEj"
        "MCEGCSqGSIb3DQEJARYUc3VwcG9ydEBjcnlwdG9wcm8ucnUxCzAJBgNVBAYTAlJV"
        "MQ8wDQYDVQQHEwZNb3Njb3cxFzAVBgNVBAoTDkNSWVBUTy1QUk8gTExDMSEwHwYD"
        "VQQDExhDUllQVE8tUFJPIFRlc3QgQ2VudGVyIDIwHhcNMTYwOTIyMDY1MTQ5WhcN"
        ;




#define CHECK_BASE64( num , valid , buf_size )     ret = base64_decode_formatted( (test_str_##num##_dec.c_str()) , test_str_##num##_dec.size() , buf);  \
    base64_encode(buf.data(), buf.size(), encoded); \
    fail_unless( test_str_##num##_enc == encoded, "wrong data test " #num  \
            " data:\n" \
            "===================================\n%s\n" \
            "===================================\n ", encoded.c_str());\
    fail_unless( buf.size() == (buf_size) , "wrong size test " #num " size = %zu ", buf.size() ); \
    fail_unless( (valid) == ret , "wrong ret test " #num );




START_TEST(base64_test)
{
    bool ret = false;
    std::vector<unsigned char> buf;
    std::string encoded;
    CHECK_BASE64(1, true, 818);
    CHECK_BASE64(2, true, 817);
    CHECK_BASE64(3, false, 816);
    CHECK_BASE64(4, false, 480);
    CHECK_BASE64(5, false, 480);
    CHECK_BASE64(6, true, 818);
    CHECK_BASE64(7, true, 818);
    CHECK_BASE64(8, false, 480);
    CHECK_BASE64(9, true, 817);
    CHECK_BASE64(10, true, 817);
    CHECK_BASE64(11, false, 192);
    CHECK_BASE64(12, false, 192);
    CHECK_BASE64(13, false, 192);

}
END_TEST;


#define SUITENAME "Serverlib"
TCASEREGISTER(0,0)
{
    ADD_TEST(base64_test);
}
TCASEFINISH;
} /* namespace */
#endif /*XP_TESTING*/


