#include "soundgrind/engine.hpp"
#include <fstream>
#include <sstream>

namespace soundgrind {
namespace {
std::string escape_html(const std::string& s) { std::string o;for(char c:s){switch(c){case '&':o+="&amp;";break;case '<':o+="&lt;";break;case '>':o+="&gt;";break;case '"':o+="&quot;";break;case '\'':o+="&#39;";break;default:o+=c;}}return o; }
}
void write_html_report(const std::filesystem::path& input,const std::filesystem::path& output) {
  std::ifstream in(input);if(!in)throw std::runtime_error("cannot open campaign JSON: "+input.string());Json j=Json::parse(in);
  std::uint64_t pass=0,fail=0,unsupported=0;for(const auto& c:j.value("cases",Json::array()))for(const auto& m:c.value("module_results",Json::array())){auto v=m.value("verdict","");if(v=="PASS")++pass;else if(v=="FAIL")++fail;else if(v=="UNSUPPORTED")++unsupported;}
  const auto raw=j.dump(2);if(!output.parent_path().empty())std::filesystem::create_directories(output.parent_path());std::ofstream out(output,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("cannot write HTML report");
  out<<"<!doctype html><html lang=\"en\"><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>SoundGrind report</title>"
    <<"<style>body{font:15px/1.5 system-ui,sans-serif;max-width:1100px;margin:2rem auto;padding:0 1rem;color:#18202a}h1{margin-bottom:.2rem}.cards{display:flex;gap:1rem;flex-wrap:wrap}.card{background:#f0f3f6;padding:1rem 1.4rem;border-radius:.5rem}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#101820;color:#e9f1f6;padding:1rem;border-radius:.5rem;max-height:70vh;overflow:auto}</style>"
    <<"<body><h1>SoundGrind campaign report</h1><p>Results are observations for this target and configuration; passing checks do not prove complete correctness.</p><div class=\"cards\"><div class=\"card\"><b>Planned cases</b><br>"<<j.value("planned_cases",0ull)<<"</div><div class=\"card\"><b>Completed cases</b><br>"<<j.value("completed_cases",0ull)<<"</div><div class=\"card\"><b>Module passes</b><br>"<<pass<<"</div><div class=\"card\"><b>Module failures</b><br>"<<fail<<"</div><div class=\"card\"><b>Unsupported checks</b><br>"<<unsupported<<"</div></div><h2>Campaign data</h2><pre>"<<escape_html(raw)<<"</pre></body></html>";
  out.flush();if(!out)throw std::runtime_error("failed writing HTML report");
}
}
