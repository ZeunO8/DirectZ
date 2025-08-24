#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <dz/function.hpp>
#include <algorithm>
#include <cctype>
#include <imgui_internal.h>

namespace dz::editor
{
    struct Color
    {
        ImU32 abgr;
        static inline ImU32 Make(float r, float g, float b, float a)
        {
            return IM_COL32((int)(r * 255.f), (int)(g * 255.f), (int)(b * 255.f), (int)(a * 255.f));
        }
    };

    struct TextSpan
    {
        int start;
        int end;
        ImU32 color;
    };

    struct LineRender
    {
        std::vector<TextSpan> spans;
    };

    struct ISyntaxHighlighter
    {
        virtual ~ISyntaxHighlighter() {}
        virtual void ColorizeLine(const std::string& line, LineRender& out) = 0;
        virtual const char* Name() const = 0;
    };

    struct CppSyntaxHighlighter final : ISyntaxHighlighter
    {
        std::unordered_set<std::string> keywords;
        ImU32 colKeyword;
        ImU32 colType;
        ImU32 colLiteral;
        ImU32 colNumber;
        ImU32 colComment;
        ImU32 colPreproc;
        ImU32 colIdentifier;

        CppSyntaxHighlighter()
        {
            const char* list[] =
            {
                "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
            };

            for (auto* k : list) keywords.insert(k);
            colKeyword = Color::Make(0.82f, 0.56f, 0.16f, 1.f);
            colType = Color::Make(0.54f, 0.72f, 1.f, 1.f);
            colLiteral = Color::Make(0.47f, 0.84f, 0.47f, 1.f);
            colNumber = Color::Make(0.98f, 0.60f, 0.60f, 1.f);
            colComment = Color::Make(0.50f, 0.60f, 0.50f, 1.f);
            colPreproc = Color::Make(0.60f, 0.76f, 0.96f, 1.f);
            colIdentifier = Color::Make(0.86f, 0.86f, 0.86f, 1.f);
        }

        static inline bool IsIdentStart(char c)
        {
            return std::isalpha((unsigned char)c) || c == '_';
        }

        static inline bool IsIdent(char c)
        {
            return std::isalnum((unsigned char)c) || c == '_';
        }

        void PushSpan(LineRender& out, int s, int e, ImU32 c)
        {
            if (e <= s) return;
            out.spans.push_back({ s, e, c });
        }

        void ColorizeLine(const std::string& line, LineRender& out) override
        {
            out.spans.clear();
            const int n = (int)line.size();
            if (n == 0)
            {
                return;
            }

            if (line[0] == '#')
            {
                PushSpan(out, 0, n, colPreproc);
                return;
            }

            int i = 0;
            while (i < n)
            {
                char c = line[i];
                if (c == '/' && i + 1 < n && line[i + 1] == '/')
                {
                    PushSpan(out, i, n, colComment);
                    break;
                }

                if (c == '\'' || c == '"')
                {
                    const char quote = c;
                    int s = i;
                    ++i;
                    while (i < n)
                    {
                        if (line[i] == '\\' && i + 1 < n)
                        {
                            i += 2;
                            continue;
                        }
                        if (line[i] == quote)
                        {
                            ++i;
                            break;
                        }
                        ++i;
                    }
                    PushSpan(out, s, i, colLiteral);
                    continue;
                }

                if (std::isdigit((unsigned char)c) || (c == '.' && i + 1 < n && std::isdigit((unsigned char)line[i + 1])))
                {
                    int s = i;
                    bool seen_e = false, seen_x = false;
                    while (i < n)
                    {
                        char d = line[i];
                        if (std::isalnum((unsigned char)d) || d == '.' || d == '_' || d == 'x' || d == 'X' || d == 'e' || d == 'E' || d == '+' || d == '-')
                        {
                            if (d == 'e' || d == 'E') seen_e = true;
                            if (d == 'x' || d == 'X') seen_x = true;
                            ++i;
                        }
                        else break;
                    }
                    PushSpan(out, s, i, colNumber);
                    continue;
                }

                if (IsIdentStart(c))
                {
                    int s = i;
                    ++i;
                    while (i < n && IsIdent(line[i])) ++i;
                    std::string ident = line.substr(s, i - s);
                    if (keywords.count(ident))
                    {
                        PushSpan(out, s, i, colKeyword);
                    }
                    // TODO: reduce this to an ir lookup expression
                    else if (ident == "size_t" || ident == "int32_t" || ident == "int64_t" || ident == "uint32_t" || ident == "uint64_t" || ident == "float" || ident == "double" || ident == "bool" || ident == "char" || ident == "wchar_t" || ident == "std" || ident == "string" || ident == "u8string" || ident == "u16string" || ident == "u32string")
                    {
                        PushSpan(out, s, i, colType);
                    }
                    else
                    {
                        PushSpan(out, s, i, colIdentifier);
                    }
                    continue;
                }
                ++i;
            }
        }

        const char* Name() const override
        {
            return "C++";
        }
    };

    struct EditorConfig
    {
        float lineSpacing;
        float gutterPadding;
        int tabSize;
        ImU32 colBg;
        ImU32 colCaret;
        ImU32 colGutterBg;
        ImU32 colGutterText;
        ImU32 colCurrentLineBg;
        ImU32 colWhitespace;
        ImU32 colSelectionBg;

        EditorConfig()
        {
            lineSpacing = 1.0f;
            gutterPadding = 8.0f;
            tabSize = 4;
            colBg = IM_COL32(20, 22, 28, 255);
            colCaret = IM_COL32(240, 240, 240, 255);
            colGutterBg = IM_COL32(28, 30, 38, 255);
            colGutterText = IM_COL32(150, 150, 160, 255);
            colCurrentLineBg = IM_COL32(36, 38, 48, 255);
            colWhitespace = IM_COL32(80, 80, 90, 120);
            colSelectionBg = IM_COL32(50, 100, 200, 100);
        }
    };

    struct CodeBuffer
    {
        std::vector<std::string> lines;

        CodeBuffer()
        {
            lines.clear();
            lines.emplace_back("");
        }

        void Set(const std::string& text)
        {
            lines.clear();
            size_t b = 0;
            for (size_t i = 0; i <= text.size(); ++i)
            {
                if (i == text.size() || text[i] == '\n')
                {
                    lines.emplace_back(text.substr(b, i - b));
                    b = i + 1;
                }
            }
            if (lines.empty()) lines.emplace_back("");
        }

        std::string GetAll() const
        {
            std::string r;
            for (size_t i = 0; i < lines.size(); ++i)
            {
                r += lines[i];
                if (i + 1 < lines.size()) r += '\n';
            }
            return r;
        }
    };

    struct Cursor
    {
        int line;
        int column;
    };

    struct Selection
    {
        bool active;
        Cursor a;
        Cursor b;
    };

    struct CodeEditor
    {
        EditorConfig cfg;
        CodeBuffer buf;
        ISyntaxHighlighter* highlighter;
        std::vector<LineRender> colored;
        Cursor caret;
        Selection sel;
        bool focusRequested;
        float preferredX;
        bool hovered = false;

        CodeEditor()
        {
            highlighter = nullptr;
            caret = {0, 0};
            sel = {false, {0, 0}, {0, 0}};
            colored.clear();
            focusRequested = false;
            preferredX = -1.f;
        }

        void SetHighlighter(ISyntaxHighlighter* h)
        {
            highlighter = h;
            RecolorAll();
        }

        void SetText(const std::string& text)
        {
            buf.Set(text);
            caret = {0, 0};
            RecolorAll();
        }

        const std::string GetText() const
        {
            return buf.GetAll();
        }

        void EnsureValidCaret()
        {
            if (caret.line < 0) caret.line = 0;
            if (caret.line >= (int)buf.lines.size()) caret.line = (int)buf.lines.size() - 1;
            if (caret.column < 0) caret.column = 0;
            if (caret.column > (int)buf.lines[caret.line].size()) caret.column = (int)buf.lines[caret.line].size();
        }

        void InsertChar(char c)
        {
            if (c == '\t')
            {
                int spaces = cfg.tabSize - (caret.column % cfg.tabSize);
                for (int i = 0; i < spaces; ++i)
                    InsertChar(' ');
                return;
            }
            std::string& line = buf.lines[caret.line];
            line.insert(line.begin() + caret.column, c);
            ++caret.column;
            RecolorLine(caret.line);
        }

        void NewLine()
        {
            std::string& line = buf.lines[caret.line];
            std::string right = line.substr(caret.column);
            line.erase(line.begin() + caret.column, line.end());
            buf.lines.insert(buf.lines.begin() + caret.line + 1, right);
            ++caret.line;
            caret.column = 0;
            RecolorLine(caret.line - 1);
            RecolorLine(caret.line);
            colored.push_back({});
            RecolorLine(caret.line + 1);
        }

        void Backspace()
        {
            if (caret.column > 0)
            {
                std::string& line = buf.lines[caret.line];
                line.erase(line.begin() + caret.column - 1);
                --caret.column;
                RecolorLine(caret.line);
            }
            else if (caret.line > 0)
            {
                int prevLen = (int)buf.lines[caret.line - 1].size();
                buf.lines[caret.line - 1] += buf.lines[caret.line];
                buf.lines.erase(buf.lines.begin() + caret.line);
                colored.erase(colored.begin() + caret.line);
                --caret.line;
                caret.column = prevLen;
                RecolorLine(caret.line);
            }
        }

        void DeleteForward()
        {
            std::string& line = buf.lines[caret.line];
            if (caret.column < (int)line.size())
            {
                line.erase(line.begin() + caret.column);
                RecolorLine(caret.line);
            }
            else if (caret.line + 1 < (int)buf.lines.size())
            {
                line += buf.lines[caret.line + 1];
                buf.lines.erase(buf.lines.begin() + caret.line + 1);
                RecolorLine(caret.line);
            }
        }

        void MoveLeft(bool word)
        {
            if (!word)
            {
                if (caret.column > 0) --caret.column;
                else if (caret.line > 0)
                {
                    --caret.line;
                    caret.column = (int)buf.lines[caret.line].size();
                }
                return;
            }

            if (caret.column == 0)
            {
                if (caret.line > 0)
                {
                    --caret.line;
                    caret.column = (int)buf.lines[caret.line].size();
                }
                return;
            }

            const std::string& s = buf.lines[caret.line];
            int i = caret.column - 1;
            while (i > 0 && std::isspace((unsigned char)s[i])) --i;
            while (i > 0 && !std::isspace((unsigned char)s[i - 1])) --i;
            caret.column = i;
        }

        void MoveRight(bool word)
        {
            const std::string& s = buf.lines[caret.line];
            if (!word)
            {
                if (caret.column < (int)s.size()) ++caret.column;
                else if (caret.line + 1 < (int)buf.lines.size())
                {
                    ++caret.line;
                    caret.column = 0;
                }
                return;
            }

            int i = caret.column;
            while (i < (int)s.size() && !std::isspace((unsigned char)s[i])) ++i;
            while (i < (int)s.size() && std::isspace((unsigned char)s[i])) ++i;
            caret.column = i;
        }

        void MoveUp()
        {
            if (caret.line > 0)
            {
                float x = preferredX < 0.f ? -1.f : preferredX;
                --caret.line;
                caret.column = ClosestColumnForX(buf.lines[caret.line], x);
            }
        }

        void MoveDown()
        {
            if (caret.line + 1 < (int)buf.lines.size())
            {
                float x = preferredX < 0.f ? -1.f : preferredX;
                ++caret.line;
                caret.column = ClosestColumnForX(buf.lines[caret.line], x);
            }
        }

        int ClosestColumnForX(const std::string& s, float x)
        {
            if (x < 0.f) return std::min<int>(caret.column, (int)s.size());
            auto& g = *ImGui::GetCurrentContext();
            ImFont* font = g.Font;
            float bestDiff = 1e9f;
            int best = 0;
            float acc = 0.f;

            for (int i = 0; i <= (int)s.size(); ++i)
            {
                float diff = fabsf(acc - x);
                if (diff < bestDiff)
                {
                    bestDiff = diff;
                    best = i;
                }

                if (i < (int)s.size())
                {
                    char c = s[i];
                    if (c == '\t')
                    {
                        int spaces = cfg.tabSize - (i % cfg.tabSize);
                        acc += font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, std::string(spaces, ' ').c_str()).x;
                    }
                    else
                    {
                        acc += font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, std::string(1, c).c_str()).x;
                    }
                }
            }
            return best;
        }
        void CopySelectionToClipboard()
        {
            if (!sel.active) return;
            Cursor a = sel.a, b = sel.b;
            if (CompareCursor(b, a) < 0) std::swap(a, b);
            std::string out;
            if (a.line == b.line)
            {
                out = buf.lines[a.line].substr(a.column, b.column - a.column);
            }
            else
            {
                out.reserve(256);
                out += buf.lines[a.line].substr(a.column);
                out += '\n';
                for (int l = a.line + 1; l < b.line; ++l)
                {
                    out += buf.lines[l];
                    out += '\n';
                }
                out += buf.lines[b.line].substr(0, b.column);
            }
            ImGui::SetClipboardText(out.c_str());
        }

        void DeleteSelection()
        {
            if (!sel.active) return;
            Cursor a = sel.a, b = sel.b;
            if (CompareCursor(b, a) < 0) std::swap(a, b);
            if (a.line == b.line)
            {
                std::string& s = buf.lines[a.line];
                s.erase(s.begin() + a.column, s.begin() + b.column);
                caret = a;
                sel.active = false;
                RecolorLine(a.line);
                return;
            }
            std::string left = buf.lines[a.line].substr(0, a.column);
            std::string right = buf.lines[b.line].substr(b.column);
            buf.lines[a.line] = left + right;
            buf.lines.erase(buf.lines.begin() + a.line + 1, buf.lines.begin() + b.line + 1);
            caret = a;
            sel.active = false;
            RecolorLine(a.line);
        }

        void PasteFromClipboard()
        {
            const char* t = ImGui::GetClipboardText();
            if (!t) return;
            std::string text = t;
            if (sel.active) DeleteSelection();
            size_t b = 0;
            int insertedLines = 0;
            while (b <= text.size())
            {
                size_t e = text.find('\n', b);
                if (e == std::string::npos) e = text.size();
                std::string part = text.substr(b, e - b);
                for (char ch : part) InsertChar(ch);
                if (e < text.size())
                {
                    NewLine();
                    ++insertedLines;
                }
                b = e + 1;
            }
            if (insertedLines == 0) RecolorLine(caret.line);
        }

        static int CompareCursor(const Cursor& a, const Cursor& b)
        {
            if (a.line != b.line) return a.line < b.line ? -1 : 1;
            if (a.column != b.column) return a.column < b.column ? -1 : 1;
            return 0;
        }

        void RecolorAll()
        {
            colored.resize(buf.lines.size());
            if (highlighter == nullptr)
            {
                for (size_t i = 0; i < buf.lines.size(); ++i)
                {
                    colored[i].spans.clear();
                    colored[i].spans.push_back({0, (int)buf.lines[i].size(), IM_COL32(220, 220, 220, 255)});
                }
                return;
            }
            for (size_t i = 0; i < buf.lines.size(); ++i)
            {
                highlighter->ColorizeLine(buf.lines[i], colored[i]);
                if (colored[i].spans.empty()) colored[i].spans.push_back({0, (int)buf.lines[i].size(), IM_COL32(220, 220, 220, 255)});
            }
        }

        void RecolorLine(int line)
        {
            if (line < 0 || line >= (int)buf.lines.size()) return;
            if (highlighter == nullptr)
            {
                colored[line].spans.clear();
                colored[line].spans.push_back({0, (int)buf.lines[line].size(), IM_COL32(220, 220, 220, 255)});
                return;
            }
            highlighter->ColorizeLine(buf.lines[line], colored[line]);
            if (colored[line].spans.empty()) colored[line].spans.push_back({0, (int)buf.lines[line].size(), IM_COL32(220, 220, 220, 255)});
        }

        bool Render(const char* id, const ImVec2& size)
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, cfg.colBg);
            bool changed = false;

            ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);

            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImVec2 child_size = ImGui::GetContentRegionAvail();
            ImVec2 hitbox_min = origin;
            ImVec2 hitbox_max = ImVec2(origin.x + child_size.x, origin.y + child_size.y);

            ImGui::SetCursorScreenPos(hitbox_min);
            ImGui::InvisibleButton((std::string(id) + "##hitbox").c_str(), child_size, ImGuiButtonFlags_AllowOverlap);
            hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

            ImGui::SetCursorScreenPos(origin);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            float lineHeight = ImGui::GetTextLineHeight() * cfg.lineSpacing;
            int totalLines = (int)buf.lines.size();
            float digitsWidth = ImGui::CalcTextSize("00000").x + cfg.gutterPadding * 2.f;
            const float contentRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

            ImGuiListClipper clipper;
            clipper.Begin(totalLines, lineHeight);

            while (clipper.Step())
            {
                for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line)
                {
                    ImVec2 linePos = ImVec2(origin.x, origin.y + line * lineHeight);

                    // Current line background
                    if (line == caret.line)
                        dl->AddRectFilled(ImVec2(linePos.x + digitsWidth, linePos.y), ImVec2(contentRight, linePos.y + lineHeight), cfg.colCurrentLineBg);

                    // Gutter
                    dl->AddRectFilled(linePos, ImVec2(linePos.x + digitsWidth, linePos.y + lineHeight), cfg.colGutterBg);

                    // Line numbers
                    char bufn[32];
                    snprintf(bufn, 32, "%d", line + 1);
                    ImU32 lnCol = (line == caret.line) ? ImGui::GetColorU32(ImGuiCol_Text) : cfg.colGutterText;
                    dl->AddText(ImVec2(linePos.x + cfg.gutterPadding, linePos.y), lnCol, bufn);

                    const std::string& s = buf.lines[line];
                    const auto& spans = colored[line].spans;

                    // --- Selection Rendering ---
                    if (sel.active)
                    {
                        Cursor a = sel.a, b = sel.b;
                        if (CompareCursor(b, a) < 0) std::swap(a, b);

                        if (line >= a.line && line <= b.line)
                        {
                            int startCol = (line == a.line) ? a.column : 0;
                            int endCol   = (line == b.line) ? b.column : (int)s.size();

                            if (startCol < endCol)
                            {
                                std::string selectedText = s.substr(startCol, endCol - startCol);
                                float selX = linePos.x + digitsWidth + ImGui::CalcTextSize(s.substr(0, startCol).c_str()).x;
                                float selW = ImGui::CalcTextSize(selectedText.c_str()).x;
                                dl->AddRectFilled(ImVec2(selX, linePos.y), ImVec2(selX + selW, linePos.y + lineHeight), cfg.colSelectionBg);
                            }
                        }
                    }

                    // --- Text Rendering ---
                    float xCursor = linePos.x + digitsWidth;
                    int cursorByte = 0;

                    for (const auto& sp : spans)
                    {
                        if (sp.end <= sp.start) continue;
                        if (sp.start > cursorByte)
                        {
                            std::string gap = s.substr(cursorByte, sp.start - cursorByte);
                            dl->AddText(ImVec2(xCursor, linePos.y), ImGui::GetColorU32(ImGuiCol_Text), gap.c_str());
                            xCursor += ImGui::CalcTextSize(gap.c_str()).x;
                            cursorByte = sp.start;
                        }
                        std::string chunk = s.substr(sp.start, sp.end - sp.start);
                        dl->AddText(ImVec2(xCursor, linePos.y), sp.color, chunk.c_str());
                        xCursor += ImGui::CalcTextSize(chunk.c_str()).x;
                        cursorByte = sp.end;
                    }

                    if (cursorByte < (int)s.size())
                    {
                        std::string rest = s.substr(cursorByte);
                        dl->AddText(ImVec2(xCursor, linePos.y), ImGui::GetColorU32(ImGuiCol_Text), rest.c_str());
                    }

                    // --- Caret ---
                    if (line == caret.line && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
                    {
                        std::string prefix = s.substr(0, caret.column);
                        float caretX = linePos.x + digitsWidth + ImGui::CalcTextSize(prefix.c_str()).x;
                        dl->AddRectFilled(ImVec2(caretX, linePos.y), ImVec2(caretX + 1.0f, linePos.y + lineHeight), cfg.colCaret);
                    }
                }
            }

            HandleInput(id, changed);
            ImGui::EndChild();
            ImGui::PopStyleColor();
            return changed;
        }
        void HandleInput(const char* id,bool& changed)
        {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetItemAllowOverlap();
            bool focused = ImGui::IsItemFocused() || ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                float scrollY = ImGui::GetScrollY();
                float scrollX = ImGui::GetScrollX();
                ImVec2 pos = ImGui::GetMousePos();
                ImVec2 origin = ImGui::GetItemRectMin();
                float lineHeight = ImGui::GetTextLineHeight() * cfg.lineSpacing;
                float digitsWidth = ImGui::CalcTextSize("00000").x + cfg.gutterPadding * 2.f;

                int line = (int)((pos.y - origin.y + scrollY) / lineHeight);
                line = std::max(0, std::min(line, (int)buf.lines.size() - 1));

                float localX = pos.x - origin.x + scrollX - digitsWidth;
                if (localX < 0.f) localX = 0.f;

                caret.line = line;
                caret.column = ClosestColumnForX(buf.lines[line], localX);
                preferredX = localX;

                sel.active = true;
                sel.a = caret;   // start of selection
                sel.b = caret;   // initial
            }

            if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                float scrollY = ImGui::GetScrollY();
                float scrollX = ImGui::GetScrollX();
                ImVec2 pos = ImGui::GetMousePos();
                ImVec2 origin = ImGui::GetItemRectMin();
                float lineHeight = ImGui::GetTextLineHeight() * cfg.lineSpacing;
                float digitsWidth = ImGui::CalcTextSize("00000").x + cfg.gutterPadding * 2.f;

                int line = (int)((pos.y - origin.y + scrollY) / lineHeight);
                line = std::max(0, std::min(line, (int)buf.lines.size() - 1));

                float localX = pos.x - origin.x + scrollX - digitsWidth;
                if (localX < 0.f) localX = 0.f;

                caret.line = line;
                caret.column = ClosestColumnForX(buf.lines[line], localX);
                preferredX = localX;

                sel.b = caret;  // update selection as mouse moves
            }

            if (!focused) return;

            auto mods = io.KeyMods;
            bool ctrl = (mods & ImGuiMod_Ctrl) != 0;
            bool alt = (mods & ImGuiMod_Alt) != 0;
            bool shift = (mods & ImGuiMod_Shift) != 0;

            for (int n = 0; n < io.InputQueueCharacters.Size; n++)
            {
                ImWchar w = io.InputQueueCharacters[n];
                if (w == 0 || w == 127) continue;
                if (w == '\n' || w == '\r')
                {
                    if (sel.active) DeleteSelection();
                    NewLine();
                    changed = true;
                }
                else if (w >= 32 && w != 127)
                {
                    if (sel.active) DeleteSelection();
                    InsertChar((char)w);
                    changed = true;
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
            {
                if (sel.active) DeleteSelection();
                NewLine();
                changed = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
            {
                if (sel.active) DeleteSelection(); else Backspace();
                changed = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                if (sel.active) DeleteSelection(); else DeleteForward();
                changed = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Tab)) { InsertChar('\t'); changed = true; }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
            {
                Cursor oldCaret = caret;
                if (ctrl) MoveLeft(true); else MoveLeft(false);
                preferredX = -1.f;
                if (shift)
                {
                    if (!sel.active) { sel.active = true; sel.a = oldCaret; sel.b = caret; }
                    else { sel.b = caret; }
                }
                else
                {
                    sel.active = false;
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
            {
                Cursor oldCaret = caret;
                if (ctrl) MoveRight(true); else MoveRight(false);
                preferredX = -1.f;
                if (shift)
                {
                    if (!sel.active) { sel.active = true; sel.a = oldCaret; sel.b = caret; }
                    else { sel.b = caret; }
                }
                else
                {
                    sel.active = false;
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            {
                Cursor oldCaret = caret;
                MoveUp();
                if (shift)
                {
                    if (!sel.active) { sel.active = true; sel.a = oldCaret; sel.b = caret; }
                    else { sel.b = caret; }
                }
                else
                {
                    sel.active = false;
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            {
                Cursor oldCaret = caret;
                MoveDown();
                if (shift)
                {
                    if (!sel.active) { sel.active = true; sel.a = oldCaret; sel.b = caret; }
                    else { sel.b = caret; }
                }
                else
                {
                    sel.active = false;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Home)) { caret.column = 0; preferredX = -1.f; }
            if (ImGui::IsKeyPressed(ImGuiKey_End)) { caret.column = (int)buf.lines[caret.line].size(); preferredX = -1.f; }
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_A)) { sel.active = true; sel.a = {0,0}; sel.b = {(int)buf.lines.size() - 1, (int)buf.lines.back().size()}; }
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C)) { CopySelectionToClipboard(); }
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X)) { CopySelectionToClipboard(); DeleteSelection(); changed = true; }
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V)) { PasteFromClipboard(); changed = true; }
        }
    };

    inline static bool DemoWindow(bool* p_open = nullptr)
    {
        static CodeEditor editor;
        static CppSyntaxHighlighter cpp;
        static bool init=false;
        if(!init)
        {
            editor.SetHighlighter(&cpp);
            editor.SetText(
                "#include <iostream>\n"
                "int main()\n"
                "{\n"
                "    std::cout << \"Hello, ImGui Code Editor!\" << std::endl;\n"
                "    // edit me\n"
                "    return 0;\n"
                "}\n"
            );
            init=true;
        }
        bool changed=false;
        ImGui::SetNextWindowSize(ImVec2(900,600),ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Code Editor Demo",p_open,ImGuiWindowFlags_MenuBar))
        {
            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("Highlighter"))
                {
                    bool isCpp=editor.highlighter==&cpp;
                    if(ImGui::MenuItem("C++",NULL,isCpp))
                    {
                        editor.SetHighlighter(&cpp);
                    }
                    if(ImGui::MenuItem("None",NULL,!isCpp))
                    {
                        editor.SetHighlighter(nullptr);
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Edit"))
                {
                    if(ImGui::MenuItem("Copy","Ctrl+C")) {}
                    if(ImGui::MenuItem("Paste","Ctrl+V")) {}
                    if(ImGui::MenuItem("Cut","Ctrl+X")) {}
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            changed = editor.Render("##code",ImVec2(0,0));
        }
        ImGui::End();
        return changed;
    }
}