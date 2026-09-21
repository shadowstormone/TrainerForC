#include "platform/Logger.h"

#include <vector>

namespace
{
    std::vector<ILogger*> g_sinks;

    void Write(const char* level, const std::string& message)
    {
        for (ILogger* sink : g_sinks)
        {
            if (sink) sink->Log(level, message);
        }
    }
}

void Log::SetSink(ILogger* sink)
{
    g_sinks.clear();
    if (sink) g_sinks.push_back(sink);
}

void Log::AddSink(ILogger* sink)
{
    if (sink) g_sinks.push_back(sink);
}

void Log::Info(const std::string& message)  { Write("INFO", message); }
void Log::Error(const std::string& message) { Write("ERROR", message); }
void Log::Debug(const std::string& message) { Write("DEBUG", message); }
void Log::Fatal(const std::string& message) { Write("FATAL", message); }
