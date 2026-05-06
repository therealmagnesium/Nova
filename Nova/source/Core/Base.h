#pragma once
#include <stdint.h>
#include <stddef.h>

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

class string
{
public:
    string() = default;
    string(const char* format, ...);
    ~string();

    inline const char* c_str() const { return !m_IsHeapAllocated ? m_BufferInline : m_BufferHeap; }
    inline bool IsHeapAllocated() const { return m_IsHeapAllocated; }

    inline string& operator=(const char* rhs)
    {
        string& lhs = *this;
        lhs = string(rhs);
        return lhs;
    }

private:
    char m_BufferInline[64] = {};
    char* m_BufferHeap = NULL;
    bool m_IsHeapAllocated = false;
};

#define LEN(array) sizeof(array) / sizeof(array[0])

#define KB(x) (u64)(x << 10)
#define MB(x) (u64)(x << 20)
#define GB(x) (u64)(x << 30)
