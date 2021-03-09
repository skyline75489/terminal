// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "NamedPipeConnection.g.h"

#include "../cascadia/inc/cppwinrt_utils.h"

namespace winrt::Microsoft::Terminal::TerminalConnection::implementation
{
    struct NamedPipeConnection : NamedPipeConnectionT<NamedPipeConnection>
    {
        NamedPipeConnection(hstring const& pipeName) noexcept;

        void Start() noexcept;
        void WriteInput(hstring const& data);
        void Resize(uint32_t rows, uint32_t columns) noexcept;
        void Close() noexcept;

        ConnectionState State() const noexcept { return ConnectionState::Connected; }

        WINRT_CALLBACK(TerminalOutput, TerminalOutputHandler);
        TYPED_EVENT(StateChanged, ITerminalConnection, IInspectable);


    private:
        std::chrono::high_resolution_clock::time_point _startTime{};

        wil::unique_handle _hOutputThread;
        wil::unique_hfile _pipeHandle;

        til::u8state _u8State;
        std::wstring _u16Str;
        std::array<char, 4096> _buffer;
        DWORD _OutputThread();
    };
}

namespace winrt::Microsoft::Terminal::TerminalConnection::factory_implementation
{
    struct NamedPipeConnection : NamedPipeConnectionT<NamedPipeConnection, implementation::NamedPipeConnection>
    {
    };
}
