// listfiles.cpp
#include "converter.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <thread>
#include <string>
#include <filesystem>
#include <exception>
#include <cstdlib>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <map>
#include <vector>


#include <canlib.h>
#include <kvmlib.h>
#include <kvlclib.h>
#include <kvaDbLib.h>
#include <yaml-cpp/yaml.h>

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

class KvlcException : public CanBaseException {
public:
    KvlcStatus status;
    KvlcException(KvlcStatus s) : status(s) {
        kvlcGetErrorText(status, err_txt, sizeof(err_txt));
        snprintf(err_txt + strlen(err_txt), sizeof(err_txt) - strlen(err_txt),
            "(%d) :", status);
    }
    static KvlcStatus throw_on_error(KvlcStatus s) {
        if (s != kvlcOK)
            throw KvlcException(s);
        return s;
    };
};

ConfigResult loadConfig(const std::string& filename) {
    YAML::Node config = YAML::LoadFile(filename);
    ConfigResult result;

    // Load dbc_files
    for (const auto& node : config["dbc_files"]) {
        DbcConfig dbcConfig;
        dbcConfig.relativePath = node["relativepath"].as<std::string>();
        dbcConfig.channelMask = node["channel_mask"].as<std::string>();
        result.dbcFiles.push_back(dbcConfig);
    }

    // Load resultDir
    if (config["resultDir"]) {
        result.resultDir = config["resultDir"].as<std::string>();
    }

    // Load inputfilename
    if (config["inputfilename"]) {
        result.inputfilename = config["inputfilename"].as<std::string>();
    }
    return result;
}

std::string unix_to_iso_with_underscores(time_t unixTimestamp) {
    struct tm timeInfo;
    if (localtime_s(&timeInfo, &unixTimestamp) != 0) {
        return "Invalid timestamp";
    }

    char isoTimestamp[20];
    strftime(isoTimestamp, sizeof(isoTimestamp), "%Y%m%d_%H%M%S", &timeInfo);
    return isoTimestamp;
}

std::vector<LogFile> listLogFiles(const std::string& inputfilename, Counters& counters) {
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
    counters.current.store(0);

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
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    counters.current.store(nr_logfiles);
    kvmClose(kvm_handle);
    return logFiles;
}


class MDF4Converter {
private:
    KvlcHandle h;
public:
    MDF4Converter(const std::string& outpath) {
        uint32_t yes = 1;
        int compression_level = 6;
        KvlcException::throw_on_error(kvlcCreateConverter(&h, outpath.c_str(),KVLC_FILE_FORMAT_MDF_4X_SIGNAL));
        KvlcException::throw_on_error(kvlcFeedSelectFormat(h, KVLC_FILE_FORMAT_MEMO_LOG));
        KvlcException::throw_on_error(kvlcSetProperty(h, KVLC_PROPERTY_OVERWRITE, &yes, sizeof(yes)));
        KvlcException::throw_on_error(kvlcSetProperty(h, KVLC_PROPERTY_COMPRESSION_LEVEL, &compression_level, sizeof(compression_level)));
    }
    
    ~MDF4Converter() {
        KvlcException::throw_on_error(kvlcDeleteConverter(h));
    }

    void convert_event(kvmLogEventEx& e) {
        KvlcException::throw_on_error(kvlcFeedLogEvent(h, &e));
        KvlcException::throw_on_error(kvlcConvertEvent(h));
    }

    void addDatabaseFile(const std::string& path, unsigned int channelMask) {
        KvlcException::throw_on_error(kvlcAddDatabaseFile(h, path.c_str(), channelMask));
        //std::cout << "Adding DBC file: " << path << " with channel mask: " << channelMask << std::endl;
    }
};

void add_dbc_files_from_config(MDF4Converter& kc, const std::string& basePath, std::vector<DbcConfig> dbcFiles) {
    //std::vector<DbcConfig> dbcFiles = loadConfig(configFilePath);

    // Mapping from string to ChannelMask Bitmask
    std::map<std::string, unsigned int> channelMaskMapping = {
        {"ONE", 1},
        {"TWO", 2},
        {"THREE", 4},
        {"FOUR", 8},
        {"FIVE", 16}
    };

    // Iterate through each DBC file configuration and add them.
    for (const auto& dbcConfig : dbcFiles) {
        std::string fullPath = (std::filesystem::path(basePath) / dbcConfig.relativePath).string();
        auto it = channelMaskMapping.find(dbcConfig.channelMask);

        if (it == channelMaskMapping.end()) {
            throw std::invalid_argument("Invalid channel mask: " + dbcConfig.channelMask);
        }

        unsigned int channelMask = it->second;
        kc.addDatabaseFile(fullPath, channelMask);
    }
}

std::vector<ConvertedLogs> convertlogfiles(std::vector<int> logstoexport, const std::string& location, Counters& filecounter, Counters& eventcounter) {
    std::vector<ConvertedLogs> convertedFiles;


    ConfigResult configResult = loadConfig("C:/Users/Zbook15uG5/Documents/GitHub/DataProcessing/config.yml");
    std::vector<DbcConfig> dbcFiles = configResult.dbcFiles;
    std::string resultDir = configResult.resultDir;
    std::string inputfilename = configResult.inputfilename;
    

    kvmStatus status;
    kvmInitialize();

    int32_t ldfMajor = 0;
    int32_t ldfMinor = 0;
    int32_t kvmDEVICE = 1;

    uint32_t nr_logfiles;
    int16_t converted_count = 0;

    kvmHandle kvm_handle = kvmKmfOpenEx(inputfilename.c_str(), &status, kvmDEVICE, &ldfMajor, &ldfMinor);

    KvmException::throw_on_error(status);

    KvmException::throw_on_error(kvmDeviceMountKmfEx(kvm_handle, &ldfMajor, &ldfMinor));

    KvmException::throw_on_error(kvmLogFileGetCount(kvm_handle, &nr_logfiles));

    filecounter.max.store(logstoexport.size());
    filecounter.current.store(0);
    

    for (uint16_t log_nr = 1; log_nr < nr_logfiles; log_nr++) {
        // Check if log_nr is in log_numbers_to_run
        if (std::find(logstoexport.begin(), logstoexport.end(), log_nr) != logstoexport.end()) {
        
            int64_t event_count;
            kvmLogEventEx e;
            KvmException::throw_on_error(kvmLogFileMountEx(kvm_handle, log_nr, &event_count));

            // Create output file path, C++14 needed for std::filesystem.
            std::filesystem::path outpath = "export";

            uint32_t start_time;
            uint32_t end_time;
            kvmLogFileGetStartTime(kvm_handle, &start_time);
            kvmLogFileGetEndTime(kvm_handle, &end_time);

            uint32_t duration = end_time - start_time;

            outpath /= unix_to_iso_with_underscores(start_time).substr(0, 8) + "_" + location;
            outpath /= unix_to_iso_with_underscores(start_time) + "_" + location + "_" + std::to_string(duration) + "s" ".mf4";

            std::filesystem::create_directories(outpath.parent_path());

            std::cout << "Exporting log to: " << std::filesystem::absolute(outpath) << std::endl;
            MDF4Converter conv(outpath.string());

            // Add DBC files to the converter.
            std::string basePath = "C:/Users/Zbook15uG5/Documents/GitHub/DataProcessing/"; // Change this to your base path
            //std::string basePath = std::filesystem::path(argv[0]).parent_path().string();
            add_dbc_files_from_config(conv, basePath, dbcFiles);

            std::cout << "Rough Event Estimate: " << event_count << std::endl;


            int64_t counter = 0;
            int64_t update_frequency = 100000;
            int64_t total_bar = event_count / update_frequency;

            eventcounter.max.store(event_count);
            eventcounter.current.store(0);

            while ((status = kvmLogFileReadEvent(kvm_handle, &e)) != kvmERR_NOLOGMSG) {

                KvmException::throw_on_error(status);
                conv.convert_event(e);

                ++eventcounter.current;
                ++counter;
                if (counter == update_frequency) {
                    counter = 0;
                }           
            }
            eventcounter.current.store(event_count);
            ++filecounter.current;
            // Print status message
            std::cout << "Status: Log " << filecounter.current << " of " << filecounter.max << " converted." << std::endl;

            //std::cout << "Conversion finished"<< std::endl;
    }   
    }
    kvmClose(kvm_handle);
    
    return convertedFiles;
    exit(0);
}


