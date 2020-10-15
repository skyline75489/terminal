// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "PixelStorage.hpp"

PixelStorage::PixelStorage() noexcept :
    _map{}
{
}

// Routine Description:
// - fetches the pixel associated with key
// Arguments:
// - key - the key into the storage
// Return Value:
// - the pixel data associated with key
// Note: will throw exception if key is not stored yet
const PixelStorage::mapped_type& PixelStorage::GetData(const key_type key) const
{
    return _map.at(key);
}

// Routine Description:
// - stores pixel data associated with key.
// Arguments:
// - key - the key into the storage
// - glyph - the glyph data to store
void PixelStorage::StoreData(const key_type key, mapped_type data)
{
    _map.insert_or_assign(key, std::move(data));
}

const bool PixelStorage::HasData(const key_type key) const
{
    return _map.find(key) != _map.end();
}

// Routine Description:
// - erases key and its associated data from the storage
// Arguments:
// - key - the key to remove
void PixelStorage::Erase(const key_type key) noexcept
{
    _map.erase(key);
}

void PixelStorage::Scroll(const SHORT firstRow, const SHORT size, const SHORT delta)
{
    WORD lastRow = firstRow + size;
    std::vector<COORD> inRangeCoords;
    std::vector<COORD> outRangeCoords;
    SHORT outRangeDelta;

    auto sort = [=](std::vector<COORD>& vec)
    {
        if (delta > 0)
        {
            std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) { return lhs.Y > rhs.Y; });
        }
        else
        {
            std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) { return lhs.Y < rhs.Y; });
        }
    };

    for (const auto& [origin, region] : _map)
    {
        if (firstRow <= origin.Y && origin.Y < lastRow)
        {
            inRangeCoords.push_back(origin);
        }
        else if ( (lastRow <= origin.Y && origin.Y < delta + lastRow)
                  || (firstRow + delta <= origin.Y && origin.Y < firstRow))
        {
            outRangeCoords.push_back(origin);
        }
    }

    sort(inRangeCoords);
    sort(outRangeCoords);

    outRangeDelta = gsl::narrow_cast<SHORT>(inRangeCoords.size());
    if (delta > 0)
    {
        outRangeDelta = -outRangeDelta;
    }

    for (const auto& origin : inRangeCoords)
    {
        auto node = _map.extract(origin);
        node.key().Y += delta;
        if (node.key().Y >= 0)
        {
            _map.insert(std::move(node));
        }
    }

    for (const auto& origin : outRangeCoords)
    {
        auto node = _map.extract(origin);
        node.key().Y += outRangeDelta;
        if (node.key().Y >= 0)
        {
            _map.insert(std::move(node));
        }
    }
}
