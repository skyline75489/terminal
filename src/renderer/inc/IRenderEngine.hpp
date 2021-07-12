/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- IRenderEngine.hpp

Abstract:
- This serves as the entry point for a specific graphics engine specific renderer.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "CursorOptions.h"
#include "Cluster.hpp"
#include "FontInfoDesired.hpp"
#include "IRenderData.hpp"
#include "../../buffer/out/LineRendition.hpp"

namespace Microsoft::Console::Render
{
    struct RenderFrameInfo
    {
        std::optional<CursorOptions> cursorInfo;
    };

    enum class IntereactionType
    {
        Selection
    };

    class IRenderEngine
    {
    public:
        enum GridLines
        {
            None = 0x0,
            Top = 0x1,
            Bottom = 0x2,
            Left = 0x4,
            Right = 0x8,
            Underline = 0x10,
            DoubleUnderline = 0x20,
            Strikethrough = 0x40,
            HyperlinkUnderline = 0x80
        };

        virtual ~IRenderEngine() = 0;

    protected:
        IRenderEngine() = default;
        IRenderEngine(const IRenderEngine&) = default;
        IRenderEngine(IRenderEngine&&) = default;
        IRenderEngine& operator=(const IRenderEngine&) = default;
        IRenderEngine& operator=(IRenderEngine&&) = default;

    public:
        [[nodiscard]] virtual HRESULT StartPaint() noexcept = 0;
        [[nodiscard]] virtual HRESULT PaintFrame(IRenderData *pData) noexcept = 0;

        [[nodiscard]] virtual HRESULT EndPaint() noexcept = 0;

        [[nodiscard]] virtual bool RequiresContinuousRedraw() noexcept = 0;
        virtual void WaitUntilCanRender() noexcept = 0;
        [[nodiscard]] virtual HRESULT Present() noexcept = 0;

        [[nodiscard]] virtual HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) noexcept = 0;

        [[nodiscard]] virtual HRESULT InvalidateIntereaction(IRenderData* pData, IntereactionType type) noexcept = 0;

        [[nodiscard]] virtual HRESULT Invalidate(const SMALL_RECT* const psrRegion) noexcept = 0;
        [[nodiscard]] virtual HRESULT InvalidateCursor(const SMALL_RECT* const psrRegion) noexcept = 0;
        [[nodiscard]] virtual HRESULT InvalidateSystem(const RECT* const prcDirtyClient) noexcept = 0;
        [[nodiscard]] virtual HRESULT InvalidateScroll(const COORD* const pcoordDelta) noexcept = 0;
        [[nodiscard]] virtual HRESULT InvalidateAll() noexcept = 0;
        [[nodiscard]] virtual HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) noexcept = 0;

        [[nodiscard]] virtual HRESULT InvalidateTitle(const std::wstring_view proposedTitle) noexcept = 0;

        [[nodiscard]] virtual HRESULT UpdateFont(const FontInfoDesired& FontInfoDesired,
                                                 _Out_ FontInfo& FontInfo) noexcept = 0;
        [[nodiscard]] virtual HRESULT UpdateDpi(const int iDpi) noexcept = 0;
        [[nodiscard]] virtual HRESULT UpdateViewport(const SMALL_RECT srNewViewport) noexcept = 0;

        [[nodiscard]] virtual HRESULT GetProposedFont(const FontInfoDesired& FontInfoDesired,
                                                      _Out_ FontInfo& FontInfo,
                                                      const int iDpi) noexcept = 0;

        [[nodiscard]] virtual HRESULT GetDirtyArea(gsl::span<const til::rectangle>& area) noexcept = 0;
        [[nodiscard]] virtual HRESULT GetFontSize(_Out_ COORD* const pFontSize) noexcept = 0;
        [[nodiscard]] virtual HRESULT IsGlyphWideByFont(const std::wstring_view glyph, _Out_ bool* const pResult) noexcept = 0;
    };

    inline Microsoft::Console::Render::IRenderEngine::~IRenderEngine() {}
}

DEFINE_ENUM_FLAG_OPERATORS(Microsoft::Console::Render::IRenderEngine::GridLines)
