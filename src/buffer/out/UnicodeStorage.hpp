/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- UnicodeStorage.hpp

Abstract:
- dynamic storage location for glyphs that can't normally fit in the output buffer

Author(s):
- Austin Diviness (AustDi) 02-May-2018
--*/

#pragma once

#include <vector>
#include <unordered_map>
#include <climits>

class UnicodeStorage final
{
public:
    using key_type = typename COORD;
    using mapped_type = typename std::vector<wchar_t>;

    UnicodeStorage() noexcept;

    const mapped_type& GetText(const key_type key) const;

    void StoreGlyph(const key_type key, const mapped_type& glyph);

    void Erase(const key_type key) noexcept;

    void Remap(const std::unordered_map<SHORT, SHORT>& rowMap, const std::optional<SHORT> width);

private:
    std::unordered_map<key_type, mapped_type> _map;

#ifdef UNIT_TESTING
    friend class UnicodeStorageTests;
    friend class TextBufferTests;
#endif
};
