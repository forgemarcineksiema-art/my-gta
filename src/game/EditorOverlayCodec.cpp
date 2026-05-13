#include "EditorOverlayCodec.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace bs3d {

namespace {

enum class JsonType {
    Null,
    Number,
    String,
    Array,
    Object
};

struct JsonValue {
    JsonType type = JsonType::Null;
    double number = 0.0;
    std::string text;
    std::vector<JsonValue> array;
    std::unordered_map<std::string, JsonValue> object;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& source) : source_(source) {}

    JsonValue parse() {
        skipWhitespace();
        JsonValue value = parseValue();
        skipWhitespace();
        if (cursor_ < source_.size()) {
            trailingInput_ = true;
        }
        return value;
    }

    bool trailingInput() const { return trailingInput_; }

private:
    const std::string& source_;
    std::size_t cursor_ = 0;
    bool trailingInput_ = false;

    void skipWhitespace() {
        while (cursor_ < source_.size() &&
               std::isspace(static_cast<unsigned char>(source_[cursor_]))) {
            ++cursor_;
        }
    }

    bool consume(char expected) {
        skipWhitespace();
        if (cursor_ >= source_.size() || source_[cursor_] != expected) {
            return false;
        }
        ++cursor_;
        return true;
    }

    JsonValue parseValue() {
        skipWhitespace();
        if (cursor_ >= source_.size()) {
            return {};
        }
        const char ch = source_[cursor_];
        if (ch == '"') {
            JsonValue value;
            value.type = JsonType::String;
            value.text = parseString();
            return value;
        }
        if (ch == '[') {
            return parseArray();
        }
        if (ch == '{') {
            return parseObject();
        }
        return parseNumberOrLiteral();
    }

    std::string parseString() {
        std::string result;
        if (!consume('"')) {
            return result;
        }
        while (cursor_ < source_.size()) {
            const char ch = source_[cursor_++];
            if (ch == '"') {
                break;
            }
            if (ch == '\\' && cursor_ < source_.size()) {
                const char escaped = source_[cursor_++];
                switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped);
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                default:
                    result.push_back(escaped);
                    break;
                }
            } else {
                result.push_back(ch);
            }
        }
        return result;
    }

    JsonValue parseArray() {
        JsonValue value;
        value.type = JsonType::Array;
        consume('[');
        skipWhitespace();
        if (consume(']')) {
            return value;
        }
        while (cursor_ < source_.size()) {
            value.array.push_back(parseValue());
            skipWhitespace();
            if (consume(']')) {
                break;
            }
            if (!consume(',')) {
                break;
            }
        }
        return value;
    }

    JsonValue parseObject() {
        JsonValue value;
        value.type = JsonType::Object;
        consume('{');
        skipWhitespace();
        if (consume('}')) {
            return value;
        }
        while (cursor_ < source_.size()) {
            skipWhitespace();
            if (cursor_ >= source_.size() || source_[cursor_] != '"') {
                break;
            }
            const std::string key = parseString();
            if (!consume(':')) {
                break;
            }
            value.object[key] = parseValue();
            skipWhitespace();
            if (consume('}')) {
                break;
            }
            if (!consume(',')) {
                break;
            }
        }
        return value;
    }

    JsonValue parseNumberOrLiteral() {
        const std::size_t start = cursor_;
        while (cursor_ < source_.size() &&
               (std::isalnum(static_cast<unsigned char>(source_[cursor_])) ||
                source_[cursor_] == '-' ||
                source_[cursor_] == '+' ||
                source_[cursor_] == '.')) {
            ++cursor_;
        }
        JsonValue value;
        const std::string token = source_.substr(start, cursor_ - start);
        char* end = nullptr;
        const double parsed = std::strtod(token.c_str(), &end);
        if (end != nullptr && *end == '\0') {
            value.type = JsonType::Number;
            value.number = parsed;
        }
        return value;
    }
};

const JsonValue* objectMember(const JsonValue& value, const std::string& key) {
    if (value.type != JsonType::Object) {
        return nullptr;
    }
    const auto found = value.object.find(key);
    return found == value.object.end() ? nullptr : &found->second;
}

int intMember(const JsonValue& value, const std::string& key, int fallback = 0) {
    const JsonValue* member = objectMember(value, key);
    return member != nullptr && member->type == JsonType::Number ? static_cast<int>(member->number) : fallback;
}

float numberMember(const JsonValue& value, const std::string& key, float fallback = 0.0f) {
    const JsonValue* member = objectMember(value, key);
    return member != nullptr && member->type == JsonType::Number ? static_cast<float>(member->number) : fallback;
}

std::string stringMember(const JsonValue& value, const std::string& key) {
    const JsonValue* member = objectMember(value, key);
    return member != nullptr && member->type == JsonType::String ? member->text : std::string{};
}

Vec3 vec3Member(const JsonValue& value, const std::string& key, Vec3 fallback = {}) {
    const JsonValue* member = objectMember(value, key);
    if (member == nullptr || member->type != JsonType::Array || member->array.size() < 3) {
        return fallback;
    }
    if (member->array[0].type != JsonType::Number ||
        member->array[1].type != JsonType::Number ||
        member->array[2].type != JsonType::Number) {
        return fallback;
    }
    return {static_cast<float>(member->array[0].number),
            static_cast<float>(member->array[1].number),
            static_cast<float>(member->array[2].number)};
}

std::vector<std::string> stringArrayMember(const JsonValue& value, const std::string& key) {
    std::vector<std::string> result;
    const JsonValue* member = objectMember(value, key);
    if (member == nullptr || member->type != JsonType::Array) {
        return result;
    }
    for (const JsonValue& item : member->array) {
        if (item.type == JsonType::String) {
            result.push_back(item.text);
        }
    }
    return result;
}

EditorOverlayObject parseOverlayObject(const JsonValue& value, std::vector<std::string>& warnings) {
    EditorOverlayObject object;
    if (value.type != JsonType::Object) {
        warnings.push_back("editor overlay object entry must be an object");
        return object;
    }
    object.id = stringMember(value, "id");
    object.assetId = stringMember(value, "assetId");
    object.position = vec3Member(value, "position");
    object.scale = vec3Member(value, "scale", {1.0f, 1.0f, 1.0f});
    object.yawRadians = numberMember(value, "yawRadians", 0.0f);
    object.zoneTag = worldLocationTagFromName(stringMember(value, "zoneTag"));
    object.gameplayTags = stringArrayMember(value, "gameplayTags");
    if (object.id.empty()) {
        warnings.push_back("editor overlay object id must be non-empty");
    }
    if (object.assetId.empty()) {
        warnings.push_back("editor overlay object assetId must be non-empty: " + object.id);
    }
    return object;
}

std::vector<EditorOverlayObject> parseObjectArray(const JsonValue& root,
                                                  const std::string& key,
                                                  std::vector<std::string>& warnings) {
    std::vector<EditorOverlayObject> result;
    const JsonValue* member = objectMember(root, key);
    if (member == nullptr) {
        return result;
    }
    if (member->type != JsonType::Array) {
        warnings.push_back("editor overlay " + key + " must be an array");
        return result;
    }
    for (const JsonValue& item : member->array) {
        result.push_back(parseOverlayObject(item, warnings));
    }
    return result;
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::string escapeJson(const std::string& value) {
    std::string escaped;
    for (const char ch : value) {
        switch (ch) {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

void writeVec3(std::ostringstream& out, Vec3 value) {
    out << "[" << value.x << ", " << value.y << ", " << value.z << "]";
}

void writeStringArray(std::ostringstream& out, const std::vector<std::string>& values) {
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << "\"" << escapeJson(values[i]) << "\"";
    }
    out << "]";
}

void writeOverlayObject(std::ostringstream& out, const EditorOverlayObject& object, const std::string& indent) {
    out << indent << "{\n";
    out << indent << "  \"id\": \"" << escapeJson(object.id) << "\",\n";
    out << indent << "  \"assetId\": \"" << escapeJson(object.assetId) << "\",\n";
    out << indent << "  \"position\": ";
    writeVec3(out, object.position);
    out << ",\n";
    out << indent << "  \"yawRadians\": " << object.yawRadians << ",\n";
    out << indent << "  \"scale\": ";
    writeVec3(out, object.scale);
    out << ",\n";
    out << indent << "  \"zoneTag\": \"" << worldLocationTagName(object.zoneTag) << "\",\n";
    out << indent << "  \"gameplayTags\": ";
    writeStringArray(out, object.gameplayTags);
    out << "\n";
    out << indent << "}";
}

void writeObjectArray(std::ostringstream& out,
                      const std::vector<EditorOverlayObject>& objects,
                      const std::string& indent) {
    out << "[";
    if (!objects.empty()) {
        out << "\n";
    }
    for (std::size_t i = 0; i < objects.size(); ++i) {
        writeOverlayObject(out, objects[i], indent + "  ");
        if (i + 1 < objects.size()) {
            out << ",";
        }
        out << "\n";
    }
    if (!objects.empty()) {
        out << indent;
    }
    out << "]";
}

} // namespace

const char* worldLocationTagName(WorldLocationTag tag) {
    switch (tag) {
    case WorldLocationTag::Block:
        return "Block";
    case WorldLocationTag::Shop:
        return "Shop";
    case WorldLocationTag::Parking:
        return "Parking";
    case WorldLocationTag::Garage:
        return "Garage";
    case WorldLocationTag::Trash:
        return "Trash";
    case WorldLocationTag::RoadLoop:
        return "RoadLoop";
    case WorldLocationTag::Unknown:
        return "Unknown";
    }
    return "Unknown";
}

WorldLocationTag worldLocationTagFromName(const std::string& name) {
    if (name == "Block") {
        return WorldLocationTag::Block;
    }
    if (name == "Shop") {
        return WorldLocationTag::Shop;
    }
    if (name == "Parking") {
        return WorldLocationTag::Parking;
    }
    if (name == "Garage") {
        return WorldLocationTag::Garage;
    }
    if (name == "Trash") {
        return WorldLocationTag::Trash;
    }
    if (name == "RoadLoop") {
        return WorldLocationTag::RoadLoop;
    }
    return WorldLocationTag::Unknown;
}

EditorOverlayLoadResult parseEditorOverlay(const std::string& text) {
    EditorOverlayLoadResult result;
    if (text.empty()) {
        result.success = true;
        return result;
    }
    JsonParser parser(text);
    const JsonValue root = parser.parse();
    if (parser.trailingInput()) {
        result.warnings.push_back("editor overlay JSON has trailing content after root value");
        return result;
    }
    if (root.type != JsonType::Object) {
        result.warnings.push_back("editor overlay root must be an object");
        return result;
    }
    result.document.schemaVersion = intMember(root, "schemaVersion", 0);
    if (result.document.schemaVersion != 1) {
        result.warnings.push_back("unsupported editor overlay schemaVersion");
        return result;
    }
    result.document.overrides = parseObjectArray(root, "overrides", result.warnings);
    result.document.instances = parseObjectArray(root, "instances", result.warnings);
    result.success = result.warnings.empty();
    return result;
}

EditorOverlayLoadResult loadEditorOverlayFile(const std::string& path) {
    EditorOverlayLoadResult result;
    const std::string text = readTextFile(path);
    if (text.empty() && !std::filesystem::exists(path)) {
        result.warnings.push_back("editor overlay file not found: " + path);
        return result;
    }
    return parseEditorOverlay(text);
}

std::string serializeEditorOverlay(const EditorOverlayDocument& document) {
    std::ostringstream out;
    out << std::setprecision(6);
    out << "{\n";
    out << "  \"schemaVersion\": " << document.schemaVersion << ",\n";
    out << "  \"overrides\": ";
    writeObjectArray(out, document.overrides, "  ");
    out << ",\n";
    out << "  \"instances\": ";
    writeObjectArray(out, document.instances, "  ");
    out << "\n";
    out << "}\n";
    return out.str();
}

bool saveEditorOverlayFile(const std::string& path,
                           const EditorOverlayDocument& document,
                           std::vector<std::string>& warnings) {
    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        warnings.push_back("could not write editor overlay file: " + path);
        return false;
    }
    out << serializeEditorOverlay(document);
    return true;
}

} // namespace bs3d
