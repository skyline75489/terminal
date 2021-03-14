/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- TerminalSettings.h

Abstract:
- The implementation of the TerminalSettings winrt class. Provides both
        terminal control settings and terminal core settings.
Author(s):
- Mike Griese - March 2019

--*/
#pragma once

#include "TerminalSettings.g.h"
#include "../TerminalSettingsModel/IInheritable.h"
#include "../inc/cppwinrt_utils.h"
#include <DefaultSettings.h>
#include <conattrs.hpp>

// fwdecl unittest classes
namespace TerminalAppLocalTests
{
    class SettingsTests;
}

namespace winrt::TerminalApp::implementation
{
    struct TerminalSettings : TerminalSettingsT<TerminalSettings>, winrt::Microsoft::Terminal::Settings::Model::implementation::IInheritable<TerminalSettings>
    {
        TerminalSettings() = default;
        TerminalSettings(const Microsoft::Terminal::Settings::Model::CascadiaSettings& appSettings,
                         guid profileGuid,
                         const Microsoft::Terminal::TerminalControl::IKeyBindings& keybindings);

        static std::tuple<guid, TerminalApp::TerminalSettings> BuildSettings(const Microsoft::Terminal::Settings::Model::CascadiaSettings& appSettings,
                                                                             const Microsoft::Terminal::Settings::Model::NewTerminalArgs& newTerminalArgs,
                                                                             const Microsoft::Terminal::TerminalControl::IKeyBindings& keybindings);

        void ApplyColorScheme(const Microsoft::Terminal::Settings::Model::ColorScheme& scheme);

        // --------------------------- Core Settings ---------------------------
        //  All of these settings are defined in ICoreSettings.

        // GetColorTableEntry needs to be implemented manually, to get a
        // particular value from the array.
        uint32_t GetColorTableEntry(int32_t index) noexcept;
        void ColorTable(std::array<uint32_t, 16> colors);
        std::array<uint32_t, 16> ColorTable();

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, uint32_t, DefaultForeground, DEFAULT_FOREGROUND_WITH_ALPHA);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, uint32_t, DefaultBackground, DEFAULT_BACKGROUND_WITH_ALPHA);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, uint32_t, SelectionBackground, DEFAULT_FOREGROUND);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, int32_t, HistorySize, DEFAULT_HISTORY_SIZE);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, int32_t, InitialRows, 30);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, int32_t, InitialCols, 80);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, SnapOnInput, true);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, AltGrAliasing, true);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, uint32_t, CursorColor, DEFAULT_CURSOR_COLOR);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, Microsoft::Terminal::TerminalControl::CursorStyle, CursorShape, Microsoft::Terminal::TerminalControl::CursorStyle::Vintage);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, uint32_t, CursorHeight, DEFAULT_CURSOR_HEIGHT);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, WordDelimiters, DEFAULT_WORD_DELIMITERS);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, CopyOnSelect, false);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, InputServiceWarning, true);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, FocusFollowMouse, false);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, Windows::Foundation::IReference<uint32_t>, TabColor, nullptr);

        // When set, StartingTabColor allows to create a terminal with a "sticky" tab color.
        // This color is prioritized above the TabColor (that is usually initialized based on profile settings).
        // Due to this prioritization, the tab color will be preserved upon settings reload
        // (even if the profile's tab color gets altered or removed).
        // This property is expected to be passed only once upon terminal creation.
        // TODO: to ensure that this property is not populated during settings reload,
        // we should consider moving this property to a separate interface,
        // passed to the terminal only upon creation.
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, Windows::Foundation::IReference<uint32_t>, StartingTabColor, nullptr);

        // ------------------------ End of Core Settings -----------------------

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, ProfileName);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, UseAcrylic, false);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, double, TintOpacity, 0.5);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, Padding, DEFAULT_PADDING);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, FontFace, DEFAULT_FONT_FACE);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, int32_t, FontSize, DEFAULT_FONT_SIZE);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, winrt::Windows::UI::Text::FontWeight, FontWeight);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, BackgroundImage);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, double, BackgroundImageOpacity, 1.0);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, winrt::Windows::UI::Xaml::Media::Stretch, BackgroundImageStretchMode, winrt::Windows::UI::Xaml::Media::Stretch::UniformToFill);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, winrt::Windows::UI::Xaml::HorizontalAlignment, BackgroundImageHorizontalAlignment, winrt::Windows::UI::Xaml::HorizontalAlignment::Center);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, winrt::Windows::UI::Xaml::VerticalAlignment, BackgroundImageVerticalAlignment, winrt::Windows::UI::Xaml::VerticalAlignment::Center);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, Microsoft::Terminal::TerminalControl::IKeyBindings, KeyBindings, nullptr);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, Commandline);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, StartingDirectory);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, StartingTitle);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, SuppressApplicationTitle);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, EnvironmentVariables);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, Microsoft::Terminal::TerminalControl::ScrollbarState, ScrollState, Microsoft::Terminal::TerminalControl::ScrollbarState::Visible);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, Microsoft::Terminal::TerminalControl::TextAntialiasingMode, AntialiasingMode, Microsoft::Terminal::TerminalControl::TextAntialiasingMode::Grayscale);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, RetroTerminalEffect, false);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, ForceFullRepaintRendering, false);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, SoftwareRendering, false);
        INHERITABLE_SETTING(TerminalApp::TerminalSettings, bool, ForceVTInput, false);

        INHERITABLE_SETTING(TerminalApp::TerminalSettings, hstring, PixelShaderPath);

    private:
        std::optional<std::array<uint32_t, COLOR_TABLE_SIZE>> _ColorTable;
        gsl::span<uint32_t> _getColorTableImpl();
        void _ApplyProfileSettings(const Microsoft::Terminal::Settings::Model::Profile& profile, const Windows::Foundation::Collections::IMapView<hstring, Microsoft::Terminal::Settings::Model::ColorScheme>& schemes);
        void _ApplyGlobalSettings(const Microsoft::Terminal::Settings::Model::GlobalAppSettings& globalSettings) noexcept;

        friend class TerminalAppLocalTests::SettingsTests;
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    BASIC_FACTORY(TerminalSettings);
}
