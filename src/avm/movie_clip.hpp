// Copyright 2025 WAFlash-ReFlexed Authors
// Apache License 2.0

#pragma once
#include <string>
#include <functional>
#include <cstdint>
#include <vector>

namespace waflash {

enum class LoadState { IDLE, LOADING, COMPLETE, FAILED };

class MovieClip {
public:
    MovieClip(const std::string& name, int depth);
    ~MovieClip();

    // AS2: loadMovie("stg_1.swf")
    bool loadMovie(const std::string& url);

    // AS2: getBytesLoaded() / getBytesTotal()
    uint32_t getBytesLoaded() const;
    uint32_t getBytesTotal()  const;

    // AS2: play() / stop() / gotoAndPlay() / gotoAndStop()
    void play();
    void stop();
    void gotoAndPlay(int frame);
    void gotoAndStop(int frame);

    // Frame info
    int getCurrentFrame() const;
    int getTotalFrames()  const;

    // CRITICAL: called by engine_tick()
    // Order MUST be:
    //   1. processLoad()    ← update bytes, complete loading
    //   2. advanceFrame()   ← move timeline if playing
    //   3. onEnterFrame()   ← fire AS2 callback LAST
    void tick();

    // AS2 event callbacks
    std::function<void()> onEnterFrame;
    std::function<void()> onLoad;

    LoadState getLoadState() const { return m_load_state; }
    const std::string& getName() const { return m_name; }

private:
    std::string  m_name;
    int          m_depth;
    int          m_current_frame;
    int          m_total_frames;
    bool         m_playing;
    LoadState    m_load_state;
    uint32_t     m_bytes_loaded;
    uint32_t     m_bytes_total;
    std::string  m_pending_url;

    void processLoad();
    void advanceFrame();
};

} // namespace waflash
