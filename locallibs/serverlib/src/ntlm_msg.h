#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------

using Byte = unsigned char;
using ByteVec = std::vector<Byte>;

template <std::size_t T>
using ByteArr = std::array<Byte, T>;

//-----------------------------------------------------------------------------

// "NTLMSSP" signature is always in ASCII regardless of the platform
#define NTLM_PROTOCOL "\x4e\x54\x4c\x4d\x53\x53\x50"

// Indicates that Unicode strings are supported for use in security buffer data.
#define NTLMFLAG_NEGOTIATE_UNICODE (1 << 0)

// Indicates that OEM strings are supported for use in security buffer data.
#define NTLMFLAG_NEGOTIATE_OEM (1 << 1)

// Requests that the server's authentication realm be included in the Type 2 message.
#define NTLMFLAG_REQUEST_TARGET (1 << 2)

// unknown (1<<3)

// Specifies that authenticated communication between the client and server
// should carry a digital signature (message integrity).
#define NTLMFLAG_NEGOTIATE_SIGN (1 << 4)

// Specifies that authenticated communication between the client and server should be encrypted (message confidentiality).
#define NTLMFLAG_NEGOTIATE_SEAL (1 << 5)

// Indicates that datagram authentication is being used.
#define NTLMFLAG_NEGOTIATE_DATAGRAM_STYLE (1 << 6)

// Indicates that the LAN Manager session key should be used for signing and sealing authenticated communications.
#define NTLMFLAG_NEGOTIATE_LM_KEY (1 << 7)

// unknown purpose
#define NTLMFLAG_NEGOTIATE_NETWARE (1 << 8)

// Indicates that NTLM authentication is being used.
#define NTLMFLAG_NEGOTIATE_NTLM_KEY (1 << 9)

// unknown (1<<10)

// Sent by the client in the Type 3 message to indicate that an anonymous context has been established. This also affects the response fields.
#define NTLMFLAG_NEGOTIATE_ANONYMOUS (1 << 11)

// Sent by the client in the Type 1 message to indicate that a desired authentication realm is included in the message.
#define NTLMFLAG_NEGOTIATE_DOMAIN_SUPPLIED (1 << 12)

// Sent by the client in the Type 1 message to indicate that the client workstation's name is included in the message.
#define NTLMFLAG_NEGOTIATE_WORKSTATION_SUPPLIED (1 << 13)

// Sent by the server to indicate that the server and client are on the same machine. Implies that the client may use a pre-established local security context rather than responding to the challenge.
#define NTLMFLAG_NEGOTIATE_LOCAL_CALL (1 << 14)

// Indicates that authenticated communication between the client and server should be signed with a "dummy" signature.
#define NTLMFLAG_NEGOTIATE_ALWAYS_SIGN (1 << 15)

// Sent by the server in the Type 2 message to indicate that the target authentication realm is a domain.
#define NTLMFLAG_TARGET_TYPE_DOMAIN (1 << 16)

// Sent by the server in the Type 2 message to indicate that the target authentication realm is a server.
#define NTLMFLAG_TARGET_TYPE_SERVER (1 << 17)

// Sent by the server in the Type 2 message to indicate that the target authentication realm is a share. Presumably, this is for share-level authentication. Usage is unclear.
#define NTLMFLAG_TARGET_TYPE_SHARE (1 << 18)

// Indicates that the NTLM2 signing and sealing scheme should be used for protecting authenticated communications.
#define NTLMFLAG_NEGOTIATE_NTLM2_KEY (1 << 19)

// unknown purpose
#define NTLMFLAG_REQUEST_INIT_RESPONSE (1 << 20)

// unknown purpose
#define NTLMFLAG_REQUEST_ACCEPT_RESPONSE (1 << 21)

// unknown purpose
#define NTLMFLAG_REQUEST_NONNT_SESSION_KEY (1 << 22)

// Sent by the server in the Type 2 message to indicate that it is including a Target Information block in the message.
#define NTLMFLAG_NEGOTIATE_TARGET_INFO (1 << 23)

// unknown (1<24)
// unknown (1<25)
// unknown (1<26)
// unknown (1<27)
// unknown (1<28)

// Indicates that 128-bit encryption is supported.
#define NTLMFLAG_NEGOTIATE_128 (1 << 29)

// Indicates that the client will provide an encrypted master key in the "Session Key" field of the Type 3 message.
#define NTLMFLAG_NEGOTIATE_KEY_EXCHANGE (1 << 30)

// Indicates that 56-bit encryption is supported.
#define NTLMFLAG_NEGOTIATE_56 (1 << 31)

//-----------------------------------------------------------------------------

uint32_t read16le(const Byte *buf);
uint32_t read32le(const Byte *buf);

void write32le(const int32_t value, Byte *buf);
void write64le(const int64_t value, Byte *buf);

void asc2utf(ByteVec &dest, const std::string &src, const std::size_t srclen);
void asc2utf(Byte *dest, const std::string &src, const std::size_t srclen);
void ascUpper2utf(ByteVec &dest, const std::string &src, const std::size_t srclen);
void asc2asc(ByteVec &dest, const std::string &str);
char ascToupper(char in);

//-----------------------------------------------------------------------------

struct MsgData {
    std::string ascDomain;
    std::string ascUsername;
    std::string ascPassword;
    std::string ascHostname;

    ByteVec tname;
    ByteVec tinfo;

    ByteVec servChallenge;
    ByteVec userChallenge = {0xff, 0xff, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44};
    ByteVec context;

    uint32_t userFlags = NTLMFLAG_NEGOTIATE_OEM |
                         NTLMFLAG_REQUEST_TARGET |
                         NTLMFLAG_NEGOTIATE_NTLM_KEY |
                         NTLMFLAG_NEGOTIATE_NTLM2_KEY |
                         NTLMFLAG_NEGOTIATE_ALWAYS_SIGN;

    uint32_t servFlags = 0;
};

std::string type1msgFunc(const MsgData &mdata);
void type2msgFunc(const std::string &encoded, MsgData &mdata);
std::string type3msgFunc(MsgData &mdata);
