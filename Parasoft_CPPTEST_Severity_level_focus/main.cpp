#define NOMINMAX
#define XMLDocument WindowsXMLDocument

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#undef XMLDocument

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <tinyxml2.h>
#include <sstream>
#include <cctype>
#include <ctime>

using namespace tinyxml2;
using namespace std;

const char* VERSION = "1.1.5";
const bool DEBUG_MODE = false;

const char* severityLabel(int sev) {
    switch (sev) {
    case 0: return "Lowest";
    case 1: return "Low";
    case 2: return "Medium";
    case 3: return "High";
    case 4: return "Highest";
    default: return "Unknown";
    }
}

struct Violation {
    string filePath;
    int line;
    string message;
    int severity = -1;
    string category;
    string ruleCode;
};

bool hasHtmlExtension(const string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == string::npos) return false;
    string ext = path.substr(dot + 1);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "html" || ext == "htm";
}

int main() {
    cout << "Parasoft Static Analysis Report Parser v" << VERSION << endl;
    cout << "--------------------------------------------" << endl;

    string choice;
    do {
        try {
            char filename[MAX_PATH] = "";
            OPENFILENAMEA ofn;
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = nullptr;
            ofn.lpstrFilter = "All Supported Files (*.xml;*.html)\0*.xml;*.html\0XML Files (*.xml)\0*.xml\0HTML Files (*.html)\0*.html\0";
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            ofn.lpstrTitle = "Select Parasoft Report (XML or HTML)";

            if (!GetOpenFileNameA(&ofn)) {
                cerr << "No file selected or dialog cancelled." << endl;
                continue;
            }

            string filePath = filename;
            cout << "Selected report: " << filePath << endl;

            string projectName = "UnknownProject";
            vector<string> projectPaths;
            map<int, vector<Violation>> violationsBySeverity;
            int total = 0;

            if (hasHtmlExtension(filePath)) {
                ifstream file(filePath);
                if (!file.is_open()) {
                    cerr << "Failed to open HTML file." << endl;
                    continue;
                }

                string line;
                while (getline(file, line)) {
                    if (line.find(".c") != string::npos || line.find(".cpp") != string::npos || line.find(".h") != string::npos) {
                        Violation v;
                        size_t lineStart = line.find("gray\">") + 6;
                        size_t lineEnd = line.find("&nbsp;", lineStart);
                        if (lineStart != string::npos && lineEnd != string::npos) {
                            v.line = stoi(line.substr(lineStart, lineEnd - lineStart));
                        }

                        size_t msgStart = line.find("</font>");
                        size_t msgEnd = line.rfind("<font class=\"gray\">");
                        if (msgStart != string::npos && msgEnd != string::npos && msgEnd > msgStart) {
                            v.message = line.substr(msgStart + 7, msgEnd - msgStart - 7);
                        }

                        size_t ruleStart = line.rfind("gray\">");
                        size_t ruleEnd = line.rfind("</font>");
                        if (ruleStart != string::npos && ruleEnd != string::npos && ruleEnd > ruleStart) {
                            v.ruleCode = line.substr(ruleStart + 6, ruleEnd - ruleStart - 6);
                        }

                        v.filePath = filePath;
                        v.severity = 1; // Placeholder; real severity parsing from HTML can be improved
                        violationsBySeverity[v.severity].push_back(v);
                        total++;
                    }
                }
                file.close();
            }
            else {
                XMLDocument doc;
                if (doc.LoadFile(filePath.c_str()) != XML_SUCCESS) {
                    cerr << "Failed to load XML file." << endl;
                    continue;
                }

                XMLElement* root = doc.RootElement();
                if (!root) {
                    cerr << "No root element in XML." << endl;
                    continue;
                }

                XMLElement* codingStandardsElement = root->FirstChildElement("CodingStandards");
                if (!codingStandardsElement) {
                    cerr << "No <CodingStandards> element found." << endl;
                    continue;
                }

                map<string, pair<string, string>> ruleIdToDescCat;
                XMLElement* rulesElement = codingStandardsElement->FirstChildElement("Rules");
                if (rulesElement) {
                    XMLElement* rulesList = rulesElement->FirstChildElement("RulesList");
                    if (rulesList) {
                        for (XMLElement* rule = rulesList->FirstChildElement("Rule"); rule; rule = rule->NextSiblingElement("Rule")) {
                            const char* id = rule->Attribute("id");
                            const char* desc = rule->Attribute("desc");
                            const char* cat = rule->Attribute("cat");
                            if (id) {
                                ruleIdToDescCat[id] = { desc ? desc : "", cat ? cat : "" };
                            }
                        }
                    }
                }

                XMLElement* locations = root->FirstChildElement("Locations");
                if (locations) {
                    XMLElement* loc = locations->FirstChildElement("Loc");
                    if (loc) {
                        if (const char* proj = loc->Attribute("project")) projectName = proj;
                        for (; loc; loc = loc->NextSiblingElement("Loc")) {
                            const char* path = loc->Attribute("fsPath");
                            if (path) projectPaths.push_back(path);
                        }
                    }
                }

                XMLElement* stdViols = codingStandardsElement->FirstChildElement("StdViols");
                if (stdViols) {
                    for (XMLElement* v = stdViols->FirstChildElement("StdViol"); v; v = v->NextSiblingElement("StdViol")) {
                        Violation viol;
                        viol.filePath = v->Attribute("locFile") ? v->Attribute("locFile") : "";
                        viol.line = v->IntAttribute("ln", -1);
                        viol.message = v->Attribute("msg") ? v->Attribute("msg") : "";
                        const char* ruleAttr = v->Attribute("rule");
                        viol.ruleCode = ruleAttr ? ruleAttr : "";
                        auto it = ruleIdToDescCat.find(viol.ruleCode);
                        if (it != ruleIdToDescCat.end()) {
                            viol.category = it->second.second;
                            if (viol.message.empty())
                                viol.message = it->second.first;
                        }
                        viol.severity = v->IntAttribute("sev", 0);
                        violationsBySeverity[viol.severity].push_back(viol);
                        total++;
                    }
                }
            }

            time_t now = time(nullptr);
            char timebuf[32];
            strftime(timebuf, sizeof(timebuf), "%Y%m%d_%H%M%S", localtime(&now));
            string htmlFilePath = projectName + "_report_" + timebuf + ".html";

            ofstream html(htmlFilePath);
            html << "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Parasoft Report</title><style>"
                << "body { font-family: Arial; margin: 20px; }"
                << ".severity { margin: 10px 0; font-weight: bold; padding: 10px; border-radius: 4px; color: #fff; }"
                << ".severity-0 { background-color: #3498db; }"
                << ".severity-1 { background-color: #2ecc71; }"
                << ".severity-2 { background-color: #f39c12; }"
                << ".severity-3 { background-color: #e67e22; }"
                << ".severity-4 { background-color: #e74c3c; }"
                << ".violation { margin-left: 20px; margin-bottom: 8px; }"
                << ".header { font-size: 14px; margin-bottom: 20px; color: #555; }"
                << "</style></head><body>";

            html << "<h1>Parasoft Static Analysis Report - " << projectName << "</h1>";
            html << "<div class='header'><strong>Project Files:</strong><ul>";
            for (const auto& path : projectPaths) {
                html << "<li>" << path << "</li>";
            }
            html << "</ul></div>";

            html << "<p>Total Violations: " << total << "</p>";

            for (const auto& [sev, list] : violationsBySeverity) {
                html << "<div class='severity severity-" << sev << "'>" << severityLabel(sev)
                    << " Severity - " << list.size() << " violations</div>";
                for (const auto& v : list) {
                    html << "<div class='violation'>"
                        << "<b>Location:</b> " << v.filePath << "<br>"
                        << "<b>Line:</b> " << v.line << "<br>"
                        << "<b>Rule:</b> " << v.ruleCode << "<br>"
                        << "<i>" << v.message << "</i></div>";
                }
            }

            html << "</body></html>";
            html.close();

            cout << "Report written to: " << htmlFilePath << endl;
            Sleep(100);
            ShellExecuteA(NULL, "open", htmlFilePath.c_str(), NULL, NULL, SW_SHOWNORMAL);

        }
        catch (const std::exception& ex) {
            cerr << "Unexpected error: " << ex.what() << endl;
        }
        catch (...) {
            cerr << "Unknown error occurred during report generation." << endl;
        }

        cout << "\nWould you like to generate another report? (y = yes, q = quit): ";
        getline(cin, choice);

    } while (choice == "y" || choice == "Y");

    cout << "Exiting Parasoft Report Parser. Goodbye!" << endl;
    return 0;
}
