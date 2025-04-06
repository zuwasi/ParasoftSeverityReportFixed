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

const char* VERSION = "1.0.6";
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
};

int main() {
    cout << "Parasoft Static Analysis Report Parser v" << VERSION << endl;
    cout << "--------------------------------------------" << endl;

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

    if (!GetOpenFileNameA(&ofn)) {
        cerr << "No file selected or dialog cancelled." << endl;
        return 1;
    }

    string filePath = filename;
    cout << "Selected XML report: " << filePath << endl;

    XMLDocument doc;
    if (doc.LoadFile(filePath.c_str()) != XML_SUCCESS) {
        cerr << "Failed to load XML file." << endl;
        return 1;
    }

    XMLElement* root = doc.RootElement();
    if (!root) {
        cerr << "No root element in XML." << endl;
        return 1;
    }

    XMLElement* codingStandardsElement = root->FirstChildElement("CodingStandards");
    if (!codingStandardsElement) {
        cerr << "No <CodingStandards> element found." << endl;
        return 1;
    }

    string projectName = "UnknownProject";
    XMLElement* locations = root->FirstChildElement("Locations");
    vector<string> projectPaths;
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

    map<int, vector<Violation>> violationsBySeverity;
    int total = 0;
    XMLElement* stdViols = codingStandardsElement->FirstChildElement("StdViols");
    if (stdViols) {
        for (XMLElement* v = stdViols->FirstChildElement("StdViol"); v; v = v->NextSiblingElement("StdViol")) {
            Violation viol;
            viol.filePath = v->Attribute("locFile") ? v->Attribute("locFile") : "";
            viol.line = v->IntAttribute("ln", -1);
            viol.message = v->Attribute("msg") ? v->Attribute("msg") : "";
            viol.category = v->Attribute("cat") ? v->Attribute("cat") : "";
            viol.severity = v->IntAttribute("sev", 0);
            violationsBySeverity[viol.severity].push_back(viol);
            total++;
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
                << "<i>" << v.message << "</i></div>";
        }
    }

    html << "</body></html>";
    html.close();

    cout << "Report written to: " << htmlFilePath << endl;
    Sleep(100);
    ShellExecuteA(NULL, "open", htmlFilePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return 0;
}