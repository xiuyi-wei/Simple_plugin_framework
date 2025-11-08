#include <iostream>
#include <vector>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include "runtime/Services.hpp"

#if defined(_WIN32)
  #include <direct.h>   // _mkdir
  #include <sys/stat.h> // _stat
  #include <io.h>       // _access
  #define RT_PATH_SEP '\\'
  #define RT_OTHER_SEP '/'
#else
  #include <sys/stat.h> // ::stat, ::mkdir
  #include <unistd.h>   // ::access, getcwd
  #define RT_PATH_SEP '/'
  #define RT_OTHER_SEP '\\'
#endif

namespace rt {

// =============== StdLogger ===============
void StdLogger::info (const std::string& msg) { std::cout  << "[INFO]  " << msg << std::endl; }
void StdLogger::warn (const std::string& msg) { std::cout  << "[WARN]  " << msg << std::endl; }
void StdLogger::error(const std::string& msg) { std::cerr << "[ERROR] " << msg << std::endl; }

// =============== SteadyClock ===============
std::chrono::steady_clock::time_point SteadyClock::now() const {
    return std::chrono::steady_clock::now();
}

// =============== Helpers ===============
static inline bool isAbsPath(const std::string& p) {
#if defined(_WIN32)
    // C:\...  或  \\server\share...
    if (p.size() >= 2 && std::isalpha(static_cast<unsigned char>(p[0])) && p[1] == ':') return true;
    if (p.size() >= 2 && p[0] == '\\' && p[1] == '\\') return true;
    return false;
#else
    return !p.empty() && p[0] == '/';
#endif
}

static inline void split(const std::string& s, char sep, std::vector<std::string>& out) {
    out.clear();
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            out.push_back(std::string(1, c));
            // 上面逐字符会太细，这里修正为正常的 split：
            //（简单实现：重新写一版）
        }
    }
}

// 一个简单的 split（按单个分隔符）
static inline std::vector<std::string> split1(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// 去掉多余的分隔符，处理 . 和 .. （不依赖 C++17）
static inline std::string normalize_impl(std::string p) {
    if (p.empty()) return p;

    // 统一分隔符为本平台
    for (char& c : p) {
        if (c == RT_OTHER_SEP) c = RT_PATH_SEP;
    }

    // 保留盘符或绝对前缀
#if defined(_WIN32)
    std::string prefix;
    if (p.size() >= 2 && std::isalpha(static_cast<unsigned char>(p[0])) && p[1] == ':') {
        prefix = p.substr(0, 2); // e.g. "C:"
        p = p.substr(2);
    } else if (p.size() >= 2 && p[0] == '\\' && p[1] == '\\') {
        // UNC 前缀，保留开头的两个反斜杠
        prefix = "\\\\";
        p = p.substr(2);
    }
    bool absolutePrefix = (!prefix.empty());
#else
    bool absolutePrefix = (!p.empty() && p[0] == '/');
    if (absolutePrefix) p = p.substr(1);
    std::string prefix = absolutePrefix ? std::string(1, RT_PATH_SEP) : std::string();
#endif

    // 分段处理
    std::vector<std::string> parts = split1(p, RT_PATH_SEP);
    std::vector<std::string> stack;
    for (const auto& seg : parts) {
        if (seg.empty() || seg == ".") continue;
        if (seg == "..") {
            if (!stack.empty()) stack.pop_back();
            else {
                // 相对路径前置 .. 保留
                if (!absolutePrefix) stack.push_back("..");
            }
        } else {
            stack.push_back(seg);
        }
    }

    // 拼回去
    std::string out = prefix;
    if (!stack.empty()) {
        if (!out.empty() && out.back() != RT_PATH_SEP) out.push_back(RT_PATH_SEP);
        for (size_t i = 0; i < stack.size(); ++i) {
            if (i) out.push_back(RT_PATH_SEP);
            out += stack[i];
        }
    } else {
        // 空路径 -> "." 或根
        if (out.empty()) out = ".";
    }
    return out;
}

static inline std::string getCwd() {
    char buf[4096] = {0};
#if defined(_WIN32)
    if (_getcwd(buf, sizeof(buf))) return std::string(buf);
#else
    if (getcwd(buf, sizeof(buf))) return std::string(buf);
#endif
    return std::string(".");
}

static inline bool mkOne(const std::string& dir) {
#if defined(_WIN32)
    if (_mkdir(dir.c_str()) == 0) return true;
    if (errno == EEXIST) return true;
    return false;
#else
    if (::mkdir(dir.c_str(), 0755) == 0) return true;
    if (errno == EEXIST) return true;
    return false;
#endif
}

static inline bool isDirExists(const std::string& p) {
#if defined(_WIN32)
    struct _stat st; 
    if (_stat(p.c_str(), &st) == 0) return (st.st_mode & _S_IFDIR) != 0;
    return false;
#else
    struct stat st;
    if (::stat(p.c_str(), &st) == 0) return S_ISDIR(st.st_mode);
    return false;
#endif
}

// =============== LocalFS ===============
bool LocalFS::exists(const std::string& path) const {
#if defined(_WIN32)
    return (_access(path.c_str(), 0) == 0);
#else
    return (::access(path.c_str(), F_OK) == 0);
#endif
}

std::string LocalFS::join(const std::string& a, const std::string& b) const {
    if (a.empty()) return normalize_impl(b);
    if (b.empty()) return normalize_impl(a);

    std::string res = a;
    char last = res.empty() ? 0 : res.back();
    if (last != RT_PATH_SEP && last != RT_OTHER_SEP) res.push_back(RT_PATH_SEP);
    res += b;
    return normalize_impl(res);
}

std::string LocalFS::dirname(const std::string& p) const {
    if (p.empty()) return ".";
    // 统一分隔符
    std::string s = p;
    for (char& c : s) if (c == RT_OTHER_SEP) c = RT_PATH_SEP;

    // 去掉末尾的分隔符
    while (s.size() > 1 && s.back() == RT_PATH_SEP) s.pop_back();

    size_t pos = s.find_last_of(RT_PATH_SEP);
    if (pos == std::string::npos) return ".";

#if defined(_WIN32)
    // 处理 "C:\"
    if (pos == 2 && std::isalpha(static_cast<unsigned char>(s[0])) && s[1] == ':')
        return s.substr(0, 3); // "C:\"
#endif
    if (pos == 0) return std::string(1, RT_PATH_SEP);
    return s.substr(0, pos);
}

std::string LocalFS::basename(const std::string& p) const {
    if (p.empty()) return "";
    std::string s = p;
    for (char& c : s) if (c == RT_OTHER_SEP) c = RT_PATH_SEP;

    // 去掉末尾的分隔符
    while (s.size() > 1 && s.back() == RT_PATH_SEP) s.pop_back();

    size_t pos = s.find_last_of(RT_PATH_SEP);
    if (pos == std::string::npos) return s;
    if (pos == s.size() - 1) return ""; // 全是分隔符
    return s.substr(pos + 1);
}

std::string LocalFS::normalize(const std::string& p) const {
    return normalize_impl(p);
}

std::string LocalFS::absolute(const std::string& p) const {
    if (p.empty()) return normalize_impl(getCwd());
    if (isAbsPath(p)) return normalize_impl(p);
    return normalize_impl(join(getCwd(), p));
}

bool LocalFS::ensureDir(const std::string& dir) const {
    if (dir.empty()) return false;
    std::string target = normalize_impl(dir);

    if (isDirExists(target)) return true;

    // 逐级创建
    std::vector<std::string> parts = split1(target, RT_PATH_SEP);
    std::string cur;

#if defined(_WIN32)
    // 处理盘符/UNC
    if (parts.size() >= 1 && parts[0].size() == 2 && std::isalpha(static_cast<unsigned char>(parts[0][0])) && parts[0][1] == ':') {
        cur = parts[0] + RT_PATH_SEP;
        parts.erase(parts.begin());
    } else if (target.size() >= 2 && target[0] == '\\' && target[1] == '\\') {
        cur = "\\\\";
    }
#else
    if (!target.empty() && target[0] == RT_PATH_SEP) {
        cur = std::string(1, RT_PATH_SEP);
    }
#endif

    for (size_t i = 0; i < parts.size(); ++i) {
        if (!parts[i].empty()) {
            if (!cur.empty() && cur.back() != RT_PATH_SEP) cur.push_back(RT_PATH_SEP);
            cur += parts[i];
            if (!isDirExists(cur) && !mkOne(cur)) {
                return false;
            }
        }
    }
    return true;
}

bool LocalFS::ensureParentDir(const std::string& path) const {
    return ensureDir(dirname(path));
}

} // namespace rt
