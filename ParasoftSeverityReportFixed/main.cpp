
// This program reads a Parasoft static analysis report in XML format and generates an HTML report
// grouped by severity level, showing the rule ID, description, and locations of violations.
//this is the standard version of the code, we commenting it our and moving to Cody to see if it works
#if 0
#define NOMINMAX
#define XMLDocument WindowsXMLDocument

#include <windows.h>
#include <commdlg.h>

#undef XMLDocument

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <tinyxml2.h>

using namespace tinyxml2;
using namespace std;

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

struct RuleInfo {
    string id;
    string desc;
    vector<pair<string, int>> locations;
};

string toLower(const string& str) {
    string out = str;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

string openFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "XML Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = "Select Parasoft XML Report";

    if (GetOpenFileNameA(&ofn)) {
        return string(filename);
    }
    else {
        cerr << "No file selected or dialog cancelled." << endl;
        return "";
    }
}

int main() {
    string filePath = openFileDialog();
    if (filePath.empty()) return 1;

    XMLDocument doc;
    XMLError result = doc.LoadFile(filePath.c_str());
    if (result != XML_SUCCESS) {
        cerr << "Failed to load XML file!" << endl;
        return 1;
    }

    map<string, vector<pair<string, int>>> violationLocations;
    XMLElement* root = doc.RootElement();

    XMLElement* modulesRoot = root->FirstChildElement("Modules");
    if (modulesRoot) {
        for (XMLElement* module = modulesRoot->FirstChildElement("Module"); module; module = module->NextSiblingElement("Module")) {
            for (XMLElement* fileElem = module->FirstChildElement("File"); fileElem; fileElem = fileElem->NextSiblingElement("File")) {
                const char* filePathAttr = fileElem->Attribute("path");
                if (!filePathAttr) continue;
                XMLElement* viols = fileElem->FirstChildElement("Viols");
                if (!viols) continue;
                for (XMLElement* v = viols->FirstChildElement("V"); v; v = v->NextSiblingElement("V")) {
                    const char* id = v->Attribute("id");
                    int line = v->IntAttribute("line", -1);
                    if (id && line >= 0) {
                        violationLocations[toLower(id)].emplace_back(filePathAttr, line);
                    }
                }
            }
        }
    }
    else {
        cerr << "Warning: <Modules> section not found in XML." << endl;
    }

    map<int, vector<RuleInfo>> rulesBySeverity;
    XMLElement* rulesList = root->FirstChildElement("CodingStandards")
        ? root->FirstChildElement("CodingStandards")->FirstChildElement("Rules")
        ? root->FirstChildElement("CodingStandards")->FirstChildElement("Rules")->FirstChildElement("RulesList")
        : nullptr
        : nullptr;

    if (!rulesList) {
        cerr << "Error: <RulesList> not found in XML." << endl;
    }
    else {
        for (XMLElement* rule = rulesList->FirstChildElement("Rule"); rule; rule = rule->NextSiblingElement("Rule")) {
            const char* id = rule->Attribute("id");
            const char* desc = rule->Attribute("desc");
            int sev = rule->IntAttribute("sev", -1);
            XMLElement* stats = rule->FirstChildElement("Stats");
            int total = stats ? stats->IntAttribute("total", 0) : 0;

            if (id && desc && sev >= 0 && total > 0) {
                RuleInfo info;
                info.id = id;
                info.desc = desc;

                string idKey = toLower(id);
                if (violationLocations.count(idKey)) {
                    info.locations = violationLocations[idKey];
                    cout << "Matched rule: " << id << " with " << info.locations.size() << " locations.\n";
                }
                else {
                    cout << "Rule " << id << " has no location matches.\n";
                }

                rulesBySeverity[sev].push_back(info);
            }
        }
    }

    ofstream html("parasoft_report_by_severity.html");
    html << "<html><head><title>Parasoft Report by Severity</title></head><body>\n";
    html << "<h1>Static Analysis Results Grouped by Severity</h1>\n";

    for (const auto& [sev, rules] : rulesBySeverity) {
        html << "<h2>Severity: " << severityLabel(sev) << "</h2>\n<ul>\n";
        for (const auto& rule : rules) {
            html << "  <li><b>" << rule.id << "</b>: " << rule.desc << "<br/>\n";
            for (const auto& [file, line] : rule.locations) {
                html << "    ↳ <i>" << file << ":" << line << "</i><br/>\n";
            }
            html << "  </li>\n";
        }
        html << "</ul>\n";
    }

    html << "</body></html>\n";
    html.close();

    cout << "HTML report generated as 'parasoft_report_by_severity.html'" << endl;
    return 0;
}
#endif
//from now it is the cody version of the code
#define NOMINMAX
#define XMLDocument WindowsXMLDocument

#include <windows.h>
#include <commdlg.h>

#undef XMLDocument

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <tinyxml2.h>

using namespace tinyxml2;
using namespace std;

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

struct RuleInfo {
    string id;
    string desc;
    vector<pair<string, int>> locations;
};

string toLower(const string& str) {
    string out = str;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

string openFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "XML Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = "Select Parasoft XML Report";

    if (GetOpenFileNameA(&ofn)) {
        return string(filename);
    }
    else {
        cerr << "No file selected or dialog cancelled." << endl;
        return "";
    }
}

int main() {
    string filePath = openFileDialog();
    if (filePath.empty()) return 1;

    XMLDocument doc;
    XMLError result = doc.LoadFile(filePath.c_str());
    if (result != XML_SUCCESS) {
        cerr << "Failed to load XML file!" << endl;
        return 1;
    }

    map<string, vector<pair<string, int>>> violationLocations;
    XMLElement* root = doc.RootElement();

    XMLElement* modulesRoot = root->FirstChildElement("Modules");
    if (modulesRoot) {
        for (XMLElement* module = modulesRoot->FirstChildElement("Module"); module; module = module->NextSiblingElement("Module")) {
            for (XMLElement* fileElem = module->FirstChildElement("File"); fileElem; fileElem = fileElem->NextSiblingElement("File")) {
                const char* filePathAttr = fileElem->Attribute("path");
                if (!filePathAttr) continue;
                string filePath = filePathAttr;

                XMLElement* viols = fileElem->FirstChildElement("Viols");
                if (!viols) continue;

                for (XMLElement* v = viols->FirstChildElement("V"); v; v = v->NextSiblingElement("V")) {
                    const char* id = v->Attribute("id");
                    int line = v->IntAttribute("line", -1);
                    if (id && line >= 0) {
                        // Store with original case to ensure proper matching
                        violationLocations[id].emplace_back(filePath, line);
                    }
                }
            }
        }
    }
    else {
        cerr << "Warning: <Modules> section not found in XML." << endl;
    }

    map<int, vector<RuleInfo>> rulesBySeverity;
    XMLElement* rulesList = root->FirstChildElement("CodingStandards")
        ? root->FirstChildElement("CodingStandards")->FirstChildElement("Rules")
        ? root->FirstChildElement("CodingStandards")->FirstChildElement("Rules")->FirstChildElement("RulesList")
        : nullptr
        : nullptr;

    if (!rulesList) {
        cerr << "Error: <RulesList> not found in XML." << endl;
    }
    else {
        for (XMLElement* rule = rulesList->FirstChildElement("Rule"); rule; rule = rule->NextSiblingElement("Rule")) {
            const char* id = rule->Attribute("id");
            const char* desc = rule->Attribute("desc");
            int sev = rule->IntAttribute("sev", -1);
            XMLElement* stats = rule->FirstChildElement("Stats");
            int total = stats ? stats->IntAttribute("total", 0) : 0;

            if (id && desc && sev >= 0 && total > 0) {
                RuleInfo info;
                info.id = id;
                info.desc = desc;

                // Check if we have locations for this rule
                if (violationLocations.count(id)) {
                    info.locations = violationLocations[id];
                    cout << "Matched rule: " << id << " with " << info.locations.size() << " locations.\n";
                }
                else {
                    cout << "Rule " << id << " has no location matches.\n";
                }

                rulesBySeverity[sev].push_back(info);
            }
        }
    }

    ofstream html("parasoft_report_by_severity.html");
    html << "<!DOCTYPE html>\n<html><head>\n";
    html << "<title>Parasoft Report by Severity</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html << "h1 { color: #2c3e50; }\n";
    html << "h2 { color: #3498db; margin-top: 30px; }\n";
    html << "ul { list-style-type: none; padding-left: 10px; }\n";
    html << "li { margin-bottom: 15px; border-left: 3px solid #7f8c8d; padding-left: 10px; }\n";
    html << ".rule-id { font-weight: bold; color: #c0392b; }\n";
    html << ".location { font-style: italic; color: #27ae60; margin-left: 20px; display: block; }\n";
    html << "</style>\n";
    html << "</head><body>\n";
    html << "<h1>Static Analysis Results Grouped by Severity</h1>\n";

    for (const auto& [sev, rules] : rulesBySeverity) {
        html << "<h2>Severity: " << severityLabel(sev) << "</h2>\n<ul>\n";
        for (const auto& rule : rules) {
            html << "  <li><span class='rule-id'>" << rule.id << "</span>: " << rule.desc << "<br/>\n";

            if (rule.locations.empty()) {
                html << "    <span class='location'>No specific locations found</span>\n";
            }
            else {
                for (const auto& [file, line] : rule.locations) {
                    html << "    <span class='location'>" << file << ":" << line << "</span>\n";
                }
            }

            html << "  </li>\n";
        }
        html << "</ul>\n";
    }

    html << "</body></html>\n";
    html.close();

    cout << "HTML report generated as 'parasoft_report_by_severity.html'" << endl;

    // Open the report in the default browser
    ShellExecuteA(NULL, "open", "parasoft_report_by_severity.html", NULL, NULL, SW_SHOWNORMAL);

    return 0;
}
