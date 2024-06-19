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

// Define the Counters struct with atomic members to handle current progress
struct Counters {
    std::atomic<int> current = 0;
    std::atomic<int> max = 1;
};


// Function to retrieve log files
std::vector<LogFile> retrieveLogFiles(const std::string& inputfilename, Counters& counters);

#endif // LISTFILES_H