// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
Module Name:
- TmuxEngine.hpp

*/
#pragma once

#include "../adapter/DispatchTypes.hpp"

namespace Microsoft::Console::VirtualTerminal
{
    class TmuxEngine
    {
    public:
        void ProcessString(const std::wstring_view string);
        void ProcessCharacter(const wchar_t wch);

        void SetPaneOutputCallback(std::function<void(size_t paneId, wchar_t wch)> pfnPaneOutputCallback);

    private:
        const VTID TMUX_TOKEN_INDICATOR = VTID("%");
        const VTID TMUX_TOKEN_SPACE = VTID(" ");
        const VTID TMUX_TOKEN_ESCAPE = VTID("\\");
        const VTID TMUX_TOKEN_CR = VTID("\r");
        const VTID TMUX_TOKEN_LF = VTID("\n");

        const std::wstring_view TMUX_OUTPUT_BEGIN = L"begin";
        const std::wstring_view TMUX_OUTPUT_END = L"end";
        const std::wstring_view TMUX_OUTPUT_ERROR = L"error";
        const std::wstring_view TMUX_OUTPUT_PANE = L"output";
        
        enum class TmuxStates
        {
            Ground,
            Token,
            OutputBegin,
            Output,
            PaneOutput,
            OutputEnd,
            Notification,
            Error
        };

        void _ActionToken(const wchar_t wch) noexcept;
        void _ActionOutput(const wchar_t wch) noexcept;
        void _ActionError(const wchar_t wch) noexcept;

        void _EnterGround() noexcept;
        void _EnterToken() noexcept;
        void _EnterOutputBegin() noexcept;
        void _EnterOutput() noexcept;
        void _EnterPaneOutput() noexcept;
        void _EnterOutputEnd() noexcept;
        void _EnterNotification() noexcept;
        void _EnterError() noexcept;

        void _EventGround(const wchar_t wch) noexcept;
        void _EventToken(const wchar_t wch) noexcept;
        void _EventOutputBegin(const wchar_t wch) noexcept;
        void _EventOutput(const wchar_t wch) noexcept;
        void _EventPaneOutput(const wchar_t wch) noexcept;
        void _EventOutputEnd(const wchar_t wch) noexcept;
        void _EventNotification(const wchar_t wch) noexcept;
        void _EventError(const wchar_t wch) noexcept;

        void _DispatchNotification(const std::wstring_view notification, const std::wstring_view param);
        void _DispatchPaneOutput(const size_t paneId, const wchar_t c);

        void _AccumulateTo(const wchar_t wch, size_t& value) noexcept;
        void _AccumulateToOct(const wchar_t wch, size_t& value) noexcept;

        std::function<void(size_t paneId, wchar_t wch)> _pfnPaneOutputCallback;

        std::wstring _command;
        std::wstring _notification;
        std::wstring _notificationParam;
        std::wstring _outputLine;

        size_t _paneId;
        bool _paneEscape;
        size_t _paneEscapeOrd;
        bool _paneIdReady;
        std::wstring _paneLine;

        std::wstring _token;
        TmuxStates _state = TmuxStates::Ground;
    };
}
