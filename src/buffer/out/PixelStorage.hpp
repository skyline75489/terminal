#pragma once

#include <vector>
#include <unordered_map>


struct PixelRegion final {
    til::size size;
    std::unique_ptr<std::vector<std::vector<COLORREF>>> data;
    // The anchored-down size in cell metric.
    // Changing the font results in distorted aspect ration, but
    // this guarantees that the cell coordinates of the pixel region
    // remains invariant.
    float_t cellWidth, cellHeight;
    bool exclusive;

    PixelRegion(std::unique_ptr<std::vector<std::vector<COLORREF>>> inData, const COORD& fontSize, bool exclusive) :
        data(std::move(inData)),
        exclusive(exclusive)
    {
        auto rawData = data.get();
        if (rawData != nullptr)
        {
            size_t height = rawData->size();
            size_t width = 0;
            if (height > 0)
            {
                width = rawData->at(0).size();
            }

            size = til::size(width, height);
            THROW_HR_IF(E_ABORT, !base::CheckDiv(size.width<float>(), static_cast<float_t>(fontSize.X)).AssignIfValid(&cellWidth));
            THROW_HR_IF(E_ABORT, !base::CheckDiv(size.height<float>(), static_cast<float_t>(fontSize.Y)).AssignIfValid(&cellHeight));
        }
    }

    // Get the rounded-up cell region
    COORD RoundCellRegion()
    {
        return { gsl::narrow_cast<short>(ceil(cellWidth)), gsl::narrow_cast<short>(ceil(cellHeight)) };
    }
};

class PixelStorage final
{
public:

    using key_type = typename COORD;
    using mapped_type = typename std::unique_ptr<PixelRegion>;

    PixelStorage() noexcept;

    const mapped_type& GetData(const key_type key) const;

    void StoreData(const key_type key, mapped_type data);

    const bool HasData(const key_type key) const;

    auto begin() const { return _map.begin(); }

    auto end() const { return _map.end(); }

    void Erase(const key_type key) noexcept;

private:
    std::unordered_map<key_type, mapped_type> _map;
};
