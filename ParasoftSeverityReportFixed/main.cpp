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
                    const char* rid = v->Attribute("rid");
                    int line = v->IntAttribute("line", -1);
                    if (rid && line >= 0) {
                        violationLocations[rid].emplace_back(filePathAttr, line);
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
            const char* rid = rule->Attribute("rid");
            const char* desc = rule->Attribute("desc");
            int sev = rule->IntAttribute("sev", -1);
            XMLElement* stats = rule->FirstChildElement("Stats");
            int total = stats ? stats->IntAttribute("total", 0) : 0;

            if (id && rid && desc &&