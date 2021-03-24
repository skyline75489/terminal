/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- Profile.hpp

Abstract:
- A profile acts as a single set of terminal settings. Many tabs or panes could
     exist side-by-side with different profiles simultaneously.

Author(s):
- Mike Griese - March 2019

--*/
#pragma once

#include "Profile.g.h"
#include "IInheritable.h"

#include "../inc/cppwinrt_utils.h"
#include "JsonUtils.h"
#include <DefaultSettings.h>

// fwdecl unittest classes
namespace SettingsModelLocalTests
{
    class DeserializationTests;
    class ProfileTests;
    class ColorSchemeTests;
    class KeyBindingsTests;
};
namespace TerminalAppUnitTests
{
    class DynamicProfileTests;
    class JsonTests;
};

// GUID used for generating GUIDs at runtime, for profiles that did not have a
// GUID specified manually.
constexpr GUID RUNTIME_GENERATED_PROFILE_NAMESPACE_GUID = { 0xf65ddb7e, 0x706b, 0x4499, { 0x8a, 0x50, 0x40, 0x31, 0x3c, 0xaf, 0x51, 0x0a } };

namespace winrt::Microsoft::Terminal::Settings::Model::implementation
{
    struct Profile : ProfileT<Profile>, IInheritable<Profile>
    {
    public:
        Profile();
        Profile(guid guid);

        hstring ToString()
        {
            return Name();
        }

        static com_ptr<Profile> CloneInheritanceGraph(com_ptr<Profile> oldProfile, com_ptr<Profile> newProfile, std::unordered_map<void*, com_ptr<Profile>>& visited);
        static com_ptr<Profile> CopySettings(com_ptr<Profile> source);

        Json::Value GenerateStub() const;
        static com_ptr<Profile> FromJson(const Json::Value& json);
        bool ShouldBeLayered(const Json::Value& json) const;
        void LayerJson(const Json::Value& json);
        static bool IsDynamicProfileObject(const Json::Value& json);
        Json::Value ToJson() const;

        hstring EvaluatedStartingDirectory() const;
        hstring ExpandedBackgroundImagePath() const;
        static guid GetGuidOrGenerateForJson(const Json::Value& json) noexcept;

        WINRT_PROPERTY(OriginTag, Origin, OriginTag::Custom);

        INHERITABLE_SETTING(Model::Profile, guid, Guid, _GenerateGuidForProfile(Name(), Source()));
        INHERITABLE_SETTING(Model::Profile, hstring, Name, L"Default");
        INHERITABLE_SETTING(Model::Profile, hstring, Source);
        INHERITABLE_SETTING(Model::Profile, bool, Hidden, false);
        INHERITABLE_SETTING(Model::Profile, guid, ConnectionType);

        // Default Icon: Segoe MDL2 CommandPrompt icon
        INHERITABLE_SETTING(Model::Profile, hstring, Icon, L"\uE756");

        INHERITABLE_SETTING(Model::Profile, CloseOnExitMode, CloseOnExit, CloseOnExitMode::Graceful);
        INHERITABLE_SETTING(Model::Profile, hstring, TabTitle);
        INHERITABLE_NULLABLE_SETTING(Model::Profile, Windows::UI::Color, TabColor, nullptr);
        INHERITABLE_SETTING(Model::Profile, bool, SuppressApplicationTitle, false);

        INHERITABLE_SETTING(Model::Profile, bool, UseAcrylic, false);
        INHERITABLE_SETTING(Model::Profile, double, AcrylicOpacity, 0.5);
        INHERITABLE_SETTING(Model::Profile, Microsoft::Terminal::Control::ScrollbarState, ScrollState, Microsoft::Terminal::Control::ScrollbarState::Visible);

        INHERITABLE_SETTING(Model::Profile, hstring, FontFace, DEFAULT_FONT_FACE);
        INHERITABLE_SETTING(Model::Profile, int32_t, FontSize, DEFAULT_FONT_SIZE);
        INHERITABLE_SETTING(Model::Profile, Windows::UI::Text::FontWeight, FontWeight, DEFAULT_FONT_WEIGHT);
        INHERITABLE_SETTING(Model::Profile, hstring, Padding, DEFAULT_PADDING);

        INHERITABLE_SETTING(Model::Profile, hstring, Commandline, L"cmd.exe");
        INHERITABLE_SETTING(Model::Profile, hstring, StartingDirectory);

        INHERITABLE_SETTING(Model::Profile, hstring, BackgroundImagePath);
        INHERITABLE_SETTING(Model::Profile, double, BackgroundImageOpacity, 1.0);
        INHERITABLE_SETTING(Model::Profile, Windows::UI::Xaml::Media::Stretch, BackgroundImageStretchMode, Windows::UI::Xaml::Media::Stretch::UniformToFill);
        INHERITABLE_SETTING(Model::Profile, ConvergedAlignment, BackgroundImageAlignment, ConvergedAlignment::Horizontal_Center | ConvergedAlignment::Vertical_Center);

        INHERITABLE_SETTING(Model::Profile, Microsoft::Terminal::Control::TextAntialiasingMode, AntialiasingMode, Microsoft::Terminal::Control::TextAntialiasingMode::Grayscale);
        INHERITABLE_SETTING(Model::Profile, bool, RetroTerminalEffect, false);
        INHERITABLE_SETTING(Model::Profile, hstring, PixelShaderPath, L"");
        INHERITABLE_SETTING(Model::Profile, bool, ForceFullRepaintRendering, false);
        INHERITABLE_SETTING(Model::Profile, bool, SoftwareRendering, false);

        INHERITABLE_SETTING(Model::Profile, hstring, ColorSchemeName, L"Campbell");

        INHERITABLE_NULLABLE_SETTING(Model::Profile, Windows::UI::Color, Foreground, nullptr);
        INHERITABLE_NULLABLE_SETTING(Model::Profile, Windows::UI::Color, Background, nullptr);
        INHERITABLE_NULLABLE_SETTING(Model::Profile, Windows::UI::Color, SelectionBackground, nullptr);
        INHERITABLE_NULLABLE_SETTING(Model::Profile, Windows::UI::Color, CursorColor, nullptr);

        INHERITABLE_SETTING(Model::Profile, int32_t, HistorySize, DEFAULT_HISTORY_SIZE);
        INHERITABLE_SETTING(Model::Profile, bool, SnapOnInput, true);
        INHERITABLE_SETTING(Model::Profile, bool, AltGrAliasing, true);

        INHERITABLE_SETTING(Model::Profile, Microsoft::Terminal::Core::CursorStyle, CursorShape, Microsoft::Terminal::Core::CursorStyle::Bar);
        INHERITABLE_SETTING(Model::Profile, uint32_t, CursorHeight, DEFAULT_CURSOR_HEIGHT);

        INHERITABLE_SETTING(Model::Profile, Model::BellStyle, BellStyle, BellStyle::Audible);

    private:
        static std::wstring EvaluateStartingDirectory(const std::wstring& directory);

        static guid _GenerateGuidForProfile(const hstring& name, const hstring& source) noexcept;

        friend class SettingsModelLocalTests::DeserializationTests;
        friend class SettingsModelLocalTests::ProfileTests;
        friend class SettingsModelLocalTests::ColorSchemeTests;
        friend class SettingsModelLocalTests::KeyBindingsTests;
        friend class TerminalAppUnitTests::DynamicProfileTests;
        friend class TerminalAppUnitTests::JsonTests;
    };
}

namespace winrt::Microsoft::Terminal::Settings::Model::factory_implementation
{
    BASIC_FACTORY(Profile);
}
