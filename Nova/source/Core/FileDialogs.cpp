#include "Core/FileDialogs.h"
#include "Core/Application.h"
#include "Core/Log.h"

#include <nfd.hpp>
#include <nfd_sdl3.h>

namespace Nova::FileDialogs
{
    local std::filesystem::path path_default;
    local std::filesystem::path path_selected;
    local bool is_initialized = false;

    std::filesystem::path EnsureExtension(const std::filesystem::path& path, std::string_view extension);

    void Init()
    {
        if (is_initialized)
        {
            WARN("%s", "File Dialogs cannot be initialized more than once");
            return;
        }

        if (!NFD::Init())
        {
            ERROR("FileDialogs::Init - %s", "Failed to initialize File Dialogs!");
            return;
        }

        path_default = std::filesystem::current_path();
        INFO("%s", "File Dialogs were initialized successfully");
    }

    void Shutdown()
    {
        INFO("%s", "File Dialogs are shutting down...");
        NFD::Quit();
        is_initialized = false;
        path_default = "Assets";
        path_selected.clear();
    }

    bool AskOpen(const FileDialogFilter* filters, u8 filter_count)
    {
        const Window& window = Application::GetWindow();
        SDL_Window* const window_handle = static_cast<SDL_Window* const>(window.handle);
        char* path_out = NULL;

        nfdwindowhandle_t parent_window;
        NFD_GetNativeWindowFromSDLWindow(window_handle, &parent_window);

        nfdresult_t result = NFD::OpenDialog(path_out, (nfdfilteritem_t*)filters, filter_count, path_default.string().c_str(), parent_window);
        if (result == NFD_CANCEL || result == NFD_ERROR)
            return false;

        path_selected = path_out;
        free(path_out);

        return true;
    }

    bool AskSave(const FileDialogFilter* filters, u8 filter_count)
    {
        const Window& window = Application::GetWindow();
        SDL_Window* const window_handle = static_cast<SDL_Window* const>(window.handle);
        char* path_out = NULL;

        nfdwindowhandle_t parent_window;
        const bool what = NFD_GetNativeWindowFromSDLWindow(window_handle, &parent_window);

        nfdresult_t result = NFD::SaveDialog(path_out, (nfdfilteritem_t*)filters, filter_count, path_default.string().c_str(), NULL, parent_window);
        if (result == NFD_CANCEL || result == NFD_ERROR)
            return false;

        path_selected = path_out;
        free(path_out);

        // Ensure the path has the correct extension based on the first filter
        if (filters != NULL && filter_count > 0)
        {
            // Get the first extension from the specification (e.g., "anim" from "anim")
            // If there are multiple extensions (e.g., "png,jpg,jpeg"), take the first one
            std::string_view spec = filters[0].specification;
            u64 commaPos = spec.find(',');
            std::string_view first_extension = (commaPos != std::string::npos) ? spec.substr(0, commaPos) : spec;

            path_selected = EnsureExtension(path_selected, first_extension);
        }

        return true;
    }

    void OpenExplorer(const std::filesystem::path& path)
    {
        // Ensure the path actually exists to avoid OS errors
        if (!std::filesystem::exists(path)) return;

        // Convert to an absolute path for safety
        std::string path_str = std::filesystem::absolute(path).string();

#if defined(_WIN32)
        // Windows: "explorer" opens the file manager
        std::string command = "explorer \"" + path_str + "\"";
#elif defined(__APPLE__)
        // macOS: "open" launches Finder
        std::string command = "open \"" + path_str + "\"";
#else
        // Linux/Unix: "xdg-open" launches the user's default file manager (Nautilus, Dolphin, etc.)
        std::string command = "xdg-open \"" + path_str + "\"";
#endif

        std::system(command.c_str());
    }

    const std::filesystem::path& GetPathSelected() { return path_selected; }
    const std::filesystem::path& GetPathDefault() { return path_default; }
    void SetDefaultPath(const std::filesystem::path& path) { path_selected = path; }

    std::filesystem::path EnsureExtension(const std::filesystem::path& path, std::string_view extension)
    {
        std::filesystem::path path_result = path;

        // Check if the path already has an extension
        if (path_result.has_extension())
        {
            std::string current_extension = path_result.extension().string();
            // Remove the leading dot from the extension for comparison
            if (!current_extension.empty() && current_extension[0] == '.')
                current_extension = current_extension.substr(1);

            // If it matches the expected extension, we're good
            if (current_extension == extension)
                return path_result;
        }

        // Either no extension or wrong extension - append the correct one
        path_result += ".";
        path_result += extension;
        return path_result;
    }

}
