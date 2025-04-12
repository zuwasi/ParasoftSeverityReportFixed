📄 Parasoft Static Analysis Report Parser

Version =1.1.5  Platform =  Windows (GUI file selector, HTML output viewer)   *Input Formats= .xml, .htmlParasoft static analysis reports   Output Format=Richly styled.html` summary report grouped by severity

🔍 Overview

This tool is a Windows desktop utility for parsing and visualizing static analysis results from Parasoft in XML or HTML format.
It categorizes rule violations by severity and generates a readable HTML report with clear formatting for quick inspection.

The tool is ideal for developers, QA engineers, or managers who need a quick summary of static analysis results in a friendly format without manually opening complex XML or HTML files.

🎯 Features

✅ Parses both XML and HTML Parasoft reports
📂 Uses a native Windows file selection dialog to open reports
📊 Groups violations by severity level (Lowest to Highest)
📁 Displays involved project files
📃 Shows line number, rule ID, and violation message
🌐 Generates a modern, styled HTML summary report
🧠 Remembers coding standard rules and messages from the XML
🔄 Lets users repeat analysis for multiple reports in one session
🖼️ Sample Output

The generated report includes:

Total number of violations
Files associated with the project
Categorized sections:
🔵 Lowest
🟢 Low
🟠 Medium
🟧 High
🔴 Highest
Each violation entry includes:

Location: File path
Line number
Rule ID
Violation message
🧱 How It Works

Select Report File
User is prompted with a file selection dialog to choose .xml or .html Parasoft report.

Parse & Extract Data

If XML:
Parses <CodingStandards>, <Rules>, <Locations>, and <StdViols> to extract rule definitions and violations.
If HTML:
Uses string patterns to identify C/C++ source file references and extract violations heuristically.
3. Group & Summarize
Violations are grouped by severity into a map: map<int, vector<Violation>>.

Generate HTML
A styled HTML file is created with sections per severity and summary info.

Open in Browser
The generated HTML report is automatically opened using the default browser.

🛠️ Dependencies

Windows API: For native file picker and ShellExecute
tinyxml2: For XML parsing
Standard C++ libraries: Strings, files, maps, vectors, etc.
🚀 How to Run

✅ Prerequisites

Windows 10 or 11
Built executable (.exe) or compile with g++, cl.exe, etc.
Parasoft reports in .xml or .html format
▶️ Steps

Run the executable
Choose a .xml or .html report file when prompted
Wait for the analysis
The HTML report opens automatically in your browser
Optionally repeat for another report
💡 Future Enhancements

Better severity extraction from raw HTML
Support for other static analysis formats
Export options (PDF, CSV)
Severity threshold filters
Summary charts (pie/bar)
📝 License

MIT License

👨‍💻 Author

Made by: Zuwasi
Email: liezrowice@gmail.com
Contributions welcome!
