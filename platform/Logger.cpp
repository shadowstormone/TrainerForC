#include "platform/Logger.h"

namespace
{
    ILogger* g_sink = nullptr;

    void Write(const char* level, const std::string& message)
    {
        if (g_sink) g_sink->Log(level, message);
    }
}

void Log::SetSink(ILogger* sink)
{
    g_sink = sink;
}

void Log::Info(const std::string& message)  { Write("INFO", message); }
void Log::Error(const std::string& message) { Write("ERROR", message); }
void Log::Debug(const std::string& message) { Write("DEBUG", message); }
void Log::Fatal(const std::string& message) { Write("FATAL", message); }
