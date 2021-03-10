// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "NamedPipeConnection.h"
#include <sstream>

#include "NamedPipeConnection.g.cpp"

#include "../../types/inc/utils.hpp"

namespace winrt::Microsoft::Terminal::TerminalConnection::implementation
{
    NamedPipeConnection::NamedPipeConnection(hstring const& pipeName) noexcept:
        _u8State{},
        _u16Str{},
        _buffer{}
    {
        _pipeHandle = wil::unique_hfile(CreateFileW(
            pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL));
    }

    void NamedPipeConnection::Start() noexcept
    {
        _startTime = std::chrono::high_resolution_clock::now();

        // Create our own output handling thread
        // This must be done after the pipes are populated.
        // Each connection needs to make sure to drain the output from its backing host.
        _hOutputThread.reset(CreateThread(
            nullptr,
            0,
            [](LPVOID lpParameter) noexcept {
                NamedPipeConnection* const pInstance = static_cast<NamedPipeConnection*>(lpParameter);
                if (pInstance)
                {
                    return pInstance->_OutputThread();
                }
                return gsl::narrow_cast<DWORD>(E_INVALIDARG);
            },
            this,
            0,
            nullptr));

        THROW_LAST_ERROR_IF_NULL(_hOutputThread);

    }

    void NamedPipeConnection::WriteInput(hstring const& data)
    {
        std::string str = winrt::to_string(data);
        LOG_IF_WIN32_BOOL_FALSE(WriteFile(_pipeHandle.get(), str.c_str(), (DWORD)str.length(), nullptr, nullptr));
    }

    void NamedPipeConnection::Resize(uint32_t /*rows*/, uint32_t /*columns*/) noexcept
    {
    }

    void NamedPipeConnection::Close() noexcept
    {
        _pipeHandle.reset(); // break the pipes

        if (_hOutputThread)
        {
            // Tear down our output thread -- now that the output pipe was closed on the
            // far side, we can run down our local reader.
            LOG_LAST_ERROR_IF(WAIT_FAILED == WaitForSingleObject(_hOutputThread.get(), INFINITE));
            _hOutputThread.reset();
        }
    }

    DWORD NamedPipeConnection::_OutputThread()
    {
        // Keep us alive until the output thread terminates; the destructor
        // won't wait for us, and the known exit points _do_.
        auto strongThis{ get_strong() };

        // process the data of the output pipe in a loop
        while (true)
        {
            DWORD read{};

            const auto readFail{ !ReadFile(_pipeHandle.get(), _buffer.data(), gsl::narrow_cast<DWORD>(_buffer.size()), &read, nullptr) };
            if (readFail) // reading failed (we must check this first, because read will also be 0.)
            {
                const auto lastError = GetLastError();
                if (lastError != ERROR_BROKEN_PIPE)
                {
                    return gsl::narrow_cast<DWORD>(HRESULT_FROM_WIN32(lastError));
                }
                // else we call convertUTF8ChunkToUTF16 with an empty string_view to convert possible remaining partials to U+FFFD
            }

            // Pass the output to our registered event handlers
            _TerminalOutputHandlers(std::wstring{ _buffer.data(), read / 2 });
        }

        return 0;
    }
}
