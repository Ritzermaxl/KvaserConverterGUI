#include "hello_imgui/hello_imgui.h"

#include "converter.h"
#include <future>



Counters counters;
Counters filecounter;
Counters eventcounter;

std::vector<LogFile> logs;
std::future<std::vector<LogFile>> futureLogs;
std::vector<ConvertedLogs> convertedlogs;
std::future<std::vector<ConvertedLogs>> futureConvtertedLogs;

int main(int , char *[])
{
        // Hello ImGui params (they hold the settings as well as the Gui callbacks)
    HelloImGui::RunnerParams runnerParams;
    runnerParams.appWindowParams.windowTitle = "Docking Demo";
    runnerParams.imGuiWindowParams.menuAppTitle = "Docking Demo";
    runnerParams.appWindowParams.windowGeometry.size = {1200, 1000};
    runnerParams.appWindowParams.restorePreviousGeometry = true;

    // Our application uses a borderless window, but is movable/resizable
    runnerParams.appWindowParams.borderless = true;
    runnerParams.appWindowParams.borderlessMovable = true;
    runnerParams.appWindowParams.borderlessResizable = true;
    runnerParams.appWindowParams.borderlessClosable = true;
    
    auto guiFunction = []() {
        ImGui::Text("ConvertLogFiles");                    // Display a simple label

        if (ImGui::Button("Bye!"))                 // Display a button
            // and immediately handle its action if it is clicked!
            HelloImGui::GetRunnerParams()->appShallExit = true;

        if (ImGui::Button("List Log Files")) {
			std::string inputfilename = "E:/LOG00000.KMF";
            
            futureLogs = std::async(std::launch::async, listLogFiles, inputfilename, std::ref(counters));

        };

        if (futureLogs.valid())
            if (futureLogs.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                logs = futureLogs.get();

        if (ImGui::Button("Clear Logs"))                 // Display a button
            // and immediately handle its action if it is clicked!
            logs.clear();


        float currentfilelistProgress = float(counters.current.load()) / float(counters.max.load());

        ImGui::ProgressBar(currentfilelistProgress,HelloImGui::EmToVec2(21.0f, 2.0f));

        if (ImGui::Button("Convert Log Files")) {
			std::string location = "Innenhof";
            std::vector<int> logstoexport = {1, 8, 9, 12, 22, 38, 58, };

            futureConvtertedLogs = std::async(std::launch::async, convertlogfiles, logstoexport, location, std::ref(filecounter), std::ref(eventcounter));
        };

        if (futureConvtertedLogs.valid())
            if (futureConvtertedLogs.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                convertedlogs = futureConvtertedLogs.get();

        

        // Progress Calculations:
        float currentFileProgress = float(filecounter.current.load()) / float(filecounter.max.load());
        float currentLogProgress = float(eventcounter.current.load()) / float(eventcounter.max.load());

        ImGui::Text("Overall Progress:"); 
        ImGui::ProgressBar(currentFileProgress, HelloImGui::EmToVec2(21.0f, 2.0f));
        ImGui::Text("Current File Progress:"); 
        ImGui::ProgressBar(currentLogProgress, HelloImGui::EmToVec2(21.0f, 2.0f));

        float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing(); // Declaration and initialization
        if (!logs.empty()) {

            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;


            ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
            if (ImGui::BeginTable("Log Files", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                ImGui::TableSetupColumn("Nr");
                ImGui::TableSetupColumn("Start Date");
                ImGui::TableSetupColumn("End Date");
                ImGui::TableSetupColumn("Duration");
                ImGui::TableSetupColumn("Event Count");
				ImGui::TableSetupColumn("Select");
                ImGui::TableHeadersRow();

                for (auto& log : logs) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
					ImGui::Text("%d", log.number);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", log.startDate.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", log.endDate.c_str());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", log.duration.c_str());
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%d", log.eventCount);
					ImGui::TableSetColumnIndex(5);
					ImGui::Checkbox(("##" + std::to_string(log.number)).c_str(), &log.selected);
                }
                ImGui::EndTable();
            };
        };

     };
    HelloImGui::Run(guiFunction, "Hello, globe", true);
    return 0;
}
