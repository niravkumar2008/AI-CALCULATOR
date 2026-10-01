#include "solve_result.h"

#include "json.h"

namespace calc {

bool parseSolveResult(const std::string& text, SolveResult& out, std::string& err) {
  out = SolveResult();
  Json j;
  if (!Json::parse(text, j, err)) return false;
  if (!j.isObject()) {
    err = "reply is not a JSON object";
    return false;
  }
  auto str = [](const Json& v) { return v.isString() ? v.asString() : std::string(); };
  auto list = [](const Json& v) {
    std::vector<std::string> r;
    for (const Json& i : v.items())
      if (i.isString() && !i.asString().empty()) r.push_back(i.asString());
    return r;
  };
  if (j["readable"].isBool()) out.readable = j["readable"].asBool();
  if (j["confidence"].isNumber()) {
    double c = j["confidence"].asNumber();
    out.confidence = c < 0 ? 0 : (c > 1 ? 1 : c);
  }
  out.readAs = str(j["read_as"]);
  out.unclear = list(j["unclear"]);
  out.expression = str(j["expression"]);
  out.choice = str(j["choice"]);
  if (out.choice.size() > 4) out.choice.clear();  // a label, not a sentence
  out.answer = str(j["answer"]);
  out.steps = list(j["steps"]);
  if (out.readable && out.answer.empty()) {
    err = "reply has no answer";
    return false;
  }
  return true;
}

}  // namespace calc
