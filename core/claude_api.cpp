#include "claude_api.h"

#include "json.h"

namespace calc {

namespace {

// What Claude is told. The screen limits come from the 2.13" e-paper (25x6 text).
const char* kInstructions =
    "You are the solver inside a pocket calculator. The user photographed a problem "
    "(math, physics, chemistry, biology, engineering or another science). Solve it.\n"
    "The photo comes from a tiny fixed-focus grayscale camera held by hand: expect slightly "
    "soft focus, uneven light, a tilted, sideways or upside-down page, and nearby clutter. "
    "Read it the way a patient teacher would: use the surrounding symbols, the subject and "
    "what makes mathematical sense to decide unclear characters, and solve the problem the "
    "student most likely wrote. Never refuse to answer because of a doubtful character: "
    "always commit to your single best reading, solve it fully, set confidence honestly, "
    "and name the doubt in unclear (the user sees your answer marked as a best guess and can "
    "simply take another photo).\n"
    "Worksheets often show several problems. The student picks one by drawing a box, circle "
    "or similar mark by hand, either around the whole problem or around just its question "
    "number (a boxed or circled \"3\" or \"3.\" means: solve all of question 3, including its "
    "text that is outside the mark). Solve only the marked problem, and start read_as with "
    "its number (e.g. \"Q3: ...\"). Printed boxes, answer boxes and table borders are not "
    "marks. If more than one problem is marked, solve the first in reading order and say so "
    "in unclear. If nothing is marked, solve the problem nearest the centre.\n"
    "Handwritten and printed math is two-dimensional; most misreadings come from position, "
    "not shape. Before solving, transcribe the problem into one line with explicit brackets "
    "and check every symbol against the photo:\n"
    "- Exponents are smaller and raised to the upper right of their base; subscripts are "
    "smaller and lowered (x\xE2\x82\x81, a\xE2\x82\x99, log\xE2\x82\x82). A full-size digit on "
    "the baseline is a coefficient or factor, not a power. Find where each exponent ends: "
    "e^(2x+1) versus e^2x + 1.\n"
    "- Integrals, sums, products and limits: the small number or expression at the top of "
    "\xE2\x88\xAB or \xCE\xA3 is the upper limit, the one at the bottom is the lower limit; the "
    "integrand runs to the dx. Do not read a limit as an exponent or a coefficient. lim "
    "has its approach (x\xE2\x86\x92" "a) written below it.\n"
    "- Fractions: the bar's length sets what is in the numerator and denominator. A radical's "
    "top line sets what is under the root; a small index in its crook makes it a cube or nth "
    "root.\n"
    "- Look-alikes: x and \xC3\x97, 1, l, | and /, 2 and z, 5 and s, 0 and o, 9 and g, t and +, "
    "a minus sign and a fraction bar, dx and d\xC3\x97. Decide from context.\n"
    "Then solve, and verify the result before answering: substitute solutions back, "
    "differentiate an antiderivative, re-check signs and powers.\n"
    "The screen is 25 characters wide and 6 lines tall, monochrome, and scrolls. So:\n"
    "- answer: the final answer with units, at most about 50 characters. If the photo has "
    "several parts (a, b, c...), answer each part on its own line.\n"
    "- expression: if the problem is (or contains) one arithmetic calculation, write it so "
    "it can be typed into a scientific calculator, using only digits . + - * / ^ ( ) , "
    "sqrt( cbrt( sin( cos( tan( asin( acos( atan( log( ln( abs( pi e ! % and E for "
    "\xC3\x97" "10^ (e.g. 3.2E-5). Degrees are the default angle unit. Write exactly what "
    "was photographed, not your simplification. Empty string if there is no single "
    "calculation (e.g. a word problem that needs algebra).\n"
    "- choice: only when the problem lists answer options (A-E, a-d, 1-4, ...): the label of "
    "the correct option exactly as printed, e.g. \"C\". Then answer starts with that label "
    "in parentheses and gives the option's content, e.g. \"(C) 9.90 m/s\". If your result "
    "matches no option, pick the closest, and say so in unclear. Empty string when there are "
    "no options: then answer normally.\n"
    "- check: if the final answer is a single number, a calculation in the same notation as "
    "expression whose value is that number before rounding (e.g. \"9.81*2.5\" or \"(5-2)/(4-1)\"), so "
    "the calculator can re-compute it and confirm your arithmetic. Empty string when the answer "
    "is not one number (several parts, an expression in x, a word).\n"
    "- steps: 2 to 8 short steps, each at most about 75 characters, in plain calculator "
    "notation: x\xC2\xB2, \xE2\x88\x9A(...), \xC3\x97, \xC3\xB7, \xCF\x80, H\xE2\x82\x82O, "
    "3.0\xC3\x97" "10^8. No LaTeX, no markdown. Always give steps whenever readable is true, "
    "even at low confidence or when parts are guessed: at least 2 basic steps that solve your "
    "best reading, so the student can follow the method and redo it if the guess was wrong.\n"
    "- read_as: one short sentence (at most about 80 characters, up to 120 for a long "
    "formula) restating the question with its given values. For math, give your one-line "
    "transcription with its exact structure, e.g. \"\xE2\x88\xAB from 0 to 2 of (3x\xC2\xB2+1) dx\" "
    "or \"solve 2^(x+1) = 16\", so the user can check what you read.\n"
    "- confidence: 0 to 1, how sure you are that you READ the photo correctly (not how "
    "hard the problem is). Softness alone is not a reason for low confidence when every "
    "character can be made out.\n"
    "- unclear: short notes (at most about 40 characters each) naming anything hard to "
    "read or cut off, e.g. \"line 2: 7 or 1?\" or \"cut off at bottom\". Empty if all clear.\n"
    "- readable: false only if the photo shows no problem at all (blank page, not a "
    "problem); a blurry or partly cut-off problem is still readable: guess it. When false, "
    "answer is \"\" and steps is [].\n"
    "Match significant figures to the given data. Check your arithmetic before answering.";

// Property order matters: the answer streams before the steps.
const char* kSchema =
    "{\"type\":\"object\",\"properties\":{"
    "\"readable\":{\"type\":\"boolean\"},"
    "\"confidence\":{\"type\":\"number\"},"
    "\"unclear\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}},"
    "\"expression\":{\"type\":\"string\"},"
    "\"choice\":{\"type\":\"string\"},"
    "\"answer\":{\"type\":\"string\"},"
    "\"check\":{\"type\":\"string\"},"
    "\"read_as\":{\"type\":\"string\"},"
    "\"steps\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},"
    "\"required\":[\"readable\",\"confidence\",\"unclear\",\"expression\",\"choice\",\"answer\",\"check\",\"read_as\",\"steps\"],"
    "\"additionalProperties\":false}";

std::string jsonString(const std::string& s) {
  std::string o = "\"";
  for (unsigned char c : s) {
    switch (c) {
      case '"': o += "\\\""; break;
      case '\\': o += "\\\\"; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default:
        if (c < 0x20) {
          char b[8];
          static const char* hex = "0123456789abcdef";
          b[0] = '\\'; b[1] = 'u'; b[2] = '0'; b[3] = '0';
          b[4] = hex[c >> 4]; b[5] = hex[c & 15]; b[6] = 0;
          o += b;
        } else {
          o += static_cast<char>(c);
        }
    }
  }
  return o + "\"";
}

}  // namespace

std::string base64Encode(const std::string& in) {
  static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((in.size() + 2) / 3 * 4);
  size_t i = 0;
  for (; i + 2 < in.size(); i += 3) {
    unsigned n = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8 | (unsigned char)in[i + 2];
    out += t[n >> 18]; out += t[(n >> 12) & 63]; out += t[(n >> 6) & 63]; out += t[n & 63];
  }
  if (i + 1 == in.size()) {
    unsigned n = (unsigned char)in[i] << 16;
    out += t[n >> 18]; out += t[(n >> 12) & 63]; out += "==";
  } else if (i + 2 == in.size()) {
    unsigned n = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8;
    out += t[n >> 18]; out += t[(n >> 12) & 63]; out += t[(n >> 6) & 63]; out += '=';
  }
  return out;
}

const char* solveInstructions() { return kInstructions; }
const char* solveSchema() { return kSchema; }

std::string buildSolveRequest(const std::string& jpeg, const std::string& detail, const char* effort,
                              bool withSystem) {
  auto image = [](const std::string& bytes) {
    return "{\"type\":\"image\",\"source\":{\"type\":\"base64\",\"media_type\":\"image/jpeg\","
           "\"data\":\"" + base64Encode(bytes) + "\"}},";
  };
  const char* ask = detail.empty()
                        ? "Solve the problem in this photo."
                        : "Image 1 is the whole photo. Image 2 is a zoomed-in, enhanced copy of the "
                          "writing found in it (lighting evened out, contrast raised, sharpened): read "
                          "the characters from image 2 and use image 1 for anything it cuts off. "
                          "Solve the problem.";
  return std::string("{\"model\":\"") + kModel + "\",\"max_tokens\":32000,\"stream\":true,"
         "\"thinking\":{\"type\":\"adaptive\"},"
         + (withSystem ? "\"system\":" + jsonString(kInstructions) + "," : std::string()) +
         "\"output_config\":{\"effort\":\"" + effort + "\",\"format\":{\"type\":\"json_schema\",\"schema\":" + kSchema + "}},"
         "\"messages\":[{\"role\":\"user\",\"content\":[" +
         image(jpeg) + (detail.empty() ? std::string() : image(detail)) +
         "{\"type\":\"text\",\"text\":" + jsonString(ask) + "}]}]}";
}

void StreamReader::feed(const std::string& chunk) {
  for (char c : chunk)
    if (c != '\r') buf_ += c;  // accept \r\n line endings too
  // Events are separated by a blank line; only "data:" lines matter.
  size_t end;
  while ((end = buf_.find("\n\n")) != std::string::npos) {
    std::string event = buf_.substr(0, end);
    buf_.erase(0, end + 2);
    size_t pos = 0;
    while (pos < event.size()) {
      size_t nl = event.find('\n', pos);
      std::string line = event.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
      if (line.compare(0, 5, "data:") == 0) handleEvent(line.substr(line[5] == ' ' ? 6 : 5));
      if (nl == std::string::npos) break;
      pos = nl + 1;
    }
  }
}

void StreamReader::handleEvent(const std::string& data) {
  Json j;
  std::string err;
  if (!Json::parse(data, j, err)) return;
  const std::string type = j["type"].isString() ? j["type"].asString() : "";
  if (type == "content_block_delta" && j["delta"]["type"].isString() &&
      j["delta"]["type"].asString() == "text_delta") {
    text_ += j["delta"]["text"].asString();
  } else if (type == "message_delta" && j["delta"]["stop_reason"].isString()) {
    stopReason_ = j["delta"]["stop_reason"].asString();
  } else if (type == "message_stop") {
    finished_ = true;
  } else if (type == "error") {
    errorType_ = j["error"]["type"].isString() ? j["error"]["type"].asString() : "error";
    errorMessage_ = j["error"]["message"].isString() ? j["error"]["message"].asString() : "";
  }
}

bool peekAnswer(const std::string& s, double& confidence, std::string& answer) {
  auto valueAfter = [&](const char* key) -> size_t {  // index just past "key":
    size_t k = s.find(key);
    if (k == std::string::npos) return std::string::npos;
    k += std::char_traits<char>::length(key);
    while (k < s.size() && (s[k] == ' ' || s[k] == '\n')) ++k;
    if (k >= s.size() || s[k] != ':') return std::string::npos;
    ++k;
    while (k < s.size() && (s[k] == ' ' || s[k] == '\n')) ++k;
    return k;
  };
  size_t c = valueAfter("\"confidence\"");
  size_t a = valueAfter("\"answer\"");
  if (c == std::string::npos || a == std::string::npos || a >= s.size() || s[a] != '"') return false;
  size_t e = a + 1;  // find the closing quote, skipping escapes
  while (e < s.size() && s[e] != '"') e += (s[e] == '\\') ? 2 : 1;
  if (e >= s.size()) return false;
  Json str, num;
  std::string err;
  size_t ce = c;
  while (ce < s.size() && s[ce] != ',' && s[ce] != '}') ++ce;
  if (!Json::parse(s.substr(c, ce - c), num, err) || !num.isNumber()) return false;
  if (!Json::parse(s.substr(a, e - a + 1), str, err)) return false;
  confidence = num.asNumber();
  answer = str.asString();
  return !answer.empty();
}

bool classifyFailure(int status, const std::string& body, const StreamReader& stream,
                     Failure& f, std::string& detail) {
  detail.clear();
  if (status != 200) {
    // The calculator's proxy (server/proxy) answers account problems with its own error
    // types and a short message meant for the screen (it may carry the pairing code).
    // "rate_limited" is the proxy's burst limit ("wait a minute"); older proxies sent it as
    // "fair_use_exceeded", which still works. Claude's own "rate_limit_error" stays "busy".
    Json j;
    std::string err;
    if (Json::parse(body, j, err) && j["error"]["type"].isString() && j["error"]["message"].isString()) {
      const std::string type = j["error"]["type"].asString();
      if (type == "device_unknown" || type == "device_not_linked" || type == "subscription_inactive" ||
          type == "fair_use_exceeded" || type == "free_tier_exhausted" || type == "rate_limited") {
        f = Failure::Account;
        detail = j["error"]["message"].asString().substr(0, 110);
        return true;
      }
    }
  }
  if (status == 401 || status == 403) {
    f = Failure::ApiError;
    detail = "API key was rejected.";
    return true;
  }
  if (status == 429 || status == 500 || status == 502 || status == 503 || status == 529) {
    f = Failure::ApiBusy;
    return true;
  }
  if (status != 200) {
    f = Failure::ApiError;
    Json j;
    std::string err;
    if (Json::parse(body, j, err) && j["error"]["message"].isString())
      detail = j["error"]["message"].asString().substr(0, 70);
    else
      detail = "HTTP error " + std::to_string(status) + ".";
    return true;
  }
  if (!stream.errorType().empty()) {  // error event mid-stream
    f = stream.errorType() == "overloaded_error" ? Failure::ApiBusy : Failure::ApiError;
    detail = stream.errorMessage().substr(0, 70);
    return true;
  }
  if (stream.stopReason() == "refusal") {
    f = Failure::ApiError;
    detail = "Claude declined to answer this one.";
    return true;
  }
  if (stream.stopReason() == "max_tokens") {
    f = Failure::ApiError;
    detail = "Answer too long. Photograph one part.";
    return true;
  }
  if (!stream.finished()) {
    f = Failure::NoConnection;
    detail = "Connection dropped mid-reply.";
    return true;
  }
  return false;
}

}  // namespace calc
