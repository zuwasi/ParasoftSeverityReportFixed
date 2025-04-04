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
#include <sstream>

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

struct Violation {
    string filePath;
    int line;
    string message;
};

struct RuleInfo {
    string id;
    string desc;
    vector<Violation> violations;
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
    cout << "Starting Parasoft XML Report Parser" << endl;

    string filePath = openFileDialog();
    if (filePath.empty()) {
        cerr << "No file selected. Exiting." << endl;
        return 1;
    }

    cout << "Opening XML file: " << filePath << endl;

    XMLDocument doc;
    XMLError result = doc.LoadFile(filePath.c_str());
    if (result != XML_SUCCESS) {
        cerr << "Failed to load XML file! Error code: " << result << endl;
        return 1;
    }

    cout << "XML file loaded successfully" << endl;

    // Map to store rule ID -> violations
    map<string, vector<Violation>> violationsByRuleId;

    // Map to store severity -> rules
    map<int, vector<RuleInfo>> rulesBySeverity;

    XMLElement* root = doc.RootElement();
    if (!root) {
        cerr << "Error: XML document has no root element" << endl;
        return 1;
    }

    cout << "Root element name: " << (root->Name() ? root->Name() : "unnamed") << endl;

    // First, collect all violations from the modules section (traditional format)
    XMLElement* modulesRoot = root->FirstChildElement("Modules");
    if (modulesRoot) {
        cout << "Found Modules section" << endl;
        int moduleCount = 0;
        int fileCount = 0;
        int violationCount = 0;

        for (XMLElement* module = modulesRoot->FirstChildElement("Module"); module; module = module->NextSiblingElement("Module")) {
            moduleCount++;
            for (XMLElement* fileElem = module->FirstChildElement("File"); fileElem; fileElem = fileElem->NextSiblingElement("File")) {
                fileCount++;
                const char* filePathAttr = fileElem->Attribute("path");
                if (!filePathAttr) {
                    continue;
                }
                string filePath = filePathAttr;

                XMLElement* viols = fileElem->FirstChildElement("Viols");
                if (!viols) continue;

                for (XMLElement* v = viols->FirstChildElement("V"); v; v = v->NextSiblingElement("V")) {
                    violationCount++;
                    const char* id = v->Attribute("id");
                    int line = v->IntAttribute("line", -1);
                    const char* msg = v->Attribute("msg");

                    if (id && line >= 0) {
                        Violation violation;
                        violation.filePath = filePath;
                        violation.line = line;
                        violation.message = msg ? msg : "";

                        violationsByRuleId[id].push_back(violation);
                    }
                }
            }
        }

        cout << "Found " << moduleCount << " modules, " << fileCount << " files, and " << violationCount << " violations in Modules section" << endl;
    }
    else {
        cout << "No <Modules> section found with violations, this might be a different format." << endl;
    }

    // Now also collect violations from StdViols section (which Parasoft C++test uses)
    XMLElement* codingStandardsElement = root->FirstChildElement("CodingStandards");
    if (codingStandardsElement) {
        XMLElement* stdViolsElement = codingStandardsElement->FirstChildElement("StdViols");
        if (stdViolsElement) {
            cout << "Found StdViols section" << endl;
            int violationCount = 0;

            for (XMLElement* v = stdViolsElement->FirstChildElement("StdViol"); v; v = v->NextSiblingElement("StdViol")) {
                violationCount++;
                const char* msg = v->Attribute("msg");
                const char* rule = v->Attribute("rule");
                int line = v->IntAttribute("ln", -1);
                int sev = v->IntAttribute("sev", -1);
                const char* locFile = v->Attribute("locFile");

                if (rule && line >= 0) {
                    Violation violation;
                    violation.filePath = locFile ? locFile : "Unknown";
                    violation.line = line;
                    violation.message = msg ? msg : "";

                    violationsByRuleId[rule].push_back(violation);
                }
            }

            cout << "Found " << violationCount << " violations in StdViols section" << endl;
        }
        else {
            cout << "No <StdViols> section found in XML." << endl;
        }
    }

    cout << "Found " << violationsByRuleId.size() << " rules with violations" << endl;

    // Now, process the rules and associate them with their violations
    if (codingStandardsElement) {
        XMLElement* rulesElement = codingStandardsElement->FirstChildElement("Rules");
        if (rulesElement) {
            XMLElement* rulesList = rulesElement->FirstChildElement("RulesList");
            if (rulesList) {
                int ruleCount = 0;
                int rulesWithViolationsCount = 0;

                for (XMLElement* rule = rulesList->FirstChildElement("Rule"); rule; rule = rule->NextSiblingElement("Rule")) {
                    ruleCount++;
                    const char* id = rule->Attribute("id");
                    const char* desc = rule->Attribute("desc");
                    int sev = rule->IntAttribute("sev", -1);

                    if (id && desc && sev >= 0) {
                        // Only add rules that have violations
                        if (violationsByRuleId.count(id) > 0 && !violationsByRuleId[id].empty()) {
                            rulesWithViolationsCount++;
                            RuleInfo info;
                            info.id = id;
                            info.desc = desc;
                            info.violations = violationsByRuleId[id];

                            cout << "Adding rule: " << id << " (severity " << sev << ") with "
                                << info.violations.size() << " violations" << endl;

                            rulesBySeverity[sev].push_back(info);
                        }
                    }
                }

                cout << "Processed " << ruleCount << " rules, " << rulesWithViolationsCount << " have violations" << endl;
            }
            else {
                cerr << "Error: <RulesList> not found in XML." << endl;
            }
        }
        else {
            cerr << "Error: <Rules> not found in XML." << endl;
        }
    }
    else {
        cerr << "Error: <CodingStandards> not found in XML." << endl;
    }

    // Handle cases where we found violations but couldn't match them to rule definitions
    // This can happen if the XML format is different from what we expect
    if (!violationsByRuleId.empty() && rulesBySeverity.empty()) {
        cout << "Found violations but couldn't match them to rule definitions." << endl;
        cout << "Adding violations with default severity grouping..." << endl;

        // Create a map to group violations by severity
        map<int, map<string, RuleInfo>> tempRulesBySev;

        // Process each violation and group by rule ID
        for (const auto& [ruleId, violations] : violationsByRuleId) {
            if (violations.empty()) continue;

            // Extract severity from the rule ID if possible, or default to 1 (Low)
            int sev = 1; // Default to Low

            // For each violation, create rule info
            RuleInfo info;
            info.id = ruleId;
            info.desc = "No description available"; // Default description
            info.violations = violations;

            // Try to extract severity from the first violation
            for (const auto& v : violations) {
                if (v.message.find("sev=") != string::npos) {
                    size_t pos = v.message.find("sev=");
                    string sevStr = v.message.substr(pos + 4, 1);
                    try {
                        sev = stoi(sevStr);
                    }
                    catch (...) {
                        // Keep default severity if parsing fails
                    }
                    break;
                }
            }

            // Store in temporary map
            tempRulesBySev[sev][ruleId] = info;
        }

        // Transfer to final rulesBySeverity map
        for (const auto& [sev, rulesMap] : tempRulesBySev) {
            for (const auto& [ruleId, info] : rulesMap) {
                rulesBySeverity[sev].push_back(info);
            }
        }
    }

    // Report counts per severity
    for (const auto& [sev, rules] : rulesBySeverity) {
        cout << "Severity " << severityLabel(sev) << " has " << rules.size() << " rules with violations" << endl;
    }

    // Generate HTML report
    string htmlFilePath = "parasoft_report_by_severity.html";
    ofstream htmlFile(htmlFilePath);

    if (!htmlFile.is_open()) {
        cerr << "Error: Could not open output file for writing: " << htmlFilePath << endl;
        return 1;
    }

    // Write HTML header
    htmlFile << "<!DOCTYPE html>\n"
        << "<html>\n"
        << "<head>\n"
        << "  <title>Parasoft Report by Severity</title>\n"
        << "  <style>\n"
        << "    body { font-family: Arial, sans-serif; margin: 20px; }\n"
        << "    h1 { color: #2c3e50; }\n"
        << "    .tree { margin-left: 20px; }\n"
        << "    .severity { color: white; font-weight: bold; padding: 10px; margin: 10px 0; border-radius: 5px; }\n"
        << "    .severity-0 { background-color: #3498db; }\n"
        << "    .severity-1 { background-color: #2ecc71; }\n"
        << "    .severity-2 { background-color: #f39c12; }\n"
        << "    .severity-3 { background-color: #e67e22; }\n"
        << "    .severity-4 { background-color: #e74c3c; }\n"
        << "    .rule { margin: 10px 0; padding: 8px; background-color: #f8f9fa; border-left: 4px solid #2c3e50; }\n"
        << "    .rule-id { font-weight: bold; color: #2c3e50; }\n"
        << "    .rule-desc { margin-left: 5px; }\n"
        << "    .violations { margin-left: 30px; }\n"
        << "    .violation { margin: 5px 0; padding: 5px; background-color: #f1f2f6; border-left: 3px solid #a4b0be; }\n"
        << "    .file-path { color: #2980b9; }\n"
        << "    .line-number { font-weight: bold; color: #c0392b; }\n"
        << "    .message { display: block; margin-top: 3px; color: #34495e; font-style: italic; }\n"
        << "    .toggle-btn { cursor: pointer; user-select: none; }\n"
        << "    .hidden { display: none; }\n"
        << "  </style>\n"
        << "  <script>\n"
        << "    function toggleSection(id) {\n"
        << "      const element = document.getElementById(id);\n"
        << "      if (element.classList.contains('hidden')) {\n"
        << "        element.classList.remove('hidden');\n"
        << "      } else {\n"
        << "        element.classList.add('hidden');\n"
        << "      }\n"
        << "    }\n"
        << "  </script>\n"
        << "</head>\n"
        << "<body>\n"
        << "  <h1>Static Analysis Results - Tree View by Severity</h1>\n";

    if (rulesBySeverity.empty()) {
        htmlFile << "  <p>No rule violations found in the analysis results.</p>\n";
    }
    else {
        // For each severity level
        for (const auto& [sev, rules] : rulesBySeverity) {
            htmlFile << "  <div class='severity severity-" << sev << "'>" << severityLabel(sev)
                << " Severity <span class='toggle-btn' onclick='toggleSection(\"sev-" << sev << "\")'>▼</span></div>\n"
                << "  <div id='sev-" << sev << "' class='tree'>\n";

            // For each rule in this severity
            for (const auto& rule : rules) {
                string ruleId = rule.id;
                // Replace any characters that might cause issues with HTML IDs
                replace(ruleId.begin(), ruleId.end(), '.', '_');
                replace(ruleId.begin(), ruleId.end(), ' ', '_');

                htmlFile << "    <div class='rule'>\n"
                    << "      <span class='rule-id'>" << rule.id << "</span>\n"
                    << "      <span class='rule-desc'>" << rule.desc << "</span>\n"
                    << "      <span class='toggle-btn' onclick='toggleSection(\"rule-" << ruleId << "\")'>▼</span>\n"
                    << "      <div id='rule-" << ruleId << "' class='violations'>\n";

                if (rule.violations.empty()) {
                    htmlFile << "        <div class='no-violations'>No specific violations found</div>\n";
                }
                else {
                    // For each violation of this rule
                    for (const auto& violation : rule.violations) {
                        htmlFile << "        <div class='violation'>\n"
                            << "          <span class='file-path'>" << violation.filePath << "</span>:"
                            << "<span class='line-number'>" << violation.line << "</span>\n";

                        if (!violation.message.empty()) {
                            htmlFile << "          <span class='message'>" << violation.message << "</span>\n";
                        }

                        htmlFile << "        </div>\n";
                    }
                }

                htmlFile << "      </div>\n"  // End violations
                    << "    </div>\n";   // End rule
            }

            htmlFile << "  </div>\n";  // End tree for this severity
        }
    }

    htmlFile << "</body>\n</html>\n";
    htmlFile.close();

    if (htmlFile.good()) {
        cout << "HTML report successfully generated: " << htmlFilePath << endl;
        // Open the report in the default browser
        ShellExecuteA(NULL, "open", htmlFilePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return 0;
    }
    else {
        cerr << "Error writing to HTML file" << endl;
        return 1;
    }
}