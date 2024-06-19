// listfiles.cpp
#include "converter.h"
#include <iostream>
#include <ctime>
#include <sstream>


#include <canlib.h>
#include <kvmlib.h>
#include <kvlclib.h>
#include <kvaDbLib.h>

// Exceptions and helper functions...

class CanBaseException : public std::exception {
public:
    const char* what() const noexcept override { return err_txt; }
protected:
    char err_txt[256] = { 0 };
};

class KvmException : public CanBaseException {
public:
    kvmStatus status;
    KvmException(kvmStatus s) : status(s) {
        kvmGetErrorText(status, err_txt, sizeof(err_txt));
        snprintf(err_txt + strlen(err_txt), sizeof(err_txt) - strlen(err_txt),
            "(%d) :", status);
    }
    static kvmStatus throw_on_error(kvmStatus s) {
        if (s != kvmOK) {
            if (s == kvmERR_FILE_ERROR) {
                std::cerr << "SD Card not found!" << std::endl;
                //std::exit(1); // Exit the program with code 1
            }
            throw KvmException(s);
        }
        return s;
    };
};

std::string unix_to_iso_with_underscores(time_t unixTimestamp) {
    struct tm timeInfo;
    if (localtime_s(&timeInfo, &unixTimestamp) != 0) {
        return "Invalid timestamp";
    }

    char isoTimestamp[20];
    strftime(isoTimestamp, sizeof(isoTimestamp), "%Y%m%d_%H%M%S", &timeInfo);
    return isoTimestamp;
}

std::vector<LogFile> retrieveLogFiles(const std::string& inputfilename, Counters& counters) {
    std::vector<LogFile> logFiles;
    logFiles.clear();

    kvmStatus status;
    kvmInitialize();

    int32_t ldfMajor = 0;
    int32_t ldfMinor = 0;
    int32_t kvmDEVICE = 1;
    uint32_t nr_logfiles;

    kvmHandle kvm_handle = kvmKmfOpenEx(inputfilename.c_str(), &status, kvmDEVICE, &ldfMajor, &ldfMinor);
    KvmException::throw_on_error(status);
    KvmException::throw_on_error(kvmDeviceMountKmfEx(kvm_handle, &ldfMajor, &ldfMinor));
    KvmException::throw_on_error(kvmLogFileGetCount(kvm_handle, &nr_logfiles));
    counters.max.store(nr_logfiles);

    for (uint16_t log_nr = 0; log_nr < nr_logfiles; log_nr++) {
        kvmLogEventEx e;

        int64_t event_count;
        KvmException::throw_on_error(kvmLogFileMountEx(kvm_handle, log_nr, &event_count));
        uint32_t start_time;
        uint32_t end_time;
        kvmLogFileGetStartTime(kvm_handle, &start_time);
        kvmLogFileGetEndTime(kvm_handle, &end_time);

        std::string start = unix_to_iso_with_underscores(start_time);
        std::string end = unix_to_iso_with_underscores(end_time);

        uint32_t duration = end_time - start_time;

        LogFile log;
        log.number = log_nr;
        log.startDate = start;
        log.endDate = end;
        log.duration = std::to_string(duration);
        log.eventCount = event_count;
        log.selected = false;  // Assuming default value is false

        logFiles.push_back(log);
        counters.current.store(log_nr);
    }
    counters.current.store(nr_logfiles);
    kvmClose(kvm_handle);
    return logFiles;
}
