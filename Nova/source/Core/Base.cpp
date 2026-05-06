#include "Core/Base.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

string::string(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    s32 length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (length < 64)
    {
        if (m_IsHeapAllocated)
        {
            free(m_BufferHeap);
            m_BufferHeap = NULL;
            m_IsHeapAllocated = false;
        }

        va_start(args, format);
        vsnprintf(m_BufferInline, 64, format, args);
        va_end(args);
    }
    else
    {
        char* new_buffer = (char*)realloc(m_IsHeapAllocated ? m_BufferHeap : NULL, length + 1);
        if (new_buffer)
        {
            m_BufferHeap = new_buffer;
            m_IsHeapAllocated = true;

            va_start(args, format);
            vsnprintf(m_BufferHeap, length + 1, format, args);
            va_end(args);
        }
    }
}

string::~string()
{
    if (m_IsHeapAllocated)
    {
        free(m_BufferHeap);
        m_BufferHeap = NULL;
        m_IsHeapAllocated = false;
    }
}
