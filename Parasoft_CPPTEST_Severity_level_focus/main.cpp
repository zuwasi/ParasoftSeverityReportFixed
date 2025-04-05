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
#include <set>
#include <tinyxml2.h>
#include <sstream>
#include <cctype>

using namespace tinyxml2;
using namespace std;

// Version
const char* VERSION = "1.0.2";

// Enable debug mode
const bool DEBUG_MODE = false;

// Constants for severity labels
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

// Structure to hold violation information
struct Violation {
    string filePath;      // File path where violation occurred
    int line;             // Line number of violation
    string message;       // Violation message/description
    int severity = -1;    // Severity level (if available)
    string category;      // Category of violation
};

// Structure to hold rule information
struct RuleInfo {
    string id;            // Rule ID
    string desc;          // Rule description
    string category;      // Category of rule
    vector<Violation> violations; // List of violations for this rule
};

// Open file dialog to select XML file
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

// Main function
int main() {
    cout << "Parasoft Static Analysis XML Report Parser v" << VERSION << endl;
    cout << "--------------------------------------------" << endl;

    string filePath = openFileDialog();
    if (filePath.empty()) {
        cerr << "No file selected. Exiting." << endl;
        return 1;
    }

    cout << "Opening XML file: " << filePath << endl;

    // Parse XML file
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

    // Get root element
    XMLElement* root = doc.RootElement();
    if (!root) {
        cerr << "Error: XML document has no root element" << endl;
        return 1;
    }

    cout << "Root element name: " << (root->Name() ? root->Name() : "unnamed") << endl;

    // Validate that this is a static analysis report
    XMLElement* codingStandardsElement = root->FirstChildElement("CodingStandards");
    if (!codingStandardsElement) {
        cerr << "Error: This does not appear to be a Static Analysis report." << endl;
        cerr << "This tool only processes Parasoft Static Analysis reports, not Unit Test reports." << endl;
        return 1;
    }

    // First, collect all violations from the modules section (traditional format)
    XMLElement* modulesRoot = root->FirstChildElement("Modules");
    int moduleViolationCount = 0;

    if (modulesRoot) {
        cout << "Found Modules section" << endl;
        int moduleCount = 0;
        int fileCount = 0;

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
                    moduleViolationCount++;
                    const char* id = v->Attribute("id");
                    int line = v->IntAttribute("line", -1);
                    const char* msg = v->Attribute("msg");
                    int sev = v->IntAttribute("sev", -1);
                    const char* cat = v->Attribute("cat");

                    if (id && line >= 0) {
                        Violation violation;
                        violation.filePath = filePath;
                        violation.line = line;
                        violation.message = msg ? msg : "";
                        violation.severity = sev;
                        violation.category = cat ? cat : "";

                        violationsByRuleId[id].push_back(violation);
                    }
                }
            }
        }

        cout << "Found " << moduleCount << " modules, " << fileCount << " files, and "
            << moduleViolationCount << " violations in Modules section" << endl;
    }
    else {
        cout << "No <Modules> section found with violations." << endl;
    }

    // Now collect violations from StdViols section (which Parasoft C++test uses)
    int stdViolationCount = 0;
    int flowViolationCount = 0;

    XMLElement* stdViolsElement = codingStandardsElement->FirstChildElement("StdViols");
    if (stdViolsElement) {
        cout << "Found StdViols section" << endl;

        // Process regular StdViol elements
        for (XMLElement* v = stdViolsElement->FirstChildElement("StdViol"); v; v = v->NextSiblingElement("StdViol")) {
            stdViolationCount++;
            const char* msg = v->Attribute("msg");
            const char* rule = v->Attribute("rule");
            int line = v->IntAttribute("ln", -1);
            int sev = v->IntAttribute("sev", -1);
            const char* locFile = v->Attribute("locFile");
            const char* cat = v->Attribute("cat");

            if (rule && line >= 0) {
                Violation violation;
                violation.filePath = locFile ? locFile : "Unknown";
                violation.line = line;
                violation.message = msg ? msg : "";
                violation.severity = sev;
                violation.category = cat ? cat : "";

                if (DEBUG_MODE) {
                    cout << "Found StdViol: rule=" << rule << ", sev=" << sev << ", line=" << line << endl;
                }

                violationsByRuleId[rule].push_back(violation);
            }
        }

        // Also process FlowViol elements within StdViols
        for (XMLElement* v = stdViolsElement->FirstChildElement("FlowViol"); v; v = v->NextSiblingElement("FlowViol")) {
            flowViolationCount++;
            const char* msg = v->Attribute("msg");
            const char* rule = v->Attribute("rule");
            int line = v->IntAttribute("ln", -1);
            int sev = v->IntAttribute("sev", -1);
            const char* locFile = v->Attribute("locFile");
            const char* cat = v->Attribute("cat");

            if (rule && line >= 0) {
                Violation violation;
                violation.filePath = locFile ? locFile : "Unknown";
                violation.line = line;
                violation.message = msg ? msg : "";
                violation.severity = sev;
                violation.category = cat ? cat : "";

                if (DEBUG_MODE) {
                    cout << "Found FlowViol: rule=" << rule << ", sev=" << sev << ", line=" << line << endl;
                }

                violationsByRuleId[rule].push_back(violation);
            }
        }

        cout << "Found " << stdViolationCount << " standard violations and "
            << flowViolationCount << " flow violations in StdViols section" << endl;
    }
    else {
        cout << "No <StdViols> section found in XML." << endl;
    }

    if (DEBUG_MODE) {
        // Output detailed violation count information for debugging
        cout << "\n=== DETAILED VIOLATION REPORT ===" << endl;
        cout << "Rule IDs found in XML:" << endl;
        set<string> uniqueRuleIds;
        for (const auto& [ruleId, violations] : violationsByRuleId) {
            uniqueRuleIds.insert(ruleId);
        }
        for (const auto& ruleId : uniqueRuleIds) {
            cout << "  " << ruleId << endl;
        }

        // Print out all severity levels we found
        cout << "Severity levels found in violations:" << endl;
        set<int> severityLevels;
        for (const auto& [ruleId, violations] : violationsByRuleId) {
            for (const auto& v : violations) {
                if (v.severity >= 0) {
                    severityLevels.insert(v.severity);
                }
            }
        }
        for (int sev : severityLevels) {
            cout << "  Severity " << sev << endl;
        }
    }

    int totalViolations = moduleViolationCount + stdViolationCount + flowViolationCount;
    cout << "Found " << violationsByRuleId.size() << " rules with a total of "
        << totalViolations << " violations" << endl;

    // Process rules and associate them with violations
    bool rulesFound = false;
    if (codingStandardsElement) {
        XMLElement* rulesElement = codingStandardsElement->FirstChildElement("Rules");
        if (rulesElement) {
            XMLElement* rulesList = rulesElement->FirstChildElement("RulesList");
            if (rulesList) {
                rulesFound = true;
                int ruleCount = 0;
                int rulesWithViolationsCount = 0;

                for (XMLElement* rule = rulesList->FirstChildElement("Rule"); rule; rule = rule->NextSiblingElement("Rule")) {
                    ruleCount++;
                    const char* id = rule->Attribute("id");
                    const char* desc = rule->Attribute("desc");
                    int sev = rule->IntAttribute("sev", -1);
                    const char* cat = rule->Attribute("cat");

                    if (id && desc && sev >= 0) {
                        // Only add rules that have violations
                        if (violationsByRuleId.count(id) > 0 && !violationsByRuleId[id].empty()) {
                            rulesWithViolationsCount++;
                            RuleInfo info;
                            info.id = id;
                            info.desc = desc;
                            info.category = cat ? cat : "";
                            info.violations = violationsByRuleId[id];

                            // Update severity for violations that don't have it set
                            for (auto& v : info.violations) {
                                if (v.severity < 0) {
                                    v.severity = sev;
                                }
                            }

                            if (DEBUG_MODE) {
                                cout << "Adding rule: " << id << " (severity " << sev << ") with "
                                    << info.violations.size() << " violations" << endl;
                            }

                            rulesBySeverity[sev].push_back(info);
                        }
                    }
                }

                cout << "Processed " << ruleCount << " rules, " << rulesWithViolationsCount
                    << " have violations" << endl;
            }
            else {
                cout << "No <RulesList> found in XML." << endl;
            }
        }
        else {
            cout << "No <Rules> found in XML." << endl;
        }
    }

    // Check if we need to use the fallback method
    if (!violationsByRuleId.empty() && rulesBySeverity.empty()) {
        cout << "Found " << violationsByRuleId.size()
            << " rules with violations but couldn't match them to rule definitions." << endl;
        cout << "Using fallback to ensure violations are displayed..." << endl;

        // Process each violation and group by rule ID
        for (const auto& [ruleId, violations] : violationsByRuleId) {
            if (violations.empty()) continue;

            // Default to severity 1 (Low)
            int sev = 1;

            // First, check if any violations have severity information
            bool severityFound = false;
            for (const auto& violation : violations) {
                if (violation.severity >= 0) {
                    sev = violation.severity;
                    severityFound = true;
                    break;
                }
            }

            // If no severity found, try to extract it from the rule ID
            if (!severityFound && ruleId.length() > 3) {
                // Look for patterns like CERT_C-XXX-d-1 where 1 might be severity
                size_t lastDash = ruleId.find_last_of('-');
                if (lastDash != string::npos && lastDash + 1 < ruleId.length()) {
                    char sevChar = ruleId[lastDash + 1];
                    if (isdigit(sevChar)) {
                        sev = sevChar - '0';
                        if (sev > 4) sev = 1; // Cap at highest severity, default to Low
                    }
                }
            }

            RuleInfo info;
            info.id = ruleId;
            info.desc = "Rule: " + ruleId; // Default description
            info.violations = violations;

            // Update severity for all violations
            for (auto& v : info.violations) {
                if (v.severity < 0) {
                    v.severity = sev;
                }
            }

            if (DEBUG_MODE) {
                cout << "  Adding rule " << ruleId << " with " << violations.size()
                    << " violations at severity " << sev << endl;
            }

            rulesBySeverity[sev].push_back(info);
        }
    }

    // Warning if we still have no rules to display
    if (rulesBySeverity.empty()) {
        cout << "WARNING: No violations were grouped by severity. This will result in an empty report." << endl;

        if (!violationsByRuleId.empty()) {
            cout << "Violations exist but weren't matched with rule definitions. Using fallback method..." << endl;

            // Last resort - create a generic rule for each violation
            cout << "Using last resort method to ensure violations are displayed..." << endl;

            // Group all violations into one generic rule
            RuleInfo info;
            info.id = "GENERIC_RULE";
            info.desc = "All violations";

            for (const auto& [ruleId, violations] : violationsByRuleId) {
                for (const auto& violation : violations) {
                    // Add rule ID to the message for clarity
                    Violation v = violation;
                    v.message = "Rule: " + ruleId + " - " + v.message;
                    info.violations.push_back(v);
                }
            }

            if (!info.violations.empty()) {
                cout << "Adding generic rule with " << info.violations.size() << " violations" << endl;
                rulesBySeverity[1].push_back(info); // Use severity 1 (Low)
            }
        }
    }

    // Report counts per severity
    for (const auto& [sev, rules] : rulesBySeverity) {
        int violationCount = 0;
        for (const auto& rule : rules) {
            violationCount += rule.violations.size();
        }
        cout << "Severity " << severityLabel(sev) << " has " << rules.size()
            << " rules with " << violationCount << " violations" << endl;
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
        << "  <title>Parasoft Static Analysis Report</title>\n"
        << "  <meta charset=\"UTF-8\">\n"
        << "  <style>\n"
        << "    body { font-family: Arial, sans-serif; margin: 20px; }\n"
        << "    h1 { color: #2c3e50; }\n"
        << "    .summary { margin-bottom: 20px; background-color: #f8f9fa; padding: 10px; border-left: 4px solid #2c3e50; }\n"
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
        << "    .rule-count { float: right; font-weight: bold; background-color: #e8eaed; padding: 2px 8px; border-radius: 10px; }\n"
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
        << "  <h1>Parasoft Static Analysis Results</h1>\n";

    // Write summary section
    htmlFile << "  <div class='summary'>\n"
        << "    <h2>Summary</h2>\n"
        << "    <p>Total Rules with Violations: " << violationsByRuleId.size() << "</p>\n"
        << "    <p>Total Violations: " << totalViolations << "</p>\n";

    // Add severity breakdown
    htmlFile << "    <h3>Violations by Severity</h3>\n"
        << "    <ul>\n";

    for (const auto& [sev, rules] : rulesBySeverity) {
        int violationCount = 0;
        for (const auto& rule : rules) {
            violationCount += rule.violations.size();
        }
        htmlFile << "      <li><strong>" << severityLabel(sev) << ":</strong> "
            << rules.size() << " rules with " << violationCount << " violations</li>\n";
    }

    htmlFile << "    </ul>\n"
        << "  </div>\n";

    if (rulesBySeverity.empty()) {
        htmlFile << "  <p>No rule violations found in the analysis results.</p>\n";
    }
    else {
        // For each severity level
        for (const auto& [sev, rules] : rulesBySeverity) {
            int violationCount = 0;
            for (const auto& rule : rules) {
                violationCount += rule.violations.size();
            }

            htmlFile << "  <div class='severity severity-" << sev << "'>" << severityLabel(sev)
                << " Severity (" << rules.size() << " rules, " << violationCount
                << " violations) <span class='toggle-btn' onclick='toggleSection(\"sev-" << sev << "\")'>▼</span></div>\n"
                << "  <div id='sev-" << sev << "' class='tree'>\n";

            // For each rule in this severity
            for (const auto& rule : rules) {
                string ruleId = rule.id;
                // Create a safe ID for HTML
                string safeRuleId = ruleId;
                replace(safeRuleId.begin(), safeRuleId.end(), '.', '_');
                replace(safeRuleId.begin(), safeRuleId.end(), ' ', '_');
                replace(safeRuleId.begin(), safeRuleId.end(), '-', '_');
                replace(safeRuleId.begin(), safeRuleId.end(), ':', '_');
                replace(safeRuleId.begin(), safeRuleId.end(), '/', '_');

                htmlFile << "    <div class='rule'>\n"
                    << "      <span class='rule-id'>" << rule.id << "</span>\n"
                    << "      <span class='rule-desc'>" << rule.desc << "</span>\n"
                    << "      <span class='rule-count'>" << rule.violations.size() << "</span>\n"
                    << "      <span class='toggle-btn' onclick='toggleSection(\"rule-" << safeRuleId << "\")'>▼</span>\n"
                    << "      <div id='rule-" << safeRuleId << "' class='violations'>\n";

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

        // Small delay to ensure file is fully written
        Sleep(100);

        // Open the report in the default browser
        ShellExecuteA(NULL, "open", htmlFilePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return 0;
    }
    else {
        cerr << "Error writing to HTML file" << endl;
        return 1;
    }
}