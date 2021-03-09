// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"

#include "TmuxEngine.hpp"

using namespace Microsoft::Console;
using namespace Microsoft::Console::VirtualTerminal;

static constexpr bool _isNumericValue(const wchar_t wch) noexcept
{
    return wch >= L'0' && wch <= L'9'; // 0x30 - 0x39
}

void TmuxEngine::ProcessString(const std::wstring_view string)
{
    for (const auto c : string)
    {
        ProcessCharacter(c);
    }
}

void TmuxEngine::ProcessCharacter(const wchar_t wch)
{
    switch (_state)
    {
    case TmuxStates::Ground:
        return _EventGround(wch);
    case TmuxStates::Token:
        return _EventToken(wch);
    case TmuxStates::OutputBegin:
        return _EventOutputBegin(wch);
    case TmuxStates::Output:
        return _EventOutput(wch);
    case TmuxStates::PaneOutput:
        return _EventPaneOutput(wch);
    case TmuxStates::OutputEnd:
        return _EventOutputEnd(wch);
    case TmuxStates::Notification:
        return _EventNotification(wch);
    default:
        break;
    }
}

void TmuxEngine::SetPaneOutputCallback(std::function<void(size_t paneId, wchar_t wch)> pfnPaneOutputCallback)
{
    _pfnPaneOutputCallback = pfnPaneOutputCallback;
}

void TmuxEngine::_ActionToken(const wchar_t wch) noexcept
{
    _token += wch;
}

void TmuxEngine::_ActionOutput(const wchar_t wch) noexcept
{
    _outputLine += wch;
}

void TmuxEngine::_ActionError(const wchar_t /*wch*/) noexcept
{

}

void TmuxEngine::_EnterGround() noexcept
{
    _command.clear();
    _token.clear();
    _paneId = 0;
    _paneIdReady = false;
    _outputLine.clear();
    _notification.clear();
    _notificationParam.clear();
    _state = TmuxStates::Ground;
}

void TmuxEngine::_EnterToken() noexcept
{
    _token.clear();
    _state = TmuxStates::Token;
}

void TmuxEngine::_EnterOutputBegin() noexcept
{
    _command.clear();
    _token.clear();
    _state = TmuxStates::OutputBegin;
}

void TmuxEngine::_EnterOutput() noexcept
{
    _outputLine.clear();
    _token.clear();
    _state = TmuxStates::Output;
}

void TmuxEngine::_EnterPaneOutput() noexcept
{
    _paneId = 0;
    _paneIdReady = false;
    _paneLine.clear();
    _token.clear();
    _state = TmuxStates::PaneOutput;
}

void TmuxEngine::_EnterOutputEnd() noexcept
{
    _token.clear();
    _state = TmuxStates::OutputEnd;
}

void TmuxEngine::_EnterNotification() noexcept
{
    _state = TmuxStates::Notification;
}

void TmuxEngine::_EnterError() noexcept
{
    _state = TmuxStates::Error;
}

void TmuxEngine::_EventGround(const wchar_t wch) noexcept
{
    if (!_command.empty())
    {
        _EnterOutput();
    }
    else if (wch == TMUX_TOKEN_INDICATOR)
    {
        _EnterToken();
    }
}

void TmuxEngine::_EventToken(const wchar_t wch) noexcept
{
    if (wch == TMUX_TOKEN_SPACE)
    {
        if (_token == TMUX_OUTPUT_BEGIN)
        {
            _EnterOutputBegin();
        }
        else if (_token == TMUX_OUTPUT_END)
        {
            _EnterOutputEnd();
        }
        else if (_token == TMUX_OUTPUT_PANE)
        {
            _EnterPaneOutput();
        }
        else
        {
            _notification = _token;
            _EnterNotification();
        }
    }
    else if (wch == TMUX_TOKEN_LF)
    {
        _EnterGround();
    }
    else if (wch == TMUX_TOKEN_INDICATOR)
    {
        _EnterError();
    }
    else
    {
        _ActionToken(wch);
    }
}

void TmuxEngine::_EventOutputBegin(const wchar_t wch) noexcept
{
    if (wch == TMUX_TOKEN_CR)
    {
        // Ignore
    }
    else if (wch == TMUX_TOKEN_LF)
    {
        _EnterOutput();
    }
    else
    {
        _command += wch;
    }
}

void TmuxEngine::_EventOutput(const wchar_t wch) noexcept
{
    if (wch == TMUX_TOKEN_INDICATOR)
    {
        _EnterToken();
    }
    else if (wch == TMUX_TOKEN_LF)
    {
        _EnterGround();
    }
    else
    {
        _ActionOutput(wch);
    }
}

void TmuxEngine::_EventPaneOutput(const wchar_t wch) noexcept
{
    if (wch == TMUX_TOKEN_INDICATOR)
    {
        // Begin of pane id.
    }
    else if (_isNumericValue(wch))
    {
        if (!_paneIdReady)
        {
            _AccumulateTo(wch, _paneId);
        }
        else if (_paneEscape)
        {
            _AccumulateToOct(wch, _paneEscapeOrd);
        }
        else
        {
            _DispatchPaneOutput(_paneId, wch);
        }
    }
    else if (! _paneIdReady && wch == TMUX_TOKEN_SPACE)
    {
        // End of pane id.
        _paneIdReady = true;
    }
    else if (wch == TMUX_TOKEN_ESCAPE)
    {
        // Begin of escape
        _paneEscapeOrd = 0;
        _paneEscape = true;
    }
    else
    {
        if (_paneEscape)
        {
            _DispatchPaneOutput(_paneId, static_cast<wchar_t>(_paneEscapeOrd));
            _paneEscape = false;
        }

        if (wch == TMUX_TOKEN_CR)
        {
            // End of pane output.
            _EnterGround();
        }
        else
        {
            _DispatchPaneOutput(_paneId, wch);
        }

    }
}

void TmuxEngine::_EventOutputEnd(const wchar_t wch) noexcept
{
    if (wch == TMUX_TOKEN_LF)
    {
        _EnterGround();
    }
}

void TmuxEngine::_EventNotification(const wchar_t wch) noexcept
{
    if (wch == TMUX_TOKEN_CR)
    {
        // Ignore
    }
    else if (wch == TMUX_TOKEN_LF)
    {
        _DispatchNotification(_notification, _notificationParam);
        _EnterGround();
    }
    else
    {
        _notificationParam += wch;
    }
}

void TmuxEngine::_EventError(const wchar_t /*wch*/) noexcept
{

}

void TmuxEngine::_DispatchNotification(const std::wstring_view notification, const std::wstring_view param)
{
    std::wcout << notification << param;
}

void TmuxEngine::_DispatchPaneOutput(const size_t paneId, const wchar_t c)
{
    if (_pfnPaneOutputCallback)
    {
        _pfnPaneOutputCallback(paneId, c);
    }
}

void TmuxEngine::_AccumulateTo(const wchar_t wch, size_t& value) noexcept
{
    const size_t digit = wch - L'0';
    value = value * 10 + digit;
}

void TmuxEngine::_AccumulateToOct(const wchar_t wch, size_t& value) noexcept
{
    const size_t digit = wch - L'0';
    value = value * 8 + digit;
}
