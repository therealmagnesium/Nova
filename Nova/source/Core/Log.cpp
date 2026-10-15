#include "Core/Log.h"

namespace Nova::Logs
{
    // Oldest entries are discarded once this many are stored, so a chatty frame loop can't grow memory forever.
    static constexpr u64 k_MaxEntries = 2048;

    // Nova is single-threaded today, so no locking. If logging ever happens off the main thread,
    // guard Push/Clear with a mutex and have GetEntries hand back a copy instead of a reference.
    static std::deque<LogEntry> entries;

    void Push(LogLevel level, const char* text)
    {
        if (entries.size() >= k_MaxEntries)
            entries.pop_front();

        entries.emplace_back(level, text != NULL ? text : "");
    }

    const std::deque<LogEntry>& GetEntries()
    {
        return entries;
    }

    void Clear()
    {
        entries.clear();
    }
}
