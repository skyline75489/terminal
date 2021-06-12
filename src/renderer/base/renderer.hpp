/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- Renderer.hpp

Abstract:
- This is the definition of our renderer.
- It provides interfaces for the console application to notify when various portions of the console state have changed and need to be redrawn.
- It requires a data interface to fetch relevant console structures required for drawing and a drawing engine target for output.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "../inc/IRenderer.hpp"
#include "../inc/IRenderEngine.hpp"
#include "../inc/IRenderData.hpp"

#include "thread.hpp"

#include "../../buffer/out/textBuffer.hpp"
#include "../../buffer/out/CharRow.hpp"

#include <chrono>

namespace til
{
    namespace details
    {
        template<typename... Args>
        class throttled_func_storage
        {
        public:
            template<typename... MakeArgs>
            bool emplace(MakeArgs&&... args)
            {
                std::scoped_lock guard{ _lock };

                const bool hadValue = _pendingRunArgs.has_value();
                _pendingRunArgs.emplace(std::forward<MakeArgs>(args)...);
                return hadValue;
            }

            template<typename F>
            void modify_pending(F f)
            {
                std::scoped_lock guard{ _lock };

                if (_pendingRunArgs.has_value())
                {
                    std::apply(f, _pendingRunArgs.value());
                }
            }

            std::tuple<Args...> extract()
            {
                decltype(_pendingRunArgs) args;
                std::scoped_lock guard{ _lock };
                _pendingRunArgs.swap(args);
                return args.value();
            }

        private:
            // std::mutex uses imperfect Critical Sections on Windows.
            // --> std::shared_mutex uses SRW locks that are small and fast.
            std::shared_mutex _lock;
            std::optional<std::tuple<Args...>> _pendingRunArgs;
        };

        template<>
        class throttled_func_storage<>
        {
        public:
            bool emplace()
            {
                return _isPending.exchange(true, std::memory_order_relaxed);
            }

            std::tuple<> extract()
            {
                reset();
                return {};
            }

            void reset()
            {
                _isPending.store(false, std::memory_order_relaxed);
            }

        private:
            std::atomic<bool> _isPending;
        };
    } // namespace details

    // Class Description:
    // - Represents a function that takes arguments and whose invocation is
    //   delayed by a specified duration and rate-limited such that if the code
    //   tries to run the function while a call to the function is already
    //   pending, then the previous call with the previous arguments will be
    //   cancelled and the call will be made with the new arguments instead.
    // - The function will be run on the the specified dispatcher.
    template<bool leading, typename... Args>
    class throttled_func
    {
    public:
        using Func = std::function<void(Args...)>;

        throttled_func(std::chrono::duration<int64_t, std::ratio<1, 10000000>> delay, Func func) :
            _delay{ -delay.count() },
            _func{ std::move(func) },
            _timer{ CreateThreadpoolTimer(&_timer_callback, this, nullptr) }
        {
            if (_delay >= 0)
            {
                throw std::invalid_argument("non-positive delay specified");
            }
        }

        // throttled_func uses its `this` pointer when creating _timer.
        // Since the timer cannot be recreated, instances cannot be moved either.
        throttled_func(const throttled_func&) = delete;
        throttled_func& operator=(const throttled_func&) = delete;
        throttled_func(throttled_func&&) = delete;
        throttled_func& operator=(throttled_func&&) = delete;

        template<typename... MakeArgs>
        void Run(MakeArgs&&... args)
        {
            if (!_storage.emplace(std::forward<MakeArgs>(args)...))
            {
                _fire();
            }
        }

        // Method Description:
        // - Runs the function later with the specified arguments, except if it
        //   is called again before with new arguments, in which case the new
        //   arguments will be used instead.
        // - For more information, read the class' documentation.
        // - This method is always thread-safe. It can be called multiple times on
        //   different threads.
        // Arguments:
        // - args: the arguments to pass to the function
        // Return Value:
        // - <none>
        template<typename... MakeArgs>
        void operator()(MakeArgs&&... args)
        {
            if (!_storage.emplace(std::forward<MakeArgs>(args)...))
            {
                _fire();
            }
        }

        // Method Description:
        // - Modifies the pending arguments for the next function invocation, if
        //   there is one pending currently.
        // - Let's say that you just called the `operator()` method with some arguments.
        //   After the delay specified in the constructor, the function specified
        //   in the constructor will be called with these arguments.
        // - By using this method, you can modify the arguments before the function
        //   is called.
        // - You pass a function to this method which will take references to
        //   the arguments (one argument corresponds to one reference to an
        //   argument) and will modify them.
        // - When there is no pending invocation of the function, this method will
        //   not do anything.
        // - This method is always thread-safe. It can be called multiple times on
        //   different threads.
        // Arguments:
        // - f: the function to call with references to the arguments
        // Return Value:
        // - <none>
        template<typename F>
        void modify_pending(F f)
        {
            _storage.modify_pending(f);
        }

        // Method Description:
        // - Makes sure that all outstanding timers are canceled and
        //   in-progress ones are awaited on for their completion.
        // - Reason for its existence: We have code that needs to explicitly
        //   ensure that the throttled_func will not call the callback anymore.
        void wait_for_completion()
        {
            WaitForThreadpoolTimerCallbacks(_timer, true);
        }

    private:
        void _fire()
        {
            if constexpr (leading)
            {
                self._func();
            }

            SetThreadpoolTimerEx(_timer, reinterpret_cast<PFILETIME>(&_delay), 0, 0);
        }

        static void _timer_callback(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context, PTP_TIMER /*timer*/) noexcept
        try
        {
            auto& self = *static_cast<throttled_func*>(context);

            if constexpr (leading)
            {
                self._storage.reset();
            }
            else
            {
                std::apply(self._func, self._storage.extract());
            }
        }
        CATCH_LOG()

        int64_t _delay;
        Func _func;
        PTP_TIMER _timer;

        details::throttled_func_storage<Args...> _storage;
    };

    template<typename... Args>
    using throttled_func_trailing = throttled_func<false, Args...>;
    using throttled_func_leading = throttled_func<true>;
} // namespace til

namespace Microsoft::Console::Render
{
    class Renderer sealed : public IRenderer
    {
    public:
        Renderer(IRenderData* pData,
                 _In_reads_(cEngines) IRenderEngine** const pEngine,
                 const size_t cEngines,
                 std::unique_ptr<IRenderThread> thread);

        [[nodiscard]] static HRESULT s_CreateInstance(IRenderData* pData,
                                                      _In_reads_(cEngines) IRenderEngine** const rgpEngines,
                                                      const size_t cEngines,
                                                      _Outptr_result_nullonfailure_ Renderer** const ppRenderer);

        [[nodiscard]] static HRESULT s_CreateInstance(IRenderData* pData,
                                                      _Outptr_result_nullonfailure_ Renderer** const ppRenderer);

        virtual ~Renderer() override;

        [[nodiscard]] HRESULT PaintFrame();

        void TriggerSystemRedraw(const RECT* const prcDirtyClient) override;
        void TriggerRedraw(const Microsoft::Console::Types::Viewport& region) override;
        void TriggerRedraw(const COORD* const pcoord) override;
        void TriggerRedrawCursor(const COORD* const pcoord) override;
        void TriggerRedrawAll() override;
        void TriggerTeardown() noexcept override;

        void TriggerSelection() override;
        void TriggerScroll() override;
        void TriggerScroll(const COORD* const pcoordDelta) override;

        void TriggerCircling() override;
        void TriggerTitleChange() override;

        void TriggerFontChange(const int iDpi,
                               const FontInfoDesired& FontInfoDesired,
                               _Out_ FontInfo& FontInfo) override;

        [[nodiscard]] HRESULT GetProposedFont(const int iDpi,
                                              const FontInfoDesired& FontInfoDesired,
                                              _Out_ FontInfo& FontInfo) override;

        bool IsGlyphWideByFont(const std::wstring_view glyph) override;

        void EnablePainting() override;
        void WaitForPaintCompletionAndDisable(const DWORD dwTimeoutMs) override;
        void WaitUntilCanRender() override;

        void AddRenderEngine(_In_ IRenderEngine* const pEngine) override;

        void SetRendererEnteredErrorStateCallback(std::function<void()> pfn);
        void ResetErrorStateAndResume();

        void UpdateLastHoveredInterval(const std::optional<interval_tree::IntervalTree<til::point, size_t>::interval>& newInterval);

    private:
        std::deque<IRenderEngine*> _rgpEngines;

        IRenderData* _pData; // Non-ownership pointer

        std::unique_ptr<IRenderThread> _pThread;
        bool _destructing = false;

        std::optional<interval_tree::IntervalTree<til::point, size_t>::interval> _hoveredInterval;

        void _NotifyPaintFrame();

        [[nodiscard]] HRESULT _PaintFrameForEngine(_In_ IRenderEngine* const pEngine) noexcept;

        bool _CheckViewportAndScroll();

        [[nodiscard]] HRESULT _PaintBackground(_In_ IRenderEngine* const pEngine);

        void _PaintBufferOutput(_In_ IRenderEngine* const pEngine);

        void _PaintBufferOutputHelper(_In_ IRenderEngine* const pEngine,
                                      TextBufferCellIterator it,
                                      const COORD target,
                                      const bool lineWrapped);

        static IRenderEngine::GridLines s_GetGridlines(const TextAttribute& textAttribute) noexcept;

        void _PaintBufferOutputGridLineHelper(_In_ IRenderEngine* const pEngine,
                                              const TextAttribute textAttribute,
                                              const size_t cchLine,
                                              const COORD coordTarget);

        void _PaintSelection(_In_ IRenderEngine* const pEngine);
        void _PaintCursor(_In_ IRenderEngine* const pEngine);

        void _PaintOverlays(_In_ IRenderEngine* const pEngine);
        void _PaintOverlay(IRenderEngine& engine, const RenderOverlay& overlay);

        [[nodiscard]] HRESULT _UpdateDrawingBrushes(_In_ IRenderEngine* const pEngine, const TextAttribute attr, const bool isSettingDefaultBrushes);

        [[nodiscard]] HRESULT _PerformScrolling(_In_ IRenderEngine* const pEngine);

        Microsoft::Console::Types::Viewport _viewport;

        static constexpr float _shrinkThreshold = 0.8f;
        std::vector<Cluster> _clusterBuffer;

        std::vector<SMALL_RECT> _GetSelectionRects() const;
        void _ScrollPreviousSelection(const til::point delta);
        std::vector<SMALL_RECT> _previousSelection;

        [[nodiscard]] HRESULT _PaintTitle(IRenderEngine* const pEngine);

        [[nodiscard]] std::optional<CursorOptions> _GetCursorInfo();
        [[nodiscard]] HRESULT _PrepareRenderInfo(_In_ IRenderEngine* const pEngine);

        // Helper functions to diagnose issues with painting and layout.
        // These are only actually effective/on in Debug builds when the flag is set using an attached debugger.
        bool _fDebug = false;

        std::shared_ptr<til::throttled_func_trailing<COORD>> _cursorThrottleFunc;
        std::function<void()> _pfnRendererEnteredErrorState;

#ifdef UNIT_TESTING
        friend class ConptyOutputTests;
#endif
    };
}
