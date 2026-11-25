#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/platform.H>

#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
const Fl_Color kHeaderColor    = fl_rgb_color(240, 224, 120);
const Fl_Color kPinActiveColor = fl_rgb_color(224, 168, 48);
const Fl_Color kBodyColor      = fl_rgb_color(255, 249, 196);
const Fl_Color kTextColor      = fl_rgb_color(45, 45, 45);
const Fl_Color kInputBg        = fl_rgb_color(255, 252, 224);

// ASCIIの空白のみを対象にする。
// UTF-8のマルチバイト文字(全角スペース等)をバイト単位で指定すると、
// 同じ先頭バイトを持つ別の文字(ひらがな等)まで削ってしまうため。
std::string trim_whitespace(const std::string& s) {
    const std::string ws = " \t\r\n";
    const size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    const size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

// Windowsで使えない予約名(CON, NUL, COM1 など)かどうか
bool is_windows_reserved_name(const std::string& name) {
    const size_t dot = name.find('.');
    std::string stem = name.substr(0, dot);
    for (char& c : stem) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL") return true;
    if (stem.size() == 4 && (stem.compare(0, 3, "COM") == 0 || stem.compare(0, 3, "LPT") == 0) &&
        stem[3] >= '1' && stem[3] <= '9') {
        return true;
    }
    return false;
}

std::string sanitize_filename(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    const std::string forbidden = R"(\/:*?"<>|)";
    for (unsigned char c : raw) {
        if (c < 32) continue;
        if (forbidden.find(static_cast<char>(c)) != std::string::npos) {
            out.push_back('_');
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    out = trim_whitespace(out);

    // Windowsでは末尾のドット・空白がファイル名に使えない
    while (!out.empty() && (out.back() == '.' || out.back() == ' ')) {
        out.pop_back();
    }
    if (!out.empty() && is_windows_reserved_name(out)) {
        out.insert(out.begin(), '_');
    }
    return out;
}

std::string make_timestamp_name() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);

    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d_%02d%02d%02d",
                  tm_buf.tm_year + 1900,
                  tm_buf.tm_mon + 1,
                  tm_buf.tm_mday,
                  tm_buf.tm_hour,
                  tm_buf.tm_min,
                  tm_buf.tm_sec);
    return std::string(buf);
}

std::string ensure_txt_extension(std::string path) {
    if (path.empty()) return path;
    const size_t slash = path.find_last_of("/\\");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        path += ".txt";
    }
    return path;
}

// パスはUTF-8。Windowsでは narrow 文字列がANSIコードページ扱いになり
// 日本語パスで失敗するため、u8path 経由で開く。
bool write_text_file(const std::string& path, const std::string& text) {
    std::ofstream ofs(std::filesystem::u8path(path), std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs.write(text.data(), static_cast<std::streamsize>(text.size()));
    return ofs.good();
}

void set_window_topmost(Fl_Window* win, bool topmost) {
    if (!win || !win->shown()) return;

#if defined(__APPLE__)
    NSWindow* nswin = (__bridge NSWindow*)fl_mac_xid(win);
    if (nswin) {
        [nswin setLevel:(topmost ? NSFloatingWindowLevel : NSNormalWindowLevel)];
        if (topmost) {
            [nswin orderFront:nil];
        }
    }
#elif defined(_WIN32)
#  if FL_API_VERSION >= 10400
    HWND hwnd = fl_win32_xid(win);
#  else
    HWND hwnd = reinterpret_cast<HWND>(fl_xid(win));
#  endif
    if (hwnd) {
        SetWindowPos(hwnd,
                     topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
#else
    (void)topmost;
#endif
}
}  // namespace

struct NoteApp {
    Fl_Window* win = nullptr;
    Fl_Input* title_input = nullptr;
    Fl_Button* top_btn = nullptr;
    Fl_Button* close_btn = nullptr;
    Fl_Multiline_Input* editor = nullptr;
    bool pinned = false;
};

void toggle_topmost(NoteApp* app) {
    if (!app || !app->win) return;
    app->pinned = !app->pinned;
    set_window_topmost(app->win, app->pinned);

    if (app->top_btn) {
        app->top_btn->color(app->pinned ? kPinActiveColor : kHeaderColor);
        app->top_btn->label(app->pinned ? "TOP*" : "TOP");
        app->top_btn->redraw();
    }
}

void on_top_button(Fl_Widget*, void* userdata) {
    toggle_topmost(static_cast<NoteApp*>(userdata));
}

class NoteWindow : public Fl_Window {
public:
    NoteWindow(int w, int h, NoteApp* app) : Fl_Window(w, h), app_(app) {}

    int handle(int event) override {
        if (event == FL_SHORTCUT || event == FL_KEYBOARD) {
            if (Fl::test_shortcut(FL_COMMAND | 't') || Fl::test_shortcut(FL_CTRL | 't')) {
                toggle_topmost(app_);
                return 1;
            }
        }
        return Fl_Window::handle(event);
    }

private:
    NoteApp* app_ = nullptr;
};

class DragHeader : public Fl_Box {
public:
    DragHeader(int x, int y, int w, int h, NoteApp* app)
        : Fl_Box(x, y, w, h), app_(app) {}

    int handle(int event) override {
        Fl_Window* win = top_window();
        if (!win) return Fl_Box::handle(event);

        switch (event) {
            case FL_PUSH:
                if (Fl::event_button() == FL_LEFT_MOUSE) {
                    offset_x_ = Fl::event_x_root() - win->x();
                    offset_y_ = Fl::event_y_root() - win->y();
                    dragging_ = true;
                    return 1;
                }
                if (Fl::event_button() == FL_RIGHT_MOUSE) {
                    toggle_topmost(app_);
                    return 1;
                }
                break;
            case FL_DRAG:
                if (dragging_) {
                    win->position(Fl::event_x_root() - offset_x_,
                                  Fl::event_y_root() - offset_y_);
                    return 1;
                }
                break;
            case FL_RELEASE:
                if (dragging_) {
                    dragging_ = false;
                    return 1;
                }
                break;
            default:
                break;
        }
        return Fl_Box::handle(event);
    }

private:
    NoteApp* app_ = nullptr;
    bool dragging_ = false;
    int offset_x_ = 0;
    int offset_y_ = 0;
};

void save_and_close(Fl_Widget*, void* userdata) {
    auto* app = static_cast<NoteApp*>(userdata);
    if (!app || !app->win || !app->editor) return;

    const std::string raw_title =
        (app->title_input && app->title_input->value()) ? app->title_input->value() : "";
    const std::string clean_title = sanitize_filename(raw_title);
    const std::string base_name =
        clean_title.empty() ? make_timestamp_name() : clean_title;
    const std::string default_filename = base_name + ".txt";

    Fl_Native_File_Chooser chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.title("付箋をテキスト形式で保存");
    chooser.filter("テキストファイル\t*.txt\n");
    chooser.preset_file(default_filename.c_str());

    if (chooser.show() == 0) {
        std::string out_path = ensure_txt_extension(chooser.filename());
        const char* body_cstr = app->editor->value();
        const std::string body = body_cstr ? body_cstr : "";
        write_text_file(out_path, body);
    }

    app->win->hide();
}

int main(int argc, char** argv) {
    const int w = 320;
    const int h = 250;
    const int header_h = 30;
    const int close_w = 26;
    const int top_w = 40;
    const int pad = 4;

    NoteApp app;
    auto* note_win = new NoteWindow(w, h, &app);
    app.win = note_win;
    app.win->border(0);
    app.win->color(kBodyColor);
    app.win->begin();

    auto* header = new DragHeader(0, 0, w, header_h, &app);
    header->box(FL_FLAT_BOX);
    header->color(kHeaderColor);

    const int title_w = w - close_w - top_w - pad * 4;
    app.title_input = new Fl_Input(pad, pad, title_w, header_h - pad * 2);
    app.title_input->box(FL_FLAT_BOX);
    app.title_input->color(kInputBg);
    app.title_input->textcolor(kTextColor);
    app.title_input->textsize(13);

    app.top_btn = new Fl_Button(w - close_w - top_w - pad * 2, pad, top_w, header_h - pad * 2, "TOP");
    app.top_btn->box(FL_UP_BOX);
    app.top_btn->color(kHeaderColor);
    app.top_btn->labelcolor(kTextColor);
    app.top_btn->labelsize(11);
    app.top_btn->clear_visible_focus();
    app.top_btn->callback(on_top_button, &app);

    app.close_btn = new Fl_Button(w - close_w - pad, pad, close_w, header_h - pad * 2, "x");
    app.close_btn->box(FL_UP_BOX);
    app.close_btn->color(kHeaderColor);
    app.close_btn->labelcolor(kTextColor);
    app.close_btn->labelsize(13);
    app.close_btn->clear_visible_focus();
    app.close_btn->callback(save_and_close, &app);

    app.editor = new Fl_Multiline_Input(0, header_h, w, h - header_h);
    app.editor->box(FL_FLAT_BOX);
    app.editor->color(kBodyColor);
    app.editor->textcolor(kTextColor);
    app.editor->textsize(14);
    app.editor->wrap(1);

    app.win->end();
    app.win->callback(save_and_close, &app);

    app.win->show(argc, argv);
    return Fl::run();
}
