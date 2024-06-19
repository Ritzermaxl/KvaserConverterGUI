#include "hello_imgui/hello_imgui.h"

#include "converter.h"
#include <future>



Counters counters;
std::vector<LogFile> logs;

int main(int , char *[])
{
    
    auto guiFunction = []() {
        ImGui::Text("Hello, ");                    // Display a simple label
        HelloImGui::ImageFromAsset("world.jpg");   // Display a static image
        if (ImGui::Button("Bye!"))                 // Display a button
            // and immediately handle its action if it is clicked!
            HelloImGui::GetRunnerParams()->appShallExit = true;

        if (ImGui::Button("List Log Files")) {
			std::string inputfilename = "E:/LOG00000.KMF";
            
            std::future<std::vector<LogFile>> futureLogs = std::async(std::launch::async, retrieveLogFiles, inputfilename, std::ref(counters));
            logs = futureLogs.get();

        };
        float currentProgress = float(counters.current.load()) / float(counters.max.load());

        ImGui::ProgressBar(currentProgress);

        float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing(); // Declaration and initialization
        if (!logs.empty()) {
			ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);

            if (ImGui::BeginTable("Log Files", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
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
