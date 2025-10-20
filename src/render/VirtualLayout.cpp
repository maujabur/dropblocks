#include "render/VirtualLayout.hpp"
#include <algorithm>

VirtualLayout::VirtualLayout()
    : virtualW_(1920)
    , virtualH_(1080)
    , physicalW_(0)
    , physicalH_(0)
    , scaleX_(1.0f)
    , scaleY_(1.0f)
    , offsetX_(0)
    , offsetY_(0)
    , customOffsetX_(0)
    , customOffsetY_(0)
    , mode_(ScaleMode::AUTO)
{
}

void VirtualLayout::configure(int virtualWidth, int virtualHeight, ScaleMode mode) {
    virtualW_ = std::max(1, virtualWidth);
    virtualH_ = std::max(1, virtualHeight);
    mode_ = mode;
}

void VirtualLayout::setCustomOffsets(int offsetX, int offsetY) {
    customOffsetX_ = offsetX;
    customOffsetY_ = offsetY;
}

void VirtualLayout::calculate(int screenW, int screenH) {
    physicalW_ = std::max(1, screenW);
    physicalH_ = std::max(1, screenH);
    
    if (mode_ == ScaleMode::AUTO) {
        // Uniform scale - maintain aspect ratio, add black bars if needed
        float scaleW = (float)physicalW_ / (float)virtualW_;
        float scaleH = (float)physicalH_ / (float)virtualH_;
        
        // Use the smaller scale to ensure everything fits
        float scale = std::min(scaleW, scaleH);
        scaleX_ = scale;
        scaleY_ = scale;
        
        // Calculate actual rendered size
        int renderedW = (int)(virtualW_ * scale);
        int renderedH = (int)(virtualH_ * scale);
        
        // Center with black bars
        offsetX_ = (physicalW_ - renderedW) / 2;
        offsetY_ = (physicalH_ - renderedH) / 2;
        
    } else if (mode_ == ScaleMode::STRETCH) {
        // STRETCH mode - independent X/Y scale (may distort)
        scaleX_ = (float)physicalW_ / (float)virtualW_;
        scaleY_ = (float)physicalH_ / (float)virtualH_;
        offsetX_ = 0;
        offsetY_ = 0;
        
    } else {
        // NATIVE mode - 1:1, no transformation, uses custom offsets from .cfg
        scaleX_ = 1.0f;
        scaleY_ = 1.0f;
        offsetX_ = customOffsetX_;
        offsetY_ = customOffsetY_;
    }
}

SDL_Rect VirtualLayout::toPhysical(int vx, int vy, int vw, int vh) const {
    SDL_Rect rect;
    rect.x = toPhysicalX(vx);
    rect.y = toPhysicalY(vy);
    rect.w = toPhysicalW(vw);
    rect.h = toPhysicalH(vh);
    return rect;
}

int VirtualLayout::toPhysicalX(int vx) const {
    return offsetX_ + (int)(vx * scaleX_);
}

int VirtualLayout::toPhysicalY(int vy) const {
    return offsetY_ + (int)(vy * scaleY_);
}

int VirtualLayout::toPhysicalW(int vw) const {
    return (int)(vw * scaleX_);
}

int VirtualLayout::toPhysicalH(int vh) const {
    return (int)(vh * scaleY_);
}

