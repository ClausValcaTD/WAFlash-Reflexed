// Copyright 2025 WAFlash-ReFlexed Authors
// Apache License 2.0

#include "movie_clip.hpp"
#include "../swf/swf_loader.hpp"
#include <cstdio>
#include <fstream>

namespace waflash {

MovieClip::MovieClip(const std::string& name, int depth)
    : m_name(name), m_depth(depth)
    , m_current_frame(1), m_total_frames(1)
    , m_playing(false)
    , m_load_state(LoadState::IDLE)
    , m_bytes_loaded(0), m_bytes_total(0)
{}

MovieClip::~MovieClip() {}

bool MovieClip::loadMovie(const std::string& url) {
    printf("[MovieClip:%s] loadMovie('%s')\n", m_name.c_str(), url.c_str());
    m_pending_url   = url;
    m_load_state    = LoadState::LOADING;
    m_bytes_loaded  = 0;
    m_current_frame = 1;
    m_playing       = false; // stg_N.swf starts stopped (frame 1 has stop())

    // Get file size for getBytesTotal() — available immediately
    std::ifstream f(url, std::ios::binary | std::ios::ate);
    if (f.is_open()) {
        m_bytes_total = static_cast<uint32_t>(f.tellg());
        printf("[MovieClip:%s] bytes_total=%u\n", m_name.c_str(), m_bytes_total);
    } else {
        m_bytes_total = 0;
    }
    return true;
}

void MovieClip::processLoad() {
    if (m_load_state != LoadState::LOADING) return;

    SWFLoader loader;
    if (loader.load(m_pending_url)) {
        m_bytes_loaded  = m_bytes_total;
        m_total_frames  = loader.getHeader().frame_count;
        m_load_state    = LoadState::COMPLETE;
        printf("[MovieClip:%s] COMPLETE — %d frames\n",
               m_name.c_str(), m_total_frames);
        if (onLoad) onLoad();
    } else {
        m_load_state = LoadState::FAILED;
        printf("[MovieClip:%s] FAILED\n", m_name.c_str());
    }
}

void MovieClip::advanceFrame() {
    if (!m_playing) return;
    if (m_load_state != LoadState::COMPLETE) return;
    if (m_current_frame < m_total_frames) {
        m_current_frame++;
        printf("[MovieClip:%s] → frame %d/%d\n",
               m_name.c_str(), m_current_frame, m_total_frames);
    }
}

void MovieClip::tick() {
    // ═══════════════════════════════════════════════════
    // CRITICAL EVENT ORDERING — matches Flash Player exactly
    // (Ruffle gets this wrong → Hoshi Saga broken since 2021)
    // ═══════════════════════════════════════════════════
    // 1. Process pending loads FIRST
    processLoad();
    // 2. Advance timeline
    advanceFrame();
    // 3. Fire onEnterFrame LAST
    if (onEnterFrame) onEnterFrame();
}

void MovieClip::play() {
    m_playing = true;
    printf("[MovieClip:%s] play() @ frame %d\n",
           m_name.c_str(), m_current_frame);
}

void MovieClip::stop()  { m_playing = false; }

void MovieClip::gotoAndPlay(int frame) {
    gotoAndStop(frame);
    play();
}

void MovieClip::gotoAndStop(int frame) {
    if (frame >= 1 && frame <= m_total_frames)
        m_current_frame = frame;
}

uint32_t MovieClip::getBytesLoaded() const { return m_bytes_loaded; }
uint32_t MovieClip::getBytesTotal()  const { return m_bytes_total;  }
int MovieClip::getCurrentFrame()     const { return m_current_frame; }
int MovieClip::getTotalFrames()      const { return m_total_frames;  }

} // namespace waflash
