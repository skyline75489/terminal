// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include <LibraryResources.h>
#include "TabBase.h"
#include "TabBase.g.cpp"

using namespace winrt;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Microsoft::Terminal::Control;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace winrt::Windows::System;

namespace winrt
{
    namespace MUX = Microsoft::UI::Xaml;
    namespace WUX = Windows::UI::Xaml;
}

namespace winrt::TerminalApp::implementation
{
    WUX::FocusState TabBase::FocusState() const noexcept
    {
        return _focusState;
    }

    // Method Description:
    // - Prepares this tab for being removed from the UI hierarchy
    void TabBase::Shutdown()
    {
        Content(nullptr);
        _ClosedHandlers(nullptr, nullptr);
    }

    // Method Description:
    // - Creates a context menu attached to the tab.
    // Currently contains elements allowing the user to close the selected tab
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TabBase::_CreateContextMenu()
    {
        auto weakThis{ get_weak() };

        // Close
        Controls::MenuFlyoutItem closeTabMenuItem;
        Controls::FontIcon closeSymbol;
        closeSymbol.FontFamily(Media::FontFamily{ L"Segoe MDL2 Assets" });
        closeSymbol.Glyph(L"\xE711");

        closeTabMenuItem.Click([weakThis](auto&&, auto&&) {
            if (auto tab{ weakThis.get() })
            {
                tab->_ClosedHandlers(nullptr, nullptr);
            }
        });
        closeTabMenuItem.Text(RS_(L"TabClose"));
        closeTabMenuItem.Icon(closeSymbol);

        // Build the menu
        Controls::MenuFlyout newTabFlyout;
        newTabFlyout.Items().Append(_CreateCloseSubMenu());
        newTabFlyout.Items().Append(closeTabMenuItem);
        TabViewItem().ContextFlyout(newTabFlyout);
    }

    // Method Description:
    // - Creates a sub-menu containing menu items to close multiple tabs
    // Arguments:
    // - <none>
    // Return Value:
    // - the created MenuFlyoutSubItem
    Controls::MenuFlyoutSubItem TabBase::_CreateCloseSubMenu()
    {
        auto weakThis{ get_weak() };

        // Close tabs after
        _closeTabsAfterMenuItem.Click([weakThis](auto&&, auto&&) {
            if (auto tab{ weakThis.get() })
            {
                tab->_CloseTabsAfter();
            }
        });
        _closeTabsAfterMenuItem.Text(RS_(L"TabCloseAfter"));

        // Close other tabs
        _closeOtherTabsMenuItem.Click([weakThis](auto&&, auto&&) {
            if (auto tab{ weakThis.get() })
            {
                tab->_CloseOtherTabs();
            }
        });
        _closeOtherTabsMenuItem.Text(RS_(L"TabCloseOther"));

        Controls::MenuFlyoutSubItem closeSubMenu;
        closeSubMenu.Text(RS_(L"TabCloseSubMenu"));
        closeSubMenu.Items().Append(_closeTabsAfterMenuItem);
        closeSubMenu.Items().Append(_closeOtherTabsMenuItem);

        return closeSubMenu;
    }

    // Method Description:
    // - Enable the Close menu items based on tab index and total number of tabs
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TabBase::_EnableCloseMenuItems()
    {
        // close other tabs is enabled only if there are other tabs
        _closeOtherTabsMenuItem.IsEnabled(TabViewNumTabs() > 1);
        // close tabs after is enabled only if there are other tabs on the right
        _closeTabsAfterMenuItem.IsEnabled(TabViewIndex() < TabViewNumTabs() - 1);
    }

    void TabBase::_CloseTabsAfter()
    {
        CloseTabsAfterArgs args{ _TabViewIndex };
        ActionAndArgs closeTabsAfter{ ShortcutAction::CloseTabsAfter, args };

        _dispatch.DoAction(closeTabsAfter);
    }

    void TabBase::_CloseOtherTabs()
    {
        CloseOtherTabsArgs args{ _TabViewIndex };
        ActionAndArgs closeOtherTabs{ ShortcutAction::CloseOtherTabs, args };

        _dispatch.DoAction(closeOtherTabs);
    }

    void TabBase::UpdateTabViewIndex(const uint32_t idx, const uint32_t numTabs)
    {
        TabViewIndex(idx);
        TabViewNumTabs(numTabs);
        _EnableCloseMenuItems();
        _UpdateSwitchToTabKeyChord();
    }

    void TabBase::SetDispatch(const winrt::TerminalApp::ShortcutActionDispatch& dispatch)
    {
        _dispatch = dispatch;
    }

    void TabBase::SetKeyMap(const Microsoft::Terminal::Settings::Model::KeyMapping& keymap)
    {
        _keymap = keymap;
        _UpdateSwitchToTabKeyChord();
    }

    // Method Description:
    // - Sets the key chord resulting in switch to the current tab.
    // Updates tool tip if required
    // Arguments:
    // - keyChord - string representation of the key chord that switches to the current tab
    // Return Value:
    // - <none>
    winrt::fire_and_forget TabBase::_UpdateSwitchToTabKeyChord()
    {
        SwitchToTabArgs args{ _TabViewIndex };
        ActionAndArgs switchToTab{ ShortcutAction::SwitchToTab, args };
        const auto keyChord = _keymap ? _keymap.GetKeyBindingForActionWithArgs(switchToTab) : nullptr;
        const auto keyChordText = keyChord ? KeyChordSerialization::ToString(keyChord) : L"";

        if (_keyChord == keyChordText)
        {
            return;
        }

        _keyChord = keyChordText;

        auto weakThis{ get_weak() };

        co_await winrt::resume_foreground(TabViewItem().Dispatcher());

        if (auto tab{ weakThis.get() })
        {
            _UpdateToolTip();
        }
    }

    // Method Description:
    // - Creates a text for the title run in the tool tip by returning tab title
    // Arguments:
    // - <none>
    // Return Value:
    // - The value to populate in the title run of the tool tip
    winrt::hstring TabBase::_CreateToolTipTitle()
    {
        return _Title;
    }

    // Method Description:
    // - Sets tab tool tip to a concatenation of title and key chord
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TabBase::_UpdateToolTip()
    {
        auto titleRun = WUX::Documents::Run();
        titleRun.Text(_CreateToolTipTitle());

        auto textBlock = WUX::Controls::TextBlock{};
        textBlock.TextAlignment(WUX::TextAlignment::Center);
        textBlock.Inlines().Append(titleRun);

        if (!_keyChord.empty())
        {
            auto keyChordRun = WUX::Documents::Run();
            keyChordRun.Text(_keyChord);
            keyChordRun.FontStyle(winrt::Windows::UI::Text::FontStyle::Italic);
            textBlock.Inlines().Append(WUX::Documents::LineBreak{});
            textBlock.Inlines().Append(keyChordRun);
        }

        WUX::Controls::ToolTip toolTip{};
        toolTip.Content(textBlock);
        WUX::Controls::ToolTipService::SetToolTip(TabViewItem(), toolTip);
    }
}
