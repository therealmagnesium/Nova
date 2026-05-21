#pragma once
#include <new>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef uint8_t u8;     //!<  8-bit unsigned integer.
typedef uint16_t u16;   //!< 16-bit unsigned integer.
typedef uint32_t u32;   //!< 32-bit unsigned integer.
typedef uint64_t u64;   //!< 64-bit unsigned integer.
typedef uintptr_t uptr; //!< Pointer-sized unsigned integer.

typedef int8_t s8;     //!<  8-bit signed integer.
typedef int16_t s16;   //!< 16-bit signed integer.
typedef int32_t s32;   //!< 32-bit signed integer.
typedef int64_t s64;   //!< 64-bit signed integer.
typedef intptr_t sptr; //!< Pointer-sized signed integer.

#define V2_FMT "<%.3f, %.3f>"
#define V3_FMT "<%.3f, %.3f, %.3f>"
#define V4_FMT "<%.3f, %.3f, %.3f, %.3f>"

#define V2_OPEN(v) v.x, v.y
#define V3_OPEN(v) v.x, v.y, v.z
#define V4_OPEN(v) v.x, v.y, v.z, v.w

#define LEN(array) (sizeof(array) / sizeof(array[0]))

#define KB(x) (u64)(x << 10)
#define MB(x) (u64)(x << 20)
#define GB(x) (u64)(x << 30)

class string
{
public:
    string() = default;
    string(const string& other);
    string(const char* format, ...);
    ~string();

    inline const char* c_str() const { return !m_IsHeapAllocated ? m_BufferInline : m_BufferHeap; }
    inline bool IsHeapAllocated() const { return m_IsHeapAllocated; }

    inline bool operator==(const string& rhs) { return !m_IsHeapAllocated ? strcmp(m_BufferInline, rhs.m_BufferInline) == 0 : strcmp(m_BufferHeap, rhs.m_BufferHeap) == 0; }
    inline bool operator!=(const string& rhs) { return !(*this == rhs); }

    inline string& operator=(const string& rhs)
    {
        if (this != &rhs)
        {
            this->~string();
            new (this) string(rhs); // uses copy constructor above
        }
        return *this;
    }

private:
    char m_BufferInline[64] = {};
    char* m_BufferHeap = NULL;
    bool m_IsHeapAllocated = false;
};
