
## 🚀 Parasoft Static Analysis XML Report Parser v1.0.2

This is a **Windows desktop tool** that parses **Parasoft C/C++test static analysis XML reports** and generates a clean, categorized **HTML report** grouping violations by severity and rule.

---

### 🧠 Key Features
- 📂 **Interactive File Selection**: Windows file dialog to easily select the `.xml` report.
- 📊 **Supports Multiple XML Formats**:
  - `<Modules>` section (legacy format)
  - `<StdViols>` and `<FlowViol>` sections (standard format in C++test)
- 📌 **Groups violations by rule and severity**.
- 📄 **Outputs a styled, interactive HTML report** with:
  - Summary statistics
  - Severity breakdown (Lowest → Highest)
  - Rule descriptions, file paths, line numbers, and messages

---

### 🎯 Use Case
Designed for developers and QA engineers using **Parasoft C/C++test** to perform static analysis, helping them quickly review and prioritize violations in a readable format.

---

### 🎨 HTML Report Highlights
- Collapsible sections by severity
- Color-coded severity labels:
  - 🔵 Lowest
  - 🟢 Low
  - 🟠 Medium
  - 🟧 High
  - 🔴 Highest
- Per-rule breakdown with violation counts and detailed messages

---

### ⚙️ Tech Stack
- **Language**: C++20  
- **Libraries**:
  - [TinyXML2](https://github.com/leethomason/tinyxml2) for XML parsing
  - Windows API (`<windows.h>`, `<commdlg.h>`, `ShellExecuteA`) for UI integration

---

### 🧪 Debugging Support
- Set `DEBUG_MODE = true` to log XML structure and parsing steps to the console for troubleshooting.

---

### 📂 Output
Generates: `parasoft_report_by_severity.html`  
Automatically opens the report in your default browser upon generation.

---

Let me know if you want a shorter version for the GitHub "tag" section or want this embedded into a README or release `.md` file.
