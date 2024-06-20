#ifndef CONVERTER_H
#define CONVERTER_H

#include <vector>
#include <string>
#include <atomic>

// Struct to hold log file information
struct LogFile {
    int number;
    std::string startDate;
    std::string endDate;
    std::string duration;
    int64_t eventCount;
    bool selected;
};

struct ConvertedLogs {
    int number;
    std::string name;
    std::string exportpath;
};

// Define the Counters struct with atomic members to handle current progress
struct Counters {
    std::atomic<int> current = 0;
    std::atomic<int> max = 1;
};

struct DbcConfig {
    std::string relativePath;
    std::string channelMask;
};

struct ConfigResult {
    std::vector<DbcConfig> dbcFiles;
    std::string resultDir;
    std::string inputfilename;
};


// Function to retrieve log files
std::vector<LogFile> listLogFiles(const std::string& inputfilename, Counters& counters);


std::vector<ConvertedLogs> convertlogfiles(std::vector<int> logstoexport, const std::string& location, Counters& filecounter, Counters& eventcounter);


#endif // LISTFILES_H